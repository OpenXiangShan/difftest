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
#include "memdep.h"

// ---------------------------------------------------
// Run ahead master process
// ---------------------------------------------------

Runahead **runahead = NULL;
int runahead_req_msgq_id = 0;
int runahead_resp_msgq_id = 0;
bool runahead_is_slave = false;

Runahead::Runahead(int coreid) : Difftest(coreid) {}

void Runahead::remove_all_checkpoints() {
  while (checkpoints.size()) {
    runahead_debug("Cleaning checkpoints, try to kill %d\n", checkpoints.back().pid);
    kill(checkpoints.back().pid, SIGKILL);
    checkpoints.pop_back();
    runahead_debug("Cleaning checkpoints, %zu remaining\n", checkpoints.size());
  }
}

void Runahead::remove_msg_queues() {
  if (runahead_req_msgq_id) {
    runahead_debug("Try to remove runahead_req_msgq %d\n", runahead_req_msgq_id);
    msgctl(runahead_req_msgq_id, IPC_RMID, NULL);
  }
  if (runahead_resp_msgq_id) {
    runahead_debug("Try to remove runahead_resp_msgq %d\n", runahead_resp_msgq_id);
    msgctl(runahead_resp_msgq_id, IPC_RMID, NULL);
  }
}

int runahead_init() {
  runahead = new Runahead *[NUM_CORES];
  assert(difftest);
  for (int i = 0; i < NUM_CORES; i++) {
    runahead[i] = new Runahead(i);
    // runahead uses DiffTestState dut in Difftest
    // to be refactored later
    runahead[i]->dut_ptr = difftest[i]->get_dut();
    runahead[i]->ref_ptr = runahead[i]->get_ref();
    runahead[i]->update_nemuproxy(i, 0);
  }
  runahead_debug("Allocate msgq for %s\n", emu_path);
  key_t req_msgq_key = ftok(emu_path, 'a');
  runahead_req_msgq_id = msgget(req_msgq_key, IPC_CREAT | 0600);
  key_t resp_msgq_key = ftok(emu_path, 'b');
  runahead_resp_msgq_id = msgget(resp_msgq_key, IPC_CREAT | 0600);
  if ((runahead_req_msgq_id <= 0) || (runahead_resp_msgq_id <= 0)) {
    runahead_debug("%s\n", std::strerror(errno));
    runahead_debug("Failed to create run ahead message queue.\n");
    assert(0);
  }
  runahead_debug("Simulator run ahead of commit enabled.\n");
  return 0;
}

int runahead_cleanup() {
  for (int i = 0; i < NUM_CORES; i++) {
    runahead[i]->remove_all_checkpoints();
    runahead[i]->remove_msg_queues();
  }
  return 0;
}

Runahead::~Runahead() {
  remove_all_checkpoints();
  remove_msg_queues();
}

// Runahead exec a step
//
// Should be called for every cycle emulated by Emulator
int runahead_step() {
  for (int i = 0; i < NUM_CORES; i++) {
    int ret = runahead[i]->step();
    if (ret) {
      return ret;
    }
  }
  return 0;
}

bool Runahead::checkpoint_num_exceed_limit() {
#ifdef AUTO_RUNAHEAD_CHECKPOINT_GC
  return false;
#endif
  return checkpoints.size() >= RUN_AHEAD_CHECKPOINT_SIZE;
}

// Just normally run a inst
//
// No checkpoint will be allocated.
// If it is the first valid inst to be runahead, some init work will be done
// in do_first_instr_runahead().
int Runahead::do_instr_runahead() {
  if (!state->has_commit) {
    do_first_instr_runahead();
  }
  request_slave_runahead();
  return 0;
}

