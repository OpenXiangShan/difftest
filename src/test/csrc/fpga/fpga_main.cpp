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
#include "diffstate.h"
#include "difftest.h"
#include "flash.h"
#include "goldenmem.h"
#include "mpool.h"
#include "ram.h"
#include "refproxy.h"
#include "xdma.h"
#include <condition_variable>
#include <cstdlib>
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
static const char *fpga_ddr_load_cmd = nullptr;
static const char *fpga_ila_dump_cmd = nullptr;

void fpga_init();
void fpga_step();
void set_diff_ref_so(char *s);
void args_parsing(int argc, char *argv[]);
static bool run_external_cmd(const char *cmd, const char *tag);

FpgaXdma *xdma_device = NULL;
#ifdef USE_SERIAL_PORT
SerialPort *serial_port = NULL;
#endif // USE_SERIAL_PORT
int main(int argc, const char *argv[]) {
  common_set_locale();

  fpga_ddr_load_cmd = std::getenv("FPGA_DDR_LOAD_CMD");
  fpga_ila_dump_cmd = std::getenv("FPGA_ILA_DUMP_CMD");
  args = parse_args(argc, argv);

  fpga_init();

  printf("fpga init\n");
  xdma_device->start(args.enable_diff); // Trigger stop by fpga_nstep
  fpga_finish();
  printf("difftest releases the fpga device and exits\n");
  return !(fpga_result == FPGA_GOODTRAP);
}

static bool run_external_cmd(const char *cmd, const char *tag) {
  if (!cmd || !cmd[0]) {
    return false;
  }

  printf("[fpga-host] running external %s command: %s\n", tag, cmd);
  fflush(stdout);

  int rc = std::system(cmd);
  if (rc == -1) {
    fprintf(stderr, "[fpga-host] failed to launch external %s command\n", tag);
    return false;
  }
  if (WIFEXITED(rc) && WEXITSTATUS(rc) == 0) {
    printf("[fpga-host] external %s command completed successfully\n", tag);
    fflush(stdout);
    return true;
  }

  if (WIFEXITED(rc)) {
    fprintf(stderr, "[fpga-host] external %s command exited with code %d\n", tag, WEXITSTATUS(rc));
  } else if (WIFSIGNALED(rc)) {
    fprintf(stderr, "[fpga-host] external %s command terminated by signal %d\n", tag, WTERMSIG(rc));
  } else {
    fprintf(stderr, "[fpga-host] external %s command failed with status 0x%x\n", tag, rc);
  }
  return false;
}

void fpga_init() {
  uint64_t ram_size = args.ram_size ? parse_ramsize(args.ram_size) : DEFAULT_EMU_RAM_SIZE;
  if (ram_size % (1024 * 1024) != 0) {
    fprintf(stderr, "[fpga-host] --ram-size must be aligned to MB, got %s\n", args.ram_size);
    exit(1);
  }
  uint32_t ram_size_mb = ram_size / (1024 * 1024);

  xdma_device = new FpgaXdma();
  xdma_device->fpga_io(HOST_IO_CFG_RESET, true);
  sleep(1);

  if (args.random_mem) {
    xdma_device->fpga_io(HOST_IO_SEED, args.seed);
    xdma_device->fpga_io(HOST_IO_RAM_SIZE_MB, ram_size_mb);
    printf("[fpga-host] init mem with seed = %d, size = %dMB\n", args.seed, ram_size_mb);
    uint32_t init_mem_start = uptime();
    xdma_device->fpga_io(HOST_IO_MEM_INIT, true);
    xdma_device->wait_fpga_io_done(HOST_IO_MEM_INIT, "memory random init");
    printf("[fpga-host] init mem done, elapsed = %ums\n", uptime() - init_mem_start);
  }

#ifdef CONFIG_USE_XDMA_H2C
  uint32_t h2c_start = uptime();
  xdma_device->fpga_io(HOST_IO_MEM_H2C, true);
  xdma_device->h2c_load_workload(args.image, ram_size);
  xdma_device->wait_fpga_io_done(HOST_IO_MEM_H2C, "memory H2C load");
  printf("[fpga-host] H2C load done, elapsed = %ums\n", uptime() - h2c_start);
#else // CONFIG_USE_XDMA_H2C
#ifdef FPGA_SIM
  xdma_sim_set_workload(args.image);
#else
  if (fpga_ddr_load_cmd) {
    if (!run_external_cmd(fpga_ddr_load_cmd, "DDR load")) {
      exit(0);
    }
  }
#endif // FPGA_SIM
#endif // CONFIG_USE_XDMA_H2C

  xdma_device->fpga_io(HOST_IO_RESET, true);
  xdma_device->fpga_io(HOST_IO_MEM_CPU, true);
  xdma_device->fpga_io(HOST_IO_DIFFTEST_ENABLE, args.enable_diff);
  xdma_device->fpga_io(HOST_IO_ILA_TRIGGER, false);
  xdma_device->fpga_io(HOST_IO_SQUASH_ENABLE, true);
#ifndef FPGA_SIM
  usleep(1000);
#endif // FPGA_SIM

#ifdef USE_SERIAL_PORT
  serial_port = new SerialPort("/dev/ttyUSB0");
  serial_port->start();
#endif // USE_SERIAL_PORT

  init_ram(args.image, ram_size, args.random_mem, args.seed);
  init_flash(args.flash_bin);

  init_device();

  difftest_init(args.enable_diff, ram_size);

  xdma_device->fpga_io(HOST_IO_ILA_TRIGGER, false);
#ifndef FPGA_SIM
  if (fpga_ila_dump_cmd) {
    if (!run_external_cmd(fpga_ila_dump_cmd, "ILA dump")) {
      fprintf(stderr, "[fpga-host] warning: failed to arm external ILA dump command\n");
      exit(1);
    }
  }
#endif // FPGA_SIM
  xdma_device->fpga_io(HOST_IO_RESET, false);
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
    if (args.warmup_instr != -1) {
      difftest[i]->warmup_display_stats();
    }
  }
}

int fpga_get_result(uint8_t step) {
  // Compare DUT and REF
  int trapCode = difftest_nstep(step, args.enable_diff);
  if (trapCode != STATE_RUNNING) {
    xdma_device->fpga_io(HOST_IO_ILA_TRIGGER, true);
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
