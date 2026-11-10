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
#include "ref_drive.h"
#include "diffstate.h"
#include <algorithm>

RefDriveState::RefDriveState(const DiffTestState &state) : regs(state.regs), event(state.event), trap(state.trap) {
  std::copy(std::begin(state.commit), std::end(state.commit), std::begin(commit));
#ifdef CONFIG_DIFFTEST_LRSCEVENT
  lrsc = state.lrsc;
#endif
#ifdef CONFIG_DIFFTEST_NONREGINTERRUPTPENDINGEVENT
  non_reg_interrupt_pending = state.non_reg_interrupt_pending;
#endif
#ifdef CONFIG_DIFFTEST_MHPMEVENTOVERFLOWEVENT
  mhpmevent_overflow = state.mhpmevent_overflow;
#endif
#ifdef CONFIG_DIFFTEST_CRITICALERROREVENT
  critical_error = state.critical_error;
#endif
#ifdef CONFIG_DIFFTEST_SYNCAIAEVENT
  sync_aia = state.sync_aia;
#endif
#ifdef CONFIG_DIFFTEST_SYNCCUSTOMMFLUSHPWREVENT
  sync_custom_mflushpwr = state.sync_custom_mflushpwr;
#endif
  for (int i = 0; i < CONFIG_DIFF_COMMIT_WIDTH; ++i)
    if (commit[i].valid && commit[i].skip && (commit[i].rfwen || commit[i].fpwen))
      commit_data[i] = get_commit_data(&state, i);
}

#ifdef CONFIG_DIFFTEST_FAST_REF
#include "checkers.h"

template <typename Checker, typename Probe>
void RefDriveExecutor::add_checker(Probe DiffTestState::*native_member, Probe RefDriveState::*drive_member) {
  auto checker = std::make_shared<Checker>(typename Checker::GetProbeFn{}, state, proxy);
  checkers.push_back([this, checker, native_member, drive_member] {
    return native ? checker->run(native->*native_member, this) : checker->run(drive->*drive_member, this);
  });
}

RefDriveExecutor::RefDriveExecutor(RefProxy *proxy, DiffState *state) : proxy(proxy), state(state) {
#ifdef CONFIG_DIFFTEST_LRSCEVENT
  add_checker<LrScChecker>(&DiffTestState::lrsc, &RefDriveState::lrsc);
#endif
#ifdef CONFIG_DIFFTEST_NONREGINTERRUPTPENDINGEVENT
  add_checker<NonRegInterruptPendingChecker>(&DiffTestState::non_reg_interrupt_pending,
                                             &RefDriveState::non_reg_interrupt_pending);
#endif
#ifdef CONFIG_DIFFTEST_MHPMEVENTOVERFLOWEVENT
  add_checker<MhpmeventOverflowChecker>(&DiffTestState::mhpmevent_overflow, &RefDriveState::mhpmevent_overflow);
#endif
#ifdef CONFIG_DIFFTEST_CRITICALERROREVENT
  add_checker<CriticalErrorChecker>(&DiffTestState::critical_error, &RefDriveState::critical_error);
#endif
#ifdef CONFIG_DIFFTEST_SYNCAIAEVENT
  add_checker<AiaChecker>(&DiffTestState::sync_aia, &RefDriveState::sync_aia);
#endif
#ifdef CONFIG_DIFFTEST_SYNCCUSTOMMFLUSHPWREVENT
  add_checker<CustomMflushpwrChecker>(&DiffTestState::sync_custom_mflushpwr, &RefDriveState::sync_custom_mflushpwr);
#endif
  arch_event_checker = std::make_unique<ArchEventChecker>(
      ArchEventChecker::GetProbeFn{}, state, proxy,
      [this]() -> const DiffTestRegState & { return native ? native->regs : drive->regs; });
}

RefDriveExecutor::~RefDriveExecutor() = default;

bool RefDriveExecutor::execute(uint64_t instr_count) {
  while (instr_count) {
    const uint64_t batch = std::min(instr_count, MAX_PENDING - pending_instrs);
    pending_instrs += batch;
    instr_count -= batch;
    if (pending_instrs == MAX_PENDING && !flush())
      return false;
  }
  return true;
}

bool RefDriveExecutor::flush() {
  if (!pending_instrs)
    return true;
  const uint64_t requested = pending_instrs;
  pending_instrs = 0;
  const uint64_t before = proxy->ref_get_instr_count();
  proxy->ref_exec(requested);
  const uint64_t actual = proxy->ref_get_instr_count() - before;
  executed_instrs += actual;
  if (actual != requested) {
    proxy->sync();
    fprintf(stderr, "FAST short execution: requested=%lu completed=%lu PC=0x%lx\n", (unsigned long)requested,
            (unsigned long)actual, (unsigned long)proxy->state.pc);
    return false;
  }
  return true;
}

template <typename State, typename CommitData>
RefDriveExecutor::Result RefDriveExecutor::run_state(State &dut, CommitData commit_data) {
  Result result;
  for (auto &checker: checkers) {
    const int ret = checker();
    if (ret == DiffTestChecker::STATE_TRAP)
      return {state->trap_code == STATE_GOODTRAP ? CRITICAL_MATCH : CRITICAL_MISMATCH};
    if (ret)
      return {ERROR};
  }
  for (int i = 0; i < CONFIG_DIFF_COMMIT_WIDTH; ++i) {
    if (dut.event.valid && i == dut.event.isLatter) {
      const int ret = arch_event_checker->run(dut.event, this);
      if (ret == DiffTestChecker::STATE_TRAP) {
        result.status = TRAP;
      } else if (ret) {
        return {ERROR};
      }
      result.arch_event = true;
      break;
    }
    const auto &c = dut.commit[i];
    if (!c.valid)
      continue;
    result.committed_instrs += 1 + c.nFused;
    if (c.skip) {
      if (!flush())
        return {ERROR};
      proxy->skip_one(c.isRVC, c.rfwen && c.wdest != 0, c.fpwen, c.vecwen, c.wdest, commit_data(i));
    } else if (!execute(1 + c.nFused)) {
      return {ERROR};
    }
  }
  if (dut.trap.hasTrap && !flush())
    return {ERROR};
  return result;
}

RefDriveExecutor::Result RefDriveExecutor::run(DiffTestState &state) {
  native = &state;
  drive = nullptr;
  return run_state(state, [&state](int i) { return get_commit_data(&state, i); });
}

RefDriveExecutor::Result RefDriveExecutor::run(const RefDriveState &state) {
  native = nullptr;
  drive = &state;
  return run_state(state, [&state](int i) { return state.commit_data[i]; });
}
#endif
