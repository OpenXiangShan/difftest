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
// Return checkpoint pid
// Return -1 if no checkpoint is needed (inst is not jump)
// Will raise error if the number of checkpoints exceeds limit
pid_t Runahead::do_instr_runahead_pc_guided(uint64_t jump_target_pc){
  assert(has_commit);
  // check if checkpoint list is full
  if(checkpoint_num_exceed_limit()){
    printf("ERROR: Checkpoint list is full, you may forget to free resolved checkpoints\n");
    assert(0);
  }
  // if not, fork to create a new checkpoint
  pid_t pid = fork();
  if(pid > 0){ // current process
    // I am your father
    checkpoints.push(pid);
    struct ExecutionGuide guide;
    guide.force_raise_exception = false;
    guide.force_set_jump_target = true;
    guide.jump_target = jump_target_pc;
    printf("force jump to %lx\n", jump_target_pc);
    proxy->guided_exec(&guide);
  } else {
    // sleep until it is wakeuped
    printf("I sleep\n");
    sleep(999);//TODO
  }
  printf("fork result %d\n", pid);
  return pid;
}

// Note: How to skip inst?
// * MMIO -> detect by ref
// * External int, time int, etc. -> should not influence run ahead

// Just normally run a inst
// 
// No checkpoint will be allocated
int Runahead::do_instr_runahead(){
  if(!has_commit){
    do_first_instr_runahead();
  }
  proxy->exec(1);
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

void Runahead::do_first_instr_runahead() {
  if (!has_commit && dut_ptr->runahead[0].valid && dut_ptr->runahead[0].pc == FIRST_INST_ADDRESS) {
    printf("The first instruction of core %d start to run ahead.\n", id);
    has_commit = 1;
    // nemu_this_pc = dut_ptr->runahead[0].pc;

    proxy->memcpy(0x80000000, get_img_start(), get_img_size(), DIFFTEST_TO_REF);
    // Manually setup simulator init regs,
    // for at this time, the first has not been initialied
    dut_ptr->csr.this_pc = FIRST_INST_ADDRESS; 
    proxy->regcpy(&dut_ptr->regs, DIFFTEST_TO_REF);
  }
}

int Runahead::step() { // override step() method
  static bool branch_reported;
  static uint64_t debug_branch_pc;
  if (dut_ptr->event.interrupt) {
    assert(0); //TODO
    do_interrupt();
  } else if(dut_ptr->event.exception) {
    // We ignored instrAddrMisaligned exception (0) for better debug interface
    // XiangShan should always support RVC, so instrAddrMisaligned will never happen
    assert(0); //TODO
    do_exception();
  } else {
    for (int i = 0; i < DIFFTEST_COMMIT_WIDTH && dut_ptr->runahead_commit[i].valid; i++) {
      dut_ptr->runahead_commit[i].valid = false;
      printf("Run ahead: jump inst %lx commited, free oldest checkpoint\n", 
        dut_ptr->runahead_commit[i].pc
      );
    }
    if(dut_ptr->runahead_redirect.valid) {
      dut_ptr->runahead_redirect.valid = false;
      printf("Run ahead: pc %lx redirect to %lx, recover cpid %lx\n",
        dut_ptr->runahead_redirect.pc,
        dut_ptr->runahead_redirect.target_pc,
        dut_ptr->runahead_redirect.checkpoint_id
      );
      // TODO: recover nemu
      printf("Run ahead: ignore run ahead req generated in current cycle\n");
      return 0; // ignore run ahead req generated in current cycle
    }
    for (int i = 0; i < DIFFTEST_RUNAHEAD_WIDTH && dut_ptr->runahead[i].valid; i++) {
      printf("Run ahead: pc %lx branch(reported by DUT) %x cpid %lx\n", 
        dut_ptr->runahead[i].pc, 
        dut_ptr->runahead[i].branch,
        dut_ptr->runahead[i].checkpoint_id 
      );
      // check if branch is reported by previous inst
      if(branch_reported) {
        do_instr_runahead_pc_guided(dut_ptr->runahead[i].pc);
        branch_reported = false;
      }
      if(dut_ptr->runahead[i].branch) { // TODO: add branch flag in hardware
        branch_reported = true;
        debug_branch_pc = dut_ptr->runahead[i].pc;
        // setup checkpoint here
      } else {
        do_instr_runahead();
      }
      dut_ptr->runahead[i].valid = 0;
    }
  }
  return 0;
}

// ---------------------------------------------------
// Run ahead control process
// ---------------------------------------------------

int runahead_init() {
  runahead = new Runahead*[NUM_CORES];
  assert(difftest);
  for (int i = 0; i < NUM_CORES; i++) {
    runahead[i] = new Runahead(i);
    // runahead uses difftest_core_state_t dut in Difftest
    // to be refactored later
    runahead[i]->dut_ptr = difftest[i]->get_dut(); 
    runahead[i]->ref_ptr = runahead[i]->get_ref(); 
    runahead[i]->update_nemuproxy(i);
  }
  printf("Simulator run ahead of commit enabled.\n");
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
