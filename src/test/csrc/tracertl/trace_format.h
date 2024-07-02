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

#define Log() printf("file: %s, line: %d\n", __FILE__, __LINE__); fflush(stdout)

// TODO : pack it
// TODO : mv instr and instr_pc to map(or more ca, icache)
struct TraceInstruction {
  uint64_t instr_pc_va;
  uint64_t instr_pc_pa;
  uint64_t memory_address_va;
  uint64_t memory_address_pa;
  uint64_t target = 0;
  uint32_t instr;
  uint8_t memory_type;
  uint8_t memory_size;
  uint8_t branch_type;
  uint8_t branch_taken;

  void dump() {
    // printf("Instr: TraceSize %ld memSize %02x PC 0x%016lx instr 0x%04x memAddr 0x%016lx\n", sizeof(TraceInstruction), memory_size, instr_pc, instr, memory_address);
    printf("PC 0x%08lx|%08lx instr 0x%08x(%s)", instr_pc_va, instr_pc_pa, instr, spike_dasm(instr));
    if (memory_type != 0) {
      printf(" is_mem %d addr %08lx|%08lx", memory_type, memory_address_va, memory_address_pa);
    }
    if (branch_type != 0) {
      printf(" is_branch %d taken %d target %08lx", branch_type, branch_taken, target);
    }
    printf("\n");
  }
};

struct Instruction {
  TraceInstruction static_inst;
  uint64_t inst_id;

  void dump() {
    printf("[0x%08lx]: ", inst_id);
    static_inst.dump();
  }
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