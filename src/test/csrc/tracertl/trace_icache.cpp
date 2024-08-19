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

#include <fstream>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <iostream>
#include "trace_icache.h"

TraceICache::TraceICache() {
  soft_tlb.clear();
  // uint64_t memory_size = 8 * 1024 * 1024 * 1024UL; // 8GB
}

void TraceICache::constructSoftTLB(uint64_t vaddr, uint16_t asid, uint16_t vmid, uint64_t paddr) {
  vaddr = vaddr & vaddr_mask();
  paddr = paddr & paddr_mask();
  // soft_tlb[TLBKeyType(vaddr >> 12, asid, vmid)] = paddr >> 12;
  soft_tlb[vaddr >> 12] = paddr >> 12;
}

void TraceICache::constructICache(uint64_t vaddr, uint32_t inst) {
  vaddr = vaddr & vaddr_mask();
  bool isRVC = (inst & 0x3) != 0x3;
  if (isRVC) {
    icache_va[vaddr >> 1] = (uint16_t) (inst & 0xffff);
  } else {
    icache_va[vaddr >> 1] = (uint16_t) (inst & 0xffff);
    icache_va[(vaddr >> 1) + 1] = (uint16_t) (inst >> 16);
  }
}

uint16_t TraceICache::readHWord(uint64_t key) {
  METHOD_TRACE();
  auto it = icache_va.find(key);
  if (it != icache_va.end()) {
    return it->second;
  } else {
    return 0;
  }
}

void TraceICache::readDWord(uint64_t &dest, uint64_t addr) {
  METHOD_TRACE();
  addr = addr & vaddr_mask();
  uint64_t addr_inner = addr >> 1;
  dest = 0;
  dest |= (uint64_t)readHWord(addr_inner + 0);
  dest |= (uint64_t)readHWord(addr_inner + 1) << 16;
  dest |= (uint64_t)readHWord(addr_inner + 2) << 32;
  dest |= (uint64_t)readHWord(addr_inner + 3) << 48;
}

uint64_t TraceICache::addrTrans(uint64_t vaddr, uint16_t asid, uint16_t vmid) {
  METHOD_TRACE();
  vaddr = vaddr & vaddr_mask();

  uint64_t vpn = vaddr >> 12;
  uint64_t off = vaddr & 0xfff;
  // auto it = soft_tlb.find(TLBKeyType(vpn, asid, vmid));
  auto it = soft_tlb.find(vpn);
  if (it != soft_tlb.end()) {
    METHOD_TRACE();
    return it->second | off;
  } else {
    METHOD_TRACE();
    return 0x90000000 | off; // give a legal addr
  }
}

bool TraceICache::addrTrans_hit(uint64_t vaddr, uint16_t asid, uint16_t vmid) {
  METHOD_TRACE();
  vaddr = vaddr & vaddr_mask();
  uint64_t vpn = vaddr >> 12;
  // auto it = soft_tlb.find(TLBKeyType(vpn, asid, vmid));
  auto it = soft_tlb.find(vpn);
  if (it != soft_tlb.end()) {
    return true;
  } else {
    return false;
  }
}

TraceICache::~TraceICache() {
}