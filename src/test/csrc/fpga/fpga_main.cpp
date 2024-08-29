/***************************************************************************************
* Copyright (c) 2024 Beijing Institute of Open Source Chip (BOSC)
* Copyright (c) 2020-2024 Institute of Computing Technology, Chinese Academy of Sciences
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
#include "diffstate.h"
#include "mpool.h"
#include "xdma.h"

#define XDMA_C2H_DEVICE "/dev/xdma0_c2h_0"

enum {
  SIMV_RUN,
  SIMV_DONE,
  SIMV_FAIL,
} simv_state;

static uint8_t simv_result = SIMV_RUN;
static uint64_t max_instrs = 0;

struct core_end_info_t {
  bool core_trap[NUM_CORES];
  double core_cpi[NUM_CORES];
  uint8_t core_trap_num;
};
static core_end_info_t core_end_info;

void simv_init();
void simv_step();
void cpu_endtime_check();
void set_dut_from_xdma();

FpgaXdma *xdma_device = NULL; 

int main(int argc, char *argv[]) {

  simv_init();

  while (simv_result == SIMV_RUN) {
    // get xdma data
    set_dut_from_xdma();

    // run difftest
    simv_step();
    cpu_endtime_check();
  }
}

void set_dut_from_xdma() {
  {
    std::unique_lock<std::mutex> lock(xdma_device->diff_mtx);
    xdma_device->diff_filled_cv.wait(lock, [] { return xdma_device->diff_packge_filled; });
    for (int i = 0; i < NUM_CORES; i++) {

      difftest[i]->dut = &xdma_device->difftest_pack[i];
    }
    xdma_device->diff_packge_filled = false;
    xdma_device->diff_empile_cv.notify_one();
  }
}

void simv_init() {
  xdma_device = new FpgaXdma(XDMA_C2H_DEVICE);
  difftest_init();
  max_instrs = 40000000;
}

void simv_step() {
  if (difftest_step())
    simv_result = SIMV_FAIL;
}

void cpu_endtime_check() {
  if (max_instrs != 0) { // 0 for no limit
    for (int i = 0; i < NUM_CORES; i++) {
      if (core_end_info.core_trap[i])
        continue;
      auto trap = difftest[i]->get_trap_event();
      if (max_instrs < trap->instrCnt) {
        core_end_info.core_trap[i] = true;
        core_end_info.core_trap_num++;
        eprintf(ANSI_COLOR_GREEN "EXCEEDED CORE-%d MAX INSTR: %ld\n" ANSI_COLOR_RESET, i, max_instrs);
        difftest[i]->display_stats();
        core_end_info.core_cpi[i] = (double)trap->cycleCnt / (double)trap->instrCnt;
        if (core_end_info.core_trap_num == NUM_CORES) {
          simv_result = SIMV_DONE;
        }
      }
    }
  }
}
