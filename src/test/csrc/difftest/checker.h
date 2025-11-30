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

class SimpleChecker {
public:
  SimpleChecker(DiffState *state, REF_PROXY *proxy) : state(state), proxy(proxy) {}
  virtual ~SimpleChecker() = default;

  int step() {
    if (get_valid()) {
      int ret = check();
      clear_valid();
      return ret;
    }
    return 0;
  }

protected:
  DiffState *state;
  REF_PROXY *proxy;

  virtual bool get_valid() {
    return true;
  }
  virtual void clear_valid() {}
  virtual int check() = 0;
};

template <typename Probe> class DiffTestChecker {
public:
  using GetProbeFn = std::function<Probe &()>;
  using GetRegsFn = std::function<const DiffTestRegState &()>;

  DiffTestChecker(GetProbeFn get_probe, DiffState *state, REF_PROXY *proxy)
      : get_probe(std::move(get_probe)), state(state), proxy(proxy) {}
  virtual ~DiffTestChecker() = default;

  int step() {
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
  DiffState *state;
  REF_PROXY *proxy;

  virtual bool get_valid(const Probe &probe) {
    return true;
  }
  virtual void clear_valid(Probe &probe) {}
  virtual int check(const Probe &probe) = 0;
};

class ArchEventChecker : public DiffTestChecker<DifftestArchEvent> {
public:
  ArchEventChecker(GetProbeFn get_probe, DiffState *state, REF_PROXY *proxy, GetRegsFn get_regs)
      : DiffTestChecker<DifftestArchEvent>(get_probe, state, proxy), get_regs(std::move(get_regs)) {}

private:
  GetRegsFn get_regs;

  bool get_valid(const DifftestArchEvent &probe) override;
  void clear_valid(DifftestArchEvent &probe) override;
  int check(const DifftestArchEvent &probe) override;

  int do_exception(const DifftestArchEvent &probe);
  int do_interrupt(const DifftestArchEvent &probe);
};

class FirstInstrCommitChecker : public DiffTestChecker<DifftestInstrCommit> {
public:
  FirstInstrCommitChecker(GetProbeFn get_probe, DiffState *state, REF_PROXY *proxy, GetRegsFn get_regs)
      : DiffTestChecker<DifftestInstrCommit>(get_probe, state, proxy), get_regs(std::move(get_regs)) {}

private:
  GetRegsFn get_regs;

  bool get_valid(const DifftestInstrCommit &probe) override;
  void clear_valid(DifftestInstrCommit &probe) override;
  int check(const DifftestInstrCommit &probe) override;
};

class InstrCommitChecker : public DiffTestChecker<DifftestInstrCommit> {
public:
  InstrCommitChecker(GetProbeFn get_probe, DiffState *state, REF_PROXY *proxy,
                     std::function<const DiffTestState &()> get_dut_state)
      : DiffTestChecker<DifftestInstrCommit>(get_probe, state, proxy), get_dut_state(std::move(get_dut_state)) {}

private:
  std::function<const DiffTestState &()> get_dut_state;

  bool get_valid(const DifftestInstrCommit &probe) override;
  void clear_valid(DifftestInstrCommit &probe) override;
  int check(const DifftestInstrCommit &probe) override;
};

class TimeoutChecker : public DiffTestChecker<DifftestTrapEvent> {
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

  TimeoutChecker(GetProbeFn get_probe, DiffState *state, REF_PROXY *proxy)
      : DiffTestChecker<DifftestTrapEvent>(get_probe, state, proxy) {}

private:
  int check(const DifftestTrapEvent &probe) override;
};

#ifdef CONFIG_DIFFTEST_LRSCEVENT
class LrScChecker : public DiffTestChecker<DifftestLrScEvent> {
public:
  LrScChecker(GetProbeFn get_probe, DiffState *state, REF_PROXY *proxy)
      : DiffTestChecker<DifftestLrScEvent>(get_probe, state, proxy) {}

private:
  bool get_valid(const DifftestLrScEvent &probe) override;
  void clear_valid(DifftestLrScEvent &probe) override;
  int check(const DifftestLrScEvent &probe) override;
};
#endif // CONFIG_DIFFTEST_LRSCEVENT

#ifdef CONFIG_DIFFTEST_L1TLBEVENT
class L1TLBChecker : public DiffTestChecker<DifftestL1TLBEvent> {
public:
  L1TLBChecker(GetProbeFn get_probe, DiffState *state, REF_PROXY *proxy)
      : DiffTestChecker<DifftestL1TLBEvent>(get_probe, state, proxy) {}

private:
  bool get_valid(const DifftestL1TLBEvent &probe) override;
  void clear_valid(DifftestL1TLBEvent &probe) override;
  int check(const DifftestL1TLBEvent &probe) override;
};
#endif // CONFIG_DIFFTEST_L1TLBEVENT

#ifdef CONFIG_DIFFTEST_L2TLBEVENT
class L2TLBChecker : public DiffTestChecker<DifftestL2TLBEvent> {
public:
  L2TLBChecker(GetProbeFn get_probe, DiffState *state, REF_PROXY *proxy)
      : DiffTestChecker<DifftestL2TLBEvent>(get_probe, state, proxy) {}

private:
  bool get_valid(const DifftestL2TLBEvent &probe) override;
  void clear_valid(DifftestL2TLBEvent &probe) override;
  int check(const DifftestL2TLBEvent &probe) override;
};
#endif // CONFIG_DIFFTEST_L2TLBEVENT

#ifdef CONFIG_DIFFTEST_REFILLEVENT
class RefillChecker : public DiffTestChecker<DifftestRefillEvent> {
public:
  RefillChecker(GetProbeFn get_probe, DiffState *state, REF_PROXY *proxy)
      : DiffTestChecker<DifftestRefillEvent>(get_probe, state, proxy) {}

private:
  bool get_valid(const DifftestRefillEvent &probe) override;
  void clear_valid(DifftestRefillEvent &probe) override;
  int check(const DifftestRefillEvent &probe) override;
};

#ifdef CONFIG_DIFFTEST_CMOINVALEVENT
class CmoInvalRecorder : public DiffTestChecker<DifftestCmoInvalEvent> {
public:
  CmoInvalRecorder(GetProbeFn get_probe, DiffState *state, REF_PROXY *proxy)
      : DiffTestChecker<DifftestCmoInvalEvent>(get_probe, state, proxy) {}

private:
  bool get_valid(const DifftestCmoInvalEvent &probe) override;
  void clear_valid(DifftestCmoInvalEvent &probe) override;
  int check(const DifftestCmoInvalEvent &probe) override;
};
#endif // CONFIG_DIFFTEST_CMOINVALEVENT
#endif // CONFIG_DIFFTEST_REFILLEVENT

#ifdef CONFIG_DIFFTEST_NONREGINTERRUPTPENDINGEVENT
class NonRegInterruptPendingChecker : public DiffTestChecker<DifftestNonRegInterruptPendingEvent> {
public:
  NonRegInterruptPendingChecker(GetProbeFn get_probe, DiffState *state, REF_PROXY *proxy)
      : DiffTestChecker<DifftestNonRegInterruptPendingEvent>(get_probe, state, proxy) {}

private:
  bool get_valid(const DifftestNonRegInterruptPendingEvent &probe) override;
  void clear_valid(DifftestNonRegInterruptPendingEvent &probe) override;
  int check(const DifftestNonRegInterruptPendingEvent &probe) override;
};
#endif // CONFIG_DIFFTEST_NONREGINTERRUPTPENDINGEVENT

#ifdef CONFIG_DIFFTEST_MHPMEVENTOVERFLOWEVENT
class MhpmeventOverflowChecker : public DiffTestChecker<DifftestMhpmeventOverflowEvent> {
public:
  MhpmeventOverflowChecker(GetProbeFn get_probe, DiffState *state, REF_PROXY *proxy)
      : DiffTestChecker<DifftestMhpmeventOverflowEvent>(get_probe, state, proxy) {}

private:
  bool get_valid(const DifftestMhpmeventOverflowEvent &probe) override;
  void clear_valid(DifftestMhpmeventOverflowEvent &probe) override;
  int check(const DifftestMhpmeventOverflowEvent &probe) override;
};
#endif // CONFIG_DIFFTEST_MHPMEVENTOVERFLOWEVENT

#ifdef CONFIG_DIFFTEST_SYNCAIAEVENT
class AiaChecker : public DiffTestChecker<DifftestSyncAiaEvent> {
public:
  AiaChecker(GetProbeFn get_probe, DiffState *state, REF_PROXY *proxy)
      : DiffTestChecker<DifftestSyncAiaEvent>(get_probe, state, proxy) {}

private:
  bool get_valid(const DifftestSyncAiaEvent &probe) override;
  void clear_valid(DifftestSyncAiaEvent &probe) override;
  int check(const DifftestSyncAiaEvent &probe) override;
};
#endif // CONFIG_DIFFTEST_SYNCAIAEVENT

#ifdef CONFIG_DIFFTEST_SYNCCUSTOMMFLUSHPWREVENT
class CustomMflushpwrChecker : public DiffTestChecker<DifftestSyncCustomMflushpwrEvent> {
public:
  CustomMflushpwrChecker(GetProbeFn get_probe, DiffState *state, REF_PROXY *proxy)
      : DiffTestChecker<DifftestSyncCustomMflushpwrEvent>(get_probe, state, proxy) {}

private:
  bool get_valid(const DifftestSyncCustomMflushpwrEvent &probe) override;
  void clear_valid(DifftestSyncCustomMflushpwrEvent &probe) override;
  int check(const DifftestSyncCustomMflushpwrEvent &probe) override;
};
#endif // CONFIG_DIFFTEST_SYNCCUSTOMMFLUSHPWREVENT

#ifdef CONFIG_DIFFTEST_CRITICALERROREVENT
class CriticalErrorChecker : public DiffTestChecker<DifftestCriticalErrorEvent> {
public:
  CriticalErrorChecker(GetProbeFn get_probe, DiffState *state, REF_PROXY *proxy)
      : DiffTestChecker<DifftestCriticalErrorEvent>(get_probe, state, proxy) {}

private:
  bool get_valid(const DifftestCriticalErrorEvent &probe) override;
  void clear_valid(DifftestCriticalErrorEvent &probe) override;
  int check(const DifftestCriticalErrorEvent &probe) override;
};
#endif

class GoldenMemoryInit : public DiffTestChecker<DifftestTrapEvent> {
public:
  GoldenMemoryInit(GetProbeFn get_probe, DiffState *state, REF_PROXY *proxy)
      : DiffTestChecker<DifftestTrapEvent>(get_probe, state, proxy) {}

private:
  bool initDump = true;

  bool get_valid(const DifftestTrapEvent &probe) override;
  void clear_valid(DifftestTrapEvent &probe) override;
  int check(const DifftestTrapEvent &probe) override;
};

#ifdef CONFIG_DIFFTEST_SBUFFEREVENT
class SbufferChecker : public DiffTestChecker<DifftestSbufferEvent> {
public:
  SbufferChecker(GetProbeFn get_probe, DiffState *state, REF_PROXY *proxy)
      : DiffTestChecker<DifftestSbufferEvent>(get_probe, state, proxy) {}

private:
  bool get_valid(const DifftestSbufferEvent &probe) override;
  void clear_valid(DifftestSbufferEvent &probe) override;
  int check(const DifftestSbufferEvent &probe) override;
};
#endif // CONFIG_DIFFTEST_SBUFFEREVENT

#ifdef CONFIG_DIFFTEST_UNCACHEMMSTOREEVENT
class UncacheMmStoreChecker : public DiffTestChecker<DifftestUncacheMmStoreEvent> {
public:
  UncacheMmStoreChecker(GetProbeFn get_probe, DiffState *state, REF_PROXY *proxy)
      : DiffTestChecker<DifftestUncacheMmStoreEvent>(get_probe, state, proxy) {}

private:
  bool get_valid(const DifftestUncacheMmStoreEvent &probe) override;
  void clear_valid(DifftestUncacheMmStoreEvent &probe) override;
  int check(const DifftestUncacheMmStoreEvent &probe) override;
};
#endif // CONFIG_DIFFTEST_UNCACHEMMSTOREEVENT

#ifdef CONFIG_DIFFTEST_ATOMICEVENT
class AtomicChecker : public DiffTestChecker<DifftestAtomicEvent> {
public:
  AtomicChecker(GetProbeFn get_probe, DiffState *state, REF_PROXY *proxy)
      : DiffTestChecker<DifftestAtomicEvent>(get_probe, state, proxy) {}

private:
  bool get_valid(const DifftestAtomicEvent &probe) override;
  void clear_valid(DifftestAtomicEvent &probe) override;
  int check(const DifftestAtomicEvent &probe) override;
};
#endif // CONFIG_DIFFTEST_ATOMICEVENT

#ifdef CONFIG_DIFFTEST_LOADEVENT
class LoadChecker : public DiffTestChecker<DifftestLoadEvent> {
public:
  LoadChecker(GetProbeFn get_probe, DiffState *state, REF_PROXY *proxy,
              std::function<const DiffTestState &()> get_dut_state)
      : DiffTestChecker<DifftestLoadEvent>(get_probe, state, proxy), get_dut_state(std::move(get_dut_state)) {}

private:
  std::function<const DiffTestState &()> get_dut_state;

  bool get_valid(const DifftestLoadEvent &probe) override;
  void clear_valid(DifftestLoadEvent &probe) override;
  int check(const DifftestLoadEvent &probe) override;
};

#ifdef CONFIG_DIFFTEST_SQUASH
class LoadSquashChecker : public SimpleChecker {
public:
  LoadSquashChecker(DiffState *state, REF_PROXY *proxy, std::function<const DiffTestState &()> get_dut_state)
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
class StoreRecorder : public DiffTestChecker<DifftestStoreEvent> {
public:
  StoreRecorder(GetProbeFn get_probe, DiffState *state, REF_PROXY *proxy)
      : DiffTestChecker<DifftestStoreEvent>(get_probe, state, proxy) {}

private:
  bool get_valid(const DifftestStoreEvent &probe) override;
  void clear_valid(DifftestStoreEvent &probe) override;
  int check(const DifftestStoreEvent &probe) override;
};

class StoreChecker : public SimpleChecker {
public:
  StoreChecker(DiffState *state, REF_PROXY *proxy) : SimpleChecker(state, proxy) {}

private:
  int check() override;
};
#endif // CONFIG_DIFFTEST_STOREEVENT

#endif // __DIFFTEST_CHECKER_H__
