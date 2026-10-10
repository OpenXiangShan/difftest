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
#include "fork_ref.h"
#endif

#ifdef CONFIG_DIFFTEST_FAST_REF
bool Difftest::set_ref_mode(RefExecMode mode) {
  bool with_fork = false;
#ifdef CONFIG_DIFFTEST_FORK_REF
  with_fork = difftest_ref_fork_enabled();
#endif
  if (mode == REF_EXEC_FAST && !proxy->require_exec_mode_interfaces(with_fork))
    return false;
  if (mode == REF_EXEC_SLOW && flush_fast_ref())
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
  if (const int ret = fast_ref_checker->step()) {
#ifdef CONFIG_DIFFTEST_FORK_REF
    if (fork_check && ret == DiffTestChecker::STATE_TRAP && get_trap_code() == STATE_GOODTRAP)
      difftest_ref_fork_publish(this);
#endif
    return ret;
  }
#ifdef CONFIG_DIFFTEST_FORK_REF
  difftest_ref_fork_publish(this);
#endif
  return DiffTestChecker::STATE_OK;
}

int FastRefChecker::flush() {
  const uint64_t before = driver.completed();
  const bool ok = driver.flush();
#ifdef CONFIG_DIFFTEST_SQUASH
  state->commit_stamp = (state->commit_stamp + driver.completed() - before) % CONFIG_DIFFTEST_SQUASH_STAMPSIZE;
#endif
  return ok ? DiffTestChecker::STATE_OK : DiffTestChecker::STATE_ERROR;
}

int FastRefChecker::do_step() {
  auto *dut = self->dut;
  const RefDriveState drive(*dut);
  const uint64_t before = driver.completed();
  const auto result = driver.run(drive);
#ifdef CONFIG_DIFFTEST_SQUASH
  state->commit_stamp = (state->commit_stamp + driver.completed() - before) % CONFIG_DIFFTEST_SQUASH_STAMPSIZE;
#endif
  if (result == RefDriveExecutor::ERROR)
    return DiffTestChecker::STATE_ERROR;
  // Consume the same sync probes as the SLOW checker chain, without executing them twice.
  for (size_t i = 1; i < self->fast_checkers.size() - 1; ++i) {
    auto *checker = self->fast_checkers[i];
    checker->discard();
#ifdef CONFIG_DIFFTEST_CRITICALERROREVENT
    if (checker == self->critical_error_checker && result != RefDriveExecutor::OK)
      break;
#endif
  }
  if (result == RefDriveExecutor::CRITICAL_MATCH || result == RefDriveExecutor::CRITICAL_MISMATCH) {
    const bool match = result == RefDriveExecutor::CRITICAL_MATCH;
    if (match)
      Info("Core %d dump: HIT CRITICAL ERROR: please check if software cause a double trap.\n", state->coreid);
    else
      Info("Core %d dump: DUT critical_error diff REF\n", state->coreid);
    bool provisional = false;
#ifdef CONFIG_DIFFTEST_FORK_REF
    provisional = difftest_ref_fork_enabled();
#endif
    // The trusted SLOW checker decides whether a fork endpoint is valid.
    state->raise_trap(match || provisional ? STATE_GOODTRAP : STATE_ABORT);
    return DiffTestChecker::STATE_TRAP;
  }
  if (drive.arch_event) {
    const auto &event = dut->event;
    if (event.interrupt)
      state->record_interrupt(event.exceptionPC, event.exceptionInst, event.interrupt);
    else
      state->record_exception(event.exceptionPC, event.exceptionInst, event.exception);
    self->arch_event_checker->discard();
  }
  if (drive.committed) {
    state->has_progress = true;
    state->last_commit_cycle = dut->trap.cycleCnt;
    state->record_group(dut->commit[0].pc, drive.committed);
  }
  for (auto &commit: dut->commit)
    commit.valid = 0;
  return DiffTestChecker::STATE_OK;
}
#endif
