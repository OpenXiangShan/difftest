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

#ifndef __SIMULATOR_VERILATOR_H
#define __SIMULATOR_VERILATOR_H

#include "VSimTop.h"
#include "VSimTop__Syms.h"
#include "waveform.h"
#ifdef VM_SAVABLE
#include "snapshot.h"
#endif // VM_SAVABLE

class VerilatorSim final : public Simulator {
private:
  VSimTop *dut;

#if VM_TRACE == 1
  TraceBindFunc trace_bind = [this](VerilatedTraceBaseC *tfp, int levels) { this->dut->trace(tfp, levels); };
  EmuWaveform *waveform = nullptr;
#endif // VM_TRACE == 1

#ifdef VM_SAVABLE
  VerilatedSaveMem *snapshot_slot = nullptr;
#endif // VM_SAVABLE

protected:
  inline unsigned get_uart_out_valid() override {
    return dut->difftest_uart_out_valid;
  }
  inline uint8_t get_uart_out_ch() override {
    return dut->difftest_uart_out_ch;
  }
  inline unsigned get_uart_in_valid() override {
    return dut->difftest_uart_in_valid;
  }
  inline void set_uart_in_ch(uint8_t ch) override {
    dut->difftest_uart_in_ch = ch;
  }

public:
  VerilatorSim();
  ~VerilatorSim();

  inline void set_clock(unsigned clock) override {
    dut->clock = clock;
  }
  inline void set_reset(unsigned reset) override {
    dut->reset = reset;
  }
  inline void step() override {
    dut->eval();
  }

  inline uint64_t get_difftest_exit() override {
    return dut->difftest_exit;
  }
  inline uint64_t get_difftest_step() override {
    return dut->difftest_step;
  }

  inline void set_perf_clean(unsigned clean) override {
    dut->difftest_perfCtrl_clean = clean;
  }
  inline void set_perf_dump(unsigned dump) override {
    dut->difftest_perfCtrl_dump = dump;
  }

  inline void set_log_begin(uint64_t begin) override {
    dut->difftest_logCtrl_begin = begin;
  }
  inline void set_log_end(uint64_t end) override {
    dut->difftest_logCtrl_end = end;
  }

  void atClone() override;

#if VM_TRACE == 1
  void waveform_init(uint64_t cycles) override;
  void waveform_init(uint64_t cycles, const char *filename) override;
  void waveform_tick() override;
#endif // VM_TRACE == 1

#ifdef VM_SAVABLE
  void snapshot_init() override;
  void snapshot_save(int index) override;
  std::function<void(void *, size_t)> snapshot_take() override;
  std::function<void(void *, size_t)> snapshot_load(const char *filename) override;
#endif // VM_SAVABLE
};

#endif // __SIMULATOR_VERILATOR_H
