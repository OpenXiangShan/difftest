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
#if defined(CONFIG_DIFFTEST_SQUASH) && !defined(CONFIG_DIFFTEST_FPGA)
#include "svdpi.h"
#endif // CONFIG_DIFFTEST_SQUASH && !CONFIG_DIFFTEST_FPGA
#ifdef CONFIG_DIFFTEST_PERFCNT
#include "perf.h"
#endif // CONFIG_DIFFTEST_PERFCNT
#ifdef CONFIG_DIFFTEST_QUERY
#include "query.h"
#endif // CONFIG_DIFFTEST_QUERY

Difftest **difftest = NULL;

int difftest_init(bool enabled, size_t ramsize) {
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
  // put init_goldenmem before update_nemuproxy because the latter requires goldenmem
  if (enabled) {
    init_goldenmem();
  }
  for (int i = 0; i < NUM_CORES; i++) {
    difftest[i] = new Difftest(i);
    difftest[i]->dut = diffstate_buffer[i]->get(0, 0);
    if (enabled) {
      difftest[i]->update_nemuproxy(i, ramsize);
      difftest[i]->init_checkers();
    }
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
  return STATE_RUNNING;
}

int difftest_nstep(int step, bool enable_diff) {
#ifdef CONFIG_DIFFTEST_PERFCNT
  difftest_calls[perf_difftest_nstep]++;
  difftest_bytes[perf_difftest_nstep] += 1;
#endif // CONFIG_DIFFTEST_PERFCNT

#if CONFIG_DIFFTEST_ZONESIZE > 1
  difftest_switch_zone();
#endif // CONFIG_DIFFTEST_ZONESIZE
  for (int i = 0; i < step; i++) {
    if (enable_diff) {
      if (int ret = difftest_step())
        return ret;
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

// difftest_step returns a trap code
int difftest_step() {
  difftest_set_dut();
#if defined(CONFIG_DIFFTEST_QUERY) && !defined(CONFIG_DIFFTEST_BATCH)
  difftest_query_step();
#endif // CONFIG_DIFFTEST_QUERY
  for (int i = 0; i < NUM_CORES; i++) {
    if (int ret = difftest[i]->step()) {
      switch (ret) {
        case DiffTestChecker::STATE_DIFF: difftest[i]->display();
        case DiffTestChecker::STATE_ERROR: return STATE_ABORT;
        default: // STATE_TRAP
          return difftest[i]->get_trap_code();
      }
    }
  }
  return DiffTestChecker::STATE_OK;
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

#if defined(CONFIG_DIFFTEST_SQUASH) && !defined(CONFIG_DIFFTEST_FPGA)
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
#endif // CONFIG_DIFFTEST_SQUASH && !CONFIG_DIFFTEST_FPGA

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

Difftest::Difftest(int coreid) {
  state = new DiffState(coreid);
#ifdef CONFIG_DIFFTEST_REPLAY
  state_ss = (DiffState *)malloc(sizeof(DiffState));
#endif // CONFIG_DIFFTEST_REPLAY
}

Difftest::~Difftest() {
  for (auto checker: checkers) {
    delete checker;
  }

  delete arch_event_checker;
  for (int i = 0; i < CONFIG_DIFF_COMMIT_WIDTH; i++) {
    delete instr_commit_checker[i];
  }
#ifdef CONFIG_DIFFTEST_STOREEVENT
  delete store_checker;
#endif // CONFIG_DIFFTEST_STOREEVENT
#ifdef CONFIG_DIFFTEST_LOADEVENT
  for (int i = 0; i < CONFIG_DIFF_LOAD_WIDTH; i++) {
    delete load_checker[i];
  }
#ifdef CONFIG_DIFFTEST_SQUASH
  delete load_squash_checker;
#endif // CONFIG_DIFFTEST_SQUASH
#endif // CONFIG_DIFFTEST_LOADEVENT

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

void Difftest::init_checkers() {
  checkers.push_back(new TimeoutChecker([this]() -> DifftestTrapEvent & { return dut->trap; }, state, proxy));

  checkers.push_back(new FirstInstrCommitChecker([this]() -> DifftestInstrCommit & { return dut->commit[0]; }, state,
                                                 proxy, [this]() -> const DiffTestRegState & { return dut->regs; }));

  // Each cycle is checked for an store event, and recorded in queue.
  // It is checked every time an instruction is committed and queue has content.
#ifdef CONFIG_DIFFTEST_STOREEVENT
  for (int i = 0; i < CONFIG_DIFF_STORE_WIDTH; i++) {
    checkers.push_back(new StoreRecorder([this, i]() -> DifftestStoreEvent & { return dut->store[i]; }, state, proxy));
  }
#endif

#ifdef CONFIG_DIFFTEST_LOADEVENT
  for (int i = 0; i < CONFIG_DIFF_LOAD_WIDTH; i++) {
    auto checker = new LoadChecker([this, i]() -> DifftestLoadEvent & { return dut->load[i]; }, state, proxy, i,
                                   [this]() -> const DiffTestState & { return *dut; });
#ifdef CONFIG_DIFFTEST_SQUASH
    checkers.push_back(checker);
#else
    load_checker[i] = checker;
#endif // CONFIG_DIFFTEST_SQUASH
  }
#endif // CONFIG_DIFFTEST_LOADEVENT

#ifdef DEBUG_GOLDENMEM
  checkers.push_back(new GoldenMemoryInit([this]() -> DifftestTrapEvent & { return dut->trap; }, state, proxy));

#ifdef CONFIG_DIFFTEST_UNCACHEMMSTOREEVENT
  for (int i = 0; i < CONFIG_DIFF_UNCACHE_MM_STORE_WIDTH; i++) {
    checkers.push_back(new UncacheMmStoreChecker(
        [this, i]() -> DifftestUncacheMMStoreEvent & { return dut->uncache_mm_store[i]; }, state, proxy));
  }
#endif // CONFIG_DIFFTEST_UNCACHEMMSTOREEVENT

#ifdef CONFIG_DIFFTEST_SBUFFEREVENT
  for (int i = 0; i < CONFIG_DIFF_SBUFFER_WIDTH; i++) {
    checkers.push_back(
        new SbufferChecker([this, i]() -> DifftestSbufferEvent & { return dut->sbuffer[i]; }, state, proxy));
  }
#endif // CONFIG_DIFFTEST_SBUFFEREVENT

#ifdef CONFIG_DIFFTEST_ATOMICEVENT
  checkers.push_back(new AtomicChecker([this]() -> DifftestAtomicEvent & { return dut->atomic; }, state, proxy));
#endif // CONFIG_DIFFTEST_ATOMICEVENT
#endif

#ifdef CONFIG_DIFFTEST_CMOINVALEVENT
  checkers.push_back(
      new CmoInvalRecorder([this]() -> DifftestCMOInvalEvent & { return dut->cmo_inval; }, state, proxy));
#endif // CONFIG_DIFFTEST_CMOINVALEVENT

#ifdef DEBUG_REFILL
#ifdef CONFIG_DIFFTEST_REFILLEVENT
  for (int i = 0; i < CONFIG_DIFF_REFILL_WIDTH; i++) {
    checkers.push_back(
        new RefillChecker([this, i]() -> DifftestRefillEvent & { return dut->refill[i]; }, state, proxy, i));
  }
#endif // CONFIG_DIFFTEST_REFILLEVENT
#endif

#ifdef DEBUG_L2TLB
#ifdef CONFIG_DIFFTEST_L2TLBEVENT
  for (int i = 0; i < CONFIG_DIFF_L2TLB_WIDTH; i++) {
    checkers.push_back(new L2TLBChecker([this, i]() -> DifftestL2TLBEvent & { return dut->l2tlb[i]; }, state, proxy));
  }
#endif // CONFIG_DIFFTEST_L2TLBEVENT
#endif

#ifdef DEBUG_L1TLB
#ifdef CONFIG_DIFFTEST_L1TLBEVENT
  for (int i = 0; i < CONFIG_DIFF_L1TLB_WIDTH; i++) {
    checkers.push_back(new L1TLBChecker([this, i]() -> DifftestL1TLBEvent & { return dut->l1tlb[i]; }, state, proxy));
  }
#endif // CONFIG_DIFFTEST_L1TLBEVENT
#endif

#ifdef CONFIG_DIFFTEST_LRSCEVENT
  checkers.push_back(new LrScChecker([this]() -> DifftestLrScEvent & { return dut->lrsc; }, state, proxy));
#endif

#ifdef CONFIG_DIFFTEST_NONREGINTERRUPTPENDINGEVENT
  checkers.push_back(new NonRegInterruptPendingChecker(
      [this]() -> DifftestNonRegInterruptPendingEvent & { return dut->non_reg_interrupt_pending; }, state, proxy));
#endif

#ifdef CONFIG_DIFFTEST_MHPMEVENTOVERFLOWEVENT
  checkers.push_back(new MhpmeventOverflowChecker(
      [this]() -> DifftestMhpmeventOverflowEvent & { return dut->mhpmevent_overflow; }, state, proxy));
#endif
#ifdef CONFIG_DIFFTEST_CRITICALERROREVENT
  checkers.push_back(
      new CriticalErrorChecker([this]() -> DifftestCriticalErrorEvent & { return dut->critical_error; }, state, proxy));
#endif
#ifdef CONFIG_DIFFTEST_SYNCAIAEVENT
  checkers.push_back(new AiaChecker([this]() -> DifftestSyncAIAEvent & { return dut->sync_aia; }, state, proxy));
#endif
#ifdef CONFIG_DIFFTEST_SYNCCUSTOMMFLUSHPWREVENT
  checkers.push_back(new CustomMflushpwrChecker(
      [this]() -> DifftestSyncCustomMflushpwrEvent & { return dut->sync_custom_mflushpwr; }, state, proxy));
#endif

  arch_event_checker = new ArchEventChecker([this]() -> DifftestArchEvent & { return dut->event; }, state, proxy,
                                            [this]() -> const DiffTestRegState & { return dut->regs; });
  for (int i = 0; i < CONFIG_DIFF_COMMIT_WIDTH; i++) {
    instr_commit_checker[i] =
        new InstrCommitChecker([this, i]() -> DifftestInstrCommit & { return dut->commit[i]; }, state, proxy, i,
                               [this]() -> const DiffTestState & { return *dut; });
  }
#if defined(CONFIG_DIFFTEST_LOADEVENT) && defined(CONFIG_DIFFTEST_SQUASH)
  load_squash_checker = new LoadSquashChecker(state, proxy, [this]() -> const DiffTestState & { return *dut; });
#endif // CONFIG_DIFFTEST_LOADEVENT && CONFIG_DIFFTEST_SQUASH
#ifdef CONFIG_DIFFTEST_STOREEVENT
  store_checker = new StoreChecker(state, proxy);
#endif // CONFIG_DIFFTEST_STOREEVENT
}

void Difftest::update_nemuproxy(int coreid, size_t ram_size = 0) {
  proxy = new REF_PROXY(coreid, ram_size);
#ifdef CONFIG_DIFFTEST_REPLAY
  proxy_reg_ss = (uint8_t *)malloc(sizeof(ref_state_t));
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
  memcpy(proxy_reg_ss, &proxy->state, sizeof(ref_state_t));
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
  memcpy(&proxy->state, proxy_reg_ss, sizeof(ref_state_t));
  proxy->ref_regcpy(&proxy->state, DUT_TO_REF, false);
  proxy->ref_csrcpy(squash_csr_buf, DUT_TO_REF);
  proxy->ref_store_log_restore();
  goldenmem_store_log_restore();
  difftest_replay_head(info.trace_head);
  // clear buffered queue
#ifdef CONFIG_DIFFTEST_STOREEVENT
  while (!state->store_event_queue.empty())
    state->store_event_queue.pop();
#endif // CONFIG_DIFFTEST_STOREEVENT
#if defined(CONFIG_DIFFTEST_LOADEVENT) && defined(CONFIG_DIFFTEST_SQUASH)
  while (!state->load_event_queue.empty())
    state->load_event_queue.pop();
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
  state->cycle_count = get_trap_event()->cycleCnt;
  state->has_progress = false;

  // normal checkers
  for (auto checker: checkers) {
    if (int ret = checker->step()) {
      return ret;
    }
  }

#ifdef DEBUG_MODE_DIFF
  // skip load & store insts in debug mode
  // for other insts copy inst content to ref's dummy debug module
  for (int i = 0; i < DIFFTEST_COMMIT_WIDTH; i++) {
    if (DEBUG_MEM_REGION(dut->commit[i].valid, dut->commit[i].pc))
      debug_mode_copy(dut->commit[i].pc, dut->commit[i].isRVC ? 2 : 4, dut->commit[i].inst);
  }
#endif

  num_commit = 0; // reset num_commit this cycle to 0
  if (dut->event.valid) {
    if (int ret = arch_event_checker->step()) {
      return ret;
    }
    dut->commit[0].valid = 0;
  } else {
#if !defined(BASIC_DIFFTEST_ONLY) && !defined(CONFIG_DIFFTEST_SQUASH)
    if (dut->commit[0].valid) {
      dut_commit_batch_pc = dut->commit[0].pc;
      ref_commit_batch_pc = proxy->state.pc;
      if (dut_commit_batch_pc != ref_commit_batch_pc) {
        pc_mismatch = true;
      }
    }
#endif
    for (int i = 0; i < CONFIG_DIFF_COMMIT_WIDTH; i++) {
      if (dut->commit[i].valid) {
        num_commit += 1 + dut->commit[i].nFused;
        if (int ret = instr_commit_checker[i]->step()) {
          return ret;
        }
#ifdef CONFIG_DIFFTEST_LOADEVENT
#ifdef CONFIG_DIFFTEST_SQUASH
        load_squash_checker->step();
#else
        load_checker[i]->step();
#endif // CONFIG_DIFFTEST_SQUASH
#endif // CONFIG_DIFFTEST_LOADEVENT
#ifdef CONFIG_DIFFTEST_STOREEVENT
        // check is the same for all checkers
        if (int ret = store_checker->step()) {
          return ret;
        }
#endif // CONFIG_DIFFTEST_STOREEVENT
      }
    }
  }

  if (int ret = update_delayed_writeback()) {
    return ret;
  }

  if (!state->has_progress) {
    return DiffTestChecker::STATE_OK;
  }

  proxy->sync();

  if (num_commit > 0) {
    state->record_group(dut->commit[0].pc, num_commit);
  }

  if (apply_delayed_writeback()) {
    return DiffTestChecker::STATE_DIFF;
  }

  if (proxy->compare(dut) || pc_mismatch) {
#ifdef FUZZING
    if (in_disambiguation_state()) {
      Info("Mismatch detected with a disambiguation state at pc = 0x%lx.\n", dut->trap.pc);
      state->raise_trap(STATE_FUZZ_COND);
      return DiffTestChecker::STATE_TRAP;
    }
#endif
#ifdef FUZZER_LIB
    stats.exit_code = SimExitCode::difftest;
#endif // FUZZER_LIB
    return DiffTestChecker::STATE_DIFF;
  }

  return DiffTestChecker::STATE_OK;
}

int Difftest::update_delayed_writeback() {
#define CHECK_DELAYED_WB(wb, delayed, n, regs_name)                                                \
  do {                                                                                             \
    for (int i = 0; i < n; i++) {                                                                  \
      auto delay = dut->wb + i;                                                                    \
      if (delay->valid) {                                                                          \
        delay->valid = false;                                                                      \
        if (!delayed[delay->address]) {                                                            \
          Info("Delayed writeback at %s has already been committed\n", regs_name[delay->address]); \
          return DiffTestChecker::STATE_DIFF;                                                      \
        }                                                                                          \
        if (delay->nack) {                                                                         \
          if (delayed[delay->address] > delay_wb_limit) {                                          \
            delayed[delay->address] -= 1;                                                          \
          }                                                                                        \
        } else {                                                                                   \
          delayed[delay->address] = 0;                                                             \
        }                                                                                          \
        state->has_progress = true;                                                                \
      }                                                                                            \
    }                                                                                              \
  } while (0);

#ifdef CONFIG_DIFFTEST_ARCHINTDELAYEDUPDATE
  CHECK_DELAYED_WB(regs_int_delayed, state->delayed_int, CONFIG_DIFF_REGS_INT_DELAYED_WIDTH, regs_name_int)
#endif // CONFIG_DIFFTEST_ARCHINTDELAYEDUPDATE
#ifdef CONFIG_DIFFTEST_ARCHFPDELAYEDUPDATE
  CHECK_DELAYED_WB(regs_fp_delayed, state->delayed_fp, CONFIG_DIFF_REGS_FP_DELAYED_WIDTH, regs_name_fp)
#endif // CONFIG_DIFFTEST_ARCHFPDELAYEDUPDATE
  return DiffTestChecker::STATE_OK;
}

int Difftest::apply_delayed_writeback() {
#define APPLY_DELAYED_WB(delayed, reg_type, regs_name)                       \
  do {                                                                       \
    static const int m = delay_wb_limit;                                     \
    for (int i = 0; i < 32; i++) {                                           \
      if (delayed[i]) {                                                      \
        if (delayed[i] > m) {                                                \
          Info("%s is delayed for more than %d cycles.\n", regs_name[i], m); \
          return DiffTestChecker::STATE_DIFF;                                \
        }                                                                    \
        delayed[i]++;                                                        \
        dut->regs.reg_type.value[i] = proxy->state.reg_type.value[i];        \
      }                                                                      \
    }                                                                        \
  } while (0);

#ifdef CONFIG_DIFFTEST_ARCHINTDELAYEDUPDATE
  APPLY_DELAYED_WB(state->delayed_int, xrf, regs_name_int)
#endif // CONFIG_DIFFTEST_ARCHINTDELAYEDUPDATE
#ifdef CONFIG_DIFFTEST_ARCHFPDELAYEDUPDATE
  APPLY_DELAYED_WB(state->delayed_fp, frf, regs_name_fp)
#endif // CONFIG_DIFFTEST_ARCHFPDELAYEDUPDATE
  return DiffTestChecker::STATE_OK;
}

void Difftest::display() {
  state->display();

  Info("\n==============  REF Regs  ==============\n");
  fflush(stdout);
  proxy->ref_reg_display();
  Info("privilegeMode: %lu\n", dut->regs.csr.privilegeMode);

  // show different register values
  proxy->display(dut);
  if (pc_mismatch) {
    REPORT_DIFFERENCE("pc", ref_commit_batch_pc, ref_commit_batch_pc, dut_commit_batch_pc);
  }
}

void Difftest::display_stats() {
  auto trap = get_trap_event();
  uint64_t instrCnt = trap->instrCnt;
  uint64_t cycleCnt = trap->cycleCnt;
  double ipc = (double)instrCnt / cycleCnt;
  Info(ANSI_COLOR_MAGENTA "Core-%d instrCnt = %'" PRIu64 ", cycleCnt = %'" PRIu64 ", IPC = %lf\n" ANSI_COLOR_RESET,
       state->coreid, instrCnt, cycleCnt, ipc);
}

// API for soft warmup, display final instr/cycle - warmup instr/cycle
void Difftest::warmup_display_stats() {
  auto trap = get_trap_event();
  uint64_t instrCnt = trap->instrCnt - warmup_info.instrCnt;
  uint64_t cycleCnt = trap->cycleCnt - warmup_info.cycleCnt;
  double ipc = (double)instrCnt / cycleCnt;
  Info(ANSI_COLOR_MAGENTA "Core-%d(Soft Warmup) instrCnt = %'" PRIu64 ", cycleCnt = %'" PRIu64
                          ", IPC = %lf\n" ANSI_COLOR_RESET,
       state->coreid, instrCnt, cycleCnt, ipc);
}
