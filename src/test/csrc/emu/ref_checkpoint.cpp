/***************************************************************************************
* Copyright (c) 2020-2026 Institute of Computing Technology, Chinese Academy of Sciences
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

#include "ref_checkpoint.h"
#include "common.h"
#include <filesystem>
#include <sys/stat.h>

#ifndef CONFIG_NO_DIFFTEST
#include "difftest.h"
#endif

void RefCheckpointScheduler::arm(const CommonArgs &args) {
  armed_ = true;
  interval_ = args.auto_checkpoint_interval;
  next_instr_ = interval_;
  if (args.auto_checkpoint_dir != nullptr && args.auto_checkpoint_dir[0] != '\0') {
    std::error_code ec;
    std::filesystem::create_directories(args.auto_checkpoint_dir, ec);
    base_ = std::string(args.auto_checkpoint_dir) + "/ref-cpt";
    (void)ec;
  } else {
    base_ = create_noop_filename("-ref-cpt");
  }
  Info("[REF_CPT] armed interval=%lu base=%s\n", interval_, base_.c_str());
}

#ifndef CONFIG_NO_DIFFTEST
bool RefCheckpointScheduler::take(Difftest *diff, uint64_t instr, uint64_t pc, uint64_t mode) {
  // 1m-004 is the slice at 4,000,000 instructions. seq is not that index.
  uint64_t index = instr / 1000000ULL;
  std::string slot = "1m-";
  if (index < 100) slot.push_back('0');
  if (index < 10) slot.push_back('0');
  slot += std::to_string(index);
  if (instr % 1000000ULL != 0) slot += "-instr" + std::to_string(instr);
  auto slash = base_.find_last_of('/');
  std::string base = (slash == std::string::npos ? slot : base_.substr(0, slash + 1) + slot);
  std::string gz = base + "_memory_.gz";
  Info("[REF_CPT] checkpoint seq=%lu instr=%lu pc=0x%lx mode=%lu file=%s\n", seq_, instr, pc, mode, gz.c_str());
  diff->proxy->trigger_checkpoint(base.c_str());
  struct stat st;
  if (stat(gz.c_str(), &st) != 0 || st.st_size <= 0) {
    Info("[REF_CPT] checkpoint output missing: %s\n", gz.c_str());
    output_failed_ = true;
    return false;
  }
  seq_++;
  return true;
}

bool RefCheckpointScheduler::maybe_take(Difftest *diff) {
  if (!armed_ || diff == nullptr || diff->proxy == nullptr || !diff->get_has_commit() || output_failed_) {
    return !output_failed_;
  }

  uint64_t instr = diff->get_trap_event()->instrCnt;
  if (instr < next_instr_) {
    return true;
  }

  uint64_t mode = diff->dut->regs.csr.privilegeMode;
  uint64_t pc = diff->proxy->state.pc;
  if (mode == 3) {
    Info("[REF_CPT] skip M-mode instr=%lu pc=0x%lx\n", instr, pc);
  } else if (!take(diff, instr, pc, mode)) {
    return false;
  }

  uint64_t step = instr / interval_;
  if (step == UINT64_MAX || interval_ > UINT64_MAX / (step + 1)) {
    next_instr_ = UINT64_MAX;
  } else {
    next_instr_ = (step + 1) * interval_;
  }
  return true;
}
#endif
