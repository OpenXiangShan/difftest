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

#include "common.h"
#include "difftrace.h"
#include "dut.h"
#include "golden.h"
#include "refproxy.h"
#include <queue>
#include <unordered_set>
#include <vector>
#ifdef FUZZING
#include "emu.h"
#endif // FUZZING

enum {
  ICACHEID,
  DCACHEID,
  PAGECACHEID
};
enum {
  ITLBID,
  LDTLBID,
  STTLBID
};

#define DEBUG_MEM_REGION(v, f) (f <= (DEBUG_MEM_BASE + 0x1000) && f >= DEBUG_MEM_BASE && v)
#define IS_LOAD_STORE(instr)   (((instr & 0x7f) == 0x03) || ((instr & 0x7f) == 0x23))
#define IS_TRIGGERCSR(instr)   (((instr & 0x7f) == 0x73) && ((instr & (0xff0 << 20)) == (0x7a0 << 20)))
#define IS_DEBUGCSR(instr)     (((instr & 0x7f) == 0x73) && ((instr & (0xffe << 20)) == (0x7b0 << 20))) // 7b0 and 7b1
#ifdef DEBUG_MODE_DIFF
#define DEBUG_MODE_SKIP(v, f, instr) DEBUG_MEM_REGION(v, f) && (IS_LOAD_STORE(instr) || IS_TRIGGERCSR(instr))
#else
#define DEBUG_MODE_SKIP(v, f, instr) false
#endif
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

class CommitTrace {
public:
  uint64_t pc;
  uint32_t inst;

  CommitTrace(uint64_t pc, uint32_t inst) : pc(pc), inst(inst) {}
  virtual ~CommitTrace() {}
  virtual const char *get_type() = 0;
  virtual void display(bool use_spike = false);

protected:
  virtual void display_custom() = 0;
};

class InstrTrace : public CommitTrace {
public:
  uint8_t wen;
  uint8_t dest;
  uint64_t data;
  char tag;

  uint16_t robidx;
  uint8_t isLoad;
  uint8_t lqidx;
  uint8_t isStore;
  uint8_t sqidx;

  InstrTrace(uint64_t pc, uint32_t inst, uint8_t wen, uint8_t dest, uint64_t data, uint8_t lqidx, uint8_t sqidx,
             uint16_t robidx, uint8_t isLoad, uint8_t isStore, bool skip = false, bool delayed = false)
      : CommitTrace(pc, inst), robidx(robidx), isLoad(isLoad), lqidx(lqidx), isStore(isStore), sqidx(sqidx), wen(wen),
        dest(dest), data(data), tag(get_tag(skip, delayed)) {}
  virtual inline const char *get_type() {
    return "commit";
  };

protected:
  void display_custom() {
    Info(" wen %d dst %02d data %016lx idx %03x", wen, dest, data, robidx);
    if (isLoad) {
      Info(" (%02x)", lqidx);
    }
    if (isStore) {
      Info(" (%02x)", sqidx);
    }
    if (tag) {
      Info(" (%c)", tag);
    }
  }

private:
  char get_tag(bool skip, bool delayed) {
    char t = '\0';
    if (skip)
      t |= 'S';
    if (delayed)
      t |= 'D';
    return t;
  }
};

class ExceptionTrace : public CommitTrace {
public:
  uint64_t cause;
  ExceptionTrace(uint64_t pc, uint32_t inst, uint64_t cause) : CommitTrace(pc, inst), cause(cause) {}
  virtual inline const char *get_type() {
    return "exception";
  };

protected:
  void display_custom() {
    Info(" cause %016lx", cause);
  }
};

class InterruptTrace : public ExceptionTrace {
public:
  InterruptTrace(uint64_t pc, uint32_t inst, uint64_t cause) : ExceptionTrace(pc, inst, cause) {}
  virtual inline const char *get_type() {
    return "interrupt";
  }
};

class DiffState {
public:
  bool dump_commit_trace = false;

