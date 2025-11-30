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
  virtual int check(const Probe &probe) = 0;
  virtual void clear_valid(Probe &probe) {}
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
#endif // CONFIG_DIFFTEST_REFILLEVENT

#endif // __DIFFTEST_CHECKER_H__
