/***************************************************************************************
* Copyright (c) 2025-2026 Beijing Institute of Open Source Chip (BOSC)
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
#include "difftest.h"
#include <cstdlib>
#ifdef CONFIG_DIFFTEST_FORK
#include "ref_fork.h"
#endif

#ifdef CONFIG_DIFFTEST_FAST_REF
int Difftest::fast_apply_events() {
  // Match check_all(): synchronization first, then interrupt/exception handling.
  // Each registered checker owns its valid test and consumes its probe.
  for (const auto &sync: fast_sync_checkers) {
#ifdef CONFIG_DIFFTEST_CRITICALERROREVENT
    if (sync.critical) {
      if (dut->critical_error.valid) {
        proxy->raise_critical_error();
        dut->critical_error.valid = 0;
      }
      continue;
    }
#endif
    if (int ret = sync.checker->step())
      return ret;
  }
  return arch_event_checker->step();
}

bool Difftest::set_ref_mode(RefExecMode mode) {
  bool with_fork = false;
#ifdef CONFIG_DIFFTEST_FORK
  with_fork = difftest_ref_fork_enabled();
#endif
  if (mode == REF_EXEC_FAST && !proxy->require_exec_mode_interfaces(with_fork))
    return false;
  // Old REFs support only SLOW and need no mode-switch export.
  if (proxy->ref_set_exec_mode)
    proxy->ref_set_exec_mode(mode);
  fast_ref_enabled = mode == REF_EXEC_FAST;
  return true;
}

int Difftest::fast_ref_step() {
  bool fork_check = false;
#ifdef CONFIG_DIFFTEST_FORK
  fork_check = difftest_ref_fork_enabled();
#endif
  state->cycle_count = dut->trap.cycleCnt;
  state->has_progress = false;
  if (!state->has_commit) {
    if (int ret = first_commit_checker->step())
      return ret;
    if (!state->has_commit) {
      for (const auto &sync: fast_sync_checkers)
        sync.discard();
      dut->event.valid = 0;
      return DiffTestChecker::STATE_OK;
    }
  }
#ifdef CONFIG_DIFFTEST_FORK
  if (fork_check) {
    const int ret = difftest_ref_fork_prepare(this);
    if (ret == 1)
      return difftest_ref_fork_check(this);
    if (ret)
      return DiffTestChecker::STATE_ERROR;
  }
#endif

  const bool consumes_commit = dut->event.valid;
  if (const int ret = fast_apply_events())
    return ret;
  uint64_t pending = 0;
  uint32_t committed = 0;
  auto execute = [&]() {
    if (!pending)
      return true;
    const uint64_t before = proxy->ref_get_instr_count();
    proxy->ref_exec(pending);
    const uint64_t completed = proxy->ref_get_instr_count() - before;
    if (completed != pending) {
      proxy->sync();
      fprintf(stderr, "FAST short execution: requested=%lu completed=%lu PC=0x%lx\n", (unsigned long)pending,
              (unsigned long)completed, (unsigned long)proxy->state.pc);
      fast_ref_error = DiffTestChecker::STATE_ERROR;
      return false;
    }
#ifdef CONFIG_DIFFTEST_SQUASH
    state->commit_stamp = (state->commit_stamp + completed) % CONFIG_DIFFTEST_SQUASH_STAMPSIZE;
#endif
    pending = 0;
    return true;
  };
  if (!consumes_commit) {
    for (int i = 0; i < CONFIG_DIFF_COMMIT_WIDTH; ++i) {
      const auto &commit = dut->commit[i];
      if (!commit.valid)
        continue;
      committed += 1 + commit.nFused;
      if (commit.skip) {
        if (!execute())
          return fast_ref_error;
        // The NEMU skip API writes integer registers. FP/vector skips use
        // the existing regcpy fallback rather than corrupting an integer GPR.
        proxy->skip_one(commit.isRVC, commit.rfwen && commit.wdest != 0, commit.fpwen, commit.vecwen, commit.wdest,
                        get_commit_data(dut, i));
      } else {
        pending += 1 + commit.nFused;
      }
    }
    // Retain the board-validated per-window boundary, batching commits within it.
    if (!execute())
      return fast_ref_error;
    if (committed) {
      state->has_progress = true;
      state->last_commit_cycle = dut->trap.cycleCnt;
      state->record_group(dut->commit[0].pc, committed);
    }
  }
#ifdef CONFIG_DIFFTEST_FORK
  difftest_ref_fork_publish(this);
#endif
  for (auto &commit: dut->commit)
    commit.valid = 0;
  return DiffTestChecker::STATE_OK;
}
#endif
