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

#include "checkers.h"
#include "flash.h"
#include "ram.h"

bool FirstInstrCommitChecker::get_valid(const DifftestInstrCommit &probe) {
  return !state->has_commit && probe.valid;
}

void FirstInstrCommitChecker::clear_valid(DifftestInstrCommit &probe) {
  state->has_commit = true;
}

int FirstInstrCommitChecker::check(const DifftestInstrCommit &probe) {
  Info("The first instruction of core %d has commited. Difftest enabled. \n", state->coreid);
  proxy->flash_init((const uint8_t *)flash_dev.base, flash_dev.img_size, flash_dev.img_path);
  simMemory->clone_on_demand(
      [this](uint64_t offset, void *src, size_t n) {
        uint64_t dest_addr = PMEM_BASE + offset;
        proxy->mem_init(dest_addr, src, n, DUT_TO_REF);
      },
      true);
  proxy->regcpy(&get_regs(), FIRST_INST_ADDRESS);
  // Do not reconfig simulator 'proxy->update_config(&nemu_config)' here:
  // If this is main sim thread, simulator has its own initial config
  // If this process is checkpoint wakeuped, simulator's config has already been updated,
  // do not override it.
  return STATE_OK;
}

int TimeoutChecker::check(const DifftestTrapEvent &probe) {
  uint64_t cycleCnt = probe.cycleCnt;
  // check whether there're any commits since the simulation starts
  if (!state->has_commit && cycleCnt > state->last_commit_cycle + first_commit_limit) {
    Info("The first instruction of core %d at 0x%lx does not commit after %lu cycles.\n", state->coreid,
         FIRST_INST_ADDRESS, first_commit_limit);
    return STATE_ERROR;
  }

  // NOTE: the WFI instruction may cause the CPU to halt for more than `stuck_limit` cycles.
  // We update the `last_commit_cycle` if the CPU has a WFI instruction
  // to allow the CPU to run at most `stuck_limit` cycles after WFI resumes execution.
  if (probe.hasWFI) {
    state->last_commit_cycle = cycleCnt;
  }

  // check whether there're any commits in the last `stuck_limit` cycles
  if (state->has_commit && cycleCnt > state->last_commit_cycle + stuck_commit_limit) {
    Info(
        "No instruction of core %d commits for %lu cycles, maybe get stuck\n"
        "(please also check whether a fence.i instruction requires more than %lu cycles to flush the icache)\n",
        state->coreid, stuck_commit_limit, stuck_commit_limit);
    Info("Let REF run one more instruction.\n");
    proxy->ref_exec(1);
    proxy->sync();
    return STATE_DIFF;
  }

  return STATE_OK;
}

#define DEBUG_MEM_REGION(v, f) (f <= (DEBUG_MEM_BASE + 0x1000) && f >= DEBUG_MEM_BASE && v)
#define IS_LOAD_STORE(instr)   (((instr & 0x7f) == 0x03) || ((instr & 0x7f) == 0x23))
#define IS_TRIGGERCSR(instr)   (((instr & 0x7f) == 0x73) && ((instr & (0xff0 << 20)) == (0x7a0 << 20)))
#define IS_DEBUGCSR(instr)     (((instr & 0x7f) == 0x73) && ((instr & (0xffe << 20)) == (0x7b0 << 20))) // 7b0 and 7b1
#ifdef DEBUG_MODE_DIFF
#define DEBUG_MODE_SKIP(v, f, instr) DEBUG_MEM_REGION(v, f) && (IS_LOAD_STORE(instr) || IS_TRIGGERCSR(instr))
#else
#define DEBUG_MODE_SKIP(v, f, instr) false
#endif

bool InstrCommitChecker::get_valid(const DifftestInstrCommit &probe) {
  return probe.valid;
}

void InstrCommitChecker::clear_valid(DifftestInstrCommit &probe) {
  probe.valid = 0;
  state->has_progress = true;
  state->last_commit_cycle = state->cycle_count;
#ifdef CONFIG_DIFFTEST_SQUASH
  state->commit_stamp = (state->commit_stamp + 1) % CONFIG_DIFFTEST_SQUASH_STAMPSIZE;
#endif // CONFIG_DIFFTEST_SQUASH
}

int InstrCommitChecker::check(const DifftestInstrCommit &probe) {
  const auto &dut = get_dut_state();

  // store the writeback info to debug array
#ifdef BASIC_DIFFTEST_ONLY
  uint64_t commit_pc = proxy->state.pc;
#else
  uint64_t commit_pc = probe.pc;
#endif
  uint64_t commit_instr = probe.instr;
  uint64_t commit_data = get_commit_data(&dut, index);
  state->record_inst(commit_pc, commit_instr, (probe.rfwen | probe.fpwen | probe.vecwen), probe.wdest, commit_data,
                     probe.skip != 0, probe.special & 0x1, probe.lqIdx, probe.sqIdx, probe.robIdx, probe.isLoad,
                     probe.isStore);

#ifdef FUZZING
  // isExit
  if (probe.special & 0x2) {
    state->raise_trap(STATE_SIM_EXIT);
#ifdef FUZZER_LIB
    stats.exit_code = SimExitCode::sim_exit;
#endif // FUZZER_LIB
    return STATE_TRAP;
  }
#endif // FUZZING

  // isDelayeWb
  if (probe.special & 0x1) {
    int *status =
#ifdef CONFIG_DIFFTEST_ARCHINTDELAYEDUPDATE
        probe.rfwen ? state->delayed_int :
#endif // CONFIG_DIFFTEST_ARCHINTDELAYEDUPDATE
#ifdef CONFIG_DIFFTEST_ARCHFPDELAYEDUPDATE
        probe.fpwen ? state->delayed_fp
                    :
#endif // CONFIG_DIFFTEST_ARCHFPDELAYEDUPDATE
                    nullptr;
    if (status) {
      if (status[probe.wdest]) {
        Info("The delayed register %s has already been delayed for %d cycles\n",
             (probe.rfwen ? regs_name_int : regs_name_fp)[probe.wdest], status[probe.wdest]);
        return STATE_DIFF;
      }
      status[probe.wdest] = 1;
    }
  }

#ifdef DEBUG_MODE_DIFF
  if (spike_valid() && (IS_DEBUGCSR(commit_instr) || IS_TRIGGERCSR(commit_instr))) {
    Info("s0 is %016lx ", dut.regs.xpr[8]);
    Info("pc is %lx %s\n", commit_pc, spike_dasm(commit_instr));
  }
#endif

  // MMIO accessing should not be a branch or jump, just +2/+4 to get the next pc
  // to skip the checking of an instruction, just copy the reg state to reference design
  if (probe.skip || (DEBUG_MODE_SKIP(probe.valid, probe.pc, probe.inst))) {
    // We use the physical register file to get wdata
    proxy->skip_one(probe.isRVC, (probe.rfwen && probe.wdest != 0), probe.fpwen, probe.vecwen, probe.wdest,
                    commit_data);
    return STATE_OK;
  }

  // Default: single step exec
  // when there's a fused instruction, let proxy execute more instructions.
  for (int j = 0; j < probe.nFused + 1; j++) {
    proxy->ref_exec(1);
  }

  return STATE_OK;
}
