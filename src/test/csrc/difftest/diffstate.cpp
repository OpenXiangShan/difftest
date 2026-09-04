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

#include "diffstate.h"
#include "spikedasm.h"
#include <algorithm>
#include <cassert>

void CommitTrace::display(bool use_spike) const {
  Info("%s pc %016lx inst %08x", get_type(), pc, inst);
  display_custom();
  if (use_spike) {
    Info(" %s", spike_dasm(inst));
  }
}

void CommitTrace::display_line(int index, bool use_spike, bool is_retire) const {
  Info("[%02d] ", index);
  display(use_spike);
  Info("%s\n", is_retire ? " <--" : "");
}

void DiffState::display() {
  Info("\n============== Commit Group Trace (Core %d) ==============\n", coreid);
  int group_index = 0;
  while (!retire_group_queue.empty()) {
    auto retire_group = retire_group_queue.front();
    auto pc = retire_group.first;
    auto cnt = retire_group.second;
    retire_group_queue.pop();
    Info("commit group [%02d]: pc %010lx cmtcnt %d%s\n", group_index, pc, cnt,
         retire_group_queue.empty() ? " <--" : "");
    group_index++;
  }

  Info("\n============== Commit Instr Trace ==============\n");
  int commit_index = 0;
  while (!commit_trace.empty()) {
    auto trace = commit_trace.front();
    commit_trace.pop();
    trace->display_line(commit_index, use_spike, commit_trace.empty());
    commit_index++;
  }

  fflush(stdout);
}

DiffState::DiffState(int coreid) : use_spike(spike_valid()), coreid(coreid) {}

#ifdef CONFIG_DIFFTEST_REPLAY
void DiffState::replay_snapshot() {
  replay_state.valid = false;
  replay_state.has_commit = has_commit;
  replay_state.last_commit_cycle = last_commit_cycle;
  replay_state.has_trap = has_trap;
  replay_state.trap_code = trap_code;
#ifdef CONFIG_DIFFTEST_ARCHINTDELAYEDUPDATE
  std::copy(delayed_int, delayed_int + 32, replay_state.delayed_int.begin());
#endif // CONFIG_DIFFTEST_ARCHINTDELAYEDUPDATE
#ifdef CONFIG_DIFFTEST_ARCHFPDELAYEDUPDATE
  std::copy(delayed_fp, delayed_fp + 32, replay_state.delayed_fp.begin());
#endif // CONFIG_DIFFTEST_ARCHFPDELAYEDUPDATE
#ifdef CONFIG_DIFFTEST_STOREEVENT
  replay_state.store_event_queue = store_event_queue;
#endif // CONFIG_DIFFTEST_STOREEVENT
#ifdef CONFIG_DIFFTEST_CMOINVALEVENT
  replay_state.cmo_inval_event_set = cmo_inval_event_set;
#endif // CONFIG_DIFFTEST_CMOINVALEVENT
#ifdef CONFIG_DIFFTEST_SQUASH
  replay_state.commit_stamp = commit_stamp;
#ifdef CONFIG_DIFFTEST_LOADEVENT
  replay_state.load_event_queue = load_event_queue;
#endif // CONFIG_DIFFTEST_LOADEVENT
#endif // CONFIG_DIFFTEST_SQUASH
#ifdef CONFIG_DIFFTEST_AMUCTRLEVENT
  replay_state.matrix_sw_rob = matrix_sw_rob;
#endif // CONFIG_DIFFTEST_AMUCTRLEVENT
#ifdef CONFIG_DIFFTEST_MSYNCEVENT
  replay_state.msync_event_queue = msync_event_queue;
#endif // CONFIG_DIFFTEST_MSYNCEVENT
  replay_state.retire_group_queue = retire_group_queue;
  replay_state.commit_trace = commit_trace;
  replay_state.commit_counter = commit_counter;
  replay_state.valid = true;
}

void DiffState::replay_restore() {
  assert(replay_state.valid);
  has_commit = replay_state.has_commit;
  last_commit_cycle = replay_state.last_commit_cycle;
  has_trap = replay_state.has_trap;
  trap_code = replay_state.trap_code;
#ifdef CONFIG_DIFFTEST_ARCHINTDELAYEDUPDATE
  std::copy(replay_state.delayed_int.begin(), replay_state.delayed_int.end(), delayed_int);
#endif // CONFIG_DIFFTEST_ARCHINTDELAYEDUPDATE
#ifdef CONFIG_DIFFTEST_ARCHFPDELAYEDUPDATE
  std::copy(replay_state.delayed_fp.begin(), replay_state.delayed_fp.end(), delayed_fp);
#endif // CONFIG_DIFFTEST_ARCHFPDELAYEDUPDATE
#ifdef CONFIG_DIFFTEST_STOREEVENT
  store_event_queue = replay_state.store_event_queue;
#endif // CONFIG_DIFFTEST_STOREEVENT
#ifdef CONFIG_DIFFTEST_CMOINVALEVENT
  cmo_inval_event_set = replay_state.cmo_inval_event_set;
#endif // CONFIG_DIFFTEST_CMOINVALEVENT
#ifdef CONFIG_DIFFTEST_SQUASH
  commit_stamp = replay_state.commit_stamp;
#ifdef CONFIG_DIFFTEST_LOADEVENT
  load_event_queue = replay_state.load_event_queue;
#endif // CONFIG_DIFFTEST_LOADEVENT
#endif // CONFIG_DIFFTEST_SQUASH
#ifdef CONFIG_DIFFTEST_AMUCTRLEVENT
  matrix_sw_rob = replay_state.matrix_sw_rob;
#endif // CONFIG_DIFFTEST_AMUCTRLEVENT
#ifdef CONFIG_DIFFTEST_MSYNCEVENT
  msync_event_queue = replay_state.msync_event_queue;
#endif // CONFIG_DIFFTEST_MSYNCEVENT
  retire_group_queue = replay_state.retire_group_queue;
  commit_trace = replay_state.commit_trace;
  commit_counter = replay_state.commit_counter;
}
#endif // CONFIG_DIFFTEST_REPLAY

static uint64_t get_int_data(const DiffTestState *state, int index) {
#ifdef CONFIG_DIFFTEST_PHYINTREGSTATE
  return state->pregs_xrf.value[state->commit[index].wpdest];
#else
  return state->regs.xrf.value[state->commit[index].wdest];
#endif // CONFIG_DIFFTEST_PHYINTREGSTATE
}

#ifdef CONFIG_DIFFTEST_ARCHFPREGSTATE
static uint64_t get_fp_data(const DiffTestState *state, int index) {
#if defined(CONFIG_DIFFTEST_PHYFPREGSTATE)
  return state->pregs_frf.value[state->commit[index].wpdest];
#else
  return state->regs.frf.value[state->commit[index].wdest];
#endif // CONFIG_DIFFTEST_PHYFPREGSTATE
}
#endif

uint64_t get_commit_data(const DiffTestState *state, int index) {
#if defined(CONFIG_DIFFTEST_COMMITDATA)
  return state->commit_data[index].data;
#else
#ifdef CONFIG_DIFFTEST_ARCHFPREGSTATE
  if (state->commit[index].fpwen) {
    return get_fp_data(state, index);
  } else
#endif // CONFIG_DIFFTEST_ARCHFPREGSTATE
    return get_int_data(state, index);
#endif // CONFIG_DIFFTEST_COMMITDATA
}
