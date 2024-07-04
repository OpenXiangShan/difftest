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
#ifdef CONFIG_DIFFTEST_SQUASH
#include "svdpi.h"
#endif // CONFIG_DIFFTEST_SQUASH
#ifdef CONFIG_DIFFTEST_PERFCNT
#include "perf.h"
#endif // CONFIG_DIFFTEST_PERFCNT

Difftest **difftest = NULL;

int difftest_init() {
#ifdef CONFIG_DIFFTEST_PERFCNT
  difftest_perfcnt_init();
#endif // CONFIG_DIFFTEST_PERFCNT
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
  difftest_switch_zone();
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
  diffstate_buffer_free();
  for (int i = 0; i < NUM_CORES; i++) {
    delete difftest[i];
  }
  delete[] difftest;
  difftest = NULL;
}

#ifdef CONFIG_DIFFTEST_SQUASH
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
#endif // CONFIG_DIFFTEST_SQUASH

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

void Difftest::update_nemuproxy(int coreid, size_t ram_size = 0) {
  proxy = new REF_PROXY(coreid, ram_size);
#ifdef CONFIG_DIFFTEST_REPLAY
  proxy_reg_size = proxy->get_reg_size();
  proxy_reg_ss = (uint8_t *)malloc(proxy_reg_size);
#endif // CONFIG_DIFFTEST_REPLAY
}

#ifdef CONFIG_DIFFTEST_REPLAY
bool Difftest::can_replay() {
  auto info = dut->trace_info;
  return !info.in_replay && info.trace_size > 1;
}

bool Difftest::in_replay_range() {
  if (dut->trace_info.trace_size > 1)
    return false;
  int pos = dut->trace_info.trace_head;
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
  replay_status.in_replay = true;
  replay_status.trace_head = dut->trace_info.trace_head;
  replay_status.trace_size = dut->trace_info.trace_size;
  memcpy(state, state_ss, sizeof(DiffState));
  memcpy(&proxy->regs_int, proxy_reg_ss, proxy_reg_size);
  proxy->ref_regcpy(&proxy->regs_int, DUT_TO_REF, false);
  proxy->ref_csrcpy(squash_csr_buf, DUT_TO_REF);
  proxy->ref_store_log_restore();
  goldenmem_store_log_restore();
  difftest_replay_head(dut->trace_info.trace_head);
}
#endif // CONFIG_DIFFTEST_REPLAY

