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
#include <cerrno>
#include <cinttypes>
#include <condition_variable>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <getopt.h>
#include <mutex>
#include <strings.h>
#include <sys/wait.h>
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
static bool env_flag_enabled(const char *name, bool default_value);
static uint64_t env_uint64(const char *name, uint64_t default_value);
static bool run_external_cmd(const char *cmd, const char *tag);
static void clone_sim_memory_to_ref();

FpgaXdma *xdma_device = NULL;
#ifdef USE_SERIAL_PORT
SerialPort *serial_port = NULL;
#endif // USE_SERIAL_PORT
int main(int argc, const char *argv[]) {
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

static bool env_flag_enabled(const char *name, bool default_value) {
  const char *value = std::getenv(name);
  if (!value || !value[0]) {
    return default_value;
  }
  if (!strcmp(value, "0") || !strcasecmp(value, "false") || !strcasecmp(value, "no") || !strcasecmp(value, "off")) {
    return false;
  }
  if (!strcmp(value, "1") || !strcasecmp(value, "true") || !strcasecmp(value, "yes") || !strcasecmp(value, "on")) {
    return true;
  }
  fprintf(stderr, "[fpga-host] invalid boolean %s=%s, using %s\n", name, value, default_value ? "true" : "false");
  return default_value;
}

static uint64_t env_uint64(const char *name, uint64_t default_value) {
  const char *value = std::getenv(name);
  if (!value || !value[0]) {
    return default_value;
  }

  errno = 0;
  char *end = nullptr;
  uint64_t result = strtoull(value, &end, 0);
  if (errno != 0 || end == value || *end != '\0') {
    fprintf(stderr, "[fpga-host] invalid integer %s=%s, using 0x%" PRIx64 "\n", name, value, default_value);
    return default_value;
  }
  return result;
}

static void clone_sim_memory_to_ref() {
  if (!args.enable_diff || !simMemory) {
    return;
  }

  simMemory->clone_on_demand(
      [](uint64_t offset, void *src, size_t n) {
        uint64_t dest_addr = PMEM_BASE + offset;
        for (int i = 0; i < NUM_CORES; i++) {
          if (difftest[i] && difftest[i]->proxy) {
            difftest[i]->proxy->mem_init(dest_addr, src, n, DUT_TO_REF);
          }
        }
      },
      true);
  printf("[fpga-host] cloned simMemory to NEMU before FPGA reset release\n");
}

void fpga_init() {
  xdma_device = new FpgaXdma();
#ifndef FPGA_SIM
  xdma_device->fpga_io(HOST_IO_RESET, true);
  xdma_device->fpga_io(HOST_IO_DIFFTEST_ENABLE, args.enable_diff);
  xdma_device->fpga_io(HOST_IO_ILA_TRIGGER, false);
  usleep(1000);
#endif // FPGA_SIM

#ifdef USE_SERIAL_PORT
  serial_port = new SerialPort("/dev/ttyUSB0");
  serial_port->start();
#endif // USE_SERIAL_PORT

  init_ram(args.image, DEFAULT_EMU_RAM_SIZE);
  init_flash(args.flash_bin);

  init_device();

#ifndef FPGA_SIM
  if (fpga_ddr_load_cmd) {
    if (!run_external_cmd(fpga_ddr_load_cmd, "DDR load")) {
      exit(0);
    }
  }
#ifdef USE_XDMA_DDR_LOAD
  else {
    xdma_device->ddr_load_workload(args.image);
  }
#endif // USE_XDMA_DDR_LOAD
#endif // FPGA_SIM

#ifndef FPGA_SIM
  if (args.enable_diff && env_flag_enabled("FPGA_XDMA_SYNC_DDR", true)) {
    uint64_t sync_addr = env_uint64("FPGA_XDMA_SYNC_DDR_ADDR", 0);
    uint64_t sync_bytes = env_uint64("FPGA_XDMA_SYNC_DDR_BYTES", simMemory->get_img_size());
    bool compare = env_flag_enabled("FPGA_XDMA_SYNC_DDR_COMPARE", true);
    bool strict_compare = env_flag_enabled("FPGA_XDMA_SYNC_DDR_STRICT", false);
    if (!xdma_device->sync_ddr_to_sim_memory(sync_addr, sync_bytes, compare, strict_compare)) {
      fprintf(stderr, "[fpga-host] failed to sync DDR initial state through XDMA\n");
      exit(1);
    }
  }
#endif // FPGA_SIM

  difftest_init(args.enable_diff, DEFAULT_EMU_RAM_SIZE);
  clone_sim_memory_to_ref();

#ifndef FPGA_SIM
  xdma_device->fpga_io(HOST_IO_ILA_TRIGGER, false);
  if (fpga_ila_dump_cmd) {
    if (!run_external_cmd(fpga_ila_dump_cmd, "ILA dump")) {
      fprintf(stderr, "[fpga-host] warning: failed to arm external ILA dump command\n");
      exit(1);
    }
  }
  xdma_device->fpga_io(HOST_IO_RESET, false);
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
    if (args.warmup_instr != -1) {
      difftest[i]->warmup_display_stats();
    }
  }
}

int fpga_get_result(uint8_t step) {
  // Compare DUT and REF
  int trapCode = difftest_nstep(step, args.enable_diff);
  if (trapCode != STATE_RUNNING) {
#ifndef FPGA_SIM
    xdma_device->fpga_io(HOST_IO_ILA_TRIGGER, true);
#endif // FPGA_SIM
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
