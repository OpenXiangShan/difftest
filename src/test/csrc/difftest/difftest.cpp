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

#include "difftest.h"
#include "difftrace.h"
#include "dut.h"
#include "flash.h"
#include "goldenmem.h"
#include "ram.h"
#include "spikedasm.h"
#if defined(CONFIG_DIFFTEST_SQUASH) && !defined(CONFIG_PLATFORM_FPGA)
#include "svdpi.h"
#endif // CONFIG_DIFFTEST_SQUASH && !CONFIG_PLATFORM_FPGA
#ifdef CONFIG_DIFFTEST_PERFCNT
#include "perf.h"
#endif // CONFIG_DIFFTEST_PERFCNT
#ifdef CONFIG_DIFFTEST_QUERY
#include "query.h"
#endif // CONFIG_DIFFTEST_QUERY

Difftest **difftest = NULL;

int difftest_init() {
#ifdef CONFIG_DIFFTEST_PERFCNT
  difftest_perfcnt_init();
#endif // CONFIG_DIFFTEST_PERFCNT
#ifdef CONFIG_DIFFTEST_IOTRACE
  difftest_iotrace_init();
#endif // CONFIG_DIFFTEST_IOTRACE
#ifdef CONFIG_DIFFTEST_QUERY
  difftest_query_init();
#endif // CONFIG_DIFFTEST_QUERY
  diffstate_buffer_init();
  difftest = new Difftest *[NUM_CORES];
  for (int i = 0; i < NUM_CORES; i++) {
    difftest[i] = new Difftest(i);
    difftest[i]->dut = diffstate_buffer[i]->get(0, 0);
  }
  return 0;
}

int init_nemuproxy(size_t ramsize = 0) {
  for (int i = 0; i < NUM_CORES; i++) {
    difftest[i]->update_nemuproxy(i, ramsize);
  }
  return 0;
}

int difftest_state() {
  for (int i = 0; i < NUM_CORES; i++) {
    if (difftest[i]->get_trap_valid()) {
      return difftest[i]->get_trap_code();
    }
    if (difftest[i]->proxy && difftest[i]->proxy->get_status()) {
      return difftest[i]->proxy->get_status();
    }
  }
  return -1;
}

int difftest_nstep(int step, bool enable_diff) {
#if CONFIG_DIFFTEST_ZONESIZE > 1
  difftest_switch_zone();
#endif // CONFIG_DIFFTEST_ZONESIZE
  for (int i = 0; i < step; i++) {
    if (enable_diff) {
      if (difftest_step())
        return STATE_ABORT;
    } else {
      difftest_set_dut();
    }
    int status = difftest_state();
    if (status != STATE_RUNNING)
      return status;
  }
  return STATE_RUNNING;
}

void difftest_switch_zone() {
  for (int i = 0; i < NUM_CORES; i++) {
    diffstate_buffer[i]->switch_zone();
  }
}
void difftest_set_dut() {
  for (int i = 0; i < NUM_CORES; i++) {
    difftest[i]->dut = diffstate_buffer[i]->next();
  }
}
int difftest_step() {
  difftest_set_dut();
#if defined(CONFIG_DIFFTEST_QUERY) && !defined(CONFIG_DIFFTEST_BATCH)
  difftest_query_step();
#endif // CONFIG_DIFFTEST_QUERY
  for (int i = 0; i < NUM_CORES; i++) {
    int ret = difftest[i]->step();
    if (ret) {
      return ret;
    }
  }
  return 0;
}

void difftest_trace_read() {
  for (int i = 0; i < NUM_CORES; i++) {
    difftest[i]->trace_read();
  }
}

void difftest_trace_write(int step) {
  for (int i = 0; i < NUM_CORES; i++) {
    difftest[i]->trace_write(step);
  }
}

void difftest_finish() {
#ifdef CONFIG_DIFFTEST_PERFCNT
  uint64_t cycleCnt = difftest[0]->get_trap_event()->cycleCnt;
  difftest_perfcnt_finish(cycleCnt);
#endif // CONFIG_DIFFTEST_PERFCNT
#ifdef CONFIG_DIFFTEST_IOTRACE
  difftest_iotrace_free();
#endif // CONFIG_DIFFTEST_IOTRACE
#ifdef CONFIG_DIFFTEST_QUERY
  difftest_query_finish();
#endif // CONFIG_DIFFTEST_QUERY
  diffstate_buffer_free();
  for (int i = 0; i < NUM_CORES; i++) {
    delete difftest[i];
  }
  delete[] difftest;
  difftest = NULL;
}

#if defined(CONFIG_DIFFTEST_SQUASH) && !defined(CONFIG_PLATFORM_FPGA)
svScope squashScope;
void set_squash_scope() {
  squashScope = svGetScope();
}

extern "C" void set_squash_enable(int enable);
void difftest_squash_enable(int enable) {
  if (squashScope == NULL) {
    printf("Error: Could not retrieve squash scope, set first\n");
    assert(squashScope);
  }
  svSetScope(squashScope);
  set_squash_enable(enable);
}
#endif // CONFIG_DIFFTEST_SQUASH && !CONFIG_PLATFORM_FPGA

#ifdef CONFIG_DIFFTEST_REPLAY
svScope replayScope;
void set_replay_scope() {
  replayScope = svGetScope();
}

extern "C" void set_replay_head(int head);
void difftest_replay_head(int head) {
  if (replayScope == NULL) {
    printf("Error: Could not retrieve replay scope, set first\n");
    assert(replayScope);
  }
  svSetScope(replayScope);
  set_replay_head(head);
}
#endif // CONFIG_DIFFTEST_REPLAY

Difftest::Difftest(int coreid) : id(coreid) {
  state = new DiffState();
#ifdef CONFIG_DIFFTEST_REPLAY
  state_ss = (DiffState *)malloc(sizeof(DiffState));
#endif // CONFIG_DIFFTEST_REPLAY
}

Difftest::~Difftest() {
  delete state;
  delete difftrace;
  if (proxy) {
    delete proxy;
  }
#ifdef CONFIG_DIFFTEST_REPLAY
  free(state_ss);
  if (proxy_reg_ss) {
    free(proxy_reg_ss);
  }
#endif // CONFIG_DIFFTEST_REPLAY
}

#if defined(CONFIG_DIFFTEST_LOADEVENT) && defined(CONFIG_DIFFTEST_ARCHVECREGSTATE)
bool enable_vec_load_goldenmem_check = true;
#endif // CONFIG_DIFFTEST_LOADEVENT && CONFIG_DIFFTEST_ARCHVECREGSTATE

void Difftest::update_nemuproxy(int coreid, size_t ram_size = 0) {
  proxy = new REF_PROXY(coreid, ram_size);
#if defined(CONFIG_DIFFTEST_LOADEVENT) && defined(CONFIG_DIFFTEST_ARCHVECREGSTATE)
  enable_vec_load_goldenmem_check = proxy->check_ref_vec_load_goldenmem();
#endif // CONFIG_DIFFTEST_LOADEVENT && CONFIG_DIFFTEST_ARCHVECREGSTATE
#ifdef CONFIG_DIFFTEST_REPLAY
  proxy_reg_size = proxy->get_reg_size();
  proxy_reg_ss = (uint8_t *)malloc(proxy_reg_size);
#endif // CONFIG_DIFFTEST_REPLAY
}

#ifdef CONFIG_DIFFTEST_REPLAY
bool Difftest::can_replay() {
  auto info = dut->trace_info;
  return info.valid && !info.in_replay && info.trace_size > 1;
}

bool Difftest::in_replay_range() {
  auto info = dut->trace_info;
  if (!info.valid || !info.in_replay || info.trace_size > 1)
    return false;
  int pos = info.trace_head;
  int head = replay_status.trace_head;
  int tail = (head + replay_status.trace_size - 1) % CONFIG_DIFFTEST_REPLAY_SIZE;
  if (tail < head) { // consider ring queue
    return (pos <= tail) || (pos >= head);
  } else {
    return (pos >= head) && (pos <= tail);
  }
}

void Difftest::replay_snapshot() {
  memcpy(state_ss, state, sizeof(DiffState));
  memcpy(proxy_reg_ss, &proxy->regs_int, proxy_reg_size);
  proxy->ref_csrcpy(squash_csr_buf, REF_TO_DUT);
  proxy->ref_store_log_reset();
  proxy->set_store_log(true);
  goldenmem_store_log_reset();
  goldenmem_set_store_log(true);
}

