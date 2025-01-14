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

#ifndef __TRACE_DYNAMIC_PAGE_TABLE_H__

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <unordered_map>
#include "trace_format.h"
#include "trace_common.h"

typedef union TracePageTableEntry {
  struct  {
    uint8_t v : 1;
    uint8_t r : 1;
    uint8_t w : 1;
    uint8_t x : 1;
    uint8_t u : 1;
    uint8_t g : 1;
    uint8_t a : 1;
    uint8_t d : 1;
    uint8_t rsw : 2;
    uint64_t ppn : 44;
    uint8_t reserved : 7;
    uint8_t pbmt : 2;
    uint8_t n : 1;
  };
  uint64_t val;
} TracePTE;

struct TracePageEntry {
  uint64_t paddr;
  uint64_t pte;
  uint64_t level; // use uint64_t for align
};

#define TraceVPNi(vpn, i)        (((vpn) >> (9 * (i))) & 0x1ff)

class DynamicSoftPageTable {
private:
  // const uint64_t TRACE_PAGE_SIZE = 4096;
  const int initLevel = TRACE_MAX_PAGE_LEVEL-1;

  std::map<uint64_t, TracePTE> pageTable;
  // set the baseAddr to maxSize of paddr from trace.
  uint64_t baseAddr = DYN_PAGE_TABLE_BASE_PADDR + TRACE_PAGE_SIZE * initLevel; // 4KB aligned
  uint64_t curAddr = baseAddr  + TRACE_PAGE_SIZE; // 4KB aligned

  std::map<uint64_t, uint64_t> soft_tlb;
  // the page frame's level
  std::map<uint64_t, uint8_t> page_level_map;


public:
  DynamicSoftPageTable() {
    // if (sizeof(TracePTE) != 8) {
    //   printf("TracePTE size is not 8 bytes, is %lu\n", sizeof(TracePTE));
    //   // sizeof(TracePTE) is 16.
    //   exit(1);
    // }
    for (int i = 0; i < initLevel; i++) {
      page_level_map[getDummyLevelNPage(i) >> 12] = i;
    }
  };
  void setBaseAddr(uint64_t addr) {
    if (addr % TRACE_PAGE_SIZE != 0) {
      printf("setBaseAddr: addr 0x%lx is not 4KB aligned\n", addr);
      exit(1);
    }
    baseAddr = addr;
    // curAddr = addr;
  }

  // return pageTable as ddr ram, return value
  // if not found, return 0
  uint64_t read(uint64_t paddr, bool construct);
  // assign a page entry for vaddr to paddr
  void write(uint64_t vpn, uint64_t ppn);
  // translate vpn to ppn, for debug
  uint64_t trans(uint64_t vpn);
  void setPte(uint64_t pteAddr, uint64_t pte, uint8_t level);

  bool exists(uint64_t vpn) {
    return soft_tlb.find(vpn) != soft_tlb.end();
  }
  void record(uint64_t vpn, uint64_t ppn) {
    soft_tlb[vpn] = ppn;
  }

  uint64_t popCurAddr() {
    uint64_t ret = curAddr;
    curAddr += TRACE_PAGE_SIZE;
    return ret;
  }

  inline uint64_t getPteAddr(uint64_t vpn, uint8_t level, uint64_t baseAddr);
  inline TracePTE genLeafPte(uint64_t ppn);
  inline TracePTE genNonLeafPte(uint64_t ppn);
  // uint64_t entryAlign(uint64_t paddr) { return (paddr >> 3) << 3; }
  // uint64_t pageAlign(uint64_t paddr) { return (paddr >> 12) << 12; }

  void dump();
  void dumpInnerSoftTLB();

private:

  // dummy page for out of trace vpn
  inline uint64_t getDummyLevelNPage(uint8_t level) {
    return DYN_PAGE_TABLE_BASE_PADDR + level * TRACE_PAGE_SIZE;
  }

  inline TracePTE getDummyPte(uint8_t level);
};

#endif // __TRACE_DYNAMIC_PAGE_TABLE_H__
