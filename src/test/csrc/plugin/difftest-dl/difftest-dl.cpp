/***************************************************************************************
* Copyright (c) 2020-2021 Institute of Computing Technology, Chinese Academy of Sciences
* Copyright (c) 2020-2021 Peng Cheng Laboratory
*
* XiangShan is licensed under Mulan PSL v2.
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

#include "difftest-dl.h"

#ifdef VERILATOR

Emulator* global_emu = NULL;

// use `extern "C"` to avoid func name mangle
extern "C" int run_emu(int argc, const char** argv) {
  printf("Emu compiled at %s, %s\n", __DATE__, __TIME__);
  global_emu = new Emulator(argc, argv);
  auto args = global_emu->get_args();
  uint64_t cycles = global_emu->execute(args.max_cycles, args.max_instr);
  bool is_good_trap = global_emu->is_good_trap();
  delete global_emu;
  eprintf(ANSI_COLOR_BLUE "Seed=%d Guest cycle spent: %'" PRIu64
      " (this will be different from cycleCnt if emu loads a snapshot)\n" ANSI_COLOR_RESET, args.seed, cycles);
  return !is_good_trap;
}

extern "C" int core_emu_init(int argc, const char** argv) {
  global_emu = new Emulator(argc, argv);
  // __attribute__(unused) auto args = global_emu->get_args();
  global_emu->execute_init();
  return 0;
}

extern "C" bool core_emu_finished() {
  return global_emu->is_finished();
}

extern "C" int core_emu_single_step() {
  if(!core_emu_finished())
    return global_emu->execute_single_cycle();
  else
    return -1;
}

extern "C" int core_emu_n_step(int n) {
  for (int i = 0; i < n; i++) {
    auto ret = core_emu_single_step();
    if (ret) return ret;
  }
  return 0;
}

extern "C" void core_emu_cleanup() {
  global_emu->execute_cleanup();
  global_emu->display_trapinfo();
}

#endif