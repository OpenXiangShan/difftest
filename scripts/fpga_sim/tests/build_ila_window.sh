set -euo pipefail

DIFFTEST_ROOT="$(cd "$(dirname "$0")/../../.." && pwd)"
OUTPUT="$(realpath -m "${1:?output directory required}")"
CONFIG="${2:-ESBIFU}"
mkdir -p "$OUTPUT"

cd "$DIFFTEST_ROOT"
NOOP_HOME="$OUTPUT" mill --no-server difftest.test.runMain difftest.IlaWindowTest "$CONFIG" \
  "$OUTPUT/build/split-rtl" >"$OUTPUT/elaborate.log" 2>&1
make fpga-build FPGA=1 FPGA_SIM=1 DIFFTEST_FAST_REF=1 DIFFTEST_FORK_REF=1 USE_THREAD_MEMPOOL=1 \
  DIFFTEST_PERFCNT=1 IMAGE_GZ_COMPRESS=0 NO_ZSTD_COMPRESSION=1 IMAGE_ELF=0 \
  NOOP_HOME="$OUTPUT" DESIGN_DIR="$OUTPUT" >"$OUTPUT/host-build.log" 2>&1
verilator --binary --timing --assert -Wno-fatal --top-module IlaWindowSimulation -j 4 \
  --Mdir "$OUTPUT/obj" -I"$OUTPUT/build/generated-src" -y "$OUTPUT/build/split-rtl" \
  "$OUTPUT/build/split-rtl/SimTop.sv" scripts/fpga_sim/tests/ila_window_test.sv \
  src/test/vsrc/fpga_sim/xdma_wrapper.v src/test/vsrc/fpga_sim/xdma_axilite.v \
  src/test/vsrc/fpga_sim/xdma_axi_c2h.v src/test/vsrc/fpga_sim/xdma_axi_h2c.v \
  src/test/vsrc/common/DifftestClockGate.v "$DIFFTEST_ROOT/src/test/csrc/fpga_sim/xdma_sim.cpp" \
  "$DIFFTEST_ROOT/scripts/fpga_sim/tests/ila_window_test.cpp" \
  -CFLAGS "-I$OUTPUT/build/generated-src -I$DIFFTEST_ROOT/src/test/csrc/fpga_sim -pthread" \
  -LDFLAGS '-pthread' >"$OUTPUT/sim-build.log" 2>&1
g++ -std=c++20 -DFPGA_HOST -I"$OUTPUT/build/generated-src" -I"$DIFFTEST_ROOT/src/test/csrc/fpga_sim" \
  scripts/fpga_sim/tests/ila_window_control.cpp src/test/csrc/fpga_sim/xdma_sim.cpp \
  -pthread -o "$OUTPUT/control-test"
printf 'Built ILA window validation: %s (%s)\n' "$OUTPUT" "$CONFIG"
