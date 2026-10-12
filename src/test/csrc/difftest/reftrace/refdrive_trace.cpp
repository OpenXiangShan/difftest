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
#include <memory>
#include <stdexcept>
#include <type_traits>

static void refdrive_check_flag(uint64_t value) {
  if (value > 1)
    throw std::runtime_error("Invalid RefDriveTrace boolean field");
}

void refdrive_validate_entry(const RefDriveTraceEntry &entry) {
  const auto terminal = entry.terminal;
  refdrive_check_flag(terminal);
  const auto &state = entry.state;
  for (const auto &c: state.commit) {
    refdrive_check_flag(c.valid);
    refdrive_check_flag(c.skip);
    refdrive_check_flag(c.isRVC);
    refdrive_check_flag(c.rfwen);
    refdrive_check_flag(c.fpwen);
    refdrive_check_flag(c.vecwen);
    if (!c.valid || !c.skip)
      continue;
    if (c.vecwen)
      throw std::runtime_error("RefDriveTrace does not support vector skips");
    if (c.nFused)
      throw std::runtime_error("RefDriveTrace does not support fused skips");
    if (c.rfwen && c.wdest >= std::extent_v<decltype(DifftestArchIntRegState::value)>)
      throw std::runtime_error("Invalid scalar skip destination");
#ifdef CONFIG_DIFFTEST_ARCHFPREGSTATE
    if (c.fpwen && c.wdest >= std::extent_v<decltype(DifftestArchFpRegState::value)>)
      throw std::runtime_error("Invalid scalar skip destination");
#else
    if (c.fpwen)
      throw std::runtime_error("RefDriveTrace profile does not support FP skips");
#endif
  }
#ifdef CONFIG_DIFFTEST_ARCHEVENT
  refdrive_check_flag(state.event.valid);
  refdrive_check_flag(state.event.isLatter);
  refdrive_check_flag(state.event.hasNMI);
  refdrive_check_flag(state.event.virtualInterruptIsHvictlInject);
  refdrive_check_flag(state.event.irToHS);
  refdrive_check_flag(state.event.irToVS);
#endif
#ifdef CONFIG_DIFFTEST_LRSCEVENT
  refdrive_check_flag(state.lrsc.valid);
  refdrive_check_flag(state.lrsc.success);
#endif
#ifdef CONFIG_DIFFTEST_NONREGINTERRUPTPENDINGEVENT
  const auto &p = state.non_reg_interrupt_pending;
  refdrive_check_flag(p.valid);
  refdrive_check_flag(p.platformIRPMeip);
  refdrive_check_flag(p.platformIRPMtip);
  refdrive_check_flag(p.platformIRPMsip);
  refdrive_check_flag(p.platformIRPSeip);
  refdrive_check_flag(p.platformIRPStip);
  refdrive_check_flag(p.platformIRPVseip);
  refdrive_check_flag(p.platformIRPVstip);
  refdrive_check_flag(p.fromAIAMeip);
  refdrive_check_flag(p.fromAIASeip);
  refdrive_check_flag(p.localCounterOverflowInterruptReq);
#endif
#ifdef CONFIG_DIFFTEST_MHPMEVENTOVERFLOWEVENT
  refdrive_check_flag(state.mhpmevent_overflow.valid);
#endif
#ifdef CONFIG_DIFFTEST_CRITICALERROREVENT
  refdrive_check_flag(state.critical_error.valid);
  refdrive_check_flag(state.critical_error.criticalError);
#endif
#ifdef CONFIG_DIFFTEST_SYNCAIAEVENT
  refdrive_check_flag(state.sync_aia.valid);
#endif
#ifdef CONFIG_DIFFTEST_SYNCCUSTOMMFLUSHPWREVENT
  refdrive_check_flag(state.sync_custom_mflushpwr.valid);
  refdrive_check_flag(state.sync_custom_mflushpwr.l2FlushDone);
#endif
  refdrive_check_flag(state.trap.hasTrap);
  refdrive_check_flag(state.trap.hasWFI);
  bool terminal_event = state.trap.hasTrap;
#ifdef CONFIG_DIFFTEST_CRITICALERROREVENT
  terminal_event |= bool(state.critical_error.valid);
#endif
  if (!terminal && terminal_event)
    throw std::runtime_error("Missing terminal marker on RefDriveTrace terminal event");
  if (terminal && !terminal_event) {
    const uint8_t *bytes = reinterpret_cast<const uint8_t *>(&state);
    for (size_t i = 0; i < sizeof(state); ++i)
      if (bytes[i])
        throw std::runtime_error("Invalid RefDriveTrace terminal record");
    for (auto data: entry.commit_data)
      if (data)
        throw std::runtime_error("Invalid RefDriveTrace terminal writeback data");
  }
}

static std::unique_ptr<DiffTrace<RefDriveTraceEntry>> recorder;
static bool initialized = false, finished = false;

void difftest_refdrive_trace_configure(const CommonArgs &args) {
  if (!args.refdrive_trace_name)
    return;
#ifndef CONFIG_REF_DRIVE
  if (args.refdrive_trace_is_read)
    throw std::runtime_error("Use ref-drive to load RefDriveTrace");
#endif
#ifndef CONFIG_DIFFTEST_FAST_REF
  throw std::runtime_error("RefDriveTrace requires DIFFTEST_FAST_REF=1");
#else
  if (!args.refdrive_trace_is_read && args.ref_mode == RefMode::FORK)
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
  if (args.refdrive_trace_is_read)
    return;
  recorder = std::make_unique<DiffTrace<RefDriveTraceEntry>>(args.refdrive_trace_name, false, 4096);
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
  RefDriveTraceEntry entry;
  entry.state = DiffRefDriveState(dut);
  entry.terminal = dut.trap.hasTrap;
#ifdef CONFIG_DIFFTEST_CRITICALERROREVENT
  entry.terminal |= bool(dut.critical_error.valid);
#endif
  refdrive_validate_entry(entry);
  for (int i = 0; i < CONFIG_DIFF_COMMIT_WIDTH; ++i)
    if (dut.commit[i].valid && dut.commit[i].skip && (dut.commit[i].rfwen || dut.commit[i].fpwen))
      entry.commit_data[i] = get_commit_data(&dut, i);
  recorder->append(&entry);
  initialized = true;
  finished = entry.terminal;
}

void difftest_refdrive_trace_finish() {
  if (!recorder)
    return;
  if (!finished) {
    RefDriveTraceEntry entry;
    entry.terminal = true;
    recorder->append(&entry);
  }
  recorder.reset();
}

void difftest_refdrive_trace_check_load_correction() {
  if (recorder)
    throw std::runtime_error("RefDriveTrace cannot omit load/golden-memory corrections");
}
