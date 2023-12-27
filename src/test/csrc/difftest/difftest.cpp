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
#include "goldenmem.h"
#include "ram.h"
#include "flash.h"
#include "spikedasm.h"
#ifdef CONFIG_DIFFTEST_SQUASH
#include "svdpi.h"
#endif // CONFIG_DIFFTEST_SQUASH

Difftest **difftest = NULL;

int difftest_init() {
  diffstate_buffer_init();
  difftest = new Difftest*[NUM_CORES];
  for (int i = 0; i < NUM_CORES; i++) {
    difftest[i] = new Difftest(i);
    difftest[i]->dut = diffstate_buffer[i]->get(0);
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

int difftest_nstep(int step){
  int last_trap_code = STATE_RUNNING;
  for(int i = 0; i < step; i++){
    if(difftest_step()){
      last_trap_code = STATE_ABORT;
      return last_trap_code;
    }
    last_trap_code = difftest_state();
    if(last_trap_code != STATE_RUNNING)
      return last_trap_code;
  }
  return last_trap_code;
}

int difftest_step() {
  for (int i = 0; i < NUM_CORES; i++) {
    difftest[i]->dut = diffstate_buffer[i]->next();
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
  for(int i = 0; i < NUM_CORES; i++) {
    difftest[i]->trace_write(step);
  }
}

void difftest_finish() {
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

#ifdef CONFIG_DIFFTEST_SQUASH_REPLAY
extern "C" void set_squash_replay(int idx);
void difftest_squash_replay(int idx) {
  if (squashScope == NULL) {
    printf("Error: Could not retrieve squash scope, set first\n");
    assert(squashScope);
  }
  svSetScope(squashScope);
  set_squash_replay(idx);
}
#endif // CONFIG_DIFFTEST_SQUASH_REPLAY
#endif // CONFIG_DIFFTEST_SQUASH

Difftest::Difftest(int coreid) : id(coreid) {
  state = new DiffState();
#ifdef CONFIG_DIFFTEST_SQUASH_REPLAY
  state_ss = (DiffState*)malloc(sizeof(DiffState));
#endif // CONFIG_DIFFTEST_SQUASH_REPLAY
}

Difftest::~Difftest() {
  delete state;
  delete difftrace;
  if (proxy) {
    delete proxy;
  }
#ifdef CONFIG_DIFFTEST_SQUASH_REPLAY
  free(state_ss);
  if (proxy_ss) {
    free(proxy_ss);
  }
#endif // CONFIG_DIFFTEST_SQUASH_REPLAY
}

void Difftest::update_nemuproxy(int coreid, size_t ram_size = 0) {
  proxy = new REF_PROXY(coreid, ram_size);
#ifdef CONFIG_DIFFTEST_SQUASH_REPLAY
  proxy_ss = (REF_PROXY*)malloc(sizeof(REF_PROXY));
  squash_memsize = ram_size;
  squash_membuf = (char *)malloc(squash_memsize);
#endif // CONFIG_DIFFTEST_SQUASH_REPLAY
}

#ifdef CONFIG_DIFFTEST_SQUASH_REPLAY
bool Difftest::squash_check() {
  int nFused_all = 0;
  for (int i = 0; i < CONFIG_DIFF_COMMIT_WIDTH && dut->commit[i].valid; i++) {
    nFused_all += dut->commit[i].nFused;
  }
  return nFused_all != 0;
}

void Difftest::squash_snapshot() {
  memcpy(state_ss, state, sizeof(DiffState));
  memcpy(proxy_ss, proxy, sizeof(REF_PROXY));
  proxy->ref_csrcpy(squash_csr_buf, REF_TO_DUT);
  proxy->ref_memcpy(PMEM_BASE, squash_membuf, squash_memsize, REF_TO_DUT);
}

void Difftest::squash_replay() {
  inReplay = true;
  replay_idx = squash_idx;
  memcpy(state, state_ss, sizeof(DiffState));
  memcpy(proxy, proxy_ss, sizeof(REF_PROXY));
  proxy->ref_regcpy(&proxy->regs_int, DUT_TO_REF, false);
  proxy->ref_csrcpy(squash_csr_buf, DUT_TO_REF);
  proxy->ref_memcpy(PMEM_BASE, squash_membuf, squash_memsize, DUT_TO_REF);
  difftest_squash_replay(replay_idx);
}
#endif // CONFIG_DIFFTEST_SQUASH_REPLAY

int Difftest::step() {
  progress = false;
  ticks++;

#ifdef CONFIG_DIFFTEST_SQUASH_REPLAY
  squash_idx = dut->trace_info.squash_idx;
  if (inReplay && squash_idx != replay_idx) {
    return 0;
  }
  isSquash = squash_check();
  if (isSquash) {
    squash_snapshot();
  }
#endif // CONFIG_DIFFTEST_SQUASH_REPLAY

  if (check_timeout()) {
    return 1;
  }
  do_first_instr_commit();
  if (do_store_check()) {
    return 1;
  }

#ifdef DEBUG_GOLDENMEM
  if (do_golden_memory_update()) {
    return 1;
  }
#endif

  if (!has_commit) {
    return 0;
  }

#ifdef DEBUG_REFILL
  if (do_irefill_check() || do_drefill_check() || do_ptwrefill_check() ) {
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
  for(int i = 0; i < DIFFTEST_COMMIT_WIDTH; i++){
    if(DEBUG_MEM_REGION(dut->commit[i].valid, dut->commit[i].pc))
      debug_mode_copy(dut->commit[i].pc, dut->commit[i].isRVC ? 2 : 4, dut->commit[i].inst);
  }

#endif

#ifdef CONFIG_DIFFTEST_LRSCEVENT
  // sync lr/sc reg microarchitectural status to the REF
  if (dut->lrsc.valid) {
    dut->lrsc.valid = 0;
    struct SyncState sync;
    sync.sc_fail = !dut->lrsc.success;
    proxy->uarchstatus_sync((uint64_t*)&sync);
  }
#endif

  num_commit = 0; // reset num_commit this cycle to 0
  if (dut->event.valid) {
  // interrupt has a higher priority than exception
    dut->event.interrupt ? do_interrupt() : do_exception();
    dut->event.valid = 0;
    dut->commit[0].valid = 0;
  } else {
    for (int i = 0; i < CONFIG_DIFF_COMMIT_WIDTH && dut->commit[i].valid; i++) {
      if (do_instr_commit(i)) {
        return 1;
      }
      dut->commit[i].valid = 0;
      num_commit += 1 + dut->commit[i].nFused;
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
  if (dut->csr.mip != proxy->csr.mip) {  // Ignore difftest for MIP
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
#ifdef CONFIG_DIFFTEST_SQUASH_REPLAY
    if (isSquash) {
      squash_replay();
      return 0;
    }
#endif // CONFIG_DIFFTEST_SQUASH_REPLAY
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
  if (dut->event.exception == 12 || dut->event.exception == 13 || dut->event.exception == 15) {
    struct ExecutionGuide guide;
    guide.force_raise_exception = true;
    guide.exception_num = dut->event.exception;
    guide.mtval = dut->csr.mtval;
    guide.stval = dut->csr.stval;
    guide.force_set_jump_target = false;
    proxy->guided_exec(guide);
  } else {
  #ifdef DEBUG_MODE_DIFF
    if(DEBUG_MEM_REGION(true, dut->event.exceptionPC)){
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
    dut->commit[i].lqIdx, dut->commit[i].sqIdx, dut->commit[i].robIdx, dut->commit[i].isLoad, dut->commit[i].isStore);

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
      dut->commit[i].rfwen ? delayed_int:
#endif // CONFIG_DIFFTEST_ARCHINTDELAYEDUPDATE
#ifdef CONFIG_DIFFTEST_ARCHFPDELAYEDUPDATE
      dut->commit[i].fpwen ? delayed_fp:
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
  int spike_invalid = test_spike();
  if (!spike_invalid && (IS_DEBUGCSR(commit_instr) || IS_TRIGGERCSR(commit_instr))) {
    char inst_str[32];
    char dasm_result[64] = {0};
    sprintf(inst_str, "%08x", commit_instr);
    spike_dasm(dasm_result, inst_str);
    Info("s0 is %016lx ", dut->regs.gpr[8]);
    Info("pc is %lx %s\n", commit_pc, dasm_result);
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

  // single step exec
  proxy->ref_exec(1);
  // when there's a fused instruction, let proxy execute more instructions.
  for (int j = 0; j < dut->commit[i].nFused; j++) {
    proxy->ref_exec(1);
  }

  // Handle load instruction carefully for SMP
  if (NUM_CORES > 1) {
#ifdef CONFIG_DIFFTEST_LOADEVENT
    if (dut->load[i].fuType == 0xC || dut->load[i].fuType == 0xF) {
      proxy->sync();
      bool reg_cmp_fail = !dut->commit[i].vecwen && *proxy->arch_reg(dut->commit[i].wdest, dut->commit[i].fpwen) != get_commit_data(i);
      if (realWen && reg_cmp_fail) {
        // printf("---[DIFF Core%d] This load instruction gets rectified!\n", this->id);
        // printf("---    ltype: 0x%x paddr: 0x%lx wen: 0x%x wdst: 0x%x wdata: 0x%lx pc: 0x%lx\n", dut->load[i].opType, dut->load[i].paddr, dut->commit[i].wen, dut->commit[i].wdest, get_commit_data(i), dut->commit[i].pc);
        uint64_t golden;
        int len = 0;
        if (dut->load[i].fuType == 0xC) {
          switch (dut->load[i].opType) {
            case 0: len = 1; break;
            case 1: len = 2; break;
            case 2: len = 4; break;
            case 3: len = 8; break;
            case 4: len = 1; break;
            case 5: len = 2; break;
            case 6: len = 4; break;
            default:
              Info("Unknown fuOpType: 0x%x\n", dut->load[i].opType);
          }
        } else {  // dut->load[i].fuType == 0xF
          if (dut->load[i].opType % 2 == 0) {
            len = 4;
          } else {  // dut->load[i].opType % 2 == 1
            len = 8;
          }
        }
        read_goldenmem(dut->load[i].paddr, &golden, len);
        if (dut->load[i].fuType == 0xC) {
          switch (dut->load[i].opType) {
            case 0: golden = (int64_t)(int8_t)golden; break;
            case 1: golden = (int64_t)(int16_t)golden; break;
            case 2: golden = (int64_t)(int32_t)golden; break;
          }
        }
        // printf("---    golden: 0x%lx  original: 0x%lx\n", golden, ref_regs_ptr[dut->commit[i].wdest]);
        if (golden == get_commit_data(i)) {
          proxy->ref_memcpy(dut->load[i].paddr, &golden, len, DUT_TO_REF);
          if (realWen) {
            if (!dut->commit[i].vecwen) {
              *proxy->arch_reg(dut->commit[i].wdest, dut->commit[i].fpwen) = get_commit_data(i);
            }
            proxy->sync(true);
          }
        } else if (dut->load[i].fuType == 0xF) {  //  atomic instr carefully handled
          proxy->ref_memcpy(dut->load[i].paddr, &golden, len, DUT_TO_REF);
          if (realWen) {
            if (!dut->commit[i].vecwen) {
              *proxy->arch_reg(dut->commit[i].wdest, dut->commit[i].fpwen) = get_commit_data(i);
            }
            proxy->sync(true);
          }
        } else {
#ifdef DEBUG_SMP
          // goldenmem check failed as well, raise error
          Info("---  SMP difftest mismatch!\n");
          Info("---  Trying to probe local data of another core\n");
          uint64_t buf;
          difftest[(NUM_CORES-1) - this->id]->proxy->memcpy(dut->load[i].paddr, &buf, len, DIFFTEST_TO_DUT);
          Info("---    content: %lx\n", buf);
#else
          proxy->ref_memcpy(dut->load[i].paddr, &golden, len, DUT_TO_REF);
          if (realWen) {
            if (!dut->commit[i].vecwen) {
              *proxy->arch_reg(dut->commit[i].wdest, dut->commit[i].fpwen) = get_commit_data(i);
            }
            proxy->sync(true);
          }
#endif
        }
      }
    }
#endif // CONFIG_DIFFTEST_LOADEVENT
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
    simMemory->clone_on_demand([this](uint64_t offset, void *src, size_t n) {
      uint64_t dest_addr = PMEM_BASE + offset;
      proxy->ref_memcpy(dest_addr, src, n, DUT_TO_REF);
    }, true);
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

int Difftest::do_store_check() {
#ifdef CONFIG_DIFFTEST_STOREEVENT
  for (int i = 0; i < CONFIG_DIFF_STORE_WIDTH; i++) {
    if (!dut->store[i].valid) {
      return 0;
    }
    auto addr = dut->store[i].addr;
    auto data = dut->store[i].data;
    auto mask = dut->store[i].mask;
    if (proxy->store_commit(&addr, &data, &mask)) {
#ifdef FUZZING
      if (in_disambiguation_state()) {
        Info("Store mismatch detected with a disambiguation state at pc = 0x%lx.\n", dut->trap.pc);
        return 0;
      }
#endif
      display();
      printf("Mismatch for store commits %d: \n", i);
      printf("  REF commits addr 0x%lx, data 0x%lx, mask 0x%x\n", addr, data, mask);
      printf("  DUT commits addr 0x%lx, data 0x%lx, mask 0x%x\n",
        dut->store[i].addr, dut->store[i].data, dut->store[i].mask);
      return 1;
    }
    dut->store[i].valid = 0;
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
  if (delay > 16) { return 1; }
  static uint64_t last_valid_addr = 0;
  char buf[512];
  uint64_t realpaddr = dut_refill->addr;
  dut_refill->addr = dut_refill->addr - dut_refill->addr % 64;
  if (dut_refill->addr != last_valid_addr) {
    last_valid_addr = dut_refill->addr;
    if(!in_pmem(dut_refill->addr)){
      // speculated illegal mem access should be ignored
      return 0;
    }
    for (int i = 0; i < 8; i++) {
      read_goldenmem(dut_refill->addr + i*8, &buf, 8);
      if (dut_refill->data[i] != *((uint64_t*)buf)) {
        printf("cacheid=%d,idtfr=%d,realpaddr=0x%lx: Refill test failed!\n",cacheid, dut_refill->idtfr, realpaddr);
        printf("addr: %lx\nGold: ", dut_refill->addr);
        for (int j = 0; j < 8; j++) {
          read_goldenmem(dut_refill->addr + j*8, &buf, 8);
          printf("%016lx", *((uint64_t*)buf));
        }
        printf("\nCore: ");
        for (int j = 0; j < 8; j++) {
          printf("%016lx", dut_refill->data[j]);
        }
        printf("\n");
        // continue run some cycle before aborted to dump wave
        if (delay == 0) { delay = 1; }
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

    uint64_t pg_base = dut->l1tlb[i].satp << 12;
    for (difftest_level = 0; difftest_level < 3; difftest_level++) {
      paddr = pg_base + VPNi(dut->l1tlb[i].vpn, difftest_level) * sizeof(uint64_t);
      read_goldenmem(paddr, &pte.val, 8);
      if (!pte.v || pte.r || pte.x || pte.w || difftest_level == 2) {
        break;
      }
      pg_base = pte.ppn << 12;
    }

    dut->l1tlb[i].ppn = dut->l1tlb[i].ppn >> (2 - difftest_level) * 9 << (2 - difftest_level) * 9;
    if (pte.difftest_ppn != dut->l1tlb[i].ppn ) {
      printf("Warning: l1tlb resp test of core %d index %d failed! vpn = %lx\n", id, i, dut->l1tlb[i].vpn);
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
    for (int j = 0; j < 8; j++) {
      if (dut->l2tlb[i].valididx[j]) {
        PTE pte;
        uint64_t pg_base = dut->l2tlb[i].satp << 12;
        uint64_t paddr;
        uint8_t difftest_level;

        for (difftest_level = 0; difftest_level < 3; difftest_level++) {
          paddr = pg_base + VPNi(dut->l2tlb[i].vpn + j, difftest_level) * sizeof(uint64_t);
          read_goldenmem(paddr, &pte.val, 8);
          if (!pte.v || pte.r || pte.x || pte.w || difftest_level == 2) {
            break;
          }
          pg_base = pte.ppn << 12;
        }

        bool difftest_pf = !pte.v || (!pte.r && pte.w);
        if (pte.difftest_ppn != dut->l2tlb[i].ppn[j] || pte.difftest_perm != dut->l2tlb[i].perm || difftest_level != dut->l2tlb[i].level || difftest_pf != dut->l2tlb[i].pf) {
          printf("Warning: L2TLB resp test of core %d index %d sector %d failed! vpn = %lx\n", id, i, j, dut->l2tlb[i].vpn + j);
          printf("  REF commits ppn 0x%lx, perm 0x%02x, level %d, pf %d\n", pte.difftest_ppn, pte.difftest_perm, difftest_level, !pte.difftest_v);
          printf("  DUT commits ppn 0x%lx, perm 0x%02x, level %d, pf %d\n", dut->l2tlb[i].ppn[j], dut->l2tlb[i].perm, dut->l2tlb[i].level, dut->l2tlb[i].pf);
          return 0;
        }
      }
    }
  }
#endif // CONFIG_DIFFTEST_L1TLBEVENT
  return 0;
}

inline int handle_atomic(int coreid, uint64_t atomicAddr, uint64_t atomicData, uint64_t atomicMask, uint8_t atomicFuop, uint64_t atomicOut) {
  // We need to do atmoic operations here so as to update goldenMem
  if (!(atomicMask == 0xf || atomicMask == 0xf0 || atomicMask == 0xff)) {
    printf("Unrecognized mask: %lx\n", atomicMask);
    return 1;
  }

  if (atomicMask == 0xff) {
    uint64_t rs = atomicData;  // rs2
    uint64_t t  = atomicOut;   // original value
    uint64_t ret;
    uint64_t mem;
    read_goldenmem(atomicAddr, &mem, 8);
    if (mem != t && atomicFuop != 007 && atomicFuop != 003) {  // ignore sc_d & lr_d
      printf("Core %d atomic instr mismatch goldenMem, mem: 0x%lx, t: 0x%lx, op: 0x%x, addr: 0x%lx\n", coreid, mem, t, atomicFuop, atomicAddr);
      return 1;
    }
    switch (atomicFuop) {
      case 002: case 003: ret = t; break;
      // if sc fails(aka atomicOut == 1), no update to goldenmem
      case 006: case 007: if (t == 1) return 0; ret = rs; break;
      case 012: case 013: ret = rs; break;
      case 016: case 017: ret = t+rs; break;
      case 022: case 023: ret = (t^rs); break;
      case 026: case 027: ret = t & rs; break;
      case 032: case 033: ret = t | rs; break;
      case 036: case 037: ret = ((int64_t)t < (int64_t)rs)? t : rs; break;
      case 042: case 043: ret = ((int64_t)t > (int64_t)rs)? t : rs; break;
      case 046: case 047: ret = (t < rs) ? t : rs; break;
      case 052: case 053: ret = (t > rs) ? t : rs; break;
      default: printf("Unknown atomic fuOpType: 0x%x\n", atomicFuop);
    }
    update_goldenmem(atomicAddr, &ret, atomicMask, 8);
  }

  if (atomicMask == 0xf || atomicMask == 0xf0) {
    uint32_t rs = (uint32_t)atomicData;  // rs2
    uint32_t t  = (uint32_t)atomicOut;   // original value
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

    if (mem != t && atomicFuop != 006 && atomicFuop != 002) {  // ignore sc_w & lr_w
      printf("Core %d atomic instr mismatch goldenMem, rawmem: 0x%lx mem: 0x%x, t: 0x%x, op: 0x%x, addr: 0x%lx\n", coreid, mem_raw, mem, t, atomicFuop, atomicAddr);
      return 1;
    }
    switch (atomicFuop) {
      case 002: case 003: ret = t; break;
      // if sc fails(aka atomicOut == 1), no update to goldenmem
      case 006: case 007: if (t == 1) return 0; ret = rs; break;
      case 012: case 013: ret = rs; break;
      case 016: case 017: ret = t+rs; break;
      case 022: case 023: ret = (t^rs); break;
      case 026: case 027: ret = t & rs; break;
      case 032: case 033: ret = t | rs; break;
      case 036: case 037: ret = ((int32_t)t < (int32_t)rs)? t : rs; break;
      case 042: case 043: ret = ((int32_t)t > (int32_t)rs)? t : rs; break;
      case 046: case 047: ret = (t < rs) ? t : rs; break;
      case 052: case 053: ret = (t > rs) ? t : rs; break;
      default: printf("Unknown atomic fuOpType: 0x%x\n", atomicFuop);
    }
    ret_sel = ret;
    if (atomicMask == 0xf0)
      ret_sel = (ret_sel << 32);
    update_goldenmem(atomicAddr, &ret_sel, atomicMask, 8);
  }
  return 0;
}

void dumpGoldenMem(const char* banner, uint64_t addr, uint64_t time) {
#ifdef DEBUG_REFILL
  char buf[512];
  if (addr == 0) {
    return;
  }
  printf("============== %s =============== time = %ld\ndata: ", banner, time);
    for (int i = 0; i < 8; i++) {
      read_goldenmem(addr + i*8, &buf, 8);
      printf("%016lx", *((uint64_t*)buf));
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
  for(int i = 0; i < CONFIG_DIFF_SBUFFER_WIDTH; i++){
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
    int ret = handle_atomic(id, dut->atomic.addr, dut->atomic.data, dut->atomic.mask, dut->atomic.fuop, dut->atomic.out);
    if (dut->atomic.addr == track_instr) {
      dumpGoldenMem("Atmoic", track_instr, ticks);
    }
    if (ret) return ret;
  }
#endif // CONFIG_DIFFTEST_ATOMICEVENT

  return 0;
}
#endif

int Difftest::check_timeout() {
  // check whether there're any commits since the simulation starts
  if (!has_commit && ticks > last_commit + firstCommit_limit) {
    Info("The first instruction of core %d at 0x%lx does not commit after %lu cycles.\n",
      id, FIRST_INST_ADDRESS, firstCommit_limit);
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
    Info("No instruction of core %d commits for %lu cycles, maybe get stuck\n"
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
#define CHECK_DELAYED_WB(wb, delayed, n, regs_name)                    \
  do {                                                                 \
    for (int i = 0; i < n; i++) {                                      \
      auto delay = dut->wb + i;                                         \
      if (delay->valid) {                                              \
        delay->valid = false;                                          \
        if (!delayed[delay->address]) {                                \
          display();                                                   \
          Info("Delayed writeback at %s has already been committed\n", \
            regs_name[delay->address]);                                \
          raise_trap(STATE_ABORT);                                     \
          return 1;                                                    \
        }                                                              \
        if (delay->nack ) {                                            \
          if (delayed[delay->address] > delay_wb_limit) {              \
            delayed[delay->address] -= 1;                              \
          }                                                            \
        }                                                              \
        else {                                                         \
          delayed[delay->address] = 0;                                 \
        }                                                              \
        progress = true;                                               \
      }                                                                \
    }                                                                  \
  } while(0);

#ifdef CONFIG_DIFFTEST_ARCHINTDELAYEDUPDATE
  CHECK_DELAYED_WB(regs_int_delayed, delayed_int, CONFIG_DIFF_REGS_INT_DELAYED_WIDTH, regs_name_int)
#endif // CONFIG_DIFFTEST_ARCHINTDELAYEDUPDATE
#ifdef CONFIG_DIFFTEST_ARCHFPDELAYEDUPDATE
  CHECK_DELAYED_WB(regs_fp_delayed, delayed_fp, CONFIG_DIFF_REGS_FP_DELAYED_WIDTH, regs_name_fp)
#endif // CONFIG_DIFFTEST_ARCHFPDELAYEDUPDATE
  return 0;
}

int Difftest::apply_delayed_writeback() {
#define APPLY_DELAYED_WB(delayed, regs, regs_name)                             \
  do {                                                                         \
    static const int m = delay_wb_limit;                                       \
    for (int i = 0; i < 32; i++) {                                             \
      if (delayed[i]) {                                                        \
        if (delayed[i] > m) {                                                  \
          display();                                                           \
          Info("%s is delayed for more than %d cycles.\n", regs_name[i], m);   \
          raise_trap(STATE_ABORT);                                             \
          return 1;                                                            \
        }                                                                      \
        delayed[i]++;                                                          \
        dut->regs.value[i] = proxy->regs.value[i];                              \
      }                                                                        \
    }                                                                          \
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
  printf("priviledgeMode: %lu\n", dut->csr.priviledgeMode);
}

void CommitTrace::display(bool use_spike) {
  printf("%s pc %016lx inst %08x", get_type(), pc, inst);
  display_custom();
  if (use_spike) {
    char inst_str[9];
    char dasm_result[32] = {0};
    sprintf(inst_str, "%x", inst);
    spike_dasm(dasm_result, inst_str);
    printf(" %s", dasm_result);
  }
}

void DiffState::display_commit_count(int i) {
  auto retire_pointer = (retire_group_pointer + DEBUG_GROUP_TRACE_SIZE - 1) % DEBUG_GROUP_TRACE_SIZE;
  printf("commit group [%02d]: pc %010lx cmtcnt %d%s\n",
      i, retire_group_pc_queue[i], retire_group_cnt_queue[i],
      (i == retire_pointer)?" <--" : "");
}

void DiffState::display_commit_instr(int i, bool spike_invalid) {
  if (!commit_trace[i]) {
    return;
  }
  printf("[%02d] ", i);
  commit_trace[i]->display(!spike_invalid);
  auto retire_pointer = (retire_inst_pointer + DEBUG_INST_TRACE_SIZE - 1) % DEBUG_INST_TRACE_SIZE;
  printf("%s\n", (i == retire_pointer)?" <--" : "");
}

void DiffState::display(int coreid) {
  int spike_invalid = test_spike();

  printf("\n============== Commit Group Trace (Core %d) ==============\n", coreid);
  for (int j = 0; j < DEBUG_GROUP_TRACE_SIZE; j++) {
    display_commit_count(j);
  }

  printf("\n============== Commit Instr Trace ==============\n");
  for (int j = 0; j < DEBUG_INST_TRACE_SIZE; j++) {
    display_commit_instr(j, spike_invalid);
  }

  fflush(stdout);
}

DiffState::DiffState() : commit_trace(DEBUG_INST_TRACE_SIZE, nullptr) {

}