// If current inst is a jump, set up a checkpoint for recovering,
// then set jump target to jump_target_pc
//
// Return checkpoint pid
// Return -1 if no checkpoint is needed (inst is not jump)
// Will raise error if the number of checkpoints exceeds limit
pid_t Runahead::do_instr_runahead_pc_guided(uint64_t jump_target_pc) {
  assert(state->has_commit);
  // check if checkpoint list is full
  if (checkpoint_num_exceed_limit()) {
    runahead_debug("Checkpoint list is full, you may forget to free resolved checkpoints\n");
    assert(0);
  }
  // if not, fork to create a new checkpoint
  pid_t pid = request_slave_runahead_pc_guided(jump_target_pc);
  runahead_debug("fork result %d\n", pid);
  return pid;
}

// Note: How to skip inst?
// * MMIO -> detect by ref
// * External int, time int, etc. -> should not influence run ahead

// Register a new checkpoint
//
// Note that this function will only add a checkpoint record to checkpoints deque
// fork() and wait() affair should be done before calling this function
void Runahead::register_checkpoint(pid_t pid, uint64_t branch_checkpoint_id, uint64_t branch_pc, bool may_replay) {
  // register new checkpoint
  RunaheadCheckpoint checkpoint;
  checkpoint.pid = pid;
  checkpoint.checkpoint_id = branch_checkpoint_id;
  checkpoint.pc = branch_pc;
  checkpoint.may_replay = may_replay;
  checkpoints.push_back(checkpoint);
  branch_reported = false;
  // lazy checkpoint gc
#ifdef AUTO_RUNAHEAD_CHECKPOINT_GC
  if (checkpoints.size() > AUTO_RUNAHEAD_CHECKPOINT_GC_THRESHOLD) {
    free_checkpoint();
  }
#endif
}

// Free the oldest checkpoint
//
// Should be called when a branch is solved or that inst is committed.
// Note that all checkpoints should be freed after that inst commits.
pid_t Runahead::free_checkpoint() {
  static int num_checkpoint_to_be_freed = 0;
  num_checkpoint_to_be_freed++;
  // debug_print_checkpoint_list();
  while (num_checkpoint_to_be_freed && checkpoints.size() > 1) { // there should always be at least 1 active slave
    pid_t to_be_freed_pid = checkpoints.front().pid;
    runahead_debug("Free checkpoint %lx\n", checkpoints.front().checkpoint_id);
    kill(to_be_freed_pid, SIGKILL);
    checkpoints.pop_front();
    num_checkpoint_to_be_freed--;
  }
  return 0;
}

// Recover execuation state from checkpoint
void Runahead::recover_checkpoint(uint64_t checkpoint_id) {
  debug_print_checkpoint_list();
  assert(checkpoints.size() > 1); // Must maintain at least 1 active slave
  // pop queue until we get the same id
  while (checkpoints.size() > 0) {
    pid_t to_be_checked_cpid = checkpoints.back().checkpoint_id;
    kill(checkpoints.back().pid, SIGKILL);
    runahead_debug("kill %d\n", checkpoints.back().pid);
    if (to_be_checked_cpid == checkpoint_id) {
      runahead_debug("Recover to checkpoint %lx.\n", checkpoint_id);
      branch_checkpoint_id = checkpoints.back().checkpoint_id;
      branch_pc = checkpoints.back().pc;
      if (!checkpoints.back().may_replay) {
        // if checkpoint was set up by branch, it will jump to the right target
        branch_reported = true;
        runahead_debug("Checkpoint generated by branch\n");
      } else {
        // if checkpoint was set up by load, it will replay itself
        branch_reported = false; // cancel the old inst
        runahead_debug("Checkpoint generated by replayable inst\n");
      }
      checkpoints.pop_back();
      return; // we have got the right checkpoint
    }
    checkpoints.pop_back();
  }
  runahead_debug("Failed to recover runahead checkpoint.\n");
  assert(0); // failed to recover checkpoint
}

// Restart run ahead process
void Runahead::restart() {}

// Sync debug info from ref
void Runahead::update_debug_info(void *dest_buffer) {}

