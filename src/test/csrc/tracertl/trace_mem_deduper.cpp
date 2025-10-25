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

#include <unordered_set>
#include "trace_mem_deduper.h"

int64_t TraceMemDeduper::mergeMem(
  std::vector<FastSimMemAddr> &addr_list,
  std::vector<Instruction> instr_list,
  size_t from_index, size_t to_index,
  size_t max_inst
) {
  printf("[TraceMemDeduper] Fast Sim from %lu to %lu\n", from_index, to_index);
  printf("[TraceMemDeduper] Detailed Sim from %lu to %lu\n", to_index, max_inst);

  // TODO: add cache-only mode
  return noMergeMemBaseline(addr_list, instr_list, from_index, to_index, max_inst);
  // return mergeMemSigdedup(addr_list, instr_list, from_index, to_index, max_inst);
  // return mergeMemDelorean(addr_list, instr_list, from_index, to_index, max_inst);
}

int64_t TraceMemDeduper::mergeMemSigdedup(
  std::vector<FastSimMemAddr> &addr_list,
  std::vector<Instruction> instr_list,
  size_t from_index, size_t to_index,
  size_t max_insts
) {
  printf("[TraceMemDeduper] Use Sigdedup\n");
  std::unordered_set<std::pair<uint64_t, uint64_t>, pair_hash> memAddrSet;

  size_t oldCount = 0;
  // in reverse order
  for (size_t i = to_index-1; (i >= from_index) && (i != 0); i--) {
    auto inst = instr_list[i];
    if (hasLegalMemAddr(inst)) { // legal memory
      oldCount ++;

      uint64_t vaddr = inst.exu_data.memory_address.va;
      uint64_t paddr = inst.exu_data.memory_address.pa;

      if (memAddrSet.find(std::make_pair(vaddr, paddr)) == memAddrSet.end()) {
        memAddrSet.insert(std::make_pair(vaddr, paddr));
        addr_list.emplace_back(FastSimMemAddr{vaddr, paddr});
      }
    }
  }
  printf("[TraceMemDeduper] Memory Insts/Ops %lu -> %lu\n", oldCount, addr_list.size());
  return addr_list.size();
}

int64_t TraceMemDeduper::mergeMemDelorean(
  std::vector<FastSimMemAddr> &addr_list,
  std::vector<Instruction> instr_list,
  size_t from_index, size_t to_index,
  size_t max_inst
) {
  printf("[TraceMemDeduper] Use Delorean(DSW)\n");
  std::unordered_set<std::pair<uint64_t, uint64_t>, pair_hash> memAddrSetSampling;
  std::unordered_set<std::pair<uint64_t, uint64_t>, pair_hash> memAddrSetWarming;
  std::unordered_set<std::pair<uint64_t, uint64_t>, pair_hash> memAddrSetFilteredBySamp;

  for (size_t i = to_index; i < max_inst; i++) {
    auto inst = instr_list[i];
    if (hasLegalMemAddr(inst)) { // legal memory
      uint64_t vaddr = inst.exu_data.memory_address.va;
      uint64_t paddr = inst.exu_data.memory_address.pa;
      memAddrSetSampling.insert(std::make_pair(vaddr, paddr));
    }
  }

  // in reverse order
  size_t oldCount = 0;
  size_t rawFilteredBySampling = 0;
  for (size_t i = to_index-1; (i >= from_index) && (i != 0); i--) {
    auto inst = instr_list[i];
    if (hasLegalMemAddr(inst)) { // legal memory
      oldCount ++;

      uint64_t vaddr = inst.exu_data.memory_address.va;
      uint64_t paddr = inst.exu_data.memory_address.pa;
      auto cur = std::make_pair(vaddr, paddr);

      // if not found in sampling phase, skip
      if (memAddrSetSampling.find(cur) == memAddrSetSampling.end()) {
        memAddrSetFilteredBySamp.insert(cur);
        rawFilteredBySampling ++;
        continue;
      }

      if (memAddrSetWarming.find(cur) == memAddrSetWarming.end()) {
        memAddrSetWarming.insert(cur);
        // put the code here to maintain order
        addr_list.emplace_back(FastSimMemAddr{vaddr, paddr});
      }
    }
  }

  printf("[TraceMemDeduper] FiltedBySampling raw %lu mergedLine %lu\n", rawFilteredBySampling, memAddrSetFilteredBySamp.size());
  printf("[TraceMemDeduper] Memory Insts/Ops All %lu raw %lu mergedLine %lu\n", oldCount, oldCount - rawFilteredBySampling, addr_list.size());
  return addr_list.size();
}

int64_t TraceMemDeduper::noMergeMemBaseline(
  std::vector<FastSimMemAddr> &addr_list,
  std::vector<Instruction> instr_list,
  size_t from_index, size_t to_index,
  size_t max_insts
) {
  printf("[TraceMemDeduper] Use Baseline(no merge)\n");

  size_t oldCount = 0;
  // in reverse order
  for (size_t i = to_index-1; (i >= from_index) && (i != 0); i--) {
    auto inst = instr_list[i];
    if (hasLegalMemAddr(inst)) { // legal memory
      oldCount ++;

      uint64_t vaddr = inst.exu_data.memory_address.va;
      uint64_t paddr = inst.exu_data.memory_address.pa;
      addr_list.emplace_back(FastSimMemAddr{vaddr, paddr});
    }
  }
  printf("[TraceMemDeduper] Memory Insts/Ops %lu -> %lu\n", oldCount, addr_list.size());
  return addr_list.size();
}