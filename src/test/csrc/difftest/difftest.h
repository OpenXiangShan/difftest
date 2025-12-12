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

#ifndef __DIFFTEST_H__
#define __DIFFTEST_H__

#include "checker.h"
#include "common.h"
#include "diffstate.h"
#include "difftrace.h"
#include "dut.h"
#include "golden.h"
#include "refproxy.h"
#include <queue>
#include <unordered_set>
#ifdef FUZZING
#include "emu.h"
#endif // FUZZING

#define PAGE_SHIFT 12
#define PAGE_SIZE  (1ul << PAGE_SHIFT)
#define PAGE_MASK  (PAGE_SIZE - 1)

enum retire_inst_type {
  RET_NORMAL = 0,
  RET_INT,
  RET_EXC
};

enum retire_mem_type {
  RET_OTHER = 0,
  RET_LOAD,
  RET_STORE
};

class store_event_t {
public:
  uint64_t addr;
  uint64_t data;
  uint8_t mask;
};

typedef struct {
  uint64_t instrCnt;
  uint64_t cycleCnt;
} WarmupInfo;

class Difftest {
public:
  DiffTestState *dut;

  // Difftest public APIs for testbench
  // Its backend should be cross-platform (NEMU, Spike, ...)
  // Initialize difftest environments
  Difftest(int coreid);
  ~Difftest();
  REF_PROXY *proxy = NULL;
  uint32_t num_commit = 0; // # of commits if made progress
  WarmupInfo warmup_info;
  // Trigger a difftest checking procdure
  int step();
  void update_nemuproxy(int, size_t);
  void init_checkers();
  inline bool get_trap_valid() {
    return dut->trap.hasTrap;
  }
  inline int get_trap_code() {
    if (dut->trap.code > STATE_FUZZ_COND && dut->trap.code < STATE_RUNNING) {
      return STATE_BADTRAP;
    } else {
      return dut->trap.code;
    }
  }

  void display();
  void display_stats();

  void set_trace(const char *name, bool is_read) {
    difftrace = new DiffTrace<DiffTestState>(name, is_read);
  }
  void trace_read() {
    if (difftrace) {
      difftrace->read_next(dut);
    }
  }
  void trace_write(int step) {
    if (difftrace) {
      int zone = 0;
      for (int i = 0; i < step; i++) {
        difftrace->append(diffstate_buffer[id]->get(zone, i));
      }
      zone = (zone + 1) % CONFIG_DIFFTEST_ZONESIZE;
    }
  }

  // Difftest public APIs for dut: called from DPI-C functions (or testbench)
  // These functions generally do nothing but copy the information to core_state.
  inline DifftestTrapEvent *get_trap_event() {
    return &(dut->trap);
  }
  uint64_t *arch_reg(uint8_t src, bool is_fp = false) {
    return
#ifdef CONFIG_DIFFTEST_ARCHFPREGSTATE
        is_fp ? dut->regs.frf.value + src :
#endif
              dut->regs.xrf.value + src;
  }

#ifdef CONFIG_DIFFTEST_ARCHVECREGSTATE
  inline uint64_t *arch_vecreg(uint8_t src) {
    return dut->regs.vrf.value + src;
  }
#endif // CONFIG_DIFFTEST_ARCHVECREGSTATE

  inline DiffTestState *get_dut() {
    return dut;
  }

#ifdef DEBUG_REFILL
  void set_track_instr(uint64_t instr) {
    state->track_instr = instr;
  }
#endif

#ifdef DEBUG_MODE_DIFF
  void debug_mode_copy(uint64_t addr, size_t size, uint32_t data) {
    proxy->debug_mem_sync(addr, &data, size);
  }
#endif

  void set_commit_trace(bool enable) {
    state->dump_commit_trace = enable;
  }

  bool get_commit_trace() {
    return state->dump_commit_trace;
  }

  void warmup_record() {
    auto trap = get_trap_event();
    warmup_info.instrCnt = trap->instrCnt;
    warmup_info.cycleCnt = trap->cycleCnt;
  }
  void warmup_display_stats();

  void set_has_commit() {
    state->has_commit = true;
  }

protected:
  DiffTrace<DiffTestState> *difftrace = nullptr;

