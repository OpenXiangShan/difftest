/***************************************************************************************
* Copyright (c) 2026 Institute of Computing Technology, Chinese Academy of Sciences
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

#ifdef GRHSIM

#include "simulator.h"

#include <cstdio>
#include <stdexcept>

GrhSIMDiffTestSim::GrhSIMDiffTestSim() : dut(new GrhSIMModel) {
  dut->init();
}

GrhSIMDiffTestSim::~GrhSIMDiffTestSim() {
  delete dut;
  dut = nullptr;
}

void GrhSIMDiffTestSim::waveform_init(uint64_t cycles) {
  (void)cycles;
#if WOLVRIX_GRHSIM_WAVEFORM
  dut->configure_waveform(true);
#else
  printf("Waveform is unsupported in this grhsim build.\n");
  printf("Please rebuild with WOLVRIX_GRHSIM_WAVEFORM=1.\n");
  throw std::runtime_error("Waveform not supported.");
#endif
}

void GrhSIMDiffTestSim::waveform_init(uint64_t cycles, const char *filename) {
  (void)cycles;
#if WOLVRIX_GRHSIM_WAVEFORM
  if (filename != nullptr && filename[0] != '\0') {
    dut->configure_waveform(true, filename);
  } else {
    dut->configure_waveform(true);
  }
#else
  printf("Waveform is unsupported in this grhsim build.\n");
  printf("Please rebuild with WOLVRIX_GRHSIM_WAVEFORM=1.\n");
  throw std::runtime_error("Waveform not supported.");
#endif
}

void GrhSIMDiffTestSim::waveform_tick() {
  // GrhSIM emits waveform records at eval boundaries after waveform_init() enables it.
}

#endif // GRHSIM
