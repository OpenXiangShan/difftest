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

#ifndef __MEMORY_PADDR_H__
#define __MEMORY_PADDR_H__

#include "common.h"
#include "ram.h"
#include <assert.h>
#include <cstdint>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef uint64_t uint64_t;
typedef uint64_t word_t;

extern uint8_t *pmem;
extern uint8_t *pmem_flag;

void init_goldenmem();
void goldenmem_finish();

extern "C" void update_goldenmem(uint64_t addr, void *data, uint64_t mask, int len, uint8_t flag = 0);
extern "C" void read_goldenmem(uint64_t addr, void *data, uint64_t len, void *flag = NULL);

/* convert the guest physical address in the guest program to host virtual address in NEMU */
void *guest_to_host(uint64_t addr);
/* convert the host virtual address in NEMU to guest physical address in the guest program */
uint64_t host_to_guest(void *addr);

word_t paddr_read(uint64_t addr, int len);
word_t paddr_flag_read(uint64_t addr, int len);
void paddr_write(uint64_t addr, word_t data, word_t flag, int len);
bool is_sfence_safe(uint64_t addr, int len);
bool in_pmem(uint64_t addr);

#ifdef ENABLE_STORE_LOG
void goldenmem_set_store_log(bool enable);
void goldenmem_store_log_reset();
void goldenmem_store_log_restore();
#endif
#endif
