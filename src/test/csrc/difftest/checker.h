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
  DiffTestChecker(DiffState *state, REF_PROXY *proxy) : state(state), proxy(proxy) {}
  virtual ~DiffTestChecker() = default;

  int step(Probe &probe, const DiffTestRegState &regs) {
    if (get_valid(probe)) {
      int ret = check(probe, regs);
      clear_valid(probe);
      return ret;
    }
    return 0;
  }

protected:
  DiffState *state;
  REF_PROXY *proxy;

  virtual bool get_valid(const Probe &probe) {
    return true;
  }
  virtual int check(const Probe &probe, const DiffTestRegState &regs) = 0;
  virtual void clear_valid(Probe &probe) {}
};

class ArchEventChecker : public DiffTestChecker<DifftestArchEvent> {
public:
  ArchEventChecker(DiffState *state, REF_PROXY *proxy)
      : DiffTestChecker<DifftestArchEvent>(state, proxy) {}

private:
  bool get_valid(const DifftestArchEvent &probe) override;
  void clear_valid(DifftestArchEvent &probe) override;
  int check(const DifftestArchEvent &probe, const DiffTestRegState &regs) override;

  int do_exception(const DifftestArchEvent &probe, const DiffTestRegState &regs);
  int do_interrupt(const DifftestArchEvent &probe);
};

class FirstInstrCommitChecker : public DiffTestChecker<DifftestInstrCommit> {
public:
  FirstInstrCommitChecker(DiffState *state, REF_PROXY *proxy)
      : DiffTestChecker<DifftestInstrCommit>(state, proxy) {}

private:
  bool get_valid(const DifftestInstrCommit &probe) override;
  void clear_valid(DifftestInstrCommit &probe) override;
  int check(const DifftestInstrCommit &probe, const DiffTestRegState &regs) override;
};

class InstrCommitChecker : public DiffTestChecker<DifftestInstrCommit> {
public:
  InstrCommitChecker(DiffState *state, REF_PROXY *proxy)
      : DiffTestChecker<DifftestInstrCommit>(state, proxy) {}
private:
  bool get_valid(const DifftestInstrCommit &probe) override;
  void clear_valid(DifftestInstrCommit &probe) override;
  int check(const DifftestInstrCommit &probe, const DiffTestRegState &regs) override;
};

#ifdef CONFIG_DIFFTEST_LRSCEVENT
class LrScChecker : public DiffTestChecker<DifftestLrScEvent> {
public:
  LrScChecker(DiffState *state, REF_PROXY *proxy)
      : DiffTestChecker<DifftestLrScEvent>(state, proxy) {}
private:
  bool get_valid(const DifftestLrScEvent &probe) override;
  void clear_valid(DifftestLrScEvent &probe) override;
  int check(const DifftestLrScEvent &probe, const DiffTestRegState &regs) override;
};
#endif // CONFIG_DIFFTEST_LRSCEVENT

#ifdef CONFIG_DIFFTEST_L1TLBEVENT
class L1TLBChecker : public DiffTestChecker<DifftestL1TLBEvent> {
public:
  L1TLBChecker(DiffState *state, REF_PROXY *proxy)
      : DiffTestChecker<DifftestL1TLBEvent>(state, proxy) {}

private:
  bool get_valid(const DifftestL1TLBEvent &probe) override;
  void clear_valid(DifftestL1TLBEvent &probe) override;
  int check(const DifftestL1TLBEvent &probe, const DiffTestRegState &regs) override;
};
#endif // CONFIG_DIFFTEST_L1TLBEVENT

#ifdef CONFIG_DIFFTEST_L2TLBEVENT
class L2TLBChecker : public DiffTestChecker<DifftestL2TLBEvent> {
public:
  L2TLBChecker(DiffState *state, REF_PROXY *proxy)
      : DiffTestChecker<DifftestL2TLBEvent>(state, proxy) {}

private:
  bool get_valid(const DifftestL2TLBEvent &probe) override;
  void clear_valid(DifftestL2TLBEvent &probe) override;
  int check(const DifftestL2TLBEvent &probe, const DiffTestRegState &regs) override;
};
#endif // CONFIG_DIFFTEST_L2TLBEVENT

#ifdef CONFIG_DIFFTEST_REFILLEVENT
class RefillChecker : public DiffTestChecker<DifftestRefillEvent> {
public:
  RefillChecker(DiffState *state, REF_PROXY *proxy)
      : DiffTestChecker<DifftestRefillEvent>(state, proxy) {}
private:
  bool get_valid(const DifftestRefillEvent &probe) override;
  void clear_valid(DifftestRefillEvent &probe) override;
  int check(const DifftestRefillEvent &probe, const DiffTestRegState &regs) override;
};
#endif // CONFIG_DIFFTEST_REFILLEVENT

#endif // __DIFFTEST_CHECKER_H__