void Runahead::do_first_instr_runahead() {
  if (!state->has_commit && dut_ptr->runahead[0].valid && dut_ptr->runahead[0].pc == FIRST_INST_ADDRESS) {
    runahead_debug("The first instruction of core %d start to run ahead.\n", id);
    state->has_commit = 1;
    // nemu_this_pc = dut_ptr->runahead[0].pc;

    proxy->memcpy(PMEM_BASE, get_img_start(), get_img_size(), DIFFTEST_TO_REF);
    // Manually setup simulator init regs,
    // for at this time, the first has not been initialied
    dut_ptr->csr.this_pc = FIRST_INST_ADDRESS;
    proxy->regcpy(&dut_ptr->regs, DIFFTEST_TO_REF);
    DynamicSimulatorConfig nemu_config;
    nemu_config.ignore_illegal_mem_access = true;
    proxy->update_config(&nemu_config);
    init_runahead_slave();
  }
}

#ifdef QUERY_MEM_ACCESS
int Runahead::memdep_check(int i, RunaheadResponseQuery *ref_mem_query_result) {
  auto mem_access_info = &ref_mem_query_result->result.mem_access_info;
  // if(dut_ptr->runahead_memdep_pred[i].valid){
  // TODO: double check
  if (mem_access_info->mem_access || dut_ptr->runahead_memdep_pred[i].valid) {
    dut_ptr->runahead_memdep_pred[i].valid = false;
    if (mem_access_info->mem_access_is_load) {
      // TODO: is seems dut_ptr->runahead_memdep_pred[i].is_load != mem_access_info->mem_access_is_load
      // runahead_debug("Runahead step: ref pc %lx dut pc %lx\n", mem_access_info->pc, dut_ptr->runahead_memdep_pred[i].pc);
      // assert(mem_access_info->mem_access_is_load);
      auto dut_result = dut_ptr->runahead_memdep_pred[i].need_wait;
      auto ref_result = mem_access_info->ref_need_wait;
      runahead_debug("Runahead memdep_check: ref is load %x dut %x ref %x\n", mem_access_info->mem_access_is_load,
                     dut_result, ref_result);
      memdep_watcher->update_pred_matrix(dut_result, ref_result);
      if (dut_result != ref_result) {
        runahead_debug("mem pred result mismatch: pc %lx dut %x ref %x\n", mem_access_info->pc, dut_result, ref_result);
      }
    }
    dut_ptr->runahead_memdep_pred[i].oracle_vaddr = mem_access_info->mem_access_vaddr;
  }
  return 0;
}
#endif

