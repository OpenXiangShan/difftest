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
#include "refdrive.h"
#ifdef CONFIG_DIFFTEST_FAST_REF
#include "diffstate.h"
#include <algorithm>

RefDriveState::Command &RefDriveState::append(Kind kind) {
  assert(count < MAX_COMMANDS);
  commands[count].kind = kind;
  return commands[count++];
}

RefDriveState::RefDriveState(const DiffTestState &dut) {
#ifdef CONFIG_DIFFTEST_LRSCEVENT
  if (dut.lrsc.valid)
    append(LRSC).lrsc = {uint64_t(!dut.lrsc.success)};
#endif
#ifdef CONFIG_DIFFTEST_NONREGINTERRUPTPENDINGEVENT
  if (dut.non_reg_interrupt_pending.valid) {
    const auto &p = dut.non_reg_interrupt_pending;
    append(PENDING).pending = {bool(p.platformIRPMeip),  bool(p.platformIRPMtip),
                               bool(p.platformIRPMsip),  bool(p.platformIRPSeip),
                               bool(p.platformIRPStip),  bool(p.platformIRPVseip),
                               bool(p.platformIRPVstip), bool(p.fromAIAMeip),
                               bool(p.fromAIASeip),      bool(p.localCounterOverflowInterruptReq)};
  }
#endif
#ifdef CONFIG_DIFFTEST_MHPMEVENTOVERFLOWEVENT
  if (dut.mhpmevent_overflow.valid)
    append(OVERFLOW).overflow = dut.mhpmevent_overflow.mhpmeventOverflow;
#endif
#ifdef CONFIG_DIFFTEST_CRITICALERROREVENT
  if (dut.critical_error.valid) {
    append(CRITICAL).critical = dut.critical_error.criticalError;
    return;
  }
#endif
#ifdef CONFIG_DIFFTEST_SYNCAIAEVENT
  if (dut.sync_aia.valid) {
    const auto &a = dut.sync_aia;
    append(AIA).aia = {a.mtopei, a.stopei, a.vstopei, a.hgeip};
  }
#endif
#ifdef CONFIG_DIFFTEST_SYNCCUSTOMMFLUSHPWREVENT
  if (dut.sync_custom_mflushpwr.valid)
    append(MFLUSH).mflush = dut.sync_custom_mflushpwr.l2FlushDone;
#endif
  for (int i = 0; i < CONFIG_DIFF_COMMIT_WIDTH; ++i) {
    if (dut.event.valid && i == dut.event.isLatter) {
      arch_event = true;
      auto &e = append(ARCH_EVENT).event;
      e.interrupt = dut.event.interrupt;
      e.nmi = dut.event.hasNMI;
      e.virtual_inject = dut.event.virtualInterruptIsHvictlInject;
      e.delegation = {bool(dut.event.irToHS), bool(dut.event.irToVS)};
      const auto exception = dut.event.exception;
      enum {
        IPF = 12,
        LPF = 13,
        SPF = 15,
        HWE = 19,
        IGPF = 20,
        LGPF = 21,
        SGPF = 23
      };
      e.guided = exception == IPF || exception == LPF || exception == SPF || exception == IGPF || exception == LGPF ||
                 exception == SGPF || exception == HWE;
      if (!e.interrupt && e.guided) {
        e.guide = {};
        e.guide.force_raise_exception = true;
        e.guide.exception_num = exception;
        e.guide.mtval = dut.regs.csr.mtval;
        e.guide.stval = dut.regs.csr.stval;
#ifdef CONFIG_DIFFTEST_HCSRSTATE
        e.guide.mtval2 = dut.regs.hcsr.mtval2;
        e.guide.htval = dut.regs.hcsr.htval;
        e.guide.vstval = dut.regs.hcsr.vstval;
#endif
      }
      break;
    }
    const auto &c = dut.commit[i];
    if (!c.valid)
      continue;
    committed += 1 + c.nFused;
    if (c.skip) {
      append(SKIP).skip = {get_commit_data(&dut, i),      c.wdest,       bool(c.isRVC),
                           bool(c.rfwen && c.wdest != 0), bool(c.fpwen), bool(c.vecwen)};
    } else if (count && commands[count - 1].kind == EXEC) {
      commands[count - 1].count += 1 + c.nFused;
    } else {
      append(EXEC).count = 1 + c.nFused;
    }
  }
  if (dut.trap.hasTrap)
    append(END);
}

bool RefDriveExecutor::flush() {
  if (!pending)
    return true;
  const uint64_t requested = pending;
  pending = 0;
  const uint64_t before = proxy->ref_get_instr_count();
  proxy->ref_exec(requested);
  const uint64_t actual = proxy->ref_get_instr_count() - before;
  executed += actual;
  if (actual != requested) {
    proxy->sync();
    fprintf(stderr, "FAST short execution: requested=%lu completed=%lu PC=0x%lx\n", (unsigned long)requested,
            (unsigned long)actual, (unsigned long)proxy->state.pc);
    return false;
  }
  return true;
}

RefDriveExecutor::Result RefDriveExecutor::run(const RefDriveState &state) {
  for (unsigned i = 0; i < state.count; ++i) {
    const auto &c = state.commands[i];
    if (c.kind == RefDriveState::EXEC) {
      uint64_t remaining = c.count;
      while (remaining) {
        const uint64_t batch = std::min(remaining, MAX_PENDING - pending);
        pending += batch;
        remaining -= batch;
        if (pending == MAX_PENDING && !flush())
          return ERROR;
      }
      continue;
    }
    if (!flush())
      return ERROR;
    switch (c.kind) {
      case RefDriveState::SKIP: {
        const auto &s = c.skip;
        proxy->skip_one(s.rvc, s.integer, s.floating, s.vector, s.dest, s.data);
        break;
      }
      case RefDriveState::LRSC: {
        auto sync = c.lrsc;
        proxy->uarchstatus_sync(reinterpret_cast<uint64_t *>(&sync));
        break;
      }
      case RefDriveState::PENDING: {
        auto pending = c.pending;
        proxy->non_reg_interrupt_pending(pending);
        break;
      }
      case RefDriveState::OVERFLOW: proxy->mhpmevent_overflow(c.overflow); break;
      case RefDriveState::CRITICAL:
        return proxy->raise_critical_error() == c.critical ? CRITICAL_MATCH : CRITICAL_MISMATCH;
      case RefDriveState::AIA: {
        auto aia = c.aia;
        proxy->sync_aia(aia);
        break;
      }
      case RefDriveState::MFLUSH: proxy->sync_custom_mflushpwr(c.mflush); break;
      case RefDriveState::ARCH_EVENT: {
        const auto &e = c.event;
        if (e.interrupt) {
          if (e.nmi)
            proxy->trigger_nmi(true);
          else if (e.virtual_inject)
            proxy->virtual_interrupt_is_hvictl_inject(true);
          auto delegation = e.delegation;
          proxy->intr_delegate(delegation);
          proxy->raise_intr(e.interrupt | (1ULL << 63));
        } else if (e.guided) {
          auto guide = e.guide;
          proxy->guided_exec(guide);
        } else {
          proxy->ref_exec(1);
        }
        break;
      }
      case RefDriveState::END: return OK;
      case RefDriveState::EXEC: break;
    }
  }
  return OK;
}
#endif
