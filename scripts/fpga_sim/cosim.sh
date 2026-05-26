set -e  # exit when some command failed
set -o pipefail

WORKLOAD=""
DIFF=""
WAVE=0
RAM_SIZE=""
SEED=""
RANDOM_MEM=0

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

cleanup() {
  echo "[CLEANUP] kill fpga-host and simv..."
  trap - INT TERM EXIT
  if [[ -n "$SIMV_PID" ]]; then
    kill -INT "$SIMV_PID" 2>/dev/null || true
  fi

  if [[ -n "$HOST_PID" ]]; then
    kill -INT "$HOST_PID" 2>/dev/null || true
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

./build/fpga-host "${HOST_ARGS[@]}" &
HOST_PID=$!

sleep 2

SIMV_ARGS=(+e=0 +no-diff)
if [[ -n "${RAM_SIZE}" ]]; then
  SIMV_ARGS+=(+ram_size="${RAM_SIZE}")
fi
if [[ "${WAVE}" -eq 1 ]]; then
  SIMV_ARGS+=(+dump-wave)
fi

./build/simv "${SIMV_ARGS[@]}" &
SIMV_PID=$!

set +e # disable exit to get exitCode
wait $HOST_PID
HOST_EXIT_CODE=$?
set -e

kill -9 $SIMV_PID

exit $HOST_EXIT_CODE