int Runahead::step() { // override step() method
  static uint64_t ticks = 0;
  ticks++;
  if (dut_ptr->event.interrupt) {
    assert(0); //TODO
    do_interrupt();
  } else if (dut_ptr->event.exception) {
    // We ignored instrAddrMisaligned exception (0) for better debug interface
    // XiangShan should always support RVC, so instrAddrMisaligned will never happen
    assert(0); //TODO
    do_exception();
  } else {
    if (dut_ptr->runahead_redirect.valid) {
      dut_ptr->runahead_redirect.valid = false;
      runahead_debug("Run ahead: pc %lx redirect to %lx, recover cpid %lx\n", dut_ptr->runahead_redirect.pc,
                     dut_ptr->runahead_redirect.target_pc, dut_ptr->runahead_redirect.checkpoint_id);
      // no need to recover checkpoint if checkpoint has not been established
      if (branch_reported && branch_checkpoint_id == dut_ptr->runahead_redirect.checkpoint_id) {
        runahead_debug("Trying to recover checkpoint %lx, which has not been established yet\n",
                       dut_ptr->runahead_redirect.checkpoint_id);
        branch_reported = true; // next run ahead request will report the correct target pc
      } else {
        runahead_debug("Trying to recover checkpoint %lx\n", dut_ptr->runahead_redirect.checkpoint_id);
        recover_checkpoint(dut_ptr->runahead_redirect.checkpoint_id); // will set branch_reported
      }
      runahead_debug("Run ahead: ignore run ahead req generated in current cycle\n");
      for (int i = 0; i < DIFFTEST_RUNAHEAD_WIDTH; i++) {
        // ignore run ahead req generated in current cycle
        dut_ptr->runahead[i].valid = false;
      }
      return 0;
    }
    for (int i = 0; i < DIFFTEST_COMMIT_WIDTH && dut_ptr->runahead_commit[i].valid; i++) {
      dut_ptr->runahead_commit[i].valid = false;
      dut_ptr->runahead_memdep_pred[i].valid = false;
      runahead_debug("Run ahead: jump inst %lx commited, free oldest checkpoint\n", dut_ptr->runahead_commit[i].pc);
      free_checkpoint();
    }
    for (int i = 0; i < DIFFTEST_RUNAHEAD_WIDTH && dut_ptr->runahead[i].valid; i++) {
      runahead_debug("Run ahead: pc %lx branch(reported by DUT) %x cpid %lx\n", dut_ptr->runahead[i].pc,
                     dut_ptr->runahead[i].branch, dut_ptr->runahead[i].checkpoint_id);
      // check if branch is reported by previous inst
      if (branch_reported) {
        pid_t pid = do_instr_runahead_pc_guided(dut_ptr->runahead[i].pc);
        // no need to call do_query_mem_access(), as runahead_pc_guided is branch inst
        // register new checkpoint
        register_checkpoint(pid, branch_checkpoint_id, branch_pc, may_replay);
        runahead_debug("New checkpoint: pid %d cpid %lx pc %lx\n", pid, branch_checkpoint_id, branch_pc);
#ifdef QUERY_MEM_ACCESS
        RunaheadResponseQuery ref_mem_access;
        do_query_mem_access(&ref_mem_access);
        // runahead_debug("dut runahead pc %lx ref pc %lx\n",
        runahead_debug("dut runahead pc %lx ref pc %lx ticks %ld\n", branch_pc,
                       ref_mem_access.result.mem_access_info.pc, ticks);
        loop_if_not(branch_pc == ref_mem_access.result.mem_access_info.pc);
        if (!(branch_pc == ref_mem_access.result.mem_access_info.pc)) {
          printf("Error: dut runahead pc %lx ref pc %lx ticks %ld\n", branch_pc,
                 ref_mem_access.result.mem_access_info.pc, ticks);
        }
        memdep_check(i, &ref_mem_access);
#endif
      }
      assert(!(dut_ptr->runahead[i].branch && dut_ptr->runahead[i].may_replay));
      if (dut_ptr->runahead[i].branch || dut_ptr->runahead[i].may_replay) {
        branch_reported = true;
        may_replay = dut_ptr->runahead[i].may_replay;
        branch_checkpoint_id = dut_ptr->runahead[i].checkpoint_id;
        branch_pc = dut_ptr->runahead[i].pc;
        // setup checkpoint here
      } else {
        do_instr_runahead();
#ifdef QUERY_MEM_ACCESS
        RunaheadResponseQuery ref_mem_access;
        do_query_mem_access(&ref_mem_access);
        // runahead_debug("dut runahead pc %lx ref pc %lx\n",
        runahead_debug("dut runahead pc %lx ref pc %lx ticks %ld\n", dut_ptr->runahead[i].pc,
                       ref_mem_access.result.mem_access_info.pc, ticks);
        loop_if_not(dut_ptr->runahead[i].pc == ref_mem_access.result.mem_access_info.pc);
        if (!(dut_ptr->runahead[i].pc == ref_mem_access.result.mem_access_info.pc)) {
          printf("Error: dut runahead pc %lx ref pc %lx ticks %ld\n", dut_ptr->runahead[i].pc,
                 ref_mem_access.result.mem_access_info.pc, ticks);
        }
        memdep_check(i, &ref_mem_access);
#endif
      }
      dut_ptr->runahead[i].valid = 0;
    }
  }
  return 0;
}

