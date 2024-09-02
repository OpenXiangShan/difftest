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
#ifndef __TRACE_FORMAT_H__
#define __TRACE_FORMAT_H__

#include <cstdint>
#include <stdio.h>
#include "spikedasm.h"

// #define TRACE_METHOD_TRACE
#define PRINT_SIMULATION_SPEED


#define Log() printf("file: %s, line: %d\n", __FILE__, __LINE__); fflush(stdout)

#ifdef TRACE_METHOD_TRACE
#define METHOD_TRACE() Log()
#else
#define METHOD_TRACE()
#endif

#define TraceFetchWidth 16

#define PADDRBITS 39
#define VADDRBITS 36

inline uint64_t paddr_mask() {
  uint64_t mask = (uint64_t) -1;
  return mask >> (64 - PADDRBITS);
};
inline uint64_t vaddr_mask() {
  uint64_t mask = (uint64_t) -1;
  return mask >> (64 - VADDRBITS);
};

// TODO : pack it
// TODO : mv instr and instr_pc to map(or more ca, icache)
struct TraceInstruction {
  uint64_t instr_pc_va;
  uint64_t instr_pc_pa;
  uint64_t memory_address_va;
  uint64_t memory_address_pa;
  uint64_t target = 0;
  uint32_t instr;
  uint8_t memory_type:4;
  uint8_t memory_size:4;
  uint8_t branch_type;
  uint8_t branch_taken;
  uint8_t exception;

  void setToForceJump(uint64_t o_pc, uint64_t o_target) {
    instr_pc_va = o_pc;
    instr_pc_pa = o_pc;
    memory_address_pa = 0;
    memory_address_va = 0;
    target = o_target;
    instr = 0;
    memory_type = 0;
    memory_size = 0;
    branch_taken = 0;
    branch_type = 0;
    exception = 0x81;
  }

  bool pc_va_match(uint64_t pc_va) {
    return (instr_pc_va & paddr_mask()) == (pc_va & paddr_mask());
  }
  bool pc_pa_match(uint64_t pc_pa) {
    return (instr_pc_pa & paddr_mask()) == (pc_pa & paddr_mask());
  }

  bool legalInst() {
    // when not exception, pc_va and inst should not be zero (when pc_pa is zero, vm not enable)
    if (exception == 0) return (instr_pc_va != 0) && (instr != 0);
    // when exception, pc_va & target should not be zero
    else if (isException()) return (instr_pc_va != 0) && (target != 0);
    // when interrupt, target should not be zero
    return target != 0;
  }

  bool isCtrlForceJump() {
    return exception != 0;
  }

  bool isInterrupt() {
    return (exception & 0x80) != 0;
  }

  bool isException() {
    return (exception & 0x80) == 0;
  }

  void dump() {
    // printf("Instr: TraceSize %ld memSize %02x PC 0x%016lx instr 0x%04x memAddr 0x%016lx\n", sizeof(TraceInstruction), memory_size, instr_pc, instr, memory_address);
    printf("PC 0x%08lx|%08lx instr 0x%08x(%s)", instr_pc_va, instr_pc_pa, instr, spike_dasm(instr));
    if (memory_type != 0) {
      printf(" is_mem %d addr %08lx|%08lx", memory_type, memory_address_va, memory_address_pa);
    }
    if (branch_type != 0) {
      printf(" is_branch %d taken %d target 0x%08lx", branch_type, branch_taken, target);
    }
    if (exception != 0) {
      printf(" is_exception 0x%02x target 0x%08lx", exception, target);
    }
    printf("\n");
    fflush(stdout);
  }
};

struct Instruction : TraceInstruction {
  // TraceInstruction static_inst;
  uint64_t inst_id;

  void dump() {
    printf("[0x%08lx]: ", inst_id);
    TraceInstruction::dump();
    fflush(stdout);
  }

  inline void fromTraceInst(TraceInstruction t) {
    instr_pc_va = t.instr_pc_va;
    instr_pc_pa = (t.instr_pc_pa != 0) ? t.instr_pc_pa : t.instr_pc_va;
    memory_address_va = t.memory_address_va;
    memory_address_pa = (t.memory_address_pa != 0) ? t.memory_address_pa : t.memory_address_va;
    target = t.target;
    instr = t.instr;
    memory_type = t.memory_type;
    memory_size = t.memory_size;
    branch_type = t.branch_type;
    branch_taken = t.branch_taken;
    exception = t.exception;
  }

  bool legalInst() {
    if (inst_id == 0) {
      return false;
    }
    return TraceInstruction::legalInst();
  }
};

struct ManyInstruction_t {
  Instruction insts[TraceFetchWidth]; // 16 is traceFetchWidth
};

struct Control {
  uint8_t type;
  uint8_t data;
  // TODO: placeholder, not implemented
};

struct TraceCollectInstruction {
  uint64_t instr_pc;
  uint32_t instr;
  uint8_t instNum;
  uint64_t instID;

  void dump() {
    if (instNum == 1 || instNum == 0) {
        printf("[0x%08lx]: PC 0x%08lx instr 0x%08x(%s)\n", instID, instr_pc, instr, spike_dasm(instr));
    } else {
        printf("[0x%08lx]: PC 0x%08lx instr 0x%08x(%s) Merge %d\n", instID, instr_pc, instr, spike_dasm(instr), instNum);
    }
  }
};

#endif