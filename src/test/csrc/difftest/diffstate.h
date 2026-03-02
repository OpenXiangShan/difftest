/***************************************************************************************
* Copyright (c) 2020-2025 Institute of Computing Technology, Chinese Academy of Sciences
* Copyright (c) 2025 Beijing Institute of Open Source Chip
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

#ifndef __DIFFSTATE_H__
#define __DIFFSTATE_H__

#include "common.h"
#include <queue>
#include <unordered_set>

class CommitTrace {
public:
  uint64_t pc;
  uint32_t inst;

  CommitTrace(uint64_t pc, uint32_t inst) : pc(pc), inst(inst) {}
  virtual ~CommitTrace() {}
  virtual const char *get_type() = 0;
  virtual void display(bool use_spike = false);
  void display_line(int index, bool use_spike, bool is_retire);

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
  int coreid;
  uint64_t cycle_count = 0;
  bool has_progress = false;
  bool has_commit = false;
  uint64_t last_commit_cycle = 0;
  bool has_trap = false;
  uint64_t trap_code = 0;

#ifdef CONFIG_DIFFTEST_ARCHINTDELAYEDUPDATE
  int delayed_int[32] = {0};
#endif // CONFIG_DIFFTEST_ARCHINTDELAYEDUPDATE
#ifdef CONFIG_DIFFTEST_ARCHFPDELAYEDUPDATE
  int delayed_fp[32] = {0};
#endif // CONFIG_DIFFTEST_ARCHFPDELAYEDUPDATE

#ifdef CONFIG_DIFFTEST_STOREEVENT
  std::queue<DifftestStoreEvent> store_event_queue;
#endif // CONFIG_DIFFTEST_STOREEVENT

#ifdef CONFIG_DIFFTEST_CMOINVALEVENT
  std::unordered_set<uint64_t> cmo_inval_event_set;
#endif

#ifdef CONFIG_DIFFTEST_SQUASH
  int commit_stamp = 0;
#ifdef CONFIG_DIFFTEST_LOADEVENT
  std::queue<DifftestLoadEvent> load_event_queue;
#endif // CONFIG_DIFFTEST_LOADEVENT
#endif // CONFIG_DIFFTEST_SQUASH

#ifdef DEBUG_REFILL
  uint64_t track_instr = 0;
#endif // DEBUG_REFILL

  bool dump_commit_trace = false;

  DiffState(int coreid);
  ~DiffState() {
    while (!commit_trace.empty()) {
      delete commit_trace.front();
      commit_trace.pop();
    }
  }

  void record_group(uint64_t pc, uint32_t count) {
    if (retire_group_queue.size() >= DEBUG_GROUP_TRACE_SIZE) {
      retire_group_queue.pop();
    }
    retire_group_queue.push(std::make_pair(pc, count));
  }
  void record_inst(uint64_t pc, uint32_t inst, uint8_t en, uint8_t dest, uint64_t data, bool skip, bool delayed,
                   uint8_t lqidx, uint8_t sqidx, uint16_t robidx, uint8_t isLoad, uint8_t isStore) {
    push_back_trace(new InstrTrace(pc, inst, en, dest, data, lqidx, sqidx, robidx, isLoad, isStore, skip, delayed));
  };
  void record_exception(uint64_t pc, uint32_t inst, uint64_t cause) {
    push_back_trace(new ExceptionTrace(pc, inst, cause));
  };
  void record_interrupt(uint64_t pc, uint32_t inst, uint64_t cause) {
    push_back_trace(new InterruptTrace(pc, inst, cause));
  };
  void display();

  void raise_trap(int code) {
    has_trap = 1;
    trap_code = code;
  }

private:
  const bool use_spike;

  static const int DEBUG_GROUP_TRACE_SIZE = 16;
  std::queue<std::pair<uint64_t, uint32_t>> retire_group_queue;

  static const int DEBUG_INST_TRACE_SIZE = 32;
  std::queue<CommitTrace *> commit_trace;

  uint64_t commit_counter = 0;
  void push_back_trace(CommitTrace *trace) {
    if (commit_trace.size() >= DEBUG_INST_TRACE_SIZE) {
      delete commit_trace.front();
      commit_trace.pop();
    }
    commit_trace.push(trace);
    if (dump_commit_trace) {
      // Traces from multiple cores may mix together. Use coreid to distinguish them.
      if (NUM_CORES > 1) {
        printf("[%d]", coreid);
      }
      trace->display_line(commit_counter, use_spike, false);
      commit_counter++;
      fflush(stdout);
    }
  }
};

extern uint64_t get_commit_data(const DiffTestState *state, int index);

#endif // __DIFFSTATE_H__
