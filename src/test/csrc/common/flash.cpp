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

#include <sys/mman.h>

#include "common.h"
#include "flash.h"

static uint64_t *flash_base;
static long flash_bin_size = 0;
static char *flash_path = NULL;

unsigned long EMU_FLASH_SIZE = DEFAULT_EMU_FLASH_SIZE;

char *get_flash_path() { return flash_path;  }
long get_flash_size() { return flash_bin_size; }

extern "C" void flash_read(uint32_t addr, uint64_t *data) {
  //addr must be 8 bytes aligned first
  uint32_t aligned_addr = addr & FLASH_ALIGH_MASK;
  uint64_t rIdx = aligned_addr / sizeof(uint64_t);
  if (rIdx >= EMU_FLASH_SIZE / sizeof(uint64_t)) {
    printf("[warning] read addr %x is out of bound\n",addr);
    *data = 0;
  }else{
    *data = flash_base[rIdx];
  }
}

void init_flash(const char *flash_bin) {
  flash_base = (uint64_t *)mmap(NULL, EMU_FLASH_SIZE, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0);
  if (flash_base == (uint64_t *)MAP_FAILED) {
    printf("Warning: Insufficient phisical memory for flash\n");
    EMU_FLASH_SIZE = 10 * 1024UL;   //10 KB
    flash_base = (uint64_t *)mmap(NULL, EMU_FLASH_SIZE, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0);
    if (flash_base == (uint64_t *)MAP_FAILED) {
      printf("Error: Cound not mmap 0x%lx bytes for flash\n", EMU_FLASH_SIZE);
      assert(0);
    }
  }
  printf("Using simulated %luB flash\n", EMU_FLASH_SIZE);

  if(!flash_bin)
  {
    /** no specified flash_path ,use defualt 3 instructions*/
    printf("[warning]no valid flash bin path, use preset flash instead\n");
    // addiw   t0,zero,1
    // slli    to,to,  0x1f
    // jr      t0
    flash_base[0] = 0x01f292930010029b;
    flash_base[1] = 0x00028067;
    return;
  }

  /** no specified flash_path ,use defualt 3 instructions*/
  flash_path = (char *)flash_bin;
  printf("[info]use %s as flash bin\n",flash_path);   

  FILE *flash_fp = fopen(flash_path, "r");
  if(!flash_fp)
  {
    eprintf(ANSI_COLOR_MAGENTA "[error] flash img not found\n");
    exit(1);
  }
  
  fseek(flash_fp, 0, SEEK_END);
  flash_bin_size = ftell(flash_fp);
  if (flash_bin_size > EMU_FLASH_SIZE) {
    printf("[warning] flash image size %ld bytes is out of bound, cut the image into %ld bytes\n",flash_bin_size,EMU_FLASH_SIZE);
    flash_bin_size = EMU_FLASH_SIZE;
  }
  fseek(flash_fp, 0, SEEK_SET);
  int ret = fread(flash_base, flash_bin_size, 1, flash_fp);
  assert(ret = 1);
  fclose(flash_fp);
}
