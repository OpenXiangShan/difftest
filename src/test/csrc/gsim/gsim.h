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

#ifndef __SIMULATOR_GSIM_H
#define __SIMULATOR_GSIM_H

#include "SimTop.h"

class GsimSim final : public Simulator {
private:
  SSimTop *dut;

protected:
  inline unsigned get_uart_out_valid() override {
    return dut->get_difftest__DOT__uart__DOT__out__DOT__valid();
  }
  inline uint8_t get_uart_out_ch() override {
    return dut->get_difftest__DOT__uart__DOT__out__DOT__ch();
  }
  inline unsigned get_uart_in_valid() override {
    return dut->get_difftest__DOT__uart__DOT__in__DOT__valid();
  }
  inline void set_uart_in_ch(uint8_t ch) override {
    dut->set_difftest__DOT__uart__DOT__in__DOT__ch(ch);
  }

public:
  GsimSim();
  ~GsimSim();

  inline void set_clock(unsigned clock) override {
    // Gsim does not use explicit clock. Simply call step() instead.
  }
  inline void set_reset(unsigned reset) override {
    dut->set_reset(reset);
  }
  inline void step() override {
    dut->step();
  }

  inline uint64_t get_difftest_exit() final {
    return dut->get_difftest__DOT__exit();
  }
  inline uint64_t get_difftest_step() final {
    return dut->get_difftest__DOT__step();
  }

  inline bool supports_difftest_debug_snapshot() const override {
    return true;
  }
  inline uint64_t debug_reset() const override {
    return dut->reset;
  }
  inline uint64_t debug_endpoint_step() const override {
    return dut->endpoint__DOT__step_REG;
  }
  inline uint64_t debug_trap_cycle_cnt() const override {
    return dut->endpoint__DOT__trap__DOT__dpic__DOT__io__DOT__cycleCnt;
  }
  inline uint64_t debug_trap_instr_cnt() const override {
    return dut->endpoint__DOT__trap__DOT__dpic__DOT__io__DOT__instrCnt;
  }
  inline uint64_t debug_event_valid() const override {
    return dut->endpoint__DOT__event__DOT__dpic__DOT__io__DOT__valid;
  }
  inline uint64_t debug_commit0_valid() const override {
    return dut->endpoint__DOT__commit__DOT__dpic__DOT__io__DOT__valid;
  }
  inline uint64_t debug_commit0_pc() const override {
    return dut->endpoint__DOT__commit__DOT__dpic__DOT__io__DOT__pc;
  }
  inline bool supports_waveform() const override {
    return false;
  }
  inline void waveform_init(uint64_t cycles) override {
    (void)cycles;
  }
  inline void waveform_init(uint64_t cycles, const char *filename) override {
    (void)cycles;
    (void)filename;
  }
  inline void waveform_tick() override {}

  inline void set_perf_clean(unsigned clean) override {
    dut->set_difftest__DOT__perfCtrl__DOT__clean(clean);
  }
  inline void set_perf_dump(unsigned dump) override {
    dut->set_difftest__DOT__perfCtrl__DOT__dump(dump);
  }

  inline void set_log_begin(uint64_t begin) override {
    dut->set_difftest__DOT__logCtrl__DOT__begin(begin);
  }
  inline void set_log_end(uint64_t end) override {
    dut->set_difftest__DOT__logCtrl__DOT__end(end);
  }
};

#endif // __SIMULATOR_GSIM_H
