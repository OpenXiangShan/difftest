/***************************************************************************************
* Copyright (c) 2020-2026 Institute of Computing Technology, Chinese Academy of Sciences
* Copyright (c) 2026 Beijing Institute of Open Source Chip (BOSC)
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

#include "refdrive_trace.h"
#include "diffstate.h"
#include "refproxy.h"
#include <algorithm>
#include <array>
#include <memory>
#include <stdexcept>

static std::unique_ptr<DiffTrace<uint64_t>> recorder;
static uint64_t pending = 0;
static bool initialized = false, finished = false;

static void flush_exec() {
  while (pending) {
    const auto count = std::min(pending, REFDRIVE_BATCH_LIMIT);
    const uint64_t word = (count << 8) | uint8_t(RefDriveOp::Exec);
    recorder->append(&word);
    pending -= count;
  }
}

static void emit(RefDriveOp op, uint64_t arg = 0) {
  flush_exec();
  const uint64_t word = (arg << 8) | uint8_t(op);
  recorder->append(&word);
}

template <typename T> static void payload(const T &data) {
  std::array<uint64_t, (sizeof(T) + 7) / 8> words{};
  memcpy(words.data(), &data, sizeof(T));
  for (const auto &word: words)
    recorder->append(&word);
}

void difftest_refdrive_trace_configure(const CommonArgs &args) {
  if (!args.refdrive_trace_name)
    return;
  if (!args.enable_diff)
    throw std::runtime_error("RefDriveTrace requires differential testing");
  if (NUM_CORES != 1 || args.enable_fork || args.gcpt_restore || args.snapshot_path || args.image_as_footprints ||
      args.cst_file || args.copy_ram_offset)
    throw std::runtime_error("RefDriveTrace requires a single core booting from its original image");
#if (defined(DEBUG_REFILL) && defined(CONFIG_DIFFTEST_REFILLEVENT)) || defined(DEBUG_MODE_DIFF)
  throw std::runtime_error("RefDriveTrace does not support debug memory corrections");
#endif
#ifdef CONFIG_DIFFTEST_REPLAY
  throw std::runtime_error("RefDriveTrace does not support RTL replay/REF rollback");
#endif
  recorder = std::make_unique<DiffTrace<uint64_t>>(args.refdrive_trace_name, false, 4096);
  pending = 0;
  initialized = finished = false;
}

void difftest_refdrive_trace_record(const DiffTestState &dut) {
  if (!recorder || finished)
    return;
#ifdef CONFIG_DIFFTEST_AMUCTRLEVENT
  for (const auto &event: dut.amu_ctrl)
    if (event.valid)
      throw std::runtime_error("RefDriveTrace does not support AMU execution");
#endif
#ifdef CONFIG_DIFFTEST_MSYNCEVENT
  for (const auto &event: dut.msync)
    if (event.valid)
      throw std::runtime_error("RefDriveTrace does not support msync");
#endif
  if (!initialized && dut.commit[0].valid) {
    emit(RefDriveOp::Init);
    payload(dut.regs);
    initialized = true;
  }
  // Preserve the synchronization order used by Difftest::init_checkers().
#ifdef CONFIG_DIFFTEST_LRSCEVENT
  if (dut.lrsc.valid)
    emit(RefDriveOp::LrSc, !dut.lrsc.success);
#endif
#ifdef CONFIG_DIFFTEST_NONREGINTERRUPTPENDINGEVENT
  if (dut.non_reg_interrupt_pending.valid) {
    const auto &ip = dut.non_reg_interrupt_pending;
    emit(RefDriveOp::InterruptPending,
         ip.platformIRPMeip | (ip.platformIRPMtip << 1) | (ip.platformIRPMsip << 2) | (ip.platformIRPSeip << 3) |
             (ip.platformIRPStip << 4) | (ip.platformIRPVseip << 5) | (ip.platformIRPVstip << 6) |
             (ip.fromAIAMeip << 7) | (ip.fromAIASeip << 8) | (ip.localCounterOverflowInterruptReq << 9));
  }
#endif
#ifdef CONFIG_DIFFTEST_MHPMEVENTOVERFLOWEVENT
  if (dut.mhpmevent_overflow.valid) {
    emit(RefDriveOp::Overflow);
    payload(dut.mhpmevent_overflow.mhpmeventOverflow);
  }
#endif
#ifdef CONFIG_DIFFTEST_CRITICALERROREVENT
  if (dut.critical_error.valid) {
    emit(RefDriveOp::CriticalError, bool(dut.critical_error.criticalError));
    emit(RefDriveOp::End);
    finished = true;
    return;
  }
#endif
#ifdef CONFIG_DIFFTEST_SYNCAIAEVENT
  if (dut.sync_aia.valid) {
    emit(RefDriveOp::Aia);
    const auto &aia = dut.sync_aia;
    const FromAIA data{aia.mtopei, aia.stopei, aia.vstopei, aia.hgeip};
    payload(data);
  }
#endif
#ifdef CONFIG_DIFFTEST_SYNCCUSTOMMFLUSHPWREVENT
  if (dut.sync_custom_mflushpwr.valid)
    emit(RefDriveOp::Mflush, bool(dut.sync_custom_mflushpwr.l2FlushDone));
#endif
#ifdef CONFIG_DIFFTEST_ARCHEVENT
  if (dut.event.valid) {
    if (!initialized)
      throw std::runtime_error("REF execution precedes the first commit");
    const auto &event = dut.event;
    if (event.interrupt) {
      const uint64_t flags = event.hasNMI | (bool(event.virtualInterruptIsHvictlInject) << 1) |
                             (bool(event.irToHS) << 2) | (bool(event.irToVS) << 3);
      emit(RefDriveOp::Interrupt, event.interrupt | (flags << REFDRIVE_INTERRUPT_BITS));
    } else {
      emit(RefDriveOp::Exception, event.exception);
      if (refdrive_guided_exception(event.exception)) {
        const uint64_t data[] = {
          dut.regs.csr.mtval,   dut.regs.csr.stval,
#ifdef CONFIG_DIFFTEST_HCSRSTATE
          dut.regs.hcsr.mtval2, dut.regs.hcsr.htval, dut.regs.hcsr.vstval,
#endif
        };
        payload(data);
      }
    }
  } else
#endif
  {
    for (int i = 0; i < CONFIG_DIFF_COMMIT_WIDTH; ++i) {
      const auto &commit = dut.commit[i];
      if (!commit.valid)
        continue;
      if (!initialized)
        throw std::runtime_error("REF commit precedes initialization");
      if (commit.skip) {
        if (commit.vecwen)
          throw std::runtime_error("RefDriveTrace does not support vector skips");
        const bool rfwen = commit.rfwen && commit.wdest != 0;
        const bool wen = rfwen || commit.fpwen;
        const uint64_t arg =
            commit.isRVC | (rfwen << 1) | (bool(commit.fpwen) << 2) | (uint64_t(wen ? commit.wdest : 0) << 3);
        if (!refdrive_valid_skip(arg))
          throw std::runtime_error("Invalid scalar skip destination");
        emit(RefDriveOp::Skip, arg);
        if (wen)
          payload(get_commit_data(&dut, i));
      } else {
        pending += uint64_t(commit.nFused) + 1;
        if (pending >= REFDRIVE_BATCH_LIMIT)
          flush_exec();
      }
    }
  }
#ifdef CONFIG_DIFFTEST_TRAPEVENT
  if (dut.trap.hasTrap) {
    emit(RefDriveOp::End);
    finished = true;
  }
#endif
}

void difftest_refdrive_trace_finish() {
  if (!recorder)
    return;
  if (!finished)
    emit(RefDriveOp::End);
  recorder.reset();
}

void difftest_refdrive_trace_check_load_correction() {
  if (recorder)
    throw std::runtime_error("RefDriveTrace cannot omit load/golden-memory corrections");
}
