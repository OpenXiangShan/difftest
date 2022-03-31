/***************************************************************************************
* Copyright (c) 2020-2021 Institute of Computing Technology, Chinese Academy of Sciences
* Copyright (c) 2020-2021 Peng Cheng Laboratory
*
* XiangShan is licensed under Mulan PSL v2.
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

#include "common.h"
#include "flash.h"

static char *flash_path = DEFAULT_FLASH_IMAGE;
static long flash_bin_size = 0;

FILE *flash_fp   = NULL;

char *get_flash_path() { return flash_path;  }
long get_flash_size() { return flash_bin_size; }

extern "C"{
void flash_read(uint32_t addr, uint64_t *data) {
  uint32_t aligned_addr = addr & FLASH_ALIGH_MASK;
  fseek(flash_fp, aligned_addr, SEEK_SET);
  fread(data, 8, 1, flash_fp);
  //TODO: assert illegal access
}
}


void init_flash(const char *flash_bin) {
  if(!flash_bin)
  {
    printf("[warning]no valid flash bin path, use default %s instead\n", flash_path);
  } else{
    flash_path = (char *)flash_bin;
    printf("[info]use %s as flash bin\n",flash_path);   
  }

  flash_fp = fopen(flash_path, "r");
  
  if(!flash_fp)
  {
    eprintf(ANSI_COLOR_MAGENTA "[error] flash img not found\n");
    exit(1);
  }
  
  fseek(flash_fp, 0, SEEK_END);
  flash_bin_size = ftell(flash_fp);
  fseek(flash_fp, 0, SEEK_SET);
}

