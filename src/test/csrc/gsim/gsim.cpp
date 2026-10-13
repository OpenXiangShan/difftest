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
static bool warn_waveform_unsupported() {
  static bool warned = false;
  if (!warned) {
    warned = true;
    fprintf(stderr,
      "[gsim] warning: waveform requested, but this emu was built without EMU_TRACE/--trace-fst; waveform APIs become no-ops\n");
  }
  return false;
}
#endif

void GsimSim::waveform_init(uint64_t cycles) {
  (void)cycles;
#ifdef GSIM_TRACE_FST
  dut->setWaveformPath(create_noop_filename(".fst"));
  dut->disableWaveform();
  waveform_active = true;
#else
  waveform_active = warn_waveform_unsupported();
#endif
}

void GsimSim::waveform_init(uint64_t cycles, const char *filename) {
  (void)cycles;
#ifdef GSIM_TRACE_FST
  dut->setWaveformPath(filename);
  dut->disableWaveform();
  waveform_active = true;
#else
  (void)filename;
  waveform_active = warn_waveform_unsupported();
#endif
}

void GsimSim::waveform_tick() {
#ifdef GSIM_TRACE_FST
  if (!waveform_active) return;
  dut->enableWaveform();
#endif
}
