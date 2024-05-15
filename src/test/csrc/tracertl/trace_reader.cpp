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

#include <stdexcept>
#include <sstream>
#include "trace_reader.h"

TraceReader::TraceReader(std::string trace_file_name)
{
  trace_stream = new std::ifstream(trace_file_name, std::ios_base::in);
  if ((!trace_stream->is_open())) {
    std::ostringstream oss;
    oss << "[TraceReader.TraceReader] Could not open file: " << trace_file_name;
    throw std::runtime_error(oss.str());
  }
}

bool TraceReader::read(Instruction &inst) {
  if (trace_stream == NULL) {
    throw std::runtime_error("[TraceReader.read] empty trace_stream.");
    return false;
  }

  if (traceOver()) {
    // this should no happen, check it outside
    throw std::runtime_error("[TraceReader.read] end of file.");
    return false;
  }

  TraceInstruction trace_entry;
  trace_stream->read(reinterpret_cast<char *> (&trace_entry), sizeof(TraceInstruction));

  trace_entry.dump();

  inst.memory_size = trace_entry.memory_size;
  inst.instr_pc    = trace_entry.instr_pc;
  inst.instr       = trace_entry.instr;
  inst.memory_address = trace_entry.memory_address;

  return true;
}

bool TraceReader::traceOver() {
  // end of file or add signal into the trace
  return trace_stream->eof();
}