void Difftest::do_replay() {
  auto info = dut->trace_info;
  replay_status.in_replay = true;
  replay_status.trace_head = info.trace_head;
  replay_status.trace_size = info.trace_size;
  memcpy(state, state_ss, sizeof(DiffState));
  memcpy(&proxy->regs_int, proxy_reg_ss, proxy_reg_size);
  proxy->ref_regcpy(&proxy->regs_int, DUT_TO_REF, false);
  proxy->ref_csrcpy(squash_csr_buf, DUT_TO_REF);
  proxy->ref_store_log_restore();
  goldenmem_store_log_restore();
  difftest_replay_head(info.trace_head);
  // clear buffered queue
#ifdef CONFIG_DIFFTEST_STOREEVENT
  while (!store_event_queue.empty())
    store_event_queue.pop();
#endif // CONFIG_DIFFTEST_STOREEVENT
#if defined(CONFIG_DIFFTEST_LOADEVENT) && defined(CONFIG_DIFFTEST_SQUASH)
  while (!load_event_queue.empty())
    load_event_queue.pop();
#endif
}
#endif // CONFIG_DIFFTEST_REPLAY

int Difftest::step() {
#ifdef CONFIG_DIFFTEST_REPLAY
  static int replay_step = 0;
  if (replay_status.in_replay) {
    if (!in_replay_range()) {
      return 0;
    } else {
      replay_step++;
      if (replay_step > replay_status.trace_size) {
        Info("*** DUT run out of replay range, failed to get error location ***\n");
        return 1;
      }
    }
  }
  bool canReplay = can_replay();
  if (canReplay) {
    replay_snapshot();
  } else {
    proxy->set_store_log(false);
    goldenmem_set_store_log(false);
  }
  int ret = check_all();
  if (ret && canReplay) {
    Info("\n**** Start replay for more accurate error location ****\n");
    do_replay();
    return 0;
  } else {
    return ret;
  }
#else
  return check_all();
#endif // CONFIG_DIFFTEST_REPLAY
}

inline int Difftest::check_all() {
  progress = false;

  if (check_timeout()) {
    return 1;
  }
  do_first_instr_commit();

  // Each cycle is checked for an store event, and recorded in queue.
  // It is checked every time an instruction is committed and queue has content.
#ifdef CONFIG_DIFFTEST_STOREEVENT
  store_event_record();
#endif

#ifdef CONFIG_DIFFTEST_SQUASH
#ifdef CONFIG_DIFFTEST_LOADEVENT
  load_event_record();
#endif // CONFIG_DIFFTEST_LOADEVENT
#endif // CONFIG_DIFFTEST_SQUASH

#ifdef DEBUG_GOLDENMEM
  if (do_golden_memory_update()) {
    return 1;
  }
#endif

#ifdef CONFIG_DIFFTEST_CMOINVALEVENT
  cmo_inval_event_record();
#endif // CONFIG_DIFFTEST_CMOINVALEVENT

  if (!has_commit) {
    return 0;
  }

#ifdef DEBUG_REFILL
  if (do_irefill_check() || do_drefill_check() || do_ptwrefill_check()) {
    return 1;
  }
#endif

#ifdef DEBUG_L2TLB
  if (do_l2tlb_check()) {
    return 1;
  }
#endif

#ifdef DEBUG_L1TLB
  if (do_l1tlb_check()) {
    return 1;
  }
#endif

#ifdef DEBUG_MODE_DIFF
  // skip load & store insts in debug mode
  // for other insts copy inst content to ref's dummy debug module
  for (int i = 0; i < DIFFTEST_COMMIT_WIDTH; i++) {
    if (DEBUG_MEM_REGION(dut->commit[i].valid, dut->commit[i].pc))
      debug_mode_copy(dut->commit[i].pc, dut->commit[i].isRVC ? 2 : 4, dut->commit[i].inst);
  }

#endif

#ifdef CONFIG_DIFFTEST_LRSCEVENT
  // sync lr/sc reg microarchitectural status to the REF
  if (dut->lrsc.valid) {
    dut->lrsc.valid = 0;
    struct SyncState sync;
    sync.sc_fail = !dut->lrsc.success;
    proxy->uarchstatus_sync((uint64_t *)&sync);
  }
#endif

#ifdef CONFIG_DIFFTEST_NONREGINTERRUPTPENDINGEVENT
  do_non_reg_interrupt_pending();
#endif

#ifdef CONFIG_DIFFTEST_MHPMEVENTOVERFLOWEVENT
  do_mhpmevent_overflow();
#endif
#ifdef CONFIG_DIFFTEST_CRITICALERROREVENT
  do_raise_critical_error();
#endif
#ifdef CONFIG_DIFFTEST_SYNCAIAEVENT
  do_sync_aia();
#endif
#ifdef CONFIG_DIFFTEST_SYNCCUSTOMMFLUSHPWREVENT
  do_sync_custom_mflushpwr();
#endif

  num_commit = 0; // reset num_commit this cycle to 0
  if (dut->event.valid) {
    // interrupt has a higher priority than exception
    dut->event.interrupt ? do_interrupt() : do_exception();
    dut->event.valid = 0;
    dut->commit[0].valid = 0;
  } else {
#if !defined(BASIC_DIFFTEST_ONLY) && !defined(CONFIG_DIFFTEST_SQUASH)
    if (dut->commit[0].valid) {
      dut_commit_first_pc = dut->commit[0].pc;
      ref_commit_first_pc = proxy->pc;
      if (dut_commit_first_pc != ref_commit_first_pc) {
        pc_mismatch = true;
      }
    }
#endif
    for (int i = 0; i < CONFIG_DIFF_COMMIT_WIDTH; i++) {
      if (dut->commit[i].valid) {
        if (do_instr_commit(i)) {
          return 1;
        }
#ifndef CONFIG_DIFFTEST_SQUASH
        do_load_check(i);
        if (do_store_check()) {
          return 1;
        }
#endif // CONFIG_DIFFTEST_SQUASH
        dut->commit[i].valid = 0;
        num_commit += 1 + dut->commit[i].nFused;
      }
    }
  }

  if (update_delayed_writeback()) {
    return 1;
  }

  if (!progress) {
    return 0;
  }

  proxy->sync();

  if (num_commit > 0) {
    state->record_group(dut->commit[0].pc, num_commit);
  }

  if (apply_delayed_writeback()) {
    return 1;
  }

  if (proxy->compare(dut) || pc_mismatch) {
#ifdef FUZZING
    if (in_disambiguation_state()) {
      Info("Mismatch detected with a disambiguation state at pc = 0x%lx.\n", dut->trap.pc);
      return 0;
    }
#endif
    display();
    proxy->display(dut);
#ifdef FUZZER_LIB
    stats.exit_code = SimExitCode::difftest;
#endif // FUZZER_LIB
    return 1;
  }

  return 0;
}

void Difftest::do_interrupt() {
  state->record_interrupt(dut->event.exceptionPC, dut->event.exceptionInst, dut->event.interrupt);
  if (dut->event.hasNMI) {
    proxy->trigger_nmi(dut->event.hasNMI);
  } else if (dut->event.virtualInterruptIsHvictlInject) {
    proxy->virtual_interrupt_is_hvictl_inject(dut->event.virtualInterruptIsHvictlInject);
  }
  proxy->raise_intr(dut->event.interrupt | (1ULL << 63));
  progress = true;
}

void Difftest::do_exception() {
  state->record_exception(dut->event.exceptionPC, dut->event.exceptionInst, dut->event.exception);
  if (dut->event.exception == 12 || dut->event.exception == 13 || dut->event.exception == 15 ||
      dut->event.exception == 20 || dut->event.exception == 21 || dut->event.exception == 23) {
    struct ExecutionGuide guide;
    guide.force_raise_exception = true;
    guide.exception_num = dut->event.exception;
    guide.mtval = dut->csr.mtval;
    guide.stval = dut->csr.stval;
#ifdef CONFIG_DIFFTEST_HCSRSTATE
    guide.mtval2 = dut->hcsr.mtval2;
    guide.htval = dut->hcsr.htval;
    guide.vstval = dut->hcsr.vstval;
#endif // CONFIG_DIFFTEST_HCSRSTATE
    guide.force_set_jump_target = false;
    proxy->guided_exec(guide);
  } else {
#ifdef DEBUG_MODE_DIFF
    if (DEBUG_MEM_REGION(true, dut->event.exceptionPC)) {
      debug_mode_copy(dut->event.exceptionPC, 4, dut->event.exceptionInst);
    }
#endif
    proxy->ref_exec(1);
  }

#ifdef FUZZING
  static uint64_t lastExceptionPC = 0xdeadbeafUL;
  static int sameExceptionPCCount = 0;
  if (dut->event.exceptionPC == lastExceptionPC) {
    if (sameExceptionPCCount >= 5) {
      Info("Found infinite loop at exception_pc %lx. Exiting.\n", dut->event.exceptionPC);
      dut->trap.hasTrap = 1;
      dut->trap.code = STATE_FUZZ_COND;
#ifdef FUZZER_LIB
      stats.exit_code = SimExitCode::exception_loop;
#endif // FUZZER_LIB
      return;
    }
    sameExceptionPCCount++;
  }
  if (!sameExceptionPCCount && dut->event.exceptionPC != lastExceptionPC) {
    sameExceptionPCCount = 0;
  }
  lastExceptionPC = dut->event.exceptionPC;
#endif // FUZZING

  progress = true;
}

