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

#include "checkers.h"
#include "common.h"
#include "diffstate.h"
#include "difftrace.h"
#include "dut.h"
#include "golden.h"
#include "refproxy.h"
#include <queue>
#include <vector>
#ifdef FUZZING
#include "emu.h"
#endif // FUZZING

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
  // Check whether DiffTest has produced any progress (step)
  // When batch is enabled, the stuck limit should be scaled accordingly
#ifdef CONFIG_DIFFTEST_BATCH
  static const uint64_t stuck_limit = TimeoutChecker::stuck_commit_limit * CONFIG_DIFFTEST_BATCH_SIZE;
#else
  static const uint64_t stuck_limit = TimeoutChecker::stuck_commit_limit;
#endif // CONFIG_DIFFTEST_BATCH

  DiffTestState *dut;
  RefProxy *proxy = NULL;
  WarmupInfo warmup_info;

  // Difftest public APIs for testbench
  // Its backend should be cross-platform (NEMU, Spike, ...)
  // Initialize difftest environments
  Difftest(int coreid);
  void update_nemuproxy(int, size_t);
  void init_checkers();

  ~Difftest();

  // Trigger a difftest checking procdure
  int step();

  inline bool get_trap_valid() {
    return dut->trap.hasTrap || state->has_trap;
  }
  inline int get_trap_code() {
    if (state->has_trap) {
      return state->trap_code;
    } else if (dut->trap.code > STATE_FUZZ_COND && dut->trap.code < STATE_RUNNING) {
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
        difftrace->append(diffstate_buffer[state->coreid]->get(zone, i));
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
  DiffState *state = NULL;
  DiffTrace<DiffTestState> *difftrace = nullptr;

  static const uint64_t delay_wb_limit = 80;
  uint32_t num_commit = 0; // # of commits if made progress

  // For compare the first instr pc of a commit group
  bool pc_mismatch = false;
  uint64_t ref_commit_batch_pc = 0;
  uint64_t dut_commit_batch_pc = 0;

  std::vector<DiffTestChecker *> checkers;
  ArchEventChecker *arch_event_checker = nullptr;
  InstrCommitChecker *instr_commit_checker[CONFIG_DIFF_COMMIT_WIDTH] = {nullptr};
#ifdef CONFIG_DIFFTEST_STOREEVENT
  StoreChecker *store_checker = nullptr;
#endif // CONFIG_DIFFTEST_STOREEVENT
#ifdef CONFIG_DIFFTEST_LOADEVENT
  LoadChecker *load_checker[CONFIG_DIFF_LOAD_WIDTH] = {nullptr};
#ifdef CONFIG_DIFFTEST_SQUASH
  LoadSquashChecker *load_squash_checker = nullptr;
#endif // CONFIG_DIFFTEST_SQUASH
#endif // CONFIG_DIFFTEST_LOADEVENT

  int check_all();

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
