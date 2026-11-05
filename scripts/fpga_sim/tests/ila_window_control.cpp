#include "difftest-state.h"
#include "xdma_sim.h"
#include <cassert>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <thread>

static void write_register(uint32_t address, uint32_t value, uint8_t strobe = 15) {
  assert(xdma_sim_axilite_write(address, value, strobe) == 0);
}

static uint32_t read_register(uint32_t address) {
  uint32_t value = 0;
  assert(xdma_sim_axilite_read(address, &value) == 0);
  return value;
}

static void wait_for_stall() {
  for (unsigned attempt = 0; attempt < 1000; ++attempt) {
    if (read_register(0x48) & 1)
      return;
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
  assert(false);
}

int main(int argc, char **argv) {
  const uint64_t base = argc > 1 ? std::strtoull(argv[1], nullptr, 10) : 0;
  xdma_sim_open(0, true);
  xdma_sim_h2c_open(0, true);
  xdma_sim_axilite_open(true);
  xdma_sim_workload_open(true);
  std::thread([] {
    constexpr unsigned packet_bytes = (CONFIG_DIFFTEST_BATCH_BYTELEN + CONFIG_DIFFTEST_HOST_AXIS_BYTES) /
                                      CONFIG_DIFFTEST_HOST_AXIS_BYTES * CONFIG_DIFFTEST_HOST_AXIS_BYTES;
    char buffer[8 * packet_bytes];
    while (xdma_sim_read(0, buffer, sizeof(buffer)) > 0) {}
  }).detach();
  puts("ILA_CONTROL_READY");
  fflush(stdout);
  assert((read_register(0x48) & ~1u) == 0x494c4102u);
  assert(read_register(0x34) == 0 && read_register(0x38) == 0);
  assert(read_register(0x3c) == 0 && read_register(0x40) == 0);
  write_register(0x04, 1);
  write_register(0x08, 1);
  write_register(0x34, static_cast<uint32_t>(base));
  write_register(0x38, static_cast<uint32_t>(base >> 32));
  write_register(0x3c, 7);
  write_register(0x40, 0);
  write_register(0x44, 1);
  write_register(0x04, 0);
  wait_for_stall();
  write_register(0x34, 0xffffffff);
  write_register(0x38, 1);
  assert(read_register(0x48) & 1);
  write_register(0x44, 1, 0);
  assert(read_register(0x48) & 1);
  write_register(0x34, 0x12345678, 1);
  assert(read_register(0x34) == 0xffffff78);
  write_register(0x34, 0);
  write_register(0x38, 0);
  write_register(0x3c, 0);
  assert(read_register(0x48) & 1);
  write_register(0x44, 1);
  assert(!(read_register(0x48) & 1));
  write_register(0x04, 1);
  write_register(0x34, static_cast<uint32_t>(base));
  write_register(0x38, static_cast<uint32_t>(base >> 32));
  write_register(0x3c, 9);
  write_register(0x44, 1);
  write_register(0x04, 0);
  wait_for_stall();
  write_register(0x08, 0);
  assert(!(read_register(0x48) & 1));
  puts("PASS atomic BAR commit, WSTRB, size-zero unlock, reset and link bypass");
}