  const uint64_t delay_wb_limit = 80;

  int id;

  // For compare the first instr pc of a commit group
  bool pc_mismatch = false;
  uint64_t dut_commit_first_pc = 0;
  uint64_t ref_commit_first_pc = 0;

  DiffState *state = NULL;

#ifdef CONFIG_DIFFTEST_SQUASH
#ifdef CONFIG_DIFFTEST_LOADEVENT
  std::queue<DifftestLoadEvent> load_event_queue;
  void load_event_record();
#endif // CONFIG_DIFFTEST_LOADEVENT
#endif // CONFIG_DIFFTEST_SQUASH

#ifdef CONFIG_DIFFTEST_CMOINVALEVENT
  std::unordered_set<uint64_t> cmo_inval_event_set;
  void cmo_inval_event_record();
#endif

  void update_last_commit() {
    state->last_commit_cycle = get_trap_event()->cycleCnt;
  }

  ArchEventChecker *arch_event_checker = nullptr;
  FirstInstrCommitChecker *first_instr_commit_checker = nullptr;
  InstrCommitChecker *instr_commit_checker[CONFIG_DIFF_COMMIT_WIDTH] = {nullptr};
  TimeoutChecker *timeout_checker = nullptr;
#ifdef CONFIG_DIFFTEST_LRSCEVENT
  LrScChecker *lrsc_checker = nullptr;
#endif // CONFIG_DIFFTEST_LRSCEVENT
#ifdef CONFIG_DIFFTEST_REFILLEVENT
  RefillChecker *refill_checker[CONFIG_DIFF_REFILL_WIDTH] = {nullptr};
#ifdef CONFIG_DIFFTEST_CMOINVALEVENT
  CmoInvalRecorder *cmo_inval_recorder = nullptr;
#endif // CONFIG_DIFFTEST_CMOINVALEVENT
#endif // CONFIG_DIFFTEST_REFILLEVENT
#ifdef CONFIG_DIFFTEST_L1TLBEVENT
  L1TLBChecker *l1tlb_checker[CONFIG_DIFF_L1TLB_WIDTH] = {nullptr};
#endif // CONFIG_DIFFTEST_L1TLBEVENT
#ifdef CONFIG_DIFFTEST_L2TLBEVENT
  L2TLBChecker *l2tlb_checker[CONFIG_DIFF_L2TLB_WIDTH] = {nullptr};
#endif // CONFIG_DIFFTEST_L2TLBEVENT
#ifdef CONFIG_DIFFTEST_NONREGINTERRUPTPENDINGEVENT
  NonRegInterruptPendingChecker *non_reg_interrupt_pending_checker = nullptr;
#endif // CONFIG_DIFFTEST_NONREGINTERRUPTPENDINGEVENT
#ifdef CONFIG_DIFFTEST_MHPMEVENTOVERFLOWEVENT
  MhpmeventOverflowChecker *mhpmevent_overflow_checker = nullptr;
#endif // CONFIG_DIFFTEST_MHPMEVENTOVERFLOWEVENT
#ifdef CONFIG_DIFFTEST_SYNCAIAEVENT
  AiaChecker *aia_checker = nullptr;
#endif // CONFIG_DIFFTEST_SYNCAIAEVENT
#ifdef CONFIG_DIFFTEST_SYNCCUSTOMMFLUSHPWREVENT
  CustomMflushpwrChecker *custom_mflushpwr_checker = nullptr;
#endif // CONFIG_DIFFTEST_SYNCCUSTOMMFLUSHPWREVENT
#ifdef CONFIG_DIFFTEST_CRITICALERROREVENT
  CriticalErrorChecker *critical_error_checker = nullptr;
#endif // CONFIG_DIFFTEST_CRITICALERROREVENT

