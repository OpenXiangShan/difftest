/***************************************************************************************
* Copyright (c) 2020-2025 Institute of Computing Technology, Chinese Academy of Sciences
* Copyright (c) 2025 Beijing Institute of Open Source Chip
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

#ifndef __DIFFTEST_CHECKER_H__
#define __DIFFTEST_CHECKER_H__

#include "common.h"
#include "diffstate.h"
#include "refproxy.h"

class DiffTestChecker {
public:
  DiffTestChecker(DiffState *state, RefProxy *proxy) : state(state), proxy(proxy) {}
  virtual ~DiffTestChecker() = default;

  virtual int step() = 0;

  static const int STATE_OK = 0;
  static const int STATE_DIFF = 1;
  static const int STATE_ERROR = 2;
  static const int STATE_TRAP = 3;

protected:
  DiffState *state;
  RefProxy *proxy;
};

class SimpleChecker : public DiffTestChecker {
public:
  SimpleChecker(DiffState *state, RefProxy *proxy) : DiffTestChecker(state, proxy) {}
  virtual ~SimpleChecker() = default;

  virtual int step() override {
    if (get_valid()) {
      int ret = check();
      clear_valid();
      return ret;
    }
    return 0;
  }

protected:
  virtual bool get_valid() {
    return true;
  }
  virtual void clear_valid() {}
  virtual int check() = 0;
};

template <typename Probe> class ProbeChecker : public DiffTestChecker {
public:
  using GetProbeFn = std::function<Probe &()>;
  using GetRegsFn = std::function<const DiffTestRegState &()>;

  ProbeChecker(GetProbeFn get_probe, DiffState *state, RefProxy *proxy)
      : DiffTestChecker(state, proxy), get_probe(std::move(get_probe)) {}
  virtual ~ProbeChecker() = default;

  virtual int step() override {
    Probe &probe = get_probe();
    if (get_valid(probe)) {
      int ret = check(probe);
      clear_valid(probe);
      return ret;
    }
    return 0;
  }

protected:
  GetProbeFn get_probe;

  virtual bool get_valid(const Probe &probe) {
    return true;
  }
  virtual void clear_valid(Probe &probe) {}
  virtual int check(const Probe &probe) = 0;
};

class ArchEventChecker : public ProbeChecker<DifftestArchEvent> {
public:
  ArchEventChecker(GetProbeFn get_probe, DiffState *state, RefProxy *proxy, GetRegsFn get_regs)
      : ProbeChecker<DifftestArchEvent>(get_probe, state, proxy), get_regs(std::move(get_regs)) {}

private:
  GetRegsFn get_regs;

  bool get_valid(const DifftestArchEvent &probe) override;
  void clear_valid(DifftestArchEvent &probe) override;
  int check(const DifftestArchEvent &probe) override;

  int do_exception(const DifftestArchEvent &probe);
  int do_interrupt(const DifftestArchEvent &probe);
};

class FirstInstrCommitChecker : public ProbeChecker<DifftestInstrCommit> {
public:
  FirstInstrCommitChecker(GetProbeFn get_probe, DiffState *state, RefProxy *proxy, GetRegsFn get_regs)
      : ProbeChecker<DifftestInstrCommit>(get_probe, state, proxy), get_regs(std::move(get_regs)) {}

private:
  GetRegsFn get_regs;

  bool get_valid(const DifftestInstrCommit &probe) override;
  void clear_valid(DifftestInstrCommit &probe) override;
  int check(const DifftestInstrCommit &probe) override;
};

class InstrCommitChecker : public ProbeChecker<DifftestInstrCommit> {
public:
  InstrCommitChecker(GetProbeFn get_probe, DiffState *state, RefProxy *proxy, uint64_t index,
                     std::function<const DiffTestState &()> get_dut_state)
      : ProbeChecker<DifftestInstrCommit>(get_probe, state, proxy), index(index),
        get_dut_state(std::move(get_dut_state)) {}

private:
  uint64_t index;
  std::function<const DiffTestState &()> get_dut_state;

  bool get_valid(const DifftestInstrCommit &probe) override;
  void clear_valid(DifftestInstrCommit &probe) override;
  int check(const DifftestInstrCommit &probe) override;
};

class TimeoutChecker : public ProbeChecker<DifftestTrapEvent> {
public:
#ifdef CONFIG_DIFFTEST_SQUASH
  static const uint64_t timeout_scale = 256;
#else
  static const uint64_t timeout_scale = 1;
#endif // CONFIG_DIFFTEST_SQUASH
#if defined(CPU_NUTSHELL) || defined(CPU_ROCKET_CHIP)
  static const uint64_t first_commit_limit = 1000;
#elif defined(CPU_XIANGSHAN)
  static const uint64_t first_commit_limit = 15000;
#endif
  static const uint64_t stuck_commit_limit = first_commit_limit * timeout_scale;

  TimeoutChecker(GetProbeFn get_probe, DiffState *state, RefProxy *proxy)
      : ProbeChecker<DifftestTrapEvent>(get_probe, state, proxy) {}

private:
  int check(const DifftestTrapEvent &probe) override;
};

#ifdef CONFIG_DIFFTEST_LRSCEVENT
class LrScChecker : public ProbeChecker<DifftestLrScEvent> {
public:
  LrScChecker(GetProbeFn get_probe, DiffState *state, RefProxy *proxy)
      : ProbeChecker<DifftestLrScEvent>(get_probe, state, proxy) {}

private:
  bool get_valid(const DifftestLrScEvent &probe) override;
  void clear_valid(DifftestLrScEvent &probe) override;
  int check(const DifftestLrScEvent &probe) override;
};
#endif // CONFIG_DIFFTEST_LRSCEVENT

#ifdef CONFIG_DIFFTEST_L1TLBEVENT
class L1TLBChecker : public ProbeChecker<DifftestL1TLBEvent> {
public:
  L1TLBChecker(GetProbeFn get_probe, DiffState *state, RefProxy *proxy)
      : ProbeChecker<DifftestL1TLBEvent>(get_probe, state, proxy) {}

private:
  bool get_valid(const DifftestL1TLBEvent &probe) override;
  void clear_valid(DifftestL1TLBEvent &probe) override;
  int check(const DifftestL1TLBEvent &probe) override;
};
#endif // CONFIG_DIFFTEST_L1TLBEVENT

#ifdef CONFIG_DIFFTEST_L2TLBEVENT
class L2TLBChecker : public ProbeChecker<DifftestL2TLBEvent> {
public:
  L2TLBChecker(GetProbeFn get_probe, DiffState *state, RefProxy *proxy)
      : ProbeChecker<DifftestL2TLBEvent>(get_probe, state, proxy) {}

private:
  bool get_valid(const DifftestL2TLBEvent &probe) override;
  void clear_valid(DifftestL2TLBEvent &probe) override;
  int check(const DifftestL2TLBEvent &probe) override;
};
#endif // CONFIG_DIFFTEST_L2TLBEVENT

#ifdef CONFIG_DIFFTEST_REFILLEVENT
class RefillChecker : public ProbeChecker<DifftestRefillEvent> {
public:
  RefillChecker(GetProbeFn get_probe, DiffState *state, RefProxy *proxy, uint64_t index)
      : ProbeChecker<DifftestRefillEvent>(get_probe, state, proxy), index(index) {}

private:
  uint64_t index;

  bool get_valid(const DifftestRefillEvent &probe) override;
  void clear_valid(DifftestRefillEvent &probe) override;
  int check(const DifftestRefillEvent &probe) override;
};

#ifdef CONFIG_DIFFTEST_CMOINVALEVENT
class CmoInvalRecorder : public ProbeChecker<DifftestCMOInvalEvent> {
public:
  CmoInvalRecorder(GetProbeFn get_probe, DiffState *state, RefProxy *proxy)
      : ProbeChecker<DifftestCMOInvalEvent>(get_probe, state, proxy) {}

private:
  bool get_valid(const DifftestCMOInvalEvent &probe) override;
  void clear_valid(DifftestCMOInvalEvent &probe) override;
  int check(const DifftestCMOInvalEvent &probe) override;
};
#endif // CONFIG_DIFFTEST_CMOINVALEVENT
#endif // CONFIG_DIFFTEST_REFILLEVENT

#ifdef CONFIG_DIFFTEST_NONREGINTERRUPTPENDINGEVENT
class NonRegInterruptPendingChecker : public ProbeChecker<DifftestNonRegInterruptPendingEvent> {
public:
  NonRegInterruptPendingChecker(GetProbeFn get_probe, DiffState *state, RefProxy *proxy)
      : ProbeChecker<DifftestNonRegInterruptPendingEvent>(get_probe, state, proxy) {}

private:
  bool get_valid(const DifftestNonRegInterruptPendingEvent &probe) override;
  void clear_valid(DifftestNonRegInterruptPendingEvent &probe) override;
  int check(const DifftestNonRegInterruptPendingEvent &probe) override;
};
#endif // CONFIG_DIFFTEST_NONREGINTERRUPTPENDINGEVENT

#ifdef CONFIG_DIFFTEST_MHPMEVENTOVERFLOWEVENT
class MhpmeventOverflowChecker : public ProbeChecker<DifftestMhpmeventOverflowEvent> {
public:
  MhpmeventOverflowChecker(GetProbeFn get_probe, DiffState *state, RefProxy *proxy)
      : ProbeChecker<DifftestMhpmeventOverflowEvent>(get_probe, state, proxy) {}

private:
  bool get_valid(const DifftestMhpmeventOverflowEvent &probe) override;
  void clear_valid(DifftestMhpmeventOverflowEvent &probe) override;
  int check(const DifftestMhpmeventOverflowEvent &probe) override;
};
#endif // CONFIG_DIFFTEST_MHPMEVENTOVERFLOWEVENT

#ifdef CONFIG_DIFFTEST_SYNCAIAEVENT
class AiaChecker : public ProbeChecker<DifftestSyncAIAEvent> {
public:
  AiaChecker(GetProbeFn get_probe, DiffState *state, RefProxy *proxy)
      : ProbeChecker<DifftestSyncAIAEvent>(get_probe, state, proxy) {}

private:
  bool get_valid(const DifftestSyncAIAEvent &probe) override;
  void clear_valid(DifftestSyncAIAEvent &probe) override;
  int check(const DifftestSyncAIAEvent &probe) override;
};
#endif // CONFIG_DIFFTEST_SYNCAIAEVENT

#ifdef CONFIG_DIFFTEST_SYNCCUSTOMMFLUSHPWREVENT
class CustomMflushpwrChecker : public ProbeChecker<DifftestSyncCustomMflushpwrEvent> {
public:
  CustomMflushpwrChecker(GetProbeFn get_probe, DiffState *state, RefProxy *proxy)
      : ProbeChecker<DifftestSyncCustomMflushpwrEvent>(get_probe, state, proxy) {}

private:
  bool get_valid(const DifftestSyncCustomMflushpwrEvent &probe) override;
  void clear_valid(DifftestSyncCustomMflushpwrEvent &probe) override;
  int check(const DifftestSyncCustomMflushpwrEvent &probe) override;
};
#endif // CONFIG_DIFFTEST_SYNCCUSTOMMFLUSHPWREVENT

#ifdef CONFIG_DIFFTEST_CRITICALERROREVENT
class CriticalErrorChecker : public ProbeChecker<DifftestCriticalErrorEvent> {
public:
  CriticalErrorChecker(GetProbeFn get_probe, DiffState *state, RefProxy *proxy)
      : ProbeChecker<DifftestCriticalErrorEvent>(get_probe, state, proxy) {}

private:
  bool get_valid(const DifftestCriticalErrorEvent &probe) override;
  void clear_valid(DifftestCriticalErrorEvent &probe) override;
  int check(const DifftestCriticalErrorEvent &probe) override;
};
#endif

class GoldenMemoryInit : public ProbeChecker<DifftestTrapEvent> {
public:
  GoldenMemoryInit(GetProbeFn get_probe, DiffState *state, RefProxy *proxy)
      : ProbeChecker<DifftestTrapEvent>(get_probe, state, proxy) {}

private:
  bool initDump = true;

  bool get_valid(const DifftestTrapEvent &probe) override;
  void clear_valid(DifftestTrapEvent &probe) override;
  int check(const DifftestTrapEvent &probe) override;
};

#ifdef CONFIG_DIFFTEST_SBUFFEREVENT
class SbufferChecker : public ProbeChecker<DifftestSbufferEvent> {
public:
  SbufferChecker(GetProbeFn get_probe, DiffState *state, RefProxy *proxy)
      : ProbeChecker<DifftestSbufferEvent>(get_probe, state, proxy) {}

private:
  bool get_valid(const DifftestSbufferEvent &probe) override;
  void clear_valid(DifftestSbufferEvent &probe) override;
  int check(const DifftestSbufferEvent &probe) override;
};
#endif // CONFIG_DIFFTEST_SBUFFEREVENT

#ifdef CONFIG_DIFFTEST_UNCACHEMMSTOREEVENT
class UncacheMmStoreChecker : public ProbeChecker<DifftestUncacheMMStoreEvent> {
public:
  UncacheMmStoreChecker(GetProbeFn get_probe, DiffState *state, RefProxy *proxy)
      : ProbeChecker<DifftestUncacheMMStoreEvent>(get_probe, state, proxy) {}

private:
  bool get_valid(const DifftestUncacheMMStoreEvent &probe) override;
  void clear_valid(DifftestUncacheMMStoreEvent &probe) override;
  int check(const DifftestUncacheMMStoreEvent &probe) override;
};
#endif // CONFIG_DIFFTEST_UNCACHEMMSTOREEVENT

#ifdef CONFIG_DIFFTEST_ATOMICEVENT
class AtomicChecker : public ProbeChecker<DifftestAtomicEvent> {
public:
  AtomicChecker(GetProbeFn get_probe, DiffState *state, RefProxy *proxy)
      : ProbeChecker<DifftestAtomicEvent>(get_probe, state, proxy) {}

private:
  bool get_valid(const DifftestAtomicEvent &probe) override;
  void clear_valid(DifftestAtomicEvent &probe) override;
  int check(const DifftestAtomicEvent &probe) override;
};
#endif // CONFIG_DIFFTEST_ATOMICEVENT

#ifdef CONFIG_DIFFTEST_LOADEVENT
class LoadChecker : public ProbeChecker<DifftestLoadEvent> {
public:
  LoadChecker(GetProbeFn get_probe, DiffState *state, RefProxy *proxy, uint64_t index,
              std::function<const DiffTestState &()> get_dut_state)
      : ProbeChecker<DifftestLoadEvent>(get_probe, state, proxy), index(index),
        get_dut_state(std::move(get_dut_state)) {}

private:
  uint64_t index;
  std::function<const DiffTestState &()> get_dut_state;
#ifdef CONFIG_DIFFTEST_ARCHVECREGSTATE
  bool enable_vec_load_goldenmem_check = proxy->check_ref_vec_load_goldenmem();
#endif // CONFIG_DIFFTEST_LOADEVENT && CONFIG_DIFFTEST_ARCHVECREGSTATE

  bool get_valid(const DifftestLoadEvent &probe) override;
  void clear_valid(DifftestLoadEvent &probe) override;
  int check(const DifftestLoadEvent &probe) override;

  int do_vec_load_check(const DifftestLoadEvent &probe, uint8_t firstLdest, const uint64_t *commitData);
  int do_load_check(const DifftestLoadEvent &probe, bool regWen, uint64_t *refRegPtr, uint64_t commitData);
};

#ifdef CONFIG_DIFFTEST_SQUASH
class LoadSquashChecker : public SimpleChecker {
public:
  LoadSquashChecker(DiffState *state, RefProxy *proxy, std::function<const DiffTestState &()> get_dut_state)
      : SimpleChecker(state, proxy), get_dut_state(std::move(get_dut_state)) {}

private:
  std::function<const DiffTestState &()> get_dut_state;

  bool get_valid() override;
  void clear_valid() override;
  int check() override;
};

#endif // CONFIG_DIFFTEST_SQUASH
#endif // CONFIG_DIFFTEST_LOADEVENT

#ifdef CONFIG_DIFFTEST_STOREEVENT
class StoreRecorder : public ProbeChecker<DifftestStoreEvent> {
public:
  StoreRecorder(GetProbeFn get_probe, DiffState *state, RefProxy *proxy)
      : ProbeChecker<DifftestStoreEvent>(get_probe, state, proxy) {}

private:
  bool get_valid(const DifftestStoreEvent &probe) override;
  void clear_valid(DifftestStoreEvent &probe) override;
  int check(const DifftestStoreEvent &probe) override;
};

class StoreChecker : public SimpleChecker {
public:
  StoreChecker(DiffState *state, RefProxy *proxy) : SimpleChecker(state, proxy) {}

private:
  int check() override;
};
#endif // CONFIG_DIFFTEST_STOREEVENT

#endif // __DIFFTEST_CHECKER_H__
