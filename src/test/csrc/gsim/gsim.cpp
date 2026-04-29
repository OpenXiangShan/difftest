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

void GsimSim::waveform_init(uint64_t cycles) {
  (void)cycles;
  dut->setWaveformPath(create_noop_filename(".fst"));
  dut->enableWaveform();
}

void GsimSim::waveform_init(uint64_t cycles, const char *filename) {
  (void)cycles;
  dut->setWaveformPath(filename);
  dut->enableWaveform();
}

void GsimSim::waveform_tick() {
  // GSIM emits waveform changes from SSimTop::step(), so no extra tick hook is needed.
}
