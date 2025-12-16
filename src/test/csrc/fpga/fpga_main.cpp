/***************************************************************************************
* Copyright (c) 2025 Beijing Institute of Open Source Chip (BOSC)
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

#include "args.h"
#include "device.h"
#include "difftest.h"
#include "flash.h"
#include "goldenmem.h"
#include "mpool.h"
#include "ram.h"
#include "refproxy.h"
#include "xdma.h"
#include <condition_variable>
#include <getopt.h>
#include <mutex>
#include <unistd.h>
#ifdef FPGA_SIM
#include "xdma_sim.h"
#endif // FPGA_SIM
#ifdef USE_SERIAL_PORT
#include "serial_port.h"
#endif // USE_SERIAL_PORT

void fpga_finish();

enum {
  FPGA_RUN,
  FPGA_GOODTRAP,
  FPGA_EXCEED,
  FPGA_FAIL,
} fpga_state;

static uint8_t fpga_result = FPGA_RUN;
static CommonArgs args;

void fpga_init();
void fpga_step();
void set_diff_ref_so(char *s);
void args_parsing(int argc, char *argv[]);

FpgaXdma *xdma_device = NULL;
#ifdef USE_SERIAL_PORT
SerialPort *serial_port = NULL;
#endif // USE_SERIAL_PORT
int main(int argc, const char *argv[]) {
  args = parse_args(argc, argv);

  fpga_init();

  printf("fpga init\n");
  xdma_device->start(); // Trigger stop by fpga_nstep
  fpga_finish();
  printf("difftest releases the fpga device and exits\n");
  return 0;
}

void fpga_init() {
  xdma_device = new FpgaXdma();
#ifndef FPGA_SIM
  xdma_device->fpga_reset_io(true);
  usleep(1000);
#endif // FPGA_SIM

#ifdef USE_SERIAL_PORT
  serial_port = new SerialPort("/dev/ttyUSB0");
  serial_port->start();
#endif // USE_SERIAL_PORT

  init_ram(args.image, DEFAULT_EMU_RAM_SIZE);
  init_flash(args.flash_bin);

  difftest_init(true, DEFAULT_EMU_RAM_SIZE);

  init_device();

#ifndef FPGA_SIM
#ifdef USE_XDMA_DDR_LOAD
  xdma_device->ddr_load_workload(args.image);
#endif // USE_XDMA_DDR_LOAD
  xdma_device->fpga_reset_io(false);
#endif // FPGA_SIM
}

void fpga_finish() {
  delete xdma_device;
#ifdef USE_SERIAL_PORT
  serial_port->stop();
  delete serial_port;
#endif // USE_SERIAL_PORT

  common_finish();

  difftest_finish();
  goldenmem_finish();
  finish_device();

  delete simMemory;
  simMemory = nullptr;
}

void fpga_display_result(int ret) {
  for (int i = 0; i < NUM_CORES; i++) {
    printf("Core %d: ", i);
    uint64_t pc = difftest[i]->get_trap_event()->pc;
    switch (ret) {
      case FPGA_GOODTRAP: eprintf(ANSI_COLOR_GREEN "HIT GOOD TRAP at pc = 0x%" PRIx64 "\n" ANSI_COLOR_RESET, pc); break;
      case FPGA_EXCEED:
        eprintf(ANSI_COLOR_YELLOW "EXCEEDING INSTR LIMIT at pc = 0x%" PRIx64 "\n" ANSI_COLOR_RESET, pc);
        break;
      case FPGA_FAIL: eprintf(ANSI_COLOR_RED "FAILED at pc = 0x%" PRIx64 "\n" ANSI_COLOR_RESET, pc); break;
      default: eprintf(ANSI_COLOR_RED "Unknown trap code: %d\n", ret);
    }
    difftest[i]->display_stats();
    if (args.warmup_instr != 0) {
      difftest[i]->warmup_display_stats();
    }
  }
}

int fpga_get_result(uint8_t step) {
  // Compare DUT and REF
  int trapCode = difftest_nstep(step, args.enable_diff);
  if (trapCode != STATE_RUNNING) {
    if (trapCode == STATE_GOODTRAP)
      return FPGA_GOODTRAP;
    else
      return FPGA_FAIL;
  }
  // Max Instr Limit Check
  if (args.max_instr != -1) {
    for (int i = 0; i < NUM_CORES; i++) {
      auto trap = difftest[i]->get_trap_event();
      if (trap->instrCnt >= args.max_instr) {
        return FPGA_EXCEED;
      }
    }
  }
  // Warmup Check
  static bool warmup_finish = false;
  if (args.warmup_instr != -1 && !warmup_finish) {
    for (int i = 0; i < NUM_CORES; i++) {
      auto trap = difftest[i]->get_trap_event();
      if (trap->instrCnt >= args.warmup_instr) {
        warmup_finish = true;
        break;
      }
    }
    if (warmup_finish) {
      // Record Instr/Cycle for soft warmup
      for (int i = 0; i < NUM_CORES; i++) {
        difftest[i]->warmup_record();
      }
    }
  }
  // Trace Debug Support
  if (args.enable_ref_trace) {
    for (int i = 0; i < NUM_CORES; i++) {
      auto trap = difftest[i]->get_trap_event();
      bool is_debug = difftest[i]->proxy->get_debug();
      if (trap->cycleCnt >= args.log_begin && !is_debug) {
        difftest[i]->proxy->set_debug(true);
      }
      if (trap->cycleCnt >= args.log_end && is_debug) {
        difftest[i]->proxy->set_debug(false);
      }
    }
  }
  if (args.enable_commit_trace) {
    for (int i = 0; i < NUM_CORES; i++) {
      auto trap = difftest[i]->get_trap_event();
      bool is_commit_trace = difftest[i]->get_commit_trace();
      if (trap->cycleCnt >= args.log_begin && !is_commit_trace) {
        difftest[i]->set_commit_trace(true);
      }
      if (trap->cycleCnt >= args.log_end && is_commit_trace) {
        difftest[i]->set_commit_trace(false);
      }
    }
  }
  return FPGA_RUN;
}

extern "C" void fpga_nstep(uint8_t step) {
  if (fpga_result != FPGA_RUN)
    return;
  int ret = fpga_get_result(step);
  if (ret != FPGA_RUN) {
    fpga_display_result(ret);
    fpga_result = ret;
    xdma_device->stop();
  }
}
