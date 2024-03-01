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

#include <common.h>
#include <locale.h>
#include "difftest.h"
#include "device.h"
#include "goldenmem.h"
#include "ram.h"
#include "flash.h"
#include "refproxy.h"
#ifdef CONFIG_DIFFTEST_DEFERRED_RESULT
#include "svdpi.h"
#endif // CONFIG_DIFFTEST_DEFERRED_RESULT
#ifdef CONFIG_DIFFTEST_PERFCNT
#include "perf.h"
#endif // CONFIG_DIFFTEST_PERFCNT

static bool has_reset = false;
static char bin_file[256] = "ram.bin";
static char *flash_bin_file = NULL;
static bool enable_difftest = true;
static uint64_t max_instrs = 0;
static char *workload_list = NULL;

enum {
  SIMV_RUN,
  SIMV_DONE,
  SIMV_FAIL,
} simv_state;
static uint8_t simv_result = SIMV_RUN;
extern "C" void set_bin_file(char *s) {
  printf("ram image:%s\n",s);
  strcpy(bin_file, s);
}

extern "C" void set_flash_bin(char *s) {
  printf("flash image:%s\n",s);
  flash_bin_file = (char *)malloc(256);
  strcpy(flash_bin_file, s);
}

extern "C" void set_max_instrs(uint64_t mc) {
  printf("set max instrs: %lu\n", mc);
  max_instrs = mc;
}

extern const char *difftest_ref_so;
extern "C" void set_diff_ref_so(char *s) {
  printf("diff-test ref so:%s\n", s);
  char* buf = (char *)malloc(256);
  strcpy(buf, s);
  difftest_ref_so = buf;
}

extern "C" void set_workload_list(char *s) {
  workload_list = (char *)malloc(256);
  strcpy(workload_list, s);
  printf("set workload list %s \n", workload_list);
}

int switch_workload() {
  static FILE * fp = fopen(workload_list, "r");
  if (fp) {
    char name[128];
    int num;
    if (fscanf(fp, "%s %d", name, &num) == 2) {
      set_bin_file(name);
      set_max_instrs(num);
    }
    else if (feof(fp)) {
      printf("Workload list is completed\n");
      return 1;
    }
    else {
      printf("Unknown workload list format\n");
      return 1;
    }
  } else {
    printf("Fail to open workload list %s\n", workload_list);
    return 1;
  }
  return 0;
}

extern "C" void set_no_diff() {
  printf("disable diff-test\n");
  enable_difftest = false;
}

extern "C" uint8_t simv_init() {
  if (workload_list != NULL) {
    if(switch_workload())
      return 1;
  }
  common_init("simv");

  init_ram(bin_file, DEFAULT_EMU_RAM_SIZE);
  init_flash(flash_bin_file);

  difftest_init();
  init_device();
  if (enable_difftest) {
    init_goldenmem();
    init_nemuproxy(DEFAULT_EMU_RAM_SIZE);
  }
  simv_result = SIMV_RUN;
  return 0;
}

extern "C" uint8_t simv_step() {
  if (assert_count > 0) {
    return SIMV_FAIL;
  }

  if (difftest_state() != -1) {
    int trapCode = difftest_state();
    for (int i = 0; i < NUM_CORES; i++) {
      printf("Core %d: ", i);
      uint64_t pc = difftest[i]->get_trap_event()->pc;
      switch (trapCode) {
        case 0:
          eprintf(ANSI_COLOR_GREEN "HIT GOOD TRAP at pc = 0x%" PRIx64 "\n" ANSI_COLOR_RESET, pc);
          break;
        default:
          eprintf(ANSI_COLOR_RED "Unknown trap code: %d\n" ANSI_COLOR_RESET, trapCode);
      }
      difftest[i]->display_stats();
    }
    if (trapCode == 0)
      return SIMV_DONE;
    else
      return SIMV_FAIL;
  }

  if (max_instrs != 0) { // 0 for no limit
    auto trap = difftest[0]->get_trap_event();
    if(max_instrs < trap->instrCnt) {
      eprintf(ANSI_COLOR_GREEN "EXCEEDED MAX INSTR: %ld\n" ANSI_COLOR_RESET,max_instrs);
      difftest[0]->display_stats();
      return SIMV_DONE;
    }
  }

  if (enable_difftest) {
    if (difftest_step())
      return SIMV_FAIL;
    else
      return 0;
  } else {
    return 0;
  }
}

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


#ifdef CONFIG_DIFFTEST_DEFERRED_RESULT
extern "C" void simv_nstep(uint8_t step) {
  if (simv_result != SIMV_RUN)
    return;
#else
extern "C" uint8_t simv_nstep(uint8_t step) {
#endif // CONFIG_DIFFTEST_DEFERRED_RESULT
#ifdef CONFIG_DIFFTEST_PERFCNT
  difftest_calls[perf_simv_nstep] ++;
  difftest_bytes[perf_simv_nstep] += 1;
#endif // CONFIG_DIFFTEST_PERFCNT
  difftest_switch_zone();
  for (int i = 0; i < step; i++) {
    int ret = simv_step();
    if (ret) {
        simv_result = ret;
        difftest_finish();
#ifdef CONFIG_DIFFTEST_DEFERRED_RESULT
        difftest_deferred_result(ret);
        return;
#else
        return ret;
#endif // CONFIG_DIFFTEST_DEFERRED_RESULT
    }
  }
#ifndef CONFIG_DIFFTEST_DEFERRED_RESULT
  return 0;
#endif // CONFIG_DIFFTEST_DEFERRED_RESULT
}
