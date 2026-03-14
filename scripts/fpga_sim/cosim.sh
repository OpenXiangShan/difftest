set -e  # exit when some command failed
set -o pipefail

WORKLOAD=""
DIFF=""
WAVE=0

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

./build/fpga-host --diff "${DIFF}" -i "${WORKLOAD}" &
HOST_PID=$!

sleep 2

WAVE_ARGS=""
if [[ "${WAVE}" -eq 1 ]]; then
  WAVE_ARGS="+dump-wave"
fi

./build/simv +workload="${WORKLOAD}" +e=0 +diff="${DIFF}" ${WAVE_ARGS} &
SIMV_PID=$!

set +e # disable exit to get exitCode
wait $HOST_PID
HOST_EXIT_CODE=$?
set -e

kill -9 $SIMV_PID

exit $HOST_EXIT_CODE
