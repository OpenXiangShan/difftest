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
#ifndef DIFFTEST_REF_DRIVE_H
#define DIFFTEST_REF_DRIVE_H
#include "refproxy.h"
#include <functional>
#include <memory>
#include <vector>

struct RefDriveState {
  DiffTestRegState regs{};
  DifftestInstrCommit commit[CONFIG_DIFF_COMMIT_WIDTH]{};
  DifftestArchEvent event{};
  DifftestTrapEvent trap{};
#ifdef CONFIG_DIFFTEST_LRSCEVENT
  DifftestLrScEvent lrsc{};
#endif
#ifdef CONFIG_DIFFTEST_NONREGINTERRUPTPENDINGEVENT
  DifftestNonRegInterruptPendingEvent non_reg_interrupt_pending{};
#endif
#ifdef CONFIG_DIFFTEST_MHPMEVENTOVERFLOWEVENT
  DifftestMhpmeventOverflowEvent mhpmevent_overflow{};
#endif
#ifdef CONFIG_DIFFTEST_CRITICALERROREVENT
  DifftestCriticalErrorEvent critical_error{};
#endif
#ifdef CONFIG_DIFFTEST_SYNCAIAEVENT
  DifftestSyncAIAEvent sync_aia{};
#endif
#ifdef CONFIG_DIFFTEST_SYNCCUSTOMMFLUSHPWREVENT
  DifftestSyncCustomMflushpwrEvent sync_custom_mflushpwr{};
#endif
  uint64_t commit_data[CONFIG_DIFF_COMMIT_WIDTH]{};

  RefDriveState() = default;
  explicit RefDriveState(const DiffTestState &state);
};

#ifdef CONFIG_DIFFTEST_FAST_REF
class DiffState;
class ArchEventChecker;

class RefDriveExecutor {
public:
  enum Status {
    OK,
    ERROR,
    TRAP,
    CRITICAL_MATCH,
    CRITICAL_MISMATCH
  };
  struct Result {
    Status status = OK;
    uint32_t committed_instrs = 0;
    bool arch_event = false;
  };
  RefDriveExecutor(RefProxy *proxy, DiffState *state);
  ~RefDriveExecutor();
  Result run(DiffTestState &state);
  Result run(const RefDriveState &state);
  bool execute(uint64_t instr_count);
  bool flush();
  uint64_t completed() const {
    return executed_instrs;
  }

private:
  template <typename Checker, typename Probe>
  void add_checker(Probe DiffTestState::*native_member, Probe RefDriveState::*drive_member);
  template <typename State, typename CommitData> Result run_state(State &state, CommitData commit_data);
  static constexpr uint64_t MAX_PENDING = 65536;
  RefProxy *proxy;
  DiffState *state;
  DiffTestState *native = nullptr;
  const RefDriveState *drive = nullptr;
  std::vector<std::function<int()>> checkers;
  std::unique_ptr<ArchEventChecker> arch_event_checker;
  uint64_t pending_instrs = 0;
  uint64_t executed_instrs = 0;
};
#endif
#endif
