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

#ifndef __TRACE_PADDR_ALLOCATOR_H__
#define __TRACE_PADDR_ALLOCATOR_H__

#include <cstdint>
#include <map>
#include "trace_common.h"

template<uint64_t baseAddr>
class TracePAddrAllocator {

private:
  uint64_t curAddr = baseAddr;
  std::map<uint64_t, uint64_t> v2pMap;

  inline uint64_t mergePageAndOff(uint64_t pn, uint64_t fullAddr) {
    return (pn << TRACE_PAGE_SHIFT) | (fullAddr & TRACE_PAGE_OFFSET_MASK);
  }

  uint64_t pop() {
    uint64_t last = curAddr;
    curAddr += TRACE_PAGE_SIZE;
    return last;
  };

public:
  uint64_t va2pa(uint64_t va) {
    uint64_t vpn = va >> TRACE_PAGE_SHIFT;
    uint64_t ppn;
    if (v2pMap.find(vpn) != v2pMap.end()) {
      ppn = v2pMap[vpn];
    } else {
      ppn = pop() >> TRACE_PAGE_SHIFT;
      v2pMap[vpn] = ppn;
    }
    return mergePageAndOff(ppn, va);
  }

  void dump() {
    printf("Trace PAddr Allocator V2P Map:\n");
    for (auto pair : v2pMap) {
      printf("  vpn %016lx -> ppn %016lx\n", pair.first, pair.second);
    }
  }
};

#endif