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

#include <chrono>
#include <iostream>
#include <vector>
#include <iomanip>
#include <typeinfo>
#include <cxxabi.h>
#include <algorithm>
#include <map>

class DiffTestChecker {
public:
  DiffTestChecker(DiffState *state, RefProxy *proxy) : state(state), proxy(proxy){
    all_checkers.push_back(this);
  }
  virtual ~DiffTestChecker() = default;

  int step() {
    if (!name_init) {
      int status;
      char* realname = abi::__cxa_demangle(typeid(*this).name(), 0, 0, &status);
      name = (status == 0) ? std::string(realname) : std::string(typeid(*this).name());
      free(realname);
      name_init = true;
    }

    auto start = std::chrono::steady_clock::now();
    int ret = do_step();
    auto end = std::chrono::steady_clock::now();
    total_time += std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    return ret;
  }

  virtual int do_step() = 0;

  static void print_stats() {
      std::map<std::string, long long> stats;
      long long total_sum = 0;

      for (auto* c : all_checkers) {
          if (c->total_time > 0) {
              stats[c->name] += c->total_time;
              total_sum += c->total_time;
          }
      }

      using Pair = std::pair<std::string, long long>;
      std::vector<Pair> sorted_stats(stats.begin(), stats.end());

      // sort by time descending
      std::sort(sorted_stats.begin(), sorted_stats.end(), [](const Pair& a, const Pair& b){
          return a.second > b.second;
      });

      std::cout << "\n===== DiffTest Checker Performance Stats =====" << std::endl;
      std::cout << std::left << std::setw(40) << "Checker Name" << "Time (us)" << std::endl;
      std::cout << std::string(60, '-') << std::endl;

      for (const auto& p : sorted_stats) {
          std::cout << std::left << std::setw(40) << p.first
                  << p.second << std::endl;
      }
      std::cout << std::string(60, '-') << std::endl;
      std::cout << std::left << std::setw(40) << "TOTAL" << total_sum << std::endl;
      std::cout << "==============================================\n" << std::endl;
  }

  static const int STATE_OK = 0;
  static const int STATE_DIFF = 1;
  static const int STATE_ERROR = 2;
  static const int STATE_TRAP = 3;

protected:
  DiffState *state;
  RefProxy *proxy;

  std::string name;
  bool name_init = false;
  long long total_time = 0;

public:
  static std::vector<DiffTestChecker*> all_checkers;
};

class SimpleChecker : public DiffTestChecker {
public:
  SimpleChecker(DiffState *state, RefProxy *proxy) : DiffTestChecker(state, proxy) {}
  virtual ~SimpleChecker() = default;

  virtual int do_step() override {
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

  virtual int do_step() override {
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
  void display_sbuffer();
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

  bool get_valid(const DifftestLoadEvent &probe) override;
  void clear_valid(DifftestLoadEvent &probe) override;
  int check(const DifftestLoadEvent &probe) override;

#ifndef CONFIG_DIFFTEST_SQUASH
  int do_load_check(const DifftestLoadEvent &probe, bool regWen, uint64_t *refRegPtr, uint64_t commitData);
#ifdef CONFIG_DIFFTEST_ARCHVECREGSTATE
  bool enable_vec_load_goldenmem_check = proxy->check_ref_vec_load_goldenmem();
  int do_vec_load_check(const DifftestLoadEvent &probe, uint8_t firstLdest, const uint64_t *commitData);
#endif // CONFIG_DIFFTEST_ARCHVECREGSTATE
#endif // CONFIG_DIFFTEST_SQUASH
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

  int do_load_check(const DifftestLoadEvent &probe, bool regWen, uint64_t *refRegPtr, uint64_t commitData);
#ifdef CONFIG_DIFFTEST_ARCHVECREGSTATE
  bool enable_vec_load_goldenmem_check = proxy->check_ref_vec_load_goldenmem();
  int do_vec_load_check(const DifftestLoadEvent &probe, uint8_t firstLdest, const uint64_t *commitData);
#endif // CONFIG_DIFFTEST_ARCHVECREGSTATE
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
