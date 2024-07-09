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

#ifndef MEMDEP_H
#define MEMDEP_H

#include "common.h"
#include <deque>

typedef struct MemInstInfo {
  uint64_t pc;
  uint64_t vaddr;
} MemInstInfo;

class MemdepWatchWindow {
public:
  void commit_load();
  void commit_store();
  void commit_load(uint64_t pc);
  void commit_store(uint64_t pc);
  void watch_load(uint64_t pc, uint64_t vaddr);
  void watch_store(uint64_t pc, uint64_t vaddr);
  bool query_load_store_dep(uint64_t load_pc, uint64_t load_vaddr);
  void update_pred_matrix(bool dut_result, bool ref_result);
  void print_pred_matrix();

private:
  std::deque<MemInstInfo> store_inflight;
  std::deque<MemInstInfo> load_inflight;
  uint64_t total_dependency = 0;
  uint64_t pred_matrix[2][2] = {};
};

#endif
