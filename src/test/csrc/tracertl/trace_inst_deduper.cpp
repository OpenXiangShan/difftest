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
#include "trace_inst_deduper.h"

void
TraceInstDeduper::predict(const std::vector<Instruction> &src, size_t from_index, size_t end_index) {
  // for (int i = from_index; i < end_index; i++) {
  for (int i = 0; i < end_index; i++) {
    const auto inst = src[i];
    if (inst.branch_type == 0) continue;

    uint64_t seq_pc = inst.instr_pc_va + (isRVC(inst.instr) ? 2 : 4);
    bool is_nonret_jr = type_fixer.is_nonret_jr(inst.instr, inst.branch_type);

    bool predTaken = bpu.predictTaken(inst.instr_pc_va, inst.branch_type);
    uint64_t predTarget = bpu.predictTarget(inst.instr_pc_va, inst.branch_type, is_nonret_jr);
    bool takenC = predTaken == inst.branch_taken;
    bool targetC = predTarget == inst.target;
    bool predC = takenC && targetC;

    bool bpu_changed = false;
    bpu.update(inst.instr_pc_va, inst.target, inst.branch_taken,
      inst.branch_type, seq_pc, is_nonret_jr, bpu_changed);
    // bool dedupable = !bpu_changed;
    bool dedupable = !bpu_changed && predC;

    // if (i >= from_index) { // the filter is ok, but complicated, just rm it
      branch_bpu_result.emplace_back(BranchBPUResult(i, inst.instr_pc_va, takenC, targetC, dedupable));
    // }
  }
}

void
TraceInstDeduper::dedup_branch(const std::vector<Instruction> &src, size_t from_index, size_t end_index) {
  // size_t src_index = from_index;
  size_t start_branch_index = 0;
  size_t end_branch_index = 0;
  while ((start_branch_index < branch_bpu_result.size()) && branch_bpu_result[start_branch_index].instIndex < from_index) {
    start_branch_index++;
    end_branch_index++;
  }
  while ((end_branch_index < branch_bpu_result.size()) && branch_bpu_result[end_branch_index].instIndex < end_index) {
    end_branch_index++;
  }

  // size_t branch_num = branch_bpu_result.size();
  size_t branch_index = start_branch_index;

  while (branch_index < end_branch_index) {
    // skip all the change_bpu branch
    // TODO: check oversize
    while ((branch_index < end_branch_index) && !branch_bpu_result[branch_index].bpuNoChange) {
      branch_index ++;
    }
    if (branch_index >= end_branch_index) break;

    // now branch_index is the noChange bpu
    size_t start_index = branch_index;
    // TODO: check oversize
    while ((branch_index < end_branch_index) && branch_bpu_result[branch_index].bpuNoChange) {
      branch_index++;
    }
    // now branch_index is the !noChange bpu
    size_t end_index = branch_index >= end_branch_index ? end_branch_index-1 : branch_index;
    // if (end_index == start_index) continue; // this should not happen

    // find dedup branch in src
    dedup_branch_interval(start_index, end_index);
  }
}

void
TraceInstDeduper::dedup_branch_interval_by_sort(size_t from_branch_index, size_t end_branch_index) {
  bool pc_same = false;
  std::set<uint64_t> pc_set;
  for (int i = from_branch_index; i < end_branch_index; i++) {
    if (pc_set.find(branch_bpu_result[i].pc) != pc_set.end()) {
      pc_same = true;
      break;
    }
    pc_set.insert(branch_bpu_result[i].pc);
  }
  if (!pc_same) {
    // printf("DP: all pc are different, cannot to dedup\n");
    return;
  }

  // for every branch, get the start_inst_index, and end_inst_index;
  std::map<uint64_t, BranchDedupResult> pc_map;
  for (int i = from_branch_index; i < end_branch_index; i++) {
    uint64_t pc = branch_bpu_result[i].pc;
    size_t inst_index = branch_bpu_result[i].instIndex;
    if (pc_map.find(pc) == pc_map.end()) {
      // create new bse
      pc_map[pc] = BranchDedupResult(inst_index, inst_index);
    } else {
      pc_map[pc].endInstIndex = inst_index;
    }
  }

  std::vector<BranchDedupResult> interval_vector;
  for (auto m : pc_map) {
    interval_vector.emplace_back(m.second);
  }

  std::sort(interval_vector.begin(), interval_vector.end());
  if (interval_vector[interval_vector.size() -1].nonEmpty()) {
    branch_dedup_result.emplace_back(interval_vector[interval_vector.size() - 1]);
  }
}