// Request slave to run a single inst
pid_t Runahead::request_slave_runahead() {
  RunaheadRequest request;
  RunaheadResponsePid resp;
  request.message_type = RUNAHEAD_MSG_REQ_EXEC;
  assert_no_error(msgsnd(runahead_req_msgq_id, &request, sizeof(RunaheadRequest) - sizeof(long int), 0));
  assert_no_error(
      msgrcv(runahead_resp_msgq_id, &resp, sizeof(RunaheadResponsePid) - sizeof(long int), RUNAHEAD_MSG_RESP_EXEC, 0));
  assert(resp.pid == 0);
  return 0;
}

// Request slave to run a inst with assigned jump target pc
//
// Return checkpoint pid. Checkpoint is generated before inst exec.
pid_t Runahead::request_slave_runahead_pc_guided(uint64_t target_pc) {
  RunaheadRequest request;
  request.message_type = RUNAHEAD_MSG_REQ_GUIDED_EXEC;
  request.target_pc = target_pc;
  assert_no_error(msgsnd(runahead_req_msgq_id, &request, sizeof(RunaheadRequest) - sizeof(long int), 0));
  RunaheadResponsePid resp_exec;
  RunaheadResponsePid resp_fork;
  assert_no_error(msgrcv(runahead_resp_msgq_id, &resp_exec, sizeof(RunaheadResponsePid) - sizeof(long int),
                         RUNAHEAD_MSG_RESP_EXEC, 0));
  assert_no_error(msgrcv(runahead_resp_msgq_id, &resp_fork, sizeof(RunaheadResponsePid) - sizeof(long int),
                         RUNAHEAD_MSG_RESP_FORK, 0));
  assert(resp_fork.pid > 0); // fork succeed
  return resp_fork.pid;
}

// Request slave to run a single inst
void Runahead::request_slave_refquery(void *resp_target, int type) {
  RunaheadRequest request;
  request.message_type = RUNAHEAD_MSG_REQ_QUERY;
  request.query_type = type;
  assert_no_error(msgsnd(runahead_req_msgq_id, &request, sizeof(RunaheadRequest) - sizeof(long int), 0));
  assert_no_error(msgrcv(runahead_resp_msgq_id, resp_target, sizeof(RunaheadResponseQuery) - sizeof(long int),
                         RUNAHEAD_MSG_RESP_QUERY, 0));
  return;
}

// Print all valid items in checkpoint_list (oldest first)
void Runahead::debug_print_checkpoint_list() {
  for (auto i: checkpoints) {
    runahead_debug("checkpoint: checkpoint_id %lx pc %lx pid %d\n", i.checkpoint_id, i.pc, i.pid);
  }
  fflush(stdout);
}

#ifdef QUERY_MEM_ACCESS
void Runahead::do_query_mem_access(RunaheadResponseQuery *result_buffer) {
  auto mem_access_info = &result_buffer->result.mem_access_info;
  request_slave_refquery(result_buffer, REF_QUERY_MEM_EVENT);
  runahead_debug("Query result: pc %lx mem access %x isload %x vaddr %lx ref_need_wait %x\n", mem_access_info->pc,
                 mem_access_info->mem_access, mem_access_info->mem_access_is_load, mem_access_info->mem_access_vaddr,
                 mem_access_info->ref_need_wait);
  return;
}
#endif

// ---------------------------------------------------
// Run ahead slave process
// ---------------------------------------------------

