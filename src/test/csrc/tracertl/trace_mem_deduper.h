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

#ifndef __TRACE_MEM_DEDUPER_H__
#define __TRACE_MEM_DEDUPER_H__

#include <cstdint>
#include <vector>
#include <map>
#include <set>
#include <algorithm>
#include <unordered_set>
#include <deque>

#include "trace_format.h"

struct FastSimMemAddr {
  uint64_t vaddr;
  uint64_t paddr;
};

class TraceMemDeduper {
private:
  inline int64_t mergeMemSigdedup(
    std::vector<FastSimMemAddr> &addr_list,
    std::vector<Instruction> instr_list,
    size_t from_index, size_t to_index,
    size_t max_inst
  );

  inline bool hasLegalMemAddr(Instruction inst) {
    return !inst.isTrap() && !inst.isInstException() && (inst.memory_type != MEM_TYPE_None);
  }

  int64_t mergeMemDelorean(
    std::vector<FastSimMemAddr> &addr_list,
    std::vector<Instruction> instr_list,
    size_t from_index, size_t to_index,
    size_t max_insts
  );

  int64_t noMergeMemBaseline(
    std::vector<FastSimMemAddr> &addr_list,
    std::vector<Instruction> instr_list,
    size_t from_index, size_t to_index,
    size_t max_insts
  );

  struct pair_hash {
    template <class T1, class T2>
    std::size_t operator () (const std::pair<T1, T2>& p) const {
        auto hash1 = std::hash<T1>{}(p.first);
        auto hash2 = std::hash<T2>{}(p.second);
        return hash1 ^ hash2;
    }
  };
  uint64_t cachelineAlign(uint64_t addr) {
    return (addr >> 6) << 6;
  }

public:

  int64_t mergeMem(
    std::vector<FastSimMemAddr> &addr_list,
    std::vector<Instruction> instr_list,
    size_t from_index, size_t to_index,
    size_t max_insts
  );
};

#endif // __TRACE_MEM_DEDUPER_H__