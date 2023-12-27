/***************************************************************************************
* Copyright (c) 2020-2023 Institute of Computing Technology, Chinese Academy of Sciences
* Copyright (c) 2020-2021 Peng Cheng Laboratory
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

#ifndef __EMU_H
#define __EMU_H

#include <sys/types.h>
#include "common.h"
#include "dut.h"
#include "lightsss.h"
#include "snapshot.h"
#include "VSimTop.h"
#include "VSimTop__Syms.h"
#ifdef ENABLE_FST
#include <verilated_fst_c.h>
#else
#include <verilated_vcd_c.h>	// Trace file format header
#endif
#ifdef EMU_THREAD
#include <verilated_threads.h>
#endif
#if VM_TRACE == 1
#include <verilated_vcd_c.h>	// Trace file format header
#endif

struct EmuArgs {
  uint32_t seed = 0;
  uint64_t max_cycles = -1;
  uint64_t fork_interval = 1000;
  uint64_t max_instr = -1;
  uint64_t warmup_instr = -1;
  uint64_t stat_cycles = -1;
  uint64_t log_begin = 0, log_end = -1;
  uint64_t overwrite_nbytes = 0xe00;
#ifdef DEBUG_REFILL
  uint64_t track_instr = 0;
#endif
#ifdef ENABLE_IPC
  uint64_t ipc_interval;
  FILE *ipc_file;
  uint64_t ipc_last_instr;
  uint64_t ipc_last_cycle;
  uint64_t ipc_times;
#endif
  const char *image = nullptr;
  const char *gcpt_restore = nullptr;
  const char *snapshot_path = nullptr;
  const char *wave_path = nullptr;
  const char *ram_size = nullptr;
  const char *flash_bin = nullptr;
  const char *select_db = nullptr;
  const char *trace_name = nullptr;
  const char *footprints_name = nullptr;
  const char *linearized_name = nullptr;
  bool enable_waveform = false;
  bool enable_waveform_full = false;
  bool enable_ref_trace = false;
  bool enable_commit_trace = false;
  bool enable_snapshot = true;
  bool force_dump_result = false;
  bool enable_diff = true;
  bool enable_fork = false;
  bool enable_jtag = false;
  bool enable_runahead = false;
  bool dump_db = false;
  bool trace_is_read = true;
  bool dump_coverage = false;
  bool image_as_footprints = false;
};

class Emulator final : public DUT {
private:
  VSimTop *dut_ptr;
#ifdef ENABLE_FST
  VerilatedFstC* tfp;
#else
  VerilatedVcdC* tfp;
#endif
  bool force_dump_wave = false;
#ifdef VM_SAVABLE
  VerilatedSaveMem *snapshot_slot = nullptr;
#endif
  EmuArgs args;
  LightSSS *lightsss = NULL;
#if VM_COVERAGE == 1
  VerilatedCovContext *coverage = NULL;
#endif // VM_COVERAGE

  // emu control variable
  uint64_t cycles;
  int trapCode;
  uint32_t lasttime_snapshot = 0;
  uint64_t core_max_instr[NUM_CORES];
  uint32_t lasttime_poll = 0;
  uint32_t elapsed_time;

  inline void reset_ncycles(size_t cycles);
  inline void single_cycle();
  void trigger_stat_dump();
  void display_trapinfo();
  inline char* timestamp_filename(time_t t, char *buf);
  inline char* logdb_filename(time_t t);
  inline char* snapshot_filename(time_t t);
  inline char* coverage_filename(time_t t);
  void snapshot_save(const char *filename);
  void snapshot_load(const char *filename);
  inline char* waveform_filename(time_t t);
  inline char* cycle_wavefile(uint64_t cycles, time_t t);
#if VM_COVERAGE == 1
  inline void save_coverage(time_t t);
#endif
  void fork_child_init();
  inline bool is_fork_child() { return lightsss->is_child(); }

public:
  Emulator(int argc, const char *argv[]);
  ~Emulator();
  uint64_t execute(uint64_t max_cycle, uint64_t max_instr);
  uint64_t get_cycles() const { return cycles; }
  EmuArgs get_args() const { return args; }
  bool is_good_trap() {
#ifdef FUZZING
    return !(trapCode == STATE_ABORT);
#else
    return trapCode == STATE_GOODTRAP || trapCode == STATE_LIMIT_EXCEEDED || trapCode == STATE_SIM_EXIT;
#endif
  };
  int get_trapcode() { return trapCode; }
  int tick();
  int is_finished();
  int is_good();
};

#endif
