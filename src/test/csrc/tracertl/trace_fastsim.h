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

struct FastSimMemAddr {
  uint64_t vaddr;
  uint64_t paddr;
};

struct FastSimMemAddrBuf : public FastSimMemAddr {
  uint8_t valid = 0;
};

class TraceFastSimManager {
private:
  // FastSimState state = FASTSIM_DISABLE;
  uint64_t warmupInst = 0;

  uint64_t fastsimInstIdx = 0;

  FastSimMemAddrBuf mem_addr_buffer[4];
  // std::deque<FastSimMemAddr> mem_addr_list;
  std::vector<FastSimMemAddr> mem_addr_list;
  std::vector<FastSimMemAddr> mem_addr_list_before_merge;
  uint64_t cur_mem_addr_idx = 0;
  uint64_t mem_addr_list_size = 0;

  bool addrSameBlock(uint64_t addr1, uint64_t addr2);

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

  bool isFastSimFinished() { return fastSimInstFinish && fastSimMemoryFinish; };
  bool isFastSimInstFinished() { return fastSimInstFinish; };
  bool isFastSimMemoryFinished() { return fastSimMemoryFinish; };
  void setFastsimInstFinish() { fastSimInstFinish = true; };
  void setFastsimMemoryFinish() { fastSimMemoryFinish = true; };
  bool avoidInstStuck() { return fastSimInstFinish && !fastSimMemoryFinish; };

  bool get_fastsim_state() { return !isFastSimFinished(); };

  void prepareMemAddrBuffer();
  void read_mem_addr(uint8_t idx, uint8_t* valid, uint64_t* vaddr, uint64_t* paddr);
  void preCollectFastSimInst() { fastsimInstIdx++; };
  bool preCollectEnoughFastSimInst() { return fastsimInstIdx >= warmupInst; };
  void addMemAddr(uint64_t vaddr, uint64_t paddr);
  void mergeMemAddr();

  bool memAddrEmpty() { return cur_mem_addr_idx >= mem_addr_list_size; }
};


#endif // __TRACE_FASTSIM_H__
