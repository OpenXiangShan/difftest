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
#ifdef CONFIG_DIFFTEST_FORK_REF
#include "ref_fork.h"
#endif

#ifdef CONFIG_DIFFTEST_FAST_REF
bool Difftest::set_ref_mode(RefExecMode mode) {
  bool with_fork = false;
#ifdef CONFIG_DIFFTEST_FORK_REF
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
#ifdef CONFIG_DIFFTEST_FORK_REF
  const bool fork_check = difftest_ref_fork_enabled();
#endif
  state->cycle_count = dut->trap.cycleCnt;
  state->has_progress = false;
  if (!state->has_commit) {
    if (int ret = fast_checkers.front()->step())
      return ret;
    if (!state->has_commit) {
      for (size_t i = 1; i < fast_checkers.size(); ++i)
        fast_checkers[i]->discard();
      dut->event.valid = 0;
      return DiffTestChecker::STATE_OK;
    }
  }
#ifdef CONFIG_DIFFTEST_FORK_REF
  if (fork_check) {
    const int ret = difftest_ref_fork_prepare(this);
    if (ret == 1)
      return difftest_ref_fork_check(this);
    if (ret)
      return DiffTestChecker::STATE_ERROR;
  }
#endif

  // First commit initialization must precede the snapshot; sync and execution follow it.
  for (size_t i = 1; i < fast_checkers.size(); ++i) {
    auto *checker = fast_checkers[i];
    if (const int ret = checker->step()) {
#ifdef CONFIG_DIFFTEST_FORK_REF
#ifdef CONFIG_DIFFTEST_CRITICALERROREVENT
      if (fork_check && checker == critical_error_checker && ret == DiffTestChecker::STATE_TRAP)
        // A provisional endpoint: the trusted SLOW checker decides success or failure.
        state->raise_trap(STATE_GOODTRAP);
#endif
      if (fork_check && ret == DiffTestChecker::STATE_TRAP && get_trap_code() == STATE_GOODTRAP)
        difftest_ref_fork_publish(this);
#endif
      return ret;
    }
  }
#ifdef CONFIG_DIFFTEST_FORK_REF
  difftest_ref_fork_publish(this);
#endif
  return DiffTestChecker::STATE_OK;
}

int FastRefChecker::do_step() {
  auto *dut = self->dut;
  const bool consumes_commit = dut->event.valid;
  if (int ret = self->arch_event_checker->step())
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
          return DiffTestChecker::STATE_ERROR;
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
      return DiffTestChecker::STATE_ERROR;
    if (committed) {
      state->has_progress = true;
      state->last_commit_cycle = dut->trap.cycleCnt;
      state->record_group(dut->commit[0].pc, committed);
    }
  }
  for (auto &commit: dut->commit)
    commit.valid = 0;
  return DiffTestChecker::STATE_OK;
}
#endif