int Difftest::do_instr_commit(int i) {

  // store the writeback info to debug array
#ifdef BASIC_DIFFTEST_ONLY
  uint64_t commit_pc = proxy->pc;
#else
  uint64_t commit_pc = dut->commit[i].pc;
#endif
  uint64_t commit_instr = dut->commit[i].instr;
  state->record_inst(commit_pc, commit_instr, (dut->commit[i].rfwen | dut->commit[i].fpwen | dut->commit[i].vecwen),
                     dut->commit[i].wdest, get_commit_data(i), dut->commit[i].skip != 0, dut->commit[i].special & 0x1,
                     dut->commit[i].lqIdx, dut->commit[i].sqIdx, dut->commit[i].robIdx, dut->commit[i].isLoad,
                     dut->commit[i].isStore);

#ifdef FUZZING
  // isExit
  if (dut->commit[i].special & 0x2) {
    dut->trap.hasTrap = 1;
    dut->trap.code = STATE_SIM_EXIT;
#ifdef FUZZER_LIB
    stats.exit_code = SimExitCode::sim_exit;
#endif // FUZZER_LIB
    return 0;
  }
#endif // FUZZING

  progress = true;
  update_last_commit();

  // isDelayeWb
  if (dut->commit[i].special & 0x1) {
    int *status =
#ifdef CONFIG_DIFFTEST_ARCHINTDELAYEDUPDATE
        dut->commit[i].rfwen ? delayed_int :
#endif // CONFIG_DIFFTEST_ARCHINTDELAYEDUPDATE
#ifdef CONFIG_DIFFTEST_ARCHFPDELAYEDUPDATE
        dut->commit[i].fpwen ? delayed_fp
                             :
#endif // CONFIG_DIFFTEST_ARCHFPDELAYEDUPDATE
                             nullptr;
    if (status) {
      if (status[dut->commit[i].wdest]) {
        display();
        Info("The delayed register %s has already been delayed for %d cycles\n",
             (dut->commit[i].rfwen ? regs_name_int : regs_name_fp)[dut->commit[i].wdest], status[dut->commit[i].wdest]);
        raise_trap(STATE_ABORT);
        return 1;
      }
      status[dut->commit[i].wdest] = 1;
    }
  }

#ifdef DEBUG_MODE_DIFF
  if (spike_valid() && (IS_DEBUGCSR(commit_instr) || IS_TRIGGERCSR(commit_instr))) {
    Info("s0 is %016lx ", dut->regs.gpr[8]);
    Info("pc is %lx %s\n", commit_pc, spike_dasm(commit_instr));
  }
#endif

  // MMIO accessing should not be a branch or jump, just +2/+4 to get the next pc
  // to skip the checking of an instruction, just copy the reg state to reference design
  if (dut->commit[i].skip || (DEBUG_MODE_SKIP(dut->commit[i].valid, dut->commit[i].pc, dut->commit[i].inst))) {
    // We use the physical register file to get wdata
    proxy->skip_one(dut->commit[i].isRVC, (dut->commit[i].rfwen && dut->commit[i].wdest != 0), dut->commit[i].fpwen,
                    dut->commit[i].vecwen, dut->commit[i].wdest, get_commit_data(i));
    return 0;
  }

  // Default: single step exec
  // when there's a fused instruction, let proxy execute more instructions.
  for (int j = 0; j < dut->commit[i].nFused + 1; j++) {
    proxy->ref_exec(1);
#ifdef CONFIG_DIFFTEST_SQUASH
    commit_stamp = (commit_stamp + 1) % CONFIG_DIFFTEST_SQUASH_STAMPSIZE;
    do_load_check(i);
    if (do_store_check()) {
      return 1;
    }
#endif // CONFIG_DIFFTEST_SQUASH
  }

  return 0;
}

void Difftest::do_first_instr_commit() {
  if (!has_commit && dut->commit[0].valid) {
#ifndef BASIC_DIFFTEST_ONLY
    if (dut->commit[0].pc != FIRST_INST_ADDRESS) {
      return;
    }
#endif
    Info("The first instruction of core %d has commited. Difftest enabled. \n", id);
    has_commit = 1;
    nemu_this_pc = FIRST_INST_ADDRESS;

    proxy->flash_init((const uint8_t *)flash_dev.base, flash_dev.img_size, flash_dev.img_path);
    simMemory->clone_on_demand(
        [this](uint64_t offset, void *src, size_t n) {
          uint64_t dest_addr = PMEM_BASE + offset;
          proxy->mem_init(dest_addr, src, n, DUT_TO_REF);
        },
        true);
    // Use a temp variable to store the current pc of dut
    uint64_t dut_this_pc = dut->commit[0].pc;
    // NEMU should always start at FIRST_INST_ADDRESS
    dut->commit[0].pc = FIRST_INST_ADDRESS;
    proxy->regcpy(dut);
    dut->commit[0].pc = dut_this_pc;
    // Do not reconfig simulator 'proxy->update_config(&nemu_config)' here:
    // If this is main sim thread, simulator has its own initial config
    // If this process is checkpoint wakeuped, simulator's config has already been updated,
    // do not override it.
  }
}

#if defined(CONFIG_DIFFTEST_LOADEVENT) && defined(CONFIG_DIFFTEST_ARCHVECREGSTATE)
void Difftest::do_vec_load_check(int index, DifftestLoadEvent load_event) {
  if (!enable_vec_load_goldenmem_check) {
    return;
  }

  // ===============================================================
  //                      Comparison data
  // ===============================================================
  uint32_t vdNum = proxy->get_ref_vdNum();

  proxy->sync();

#ifdef CONFIG_DIFFTEST_SQUASH
  auto vecFirstLdest = load_event.wdest;
#else
  auto vecFirstLdest = dut->commit[index].wdest;
#endif // CONFIG_DIFFTEST_SQUASH

  bool reg_mismatch = false;

  for (int vdidx = 0; vdidx < vdNum; vdidx++) {
#ifndef CONFIG_DIFFTEST_COMMITDATA
    bool v0Wen = dut->commit[index].v0wen && vdidx == 0;
    auto vecNextPdest = dut->commit[index].otherwpdest[vdidx];
    uint64_t *dutRegPtr = v0Wen ? dut->wb_v0[vecNextPdest].data : dut->wb_vec[vecNextPdest].data;
#endif // !CONFIG_DIFFTEST_COMMITDATA

    auto vecNextLdest = vecFirstLdest + vdidx;

    for (int i = 0; i < VLENE_64; i++) {
#ifdef CONFIG_DIFFTEST_COMMITDATA
#ifdef CONFIG_DIFFTEST_SQUASH
      uint64_t dutRegData = load_event.vecCommitData[VLENE_64 * vdidx + i];
#else
      uint64_t dutRegData = dut->commit_data[index].vecData[VLENE_64 * vdidx + i];
#endif // CONFIG_DIFFTEST_SQUASH
#else
      uint64_t dutRegData = dutRegPtr[i];
#endif // CONFIG_DIFFTEST_COMMITDATA

      uint64_t *refRegPtr = proxy->arch_vecreg(VLENE_64 * vecNextLdest + i);
      reg_mismatch |= dutRegData != *refRegPtr;
    }
  }

  // ===============================================================
  //                      Regs Mismatch handle
  // ===============================================================
  bool goldenmem_mismatch = false;

  if (reg_mismatch) {
    // ===============================================================
    //                      Check golden memory
    // ===============================================================
    uint64_t *vec_goldenmem_regPtr = (uint64_t *)proxy->get_vec_goldenmem_reg();

    if (vec_goldenmem_regPtr == nullptr) {
      Info("Vector Load comparison failed and no consistency check with golden mem was performed.\n");
      return;
    }

    for (int vdidx = 0; vdidx < vdNum; vdidx++) {
#ifndef CONFIG_DIFFTEST_COMMITDATA
      bool v0Wen = dut->commit[index].v0wen && vdidx == 0;
      auto vecNextPdest = dut->commit[index].otherwpdest[vdidx];
      uint64_t *dutRegPtr = v0Wen ? dut->wb_v0[vecNextPdest].data : dut->wb_vec[vecNextPdest].data;
#endif // !CONFIG_DIFFTEST_COMMITDATA

      for (int i = 0; i < VLENE_64; i++) {
#ifdef CONFIG_DIFFTEST_COMMITDATA
#ifdef CONFIG_DIFFTEST_SQUASH
        uint64_t dutRegData = load_event.vecCommitData[VLENE_64 * vdidx + i];
#else
        uint64_t dutRegData = dut->commit_data[index].vecData[VLENE_64 * vdidx + i];
#endif // CONFIG_DIFFTEST_SQUASH
#else
        uint64_t dutRegData = dutRegPtr[i];
#endif // CONFIG_DIFFTEST_COMMITDATA

        goldenmem_mismatch |= dutRegData != vec_goldenmem_regPtr[VLENE_64 * vdidx + i];
      }
    }

    if (!goldenmem_mismatch) {
      // ===============================================================
      //                      sync memory and regs
      // ===============================================================
      proxy->vec_update_goldenmem();

      for (int vdidx = 0; vdidx < vdNum; vdidx++) {
#ifndef CONFIG_DIFFTEST_COMMITDATA
        bool v0Wen = dut->commit[index].v0wen && vdidx == 0;
        auto vecNextPdest = dut->commit[index].otherwpdest[vdidx];
        uint64_t *dutRegPtr = v0Wen ? dut->wb_v0[vecNextPdest].data : dut->wb_vec[vecNextPdest].data;
#endif // !CONFIG_DIFFTEST_COMMITDATA

        auto vecNextLdest = vecFirstLdest + vdidx;

        for (int i = 0; i < VLENE_64; i++) {
#ifdef CONFIG_DIFFTEST_COMMITDATA
#ifdef CONFIG_DIFFTEST_SQUASH
          uint64_t dutRegData = load_event.vecCommitData[VLENE_64 * vdidx + i];
#else
          uint64_t dutRegData = dut->commit_data[index].vecData[VLENE_64 * vdidx + i];
#endif // CONFIG_DIFFTEST_SQUASH
#else
          uint64_t dutRegData = dutRegPtr[i];
#endif // CONFIG_DIFFTEST_COMMITDATA

          uint64_t *refRegPtr = proxy->arch_vecreg(VLENE_64 * vecNextLdest + i);
          *refRegPtr = dutRegData;
        }
      }

      proxy->sync(true);
    } else {
      Info("Vector Load register and golden memory mismatch\n");
    }
  }
}
#endif // CONFIG_DIFFTEST_LOADEVENT && CONFIG_DIFFTEST_ARCHVECREGSTATE

