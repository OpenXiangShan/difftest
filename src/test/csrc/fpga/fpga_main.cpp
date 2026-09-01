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
#include "common.h"
#include "device.h"
#include "diffstate.h"
#include "difftest.h"
#include "flash.h"
#include "goldenmem.h"
#include "mpool.h"
#include "ram.h"
#include "refproxy.h"
#include "splitview.h"
#include "xdma.h"
#ifdef DIFFTEST_HOSTIF_GBUS
#include "gbus_transport.h"
#endif
#include <condition_variable>
#include <cstdlib>
#include <getopt.h>
#include <inttypes.h>
#include <mutex>
#include <stdint.h>
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
static const char *fpga_ila_arm_cmd = nullptr;
static const char *fpga_ila_upload_cmd = nullptr;
static bool fpga_ila_triggered = false;

void fpga_init();
void fpga_step();
void set_diff_ref_so(char *s);
void args_parsing(int argc, char *argv[]);
static bool run_external_cmd(const char *cmd, const char *tag);
static const char *select_fpga_ddr_load_cmd();

FpgaTransport *xdma_device = NULL;
#ifdef USE_SERIAL_PORT
SerialPort *serial_port = NULL;
#endif // USE_SERIAL_PORT
int main(int argc, const char *argv[]) {
  common_set_locale();

  fpga_ddr_load_cmd = select_fpga_ddr_load_cmd();
  fpga_ila_arm_cmd = std::getenv("FPGA_ILA_ARM_CMD");
  fpga_ila_upload_cmd = std::getenv("FPGA_ILA_UPLOAD_CMD");
  // UVHS uses the arm hook for its trigger-aware capture command. Keep the
  // established FPGA_ILA_ARM_CMD name for Vivado users and accept the UVHS
  // spelling without changing the normal flow.
  if (!fpga_ila_arm_cmd || !fpga_ila_arm_cmd[0]) {
    fpga_ila_arm_cmd = std::getenv("FPGA_ILA_DUMP_CMD");
  }
  args = parse_args(argc, argv);

  common_init(argv[0]);

  fpga_init();

  printf("fpga init\n");
  dprintf(STDERR_FILENO, "[fpga-host] transport start direct marker\n");
  fprintf(stderr, "[fpga-host] transport start call begin enable_diff=%d ptr=%p\n",
          args.enable_diff ? 1 : 0, static_cast<void *>(xdma_device));
  xdma_device->start(args.enable_diff); // Trigger stop by fpga_nstep
  fprintf(stderr, "[fpga-host] transport start call end\n");
  fpga_finish();
  if (signal_num != 0) {
    return 128 + signal_num;
  }
  return !(fpga_result == FPGA_GOODTRAP);
}

static const char *get_env_nonempty(const char *name) {
  const char *value = std::getenv(name);
  return value && value[0] ? value : nullptr;
}

