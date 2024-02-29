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

#include "runahead.h"
#include <queue>

void MemdepWatchWindow::commit_load() {
  assert(load_inflight.size() > 0);
  load_inflight.pop_front();
}

void MemdepWatchWindow::commit_store() {
  assert(store_inflight.size() > 0);
  store_inflight.pop_front();
}

void MemdepWatchWindow::commit_load(uint64_t pc) {
  assert(pc == load_inflight.front().pc);
  commit_load();
}

void MemdepWatchWindow::commit_store(uint64_t pc) {
  assert(pc == store_inflight.front().pc);
  commit_store();
}

void MemdepWatchWindow::watch_load(uint64_t pc, uint64_t vaddr) {
  MemInstInfo load;
  load.pc = pc;
  load.vaddr = vaddr;
  load_inflight.push_back(load);
  if (load_inflight.size() > MEMDEP_WINDOW_SIZE) {
    load_inflight.pop_back();
  }
  runahead_debug("Memdep watcher: start to watch load %lx, vaddr %lx\n", pc, vaddr);
}

void MemdepWatchWindow::watch_store(uint64_t pc, uint64_t vaddr) {
  MemInstInfo store;
  store.pc = pc;
  store.vaddr = vaddr;
  store_inflight.push_back(store);
  if (store_inflight.size() > MEMDEP_WINDOW_SIZE) {
    store_inflight.pop_back();
  }
  runahead_debug("Memdep watcher: start to watch store %lx, vaddr %lx\n", pc, vaddr);
}

bool MemdepWatchWindow::query_load_store_dep(uint64_t load_pc, uint64_t load_vaddr) {
  runahead_debug("Memdep watcher: detecting dependency for load %lx, vaddr %lx\n", load_pc, load_vaddr);
  bool has_dependency = false;
  for (auto i: store_inflight) {
    runahead_debug("Memdep watcher: %lx (%lx) %lx\n", i.vaddr, i.pc, load_vaddr);
    if (((i.vaddr | 0x7) == (load_vaddr | 0x7)) && !((load_vaddr & 0xf0000000) == 0x40000000) // hard code mmio
    ) {
      has_dependency = true;
      runahead_debug("Memdep watcher: load %lx dependency detected: store pc %lx, vaddr %lx\n", load_pc, i.pc,
                     load_vaddr);
      total_dependency++;
    }
  }
  return has_dependency;
}

void MemdepWatchWindow::update_pred_matrix(bool dut_result, bool ref_result) {
  pred_matrix[(int)dut_result][(int)ref_result]++;
}

void MemdepWatchWindow::print_pred_matrix() {
  printf("-------------- Memdep Watcher Result ----------------\n");
  printf("DUT ndep REF ndep %ld\n", pred_matrix[0][0]);
  printf("DUT ndep REF  dep %ld\n", pred_matrix[0][1]);
  printf("DUT  dep REF ndep %ld\n", pred_matrix[1][0]);
  printf("DUT  dep REF  dep %ld\n", pred_matrix[1][1]);
  printf("-----------------------------------------------------\n");
}

// MemdepWatchWindow::~MemdepWatchWindow(){
//   print_pred_matrix();
// }