void Difftest::do_load_check(int i) {
  // Handle load instruction carefully for SMP
#ifdef CONFIG_DIFFTEST_LOADEVENT
  if (NUM_CORES > 1) {
#ifdef CONFIG_DIFFTEST_SQUASH
    if (load_event_queue.empty())
      return;
    auto load_event = load_event_queue.front();
    if (load_event.stamp != commit_stamp)
      return;
    bool regWen = load_event.regWen;
    auto refRegPtr = proxy->arch_reg(load_event.wdest, load_event.fpwen);
    auto commitData = load_event.commitData;
#else
    auto load_event = dut->load[i];
    if (!load_event.valid)
      return;
    bool regWen =
        ((dut->commit[i].rfwen && dut->commit[i].wdest != 0) || dut->commit[i].fpwen) && !dut->commit[i].vecwen;
    auto refRegPtr = proxy->arch_reg(dut->commit[i].wdest, dut->commit[i].fpwen);
    auto commitData = get_commit_data(i);
#endif // CONFIG_DIFFTEST_SQUASH

#if defined(CONFIG_DIFFTEST_LOADEVENT) && defined(CONFIG_DIFFTEST_ARCHVECREGSTATE)
    if (load_event.isVLoad) {
      do_vec_load_check(i, load_event);
#ifdef CONFIG_DIFFTEST_SQUASH
      load_event_queue.pop();
#else
      dut->load[i].valid = 0;
#endif // CONFIG_DIFFTEST_SQUASH
      return;
    }
#endif // CONFIG_DIFFTEST_LOADEVENT && CONFIG_DIFFTEST_ARCHVECREGSTATE

    if (load_event.isLoad || load_event.isAtomic) {
      proxy->sync();
      if (regWen && *refRegPtr != commitData) {
        uint64_t golden;
        int len = 0;
        if (load_event.isLoad) {
          switch (load_event.opType) {
            case 0:  // lb
            case 4:  // lbu
            case 16: // hlvb
            case 20: // hlvbu
              len = 1;
              break;

            case 1:  // lh
            case 5:  // lhu
            case 17: // hlvh
            case 21: // hlvhu
            case 29: // hlvxhu
              len = 2;
              break;

            case 2:  // lw
            case 6:  // lwu
            case 18: // hlvw
            case 22: // hlvwu
            case 30: // hlvxwu
              len = 4;
              break;

            case 3:  // ld
            case 19: // hlvd
              len = 8;
              break;

            default: Info("Unknown fuOpType: 0x%x\n", load_event.opType);
          }
        } else if (load_event.isAtomic) {
          if (load_event.opType % 2 == 0) {
            len = 4;
          } else { // load_event.opType % 2 == 1
            len = 8;
          }
        }
        read_goldenmem(load_event.paddr, &golden, len);
        if (load_event.isLoad) {
          switch (len) {
            case 1: golden = (int64_t)(int8_t)golden; break;
            case 2: golden = (int64_t)(int16_t)golden; break;
            case 4: golden = (int64_t)(int32_t)golden; break;
          }
        }
        if (golden == commitData || load_event.isAtomic) { //  atomic instr carefully handled
          proxy->ref_memcpy(load_event.paddr, &golden, len, DUT_TO_REF);
          if (regWen) {
            *refRegPtr = commitData;
            proxy->sync(true);
          }
        } else {
#ifdef DEBUG_SMP
          // goldenmem check failed as well, raise error
          Info("---  SMP difftest mismatch!\n");
          Info("---  Trying to probe local data of another core\n");
          uint64_t buf;
          difftest[(NUM_CORES - 1) - this->id]->proxy->memcpy(load_event.paddr, &buf, len, DIFFTEST_TO_DUT);
          Info("---    content: %lx\n", buf);
#else
          proxy->ref_memcpy(load_event.paddr, &golden, len, DUT_TO_REF);
          if (regWen) {
            *refRegPtr = commitData;
            proxy->sync(true);
          }
#endif
        }
      }
    }
#ifdef CONFIG_DIFFTEST_SQUASH
    load_event_queue.pop();
#else
    dut->load[i].valid = 0;
#endif // CONFIG_DIFFTEST_SQUASH
  }
#endif // CONFIG_DIFFTEST_LOADEVENT
}

int Difftest::do_store_check() {
#ifdef CONFIG_DIFFTEST_STOREEVENT
  while (!store_event_queue.empty()) {
    auto store_event = store_event_queue.front();
#ifdef CONFIG_DIFFTEST_SQUASH
    if (store_event.stamp != commit_stamp)
      return 0;
#endif // CONFIG_DIFFTEST_SQUASH
    auto addr = store_event.addr;
    auto data = store_event.data;
    auto mask = store_event.mask;

    if (proxy->store_commit(&addr, &data, &mask)) {
#ifdef FUZZING
      if (in_disambiguation_state()) {
        Info("Store mismatch detected with a disambiguation state at pc = 0x%lx.\n", dut->trap.pc);
        return 0;
      }
#endif
      uint64_t pc = store_event.pc;
      display();

      Info("\n==============  Store Commit Event (Core %d)  ==============\n", this->id);
      proxy->get_store_event_other_info(&pc);
      Info("Mismatch for store commits \n");
      Info("  REF commits addr 0x%016lx, data 0x%016lx, mask 0x%04x, pc 0x%016lx\n", addr, data, mask, pc);
      Info("  DUT commits addr 0x%016lx, data 0x%016lx, mask 0x%04x, pc 0x%016lx, robidx 0x%x\n", store_event.addr,
           store_event.data, store_event.mask, store_event.pc, store_event.robidx);

      store_event_queue.pop();
      return 1;
    }

    store_event_queue.pop();
  }
#endif // CONFIG_DIFFTEST_STOREEVENT
  return 0;
}

