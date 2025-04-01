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
#ifndef __TRACE_INST_DEDEUPER_H__
#define __TRACE_INST_DEDEUPER_H__

#include <cstdint>
#include <vector>
#include <algorithm>
#include <ctime>
#include <chrono>
#include <iomanip>
#include <set>

#include "trace_format.h"
#include "branch_predictor/trace_branch_predictor.h"
#include "branch_predictor/trace_branch_type_fixer.h"
#include "trace_legal_flow_check.h"

class TraceInstDeduper {
public:
  struct BranchBPUResult {
    size_t instIndex;  // dynamic info
    uint64_t pc;       // static info
    bool takenCorrect; // bpu info
    bool targetCorrect;
    bool bpuNoChange;

    BranchBPUResult(size_t idx, uint64_t pc, bool takC, bool tarC, bool noC):
      instIndex(idx), pc(pc), takenCorrect(takC), targetCorrect(tarC), bpuNoChange(noC) {}
  };

  struct BranchDedupResult {
    size_t startInstIndex;
    size_t endInstIndex;

    BranchDedupResult() {}

    BranchDedupResult(size_t si, size_t di) : startInstIndex(si), endInstIndex(di) {}

    bool operator<(const BranchDedupResult &other) const {
      return (endInstIndex - startInstIndex) < (other.endInstIndex - other.startInstIndex);
    }

    bool nonEmpty() {
      return endInstIndex > startInstIndex;
    }
  };

protected:
  TraceBranchPredictor bpu;
  TraceBranchTypeFixer type_fixer;

  std::vector<BranchBPUResult> branch_bpu_result;
  std::vector<BranchDedupResult> branch_dedup_result;
  size_t perf_branch_num = 0;
  size_t deduped_branch_num = 0;
  size_t deduped_inst_num = 0;

  void predict(const std::vector<Instruction> &src, size_t from_index, size_t end_index);
  void dedup_branch(const std::vector<Instruction> &src, size_t from_index, size_t end_index);

  // sort each instruction
  void dedup_branch_interval_by_sort(size_t from_branch_index, size_t end_branch_index);
  // Dynamic programming for individual interval
  void dedup_branch_interval_by_dp(size_t from_branch_index, size_t end_branch_index);
  void dedup_branch_interval(size_t from_branch_index, size_t end_branch_index);

  void dedup_inst(std::vector<Instruction> &src);

  inline bool isRVC(uint32_t inst_encoding) {
    return (inst_encoding & 0x3) != 0x3;
  }

public:
  void dedup(std::vector<Instruction> &src, size_t from_index, size_t end_index);
  size_t getSquashedInst() { return deduped_inst_num; }
};

#endif