// Slave process listens to msg queue, exec simulator according to instructions in msgq
void Runahead::runahead_slave() {
  runahead_debug("runahead_slave inited\n");
  RunaheadRequest request;
  RunaheadResponsePid resp;
  RunaheadResponseQuery resp_query;
  resp.message_type = RUNAHEAD_MSG_RESP_EXEC;
  resp.pid = 0;
  resp_query.message_type = RUNAHEAD_MSG_RESP_QUERY;
  auto mem_access_info = &resp_query.result.mem_access_info;
  while (1) {
    assert_no_error(msgrcv(runahead_req_msgq_id, &request, sizeof(request) - sizeof(long int), 0, 0));
    runahead_debug("Received msg type: %ld\n", request.message_type);
    switch (request.message_type) {
      case RUNAHEAD_MSG_REQ_EXEC:
        proxy->exec(1);
        runahead_debug("Run ahead: proxy->exec(1)\n");
        assert_no_error(msgsnd(runahead_resp_msgq_id, &resp, sizeof(RunaheadResponsePid) - sizeof(long int), 0));
        break;
      case RUNAHEAD_MSG_REQ_GUIDED_EXEC:
        if (fork_runahead_slave() == 0) { // father process wait here
          // child process continue to run
          struct ExecutionGuide guide;
          guide.force_raise_exception = false;
          guide.force_set_jump_target = true;
          guide.jump_target = request.target_pc;
          runahead_debug("force jump to %lx\n", request.target_pc);
          proxy->guided_exec(&guide);
          runahead_debug("Run ahead: proxy->guided_exec(&guide)\n");
          assert_no_error(msgsnd(runahead_resp_msgq_id, &resp, sizeof(RunaheadResponsePid) - sizeof(long int), 0));
        }
        break;
#ifdef QUERY_MEM_ACCESS
      case RUNAHEAD_MSG_REQ_QUERY:
        runahead_debug("Query runahead result, type %lx\n", request.query_type);
        proxy->query(&resp_query.result, request.query_type);
        mem_access_info->ref_need_wait = false;
        if (mem_access_info->mem_access) {
          if (mem_access_info->mem_access_is_load) {
            memdep_watcher->watch_load(mem_access_info->pc, mem_access_info->mem_access_vaddr);
            mem_access_info->ref_need_wait =
                memdep_watcher->query_load_store_dep(mem_access_info->pc, mem_access_info->mem_access_vaddr);
          } else {
            memdep_watcher->watch_store(mem_access_info->pc, mem_access_info->mem_access_vaddr);
          }
        }
        assert_no_error(
            msgsnd(runahead_resp_msgq_id, &resp_query, sizeof(RunaheadResponseQuery) - sizeof(long int), 0));
        break;
#endif
      default: runahead_debug("Runahead slave received invalid runahead req\n"); assert(0);
    }
  };
  assert(0);
}

// Create the first slave process for simulator runahead
//
// Return pid if succeed
pid_t Runahead::init_runahead_slave() {
  // run ahead simulator needs its own addr space
  // slave will be initialized after first run ahead request is sent
  pid_t pid = fork();
  if (pid < 0) {
    runahead_debug("Failed to create the first runahead slave\n");
    assert(0);
  }
  if (pid == 0) {
    runahead_slave();
  } else {
    register_checkpoint(pid, -1, FIRST_INST_ADDRESS, false);
  }
  return 0;
}

// Create run ahead slave process to establish a new checkpoint
//
// Return pid if succeed
pid_t Runahead::fork_runahead_slave() {
  // run ahead simulator needs its own addr space
  // slave will be initialized after first run ahead request is sent
  pid_t pid = fork();
  if (pid < 0) {
    runahead_debug("Failed to fork runahead slave\n");
    assert(0);
  }
  if (pid == 0) {
    return 0;
    // I am the newest checkpoint
  } else {
    // Wait until checkpoint is recovered or checkpoint is freed
    int status = -1;
    // Send new pid to master
    RunaheadResponsePid resp;
    resp.message_type = RUNAHEAD_MSG_RESP_FORK;
    resp.pid = pid;
    assert_no_error(msgsnd(runahead_resp_msgq_id, &resp, sizeof(RunaheadResponsePid) - sizeof(long int), 0));
    runahead_debug("%d wait for %d\n", getpid(), pid);
    waitpid(pid, &status, 0);
    runahead_debug("pid %d wakeup\n", getpid());
    return pid;
  }
}
