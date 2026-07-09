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

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <stdexcept>

#ifndef WOLVRIX_GRHSIM_PERF
#define WOLVRIX_GRHSIM_PERF 0
#endif

namespace {

bool env_flag(const char *name) {
  const char *value = std::getenv(name);
  return value != nullptr && value[0] != '\0' && value[0] != '0';
}

} // namespace

GrhSIMDiffTestSim::GrhSIMDiffTestSim() : dut(new GrhSIMModel) {
  phase_timing_enabled_ = env_flag("EMU_PHASE_TIMING");
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

void GrhSIMDiffTestSim::step() {
  ++model_step_count_;
  if (!phase_timing_enabled_) {
    dut->eval();
    return;
  }
  const auto begin = std::chrono::steady_clock::now();
  dut->eval();
  model_step_time_us_ += static_cast<uint64_t>(
      std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - begin).count());
}

SimulatorRuntimeStats GrhSIMDiffTestSim::runtime_stats() const {
  SimulatorRuntimeStats stats{
      .modelStepCount = model_step_count_,
      .modelStepTimeUs = model_step_time_us_,
  };
#if WOLVRIX_GRHSIM_PERF
  const auto counters = dut->perf_counters();
  stats.evalCount = counters.evalCount;
  stats.round1Count = counters.round1Count;
  stats.round2Count = counters.round2Count;
  stats.totalRoundCount = counters.totalRoundCount;
  stats.computeBatchExecCount = counters.computeBatchExecCount;
  stats.commitBatchExecCount = counters.commitBatchExecCount;
  stats.computePhaseTimeUs = counters.computePhaseTimeUs;
  stats.commitPhaseTimeUs = counters.commitPhaseTimeUs;
  stats.touchedStateShadowCount = counters.touchedStateShadowCount;
  stats.touchedWriteCount = counters.touchedWriteCount;
#endif
  return stats;
}

void GrhSIMDiffTestSim::set_runtime_profile_enabled(bool enabled) {
  dut->set_runtime_profile_enabled(enabled);
}

void GrhSIMDiffTestSim::dump_runtime_profile() const {
  dut->dump_runtime_profile();
}

#endif // GRHSIM