static const char *select_fpga_ddr_load_cmd() {
  const char *cmd = get_env_nonempty("FPGA_DDR_LOAD_CMD");
  if (cmd) {
    return cmd;
  }

#ifdef UVHS
  cmd = get_env_nonempty("UVHS_DDR_LOAD_CMD");
  if (cmd) {
    return cmd;
  }

  return nullptr;
#else
  return nullptr;
#endif
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

#ifdef DIFFTEST_HOSTIF_GBUS
  auto *gbus_device = new GbusTransport();
  gbus_device->validate_guest_ram(_PMEM_BASE, ram_size);
  xdma_device = gbus_device;
#else
  xdma_device = new FpgaXdma();
#endif
  xdma_device->fpga_io(HOST_IO_CFG_RESET, true);
  sleep(1);

  init_ram(args.image, ram_size, args.random_mem, args.seed);
  init_flash(args.flash_bin);

  init_device();

  if (args.random_mem) {
    xdma_device->fpga_io(HOST_IO_SEED, args.seed);
    xdma_device->fpga_io(HOST_IO_RAM_SIZE_MB, ram_size_mb);
    printf("[fpga-host] init mem with seed = %d, size = %dMB\n", args.seed, ram_size_mb);
    uint32_t init_mem_start = uptime();
    xdma_device->fpga_io(HOST_IO_MEM_INIT, true);
    xdma_device->wait_fpga_io_done(HOST_IO_MEM_INIT, "memory random init");
    printf("[fpga-host] init mem done, elapsed = %ums\n", uptime() - init_mem_start);
  }

#if defined(CONFIG_USE_XDMA_H2C) || defined(DIFFTEST_HOSTIF_GBUS)
  auto *mem = dynamic_cast<MmapMemory *>(simMemory);
  assert(mem);
  uint64_t h2c_size = mem->pad_img_size(1024ull * 1024ull);
  uint64_t h2c_size_mb = h2c_size / (1024ull * 1024ull);
  printf("[fpga-host] H2C workload size: %" PRIu64 " bytes (%" PRIu64 "MB)\n", h2c_size, h2c_size_mb);
  uint32_t h2c_start = uptime();
  xdma_device->fpga_io(HOST_IO_H2C_SIZE_MB, static_cast<uint32_t>(h2c_size_mb));
  xdma_device->fpga_io(HOST_IO_MEM_H2C, true);
  xdma_device->h2c_load_workload(mem->as_ptr(), h2c_size);
  xdma_device->wait_fpga_io_done(HOST_IO_MEM_H2C, "memory H2C load");
  printf("[fpga-host] H2C load done, elapsed = %ums\n", uptime() - h2c_start);
#else // CONFIG_USE_XDMA_H2C || DIFFTEST_HOSTIF_GBUS
#ifdef FPGA_SIM
  xdma_sim_set_workload(args.image);
#endif // FPGA_SIM
#endif // CONFIG_USE_XDMA_H2C || DIFFTEST_HOSTIF_GBUS

  xdma_device->fpga_io(HOST_IO_RESET, true);
  xdma_device->fpga_io(HOST_IO_CPU_AXI_DELAY, args.cpu_axi_delay);
  uint32_t cpu_axi_delay = xdma_device->fpga_io_read(HOST_IO_CPU_AXI_DELAY);
  if (cpu_axi_delay != args.cpu_axi_delay) {
    fprintf(stderr, "[fpga-host] CPU AXI delay readback mismatch: wrote %" PRIu32 ", read %" PRIu32 "\n",
            args.cpu_axi_delay, cpu_axi_delay);
    exit(1);
  }
  printf("[fpga-host] CPU AXI delay = %" PRIu32 " cycles\n", cpu_axi_delay);
  xdma_device->fpga_io(HOST_IO_MEM_CPU, true);
  xdma_device->fpga_io(HOST_IO_DIFFTEST_ENABLE, args.enable_diff);
  xdma_device->fpga_io(HOST_IO_ILA_TRIGGER, false);
  xdma_device->fpga_io(HOST_IO_SQUASH_MAX_FUSED, static_cast<uint32_t>(args.squash_size));
  xdma_device->fpga_io(HOST_IO_SQUASH_ENABLE, args.enable_squash);
  printf("[fpga-host] squash enable = %d, size = %u\n", args.enable_squash, static_cast<unsigned>(args.squash_size));
  if (args.no_squash_after_instr != UINT64_MAX) {
    if (args.enable_squash) {
      printf("[fpga-host] squash will be disabled after %" PRIu64 " committed instructions\n",
             args.no_squash_after_instr);
    } else {
      printf("[fpga-host] --no-squash-after-instr is ignored because squash is disabled\n");
    }
  }
#ifndef FPGA_SIM
  usleep(1000);
#endif // FPGA_SIM

#ifdef USE_SERIAL_PORT
  const char *serial_port_device = std::getenv("FPGA_UART_PORT");
  if (!serial_port_device || !serial_port_device[0]) serial_port_device = "/dev/ttyUSB0";
  serial_port = new SerialPort(serial_port_device);
  serial_port->start();
#endif // USE_SERIAL_PORT

#if !defined(FPGA_SIM) && !defined(CONFIG_USE_XDMA_H2C) && !defined(DIFFTEST_HOSTIF_GBUS)
  if (fpga_ddr_load_cmd) {
#ifdef UVHS
    const char *ddr_load_tag = "UVHS DDR load";
#else
    const char *ddr_load_tag = "DDR load";
#endif
    if (!run_external_cmd(fpga_ddr_load_cmd, ddr_load_tag)) {
      exit(0);
    }
  }
#endif // !FPGA_SIM && !CONFIG_USE_XDMA_H2C && !DIFFTEST_HOSTIF_GBUS

  difftest_init(args.enable_diff, ram_size);

  xdma_device->fpga_io(HOST_IO_ILA_TRIGGER, false);
#ifndef FPGA_SIM
  if (fpga_ila_arm_cmd) {
    if (!run_external_cmd(fpga_ila_arm_cmd, "ILA arm")) {
      fprintf(stderr, "[fpga-host] warning: failed to run external ILA arm command\n");
      exit(1);
    }
  }
#endif // FPGA_SIM
  xdma_device->fpga_io(HOST_IO_RESET, false);
}

void fpga_finish() {
  delete xdma_device;

  if (signal_num == 0) {
    difftest_finish();
    goldenmem_finish();
    finish_device();
  }
#ifndef FPGA_SIM
  if (signal_num == 0 && fpga_ila_triggered && fpga_ila_upload_cmd) {
    if (!run_external_cmd(fpga_ila_upload_cmd, "ILA upload")) {
      fprintf(stderr, "[fpga-host] warning: failed to run external ILA upload command\n");
    }
  }
#endif // FPGA_SIM
  printf("difftest releases the fpga device and exits\n");
  common_splitview_finish();
#ifdef USE_SERIAL_PORT
  serial_port->stop();
  delete serial_port;
#endif // USE_SERIAL_PORT

  common_finish();

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
    fpga_ila_triggered = true;
    // Release endpoint backpressure while ILA collects post-trigger samples.
    xdma_device->fpga_io(HOST_IO_DIFFTEST_ENABLE, false);
    if (trapCode == STATE_GOODTRAP)
      return FPGA_GOODTRAP;
    else
      return FPGA_FAIL;
  }
  if (args.enable_squash && args.no_squash_after_instr != UINT64_MAX) {
    for (int i = 0; i < NUM_CORES; i++) {
      uint64_t instr_count = difftest[i]->get_trap_event()->instrCnt;
      if (instr_count >= args.no_squash_after_instr) {
        xdma_device->fpga_io(HOST_IO_SQUASH_ENABLE, false);
        args.enable_squash = false;
        printf("[fpga-host] disabled squash at core %d instruction %" PRIu64 "\n", i, instr_count);
        break;
      }
    }
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