void
TraceInstDeduper::dedup_branch_interval_by_dp(size_t from_branch_index, size_t end_branch_index) {
  // fast check
  bool pc_same = false;
  std::set<uint64_t> pc_map;
  for (int i = from_branch_index; i < end_branch_index; i++) {
    if (pc_map.find(branch_bpu_result[i].pc) != pc_map.end()) {
      pc_same = true;
      break;
    }
    pc_map.insert(branch_bpu_result[i].pc);
  }
  if (!pc_same) {
    // printf("DP: all pc are different, cannot to dedup\n");
    return;
  }

  size_t branch_num = end_branch_index - from_branch_index + 1;

  // printf("DP: branchIdx from %lu to %lu\n", from_branch_index, end_branch_index);
  // printf("DP: instIndex from %lu to %lu\n", branch_bpu_result[from_branch_index].instIndex, branch_bpu_result[end_branch_index].instIndex);
  // printf("DP: index pc takenC targetC bpuNoChange\n");
  // for (size_t i = 0; i < branch_num; i++) {
  //   printf("DP: %lu %lx %d %d %d\n", branch_bpu_result[from_branch_index + i].instIndex, branch_bpu_result[from_branch_index + i].pc,
  //     branch_bpu_result[from_branch_index + i].takenCorrect, branch_bpu_result[from_branch_index + i].targetCorrect, branch_bpu_result[from_branch_index + i].bpuNoChange);
  // }
  // fflush(stdout);


  std::vector<size_t> dp(branch_num);
  struct DPResult {
    BranchDedupResult interval;
    int pre_index;
    bool interval_valid = false;
  };
  std::vector<DPResult> intervals(branch_num);
  // final result need add all the interval like a linked list

  // function to match the branch pc
  auto match_pc = [this](size_t i, size_t j) {
    return this->branch_bpu_result[i].pc == this->branch_bpu_result[j].pc;
  };
  // function to get the result of res(i, j)
  auto distance = [this](size_t i, size_t j) {
    return this->branch_bpu_result[j].instIndex - this->branch_bpu_result[i].instIndex;
  };

  // dp[i] for the pre-i branch's max result, while the i-th branch is the end branch(not being deduped)
  dp[0] = 0;
  intervals[0].interval_valid = false;
  intervals[0].pre_index = -1; // -1 to stop

  // dp[i] = max{dp[j] + distance(j, i), for all j < i}
  for (size_t i = 1; i < branch_num; i++) {
    // printf("DP: index %lu-----------\n", i);
    // fflush(stdout);
    dp[i] = 0;
    intervals[i].interval_valid = false;
    size_t bIdx_i = from_branch_index + i;

    for (size_t j = 0; j < i; j++) {
      // printf("  j %lu ", j);
      // fflush(stdout);
      size_t bIdx_j = from_branch_index + j;

      bool match = match_pc(bIdx_j, bIdx_i);
      if (!match) {
        // printf("\n");
        // fflush(stdout);
        continue;
      }
      size_t res_j_i = distance(bIdx_j, bIdx_i);
      if ((dp[j] + res_j_i) > dp[i]) {
        dp[i] = dp[j] + res_j_i;
        intervals[i].pre_index = j;
        intervals[i].interval_valid = true;
        intervals[i].interval = BranchDedupResult(branch_bpu_result[bIdx_j].instIndex, branch_bpu_result[bIdx_i].instIndex);
        // printf(" update dp[j-%lu] %lu res_%ld_%ld %lu new dp[i-%lu] %lu inter_valid %d si %ld di %ld\n",
        //   j, dp[j], j, i, res_j_i, i, dp[i], intervals[i].interval_valid,
        //   branch_bpu_result[bIdx_j].instIndex, branch_bpu_result[bIdx_i].instIndex);
        // fflush(stdout);
      }
    }
    if (!intervals[i].interval_valid) {
      // printf("  not updated, set pre_index i=-1\n");
      // fflush(stdout);
      intervals[i].pre_index = i-1;
    }
  }

  // get the final result
  // printf("DP: final result\n");
  // fflush(stdout);
  for (int i = branch_num - 1; i >= 0; i = intervals[i].pre_index) {
    if (intervals[i].interval_valid) {
    //   printf("  add interval from %lu to %lu\n", intervals[i].interval.startInstIndex, intervals[i].interval.endInstIndex);
    // fflush(stdout);
      branch_dedup_result.emplace_back(intervals[i].interval);
    }
  }

  // if (branch_bpu_result[end_branch_index].instIndex > 2011369) {
  //   exit(1);
  // }
}

void
TraceInstDeduper::dedup_branch_interval(size_t from_branch_index, size_t end_branch_index) {
  // end_branch_index should not be ignored/fast_simulation
  // dedup_branch_interval_by_sort(from_branch_index, end_branch_index);
  dedup_branch_interval_by_dp(from_branch_index, end_branch_index);
}

void
TraceInstDeduper::dedup_inst(std::vector<Instruction> &src) {
  for (auto interval : branch_dedup_result) {
    for (size_t i = interval.startInstIndex; i < interval.endInstIndex; i++) {
      src[i].is_squashed = true;

      deduped_branch_num += (src[i].branch_type != 0) ? 1 : 0;
      deduped_inst_num ++;
    }
  }
}

void
TraceInstDeduper::dedup(std::vector<Instruction> &src, size_t from_index, size_t end_index) {
  TraceLegalFlowChecker flowChecker;
  flowChecker.check(src);

  // fix wrong instruction branch type
  for (int i = from_index; i < end_index; i++) {
    src[i].branch_type = type_fixer.fix_branch_type(src[i].instr, src[i].branch_type);
    if (src[i].branch_type != 0) perf_branch_num++;
  }

  printf("Predict started from %ld to %ld\n", from_index, end_index);
  fflush(stdout);
  predict(src, from_index, end_index);
  printf("Predict ready\n");
  fflush(stdout);
  dedup_branch(src, from_index, end_index);
  printf("Dedup branch ready\n");
  fflush(stdout);
  dedup_inst(src);
  printf("Dedup instr ready\n");
  fflush(stdout);

  printf("TraceInstDeduper: Instruction Num %8lu Branch Num %8lu\n", end_index - from_index, perf_branch_num);
  printf("         Deduped: Instruction Num %8lu Branch Num %8lu\n", deduped_inst_num, deduped_branch_num);

  flowChecker.check(src);
}