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

#ifndef __TRACERTL_H__
#define __TRACERTL_H__

#include "trace_format.h"
#include "trace_reader.h"
#include "trace_icache.h"

//TraceReader *trace_reader = NULL;
extern TraceICache *trace_icache;

/*             called by emu              */
void init_tracertl(const char *tracefile_name);
void init_traceicache();
bool tracertl_prepare_read();
void tracertl_check_commit(uint64_t tick);
void tracertl_check_drive();
bool tracertl_over();
bool tracertl_error();
bool tracertl_error_drive();
bool tracertl_update_tick(uint64_t tick);
bool tracertl_stuck();
void tracertl_error_dump();
void tracertl_assert_dump();
void tracertl_success_dump();
void tracertl_error_drive_dump();

/*             called by dut              */
void __attribute__((noinline)) trace_read_insts(uint8_t enable, ManyInstruction_t insts);
extern "C" void trace_read_one_instr(
  uint64_t *pc_va, uint64_t *pc_pa, uint64_t *memory_addr_va, uint64_t *memory_addr_pa,
  uint64_t *target, uint32_t *instr,
  uint8_t *memory_type, uint8_t *memory_size,
  uint8_t *branch_type, uint8_t *branch_taken,
  uint8_t *exception,
  uint64_t *instID, uint8_t idx);
extern "C" void trace_redirect(uint64_t inst_id);
extern "C" void trace_collect_commit(uint64_t pc, uint32_t instr, uint8_t instNum, uint8_t idx);
extern "C" void trace_collect_drive(uint64_t pc, uint32_t instr, uint8_t idx);

extern "C" void trace_icache_helper(uint64_t addr, uint8_t *res_valid, uint64_t *data0, uint64_t *data1, uint64_t *data2, uint64_t *data3, uint64_t *data4, uint64_t *data5, uint64_t *data6, uint64_t *data7);
extern "C" uint64_t trace_icache_dword_helper(uint64_t addr);
extern "C" uint8_t trace_icache_legal_addr(uint64_t addr);
extern "C" uint64_t trace_tlb_ats(uint64_t vaddr, uint16_t asid, uint16_t vmid);
extern "C" bool trace_tlb_ats_hit(uint64_t vaddr, uint16_t asid, uint16_t vmid);
extern "C" uint64_t trace_dyn_pt_dword_helper(uint64_t addr);

#endif