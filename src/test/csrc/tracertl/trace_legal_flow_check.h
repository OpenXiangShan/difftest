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
#ifndef __TRACE_LEGAL_FLOW_CHECK_H__
#define __TRACE_LEGAL_FLOW_CHECK_H__

#include <vector>
#include <cstdlib>
#include "trace_format.h"

class TraceLegalFlowChecker {
private:
  inline bool isRVC(uint32_t encode) {
    return (encode & 0x3) != 0x3;
  }
  inline uint64_t getNextPC(Instruction inst) {
    if ((inst.branch_type != BRANCH_None && inst.branch_taken) || inst.isTrap()) {
      return inst.target;
    }
    return inst.instr_pc_va + (isRVC(inst.instr) ? 2 : 4);
  }

public:
  uint64_t check(const std::vector<Instruction> &flow) {
    bool init = false;
    TraceInstruction lastInst;
    uint64_t wantted_pc;
    uint64_t index = 0;
    for (auto item : flow) {
      index ++;
      // if (item.fast_simulation) continue;
      if (item.is_squashed) continue;

      if (!init) {
        wantted_pc = getNextPC(item);
        init = true;
      } else {
        if (wantted_pc != item.instr_pc_va) {
          printf("Found illegal flow:\n");
          printf("%ld: ", index-1);
          fflush(stdout);
          lastInst.dump();
          printf("%ld: ", index);
          fflush(stdout);
          item.dump();

          return index;
          // exit(1);
        } else {
          wantted_pc = getNextPC(item);
        }
      }
      lastInst = item;
    }
    return 0;
  }
};

#endif