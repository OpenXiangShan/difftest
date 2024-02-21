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
static char *checkpoint_list_path = NULL;
static bool enable_overr_gcpt = false;
static bool enable_difftest = true;

static int max_cycles = 0;
static int max_instrs = 0;

static int checkpoint_reset (char * file_path);
static char *checkpoint_affiliation = NULL;
static uint32_t checkpoin_idx = 0;

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

extern "C" void difftest_checkpoint_list (char * path) {
  checkpoint_list_path = (char *)malloc(256);
  checkpoint_affiliation = (char *)malloc(32);
  strcpy(checkpoint_list_path,path);
  printf("set ckpt path %s \n",checkpoint_list_path);
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

  difftest_commit_clean();

  printf("diff finish\n");
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
    auto trap = difftest[0]->get_trap_event();
    if(max_instrs < trap->instrCnt) {
      eprintf(ANSI_COLOR_GREEN "checkpoint reach the operating limit exit\n" ANSI_COLOR_RESET);
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

static uint64_t checkpoint_list_head = 0;
extern "C" char difftest_ram_reload() {
  assert(checkpoint_list_path);

  FILE * fp = fopen(checkpoint_list_path,"r");
  char file_name[128] = {0};
  if (fp == nullptr) {
    printf("Can't open fp file '%s'", checkpoint_list_path);
    return 1;
  }
  if (feof(fp)) {
    printf("the fp no more checkpoint \n");
    return 1;  
  }

  fseek(fp, checkpoint_list_head, SEEK_SET);

  if (fgets(file_name, 128, fp) == NULL) {
    return 1;
  }

  checkpoint_list_head = checkpoint_list_head + strlen(file_name);

  if (checkpoint_reset(file_name) == 1) {
    return 1;
  }

  fclose(fp);
  difftest_finish();
  simv_result = 0;

  return 0;
}

static int checkpoint_reset (char * file_name) {
	int line_len = strlen(file_name);
  char str_temp1[128] = {0};
  char str_temp2[128] = {0};
	if ('\n' == file_name[line_len - 1]) {
		file_name[line_len - 1] = '\0';
	}
  char * str1 = strrchr(file_name, '/');
  if (str1 != NULL)
    str1 ++;
  else 
    return 1;

  if (sscanf(str1,"_%d_0.%d_.gz", &checkpoin_idx, &max_instrs) != 2) {
    printf("get max instrs error \n");
    assert(0);
  }

  int str_len1 = strlen(str1);
  line_len = strlen(file_name);
  strncpy(str_temp1, file_name, (line_len - str_len1 - 1));
  char *str2 = strrchr(str_temp1, '/');
  if (str2 != NULL)
    str2 ++;
  else
    return 1;

  str_len1 = strlen(str_temp1);
  int str_len2 = strlen(str2);
  strncpy(str_temp2, str_temp1, (str_len1 - str_len2 - 1));

  char *str3 = strrchr(str_temp2, '/');
  if (str3 != NULL)
    str3 ++;
  else
    return 1;
  printf("checkpoint affiliation %s \n", str3);

  strcpy(checkpoint_affiliation, str3);
  strcpy(bin_file, file_name);
  return 0;
}