// cacheid: 0 -> icache
//          1 -> dcache
//          2 -> pagecache
//          3 -> icache PIQ refill ipf
//          4 -> icache mainPipe port0 toIFU
//          5 -> icache mainPipe port1 toIFU
//          6 -> icache ipf refill cache
//          7 -> icache mainPipe port0 read PIQ
//          8 -> icache mainPipe port1 read PIQ
int Difftest::do_refill_check(int cacheid) {
#ifdef CONFIG_DIFFTEST_REFILLEVENT
  auto dut_refill = &(dut->refill[cacheid]);
  if (!dut_refill->valid) {
    return 0;
  }
  dut_refill->valid = 0;
  static int delay = 0;
  delay = delay * 2;
  if (delay > 16) {
    return 1;
  }
  static uint64_t last_valid_addr = 0;
  char buf[512];
  uint64_t realpaddr = dut_refill->addr;
  dut_refill->addr = dut_refill->addr - dut_refill->addr % 64;
  if (dut_refill->addr != last_valid_addr) {
    last_valid_addr = dut_refill->addr;
    if (!in_pmem(dut_refill->addr)) {
      // speculated illegal mem access should be ignored
      return 0;
    }
    for (int i = 0; i < 8; i++) {
      read_goldenmem(dut_refill->addr + i * 8, &buf, 8);
      if (dut_refill->data[i] != *((uint64_t *)buf)) {
#ifdef CONFIG_DIFFTEST_CMOINVALEVENT
        if (cmo_inval_event_set.find(dut_refill->addr) != cmo_inval_event_set.end()) {
          // If the data inconsistency occurs in the cache block operated by CBO.INVAL,
          // it is considered reasonable and the DUT data is used to update goldenMem.
          Info("INFO: Sync GoldenMem using refill Data from DUT (Because of CBO.INVAL):\n");
          Info("      cacheid=%d, addr: %lx\n      Gold: ", cacheid, dut_refill->addr);
          for (int j = 0; j < 8; j++) {
            read_goldenmem(dut_refill->addr + j * 8, &buf, 8);
            Info("%016lx", *((uint64_t *)buf));
          }
          Info("\n      Core: ");
          for (int j = 0; j < 8; j++) {
            Info("%016lx", dut_refill->data[j]);
          }
          Info("\n");
          update_goldenmem(dut_refill->addr, dut_refill->data, 0xffffffffffffffffUL, 64);
          proxy->ref_memcpy(dut_refill->addr, dut_refill->data, 64, DUT_TO_REF);
          cmo_inval_event_set.erase(dut_refill->addr);
          return 0;
        } else {
#endif // CONFIG_DIFFTEST_CMOINVALEVENT
          Info("cacheid=%d,idtfr=%d,realpaddr=0x%lx: Refill test failed!\n", cacheid, dut_refill->idtfr, realpaddr);
          Info("addr: %lx\nGold: ", dut_refill->addr);
          for (int j = 0; j < 8; j++) {
            read_goldenmem(dut_refill->addr + j * 8, &buf, 8);
            Info("%016lx", *((uint64_t *)buf));
          }
          Info("\nCore: ");
          for (int j = 0; j < 8; j++) {
            Info("%016lx", dut_refill->data[j]);
          }
          Info("\n");
          // continue run some cycle before aborted to dump wave
          if (delay == 0) {
            delay = 1;
          }
          return 0;
#ifdef CONFIG_DIFFTEST_CMOINVALEVENT
        }
#endif // CONFIG_DIFFTEST_CMOINVALEVENT
      }
    }
  }
#endif // CONFIG_DIFFTEST_REFILLEVENT
  return 0;
}

int Difftest::do_irefill_check() {
  int r = 0;
  r |= do_refill_check(ICACHEID);
  // r |= do_refill_check(3);
  // r |= do_refill_check(4);
  // r |= do_refill_check(5);
  // r |= do_refill_check(6);
  // r |= do_refill_check(7);
  // r |= do_refill_check(8);
  return r;
}

int Difftest::do_drefill_check() {
  return do_refill_check(DCACHEID);
}

int Difftest::do_ptwrefill_check() {
  return do_refill_check(PAGECACHEID);
}

typedef struct {
  PTE pte;
  uint8_t level;
} r_s2xlate;

r_s2xlate do_s2xlate(Hgatp *hgatp, uint64_t gpaddr) {
  PTE pte;
  uint64_t hpaddr;
  uint8_t level;
  uint64_t pg_base = hgatp->ppn << 12;
  r_s2xlate r_s2;
  if (hgatp->mode == 0) {
    r_s2.pte.ppn = gpaddr >> 12;
    r_s2.level = 0;
    return r_s2;
  }
  int max_level = hgatp->mode == 8 ? 2 : 3;
  for (level = max_level; level >= 0; level--) {
    hpaddr = pg_base + GVPNi(gpaddr, level, max_level) * sizeof(uint64_t);
    read_goldenmem(hpaddr, &pte.val, 8);
    pg_base = pte.ppn << 12;
    if (!pte.v || pte.r || pte.x || pte.w || level == 0) {
      break;
    }
  }
  r_s2.pte = pte;
  r_s2.level = level;
  return r_s2;
}

int Difftest::do_l1tlb_check() {
#ifdef CONFIG_DIFFTEST_L1TLBEVENT
  for (int i = 0; i < CONFIG_DIFF_L1TLB_WIDTH; i++) {
    if (!dut->l1tlb[i].valid) {
      continue;
    }
    dut->l1tlb[i].valid = 0;
    PTE pte;
    uint64_t paddr;
    uint8_t difftest_level;
    r_s2xlate r_s2;
    bool isNapot = false;

    Satp *satp = (Satp *)&dut->l1tlb[i].satp;
    Satp *vsatp = (Satp *)&dut->l1tlb[i].vsatp;
    Hgatp *hgatp = (Hgatp *)&dut->l1tlb[i].hgatp;
    uint8_t hasS2xlate = dut->l1tlb[i].s2xlate != noS2xlate;
    uint8_t onlyS2 = dut->l1tlb[i].s2xlate == onlyStage2;
    uint8_t hasAllStage = dut->l1tlb[i].s2xlate == allStage;
    uint64_t pg_base = (hasS2xlate ? vsatp->ppn : satp->ppn) << 12;
    int mode = hasS2xlate ? vsatp->mode : satp->mode;
    int max_level = mode == 8 ? 2 : 3;
    if (onlyS2) {
      r_s2 = do_s2xlate(hgatp, dut->l1tlb[i].vpn << 12);
      pte = r_s2.pte;
      difftest_level = r_s2.level;
    } else {
      for (difftest_level = max_level; difftest_level >= 0; difftest_level--) {
        paddr = pg_base + VPNi(dut->l1tlb[i].vpn, difftest_level) * sizeof(uint64_t);
        if (hasAllStage) {
          r_s2 = do_s2xlate(hgatp, paddr);
          uint64_t pg_mask = ((1ull << VPNiSHFT(r_s2.level)) - 1);
          if (r_s2.level == 0 && r_s2.pte.n) {
            pg_mask = ((1ull << NAPOTSHFT) - 1);
          }
          pg_base = (r_s2.pte.ppn << 12 & ~pg_mask) | (paddr & pg_mask & ~PAGE_MASK);
          paddr = pg_base | (paddr & PAGE_MASK);
        }
        read_goldenmem(paddr, &pte.val, 8);
        pg_base = pte.ppn << 12;
        if (!pte.v || pte.r || pte.x || pte.w || difftest_level == 0) {
          break;
        }
      }
      if (difftest_level > 0 && pte.v) {
        uint64_t pg_mask = ((1ull << VPNiSHFT(difftest_level)) - 1);
        pg_base = (pte.ppn << 12 & ~pg_mask) | (dut->l1tlb[i].vpn << 12 & pg_mask & ~PAGE_MASK);
      } else if (difftest_level == 0 && pte.n) {
        isNapot = true;
        uint64_t pg_mask = ((1ull << NAPOTSHFT) - 1);
        pg_base = (pte.ppn << 12 & ~pg_mask) | (dut->l1tlb[i].vpn << 12 & pg_mask & ~PAGE_MASK);
      }
      if (hasAllStage && pte.v) {
        r_s2 = do_s2xlate(hgatp, pg_base);
        pte = r_s2.pte;
        difftest_level = r_s2.level;
        if (difftest_level == 0 && pte.n) {
          isNapot = true;
        }
      }
    }
    if (isNapot) {
      dut->l1tlb[i].ppn = dut->l1tlb[i].ppn >> 4 << 4;
      pte.difftest_ppn = pte.difftest_ppn >> 4 << 4;
    } else {
      dut->l1tlb[i].ppn = dut->l1tlb[i].ppn >> difftest_level * 9 << difftest_level * 9;
    }
    if (pte.difftest_ppn != dut->l1tlb[i].ppn) {
      Info("Warning: l1tlb resp test of core %d index %d failed! vpn = %lx\n", id, i, dut->l1tlb[i].vpn);
      Info("  REF commits pte.val: 0x%lx, dut s2xlate: %d\n", pte.val, dut->l1tlb[i].s2xlate);
      Info("  REF commits ppn 0x%lx, DUT commits ppn 0x%lx\n", pte.difftest_ppn, dut->l1tlb[i].ppn);
      Info("  REF commits perm 0x%02x, level %d, pf %d\n", pte.difftest_perm, difftest_level, !pte.difftest_v);
      return 0;
    }
  }
#endif // CONFIG_DIFFTEST_L1TLBEVENT
  return 0;
}

