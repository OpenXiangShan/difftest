/***************************************************************************************
* Copyright (c) 2020-2025 Institute of Computing Technology, Chinese Academy of Sciences
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

#include "waveform.h"
#include "common.h"

EmuWaveform::EmuWaveform(TraceBindFunc trace, uint64_t cycles) : EmuWaveform(trace, cycles, default_filename(cycles)) {}

EmuWaveform::EmuWaveform(TraceBindFunc trace, uint64_t cycles, const char *filename) : waveform_clock(cycles) {
#if VM_TRACE == 1
  Verilated::traceEverOn(true); // Verilator must compute traced signals

#ifdef ENABLE_FST
  tfp = new VerilatedFstC;
#else
  tfp = new VerilatedVcdC;
#endif

  trace(tfp, 99); // Trace 99 levels of hierarchy

  tfp->open(filename);
  Info("dump wave to %s...\n", filename);
#endif // VM_TRACE == 1
}

const char *EmuWaveform::default_filename(uint64_t cycles) {
  char buf[32];

  // append cycle count to filename if cycles > 0
  int cycle_len = 0;
  if (cycles > 0) {
    cycle_len = snprintf(buf, sizeof(buf), "_%ld", cycles);
  }

// append suffix based on ENABLE_FST
#ifdef ENABLE_FST
  const char *suffix = ".fst";
#else
  const char *suffix = ".vcd";
#endif
  int len = snprintf(buf + cycle_len, sizeof(buf) - cycle_len, "%s", suffix);
  assert(len == strlen(suffix));

  return create_noop_filename(buf);
}

void EmuWaveform::tick() {
#if VM_TRACE == 1
  tfp->dump(waveform_clock);
  waveform_clock++;
#endif // VM_TRACE == 1
}
