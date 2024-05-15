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

#ifndef __TRACE_READER_H__
#define __TRACE_READER_H__

#include <fstream>
#include <iostream>

// TODO : pack it
struct TraceInstruction {
  // bool is_branch;
  // bool branch_taken;
  uint8_t memory_size;
  uint8_t padding[3];
  uint32_t instr;
  uint64_t instr_pc;
  uint64_t memory_address;

  void dump() {
    printf("Instr: TraceSize %ld memSize %02x PC 0x%016lx instr 0x%04x memAddr 0x%016lx\n", sizeof(TraceInstruction), memory_size, instr_pc, instr, memory_address);
  }
};

struct Instruction : TraceInstruction {};

class TraceReader {
  std::ifstream *trace_stream;

public:
  TraceReader(std::string trace_file_name);
  ~TraceReader() {
    delete trace_stream;
  }
  /* get an instruction from file */
  bool read(Instruction &inst);
  /* if the trace is over */
  bool traceOver();
};

#endif