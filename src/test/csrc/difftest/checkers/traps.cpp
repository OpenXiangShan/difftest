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

#include "checkers.h"

enum {
  EX_IAM,       // instruction address misaligned
  EX_IAF,       // instruction address fault
  EX_II,        // illegal instruction
  EX_BP,        // breakpoint
  EX_LAM,       // load address misaligned
  EX_LAF,       // load address fault
  EX_SAM,       // store/amo address misaligned
  EX_SAF,       // store/amo address fault
  EX_ECU,       // ecall from U-mode or VU-mode
  EX_ECS,       // ecall from HS-mode
  EX_ECVS,      // ecall from VS-mode, H-extention
  EX_ECM,       // ecall from M-mode
  EX_IPF,       // instruction page fault
  EX_LPF,       // load page fault
  EX_RS0,       // reserved
  EX_SPF,       // store/amo page fault
  EX_DT,        // double trap
  EX_RS1,       // reserved
  EX_SWC,       // software check
  EX_HWE,       // hardware error
  EX_IGPF = 20, // instruction guest-page fault, H-extention
  EX_LGPF,      // load guest-page fault, H-extention
  EX_VI,        // virtual instruction, H-extention
  EX_SGPF       // store/amo guest-page fault, H-extention
};

bool ArchEventChecker::get_valid(const DifftestArchEvent &probe) {
  return probe.valid;
}

void ArchEventChecker::clear_valid(DifftestArchEvent &probe) {
  probe.valid = 0;
  state->has_progress = true;
}

int ArchEventChecker::check(const DifftestArchEvent &probe) {
  // interrupt has a higher priority than exception
  // TODO: we should ensure they don't happen at the same time
  if (probe.interrupt) {
    return do_interrupt(probe);
  } else {
    return do_exception(probe);
  }
}

int ArchEventChecker::do_interrupt(const DifftestArchEvent &probe) {
  state->record_interrupt(probe.exceptionPC, probe.exceptionInst, probe.interrupt);
  if (probe.hasNMI) {
    proxy->trigger_nmi(probe.hasNMI);
  } else if (probe.virtualInterruptIsHvictlInject) {
    proxy->virtual_interrupt_is_hvictl_inject(probe.virtualInterruptIsHvictlInject);
  }
  struct InterruptDelegate intrDeleg;
  intrDeleg.irToHS = probe.irToHS;
  intrDeleg.irToVS = probe.irToVS;
  proxy->intr_delegate(intrDeleg);
  proxy->raise_intr(probe.interrupt | (1ULL << 63));
  return STATE_OK;
}

int ArchEventChecker::do_exception(const DifftestArchEvent &probe) {
  state->record_exception(probe.exceptionPC, probe.exceptionInst, probe.exception);
  if (probe.exception == EX_IPF || probe.exception == EX_LPF || probe.exception == EX_SPF ||
      probe.exception == EX_IGPF || probe.exception == EX_LGPF || probe.exception == EX_SGPF ||
      probe.exception == EX_HWE) {
    const auto &regs = get_regs();
    struct ExecutionGuide guide;
    guide.force_raise_exception = true;
    guide.exception_num = probe.exception;
    guide.mtval = regs.csr.mtval;
    guide.stval = regs.csr.stval;
#ifdef CONFIG_DIFFTEST_HCSRSTATE
    guide.mtval2 = regs.hcsr.mtval2;
    guide.htval = regs.hcsr.htval;
    guide.vstval = regs.hcsr.vstval;
#endif // CONFIG_DIFFTEST_HCSRSTATE
    guide.force_set_jump_target = false;
    proxy->guided_exec(guide);
  } else {
#ifdef DEBUG_MODE_DIFF
    if (DEBUG_MEM_REGION(true, probe.exceptionPC)) {
      debug_mode_copy(probe.exceptionPC, 4, probe.exceptionInst);
    }
#endif
    proxy->ref_exec(1);
  }

#ifdef FUZZING
  static uint64_t lastExceptionPC = 0xdeadbeafUL;
  static int sameExceptionPCCount = 0;
  if (probe.exceptionPC == lastExceptionPC) {
    if (sameExceptionPCCount >= 5) {
      Info("Found infinite loop at exception_pc %lx. Exiting.\n", probe.exceptionPC);
      state->raise_trap(STATE_FUZZ_COND);
#ifdef FUZZER_LIB
      stats.exit_code = SimExitCode::exception_loop;
#endif // FUZZER_LIB
      return STATE_TRAP;
    }
    sameExceptionPCCount++;
  }
  if (!sameExceptionPCCount && probe.exceptionPC != lastExceptionPC) {
    sameExceptionPCCount = 0;
  }
  lastExceptionPC = probe.exceptionPC;
#endif // FUZZING

  return STATE_OK;
}

#ifdef CONFIG_DIFFTEST_CRITICALERROREVENT
bool CriticalErrorChecker::get_valid(const DifftestCriticalErrorEvent &probe) {
  return probe.valid;
}

void CriticalErrorChecker::clear_valid(DifftestCriticalErrorEvent &probe) {
  probe.valid = 0;
}

int CriticalErrorChecker::check(const DifftestCriticalErrorEvent &probe) {
  bool ref_critical_error = proxy->raise_critical_error();
  if (ref_critical_error == probe.criticalError) {
    Info("Core %d dump: " ANSI_COLOR_RED
         "HIT CRITICAL ERROR: please check if software cause a double trap. \n" ANSI_COLOR_RESET,
         state->coreid);
    state->raise_trap(STATE_GOODTRAP);
  } else {
    Info("Core %d dump: DUT critical_error diff REF \n", state->coreid);
    state->raise_trap(STATE_ABORT);
  }
  return STATE_TRAP;
}
#endif
