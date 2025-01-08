/***************************************************************************************
* Copyright (c) 2020-2023 Institute of Computing Technology, Chinese Academy of Sciences
* Copyright (c) 2020-2021 Peng Cheng Laboratory
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

#ifndef __TRACE_FASTSIM_H__
#define __TRACE_FASTSIM_H__

#include <cstdio>
#include <cstdint>
#include "trace_common.h"

enum FastSimState {
  FASTSIM_DISABLE,
  FASTSIM_ENABLE,
};

class TraceFastSimManager {
private:
  FastSimState state = FASTSIM_DISABLE;
  uint64_t reverse_interval = -1; // 20M

public:
  TraceFastSimManager(bool fastwarmup_enable) {
    if (fastwarmup_enable) {
      state = FASTSIM_ENABLE;
    }
  }
  FastSimState get_fastsim_state() { return state; }
  void set_fastsim_state(FastSimState new_state) { state = new_state; }
  void set_fastsim_state() { state = FASTSIM_ENABLE; }
  void clear_fastsim_state() { state = FASTSIM_DISABLE; }

  void check_and_change_fastsim_state(uint64_t inst_count, uint64_t cycle_count) {
    static uint64_t last_inst_count = 0;
    static uint64_t last_cycle_count = 0;
    if (inst_count > last_inst_count + reverse_interval) {
      float ipc = 1.0 * (inst_count - last_inst_count) / (cycle_count - last_cycle_count);
      printf("FastSim: reverse state at inst_count %lu, ipc %0.2f ", inst_count, ipc);
      last_inst_count = inst_count;
      last_cycle_count = cycle_count;
      if (state == FASTSIM_DISABLE) {
        printf("from disable to enable\n");
        state = FASTSIM_ENABLE;
      } else {
        printf("from enable to disable\n");
        state = FASTSIM_DISABLE;
      }
      fflush(stdout);
    }
  }
};

#endif // __TRACE_FASTSIM_H__
