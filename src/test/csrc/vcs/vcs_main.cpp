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

static bool has_reset = false;
static char bin_file[256] = "ram.bin";
static char *flash_bin_file = NULL;
static char *gcpt_bin_file = NULL;
static bool enable_overr_gcpt = false;
static bool enable_difftest = true;

static int max_cycles = 0;
static int max_instrs = 0;


extern "C" void set_bin_file(char *s) {
  printf("ram image:%s\n",s);
  strcpy(bin_file, s);
}

extern "C" void set_flash_bin(char *s) {
  printf("flash image:%s\n",s);
  flash_bin_file = (char *)malloc(256);
  strcpy(flash_bin_file, s);
}

extern "C" void set_gcpt_bin(char *s) {
  printf("gcpt image:%s\n",s);
  enable_overr_gcpt = true;
  gcpt_bin_file = (char *)malloc(256);
  strcpy(gcpt_bin_file, s);
}

extern const char *difftest_ref_so;
extern "C" void set_diff_ref_so(char *s) {
  printf("diff-test ref so:%s\n", s);
  char* buf = (char *)malloc(256);
  strcpy(buf, s);
  difftest_ref_so = buf;
}

extern "C" void set_no_diff() {
  printf("disable diff-test\n");
  enable_difftest = false;
}

extern "C" void set_max_cycles(long mc) {
  max_cycles = mc;
}

extern "C" void set_max_instrs(long mc) {
  max_instrs = mc;
}

extern "C" void get_ipc(long cycles) {
  uint64_t now_cycles = (uint64_t)cycles;
  uint64_t now_instrs = difftest_commit_sum(0);// Take the first core as the standard for now
  double CPI = (double)now_cycles / now_instrs;
  double IPC = (double)now_instrs / now_cycles;
  printf("this simpoint CPI = %lf, IPC = %lf, Instrcount %ld, Cycle %ld\n",
   CPI, IPC, now_instrs, now_cycles);
}

extern "C" void simv_init() {
  common_init("simv");
  if (enable_overr_gcpt) {
    init_ram(bin_file, DEFAULT_EMU_RAM_SIZE, gcpt_bin_file);
  } 
  else {
    init_ram(bin_file, DEFAULT_EMU_RAM_SIZE);
  }

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


  static int cycles = 0;
  if (max_cycles != 0) { // 0 for no limit
    if (cycles >= max_cycles) {
      eprintf(ANSI_COLOR_YELLOW "EXCEEDED MAX CYCLE:%d\n" ANSI_COLOR_RESET, max_cycles);
      return 1;
    }
    cycles ++;
  }

  if (max_instrs != 0) { // 0 for no limit
    if(max_instrs < difftest_commit_sum(0)) {
      return 0xff;
    }
  }


  if (difftest_state() != -1) {
    int trapCode = difftest_state();
    switch (trapCode) {
      case 0:
        eprintf(ANSI_COLOR_GREEN "HIT GOOD TRAP\n" ANSI_COLOR_RESET);
        break;
      default:
        eprintf(ANSI_COLOR_RED "Unknown trap code: %d\n" ANSI_COLOR_RESET, trapCode);
    }
    return trapCode + 1;
  }

  if (enable_difftest) {
    return difftest_step();
  } else {
    return 0;
  }
}

#ifdef CONFIG_DIFFTEST_DEFERRED_RESULT
static int simv_result = 0;
extern "C" void simv_nstep(uint8_t step) {
  if (simv_result)
    return;
  for (int i = 0; i < step; i++) {
    int ret = simv_step();
    if (ret)
      simv_result = ret;
  }
}
extern "C" int simv_result_fetch() {
  return simv_result;
}
#else
extern "C" int simv_nstep(uint8_t step) {
  for(int i = 0; i < step; i++) {
    int ret = simv_step();
    if(ret)
      return ret;
  }
  return 0;
}
#endif // TB_DEFERRED_RESULT