  DiffState();
  void record_group(uint64_t pc, uint64_t count) {
    retire_group_pc_queue[retire_group_pointer] = pc;
    retire_group_cnt_queue[retire_group_pointer] = count;
    retire_group_pointer = (retire_group_pointer + 1) % DEBUG_GROUP_TRACE_SIZE;
  };
  void record_inst(uint64_t pc, uint32_t inst, uint8_t en, uint8_t dest, uint64_t data, bool skip, bool delayed,
                   uint8_t lqidx, uint8_t sqidx, uint16_t robidx, uint8_t isLoad, uint8_t isStore) {
    push_back_trace(new InstrTrace(pc, inst, en, dest, data, lqidx, sqidx, robidx, isLoad, isStore, skip, delayed));
    retire_inst_pointer = (retire_inst_pointer + 1) % DEBUG_INST_TRACE_SIZE;
  };
  void record_exception(uint64_t pc, uint32_t inst, uint64_t cause) {
    push_back_trace(new ExceptionTrace(pc, inst, cause));
    retire_inst_pointer = (retire_inst_pointer + 1) % DEBUG_INST_TRACE_SIZE;
  };
  void record_interrupt(uint64_t pc, uint32_t inst, uint64_t cause) {
    push_back_trace(new InterruptTrace(pc, inst, cause));
    retire_inst_pointer = (retire_inst_pointer + 1) % DEBUG_INST_TRACE_SIZE;
  };
  void display(int coreid);

private:
  const static int DEBUG_GROUP_TRACE_SIZE = 16;
  int retire_group_pointer = 0;
  uint64_t retire_group_pc_queue[DEBUG_GROUP_TRACE_SIZE] = {0};
  uint32_t retire_group_cnt_queue[DEBUG_GROUP_TRACE_SIZE] = {0};

  const static int DEBUG_INST_TRACE_SIZE = 32;
  int retire_inst_pointer = 0;
  std::vector<CommitTrace *> commit_trace;

  void push_back_trace(CommitTrace *trace) {
    if (commit_trace[retire_inst_pointer]) {
      delete commit_trace[retire_inst_pointer];
    }
    commit_trace[retire_inst_pointer] = trace;
    if (dump_commit_trace) {
      display_commit_instr(retire_inst_pointer);
    }
  }
  void display_commit_count(int i);
  void display_commit_instr(int i);
  void display_commit_instr(int i, bool use_spike);
};

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
  bool has_commit = false;
  // Trigger a difftest checking procdure
  int step();
  void update_nemuproxy(int, size_t);
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
        is_fp ? dut->regs_fp.value + src :
#endif
              dut->regs_int.value + src;
  }
  inline DiffTestState *get_dut() {
    return dut;
  }

