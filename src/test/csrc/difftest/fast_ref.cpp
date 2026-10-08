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
#include "flash.h"
#include "ram.h"
#include <cstdlib>
#ifdef FPGA_HOST
#include "ref_fork.h"
#endif

#ifdef CONFIG_DIFFTEST_FAST_REF
bool Difftest::fast_window_has_event(const DiffTestState &window) const {
  if (window.event.valid)
    return true;
  // WFI and the final trap are architectural boundaries: a large ref_exec(n)
  // can execute past them, so force the fast parent to flush its batch here.
  if (window.trap.hasWFI)
    return true;
  if (window.trap.hasTrap)
    return true;
#ifdef CONFIG_DIFFTEST_LRSCEVENT
  if (window.lrsc.valid)
    return true;
#endif
#ifdef CONFIG_DIFFTEST_SYNCAIAEVENT
  if (window.sync_aia.valid)
    return true;
#endif
#ifdef CONFIG_DIFFTEST_NONREGINTERRUPTPENDINGEVENT
  if (window.non_reg_interrupt_pending.valid)
    return true;
#endif
#ifdef CONFIG_DIFFTEST_MHPMEVENTOVERFLOWEVENT
  if (window.mhpmevent_overflow.valid)
    return true;
#endif
#ifdef CONFIG_DIFFTEST_CRITICALERROREVENT
  if (window.critical_error.valid)
    return true;
#endif
#ifdef CONFIG_DIFFTEST_SYNCCUSTOMMFLUSHPWREVENT
  if (window.sync_custom_mflushpwr.valid)
    return true;
#endif
#ifdef CONFIG_DIFFTEST_DEBUGMODE
  if (window.dmregs.debugMode != 0)
    return true;
#endif
  return false;
}

static void clear_fast_events(DiffTestState &window) {
  window.event.valid = 0;
#ifdef CONFIG_DIFFTEST_LRSCEVENT
  window.lrsc.valid = 0;
#endif
#ifdef CONFIG_DIFFTEST_SYNCAIAEVENT
  window.sync_aia.valid = 0;
#endif
#ifdef CONFIG_DIFFTEST_NONREGINTERRUPTPENDINGEVENT
  window.non_reg_interrupt_pending.valid = 0;
#endif
#ifdef CONFIG_DIFFTEST_MHPMEVENTOVERFLOWEVENT
  window.mhpmevent_overflow.valid = 0;
#endif
#ifdef CONFIG_DIFFTEST_CRITICALERROREVENT
  window.critical_error.valid = 0;
#endif
#ifdef CONFIG_DIFFTEST_SYNCCUSTOMMFLUSHPWREVENT
  window.sync_custom_mflushpwr.valid = 0;
#endif
}

int Difftest::fast_apply_events() {
  int ret = DiffTestChecker::STATE_OK;

  // Keep the same ordering as check_all(): synchronization probes are applied
  // before ArchEvent, and ArchEvent takes precedence over instruction commits.
  auto apply = [&ret](DiffTestChecker *checker) {
    if (ret == DiffTestChecker::STATE_OK && checker != nullptr) {
      ret = checker->step();
    }
  };
  for (const auto &[checker, critical]: fast_sync_checkers) {
#ifdef CONFIG_DIFFTEST_CRITICALERROREVENT
    if (critical) {
      if (dut->critical_error.valid) {
        proxy->raise_critical_error();
        dut->critical_error.valid = 0;
      }
      continue;
    }
#endif
    apply(checker);
  }
  apply(arch_event_checker);

  return ret;
}

#endif

#ifdef CONFIG_DIFFTEST_FAST_REF
int Difftest::fast_only_step() {
  bool fork_check = false;
#ifdef FPGA_HOST
  fork_check = difftest_ref_fork_enabled();
#endif
  if (!fast_interfaces_checked) {
    if (!proxy->require_fast_interfaces(fork_check))
      return DiffTestChecker::STATE_ERROR;
    proxy->set_exec_mode(REF_EXEC_FAST);
    fast_interfaces_checked = true;
  }
  state->cycle_count = dut->trap.cycleCnt;
  state->has_progress = false;
  if (!fast_only_initialized) {
    bool has_commit = false;
    for (const auto &commit: dut->commit)
      has_commit |= commit.valid;
    if (!has_commit) {
      clear_fast_events(*dut);
      return DiffTestChecker::STATE_OK;
    }
    // Establish the same initial state as FirstInstrCommitChecker before fork.
    proxy->flash_init((const uint8_t *)flash_dev.base, flash_dev.img_size, flash_dev.img_path);
    simMemory->clone_on_demand(
        [this](uint64_t offset, void *src, size_t n) { proxy->mem_init(PMEM_BASE + offset, src, n, DUT_TO_REF); },
        true);
    proxy->regcpy(&dut->regs, FIRST_INST_ADDRESS);
    state->has_commit = true;
    fast_only_initialized = true;
  }
#ifdef FPGA_HOST
  if (fork_check) {
    const int ret = difftest_ref_fork_prepare(this);
    if (ret == 1)
      return difftest_ref_fork_check(this);
    if (ret)
      return DiffTestChecker::STATE_ERROR;
  }
#endif

  const bool consumes_commit = dut->event.valid;
  if (fast_window_has_event(*dut)) {
    const int ret = fast_apply_events();
    if (ret)
      return ret;
  }
  uint64_t pending = 0;
  uint32_t committed = 0;
  auto execute = [&]() {
    if (!pending)
      return true;
    const uint64_t before = proxy->get_instr_count();
    proxy->ref_exec(pending);
    const uint64_t completed = proxy->get_instr_count() - before;
    if (completed != pending) {
      fprintf(stderr, "FAST short execution: requested=%lu completed=%lu PC=0x%lx\n", (unsigned long)pending,
              (unsigned long)completed, (unsigned long)proxy->get_pc());
      fast_only_error = DiffTestChecker::STATE_ERROR;
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
          return fast_only_error;
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
      return fast_only_error;
    if (committed) {
      state->has_progress = true;
      state->last_commit_cycle = dut->trap.cycleCnt;
      state->record_group(dut->commit[0].pc, committed);
    }
  }
#ifdef FPGA_HOST
  difftest_ref_fork_publish(this);
#endif
  for (auto &commit: dut->commit)
    commit.valid = 0;
  return DiffTestChecker::STATE_OK;
}
#endif
