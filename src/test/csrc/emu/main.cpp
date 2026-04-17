/***************************************************************************************
* Copyright (c) 2020-2023 Institute of Computing Technology, Chinese Academy of Sciences
* Copyright (c) 2020-2022 Peng Cheng Laboratory
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

#include "common.h"
#include "dut.h"
#include "emu.h"
#include <csignal>
#include <cstdlib>
#include <execinfo.h>
#include <unistd.h>

namespace {

void emu_sigsegv_handler(int sig) {
  void *frames[128];
  const int frame_count = backtrace(frames, 128);
  const char header[] = "\n[EMU_BACKTRACE] caught SIGSEGV\n";
  write(STDERR_FILENO, header, sizeof(header) - 1);
  backtrace_symbols_fd(frames, frame_count, STDERR_FILENO);
  std::_Exit(128 + sig);
}

} // namespace

#ifdef FUZZER_LIB

extern "C" int sim_main(int argc, const char **argv);
int sim_main(int argc, const char *argv[]) {
  optind = 1;

  stats.reset();

#else
int main(int argc, const char *argv[]) {
#endif // FUZZER_LIB
  std::signal(SIGSEGV, emu_sigsegv_handler);
  common_init_without_assertion(argv[0]);

  // initialize the design-under-test (DUT)
  auto emu = new Emulator(argc, argv);

  // allow assertions only after DUT resets
  common_enable_assert();

  // main simulation loop
  while (!emu->is_finished()) {
    emu->tick();
  }
  bool is_good = emu->is_good();
  delete emu;

#ifdef FUZZER_LIB
  stats.accumulate();
#endif
  stats.display();

  common_finish();

#if defined(FUZZING) && !defined(FUZZER_LIB)
  if (!is_good) {
    volatile uint64_t *ptr = 0;
    uint64_t a = *ptr;
  }
  return 0;
#else
#ifdef FUZZER_LIB
  return !is_good || stats.exit_code == SimExitCode::unknown;
#else
  return !is_good;
#endif // FUZZER_LIB
#endif // FUZZING && !FUZZER_LIB
}
