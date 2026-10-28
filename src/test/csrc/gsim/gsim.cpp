/***************************************************************************************
* Copyright (c) 2025 Institute of Computing Technology, Chinese Academy of Sciences
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

#include "simulator.h"

GsimSim::GsimSim() : dut(new SSimTop) {}

GsimSim::~GsimSim() {
  delete dut;
}

#ifndef GSIM_TRACE_FST
static void warn_waveform_unsupported() {
  static bool warned = false;
  if (!warned) {
    warned = true;
    fprintf(stderr,
            "[gsim] warning: waveform requested, but this emu was built without EMU_TRACE/--trace-fst; waveform APIs "
            "become no-ops\n");
  }
}
#endif

void GsimSim::waveform_init(uint64_t cycles) {
  waveform_init(cycles, create_noop_filename(".fst"));
}

void GsimSim::waveform_init(uint64_t cycles, const char *filename) {
  (void)cycles;
#ifdef GSIM_TRACE_FST
  dut->setWaveformPath(filename);
  dut->enableWaveform();
#else
  (void)filename;
  warn_waveform_unsupported();
#endif
}

void GsimSim::waveform_tick() {
#ifdef GSIM_TRACE_FST
  dut->dump();
#endif
}
