/***************************************************************************************
* Copyright (c) 2025 Beijing Institute of Open Source Chip (BOSC)
* Copyright (c) 2020-2025 Institute of Computing Technology, Chinese Academy of Sciences
*
* DiffTest is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
***************************************************************************************/
#include "rawfork.h"
#include "xdma.h"
#include <cassert>
#include <cstring>
#include <string>

int signal_num = 0;
int pipe_fds[2];
std::atomic<unsigned> callbacks{0};
FpgaXdma *device;
extern "C" int __wrap_open(const char *path, int, ...) {
  assert(strcmp(path, "/dev/xdma0_c2h_0") == 0);
  return pipe_fds[0];
}
extern "C" void v_difftest_Batch(uint8_t *data) {
  const unsigned n = callbacks.fetch_add(1);
  assert(data[0] == n);
}
bool difftest_raw_fork_enabled() {
  return false;
}
bool difftest_raw_fork_is_child() {
  return false;
}
int difftest_raw_fork_idle() {
  return 0;
}
void difftest_raw_fork_abort_child() {}
void fpga_raw_fork_abort() {
  device->stop();
}
int main(int argc, char **argv) {
  assert(argc == 2);
  const bool truncated = std::string(argv[1]) == "truncated";
  setenv("DIFFTEST_SHARED_RAW", "1", 1);
  setenv("DIFFTEST_RAW_POOL_PACKETS", "8", 1);
  assert(pipe(pipe_fds) == 0);
  FpgaXdma xdma;
  device = &xdma;
  std::thread host([&] { xdma.start(true); });
  FpgaPackgeHead packet{};
  for (unsigned p = 0; p < 2; ++p) {
    packet.diff_packge[0].packge_idx = p;
    for (unsigned i = 0; i < DMA_PACKGE_NUM; ++i)
      packet.diff_packge[i].diff_packge[0] = p * DMA_PACKGE_NUM + i;
    assert(write(pipe_fds[1], &packet, 100) == 100);
    usleep(20000);
    assert(callbacks == p * DMA_PACKGE_NUM);
    if (truncated) {
      close(pipe_fds[1]);
      host.join();
      assert(callbacks == 0 && g_raw_packet_pool->aborted());
      return 0;
    }
    assert(write(pipe_fds[1], reinterpret_cast<char *>(&packet) + 100, sizeof(packet) - 100) == sizeof(packet) - 100);
    for (unsigned retry = 0; callbacks < (p + 1) * DMA_PACKGE_NUM; ++retry) {
      assert(retry < 1000);
      usleep(1000);
    }
  }
  // Receiver is blocked on an empty pipe with its writer still open.
  usleep(20000);
  device->stop();
  host.join();
  close(pipe_fds[1]);
  assert(callbacks == 2 * DMA_PACKGE_NUM);
  puts("XDMA partial reads, complete packet publication and blocked-read shutdown: PASS");
}
