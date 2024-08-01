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
#include "trace_format.h"

struct TraceCacheLine {
  uint64_t pc[512 / sizeof(uint64_t)];
};
struct TraceHalfCacheLine {
  uint64_t pc[256 / sizeof(uint64_t)];
};

class TraceICache {

private:
  // only for program binary
  char *ram;
  int fd;
  uint64_t mmap_size;
  uint64_t ram_size;
  uint64_t base_addr;

public:
  TraceICache(const char *binary_name);
  ~TraceICache();
  bool readCacheLine(char *line, uint64_t addr);
  bool readHalfCacheLine(char *line, uint64_t addr);
  bool readDWord(uint64_t *dest, uint64_t addr);
  uint64_t ramAddr(uint64_t addr, uint64_t size) {
    return alignAddr(addr - base_addr, size);
  }
  uint64_t alignAddr(uint64_t addr, uint32_t size) {
    return addr & ~(size - 1);
  }
  bool legalAddr(uint64_t addr, int size = 8) {
    return (addr >= base_addr) && ((addr + size) < (base_addr + ram_size));
  }
};

#endif