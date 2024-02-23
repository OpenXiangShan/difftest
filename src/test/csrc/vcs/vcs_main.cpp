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

#define STATE_LIMIT_EXCEEDED 3

static bool has_reset = false;
static char bin_file[256] = "ram.bin";
static char *flash_bin_file = NULL;
static bool enable_difftest = true;
static uint64_t max_instrs = 0;

static int ram_path_reset(char *file_name);
static char *work_load_list_path = NULL;

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
  max_instrs = mc;
}

extern const char *difftest_ref_so;
extern "C" void set_diff_ref_so(char *s) {
  printf("diff-test ref so:%s\n", s);
  char* buf = (char *)malloc(256);
  strcpy(buf, s);
  difftest_ref_so = buf;
}

extern "C" void set_workload_list(char *path) {
  work_load_list_path = (char *)malloc(256);
  strcpy(work_load_list_path, path);
  printf("set checkpoint list path %s \n", work_load_list_path);
}

extern "C" void set_no_diff() {
  printf("disable diff-test\n");
  enable_difftest = false;
}

extern "C" void simv_init() {
  common_init("simv");

  init_ram(bin_file, DEFAULT_EMU_RAM_SIZE);
  init_flash(flash_bin_file);

  difftest_init();
  init_device();
  if (enable_difftest) {
    init_goldenmem();
    init_nemuproxy(DEFAULT_EMU_RAM_SIZE);
  }
}

extern "C" int simv_step() {
  if (assert_count > 0) {
    return 1;
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
    return trapCode + 1;
  }

  if (max_instrs != 0) { // 0 for no limit
    auto trap = difftest[0]->get_trap_event();
    if(max_instrs < trap->instrCnt) {
      eprintf(ANSI_COLOR_GREEN "EXCEEDED MAX INSTR: %ld\n" ANSI_COLOR_RESET,max_instrs);
      return STATE_LIMIT_EXCEEDED;
    }
  }

  if (enable_difftest) {
    return difftest_step();
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

extern "C" void set_deferred_result();
void difftest_deferred_result() {
  if (deferredResultScope == NULL) {
    printf("Error: Could not retrieve deferred result scope, set first\n");
    assert(deferredResultScope);
  }
  svSetScope(deferredResultScope);
  set_deferred_result();
}

static int simv_result = 0;
extern "C" void simv_nstep(uint8_t step) {
#ifdef CONFIG_DIFFTEST_PERFCNT
  difftest_calls[perf_simv_nstep] ++;
  difftest_bytes[perf_simv_nstep] += 1;
#endif // CONFIG_DIFFTEST_PERFCNT
  if (simv_result)
    return;
  difftest_switch_zone();
  for (int i = 0; i < step; i++) {
    int ret = simv_step();
    if (ret) {
        simv_result = ret;
        break;
    }
  }
  if (simv_result && work_load_list_path == NULL) {
    difftest_finish();
    difftest_deferred_result();
  }
}
#else
extern "C" int simv_nstep(uint8_t step) {
#ifdef CONFIG_DIFFTEST_PERFCNT
  difftest_calls[perf_simv_nstep] ++;
  difftest_bytes[perf_simv_nstep] += 1;
#endif // CONFIG_DIFFTEST_PERFCNT
  difftest_switch_zone();
  for(int i = 0; i < step; i++) {
    int ret = simv_step();
    if(ret) {
      if (work_load_list_path == NULL) {
        difftest_finish();
      }
      return ret;
    }
  }
  return 0;
}
#endif // CONFIG_DIFFTEST_DEFERRED_RESULT

static uint64_t work_load_list_head = 0;
extern "C" char ram_reload() {
  assert(work_load_list_path);

  FILE * fp = fopen(work_load_list_path, "r");
  char file_name[128] = {0};
  if (fp == nullptr) {
    printf("Can't open fp file '%s'", work_load_list_path);
    return 1;
  }

  fseek(fp, work_load_list_head, SEEK_SET);

  if (feof(fp)) {
    printf("the fp no more checkpoint \n");
    return 1;  
  }
  if (fgets(file_name, 128, fp) == NULL) {
    return 1;
  }

  work_load_list_head = work_load_list_head + strlen(file_name);

  if (ram_path_reset(file_name)) {
    return 1;
  }

  fclose(fp);
  difftest_finish();
#ifdef CONFIG_DIFFTEST_DEFERRED_RESULT
  simv_result = 0;
#endif
  return 0;
}

static int ram_path_reset(char *file_name) {
	int line_len = strlen(file_name);

  if(file_name[0] == '\0') {
    return 1;
  }

	if ('\n' == file_name[line_len - 1]) {
		file_name[line_len - 1] = '\0';
	}

  if (sscanf(file_name, "%s %ld", bin_file, &max_instrs) != 2) {
    printf("no more work load\n");
    return 1;
  }

  return 0;
}