int Difftest::do_l2tlb_check() {
#ifdef CONFIG_DIFFTEST_L2TLBEVENT
  for (int i = 0; i < CONFIG_DIFF_L2TLB_WIDTH; i++) {
    if (!dut->l2tlb[i].valid) {
      continue;
    }
    dut->l2tlb[i].valid = 0;
    Satp *satp = (Satp *)&dut->l2tlb[i].satp;
    Satp *vsatp = (Satp *)&dut->l2tlb[i].vsatp;
    Hgatp *hgatp = (Hgatp *)&dut->l2tlb[i].hgatp;
    PTE pte;
    r_s2xlate r_s2;
    r_s2xlate check_s2;
    uint64_t paddr;
    uint8_t difftest_level;
    for (int j = 0; j < 8; j++) {
      if (dut->l2tlb[i].valididx[j]) {
        uint8_t hasS2xlate = dut->l2tlb[i].s2xlate != noS2xlate;
        uint8_t onlyS2 = dut->l2tlb[i].s2xlate == onlyStage2;
        uint64_t pg_base = (hasS2xlate ? vsatp->ppn : satp->ppn) << 12;
        int mode = hasS2xlate ? vsatp->mode : satp->mode;
        int max_level = mode == 8 ? 2 : 3;
        if (onlyS2) {
          r_s2 = do_s2xlate(hgatp, dut->l2tlb[i].vpn << 12);
          uint64_t pg_mask = ((1ull << VPNiSHFT(r_s2.level)) - 1);
          uint64_t s2_pg_base = r_s2.pte.ppn << 12;
          pg_base = (s2_pg_base & ~pg_mask) | (paddr & pg_mask & ~PAGE_MASK);
          paddr = pg_base | (paddr & PAGE_MASK);
        }
        for (difftest_level = max_level; difftest_level >= 0; difftest_level--) {
          paddr = pg_base + VPNi(dut->l2tlb[i].vpn + j, difftest_level) * sizeof(uint64_t);
          if (hasS2xlate) {
            r_s2 = do_s2xlate(hgatp, paddr);
            uint64_t pg_mask = ((1ull << VPNiSHFT(r_s2.level)) - 1);
            pg_base = (r_s2.pte.ppn << 12 & ~pg_mask) | (paddr & pg_mask & ~PAGE_MASK);
            paddr = pg_base | (paddr & PAGE_MASK);
          }
          read_goldenmem(paddr, &pte.val, 8);
          if (!pte.v || pte.r || pte.x || pte.w || difftest_level == 0) {
            break;
          }
          pg_base = pte.ppn << 12;
        }

        if (hasS2xlate) {
          r_s2 = do_s2xlate(hgatp, pg_base);
          if (dut->l2tlb[i].pteidx[j])
            check_s2 = r_s2;
        }
        bool difftest_gpf = !r_s2.pte.v || (!r_s2.pte.r && r_s2.pte.w);
        bool difftest_pf = !pte.v || (!pte.r && pte.w);
        bool s1_check_fail = pte.difftest_ppn != dut->l2tlb[i].ppn[j] || pte.difftest_perm != dut->l2tlb[i].perm ||
                             pte.difftest_pbmt != dut->l2tlb[i].pbmt || difftest_level != dut->l2tlb[i].level ||
                             difftest_pf != dut->l2tlb[i].pf;
        bool s2_check_fail = hasS2xlate ? r_s2.pte.difftest_ppn != dut->l2tlb[i].s2ppn ||
                                              r_s2.pte.difftest_perm != dut->l2tlb[i].g_perm ||
                                              r_s2.pte.difftest_pbmt != dut->l2tlb[i].g_pbmt ||
                                              r_s2.level != dut->l2tlb[i].g_level || difftest_gpf != dut->l2tlb[i].gpf
                                        : false;
        if (s1_check_fail || s2_check_fail) {
          Info("Warning: L2TLB resp test of core %d index %d sector %d failed! vpn = %lx\n", id, i, j,
               dut->l2tlb[i].vpn + j);
          Info("  REF commits ppn 0x%lx, perm 0x%02x, level %d, pf %d\n", pte.difftest_ppn, pte.difftest_perm,
               difftest_level, difftest_pf);
          if (hasS2xlate)
            Info("      s2_ppn 0x%lx, g_perm 0x%02x, g_level %d, gpf %d\n", r_s2.pte.difftest_ppn,
                 r_s2.pte.difftest_perm, r_s2.level, difftest_gpf);
          Info("  DUT commits ppn 0x%lx, perm 0x%02x, level %d, pf %d\n", dut->l2tlb[i].ppn[j], dut->l2tlb[i].perm,
               dut->l2tlb[i].level, dut->l2tlb[i].pf);
          if (hasS2xlate)
            Info("      s2_ppn 0x%lx, g_perm 0x%02x, g_level %d, gpf %d\n", dut->l2tlb[i].s2ppn, dut->l2tlb[i].g_perm,
                 dut->l2tlb[i].g_level, dut->l2tlb[i].gpf);
          return 1;
        }
      }
    }
  }
#endif // CONFIG_DIFFTEST_L1TLBEVENT
  return 0;
}

