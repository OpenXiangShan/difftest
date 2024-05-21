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
#include "tracertl.h"
#include "trace_format.h"
#include "trace_reader.h"
#include "trace_writer.h"

TraceReader *trace_reader = NULL;
// TraceWriter *trace_writer = NULL;

void init_tracertl(const char *trace_file_name) {
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

bool read_one_trace_bare(uint64_t *pc, uint32_t *instr) {
  Instruction inst;
  if (trace_reader->traceOver() || !trace_reader->read(inst)) {
    return false;
  }
  *pc = inst.instr_pc;
  *instr = inst.instr;
  return true;
}