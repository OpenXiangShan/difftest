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

#ifndef RUNAHEAD_H
#define RUNAHEAD_H

#include <queue>
#include "common.h"
#include "difftest.h"
#include "ram.h"

class Runahead: public Difftest {
public:
  Runahead(int coreid);
  pid_t free_checkpoint();
  void recover_checkpoint(int checkpoint_id);
  void restart();
  void update_debug_info(void* dest_buffer);
  void run_first_instr();
  int step();
  bool checkpoint_num_exceed_limit();
  int do_instr_runahead();
  pid_t do_instr_runahead_pc_guided(uint64_t jump_target_pc);

  void do_first_instr_runahead();

  difftest_core_state_t *dut_ptr;
  difftest_core_state_t *ref_ptr;

private:
#define RUN_AHEAD_CHECKPOINT_SIZE 64
  std::queue<pid_t> checkpoints;
};

extern Runahead** runahead;
int init_runahead_worker();
int runahead_init();
int runahead_step();

#endif