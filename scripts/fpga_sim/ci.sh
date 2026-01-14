set -e  # exit when some command failed
set -o pipefail

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

./build/fpga-host --diff ready-to-run/riscv64-nemu-interpreter-so -i ready-to-run/microbench.bin &
HOST_PID=$!

sleep 2

./build/simv +workload=./ready-to-run/microbench.bin +e=0 +diff=./ready-to-run/riscv64-nemu-interpreter-so & #> build/try.txt 2>&1 &
SIMV_PID=$!

set +e # disable exit to get exitCode
wait $HOST_PID
HOST_EXIT_CODE=$?
set -e

kill -9 $SIMV_PID

exit $HOST_EXIT_CODE
