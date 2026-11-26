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

void CommitTrace::display(bool use_spike) {
  Info("%s pc %016lx inst %08x", get_type(), pc, inst);
  display_custom();
  if (use_spike) {
    Info(" %s", spike_dasm(inst));
  }
}

void CommitTrace::display_line(int index, bool use_spike, bool is_retire) {
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
    CommitTrace *trace = commit_trace.front();
    commit_trace.pop();
    trace->display_line(commit_index, use_spike, commit_trace.empty());
    commit_index++;
  }

  fflush(stdout);
}

DiffState::DiffState(int coreid) : use_spike(spike_valid()), coreid(coreid) {}

#ifdef CONFIG_DIFFTEST_AMUCTRLEVENT
bool DiffState::record_cpu_commit(const DifftestInstrCommit &probe) {
  if (!probe.valid) return true;
#if defined(CONFIG_DIFFTEST_SQUASH)
  // A squashed lane retains only its last PC/id. It does not describe a
  // contiguous interval of that lane's original instructions. Never invent
  // the missing per-instruction identities; AMU admission below fails closed.
  return true;
#elif defined(CONFIG_DIFFTEST_REPLAY) || defined(BASIC_DIFFTEST_ONLY)
  fprintf(stderr, "AMU CPU retirement attribution requires cold, raw, non-replay commits: core=%d\n", coreid);
  return false;
#else
  const uint64_t count = uint64_t(probe.nFused) + 1;
  if (cpu_retire_id > UINT64_MAX - count || probe.retireId != cpu_retire_id + count) {
    fprintf(stderr, "CPU retirement identity discontinuity: core=%d pc=0x%016lx "
      "last=%lu count=%lu observed=%lu\n", coreid, probe.pc,
      cpu_retire_id, count, probe.retireId);
    return false;
  }

  // Classify already-committed instructions using the canonical
  // MatrixDecoder/rocket Instructions classes, not AMU metadata. Configuration
  // and fences do not deliver AMU controls; MINIT advances the CPU-owned epoch.
  int op = -1;
  if ((probe.instr & 0x7c7fU) == 0x102bU) {
    switch (probe.instr >> 25) {
      case 0x00: case 0x08: case 0x10: case 0x20: case 0x28: case 0x30:
      case 0x01: case 0x09: case 0x11: case 0x21: case 0x29: case 0x31:
      case 0x18: case 0x58: // MatrixDecoder's scale-load BitPats.
        op = 1;
        break;
      case 0x1c: case 0x5c: // Explicit dynamic candidates, rd=tr0..3 only.
        if (((probe.instr >> 7) & 31U) < 4) op = 1;
        break;
      case 0x40: case 0x48: case 0x50: case 0x51:
        if (!(probe.instr & 0x01f00000U)) op = 1; // Whole loads / C store, rs2=0.
        break;
    }
  }
  if ((probe.instr & 0xff8c7c7fU) == 0x202bU) op = 0; // MMACC.
  if ((probe.instr & 0xfffffc7fU) == 0x302bU) op = 3; // MZERO.
  if ((probe.instr & 0xfe0fffffU) == 0x9000402bU) op = 2; // MRELEASE.
  const bool minit = probe.instr == 0x0000002b;
  if ((op >= 0 || minit) && (probe.skip || probe.isRVC || probe.nFused)) {
    fprintf(stderr, "Unobservable matrix CPU retirement: core=%d pc=0x%016lx "
      "id=%lu skip=%u rvc=%u fused=%u\n", coreid, probe.pc,
      probe.retireId, probe.skip, probe.isRVC, probe.nFused);
    return false;
  }
  if (minit && cpu_minit_epoch == UINT64_MAX) return false;

  cpu_retire_id = probe.retireId;
  if (op >= 0) cpu_amu_commits.push_back({probe.pc, probe.retireId, cpu_minit_epoch, uint8_t(op)});
  if (minit) ++cpu_minit_epoch;
  return true;
#endif // CONFIG_DIFFTEST_SQUASH
}
#endif // CONFIG_DIFFTEST_AMUCTRLEVENT

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
