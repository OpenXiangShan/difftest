/***************************************************************************************
* Copyright (c) 2020-2025 Institute of Computing Technology, Chinese Academy of Sciences
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

#include "args.h"
#include "common.h"
#include "dut.h"
#include "lightsss.h"
#include "simulator.h"
#include <sys/types.h>

class Emulator final : public DUT {
private:
  Simulator *dut_ptr;

  bool force_dump_wave = false;
  CommonArgs args;
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
  void display_stats();

  inline const char *logdb_filename() {
    return create_noop_filename(".db");
  }

  void snapshot_save();
  void snapshot_load(const char *filename);

  inline const char *waveform_filename() {
#ifdef ENABLE_FST
    const char *filename = create_noop_filename(".fst");
#else
    const char *filename = create_noop_filename(".vcd");
#endif
    Info("dump wave to %s...\n", filename);
    return filename;
  }

  const char *cycle_wavefile(uint64_t cycles);

#if VM_COVERAGE == 1
  void save_coverage();
#endif

  void fork_child_init();
  inline bool is_fork_child() {
    return lightsss->is_child();
  }

public:
  Emulator(int argc, const char *argv[]);
  ~Emulator();
  uint64_t execute(uint64_t max_cycle, uint64_t max_instr);
  uint64_t get_cycles() const {
    return cycles;
  }
  CommonArgs get_args() const {
    return args;
  }
  bool is_good_trap() {
#ifdef FUZZING
    return !(trapCode == STATE_ABORT);
#else
    return trapCode == STATE_GOODTRAP || trapCode == STATE_LIMIT_EXCEEDED || trapCode == STATE_SIM_EXIT;
#endif
  };
  int get_trapcode() {
    return trapCode;
  }
  int tick();
  int is_finished();
  int is_good();
};

#endif