inline int handle_atomic(int coreid, uint64_t atomicAddr, uint64_t atomicData[], uint64_t atomicMask,
                         uint64_t atomicCmp[], uint8_t atomicFuop, uint64_t atomicOut[]) {
  // We need to do atmoic operations here so as to update goldenMem
  if (!(atomicMask == 0xf || atomicMask == 0xf0 || atomicMask == 0xff || atomicMask == 0xffff)) {
    Info("Unrecognized mask: %lx\n", atomicMask);
    return 1;
  }

  if (atomicMask == 0xff) {
    uint64_t rs = atomicData[0]; // rs2
    uint64_t t = atomicOut[0];   // original value
    uint64_t cmp = atomicCmp[0]; // rd, data to be compared
    uint64_t ret;
    uint64_t mem;
    read_goldenmem(atomicAddr, &mem, 8);
    if (mem != t && atomicFuop != 007 && atomicFuop != 003) { // ignore sc_d & lr_d
      Info("Core %d atomic instr mismatch goldenMem, mem: 0x%lx, t: 0x%lx, op: 0x%x, addr: 0x%lx\n", coreid, mem, t,
           atomicFuop, atomicAddr);
      return 1;
    }
    switch (atomicFuop) {
      case 002:
      case 003: ret = t; break;
      // if sc fails(aka atomicOut == 1), no update to goldenmem
      case 006:
      case 007:
        if (t == 1)
          return 0;
        ret = rs;
        break;
      case 012:
      case 013: ret = rs; break;
      case 016:
      case 017: ret = t + rs; break;
      case 022:
      case 023: ret = (t ^ rs); break;
      case 026:
      case 027: ret = t & rs; break;
      case 032:
      case 033: ret = t | rs; break;
      case 036:
      case 037: ret = ((int64_t)t < (int64_t)rs) ? t : rs; break;
      case 042:
      case 043: ret = ((int64_t)t > (int64_t)rs) ? t : rs; break;
      case 046:
      case 047: ret = (t < rs) ? t : rs; break;
      case 052:
      case 053: ret = (t > rs) ? t : rs; break;
      case 054:
      case 056:
      case 057: ret = (t == cmp) ? rs : t; break;
      default: Info("Unknown atomic fuOpType: 0x%x\n", atomicFuop);
    }
    update_goldenmem(atomicAddr, &ret, atomicMask, 8);
  }

  if (atomicMask == 0xf || atomicMask == 0xf0) {
    uint32_t rs = (uint32_t)atomicData[0]; // rs2
    uint32_t t = (uint32_t)atomicOut[0];   // original value
    uint32_t cmp = (uint32_t)atomicCmp[0]; // rd, data to be compared
    uint32_t ret;
    uint32_t mem;
    uint64_t mem_raw;
    uint64_t ret_sel;
    atomicAddr = (atomicAddr & 0xfffffffffffffff8);
    read_goldenmem(atomicAddr, &mem_raw, 8);

    if (atomicMask == 0xf)
      mem = (uint32_t)mem_raw;
    else
      mem = (uint32_t)(mem_raw >> 32);

    if (mem != t && atomicFuop != 006 && atomicFuop != 002) { // ignore sc_w & lr_w
      Info("Core %d atomic instr mismatch goldenMem, rawmem: 0x%lx mem: 0x%x, t: 0x%x, op: 0x%x, addr: 0x%lx\n", coreid,
           mem_raw, mem, t, atomicFuop, atomicAddr);
      return 1;
    }
    switch (atomicFuop) {
      case 002:
      case 003: ret = t; break;
      // if sc fails(aka atomicOut == 1), no update to goldenmem
      case 006:
      case 007:
        if (t == 1)
          return 0;
        ret = rs;
        break;
      case 012:
      case 013: ret = rs; break;
      case 016:
      case 017: ret = t + rs; break;
      case 022:
      case 023: ret = (t ^ rs); break;
      case 026:
      case 027: ret = t & rs; break;
      case 032:
      case 033: ret = t | rs; break;
      case 036:
      case 037: ret = ((int32_t)t < (int32_t)rs) ? t : rs; break;
      case 042:
      case 043: ret = ((int32_t)t > (int32_t)rs) ? t : rs; break;
      case 046:
      case 047: ret = (t < rs) ? t : rs; break;
      case 052:
      case 053: ret = (t > rs) ? t : rs; break;
      case 054:
      case 056:
      case 057: ret = (t == cmp) ? rs : t; break;
      default: Info("Unknown atomic fuOpType: 0x%x\n", atomicFuop);
    }
    ret_sel = ret;
    if (atomicMask == 0xf0)
      ret_sel = (ret_sel << 32);
    update_goldenmem(atomicAddr, &ret_sel, atomicMask, 8);
  }

  if (atomicMask == 0xffff) {
    uint64_t meml, memh;
    uint64_t retl, reth;
    read_goldenmem(atomicAddr, &meml, 8);
    read_goldenmem(atomicAddr + 8, &memh, 8);
    if (meml != atomicOut[0] || memh != atomicOut[1]) {
      Info("Core %d atomic instr mismatch goldenMem, mem: 0x%lx 0x%lx, t: 0x%lx 0x%lx, op: 0x%x, addr: 0x%lx\n", coreid,
           memh, meml, atomicOut[1], atomicOut[0], atomicFuop, atomicAddr);
      return 1;
    }
    switch (atomicFuop) {
      case 054: {
        bool success = atomicOut[0] == atomicCmp[0] && atomicOut[1] == atomicCmp[1];
        retl = success ? atomicData[0] : atomicOut[0];
        reth = success ? atomicData[1] : atomicOut[1];
        break;
      }
      default: Info("Unknown atomic fuOpType: 0x%x\n", atomicFuop);
    }
    update_goldenmem(atomicAddr, &retl, 0xff, 8);
    update_goldenmem(atomicAddr + 8, &reth, 0xff, 8);
  }
  return 0;
}

void dumpGoldenMem(const char *banner, uint64_t addr, uint64_t time) {
#ifdef DEBUG_REFILL
  char buf[512];
  if (addr == 0) {
    return;
  }
  Info("============== %s =============== time = %ld\ndata: ", banner, time);
  for (int i = 0; i < 8; i++) {
    read_goldenmem(addr + i * 8, &buf, 8);
    Info("%016lx", *((uint64_t *)buf));
  }
  Info("\n");
#endif
}

#ifdef DEBUG_GOLDENMEM
int Difftest::do_golden_memory_update() {
  // Update Golden Memory info
  uint64_t cycleCnt = get_trap_event()->cycleCnt;
  static bool initDump = true;
  if (cycleCnt >= 100 && initDump) {
    initDump = false;
    dumpGoldenMem("Init", track_instr, cycleCnt);
  }

#ifdef CONFIG_DIFFTEST_SBUFFEREVENT
  for (int i = 0; i < CONFIG_DIFF_SBUFFER_WIDTH; i++) {
    if (dut->sbuffer[i].valid) {
      dut->sbuffer[i].valid = 0;
      update_goldenmem(dut->sbuffer[i].addr, dut->sbuffer[i].data, dut->sbuffer[i].mask, 64);
      if (dut->sbuffer[i].addr == track_instr) {
        dumpGoldenMem("Store", track_instr, cycleCnt);
      }
    }
  }
#endif // CONFIG_DIFFTEST_SBUFFEREVENT

#ifdef CONFIG_DIFFTEST_ATOMICEVENT
  if (dut->atomic.valid) {
    dut->atomic.valid = 0;
    int ret = handle_atomic(id, dut->atomic.addr, (uint64_t *)dut->atomic.data, dut->atomic.mask,
                            (uint64_t *)dut->atomic.cmp, dut->atomic.fuop, (uint64_t *)dut->atomic.out);
    if (dut->atomic.addr == track_instr) {
      dumpGoldenMem("Atmoic", track_instr, cycleCnt);
    }
    if (ret)
      return ret;
  }
#endif // CONFIG_DIFFTEST_ATOMICEVENT

  return 0;
}
#endif

#ifdef CONFIG_DIFFTEST_STOREEVENT
void Difftest::store_event_record() {
  for (int i = 0; i < CONFIG_DIFF_STORE_WIDTH; i++) {
    if (dut->store[i].valid) {
      store_event_queue.push(dut->store[i]);
      dut->store[i].valid = 0;
    }
  }
}
#endif

#ifdef CONFIG_DIFFTEST_SQUASH
#ifdef CONFIG_DIFFTEST_LOADEVENT
void Difftest::load_event_record() {
  for (int i = 0; i < CONFIG_DIFF_LOAD_WIDTH; i++) {
    if (dut->load[i].valid) {
      load_event_queue.push(dut->load[i]);
      dut->load[i].valid = 0;
    }
  }
}
#endif // CONFIG_DIFFTEST_LOADEVENT
#endif // CONFIG_DIFFTEST_SQUASH

#ifdef CONFIG_DIFFTEST_CMOINVALEVENT
void Difftest::cmo_inval_event_record() {
  if (dut->cmo_inval.valid) {
    cmo_inval_event_set.insert(dut->cmo_inval.addr);
    dut->cmo_inval.valid = 0;
  }
}
#endif // CONFIG_DIFFTEST_CMOINVALEVENT

int Difftest::check_timeout() {
  uint64_t cycleCnt = get_trap_event()->cycleCnt;
  // check whether there're any commits since the simulation starts
  if (!has_commit && cycleCnt > last_commit + first_commit_limit) {
    Info("The first instruction of core %d at 0x%lx does not commit after %lu cycles.\n", id, FIRST_INST_ADDRESS,
         first_commit_limit);
    display();
    return 1;
  }

  // NOTE: the WFI instruction may cause the CPU to halt for more than `stuck_limit` cycles.
  // We update the `last_commit` if the CPU has a WFI instruction
  // to allow the CPU to run at most `stuck_limit` cycles after WFI resumes execution.
  if (has_wfi()) {
    update_last_commit();
  }

  // check whether there're any commits in the last `stuck_limit` cycles
  if (has_commit && cycleCnt > last_commit + stuck_commit_limit) {
    Info(
        "No instruction of core %d commits for %lu cycles, maybe get stuck\n"
        "(please also check whether a fence.i instruction requires more than %lu cycles to flush the icache)\n",
        id, stuck_commit_limit, stuck_commit_limit);
    Info("Let REF run one more instruction.\n");
    proxy->ref_exec(1);
    display();
    return 1;
  }

  return 0;
}

