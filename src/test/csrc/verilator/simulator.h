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

#ifndef __SIMULATOR_H
#define __SIMULATOR_H

#include "common.h"

class Simulator {
private:
  static void report_waveform_error() {
    printf("Waveform is unsupported in this simulator or disabled.\n");
    printf("If you are using Verilator, please compile with EMU_TRACE=1 to enable it.\n");
    throw std::runtime_error("Waveform not supported.");
  }

  static void report_snapshot_error(void *__restrict datap, size_t size) {
    printf("Snapshot is unsupported in this simulator or disabled.\n");
    printf("If you are using Verilator, please compile with EMU_SNAPSHOT=1 to enable it.\n");
    throw std::runtime_error("Snapshot not supported.");
  }

protected:
  virtual unsigned get_uart_out_valid() = 0;
  virtual uint8_t get_uart_out_ch() = 0;
  virtual unsigned get_uart_in_valid() = 0;
  virtual void set_uart_in_ch(uint8_t ch) = 0;

public:
  Simulator() = default;
  virtual ~Simulator() {};

  /******* mandatory methods for child classes *******/
  // Set the clock signal, 0 or 1.
  virtual void set_clock(unsigned clock) = 0;
  // Set the reset signal, 0 or 1.
  virtual void set_reset(unsigned reset) = 0;
  // Tick one step. Note this method may have various implementations.
  virtual void step() = 0;

  // Get the exit signal from DiffTest.
  virtual uint64_t get_difftest_exit() = 0;
  // Get the step signal from DiffTest.
  virtual uint64_t get_difftest_step() = 0;

  // Set the clean signal for performance counters.
  virtual void set_perf_clean(unsigned clean) = 0;
  // Set the dump signal for performance counters.
  virtual void set_perf_dump(unsigned dump) = 0;

  // Set the log begin signal.
  virtual void set_log_begin(uint64_t begin) = 0;
  // Set the log end signal.
  virtual void set_log_end(uint64_t end) = 0;

  /******* optional methods for child classes *******/
  // To re-initialize the simulator upon a `fork()` call.
  virtual void atClone() {};

  // To initialize the waveform.
  virtual void waveform_init(uint64_t cycles) {
    report_waveform_error();
  }
  // To tick the waveform.
  virtual void waveform_init(uint64_t cycles, const char *filename) {
    report_waveform_error();
  }
  // To tick the waveform for one timestamp.
  virtual void waveform_tick() {
    report_waveform_error();
  };

  // To initialize the simulation snapshot.
  virtual void snapshot_init() {
    report_snapshot_error(nullptr, 0);
  }
  // To clean the simulation snapshot.
  virtual void snapshot_save(int index) {
    report_snapshot_error(nullptr, 0);
  }
  // To save the simulation snapshot.
  virtual std::function<void(void *, size_t)> snapshot_take() {
    return report_snapshot_error;
  }
  // To load the simulation snapshot from a file.
  virtual std::function<void(void *, size_t)> snapshot_load(const char *filename) {
    return report_snapshot_error;
  }

  /******* final methods *******/

  // Step the UART, this is used to read/write UART data.
  // This method cannot be overridden.
  inline void step_uart() {
    if (get_uart_out_valid()) {
      printf("%c", get_uart_out_ch());
      fflush(stdout);
    }
    if (get_uart_in_valid()) {
      extern uint8_t uart_getc();
      set_uart_in_ch(uart_getc());
    }
  }
};

#ifdef VERILATOR
#define SIMULATOR VerilatorSim
#include "verilator.h"
#endif // VERILATOR

#ifdef GSIM
#define SIMULATOR GsimSim
#include "gsim.h"
#endif // GSIM

#ifndef SIMULATOR
#error "SIMULATOR undetected, please define it."
#endif // SIMULATOR

#endif
