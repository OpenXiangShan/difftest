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

#include "device.h"
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

static bool has_reset = false;
static char bin_file[256] = "/dev/zero";
static char *flash_bin_file = NULL;
static char *gcpt_restore_bin = NULL;
static bool enable_difftest = true;
static uint64_t max_instrs = 0;
static char *workload_list = NULL;
static uint32_t overwrite_nbytes = 0xe00;
static uint64_t ram_size = 0;
static uint64_t warmup_instr = 0;

enum {
  SIMV_RUN,
  SIMV_GOODTRAP,
  SIMV_EXCEED,
  SIMV_FAIL,
  SIMV_WARMUP,
} simv_state;

extern "C" void set_bin_file(char *s) {
  printf("ram image:%s\n", s);
  strcpy(bin_file, s);
}

extern "C" void set_flash_bin(char *s) {
  printf("flash image:%s\n", s);
  flash_bin_file = (char *)malloc(256);
  strcpy(flash_bin_file, s);
}

extern "C" void set_overwrite_nbytes(uint64_t len) {
  overwrite_nbytes = len;
}

extern "C" void set_ram_size(uint64_t size) {
  printf("ram size:0x%lx\n", size);
  ram_size = size;
}

extern "C" void set_overwrite_autoset() {
  FILE *fp = fopen(gcpt_restore_bin, "rb");
  if (fp == NULL) {
    printf("set the gcpt path before using auto set");
    return;
  }
  // Get the lower four bytes
  fseek(fp, 4, SEEK_SET);
  fread(&overwrite_nbytes, sizeof(uint32_t), 1, fp);
  fclose(fp);
}

// Support workload warms up and clean LogPerf after warmup instrs
extern "C" void set_warmup_instr(uint64_t instrs) {
  warmup_instr = instrs;
  printf("Warmup instrs:%ld\n", instrs);
}

extern "C" void set_gcpt_bin(char *s) {
  gcpt_restore_bin = (char *)malloc(256);
  strcpy(gcpt_restore_bin, s);
}

extern "C" void set_max_instrs(uint64_t mc) {
  printf("set max instrs: %lu\n", mc);
  max_instrs = mc;
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
  char *buf = (char *)malloc(256);
  strcpy(buf, s);
  difftest_ref_so = buf;
}
#else
extern "C" void set_diff_ref_so(char *s) {
  printf("difftest is not enabled. +diff=%s is ignore.\n", s);
}
#endif // CONFIG_NO_DIFFTEST

extern "C" void set_workload_list(char *s) {
  workload_list = (char *)malloc(256);
  strcpy(workload_list, s);
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
  enable_difftest = false;
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

  ram_size = ram_size > 0 ? ram_size : DEFAULT_EMU_RAM_SIZE;
  init_ram(bin_file, ram_size);
#ifdef WITH_DRAMSIM3
  dramsim3_init(nullptr, nullptr);
#endif
  if (gcpt_restore_bin != NULL) {
    overwrite_ram(gcpt_restore_bin, overwrite_nbytes);
  }
  init_flash(flash_bin_file);

#ifndef CONFIG_NO_DIFFTEST
  difftest_init();
#endif // CONFIG_NO_DIFFTEST

  init_device();

#ifndef CONFIG_NO_DIFFTEST
  if (enable_difftest) {
    init_goldenmem();
    init_nemuproxy(ram_size);
  }
#endif // CONFIG_NO_DIFFTEST

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
  if (enable_difftest) {
    goldenmem_finish();
  }
#endif // CONFIG_NO_DIFFTEST

  finish_device();
  delete simMemory;
  simMemory = nullptr;
}

int simv_get_result(uint8_t step) {
  // Assert Check
  if (assert_count > 0) {
    return SIMV_FAIL;
  }
#ifndef CONFIG_NO_DIFFTEST
  // Compare DUT and REF
  int trapCode = difftest_nstep(step, enable_difftest);
  if (trapCode != STATE_RUNNING) {
    if (trapCode == STATE_GOODTRAP)
      return SIMV_GOODTRAP;
    else
      return SIMV_FAIL;
  }
  // Max Instr Limit Check
  if (max_instrs != 0) {
    for (int i = 0; i < NUM_CORES; i++) {
      auto trap = difftest[i]->get_trap_event();
      if (trap->instrCnt >= max_instrs) {
        return SIMV_EXCEED;
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
      Info("Warmup finished. The performance counters will be dumped and then reset.\n");
      // Record Instr/Cycle for soft warmup
      for (int i = 0; i < NUM_CORES; i++) {
        difftest[i]->warmup_record();
      }
      // perfCtrl_clean/dump will set according to SIMV_WARMUP
      return SIMV_WARMUP;
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
    if (warmup_instr != 0 && ret != SIMV_WARMUP) {
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
