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

#include "device.h"
#include "diffstate.h"
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

static char work_load[256] = "/dev/zero";
static char *flash_bin_file = NULL;
static uint8_t fpga_result = FPGA_RUN;
static bool enable_difftest = true;
static uint64_t max_instrs = 0;
static uint64_t warmup_instr = 0;

void fpga_init();
void fpga_step();
void set_diff_ref_so(char *s);
void args_parsing(int argc, char *argv[]);

FpgaXdma *xdma_device = NULL;
#ifdef USE_SERIAL_PORT
SerialPort *serial_port = NULL;
#endif // USE_SERIAL_PORT
int main(int argc, char *argv[]) {
  args_parsing(argc, argv);

  fpga_init();

  printf("fpga init\n");
  xdma_device->start(); // Trigger stop by fpga_nstep
  fpga_finish();
  printf("difftest releases the fpga device and exits\n");
  return 0;
}

void set_diff_ref_so(char *s) {
  extern const char *difftest_ref_so;
  char *buf = (char *)malloc(256);
  strcpy(buf, s);
  difftest_ref_so = buf;
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

  init_ram(work_load, DEFAULT_EMU_RAM_SIZE);
  init_flash(flash_bin_file);

  difftest_init();

  init_device();
  init_goldenmem();
  init_nemuproxy(DEFAULT_EMU_RAM_SIZE);

#ifndef FPGA_SIM
#ifdef USE_XDMA_DDR_LOAD
  xdma_device->ddr_load_workload(work_load);
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
    if (warmup_instr != 0) {
      difftest[i]->warmup_display_stats();
    }
  }
}

int fpga_get_result(uint8_t step) {
  // Compare DUT and REF
  int trapCode = difftest_nstep(step, enable_difftest);
  if (trapCode != STATE_RUNNING) {
    if (trapCode == STATE_GOODTRAP)
      return FPGA_GOODTRAP;
    else
      return FPGA_FAIL;
  }
  // Max Instr Limit Check
  if (max_instrs != 0) {
    for (int i = 0; i < NUM_CORES; i++) {
      auto trap = difftest[i]->get_trap_event();
      if (trap->instrCnt >= max_instrs) {
        return FPGA_EXCEED;
      }
    }
  }
  // Warmup Check
  if (warmup_instr != 0) {
    bool finish = false;
    for (int i = 0; i < NUM_CORES; i++) {
      auto trap = difftest[i]->get_trap_event();
      if (trap->instrCnt >= warmup_instr) {
        warmup_instr = -1; // maxium of uint64_t
        finish = true;
        break;
      }
    }
    if (finish) {
      // Record Instr/Cycle for soft warmup
      for (int i = 0; i < NUM_CORES; i++) {
        difftest[i]->warmup_record();
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

void args_parsing(int argc, char *argv[]) {
  int opt;
  int option_index = 0;
  static struct option long_options[] = {{"diff", required_argument, 0, 0},
                                         {"max-instrs", required_argument, 0, 0},
                                         {"warmup-instr", required_argument, 0, 0},
                                         {"flash", required_argument, 0, 0},
                                         {0, 0, 0, 0}};

  while ((opt = getopt_long(argc, argv, "i:", long_options, &option_index)) != -1) {
    switch (opt) {
      case 0:
        if (strcmp(long_options[option_index].name, "diff") == 0) {
          set_diff_ref_so(optarg);
        } else if (strcmp(long_options[option_index].name, "max-instrs") == 0) {
          max_instrs = std::stoul(optarg, nullptr, 10);
        } else if (strcmp(long_options[option_index].name, "warmup-instr") == 0) {
          warmup_instr = std::stoul(optarg, nullptr, 10);
        } else if (strcmp(long_options[option_index].name, "flash") == 0) {
          flash_bin_file = (char *)malloc(256);
          strcpy(flash_bin_file, optarg);
        }
        break;
      case 'i': strncpy(work_load, optarg, sizeof(work_load) - 1); break;
      default:
        std::cerr
            << "Usage: " << argv[0]
            << " [--diff <path>] [-i <workload>] [--max-instrs <num>] [--warmup-instr <num>] [--flash <flash_img>]"
            << std::endl;
        exit(EXIT_FAILURE);
    }
  }
}