int Difftest::step() {
  progress = false;
  ticks++;

#ifdef CONFIG_DIFFTEST_REPLAY
  if (replay_status.in_replay && !in_replay_range()) {
    return 0;
  }
  if (can_replay()) {
    replay_snapshot();
  } else {
    proxy->set_store_log(false);
    goldenmem_set_store_log(false);
  }
#endif // CONFIG_DIFFTEST_REPLAY

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

  num_commit = 0; // reset num_commit this cycle to 0
  if (dut->event.valid) {
    // interrupt has a higher priority than exception
    dut->event.interrupt ? do_interrupt() : do_exception();
    dut->event.valid = 0;
    dut->commit[0].valid = 0;
  } else {
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

  // FIXME: the following code is dirty
  if (dut->csr.mip != proxy->csr.mip) { // Ignore difftest for MIP
    proxy->csr.mip = dut->csr.mip;
  }

  if (apply_delayed_writeback()) {
    return 1;
  }

  if (proxy->compare(dut)) {
#ifdef FUZZING
    if (in_disambiguation_state()) {
      Info("Mismatch detected with a disambiguation state at pc = 0x%lx.\n", dut->trap.pc);
      return 0;
    }
#endif
#ifdef CONFIG_DIFFTEST_REPLAY
    if (can_replay()) {
      do_replay();
      return 0;
    }
#endif // CONFIG_DIFFTEST_REPLAY
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

  bool realWen = (dut->commit[i].rfwen && dut->commit[i].wdest != 0) || (dut->commit[i].fpwen);

  // MMIO accessing should not be a branch or jump, just +2/+4 to get the next pc
  // to skip the checking of an instruction, just copy the reg state to reference design
  if (dut->commit[i].skip || (DEBUG_MODE_SKIP(dut->commit[i].valid, dut->commit[i].pc, dut->commit[i].inst))) {
    // We use the physical register file to get wdata
    proxy->skip_one(dut->commit[i].isRVC, realWen, dut->commit[i].wdest, get_commit_data(i));
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

    proxy->load_flash_bin(get_flash_path(), get_flash_size());
    simMemory->clone_on_demand(
        [this](uint64_t offset, void *src, size_t n) {
          uint64_t dest_addr = PMEM_BASE + offset;
          proxy->ref_memcpy(dest_addr, src, n, DUT_TO_REF);
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
    if (load_event.isLoad || load_event.isAtomic) {
      proxy->sync();
      if (regWen && *refRegPtr != commitData) {
        uint64_t golden;
        int len = 0;
        if (load_event.isLoad) {
          switch (load_event.opType) {
            case 0: len = 1; break;
            case 1: len = 2; break;
            case 2: len = 4; break;
            case 3: len = 8; break;
            case 4: len = 1; break;
            case 5: len = 2; break;
            case 6: len = 4; break;
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
          switch (load_event.opType) {
            case 0: golden = (int64_t)(int8_t)golden; break;
            case 1: golden = (int64_t)(int16_t)golden; break;
            case 2: golden = (int64_t)(int32_t)golden; break;
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
      display();
      printf("Mismatch for store commits \n");
      printf("  REF commits addr 0x%lx, data 0x%lx, mask 0x%x\n", addr, data, mask);
      printf("  DUT commits addr 0x%lx, data 0x%lx, mask 0x%x\n", store_event.addr, store_event.data, store_event.mask);

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
        printf("cacheid=%d,idtfr=%d,realpaddr=0x%lx: Refill test failed!\n", cacheid, dut_refill->idtfr, realpaddr);
        printf("addr: %lx\nGold: ", dut_refill->addr);
        for (int j = 0; j < 8; j++) {
          read_goldenmem(dut_refill->addr + j * 8, &buf, 8);
          printf("%016lx", *((uint64_t *)buf));
        }
        printf("\nCore: ");
        for (int j = 0; j < 8; j++) {
          printf("%016lx", dut_refill->data[j]);
        }
        printf("\n");
        // continue run some cycle before aborted to dump wave
        if (delay == 0) {
          delay = 1;
        }
        return 0;
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
    r_s2.level = 2;
    return r_s2;
  }
  for (level = 0; level < 3; level++) {
    hpaddr = pg_base + GVPNi(gpaddr, level) * sizeof(uint64_t);
    read_goldenmem(hpaddr, &pte.val, 8);
    pg_base = pte.ppn << 12;
    if (!pte.v || pte.r || pte.x || pte.w || level == 2) {
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

    Satp *satp = (Satp *)&dut->l1tlb[i].satp;
    Satp *vsatp = (Satp *)&dut->l1tlb[i].vsatp;
    Hgatp *hgatp = (Hgatp *)&dut->l1tlb[i].hgatp;
    uint8_t hasS2xlate = dut->l1tlb[i].s2xlate != noS2xlate;
    uint8_t onlyS2 = dut->l1tlb[i].s2xlate == onlyStage2;
    uint64_t pg_base = (hasS2xlate ? vsatp->ppn : satp->ppn) << 12;
    if (onlyS2) {
      r_s2 = do_s2xlate(hgatp, dut->l1tlb[i].vpn << 12);
      pte = r_s2.pte;
      difftest_level = r_s2.level;
    } else {
      for (difftest_level = 0; difftest_level < 3; difftest_level++) {
        paddr = pg_base + VPNi(dut->l1tlb[i].vpn, difftest_level) * sizeof(uint64_t);
        if (hasS2xlate) {
          r_s2 = do_s2xlate(hgatp, paddr);
          uint64_t pg_mask = ((1ull << VPNiSHFT(2 - r_s2.level)) - 1);
          pg_base = (r_s2.pte.ppn << 12 & ~pg_mask) | (paddr & pg_mask & ~PAGE_MASK);
          paddr = pg_base | (paddr & PAGE_MASK);
        }
        read_goldenmem(paddr, &pte.val, 8);
        pg_base = pte.ppn << 12;
        if (!pte.v || pte.r || pte.x || pte.w || difftest_level == 2) {
          break;
        }
      }
      if (difftest_level < 2 && pte.v) {
        uint64_t pg_mask = ((1ull << VPNiSHFT(2 - difftest_level)) - 1);
        pg_base = (pte.ppn << 12 & ~pg_mask) | (dut->l1tlb[i].vpn << 12 & pg_mask & ~PAGE_MASK);
      }
      if (hasS2xlate && pte.v) {
        r_s2 = do_s2xlate(hgatp, pg_base);
        pte = r_s2.pte;
        difftest_level = r_s2.level;
      }
    }

    dut->l1tlb[i].ppn = dut->l1tlb[i].ppn >> (2 - difftest_level) * 9 << (2 - difftest_level) * 9;
    if (pte.difftest_ppn != dut->l1tlb[i].ppn) {
      printf("Warning: l1tlb resp test of core %d index %d failed! vpn = %lx\n", id, i, dut->l1tlb[i].vpn);
      printf("  REF commits pte.val: 0x%lx, dut s2xlate: %d\n", pte.val, dut->l1tlb[i].s2xlate);
      printf("  REF commits ppn 0x%lx, DUT commits ppn 0x%lx\n", pte.difftest_ppn, dut->l1tlb[i].ppn);
      printf("  REF commits perm 0x%02x, level %d, pf %d\n", pte.difftest_perm, difftest_level, !pte.difftest_v);
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
        if (onlyS2) {
          r_s2 = do_s2xlate(hgatp, dut->l2tlb[i].vpn << 12);
          uint64_t pg_mask = ((1ull << VPNiSHFT(2 - r_s2.level)) - 1);
          uint64_t s2_pg_base = r_s2.pte.ppn << 12;
          pg_base = (s2_pg_base & ~pg_mask) | (paddr & pg_mask & ~PAGE_MASK);
          paddr = pg_base | (paddr & PAGE_MASK);
        }
        for (difftest_level = 0; difftest_level < 3; difftest_level++) {
          paddr = pg_base + VPNi(dut->l2tlb[i].vpn + j, difftest_level) * sizeof(uint64_t);
          if (hasS2xlate) {
            r_s2 = do_s2xlate(hgatp, paddr);
            uint64_t pg_mask = ((1ull << VPNiSHFT(2 - r_s2.level)) - 1);
            pg_base = (r_s2.pte.ppn << 12 & ~pg_mask) | (paddr & pg_mask & ~PAGE_MASK);
            paddr = pg_base | (paddr & PAGE_MASK);
          }
          read_goldenmem(paddr, &pte.val, 8);
          if (!pte.v || pte.r || pte.x || pte.w || difftest_level == 2) {
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
                             difftest_level != dut->l2tlb[i].level || difftest_pf != dut->l2tlb[i].pf;
        bool s2_check_fail = hasS2xlate ? r_s2.pte.difftest_ppn != dut->l2tlb[i].s2ppn ||
                                              r_s2.pte.difftest_perm != dut->l2tlb[i].g_perm ||
                                              r_s2.level != dut->l2tlb[i].g_level || difftest_gpf != dut->l2tlb[i].gpf
                                        : false;
        if (s1_check_fail || s2_check_fail) {
          printf("Warning: L2TLB resp test of core %d index %d sector %d failed! vpn = %lx\n", id, i, j,
                 dut->l2tlb[i].vpn + j);
          printf("  REF commits ppn 0x%lx, perm 0x%02x, level %d, pf %d\n", pte.difftest_ppn, pte.difftest_perm,
                 difftest_level, difftest_pf);
          if (hasS2xlate)
            printf("      s2_ppn 0x%lx, g_perm 0x%02x, g_level %d, gpf %d\n", r_s2.pte.difftest_ppn,
                   r_s2.pte.difftest_perm, r_s2.level, difftest_gpf);
          printf("  DUT commits ppn 0x%lx, perm 0x%02x, level %d, pf %d\n", dut->l2tlb[i].ppn[j], dut->l2tlb[i].perm,
                 dut->l2tlb[i].level, dut->l2tlb[i].pf);
          if (hasS2xlate)
            printf("      s2_ppn 0x%lx, g_perm 0x%02x, g_level %d, gpf %d\n", dut->l2tlb[i].s2ppn, dut->l2tlb[i].g_perm,
                   dut->l2tlb[i].g_level, dut->l2tlb[i].gpf);
          return 1;
        }
      }
    }
  }
#endif // CONFIG_DIFFTEST_L1TLBEVENT
  return 0;
}

inline int handle_atomic(int coreid, uint64_t atomicAddr, uint64_t atomicData, uint64_t atomicMask, uint8_t atomicFuop,
                         uint64_t atomicOut) {
  // We need to do atmoic operations here so as to update goldenMem
  if (!(atomicMask == 0xf || atomicMask == 0xf0 || atomicMask == 0xff)) {
    printf("Unrecognized mask: %lx\n", atomicMask);
    return 1;
  }

  if (atomicMask == 0xff) {
    uint64_t rs = atomicData; // rs2
    uint64_t t = atomicOut;   // original value
    uint64_t ret;
    uint64_t mem;
    read_goldenmem(atomicAddr, &mem, 8);
    if (mem != t && atomicFuop != 007 && atomicFuop != 003) { // ignore sc_d & lr_d
      printf("Core %d atomic instr mismatch goldenMem, mem: 0x%lx, t: 0x%lx, op: 0x%x, addr: 0x%lx\n", coreid, mem, t,
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
      default: printf("Unknown atomic fuOpType: 0x%x\n", atomicFuop);
    }
    update_goldenmem(atomicAddr, &ret, atomicMask, 8);
  }

  if (atomicMask == 0xf || atomicMask == 0xf0) {
    uint32_t rs = (uint32_t)atomicData; // rs2
    uint32_t t = (uint32_t)atomicOut;   // original value
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
      printf("Core %d atomic instr mismatch goldenMem, rawmem: 0x%lx mem: 0x%x, t: 0x%x, op: 0x%x, addr: 0x%lx\n",
             coreid, mem_raw, mem, t, atomicFuop, atomicAddr);
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
      default: printf("Unknown atomic fuOpType: 0x%x\n", atomicFuop);
    }
    ret_sel = ret;
    if (atomicMask == 0xf0)
      ret_sel = (ret_sel << 32);
    update_goldenmem(atomicAddr, &ret_sel, atomicMask, 8);
  }
  return 0;
}

void dumpGoldenMem(const char *banner, uint64_t addr, uint64_t time) {
#ifdef DEBUG_REFILL
  char buf[512];
  if (addr == 0) {
    return;
  }
  printf("============== %s =============== time = %ld\ndata: ", banner, time);
  for (int i = 0; i < 8; i++) {
    read_goldenmem(addr + i * 8, &buf, 8);
    printf("%016lx", *((uint64_t *)buf));
  }
  printf("\n");
#endif
}

#ifdef DEBUG_GOLDENMEM
int Difftest::do_golden_memory_update() {
  // Update Golden Memory info

  if (ticks == 100) {
    dumpGoldenMem("Init", track_instr, ticks);
  }

#ifdef CONFIG_DIFFTEST_SBUFFEREVENT
  for (int i = 0; i < CONFIG_DIFF_SBUFFER_WIDTH; i++) {
    if (dut->sbuffer[i].valid) {
      dut->sbuffer[i].valid = 0;
      update_goldenmem(dut->sbuffer[i].addr, dut->sbuffer[i].data, dut->sbuffer[i].mask, 64);
      if (dut->sbuffer[i].addr == track_instr) {
        dumpGoldenMem("Store", track_instr, ticks);
      }
    }
  }
#endif // CONFIG_DIFFTEST_SBUFFEREVENT

#ifdef CONFIG_DIFFTEST_ATOMICEVENT
  if (dut->atomic.valid) {
    dut->atomic.valid = 0;
    int ret =
        handle_atomic(id, dut->atomic.addr, dut->atomic.data, dut->atomic.mask, dut->atomic.fuop, dut->atomic.out);
    if (dut->atomic.addr == track_instr) {
      dumpGoldenMem("Atmoic", track_instr, ticks);
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

int Difftest::check_timeout() {
  // check whether there're any commits since the simulation starts
  if (!has_commit && ticks > last_commit + firstCommit_limit) {
    Info("The first instruction of core %d at 0x%lx does not commit after %lu cycles.\n", id, FIRST_INST_ADDRESS,
         firstCommit_limit);
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
  if (has_commit && ticks > last_commit + stuck_limit) {
    Info(
        "No instruction of core %d commits for %lu cycles, maybe get stuck\n"
        "(please also check whether a fence.i instruction requires more than %lu cycles to flush the icache)\n",
        id, stuck_limit, stuck_limit);
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

void Difftest::display() {
  state->display(this->id);

  printf("\n==============  REF Regs  ==============\n");
  fflush(stdout);
  proxy->ref_reg_display();
  printf("privilegeMode: %lu\n", dut->csr.privilegeMode);
}

void CommitTrace::display(bool use_spike) {
  printf("%s pc %016lx inst %08x", get_type(), pc, inst);
  display_custom();
  if (use_spike) {
    printf(" %s", spike_dasm(inst));
  }
}

void Difftest::display_stats() {
  auto trap = get_trap_event();
  uint64_t instrCnt = trap->instrCnt;
  uint64_t cycleCnt = trap->cycleCnt;
  double ipc = (double)instrCnt / cycleCnt;
  eprintf(ANSI_COLOR_MAGENTA "Core-%d instrCnt = %'" PRIu64 ", cycleCnt = %'" PRIu64 ", IPC = %lf\n" ANSI_COLOR_RESET,
          this->id, instrCnt, cycleCnt, ipc);
}

void DiffState::display_commit_count(int i) {
  auto retire_pointer = (retire_group_pointer + DEBUG_GROUP_TRACE_SIZE - 1) % DEBUG_GROUP_TRACE_SIZE;
  printf("commit group [%02d]: pc %010lx cmtcnt %d%s\n", i, retire_group_pc_queue[i], retire_group_cnt_queue[i],
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
  printf("[%02d] ", i);
  commit_trace[i]->display(use_spike);
  auto retire_pointer = (retire_inst_pointer + DEBUG_INST_TRACE_SIZE - 1) % DEBUG_INST_TRACE_SIZE;
  printf("%s\n", (i == retire_pointer) ? " <--" : "");
}

void DiffState::display(int coreid) {
  printf("\n============== Commit Group Trace (Core %d) ==============\n", coreid);
  for (int j = 0; j < DEBUG_GROUP_TRACE_SIZE; j++) {
    display_commit_count(j);
  }

  printf("\n============== Commit Instr Trace ==============\n");
  bool use_spike = spike_valid();
  for (int j = 0; j < DEBUG_INST_TRACE_SIZE; j++) {
    display_commit_instr(j, use_spike);
  }

  fflush(stdout);
}

DiffState::DiffState() : commit_trace(DEBUG_INST_TRACE_SIZE, nullptr) {}
