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


#ifdef VERILATOR
#include "emu.h"
#define DUT_MODEL Emulator
#endif

#ifdef DUT_MODEL
int main(int argc, const char *argv[]) {
  common_init(argv[0]);

  // main simulation loop
  auto emu = new DUT_MODEL(argc, argv);
  while (!emu->is_finished()) {
    emu->tick();
  }
  bool is_good = emu->is_good();
  delete emu;

  common_finish();

  return !is_good;
}
#endif // DUT_MODEL
