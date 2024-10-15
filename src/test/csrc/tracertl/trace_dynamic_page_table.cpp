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

#include <iostream>
#include "trace_dynamic_page_table.h"

uint64_t DynamicSoftPageTable::read(uint64_t paddr, bool construct) {
  if (pageTable.find(paddr) != pageTable.end()) {
    // printf("  read pte paddr: %lx, val: %lx\n", paddr, pageTable[paddr].val);
    return pageTable[paddr].val;
  } else {
    if (construct) {
      return 0;
    }

    uint64_t ppn = paddr >> 12;
    // printf("  read pte paddr: %lx. not found, return 0\n", paddr);
    if (page_level_map.find(ppn) != page_level_map.end()) {
      uint8_t level = page_level_map[ppn];
      TracePTE dummyPte = getDummyPte(level);
      return dummyPte.val;
    } else {
      printf("DSPT: read paddr's level not found. paddr %lx", paddr);
      exit(1);
      return 0;
    }
  }
}

void DynamicSoftPageTable::write(uint64_t vpn, uint64_t ppn) {
  if (exists(vpn)) { return; }
  record(vpn, ppn);

  uint64_t pgBase = baseAddr;
  static int count = 0;
  // printf("%d write vpn: %lx, ppn: %lx\n", count++, vpn, ppn);
  for (int level = initLevel; level >= 0; level--) {
    uint64_t pteAddr = getPteAddr(vpn, level, pgBase);
    // printf("  level: %d, pteAddr: %lx pgBase: %lx\n", level, pteAddr, pgBase);
    uint64_t pteVal = read(pteAddr, true);
    TracePTE pte;
    pte.val = pteVal;
    // printf("    pte: %lx\n", pte.val);
    if (!pte.v) {
      if (pte.val != 0) {
        printf("Error: invalid pte should be 0. pte: %lx\n", pte.val);
        exit(1);
      }
      // not exist, create
      pte = (level == 0) ? genLeafPte(ppn) : genNonLeafPte(popCurAddr() >> 12);
      // printf("Insert level: %d, pteAddr: %lx, pte: %lx(ppn:%lx)\n", level, pteAddr, pte.val, pte.ppn);
      page_level_map[pteAddr >> 12] = level;
      pageTable[pteAddr] = pte;
    }
    pgBase = pte.ppn << 12;
  }
}

void DynamicSoftPageTable::setPte(uint64_t pteAddr, uint64_t pte_val, uint8_t level) {
  if (level > initLevel) {
    printf("Error: level %d should not be greater than initLevel\n", level);
    printf("  initLevel: %d pteAddr: %lx pte_val: %lx\n", initLevel, pteAddr, pte_val);
    exit(1);
  }
  TracePTE pte;
  pte.val = pte_val;
  page_level_map[pteAddr >> 12] = level;
  pageTable[pteAddr] = pte;
}

uint64_t DynamicSoftPageTable::trans(uint64_t vpn) {
  uint64_t pgBase = baseAddr;
  int level = initLevel;
  static int count = 0;
  // printf("%d trans pgBase 0x%lx vpn: %lx\n", count++, pgBase, vpn);
  for (; level >= 0; level--) {
    uint64_t pteAddr = getPteAddr(vpn, level, pgBase);
    uint64_t pteVal = read(pteAddr, false);
    TracePTE pte;
    pte.val = pteVal;
    // printf("  level: %d, pteAddr: %lx, pgBase: %lx -> pte: %lx\n", level, pteAddr, pgBase, pteVal);
    if (!pte.v) {
      printf("Error: invalid pte entry for baseAddr: 0x%lx, vpn: 0x%lx, level: %d\n", baseAddr, vpn, level);
      exit(1);
    } else {
      pgBase = pte.ppn << 12;
    }
  }
  return pgBase >> 12;
}

inline TracePTE DynamicSoftPageTable::getDummyPte(uint8_t level) {
  switch (level) {
    case 0:
      return genLeafPte(OUTOF_TRACE_PPN);
    case 1:
    case 2:
      return genNonLeafPte(getDummyLevelNPage(level-1) >> 12);
    default:
      printf("DynamicSoftPageTable::dummyPte: illegal level:%d\n", level);
      fflush(stdout);
      exit(1);
  }
}


void DynamicSoftPageTable::dump() {
  printf("DynamicSoftPageTable: baseAddr: 0x%lx\n", baseAddr);
  for (auto &entry : pageTable) {
    printf("0x%lx->0x%lx(ppn:0x%lx)\n", entry.first, entry.second.val, entry.second.ppn);
  }
}

void DynamicSoftPageTable::dumpInnerSoftTLB() {
  printf("Dyn Inner Soft TLB:\n");
  for (auto &entry : soft_tlb) {
    printf("0x%lx->0x%lx\n", entry.first, entry.second);
  }
}

inline TracePTE DynamicSoftPageTable::genLeafPte(uint64_t ppn) {
  TracePTE pte;
  pte.val = 0;
  pte.v = 1;
  pte.r = 1;
  pte.w = 1;
  pte.x = 1;
  pte.u = 1;
  pte.a = 1;
  pte.d = 1;
  pte.ppn = ppn;
  return pte;
}

inline TracePTE DynamicSoftPageTable::genNonLeafPte(uint64_t ppn) {
  TracePTE pte;
  pte.val = 0;
  pte.v = 1;
  pte.ppn = ppn;
  return pte;
}

inline uint64_t DynamicSoftPageTable::getPteAddr(uint64_t vpn, uint8_t level, uint64_t baseAddr) {
  return baseAddr + TraceVPNi(vpn, level) * sizeof(uint64_t);
}
