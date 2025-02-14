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

#include "trace_fastsim.h"
#include <unordered_set>

void TraceFastSimManager::prepareMemAddrBuffer() {
  if (cur_mem_addr_idx >= mem_addr_list_size) {
    // finished
    if (!isFastSimMemoryFinished()) {
      printf("Set FastSim Memory Finish\n");
      setFastsimMemoryFinish();
    }
    return;
  }
  if (mem_addr_buffer[0].valid) {
    // not consumed
    return;
  }
  for (int i = 0; i < 4; i++) {
    if (cur_mem_addr_idx < mem_addr_list_size) {
      mem_addr_buffer[i].vaddr = mem_addr_list[cur_mem_addr_idx].vaddr;
      mem_addr_buffer[i].paddr = mem_addr_list[cur_mem_addr_idx].paddr;
      mem_addr_buffer[i].valid = true;
      cur_mem_addr_idx ++;
    } else {
      mem_addr_buffer[i].valid = 0;
      // finished
      if (!isFastSimMemoryFinished()) {
        printf("Set FastSim Memory Finish\n");
        setFastsimMemoryFinish();
      }
    }
  }
}

void TraceFastSimManager::read_mem_addr(
  uint8_t idx,
  uint8_t* valid,
  uint64_t* vaddr, uint64_t* paddr) {
  if (idx >= 4) {
    printf("TraceFastSimManager: idx %d >= 4\n", idx);
    fflush(stdout);
    return;
  }
  *valid = mem_addr_buffer[idx].valid;
  *vaddr = mem_addr_buffer[idx].vaddr;
  *paddr = mem_addr_buffer[idx].paddr;
  mem_addr_buffer[idx].valid = 0;
}

void TraceFastSimManager::addMemAddr(uint64_t vaddr, uint64_t paddr) {
  FastSimMemAddr mem_addr;
  mem_addr.vaddr = vaddr;
  mem_addr.paddr = paddr;
  mem_addr_list_before_merge.push_back(mem_addr);
}

bool TraceFastSimManager::addrSameBlock(uint64_t addr1, uint64_t addr2) {
  // blockBits = log2 (512 / 8);
  return (addr1 >> 6) == (addr2 >> 6);
}

void TraceFastSimManager::mergeMemAddr() {
  std::unordered_set<uint64_t> mem_addr_set;
  for (auto &mem_addr : std::vector<FastSimMemAddr>(mem_addr_list_before_merge.rbegin(), mem_addr_list_before_merge.rend())) {
    if (mem_addr_set.find(mem_addr.vaddr >> 6) == mem_addr_set.end()) {
      mem_addr_set.insert(mem_addr.vaddr >> 6);
      mem_addr_list.insert(mem_addr_list.begin(), mem_addr);
      // printf("TraceFastSimManager.mergeMemAddr push %lx\n", mem_addr.vaddr);
    } else {
      // printf("TraceFastSimManager.mergeMemAddr filter %lx\n", mem_addr.vaddr);
    }
  }

  mem_addr_list_size = mem_addr_list.size();
  printf("TraceFastSimManager: mergeMemAddr %lu ->(filter) %lu\n", mem_addr_list_before_merge.size(), mem_addr_list.size());
  // for (auto &mem_addr : mem_addr_list) {
  //   printf("%lx ", mem_addr.vaddr);
  // }
  // printf("\n");
}