int Difftest::update_delayed_writeback() {
#define CHECK_DELAYED_WB(wb, delayed, n, regs_name)                                                \
  do {                                                                                             \
    for (int i = 0; i < n; i++) {                                                                  \
      auto delay = dut->wb + i;                                                                    \
      if (delay->valid) {                                                                          \
        delay->valid = false;                                                                      \
        if (!delayed[delay->address]) {                                                            \
          display();                                                                               \
          Info("Delayed writeback at %s has already been committed\n", regs_name[delay->address]); \
          raise_trap(STATE_ABORT);                                                                 \
          return 1;                                                                                \
        }                                                                                          \
        if (delay->nack) {                                                                         \
          if (delayed[delay->address] > delay_wb_limit) {                                          \
            delayed[delay->address] -= 1;                                                          \
          }                                                                                        \
        } else {                                                                                   \
          delayed[delay->address] = 0;                                                             \
        }                                                                                          \
        progress = true;                                                                           \
      }                                                                                            \
    }                                                                                              \
  } while (0);

#ifdef CONFIG_DIFFTEST_ARCHINTDELAYEDUPDATE
  CHECK_DELAYED_WB(regs_int_delayed, delayed_int, CONFIG_DIFF_REGS_INT_DELAYED_WIDTH, regs_name_int)
#endif // CONFIG_DIFFTEST_ARCHINTDELAYEDUPDATE
#ifdef CONFIG_DIFFTEST_ARCHFPDELAYEDUPDATE
  CHECK_DELAYED_WB(regs_fp_delayed, delayed_fp, CONFIG_DIFF_REGS_FP_DELAYED_WIDTH, regs_name_fp)
#endif // CONFIG_DIFFTEST_ARCHFPDELAYEDUPDATE
  return 0;
}

int Difftest::apply_delayed_writeback() {
#define APPLY_DELAYED_WB(delayed, regs, regs_name)                           \
  do {                                                                       \
    static const int m = delay_wb_limit;                                     \
    for (int i = 0; i < 32; i++) {                                           \
      if (delayed[i]) {                                                      \
        if (delayed[i] > m) {                                                \
          display();                                                         \
          Info("%s is delayed for more than %d cycles.\n", regs_name[i], m); \
          raise_trap(STATE_ABORT);                                           \
          return 1;                                                          \
        }                                                                    \
        delayed[i]++;                                                        \
        dut->regs.value[i] = proxy->regs.value[i];                           \
      }                                                                      \
    }                                                                        \
  } while (0);

#ifdef CONFIG_DIFFTEST_ARCHINTDELAYEDUPDATE
  APPLY_DELAYED_WB(delayed_int, regs_int, regs_name_int)
#endif // CONFIG_DIFFTEST_ARCHINTDELAYEDUPDATE
#ifdef CONFIG_DIFFTEST_ARCHFPDELAYEDUPDATE
  APPLY_DELAYED_WB(delayed_fp, regs_fp, regs_name_fp)
#endif // CONFIG_DIFFTEST_ARCHFPDELAYEDUPDATE
  return 0;
}

void Difftest::raise_trap(int trapCode) {
  dut->trap.hasTrap = 1;
  dut->trap.code = trapCode;
}

#ifdef CONFIG_DIFFTEST_NONREGINTERRUPTPENDINGEVENT
void Difftest::do_non_reg_interrupt_pending() {
  if (dut->non_reg_interrupt_pending.valid) {
    struct NonRegInterruptPending ip;
    ip.platformIRPMeip = dut->non_reg_interrupt_pending.platformIRPMeip;
    ip.platformIRPMtip = dut->non_reg_interrupt_pending.platformIRPMtip;
    ip.platformIRPMsip = dut->non_reg_interrupt_pending.platformIRPMsip;
    ip.platformIRPSeip = dut->non_reg_interrupt_pending.platformIRPSeip;
    ip.platformIRPStip = dut->non_reg_interrupt_pending.platformIRPStip;
    ip.platformIRPVseip = dut->non_reg_interrupt_pending.platformIRPVseip;
    ip.platformIRPVstip = dut->non_reg_interrupt_pending.platformIRPVstip;
    ip.fromAIAMeip = dut->non_reg_interrupt_pending.fromAIAMeip;
    ip.fromAIASeip = dut->non_reg_interrupt_pending.fromAIASeip;
    ip.localCounterOverflowInterruptReq = dut->non_reg_interrupt_pending.localCounterOverflowInterruptReq;

    proxy->non_reg_interrupt_pending(ip);
    dut->non_reg_interrupt_pending.valid = 0;
  }
}
#endif

#ifdef CONFIG_DIFFTEST_MHPMEVENTOVERFLOWEVENT
void Difftest::do_mhpmevent_overflow() {
  if (dut->mhpmevent_overflow.valid) {
    proxy->mhpmevent_overflow(dut->mhpmevent_overflow.mhpmeventOverflow);
    dut->mhpmevent_overflow.valid = 0;
  }
}
#endif

#ifdef CONFIG_DIFFTEST_CRITICALERROREVENT
void Difftest::do_raise_critical_error() {
  if (dut->critical_error.valid) {
    bool ref_critical_error = proxy->raise_critical_error();
    if (ref_critical_error == dut->critical_error.criticalError) {
      Info("Core %d dump: " ANSI_COLOR_RED
           "HIT CRITICAL ERROR: please check if software cause a double trap. \n" ANSI_COLOR_RESET,
           this->id);
      raise_trap(STATE_GOODTRAP);
    } else {
      display();
      Info("Core %d dump: DUT critical_error diff REF \n", this->id);
      raise_trap(STATE_ABORT);
    }
  }
}
#endif

#ifdef CONFIG_DIFFTEST_SYNCAIAEVENT
void Difftest::do_sync_aia() {
  if (dut->sync_aia.valid) {
    struct FromAIA aia;
    aia.mtopei = dut->sync_aia.mtopei;
    aia.stopei = dut->sync_aia.stopei;
    aia.vstopei = dut->sync_aia.vstopei;
    aia.hgeip = dut->sync_aia.hgeip;
    proxy->sync_aia(aia);
    dut->sync_aia.valid = 0;
  }
}
#endif

#ifdef CONFIG_DIFFTEST_SYNCCUSTOMMFLUSHPWREVENT
void Difftest::do_sync_custom_mflushpwr() {
  if (dut->sync_custom_mflushpwr.valid) {
    proxy->sync_custom_mflushpwr(dut->sync_custom_mflushpwr.l2FlushDone);
    dut->sync_custom_mflushpwr.valid = 0;
  }
}
#endif

void Difftest::display() {
  Info("\n==============  In the last commit group  ==============\n");
  Info("the first commit instr pc of DUT is 0x%016lx\nthe first commit instr pc of REF is 0x%016lx\n",
       dut_commit_first_pc, ref_commit_first_pc);

  state->display(this->id);

  Info("\n==============  REF Regs  ==============\n");
  fflush(stdout);
  proxy->ref_reg_display();
  Info("privilegeMode: %lu\n", dut->csr.privilegeMode);
}

void CommitTrace::display(bool use_spike) {
  Info("%s pc %016lx inst %08x", get_type(), pc, inst);
  display_custom();
  if (use_spike) {
    Info(" %s", spike_dasm(inst));
  }
}

void Difftest::display_stats() {
  auto trap = get_trap_event();
  uint64_t instrCnt = trap->instrCnt;
  uint64_t cycleCnt = trap->cycleCnt;
  double ipc = (double)instrCnt / cycleCnt;
  Info(ANSI_COLOR_MAGENTA "Core-%d instrCnt = %'" PRIu64 ", cycleCnt = %'" PRIu64 ", IPC = %lf\n" ANSI_COLOR_RESET,
       this->id, instrCnt, cycleCnt, ipc);
}

void DiffState::display_commit_count(int i) {
  auto retire_pointer = (retire_group_pointer + DEBUG_GROUP_TRACE_SIZE - 1) % DEBUG_GROUP_TRACE_SIZE;
  Info("commit group [%02d]: pc %010lx cmtcnt %d%s\n", i, retire_group_pc_queue[i], retire_group_cnt_queue[i],
       (i == retire_pointer) ? " <--" : "");
}

void DiffState::display_commit_instr(int i) {
  display_commit_instr(retire_inst_pointer, spike_valid());
  fflush(stdout);
}

void DiffState::display_commit_instr(int i, bool use_spike) {
  if (!commit_trace[i]) {
    return;
  }
  Info("[%02d] ", i);
  commit_trace[i]->display(use_spike);
  auto retire_pointer = (retire_inst_pointer + DEBUG_INST_TRACE_SIZE - 1) % DEBUG_INST_TRACE_SIZE;
  Info("%s\n", (i == retire_pointer) ? " <--" : "");
}

void DiffState::display(int coreid) {
  Info("\n============== Commit Group Trace (Core %d) ==============\n", coreid);
  for (int j = 0; j < DEBUG_GROUP_TRACE_SIZE; j++) {
    display_commit_count(j);
  }

  Info("\n============== Commit Instr Trace ==============\n");
  bool use_spike = spike_valid();
  for (int j = 0; j < DEBUG_INST_TRACE_SIZE; j++) {
    display_commit_instr(j, use_spike);
  }

  fflush(stdout);
}

DiffState::DiffState() : commit_trace(DEBUG_INST_TRACE_SIZE, nullptr) {}
