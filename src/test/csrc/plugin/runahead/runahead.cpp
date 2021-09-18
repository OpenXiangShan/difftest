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

#include <vector>
#include "runahead.h"

// ---------------------------------------------------
// Run ahead worker process
// ---------------------------------------------------

Runahead **runahead = NULL;

Runahead::Runahead(int coreid): Difftest(coreid) {
  
}

bool Runahead::checkpoint_num_exceed_limit() {
  return checkpoints.size() >= RUN_AHEAD_CHECKPOINT_SIZE;
}

// If current inst is a jump, set up a checkpoint for recovering, 
// then set jump target to jump_target_pc
//
// Return checkpoint id
// Return -1 if no checkpoint is needed (inst is not jump)
// Will raise error if the number of checkpoints exceeds limit
int Runahead::ref_runahead_pc_guided(uint64_t jump_target_pc){
  assert(has_commit);
  // check if checkpoint list is full
  if(checkpoint_num_exceed_limit()){
    printf("run ahead checkpoint num exceeds limit");
  }

  // if not, fork to create a new checkpoint
  pid_t pid = fork();
  if(pid > 0){ // current process

  }
  return 0;
}

// Note: How to skip inst?
// * MMIO -> detect by ref
// * External int, time int, etc. -> should not influence run ahead

// Just normally run a inst
// 
// No checkpoint will be allocated
int Runahead::ref_runahead(){
  if(!has_commit){
    run_first_instr();
  } else {
    proxy->exec(1);
  }
  return 0;
}

// Free the oldest checkpoint
// 
// Should be called when a branch is solved or that inst is committed 
// Note that all checkpoints should be freed after that inst commits 
pid_t Runahead::free_checkpoint() {
  assert(checkpoints.size() > 0);
  pid_t to_be_freed_pid = checkpoints.front();
  checkpoints.pop();
  // TODO
  return to_be_freed_pid;
}
// resolve_branch(int checkpoint_pid)

// Recover execuation state from checkpoint
void Runahead::recover_checkpoint(int checkpoint_pid) {
  // pop queue until we gey the same id
  while(checkpoints.size() > 0) {
    pid_t to_be_checked_pid = checkpoints.back();
    if(to_be_checked_pid == checkpoint_pid) {
      // wake up
      // stop it self
      return;
    }
  }
  assert(0); // failed to recover checkpoint
}

// Restart run ahead process
void Runahead::restart() {
}

// Sync debug info from ref
void Runahead::update_debug_info(void* dest_buffer) {

}

void Runahead::run_first_instr() {
  printf("Waiting for the first valid inst...\n");
  // if (!has_commit && dut.runahead[0].valid && dut.runahead[0].pc == FIRST_INST_ADDRESS) {
  if (!has_commit) {
    printf("The first instruction of core %d start to run ahead.\n", id);
    has_commit = 1;
    nemu_this_pc = dut.runahead[0].pc;

    proxy->memcpy(0x80000000, get_img_start(), get_img_size(), DIFFTEST_TO_REF);
    proxy->regcpy(dut_regs_ptr, DIFFTEST_TO_REF);
  }
}

int Runahead::step() { // override step() method
  printf("astrop\n");
  return 0;
}

// ---------------------------------------------------
// Run ahead control process
// ---------------------------------------------------

int runahead_init() {
  runahead = new Runahead*[NUM_CORES];
  for (int i = 0; i < NUM_CORES; i++) {
    runahead[i] = new Runahead(i);
  }
  for (int i = 0; i < NUM_CORES; i++) {
    runahead[i]->update_nemuproxy(i);
  }
  printf("Simulator run ahead of commit enabled.");
  return 0;
}

int runahead_step() {
  for (int i = 0; i < NUM_CORES; i++) {
    int ret = runahead[i]->step();
    if (ret) {
      return ret;
    }
  }
  return 0;
}

int init_runahead_worker(){
    // run ahead simulator needs its own addr space
    // init simulator
    // TODO
    // wait for singals
    return 0;
}
