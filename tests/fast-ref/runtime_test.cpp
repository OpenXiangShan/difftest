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
#include "difftest.h"
#include "ram.h"
#include "rawfork.h"
#include "rawpool.h"
#include <cassert>
#include <csignal>
#include <cstdio>
#include <fstream>
#include <unistd.h>

RawPacketPool *g_raw_packet_pool = nullptr;
extern "C" void fpga_nstep(uint8_t) {}
class TestProxy : public RefProxy {
public:
  TestProxy() : RefProxy(0, 2 * 1024 * 1024) {}
  static inline void (*original_hash)(void *) = nullptr;
  static inline pid_t parent = 0;
  static inline bool first_hash_only = false;
  static inline unsigned parent_hashes = 0;
  static void wrong_hash(void *p) {
    original_hash(p);
    if (getpid() == parent && (!first_hash_only || parent_hashes++ == 0))
      static_cast<DifftestStateHash *>(p)->state_lo ^= 1;
  }
  void corrupt_parent(bool first_only = false) {
    first_hash_only = first_only;
    parent = getpid();
    original_hash = ref_state_hash;
    ref_state_hash = wrong_hash;
  }
};
class TestDifftest : public Difftest {
public:
  TestDifftest() : Difftest(0) {}
  void corrupt_stamp() {
#ifdef CONFIG_DIFFTEST_SQUASH
    state->commit_stamp ^= 1;
#endif
  }
};

void copy_regs(DiffTestState &dut, RefProxy &proxy) {
#define COPY(field) dut.regs.field = proxy.state.field
  COPY(xrf);
  COPY(csr);
#ifdef CONFIG_DIFFTEST_ARCHFPREGSTATE
  COPY(frf);
#endif
#ifdef CONFIG_DIFFTEST_ARCHVECREGSTATE
  COPY(vrf);
#endif
#ifdef CONFIG_DIFFTEST_HCSRSTATE
  COPY(hcsr);
#endif
#ifdef CONFIG_DIFFTEST_VECCSRSTATE
  COPY(vcsr);
#endif
#ifdef CONFIG_DIFFTEST_FPCSRSTATE
  COPY(fcsr);
#endif
#ifdef CONFIG_DIFFTEST_TRIGGERCSRSTATE
  COPY(triggercsr);
#endif
#undef COPY
}

int main(int argc, char **argv) {
  assert(argc == 3);
  const std::string scenario = argv[2];
  difftest_ref_so = argv[1];
  FIRST_INST_ADDRESS = PMEM_BASE = 0x80000000;
  uint32_t code[] = {0x00000013, 0x00108093, 0x00113023, 0xff9ff06f};
  if (scenario == "stall")
    code[1] = 0x0000006b;
  char image[] = "/tmp/difftest-fast-ref-XXXXXX";
  const int image_fd = mkstemp(image);
  assert(image_fd >= 0);
  close(image_fd);
  std::ofstream file(image, std::ios::binary);
  file.write(reinterpret_cast<char *>(code), sizeof(code));
  file.close();
  simMemory = new MmapMemory(image, 2 * 1024 * 1024, false, 0);
  unlink(image);
  TestDifftest self;
  self.proxy = new TestProxy();
  auto *proxy = static_cast<TestProxy *>(self.proxy);
  proxy->sync();
  DiffTestState input{};
  copy_regs(input, *proxy);
  input.regs.xrf.value[2] = PMEM_BASE + 4096;
  self.dut = &input;
  self.init_checkers();
  Difftest *objects[] = {&self};
  difftest = objects;
  RawPacketPool pool(8, 64);
  g_raw_packet_pool = &pool;
  if (scenario == "hash" || scenario == "prefix")
    proxy->corrupt_parent(scenario == "prefix");
  const bool segmented = scenario == "segments" || scenario == "out-of-order" || scenario == "prefix";
  const unsigned limit = segmented ? 3 : (scenario == "skip" ? 2 : 101);
  for (unsigned window = 0; window < limit; ++window) {
    input.commit[0].valid = 1;
    input.commit[0].pc = PMEM_BASE + (window ? 4 : 0);
    input.commit[0].instr = window ? code[1] : code[0];
    input.commit[0].nFused = window ? 2 : 0;
    if (scenario == "skip" && window == 1) {
      input.commit[0].nFused = 0;
      input.commit[0].skip = 1;
      input.commit[0].rfwen = 1;
      input.commit[0].wdest = 1;
      input.pregs_xrf.value[input.commit[0].wpdest] = 1;
    }
    input.regs.xrf.value[1] = window;
    input.trap.instrCnt = 1 + window * 3;
    input.trap.cycleCnt = input.trap.instrCnt;
    const int ret = self.step();
    if (ret != DiffTestChecker::STATE_OK) {
      if (difftest_raw_fork_is_child())
        difftest_raw_fork_abort_child();
      pool.fail();
      difftest_raw_fork_finish();
      printf("TEST_ERROR ret=%d window=%u\n", ret, window);
      return 2;
    }
    if (scenario == "hang" && window == 0 && difftest_raw_fork_is_child())
      raise(SIGSTOP);
    if (scenario == "killed" && window == 0 && difftest_raw_fork_is_child())
      _exit(7);
    if (scenario == "stamp" && window == limit - 1 && !difftest_raw_fork_is_child())
      self.corrupt_stamp();
    if (segmented && window < limit - 1) {
      if ((scenario == "out-of-order" || scenario == "prefix") && window == 0 && difftest_raw_fork_is_child())
        usleep(5000000);
      usleep(3100000);
    }
  }
  if (difftest_raw_fork_is_child()) {
    while (true) {
      difftest_raw_fork_idle();
      usleep(50);
    }
  }
  const int ret = difftest_raw_fork_finish();
#ifdef CONFIG_DIFFTEST_FAST_REF
  if (getenv("DIFFTEST_FAST_ONLY") || difftest_raw_fork_enabled()) {
    if (!ret && proxy->get_instr_count() != (scenario == "skip" ? 1 : 1 + (limit - 1) * 3))
      return 3;
  }
#endif
  printf("TEST_RESULT ret=%d windows=%u\n", ret, limit);
  fflush(stdout);
  return ret;
}
