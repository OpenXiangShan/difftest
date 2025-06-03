set -e  # exit when some command failed
set -o pipefail

./build/fpga-host --diff ready-to-run/riscv64-nemu-interpreter-so -i ready-to-run/microbench.bin &
HOST_PID=$!

sleep 2

./build/simv +workload=./ready-to-run/microbench.bin +e=0 +diff=./ready-to-run/riscv64-nemu-interpreter-so & #> build/try.txt 2>&1 &
SIMV_PID=$!

wait $HOST_PID
HOST_EXIT_CODE=$?

kill -9 $SIMV_PID

exit $HOST_EXIT_CODE
