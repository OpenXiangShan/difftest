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

#include <goldenmem.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <time.h>
#include "common.h"
#include "compress.h"
#include "ram.h"
#include "refproxy.h"
#include "ram.h"

void* sparse_gd_mm = NULL;
void * get_gd_sparsemm(){
#ifdef CONFIG_USE_SPARSEMM
  if (sparse_gd_mm==NULL){
    sparse_gd_mm = sparse_mem_new(4, 1024); //4kB
  }
  assert(sparse_gd_mm != NULL);
#endif
  return sparse_gd_mm;
}

uint8_t *pmem;
static uint64_t pmem_size;

void* guest_to_host(uint64_t addr) { 
  if (simMemory->get_type() == T_SparseMemory){
    Info("Sparse Memory do not support \"guest_to_host\" convert\n");
    assert(0);
  }
  return &pmem[addr]; 
}

void init_goldenmem() {
  pmem_size = simMemory->get_size();
  if (simMemory->get_type() == T_SparseMemory){
    sparse_mem_copy(get_gd_sparsemm(), simMemory->as_ptr());
    ref_golden_mem = (uint8_t *) get_gd_sparsemm();
  }else{
    pmem = (uint8_t *)mmap(NULL, pmem_size, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0);
    if (pmem == (uint8_t *)MAP_FAILED) {
      printf("ERROR allocating physical memory. \n");
    }
    simMemory->clone_on_demand([](uint64_t offset, void *src, size_t n) {
      nonzero_large_memcpy(pmem + offset, src, n);
    }, true);
    ref_golden_mem = pmem;
  }
}

void goldenmem_finish() {
  if (simMemory->get_type() == T_SparseMemory){
    sparse_mem_del(sparse_gd_mm);
  }else{
    munmap(pmem, pmem_size);
    pmem = NULL;
  }
}

void update_goldenmem(uint64_t addr, void *data, uint64_t mask, int len) {
  uint8_t *dataArray = (uint8_t*)data;
  for (int i = 0; i < len; i++) {
		if (((mask >> i) & 1) != 0) {
			paddr_write(addr + i, dataArray[i], 1);
		}
  }
}

void read_goldenmem(uint64_t addr, void *data, uint64_t len) {
  *(uint64_t*)data = paddr_read(addr, len);
}

bool in_pmem(uint64_t addr) {
  if (simMemory->get_type() == T_SparseMemory){
    return true;
  }
  return (PMEM_BASE <= addr) && (addr <= PMEM_BASE + simMemory->get_size() - 1);
}

static inline word_t pmem_read(uint64_t addr, int len) {
  if (simMemory->get_type() == T_SparseMemory){
    return sparse_mem_wread(get_gd_sparsemm(), addr, len);
  }
  void *p = &pmem[addr - PMEM_BASE];
  switch (len) {
    case 1: return *(uint8_t  *)p;
    case 2: return *(uint16_t *)p;
    case 4: return *(uint32_t *)p;
    case 8: return *(uint64_t *)p;
    default: assert(0);
  }
}

static inline void pmem_write(uint64_t addr, word_t data, int len) {
#ifdef DIFFTEST_STORE_COMMIT
  store_commit_queue_push(addr, data, len);
#endif
  if (simMemory->get_type() == T_SparseMemory){
    return sparse_mem_wwrite(get_gd_sparsemm(), addr, len, data);
  }
  void *p = &pmem[addr - PMEM_BASE];
  switch (len) {
    case 1:
      *(uint8_t  *)p = data;
      return;
    case 2:
      *(uint16_t *)p = data;
      return;
    case 4:
      *(uint32_t *)p = data;
      return;
    case 8:
      *(uint64_t *)p = data;
      return;
    default: assert(0);
  }
}

/* Memory accessing interfaces */

inline word_t paddr_read(uint64_t addr, int len) {
  if (in_pmem(addr)) return pmem_read(addr, len);
  else {
    printf("[Hint] read not in pmem, maybe in speculative state! addr: %lx\n", addr);
    return 0;
  }
  return 0;
}

inline void paddr_write(uint64_t addr, word_t data, int len) {
  if (in_pmem(addr)) pmem_write(addr, data, len);
  else panic("write not in pmem!");
}