#ifdef DEBUG_REFILL
  void save_track_instr(uint64_t instr) {
    track_instr = instr;
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

protected:
  DiffTrace<DiffTestState> *difftrace = nullptr;

#ifdef CONFIG_DIFFTEST_BATCH
  static const uint64_t commit_storage = CONFIG_DIFFTEST_BATCH_SIZE;
#else
  static const uint64_t commit_storage = 1;
#endif // CONFIG_DIFFTEST_BATCH
#ifdef CONFIG_DIFFTEST_SQUASH
  static const uint64_t timeout_scale = 256;
#else
  static const uint64_t timeout_scale = 1;
#endif // CONFIG_DIFFTEST_SQUASH
#if defined(CPU_NUTSHELL) || defined(CPU_ROCKET_CHIP)
  static const uint64_t first_commit_limit = 1000;
#elif defined(CPU_XIANGSHAN)
  static const uint64_t first_commit_limit = 15000;
#endif
  static const uint64_t stuck_commit_limit = first_commit_limit * timeout_scale;

public:
  static const uint64_t stuck_limit = stuck_commit_limit * commit_storage;

protected:
  const uint64_t delay_wb_limit = 80;

  int id;

  bool progress = false;
  uint64_t last_commit = 0;

  // For compare the first instr pc of a commit group
  bool pc_mismatch = false;
  uint64_t dut_commit_first_pc = 0;
  uint64_t ref_commit_first_pc = 0;

  uint64_t nemu_this_pc;
  DiffState *state = NULL;
#ifdef DEBUG_REFILL
  uint64_t track_instr = 0;
#endif

#ifdef CONFIG_DIFFTEST_SQUASH
  int commit_stamp = 0;
#ifdef CONFIG_DIFFTEST_LOADEVENT
  std::queue<DifftestLoadEvent> load_event_queue;
  void load_event_record();
#endif // CONFIG_DIFFTEST_LOADEVENT
#endif // CONFIG_DIFFTEST_SQUASH

#ifdef CONFIG_DIFFTEST_STOREEVENT
  std::queue<DifftestStoreEvent> store_event_queue;
  void store_event_record();
#endif

#ifdef CONFIG_DIFFTEST_CMOINVALEVENT
  std::unordered_set<uint64_t> cmo_inval_event_set;
  void cmo_inval_event_record();
#endif

  void update_last_commit() {
    last_commit = get_trap_event()->cycleCnt;
  }
  int check_timeout();
  int check_all();
  void do_first_instr_commit();
  void do_interrupt();
  void do_exception();
  int do_instr_commit(int index);
  void do_load_check(int index);
  int do_store_check();
  int do_refill_check(int cacheid);
  int do_irefill_check();
  int do_drefill_check();
  int do_ptwrefill_check();
  int do_l1tlb_check();
  int do_l2tlb_check();
  int do_golden_memory_update();

  inline uint64_t get_commit_data(int i) {
#ifdef CONFIG_DIFFTEST_COMMITDATA
    return dut->commit_data[i].data;
#else
#ifdef CONFIG_DIFFTEST_ARCHFPREGSTATE
    if (dut->commit[i].fpwen) {
      return
#ifdef CONFIG_DIFFTEST_FPWRITEBACK
          dut->wb_fp[dut->commit[i].wpdest].data;
#else
          dut->regs_fp.value[dut->commit[i].wdest];
#endif // CONFIG_DIFFTEST_FPWRITEBACK
    } else
#endif // CONFIG_DIFFTEST_ARCHFPREGSTATE
#ifdef CONFIG_DIFFTEST_ARCHVECREGSTATE
        if (dut->commit[i].vecwen) {
      return
#ifdef CONFIG_DIFFTEST_VECWRITEBACK
          dut->wb_vec[dut->commit[i].wpdest].data;
#else
          dut->regs_vec.value[dut->commit[i].wdest];
#endif // CONFIG_DIFFTEST_VECWRITEBACK
    } else
#endif // CONFIG_DIFFTEST_ARCHVECREGSTATE
      return
#ifdef CONFIG_DIFFTEST_INTWRITEBACK
          dut->wb_int[dut->commit[i].wpdest].data;
#else
        dut->regs_int.value[dut->commit[i].wdest];
#endif // CONFIG_DIFFTEST_INTWRITEBACK
#endif // CONFIG_DIFFTEST_COMMITDATA
  }
  inline bool has_wfi() {
    return dut->trap.hasWFI;
  }
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

#ifdef CONFIG_DIFFTEST_ARCHINTDELAYEDUPDATE
  int delayed_int[32] = {0};
#endif // CONFIG_DIFFTEST_ARCHINTDELAYEDUPDATE
#ifdef CONFIG_DIFFTEST_ARCHFPDELAYEDUPDATE
  int delayed_fp[32] = {0};
#endif // CONFIG_DIFFTEST_ARCHFPDELAYEDUPDATE
  int update_delayed_writeback();
  int apply_delayed_writeback();

  void raise_trap(int trapCode);
#ifdef CONFIG_DIFFTEST_NONREGINTERRUPTPENDINGEVENT
  void do_non_reg_interrupt_pending();
#endif
#ifdef CONFIG_DIFFTEST_MHPMEVENTOVERFLOWEVENT
  void do_mhpmevent_overflow();
#endif
#ifdef CONFIG_DIFFTEST_CRITICALERROREVENT
  void do_raise_critical_error();
#endif
#ifdef CONFIG_DIFFTEST_SYNCAIAEVENT
  void do_sync_aia();
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
  int proxy_reg_size = 0;
  uint8_t *proxy_reg_ss = NULL;
  uint64_t squash_csr_buf[4096];
  bool can_replay();
  bool in_replay_range();
  void replay_snapshot();
  void do_replay();
#endif // CONFIG_DIFFTEST_REPLAY
};

extern Difftest **difftest;
int difftest_init();

int difftest_nstep(int step, bool enable_diff);
void difftest_switch_zone();
void difftest_set_dut();
int difftest_step();
int difftest_state();
void difftest_finish();

void difftest_trace_read();
void difftest_trace_write(int step);

int init_nemuproxy(size_t);

#ifdef CONFIG_DIFFTEST_SQUASH
extern "C" void set_squash_scope();
extern "C" void difftest_squash_enable(int enable);
#endif // CONFIG_DIFFTEST_SQUASH
#ifdef CONFIG_DIFFTEST_REPLAY
extern "C" void set_replay_scope();
extern "C" void difftest_replay_head(int idx);
#endif // CONFIG_DIFFTEST_REPLAY

#endif
