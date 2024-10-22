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

#include "trace_decompress.h"
#include "trace_icache.h"
#include "trace_common.h"

TraceICache::TraceICache(const char* tracept_file) {
  soft_tlb.clear();
  // uint64_t memory_size = 8 * 1024 * 1024 * 1024UL; // 8GB


  if (tracept_file) {
    // read tracept file and set DynamicPageTable
    printf("TraceRTL: read trace page table file. %s\n", tracept_file);
    std::ifstream *tracept_stream = new std::ifstream(tracept_file, std::ios::in);
    if (!tracept_stream->is_open()) {
      std::cout << "Error: can't open tracept file" << std::endl;
      exit(1);
    }

    tracept_stream->seekg(0, std::ios::end);
    std::streampos fileSize = tracept_stream->tellg();
    tracept_stream->seekg(0, std::ios::beg);
    char *fileBuffer = new char[fileSize];
    tracept_stream->read(fileBuffer, fileSize);
    tracept_stream->close();
    delete tracept_stream;

    if (fileSize == 0) {
      std::cerr << "TracePT file is empty" << std::endl;
      exit(1);
    }

    printf("TraceRTL: decompress trace page table file.\n");
    size_t sizeAfterDC = traceDecompressSizeZSTD(fileBuffer, fileSize);
    if ((sizeAfterDC % sizeof(TracePageEntry)) != 0) {
      std::cerr << "TracePT file decompress result wrong. sizeAfterDC cannot be divided exactly\n" << std::endl;
      exit(1);
    }

    uint64_t tracePtNum = sizeAfterDC / sizeof(TracePageEntry);
    TracePageEntry *traceDecompressBuffer = new TracePageEntry[tracePtNum];
    uint64_t decompressedSize = traceDecompressZSTD((char *)traceDecompressBuffer, sizeAfterDC, fileBuffer, fileSize);
    if (decompressedSize != sizeAfterDC) {
      std::cerr << "TracePT file decompress result wrong. decompressedSize != sizeAfterDC\n" << std::endl;
      exit(1);
    }

    satp = traceDecompressBuffer[0].pte;
    for (uint64_t i = 1; i < tracePtNum; i++) {
      TracePageEntry entry = traceDecompressBuffer[i];
      dynamic_page_table.setPte(entry.paddr, entry.pte, (uint8_t )entry.level);
    }
  }
  uint64_t baseAddr = (satp & TRACE_SATP64_PPN) << TRACE_PAGE_SHIFT;
  dynamic_page_table.setBaseAddr(baseAddr);
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
    return (it->second << 12) | off;
  } else {
    METHOD_TRACE();
    return OUTOF_TRACE_PAGE_PADDR | off; // give a legal addr
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

void TraceICache::dumpICache() {
  fprintf(stderr, "TraceICache Content:\n");
  for (const auto &pair : icache_va) {
    fprintf(stderr, "%08lx -> %04hx\n", pair.first, pair.second);
  }
}

void TraceICache::dumpSoftTlb() {
  // fprintf(stderr, "TraceSoftTlb Content:\n");
  printf("TraceSoftTlb Content:\n");
  for (const auto &pair : soft_tlb) {
    printf("  %016lx -> %09lx\n", pair.first, pair.second);
  }
}

void TraceICache::test() {
  // test for icache

  // test for soft_tlb

  // dumpSoftTlb();
  // dynamic_page_table.dumpInnerSoftTLB();

  // test for dynPageTable
  // {
  //   uint64_t vpn = 0xf63d;
  //   uint64_t ppn_pt = dynPageTrans(vpn);
    // printf("DYN Test vpn = %016lx, ppn_pt = %016lx\n", vpn, ppn_pt);
  //   fflush(stdout);
  // }
  for (auto &pair : soft_tlb) {
    uint64_t vpn = pair.first;
    uint64_t ppn_tlb = pair.second;

    uint64_t ppn_pt = dynPageTrans(vpn);
    if (ppn_pt != ppn_tlb) {
      printf("Warn: vpn = %016lx, ppn_pt = %016lx, ppn_tlb = %016lx\n", vpn, ppn_pt, ppn_tlb);
      // exit(1);
    }
  }

  // dynamic_page_table.dump();
}

uint64_t TraceICache::getSatpPpn() {
  return satp & TRACE_SATP64_PPN;
}

TraceICache::~TraceICache() {
}