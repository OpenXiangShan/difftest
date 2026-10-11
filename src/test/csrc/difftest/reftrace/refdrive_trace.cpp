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
#include "fork_ref/ref_drive.h"
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
    const uint64_t word = (count << 8) | uint8_t(RefDriveTraceOp::Exec);
    recorder->append(&word);
    pending -= count;
  }
}

static void emit(RefDriveTraceOp op, uint64_t arg = 0) {
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
#ifndef CONFIG_REF_DRIVE
  if (args.refdrive_trace_is_read)
    throw std::runtime_error("Use ref-drive to load RefDriveTrace");
#endif
  if (!args.refdrive_trace_name || args.refdrive_trace_is_read)
    return;
#ifndef CONFIG_DIFFTEST_FAST_REF
  throw std::runtime_error("RefDriveTrace requires DIFFTEST_FAST_REF=1");
#else
  if (args.ref_mode == RefMode::FORK)
    throw std::runtime_error("RefDriveTrace does not support --ref-mode=fork");
#endif
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
  if (!recorder || finished || (!initialized && !dut.commit[0].valid))
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
#ifdef CONFIG_DIFFTEST_FAST_REF
  if (!initialized) {
    emit(RefDriveTraceOp::Init);
    payload(dut.regs);
    initialized = true;
  }
  const RefDriveState state(dut);
  for (unsigned i = 0; i < state.num_commands; ++i) {
    const auto &c = state.commands[i];
    switch (c.op) {
      case RefDriveOp::EXEC:
        pending += c.instr_count;
        if (pending >= REFDRIVE_BATCH_LIMIT)
          flush_exec();
        break;
      case RefDriveOp::SKIP: {
        const auto &s = c.skip;
        if (s.vector)
          throw std::runtime_error("RefDriveTrace does not support vector skips");
        const bool wen = s.integer || s.floating;
        const uint64_t arg = s.rvc | (s.integer << 1) | (s.floating << 2) | (uint64_t(wen ? s.dest : 0) << 3);
        if (!refdrive_valid_skip(arg))
          throw std::runtime_error("Invalid scalar skip destination");
        emit(RefDriveTraceOp::Skip, arg);
        if (wen)
          payload(s.data);
        break;
      }
#ifdef CONFIG_DIFFTEST_LRSCEVENT
      case RefDriveOp::LRSC: emit(RefDriveTraceOp::LrSc, c.lrsc.sc_fail); break;
#endif
#ifdef CONFIG_DIFFTEST_NONREGINTERRUPTPENDINGEVENT
      case RefDriveOp::PENDING: {
        const auto &p = c.pending;
        emit(RefDriveTraceOp::InterruptPending,
             p.platformIRPMeip | (p.platformIRPMtip << 1) | (p.platformIRPMsip << 2) | (p.platformIRPSeip << 3) |
                 (p.platformIRPStip << 4) | (p.platformIRPVseip << 5) | (p.platformIRPVstip << 6) |
                 (p.fromAIAMeip << 7) | (p.fromAIASeip << 8) | (p.localCounterOverflowInterruptReq << 9));
        break;
      }
#endif
#ifdef CONFIG_DIFFTEST_MHPMEVENTOVERFLOWEVENT
      case RefDriveOp::OVERFLOW:
        emit(RefDriveTraceOp::Overflow);
        payload(c.overflow);
        break;
#endif
#ifdef CONFIG_DIFFTEST_SYNCAIAEVENT
      case RefDriveOp::AIA:
        emit(RefDriveTraceOp::Aia);
        payload(c.aia);
        break;
#endif
#ifdef CONFIG_DIFFTEST_SYNCCUSTOMMFLUSHPWREVENT
      case RefDriveOp::MFLUSH: emit(RefDriveTraceOp::Mflush, c.mflush); break;
#endif
#ifdef CONFIG_DIFFTEST_CRITICALERROREVENT
      case RefDriveOp::CRITICAL:
        emit(RefDriveTraceOp::CriticalError, c.critical);
        emit(RefDriveTraceOp::End);
        finished = true;
        return;
#endif
#ifdef CONFIG_DIFFTEST_ARCHEVENT
      case RefDriveOp::ARCH_EVENT: {
        const auto &e = c.event;
        if (e.interrupt) {
          const uint64_t flags =
              e.nmi | (e.virtual_inject << 1) | (e.delegation.irToHS << 2) | (e.delegation.irToVS << 3);
          emit(RefDriveTraceOp::Interrupt, e.interrupt | (flags << REFDRIVE_INTERRUPT_BITS));
        } else {
          emit(RefDriveTraceOp::Exception, dut.event.exception);
          if (e.guided) {
            const uint64_t values[] = {
              e.guide.mtval,  e.guide.stval,
#ifdef CONFIG_DIFFTEST_HCSRSTATE
              e.guide.mtval2, e.guide.htval, e.guide.vstval,
#endif
            };
            payload(values);
          }
        }
        break;
      }
#endif
      case RefDriveOp::END:
        emit(RefDriveTraceOp::End);
        finished = true;
        return;
      default: throw std::runtime_error("Unsupported native REF drive operation");
    }
  }
#endif
}

void difftest_refdrive_trace_finish() {
  if (!recorder)
    return;
  if (!finished)
    emit(RefDriveTraceOp::End);
  recorder.reset();
}

void difftest_refdrive_trace_check_load_correction() {
  if (recorder)
    throw std::runtime_error("RefDriveTrace cannot omit load/golden-memory corrections");
}
