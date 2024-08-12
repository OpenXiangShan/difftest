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

class TraceICache {

private:
  std::unordered_map<uint64_t, uint16_t> icache_va;

public:
  TraceICache();
  ~TraceICache();
  void constructICache(uint64_t vaddr, uint32_t inst);
  void readDWord(uint64_t &dest, uint64_t addr);
  uint16_t readHWord(uint64_t key);
};

#endif
