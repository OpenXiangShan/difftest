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

#ifndef __TRACE_ICACHE_H__
#define __TRACE_ICACHE_H__

#include <cstdint>
#include <cstdio>
#include <map>
#include <unordered_map>
#include "trace_format.h"
#include "trace_dynamic_page_table.h"

struct TLBKeyType {
  uint64_t vaddr;
  uint16_t asid;
  uint16_t vmid;

  bool operator<(const TLBKeyType &other) const {
    if (vaddr != other.vaddr) return vaddr < other.vaddr;
    if (asid  != other.asid)  return asid  < other.asid;
    return vmid < other.vmid;
  };

  bool operator==(const TLBKeyType &other) const {
    return vaddr == other.vaddr && asid == other.asid && vmid == other.vmid;
  };

  TLBKeyType(uint64_t ovpn, uint16_t oasid, uint16_t ovmid):
    vaddr(ovpn), asid(oasid), vmid(ovmid) {
  };
};

class TraceICache {

private:
  std::unordered_map<uint64_t, uint16_t> icache_va;
  // std::unordered_map<TLBKeyType, uint64_t> soft_tlb;
  std::unordered_map<uint64_t, uint64_t> soft_tlb;

  // ddr/dram image for pagetable
  uint64_t satp = (DYN_PAGE_TABLE_BASE_PADDR + TRACE_PAGE_SIZE * TRACE_MAX_PAGE_LEVEL) >> TRACE_PAGE_SHIFT;
  DynamicSoftPageTable dynamic_page_table;

public:
  TraceICache(const char *tracept_file);
  ~TraceICache();

  void constructSoftTLB(uint64_t vaddr, uint16_t asid, uint16_t vmid, uint64_t paddr);
  uint64_t addrTrans(uint64_t vaddr, uint16_t asid, uint16_t vmid);
  bool addrTrans_hit(uint64_t vaddr, uint16_t asid, uint16_t vmid);

  void constructICache(uint64_t vaddr, uint32_t inst);
  void readDWord(uint64_t &dest, uint64_t addr);
  uint16_t readHWord(uint64_t key);


  void dumpICache();
  void dumpSoftTlb();

  // test if the icache is working
  void test();

  // wrap function for DynamicSoftPageTable
  uint64_t dynPageRead(uint64_t paddr) {
    return dynamic_page_table.read(paddr, false);
  }
  void dynPageWrite(uint64_t vpn, uint64_t ppn) {
    dynamic_page_table.write(vpn, ppn);
  }
  uint64_t dynPageTrans(uint64_t vpn) {
    return dynamic_page_table.trans(vpn);
  }
  void dumpDynPageTable() {
    dynamic_page_table.dump();
  };

  uint64_t getSatpPpn();
};

#endif
