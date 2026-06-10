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

static bool warn_waveform_unsupported() {
  static bool warned = false;
  if (!warned) {
    warned = true;
    fprintf(stderr,
      "[gsim] warning: waveform requested, but this emu was built without EMU_TRACE/--trace-fst; waveform APIs become no-ops\n");
  }
  return false;
}

void GsimSim::waveform_init(uint64_t cycles) {
  (void)cycles;
#if defined(GSIM_HAS_FST_WAVE)
  if (!SSimTop::kTraceFstCompiled) {
    waveform_active = warn_waveform_unsupported();
    return;
  }
  dut->setWaveformPath(create_noop_filename(".fst"));
  dut->enableWaveform();
  waveform_active = true;
#else
  waveform_active = warn_waveform_unsupported();
#endif
}

void GsimSim::waveform_init(uint64_t cycles, const char *filename) {
  (void)cycles;
  (void)filename;
#if defined(GSIM_HAS_FST_WAVE)
  if (!SSimTop::kTraceFstCompiled) {
    waveform_active = warn_waveform_unsupported();
    return;
  }
  dut->setWaveformPath(filename);
  dut->enableWaveform();
  waveform_active = true;
#else
  waveform_active = warn_waveform_unsupported();
#endif
}

void GsimSim::waveform_tick() {
#if defined(GSIM_HAS_FST_WAVE)
  if (!waveform_active) return;
  dut->emitAllSignalValues();
#endif
}