  GoldenMemoryInit *golden_memory_init_checker = nullptr;
#ifdef CONFIG_DIFFTEST_SBUFFEREVENT
  SbufferChecker *sbuffer_checker[CONFIG_DIFF_SBUFFER_WIDTH] = {nullptr};
#endif // CONFIG_DIFFTEST_SBUFFEREVENT
#ifdef CONFIG_DIFFTEST_UNCACHEMMSTOREEVENT
  UncacheMmStoreChecker *uncache_mm_store_checker[CONFIG_DIFF_UNCACHE_MM_STORE_WIDTH] = {nullptr};
#endif // CONFIG_DIFFTEST_UNCACHEMMSTOREEVENT
#ifdef CONFIG_DIFFTEST_ATOMICEVENT
  AtomicChecker *atomic_checker = nullptr;
#endif // CONFIG_DIFFTEST_ATOMICEVENT
#ifdef CONFIG_DIFFTEST_STOREEVENT
  StoreRecorder *store_recorder[CONFIG_DIFF_STORE_WIDTH] = {nullptr};
  StoreChecker *store_checker = nullptr;
#endif // CONFIG_DIFFTEST_STOREEVENT
#ifdef CONFIG_DIFFTEST_LOADEVENT
  LoadChecker *load_checker[CONFIG_DIFF_LOAD_WIDTH] = {nullptr};
#ifdef CONFIG_DIFFTEST_SQUASH
  LoadSquashChecker *load_squash_checker = nullptr;
#endif // CONFIG_DIFFTEST_SQUASH
#endif // CONFIG_DIFFTEST_LOADEVENT

  int check_all();
#ifdef CONFIG_DIFFTEST_LOADEVENT
  void do_load_check(int index);
  void do_load_check(DifftestLoadEvent &load_event, bool regWen, uint64_t *refRegPtr, uint64_t commitData);
#ifdef CONFIG_DIFFTEST_SQUASH
  void do_load_check_squash();
#endif // CONFIG_DIFFTEST_SQUASH
#ifdef CONFIG_DIFFTEST_ARCHVECREGSTATE
  void do_vec_load_check(DifftestLoadEvent &load_event, uint8_t vecFirstLdest, uint64_t vecCommitData[]);
#endif // CONFIG_DIFFTEST_ARCHVECREGSTATE
#endif // CONFIG_DIFFTEST_LOADEVENT
  int do_golden_memory_update();

  inline bool in_disambiguation_state() {
    static bool was_found = false;
#ifdef FUZZING
    // Only in fuzzing mode
    if (proxy->in_disambiguation_state()) {
      was_found = true;
      dut->trap.hasTrap = 1;
      dut->trap.code = STATE_AMBIGUOUS;
#ifdef FUZZER_LIB
      stats.exit_code = SimExitCode::ambiguous;
#endif // FUZZER_LIB
    }
#endif // FUZZING
    return was_found;
  }

  int update_delayed_writeback();
  int apply_delayed_writeback();

  void raise_trap(int trapCode);
#ifdef CONFIG_DIFFTEST_CRITICALERROREVENT
  void do_raise_critical_error();
#endif

#ifdef CONFIG_DIFFTEST_SYNCCUSTOMMFLUSHPWREVENT
  void do_sync_custom_mflushpwr();
#endif
#ifdef CONFIG_DIFFTEST_REPLAY
  struct {
    bool in_replay = false;
    int trace_head;
    int trace_size;
  } replay_status;

  DiffState *state_ss = NULL;
  uint8_t *proxy_reg_ss = NULL;
  uint64_t squash_csr_buf[4096];
  bool can_replay();
  bool in_replay_range();
  void replay_snapshot();
  void do_replay();
#endif // CONFIG_DIFFTEST_REPLAY
};

extern Difftest **difftest;
int difftest_init(bool enabled, size_t ramsize);

int difftest_nstep(int step, bool enable_diff);
void difftest_switch_zone();
void difftest_set_dut();
int difftest_step();
int difftest_state();
void difftest_finish();

void difftest_trace_read();
void difftest_trace_write(int step);

#ifdef CONFIG_DIFFTEST_SQUASH
extern "C" void set_squash_scope();
extern "C" void difftest_squash_enable(int enable);
#endif // CONFIG_DIFFTEST_SQUASH
#ifdef CONFIG_DIFFTEST_REPLAY
extern "C" void set_replay_scope();
extern "C" void difftest_replay_head(int idx);
#endif // CONFIG_DIFFTEST_REPLAY

#endif
