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

Difftest::Difftest(int coreid) : id(coreid) {
  state = new DiffState(coreid);
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

void Difftest::init_checkers() {
  arch_event_checker = new ArchEventChecker([this]() -> DifftestArchEvent & { return dut->event; }, state, proxy,
                                            [this]() -> const DiffTestRegState & { return dut->regs; });
  first_instr_commit_checker =
      new FirstInstrCommitChecker([this]() -> DifftestInstrCommit & { return dut->commit[0]; }, state, proxy,
                                  [this]() -> const DiffTestRegState & { return dut->regs; });
  for (int i = 0; i < CONFIG_DIFF_COMMIT_WIDTH; i++) {
    instr_commit_checker[i] = new InstrCommitChecker([this, i]() -> DifftestInstrCommit & { return dut->commit[i]; },
                                                     state, proxy, [this]() -> const DiffTestState & { return *dut; });
  }
#ifdef CONFIG_DIFFTEST_LRSCEVENT
  lrsc_checker = new LrScChecker([this]() -> DifftestLrScEvent & { return dut->lrsc; }, state, proxy);
#endif // CONFIG_DIFFTEST_LRSCEVENT
#ifdef CONFIG_DIFFTEST_REFILLEVENT
  for (int i = 0; i < CONFIG_DIFF_REFILL_WIDTH; i++) {
    refill_checker[i] =
        new RefillChecker([this, i]() -> DifftestRefillEvent & { return dut->refill[i]; }, state, proxy);
  }
#endif // CONFIG_DIFFTEST_REFILLEVENT
#ifdef CONFIG_DIFFTEST_L1TLBEVENT
  for (int i = 0; i < CONFIG_DIFF_L1TLB_WIDTH; i++) {
    l1tlb_checker[i] = new L1TLBChecker([this, i]() -> DifftestL1TLBEvent & { return dut->l1tlb[i]; }, state, proxy);
  }
#endif // CONFIG_DIFFTEST_L1TLBEVENT
#ifdef CONFIG_DIFFTEST_L2TLBEVENT
  for (int i = 0; i < CONFIG_DIFF_L2TLB_WIDTH; i++) {
    l2tlb_checker[i] = new L2TLBChecker([this, i]() -> DifftestL2TLBEvent & { return dut->l2tlb[i]; }, state, proxy);
  }
#endif // CONFIG_DIFFTEST_L2TLBEVENT
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
  state->has_progress = false;

  if (check_timeout()) {
    return 1;
  }
  first_instr_commit_checker->step();

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

#ifdef DEBUG_REFILL
#ifdef CONFIG_DIFFTEST_REFILLEVENT
  for (int i = 0; i < CONFIG_DIFF_REFILL_WIDTH; i++) {
    if (int ret = refill_checker[i]->step()) {
      return ret;
    }
  }
#endif // CONFIG_DIFFTEST_REFILLEVENT
#endif

#ifdef DEBUG_L2TLB
#ifdef CONFIG_DIFFTEST_L2TLBEVENT
  for (int i = 0; i < CONFIG_DIFF_L2TLB_WIDTH; i++) {
    if (int ret = l2tlb_checker[i]->step()) {
      return ret;
    }
  }
#endif // CONFIG_DIFFTEST_L2TLBEVENT
#endif

#ifdef DEBUG_L1TLB
#ifdef CONFIG_DIFFTEST_L1TLBEVENT
  for (int i = 0; i < CONFIG_DIFF_L1TLB_WIDTH; i++) {
    if (int ret = l1tlb_checker[i]->step()) {
      return ret;
    }
  }
#endif // CONFIG_DIFFTEST_L1TLBEVENT
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
  lrsc_checker->step();
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
    if (int ret = arch_event_checker->step()) {
      return ret;
    }
  } else {
#if !defined(BASIC_DIFFTEST_ONLY) && !defined(CONFIG_DIFFTEST_SQUASH)
    if (dut->commit[0].valid) {
      dut_commit_first_pc = dut->commit[0].pc;
      ref_commit_first_pc = proxy->state.pc;
      if (dut_commit_first_pc != ref_commit_first_pc) {
        pc_mismatch = true;
      }
    }
#endif
    for (int i = 0; i < CONFIG_DIFF_COMMIT_WIDTH; i++) {
      if (dut->commit[i].valid) {
        if (instr_commit_checker[i]->step()) {
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

  if (!state->has_progress) {
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
    if (pc_mismatch) {
      REPORT_DIFFERENCE("pc", ref_commit_first_pc, ref_commit_first_pc, dut_commit_first_pc);
    }
#ifdef FUZZER_LIB
    stats.exit_code = SimExitCode::difftest;
#endif // FUZZER_LIB
    return 1;
  }

  return 0;
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
    auto vecNextLdest = vecFirstLdest + vdidx;
    for (int i = 0; i < VLENE_64; i++) {
      int dataIndex = VLENE_64 * vdidx + i;
#ifdef CONFIG_DIFFTEST_VECCOMMITDATA
#ifdef CONFIG_DIFFTEST_SQUASH
      uint64_t dutRegData = load_event.vecCommitData[dataIndex];
#else
      uint64_t dutRegData = dut->vec_commit_data[index].data[dataIndex];
#endif // CONFIG_DIFFTEST_SQUASH
#else
      // TODO: support Squash without vec_commit_data (i.e. update ArchReg in software)
      auto vecNextPdest = dut->commit[index].otherwpdest[dataIndex];
      uint64_t dutRegData = dut->pregs_vrf.value[vecPdest];
#endif // CONFIG_DIFFTEST_VECCOMMITDATA

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
      uint64_t *dutRegPtr = v0Wen ? dut->wb_v0[vecNextPdest].data : dut->wb_vrf[vecNextPdest].data;
#endif // !CONFIG_DIFFTEST_COMMITDATA

      for (int i = 0; i < VLENE_64; i++) {
#ifdef CONFIG_DIFFTEST_COMMITDATA
#ifdef CONFIG_DIFFTEST_SQUASH
        uint64_t dutRegData = load_event.vecCommitData[VLENE_64 * vdidx + i];
#else
        uint64_t dutRegData = dut->vec_commit_data[index].data[VLENE_64 * vdidx + i];
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
        uint64_t *dutRegPtr = v0Wen ? dut->wb_v0[vecNextPdest].data : dut->wb_vrf[vecNextPdest].data;
#endif // !CONFIG_DIFFTEST_COMMITDATA

        auto vecNextLdest = vecFirstLdest + vdidx;

        for (int i = 0; i < VLENE_64; i++) {
#ifdef CONFIG_DIFFTEST_COMMITDATA
#ifdef CONFIG_DIFFTEST_SQUASH
          uint64_t dutRegData = load_event.vecCommitData[VLENE_64 * vdidx + i];
#else
          uint64_t dutRegData = dut->vec_commit_data[index].data[VLENE_64 * vdidx + i];
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
        uint64_t golden_flag;
        uint64_t mask = 0xFFFFFFFFFFFFFFFF;
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
        read_goldenmem(load_event.paddr, &golden, len, &golden_flag);
        if (load_event.isLoad) {
          switch (len) {
            case 1:
              golden = (int64_t)(int8_t)golden;
              golden_flag = (int64_t)(int8_t)golden_flag;
              mask = (uint64_t)(0xFF);
              break;
            case 2:
              golden = (int64_t)(int16_t)golden;
              golden_flag = (int64_t)(int16_t)golden_flag;
              mask = (uint64_t)(0xFFFF);
              break;
            case 4:
              golden = (int64_t)(int32_t)golden;
              golden_flag = (int64_t)(int32_t)golden_flag;
              mask = (uint64_t)(0xFFFFFFFF);
              break;
          }
        }
        if (golden == commitData || load_event.isAtomic) { //  atomic instr carefully handled
          proxy->ref_memcpy(load_event.paddr, &golden, len, DUT_TO_REF);
          if (regWen) {
            *refRegPtr = commitData;
            proxy->sync(true);
          }
        } else if (load_event.isLoad && golden_flag != 0) {
          // goldenmem check failed, but the flag is set, so use DUT data to reset
          Info("load check of uncache mm store flag\n");
          Info("  DUT data: 0x%lx, regWen: %d, refRegPtr: 0x%lx\n", commitData, regWen, refRegPtr);
          proxy->ref_memcpy(load_event.paddr, &commitData, len, DUT_TO_REF);
          update_goldenmem(load_event.paddr, &commitData, mask, len);
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

#ifdef CONFIG_DIFFTEST_UNCACHEMMSTOREEVENT
  for (int i = 0; i < CONFIG_DIFF_UNCACHE_MM_STORE_WIDTH; i++) {
    if (dut->uncache_mm_store[i].valid) {
      dut->uncache_mm_store[i].valid = 0;
      // the flag is set only in the case of multi-cores and uncache mm store
      uint8_t flag = NUM_CORES > 1 ? 1 : 0;
      update_goldenmem(dut->uncache_mm_store[i].addr, dut->uncache_mm_store[i].data, dut->uncache_mm_store[i].mask, 8,
                       flag);
      if (dut->uncache_mm_store[i].addr == track_instr) {
        dumpGoldenMem("Uncache MM Store", track_instr, cycleCnt);
      }
    }
  }
#endif // CONFIG_DIFFTEST_UNCACHEMMSTOREEVENT
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
  if (!state->has_commit && cycleCnt > state->last_commit_cycle + first_commit_limit) {
    Info("The first instruction of core %d at 0x%lx does not commit after %lu cycles.\n", id, FIRST_INST_ADDRESS,
         first_commit_limit);
    display();
    return 1;
  }

  // NOTE: the WFI instruction may cause the CPU to halt for more than `stuck_limit` cycles.
  // We update the `last_commit_cycle` if the CPU has a WFI instruction
  // to allow the CPU to run at most `stuck_limit` cycles after WFI resumes execution.
  if (has_wfi()) {
    update_last_commit();
  }

  // check whether there're any commits in the last `stuck_limit` cycles
  if (state->has_commit && cycleCnt > state->last_commit_cycle + stuck_commit_limit) {
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
  return 0;
}

int Difftest::apply_delayed_writeback() {
#define APPLY_DELAYED_WB(delayed, reg_type, regs_name)                       \
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
  state->display();

  Info("\n==============  REF Regs  ==============\n");
  fflush(stdout);
  proxy->ref_reg_display();
  Info("privilegeMode: %lu\n", dut->regs.csr.privilegeMode);
}

void Difftest::display_stats() {
  auto trap = get_trap_event();
  uint64_t instrCnt = trap->instrCnt;
  uint64_t cycleCnt = trap->cycleCnt;
  double ipc = (double)instrCnt / cycleCnt;
  Info(ANSI_COLOR_MAGENTA "Core-%d instrCnt = %'" PRIu64 ", cycleCnt = %'" PRIu64 ", IPC = %lf\n" ANSI_COLOR_RESET,
       this->id, instrCnt, cycleCnt, ipc);
}

// API for soft warmup, display final instr/cycle - warmup instr/cycle
void Difftest::warmup_display_stats() {
  auto trap = get_trap_event();
  uint64_t instrCnt = trap->instrCnt - warmup_info.instrCnt;
  uint64_t cycleCnt = trap->cycleCnt - warmup_info.cycleCnt;
  double ipc = (double)instrCnt / cycleCnt;
  Info(ANSI_COLOR_MAGENTA "Core-%d(Soft Warmup) instrCnt = %'" PRIu64 ", cycleCnt = %'" PRIu64
                          ", IPC = %lf\n" ANSI_COLOR_RESET,
       this->id, instrCnt, cycleCnt, ipc);
}
