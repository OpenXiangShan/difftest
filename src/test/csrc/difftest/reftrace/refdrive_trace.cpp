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
  entry.state = RefDriveState(dut);
  entry.terminal = dut.trap.hasTrap;
#ifdef CONFIG_DIFFTEST_CRITICALERROREVENT
  entry.terminal |= bool(dut.critical_error.valid);
#endif
  for (int i = 0; i < CONFIG_DIFF_COMMIT_WIDTH; ++i) {
    const auto &c = dut.commit[i];
    if (!c.valid || !c.skip)
      continue;
    if (c.vecwen || c.nFused)
      throw std::runtime_error("RefDriveTrace does not support vector or fused skips");
  }
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
