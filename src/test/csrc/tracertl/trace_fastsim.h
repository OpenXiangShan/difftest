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
#include <vector>
#include <deque>
#include "trace_common.h"
#include "trace_inst_deduper.h"
#include "trace_mem_deduper.h"



struct FastSimMemAddrBuf : public FastSimMemAddr {
  uint8_t valid = 0;
};

class TraceFastSimManager {
private:
  uint64_t warmupInst = 0;

  uint64_t fastsimInstIdx = 0;

  FastSimMemAddrBuf mem_addr_buffer[4];
  std::vector<FastSimMemAddr> mem_addr_list;
  uint64_t cur_mem_addr_idx = 0;
  uint64_t mem_addr_list_size = 0;

  bool fastSimInstFinish = true;
  bool fastSimMemoryFinish = true;


public:
  TraceFastSimManager(bool fastwarmup_enable, uint64_t warmup_inst) {
    printf("TraceFastSimManager: fastwarmup_enable %d\n", fastwarmup_enable);
    fflush(stdout);
    if (fastwarmup_enable) {
      // state = FASTSIM_ENABLE;
      warmupInst = warmup_inst;
      fastSimInstFinish = false;
      fastSimMemoryFinish = false;
    }
  }

  // TraceInstLoopDedup instDedup;
  TraceInstDeduper instDedup;
  TraceMemDeduper memDedup;

  bool isFastSimFinished() { return fastSimInstFinish && fastSimMemoryFinish; };
  bool isFastSimInstFinished() { return fastSimInstFinish; };
  bool isFastSimMemoryFinished() { return fastSimMemoryFinish; };
  void setFastsimInstFinish() { fastSimInstFinish = true; };
  void setFastsimMemoryFinish() { fastSimMemoryFinish = true; };
  bool avoidInstStuck() { return fastSimInstFinish && !fastSimMemoryFinish; };
  bool isFastSimInstByIdx(uint64_t idx) {
    return idx < warmupInst;
  };

  bool get_fastsim_state() { return !isFastSimFinished(); };
  uint64_t getWarmupInstNum() { return warmupInst; }
  uint64_t getSquashedInstNum() { return instDedup.getSquashedInst(); }

  // used by DPI-C
  void prepareMemAddrBuffer();
  void read_mem_addr(uint8_t idx, uint8_t* valid, uint64_t* vaddr, uint64_t* paddr);
  // used by emu-driver
  bool memAddrEmpty() { return cur_mem_addr_idx >= mem_addr_list_size; }

  // mem dedup
  void mergeMemAddr(std::vector<Instruction> inst_list, size_t max_insts) {
    memDedup.mergeMem(mem_addr_list, inst_list, 0, warmupInst, max_insts);
  };
};


#endif // __TRACE_FASTSIM_H__
