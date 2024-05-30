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
TraceICache *trace_icache = NULL;
// TraceWriter *trace_writer = NULL;

extern "C" void init_tracertl(const char *trace_file_name) {
  printf("init_tracertl: %s\n", trace_file_name);
  trace_reader = new TraceReader(trace_file_name);
}


Instruction read_one_trace() {
  Instruction inst;
  if (trace_reader->traceOver() || !trace_reader->read(inst)) {
    inst.instr = 0;
  }
  return inst;
}

extern "C" bool read_one_trace_bare(uint64_t *pc, uint32_t *instr) {
  Instruction inst;
  if (trace_reader->traceOver() || !trace_reader->read(inst)) {
    return false;
  }
  *pc = inst.instr_pc;
  *instr = inst.instr;
  return true;
}

/*
 * TraceICache init and DPI-C Helper
 **/

extern "C" void init_traceicache(const char *binary_name) {
  trace_icache = new TraceICache(binary_name);
}

extern "C" void trace_read_one_instr(uint8_t enable, uint64_t *pc, uint32_t *instr) {
  if (!enable) {
    *pc = 0;
    *instr = 0;
    return ;
  }
  Instruction inst;
  if (trace_reader->traceOver()) {
    printf("trace_read_one_instr: traceOver. Finish\n");
    printf("TODO: add trap signal to finish the simulation\n");
    exit(0);
    return ;
  }
  if (!trace_reader->read(inst)) {
    printf("trace_read_one_instr: read failed. Finish\n");
    printf("TODO: add trap signal to finish the simulation\n");
    exit(1);
    return ;
  }
  *pc = inst.instr_pc;
  *instr = inst.instr;
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