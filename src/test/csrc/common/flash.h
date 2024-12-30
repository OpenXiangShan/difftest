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

#ifndef __FLASH_H
#define __FLASH_H

#include "common.h"

struct flash_device_t {
  uint64_t *base;
  uint64_t size;
  char *img_path;
  uint64_t img_size;
};

extern flash_device_t flash_dev;

void init_flash(const char *flash_bin);
void flash_finish();

extern "C" void flash_read(uint32_t addr, uint64_t *data);
#endif // __FLASH_H
