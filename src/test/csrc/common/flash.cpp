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

#include "flash.h"
#include "common.h"
#ifdef CONFIG_DIFFTEST_PERFCNT
#include "perf.h"
#endif // CONFIG_DIFFTEST_PERFCNT

flash_device_t flash_dev = {
  nullptr,                // base
  DEFAULT_EMU_FLASH_SIZE, // size
  nullptr,                // img_path
  0                       // img_size
};

void flash_read(uint32_t addr, uint64_t *data) {
#ifdef CONFIG_DIFFTEST_PERFCNT
  difftest_calls[perf_flash_read]++;
  difftest_bytes[perf_flash_read] += 12;
#endif // CONFIG_DIFFTEST_PERFCNT
  if (!flash_dev.base) {
    return;
  }
  //addr must be 8 bytes aligned first
  uint32_t aligned_addr = addr & FLASH_ALIGH_MASK;
  uint64_t rIdx = aligned_addr / sizeof(uint64_t);
  if (rIdx >= flash_dev.size / sizeof(uint64_t)) {
    printf("[warning] read addr %x is out of bound\n", addr);
    *data = 0;
  } else {
    *data = flash_dev.base[rIdx];
  }
}

void init_flash(const char *flash_bin) {
  flash_dev.base = (uint64_t *)mmap(NULL, flash_dev.size, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0);
  if (flash_dev.base == (uint64_t *)MAP_FAILED) {
    printf("Warning: Insufficient phisical memory for flash\n");
    flash_dev.size = 10 * 1024UL; //10 KB
    flash_dev.base = (uint64_t *)mmap(NULL, flash_dev.size, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0);
    if (flash_dev.base == (uint64_t *)MAP_FAILED) {
      printf("Error: Cound not mmap 0x%lx bytes for flash\n", flash_dev.size);
      assert(0);
    }
  }
  Info("Using simulated %luB flash\n", flash_dev.size);

  if (!flash_bin) {
    /** no specified flash_path, use defualt 3 instructions */
    // addiw   t0,zero,1
    // slli    to,to,  0x1f
    // jr      t0
    flash_dev.base[0] = 0x01f292930010029b;
    flash_dev.base[1] = 0x00028067;
    flash_dev.img_size = 2 * sizeof(uint64_t);
    return;
  }

  flash_dev.img_path = (char *)flash_bin;
  Info("use %s as flash bin\n", flash_dev.img_path);

  FILE *flash_fp = fopen(flash_dev.img_path, "r");
  if (!flash_fp) {
    eprintf(ANSI_COLOR_MAGENTA "[error] flash img not found\n");
    exit(1);
  }

  fseek(flash_fp, 0, SEEK_END);
  flash_dev.img_size = ftell(flash_fp);
  if (flash_dev.img_size > flash_dev.size) {
    printf("[warning] flash image size %ld bytes is out of bound, cut the image into %ld bytes\n", flash_dev.img_size,
           flash_dev.size);
    flash_dev.img_size = flash_dev.size;
  }
  fseek(flash_fp, 0, SEEK_SET);
  int ret = fread(flash_dev.base, flash_dev.img_size, 1, flash_fp);
  assert(ret == 1);
  fclose(flash_fp);
}

void flash_finish() {
  munmap(flash_dev.base, flash_dev.size);
  flash_dev.base = NULL;
}
