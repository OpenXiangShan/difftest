/***************************************************************************************
* Copyright (c) 2020-2021 Institute of Computing Technology, Chinese Academy of Sciences
* Copyright (c) 2020-2021 Peng Cheng Laboratory
*
* XiangShan is licensed under Mulan PSL v2.
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

#include "common.h"
#include "snapshot.h"
#include "lightsss.h"
#include "VSimTop.h"
#include "VSimTop__Syms.h"
#include <verilated_vcd_c.h>	// Trace file format header
#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#ifdef EMU_THREAD
#include <verilated_threads.h>
#endif
#ifdef CONDUCT_DSE
#include "dse.h"
#include "perfprocess.h"
#endif

struct EmuArgs {
  uint32_t seed;
  uint64_t max_cycles;
  uint64_t max_instr;
  uint64_t warmup_instr;
  uint64_t stat_cycles;
  uint64_t log_begin, log_end;
#ifdef DEBUG_REFILL
  uint64_t track_instr;
#endif
  const char *image;
  const char *snapshot_path;
  const char *wave_path;
  const char *flash_bin;
  bool enable_waveform;
  bool enable_snapshot;
  bool force_dump_result;
  bool enable_diff;
  bool enable_fork;
  bool enable_jtag;
  bool enable_runahead;
  bool dump_tl;
  bool jtag_test;

  EmuArgs() {
    seed = 0;
    max_cycles = -1;
    max_instr = -1;
    warmup_instr = -1;
    stat_cycles = -1;
    log_begin = 1;
    log_end = -1;
#ifdef DEBUG_REFILL
    track_instr = 0;
#endif
    snapshot_path = NULL;
    wave_path = NULL;
    image = NULL;
    flash_bin = "./dse-driver/build/dse.bin";
    // flash_bin = NULL;
    enable_waveform = false;
    enable_snapshot = true;
    force_dump_result = false;
    enable_diff = true;
    enable_fork = false;
    enable_jtag = false;
    enable_runahead = false;
    dump_tl = false;
    jtag_test = false;
  }
};

class Emulator {
private:
  VSimTop *dut_ptr;
  VerilatedVcdC* tfp;
  bool enable_waveform;
  bool force_dump_wave = false;
#ifdef VM_SAVABLE
  VerilatedSaveMem snapshot_slot[2];
#endif
  EmuArgs args;
  LightSSS lightsss;
#ifdef CONDUCT_DSE
  DSE dse = DSE();
#endif

  enum {
    STATE_GOODTRAP = 0,
    STATE_BADTRAP = 1,
    STATE_ABORT = 2,
    STATE_LIMIT_EXCEEDED = 3,
    STATE_SIG = 4,
    STATE_RUNNING = -1
  };

  // emu control variable
  uint64_t cycles;
  int trapCode;

  inline void reset_ncycles(size_t cycles);
  inline void first_reset_ncycles(size_t cycles);
  inline void reset_dse_ncycles(size_t cycles);
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
  bool is_fork_child() { return lightsss.is_child(); }

public:
  Emulator(int argc, const char *argv[]);
  ~Emulator();
  uint64_t execute(uint64_t max_cycle, uint64_t max_instr);
  uint64_t get_cycles() const { return cycles; }
  EmuArgs get_args() const { return args; }
  bool is_good_trap() {
    return trapCode == STATE_GOODTRAP || trapCode == STATE_LIMIT_EXCEEDED;
  };
  int get_trapcode() { return trapCode; }
};

#endif

void init_cmd(void);
extern bool enable_jtag_testcase;
extern std::vector<uint64_t> getIOPerfCnts(VSimTop *dut_ptr);
extern std::vector<std::string> getIOPerfNames();
