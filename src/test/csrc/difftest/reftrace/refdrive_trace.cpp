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
#include "difftest.h"
#include <atomic>
#include <memory>
#include <stdexcept>
#include <unistd.h>

static std::unique_ptr<DiffTrace<RefDriveTraceEntry>> recorder;
static bool initialized = false;
static std::atomic<bool> finished{false};
static pid_t recorder_pid = 0;

void difftest_refdrive_trace_configure(const CommonArgs &args) {
  if (!args.refdrive_trace_name)
    return;
#ifndef CONFIG_REF_DRIVE
  if (args.refdrive_trace_is_read)
    throw std::runtime_error("Use ref-drive to load RefDriveTrace");
#endif
  if (!args.enable_diff)
    throw std::runtime_error("RefDriveTrace requires differential testing");
  if (NUM_CORES != 1 || args.enable_fork || args.gcpt_restore || args.snapshot_path || args.image_as_footprints ||
      args.cst_file || args.copy_ram_offset)
    throw std::runtime_error("RefDriveTrace requires a single core booting from its original image");
#ifdef CONFIG_DIFFTEST_REPLAY
  throw std::runtime_error("RefDriveTrace does not support RTL replay/REF rollback");
#endif
  if (args.refdrive_trace_is_read)
    return;
  recorder = std::make_unique<DiffTrace<RefDriveTraceEntry>>(args.refdrive_trace_name, false, 4096);
  recorder_pid = getpid();
  initialized = finished = false;
}

bool difftest_refdrive_trace_pending() {
  // Forked REF workers inherit the recorder; only the creating process may write or wait for it.
  return recorder && recorder_pid == getpid() && !finished;
}

void difftest_refdrive_trace_dump(int step) {
#if CONFIG_DIFFTEST_ZONESIZE > 1
  difftest_switch_zone();
#endif
  for (int i = 0; i < step && difftest_refdrive_trace_pending(); i++) {
    difftest_set_dut();
  }
}

void difftest_refdrive_trace_record(const DiffTestState &dut) {
  if (!difftest_refdrive_trace_pending() || (!initialized && !dut.commit[0].valid))
    return;
  RefDriveTraceEntry entry;
  entry.state = RefDriveState(dut);
  entry.terminal = dut.trap.hasTrap;
#ifdef CONFIG_DIFFTEST_CRITICALERROREVENT
  entry.terminal |= bool(dut.critical_error.valid);
#endif
  recorder->append(&entry);
  initialized = true;
  finished = entry.terminal;
}

void difftest_refdrive_trace_finish() {
  if (!recorder || recorder_pid != getpid())
    return;
  if (!finished) {
    RefDriveTraceEntry entry;
    entry.terminal = true;
    recorder->append(&entry);
  }
  recorder.reset();
}
