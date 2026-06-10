set -e  # exit when some command failed
set -o pipefail

WORKLOAD=""
DIFF=""
WAVE=0
RAM_SIZE=""
SEED=""
RANDOM_MEM=0
SPLITVIEW="${SPLITVIEW:-0}"
SPLITVIEW_LOG="${SPLITVIEW_LOG:-}"

for arg in "$@"; do
  case "${arg}" in
    WORKLOAD=*)
      WORKLOAD="${arg#WORKLOAD=}"
      ;;
    DIFF=*)
      DIFF="${arg#DIFF=}"
      ;;
    WAVE=*)
      WAVE="${arg#WAVE=}"
      ;;
    RAM_SIZE=*)
      RAM_SIZE="${arg#RAM_SIZE=}"
      ;;
    SEED=*)
      SEED="${arg#SEED=}"
      ;;
    RANDOM_MEM=*)
      RANDOM_MEM="${arg#RANDOM_MEM=}"
      ;;
    SPLITVIEW=*)
      SPLITVIEW="${arg#SPLITVIEW=}"
      ;;
    SPLITVIEW_LOG=*)
      SPLITVIEW_LOG="${arg#SPLITVIEW_LOG=}"
      ;;
    *)
      echo "Unknown argument: ${arg}" >&2
      exit 1
      ;;
  esac
done

: "${WORKLOAD:?WORKLOAD is required}"
: "${DIFF:?DIFF is required}"

HOST_PID=""
SIMV_PID=""
UART_FIFO=""
UART_FIFO_DIR=""
SIMV_LAUNCHER_PID=""

cleanup() {
  echo "[CLEANUP] kill fpga-host and simv..."
  trap - INT TERM EXIT
  if [[ -n "$SIMV_PID" ]]; then
    kill -INT "$SIMV_PID" 2>/dev/null || true
  fi

  if [[ -n "$HOST_PID" ]]; then
    kill -INT "$HOST_PID" 2>/dev/null || true
  fi
  if [[ -n "$SIMV_LAUNCHER_PID" ]]; then
    kill "$SIMV_LAUNCHER_PID" 2>/dev/null || true
  fi
  if [[ -n "$UART_FIFO_DIR" ]]; then
    rm -rf "$UART_FIFO_DIR"
  fi
}

trap cleanup INT TERM EXIT

HOST_ARGS=(--diff "${DIFF}" -i "${WORKLOAD}")
if [[ -n "${RAM_SIZE}" ]]; then
  HOST_ARGS+=(--ram-size="${RAM_SIZE}")
fi
if [[ -n "${SEED}" ]]; then
  HOST_ARGS+=(--seed="${SEED}")
fi
if [[ "${RANDOM_MEM}" -eq 1 ]]; then
  HOST_ARGS+=(--random-mem)
fi

HOST_ENV=()
if [[ "${SPLITVIEW}" -eq 1 ]]; then
  UART_FIFO_DIR="$(mktemp -d /tmp/difftest-fpgasim.XXXXXX)"
  UART_FIFO="$UART_FIFO_DIR/uart"
  mkfifo "$UART_FIFO"
  HOST_ARGS+=(--splitview-log="${SPLITVIEW_LOG:-build/logs/fpga-sim-splitview}")
  HOST_ENV+=(DIFFTEST_SPLITVIEW_UART_INPUT="$UART_FIFO")
fi

SIMV_ARGS=(+e=0 +no-diff)
if [[ -n "${RAM_SIZE}" ]]; then
  SIMV_ARGS+=(+ram_size="${RAM_SIZE}")
fi
if [[ "${WAVE}" -eq 1 ]]; then
  SIMV_ARGS+=(+dump-wave)
fi

if [[ -n "$UART_FIFO" ]]; then
  (
    sleep 2
    ./build/simv "${SIMV_ARGS[@]}" >"$UART_FIFO" 2>&1 &
    echo "$!" >"$UART_FIFO_DIR/simv.pid"
    wait "$!"
  ) &
  SIMV_LAUNCHER_PID=$!

  set +e # disable exit to get exitCode
  env "${HOST_ENV[@]}" ./build/fpga-host "${HOST_ARGS[@]}"
  HOST_EXIT_CODE=$?
  set -e

  if [[ -s "$UART_FIFO_DIR/simv.pid" ]]; then
    SIMV_PID="$(cat "$UART_FIFO_DIR/simv.pid")"
    kill -9 "$SIMV_PID" 2>/dev/null || true
  fi
  kill "$SIMV_LAUNCHER_PID" 2>/dev/null || true
  wait "$SIMV_LAUNCHER_PID" 2>/dev/null || true
  exit $HOST_EXIT_CODE
fi

./build/fpga-host "${HOST_ARGS[@]}" &
HOST_PID=$!

sleep 2

./build/simv "${SIMV_ARGS[@]}" &
SIMV_PID=$!

set +e # disable exit to get exitCode
wait $HOST_PID
HOST_EXIT_CODE=$?
set -e

kill -9 $SIMV_PID

exit $HOST_EXIT_CODE
