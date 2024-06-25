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

#include <cstdio>
#include <cstdlib>
#include "tracertl.h"
#include "trace_format.h"
#include "trace_reader.h"
#include "trace_writer.h"
#include "trace_icache.h"

TraceReader *trace_reader = NULL;
// TraceWriter *trace_writer = NULL;

/** Used By Emulator */

void init_tracertl(const char *trace_file_name) {
  printf("init_tracertl: %s\n", trace_file_name);
  trace_reader = new TraceReader(trace_file_name);
}

bool tracertl_over() {
  return trace_reader->isOver();
}

bool tracertl_error() {
  return trace_reader->isError();
}

bool tracertl_error_drive() {
  return trace_reader->isErrorDrive();
}

bool tracertl_stuck() {
  return trace_reader->isStuck();
}

bool tracertl_update_tick(uint64_t tick) {
  return trace_reader->update_tick(tick);
}

void tracertl_error_dump() {
  trace_reader->error_dump();
}

void tracertl_error_drive_dump() {
  trace_reader->error_drive_dump();
}

void tracertl_success_dump() {
  trace_reader->success_dump();
}

/*
 * TraceICache init and DPI-C Helper
 **/

extern "C" void trace_read_one_instr(
  uint64_t *pc_va, uint64_t *pc_pa, uint64_t *memory_addr_va, uint64_t *memory_addr_pa,
  uint64_t *target, uint32_t *instr,
  uint8_t *memory_type, uint8_t *memory_size, uint8_t *branch_type, uint8_t *branch_taken) {

  if (trace_reader->traceOver()) {
    printf("trace_read_one_instr: traceOver. Finish\n");
    trace_reader->setOver();
    // TODO: insert nop
    return ;
  }
  Instruction inst;
  trace_reader->read(inst);

  *pc_va = inst.instr_pc_va;
  *pc_pa = inst.instr_pc_pa;
  *memory_addr_va = inst.memory_address_va;
  *memory_addr_pa = inst.memory_address_pa;
  *target = inst.target;
  *instr = inst.instr;
  *memory_type = inst.memory_type;
  *memory_size = inst.memory_size;
  *branch_type = inst.branch_type;
  *branch_taken = inst.branch_taken;
}

extern "C" void trace_collect_one_instr(uint64_t pc, uint32_t instr, uint8_t instNum) {
    if (!trace_reader->check(pc, instr, instNum)) {
      trace_reader->setError();
      return ;
    };
}

extern "C" void trace_drive_collect_one_instr(uint64_t pc, uint32_t instr) {
    if (!trace_reader->check_drive(pc, instr)) {
      trace_reader->setErrorDrive();
      return ;
    };
}

/** Fake ICache */

TraceICache *trace_icache = NULL;

extern "C" void init_traceicache(const char *binary_name) {
  trace_icache = new TraceICache(binary_name);
}

extern "C" uint64_t trace_icache_dword_helper(uint64_t addr) {
  uint64_t data;
  trace_icache->readDWord(&data, addr);
  return data;
}

extern "C" uint8_t trace_icache_legal_addr(uint64_t addr) {
  if (trace_icache->legalAddr(addr)) return 1;
  else return 0;
}

extern "C" void trace_icache_helper(uint64_t addr, uint8_t *result_valid, uint64_t *data0, uint64_t *data1, uint64_t *data2, uint64_t *data3, uint64_t *data4, uint64_t *data5, uint64_t *data6, uint64_t *data7) {
  uint64_t line[4];
  if (!trace_icache->readHalfCacheLine((char *)line, addr)) {
    *result_valid = 0;
    return ;
  }
  *data0 = line[0];
  *data1 = line[1];
  *data2 = line[2];
  *data3 = line[3];
  if (!trace_icache->readHalfCacheLine((char *)line, addr + 256)) {
    *result_valid = 0;
    return ;
  }
  *data4 = line[0];
  *data5 = line[1];
  *data6 = line[2];
  *data7 = line[3];

  *result_valid = 1;
  return ;
}