/***************************************************************************************
* Copyright (c) 2020-2023 Institute of Computing Technology, Chinese Academy of Sciences
* Copyright (c) 2020-2021 Peng Cheng Laboratory
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
#include "string.h"
#ifndef CONFIG_NO_DIFFTEST
#include "difftest.h"
#endif // CONFIG_NO_DIFFTEST
#include "flash.h"
#ifndef CONFIG_NO_DIFFTEST
#include "goldenmem.h"
#endif // CONFIG_NO_DIFFTEST
#include "ram.h"
#ifndef CONFIG_NO_DIFFTEST
#include "refproxy.h"
#endif // CONFIG_NO_DIFFTEST
#include "svdpi.h"
#include <common.h>
#include <locale.h>
#ifdef CONFIG_DIFFTEST_PERFCNT
#include "perf.h"
#endif // CONFIG_DIFFTEST_PERFCNT
#include "remote_bitbang.h"
#ifdef FPGA_SIM
#include "xdma_sim.h"
#endif // FPGA_SIM

static bool has_reset = false;
static char *workload_list = NULL;
static CommonArgs args;

enum {
  SIMV_RUN,
  SIMV_GOODTRAP,
  SIMV_EXCEED,
  SIMV_FAIL,
  SIMV_WARMUP,
} simv_state;

extern "C" void set_bin_file(char *s) {
  printf("ram image:%s\n", s);
  args.image = strdup(s);
}

extern "C" void set_copy_ram_offset(char *s) {
  args.copy_ram_offset = parse_ramsize(s);
}

extern "C" void set_flash_bin(char *s) {
  printf("flash image:%s\n", s);
  args.flash_bin = strdup(s);
}

extern "C" void set_overwrite_nbytes(uint64_t len) {
  args.overwrite_nbytes = len;
}

extern "C" void set_ram_size(char *size) {
  printf("ram size: %s\n", size);
  args.ram_size = strdup(size);
}

extern "C" void set_overwrite_autoset() {
  FILE *fp = fopen(args.gcpt_restore, "rb");
  if (fp == NULL) {
    printf("set the gcpt path before using auto set");
    return;
  }
  // Get the lower four bytes
  fseek(fp, 4, SEEK_SET);
  fread(&args.overwrite_nbytes, sizeof(uint32_t), 1, fp);
  fclose(fp);
}

// Support workload warms up and clean LogPerf after warmup instrs
extern "C" void set_warmup_instr(uint64_t instrs) {
  args.warmup_instr = instrs;
  printf("Warmup instrs:%ld\n", instrs);
}

extern "C" void set_gcpt_bin(char *s) {
  args.gcpt_restore = strdup(s);
}

extern "C" void set_max_instrs(uint64_t mc) {
  printf("set max instrs: %lu\n", mc);
  args.max_instr = mc;
}

extern "C" void set_ref_trace() {
  printf("dump_ref_trace is enabled\n");
  args.enable_ref_trace = true;
}

extern "C" void set_commit_trace() {
  printf("dump_commit_trace is enabled\n");
  args.enable_commit_trace = true;
}

extern "C" uint64_t get_stuck_limit() {
#ifdef CONFIG_NO_DIFFTEST
  return 0;
#else
  return Difftest::stuck_limit;
#endif // CONFIG_NO_DIFFTEST
}

#ifndef CONFIG_NO_DIFFTEST
extern const char *difftest_ref_so;
extern "C" void set_diff_ref_so(char *s) {
  printf("diff-test ref so:%s\n", s);
  difftest_ref_so = strdup(s);
}
#else
extern "C" void set_diff_ref_so(char *s) {
  printf("difftest is not enabled. +diff=%s is ignore.\n", s);
}
#endif // CONFIG_NO_DIFFTEST

extern "C" void set_workload_list(char *s) {
  workload_list = strdup(s);
  printf("set workload list %s \n", workload_list);
}

bool switch_workload_completed = false;
int switch_workload() {
  static FILE *fp = fopen(workload_list, "r");
  if (fp) {
    char name[128];
    int num;
    if (fscanf(fp, "%s %d", name, &num) == 2) {
      set_bin_file(name);
      set_max_instrs(num);
    } else if (feof(fp)) {
      printf("Workload list is completed\n");
      switch_workload_completed = true;
      fclose(fp);
      return 1;
    } else {
      printf("Unknown workload list format\n");
      fclose(fp);
      return 1;
    }
  } else {
    printf("Fail to open workload list %s\n", workload_list);
    return 1;
  }
  return 0;
}

extern "C" bool workload_list_completed() {
  return switch_workload_completed;
}

extern "C" void set_no_diff() {
  printf("disable diff-test\n");
  args.enable_diff = false;
}

extern "C" void set_simjtag() {
  enable_simjtag = true;
}

extern "C" uint8_t simv_init() {
  if (workload_list != NULL) {
    if (switch_workload())
      return 1;
  }
  common_init("simv");

  uint64_t ram_size = DEFAULT_EMU_RAM_SIZE;
  if (args.ram_size) {
    ram_size = parse_ramsize(args.ram_size);
  }
  init_ram(args.image, ram_size);
#ifdef WITH_DRAMSIM3
  dramsim3_init(nullptr, nullptr);
#endif
  if (args.gcpt_restore != NULL) {
    overwrite_ram(args.gcpt_restore, args.overwrite_nbytes);
  }
  if (args.copy_ram_offset) {
    copy_ram(args.copy_ram_offset);
  }

  init_flash(args.flash_bin);

#ifndef CONFIG_NO_DIFFTEST
  difftest_init(args.enable_diff, ram_size);
#endif // CONFIG_NO_DIFFTEST

  init_device();

#ifdef FPGA_SIM
  xdma_sim_open(0, false);
#endif // FPGA_SIM

  return 0;
}

#ifdef OUTPUT_CPI_TO_FILE
void output_cpi_to_file() {
  FILE *cpi_file = fopen(OUTPUT_CPI_TO_FILE, "w+");
  printf("OUTPUT CPI to %s\n", OUTPUT_CPI_TO_FILE);
  for (size_t i = 0; i < NUM_CORES; i++) {
    auto trap = difftest[i]->get_trap_event();
    auto warmup = difftest[i]->warmup_info;
    uint64_t instrCnt = trap->instrCnt - warmup.instrCnt;
    uint64_t cycleCnt = trap->cycleCnt - warmup.cycleCnt;
    double cpi = (double)cycleCnt / instrCnt;
    // Record CPI, note the final instr/cycle will minus warmup instr/cycle
    fprintf(cpi_file, "%d,%.6lf\n", i, cpi);
  }
  fclose(cpi_file);
}
#endif

#ifdef CONFIG_DIFFTEST_DEFERRED_RESULT
svScope deferredResultScope;
extern "C" void set_deferred_result_scope();
void set_deferred_result_scope() {
  deferredResultScope = svGetScope();
}

extern "C" void set_deferred_result(uint8_t result);
void difftest_deferred_result(uint8_t result) {
  if (deferredResultScope == NULL) {
    printf("Error: Could not retrieve deferred result scope, set first\n");
    assert(deferredResultScope);
  }
  svSetScope(deferredResultScope);
  set_deferred_result(result);
}
#endif // CONFIG_DIFFTEST_DEFERRED_RESULT

#ifdef WITH_DRAMSIM3
extern "C" void simv_tick() {
  dramsim3_step();
}
#endif

void simv_finish() {
#ifdef OUTPUT_CPI_TO_FILE
  output_cpi_to_file();
#endif
  common_finish();
  flash_finish();

#ifndef CONFIG_NO_DIFFTEST
  difftest_finish();
  if (args.enable_diff) {
    goldenmem_finish();
  }
#endif // CONFIG_NO_DIFFTEST

  finish_device();
  delete simMemory;
  simMemory = nullptr;

#ifdef FPGA_SIM
  xdma_sim_close(0);
#endif //FPGA_SIM
}

int simv_get_result(uint8_t step) {
  // Assert Check
  if (assert_count > 0) {
    return SIMV_FAIL;
  }
#ifndef CONFIG_NO_DIFFTEST
  // Compare DUT and REF
  int trapCode = difftest_nstep(step, args.enable_diff);
  if (trapCode != STATE_RUNNING) {
    if (trapCode == STATE_GOODTRAP)
      return SIMV_GOODTRAP;
    else
      return SIMV_FAIL;
  }
  // Max Instr Limit Check
  if (args.max_instr != -1) {
    for (int i = 0; i < NUM_CORES; i++) {
      auto trap = difftest[i]->get_trap_event();
      if (trap->instrCnt >= args.max_instr) {
        return SIMV_EXCEED;
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
      Info("Warmup finished. The performance counters will be dumped and then reset.\n");
      // Record Instr/Cycle for soft warmup
      for (int i = 0; i < NUM_CORES; i++) {
        difftest[i]->warmup_record();
      }
      // perfCtrl_clean/dump will set according to SIMV_WARMUP
      return SIMV_WARMUP;
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
#endif // CONFIG_NO_DIFFTEST
  return SIMV_RUN;
}

void simv_display_result(int ret) {
#ifndef CONFIG_NO_DIFFTEST
  for (int i = 0; i < NUM_CORES; i++) {
    printf("Core %d: ", i);
    uint64_t pc = difftest[i]->get_trap_event()->pc;
    switch (ret) {
      case SIMV_GOODTRAP: eprintf(ANSI_COLOR_GREEN "HIT GOOD TRAP at pc = 0x%" PRIx64 "\n" ANSI_COLOR_RESET, pc); break;
      case SIMV_EXCEED:
        eprintf(ANSI_COLOR_YELLOW "EXCEEDING INSTR LIMIT at pc = 0x%" PRIx64 "\n" ANSI_COLOR_RESET, pc);
        break;
      case SIMV_FAIL: eprintf(ANSI_COLOR_RED "FAILED at pc = 0x%" PRIx64 "\n" ANSI_COLOR_RESET, pc); break;
      case SIMV_WARMUP: eprintf(ANSI_COLOR_MAGENTA "WARMUP DONE at pc = 0x%" PRIx64 "\n"); break;
      default: eprintf(ANSI_COLOR_RED "Unknown trap code: %d\n", ret);
    }
    difftest[i]->display_stats();
    if (args.warmup_instr != -1 && ret != SIMV_WARMUP) {
      difftest[i]->warmup_display_stats();
    }
  }
#endif // CONFIG_NO_DIFFTEST
}

static uint8_t simv_result = SIMV_RUN;
#ifdef CONFIG_DIFFTEST_DEFERRED_RESULT
extern "C" void simv_nstep(uint8_t step) {
  if (simv_result == SIMV_GOODTRAP || simv_result == SIMV_EXCEED || simv_result == SIMV_FAIL || difftest == NULL)
    return;
#else
extern "C" uint8_t simv_nstep(uint8_t step) {
#ifndef CONFIG_NO_DIFFTEST
  if (difftest == NULL)
    return 0;
#endif // CONFIG_NO_DIFFTEST
#endif // CONFIG_DIFFTEST_DEFERRED_RESULT

  int ret = simv_get_result(step);
  // Return result
  if (ret != SIMV_RUN) {
    simv_display_result(ret);
    if (ret == SIMV_GOODTRAP || ret == SIMV_EXCEED || ret == SIMV_FAIL) {
      simv_result = ret;
      simv_finish();
    }
#ifdef CONFIG_DIFFTEST_DEFERRED_RESULT
    difftest_deferred_result(ret);
    return;
#else
    return ret;
#endif // CONFIG_DIFFTEST_DEFERRED_RESULT
  }

#ifndef CONFIG_DIFFTEST_DEFERRED_RESULT
  return SIMV_RUN;
#endif // CONFIG_DIFFTEST_DEFERRED_RESULT
}
