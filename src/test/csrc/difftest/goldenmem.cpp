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

#include "common.h"
#include "compress.h"
#include "ram.h"
#include "refproxy.h"
#include <goldenmem.h>
#include <stdlib.h>

uint8_t *pmem;
uint8_t *pmem_flag; // 1: store update but load check skip; 0: update and check. others: assert
static uint64_t pmem_size;

void *guest_to_host(uint64_t addr) {
  return &pmem[addr];
}

void init_goldenmem() {
  pmem_size = simMemory->get_size();
  pmem = (uint8_t *)mmap(NULL, pmem_size, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE | MAP_NORESERVE, -1, 0);
  pmem_flag = (uint8_t *)mmap(NULL, pmem_size, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE | MAP_NORESERVE, -1, 0);
  if (pmem == (uint8_t *)MAP_FAILED) {
    Info("ERROR allocating physical memory. \n");
  }
  simMemory->clone_on_demand([](uint64_t offset, void *src, size_t n) { nonzero_large_memcpy(pmem + offset, src, n); },
                             true);
  ref_golden_mem = pmem;
}

void goldenmem_finish() {
  munmap(pmem, pmem_size);
  munmap(pmem_flag, pmem_size);
  pmem = NULL;
  pmem_flag = NULL;
}

void update_goldenmem(uint64_t addr, void *data, uint64_t mask, int len, uint8_t flag) {
  uint8_t *dataArray = (uint8_t *)data;
  for (int i = 0; i < len; i++) {
    if (((mask >> i) & 1) != 0) {
      paddr_write(addr + i, dataArray[i], flag, 1);
    }
  }
}

void read_goldenmem(uint64_t addr, void *data, uint64_t len, void *flag) {
  *(uint64_t *)data = paddr_read(addr, len);
  if (flag != NULL) {
    *(uint64_t *)flag = paddr_flag_read(addr, len);
  }
}

bool in_pmem(uint64_t addr) {
  return (PMEM_BASE <= addr) && (addr <= PMEM_BASE + simMemory->get_size() - 1);
}

static inline word_t pmem_read(uint64_t addr, int len) {
  void *p = &pmem[addr - PMEM_BASE];
  switch (len) {
    case 1: return *(uint8_t *)p;
    case 2: return *(uint16_t *)p;
    case 4: return *(uint32_t *)p;
    case 8: return *(uint64_t *)p;
    default: assert(0);
  }
}

static inline word_t pmem_flag_read(uint64_t addr, int len) {
  void *p = &pmem_flag[addr - PMEM_BASE];
  uint8_t *tmpArray = (uint8_t *)p;
  for (int i = 0; i < len; i++) {
    if (*(uint8_t *)(tmpArray + i) != 0 && *(uint8_t *)(tmpArray + i) != 1)
      assert(0);
  }
  switch (len) {
    case 1: return *(uint8_t *)p;
    case 2: return *(uint16_t *)p;
    case 4: return *(uint32_t *)p;
    case 8: return *(uint64_t *)p;
    default: assert(0);
  }
}

static inline void pmem_write(uint64_t addr, word_t data, word_t flag, int len) {
#ifdef DIFFTEST_STORE_COMMIT
  store_commit_queue_push(addr, data, len);
#endif

  void *p = &pmem[addr - PMEM_BASE];
  void *pf = &pmem_flag[addr - PMEM_BASE];
  switch (len) {
    case 1:
      *(uint8_t *)p = data;
      *(uint8_t *)pf = flag;
      return;
    case 2:
      *(uint16_t *)p = data;
      *(uint16_t *)pf = flag;
      return;
    case 4:
      *(uint32_t *)p = data;
      *(uint32_t *)pf = flag;
      return;
    case 8:
      *(uint64_t *)p = data;
      *(uint64_t *)pf = flag;
      return;
    default: assert(0);
  }
}

// Golden Memory Store Log
#ifdef ENABLE_STORE_LOG
#define GOLDENMEM_STORE_LOG_SIZE 1024
struct store_log {
  uint64_t addr;
  word_t org_data;
  word_t org_flag;
} goldenmem_store_log_buf[GOLDENMEM_STORE_LOG_SIZE];

bool goldenmem_store_log_enable = false;
int goldenmem_store_log_ptr = 0;

void goldenmem_set_store_log(bool enable) {
  goldenmem_store_log_enable = enable;
}

void goldenmem_store_log_reset() {
  goldenmem_store_log_ptr = 0;
}

void pmem_record_store(uint64_t addr) {
  if (goldenmem_store_log_enable) {
    // align to 8 byte
    addr = (addr >> 3) << 3;
    word_t rdata = pmem_read(addr, 8);
    word_t rflag = pmem_flag_read(addr, 8);
    goldenmem_store_log_buf[goldenmem_store_log_ptr].addr = addr;
    goldenmem_store_log_buf[goldenmem_store_log_ptr].org_data = rdata;
    goldenmem_store_log_buf[goldenmem_store_log_ptr].org_flag = rflag;
    ++goldenmem_store_log_ptr;
    if (goldenmem_store_log_ptr >= GOLDENMEM_STORE_LOG_SIZE)
      assert(0);
  }
}

void goldenmem_store_log_restore() {
  for (int i = goldenmem_store_log_ptr - 1; i >= 0; i--) {
    pmem_write(goldenmem_store_log_buf[i].addr, goldenmem_store_log_buf[i].org_data,
               goldenmem_store_log_buf[i].org_flag, 8);
  }
}
#endif // ENABLE_STORE_LOG

/* Memory accessing interfaces */

inline word_t paddr_read(uint64_t addr, int len) {
  if (in_pmem(addr))
    return pmem_read(addr, len);
  else {
    Info("[Hint] read not in pmem, maybe in speculative state! addr: %lx\n", addr);
    return 0;
  }
  return 0;
}

inline word_t paddr_flag_read(uint64_t addr, int len) {
  if (in_pmem(addr))
    return pmem_flag_read(addr, len);
  else {
    Info("[Hint] read not in pmem, maybe in speculative state! addr: %lx\n", addr);
    return 0;
  }
  return 0;
}

inline void paddr_write(uint64_t addr, word_t data, word_t flag, int len) {
  if (in_pmem(addr)) {
#ifdef ENABLE_STORE_LOG
    if (goldenmem_store_log_enable)
      pmem_record_store(addr);
#endif // ENABLE_STORE_LOG
    pmem_write(addr, data, flag, len);
  } else
    panic("write not in pmem!");
}
