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
#include "trace_writer.h"

TraceWriter::TraceWriter(std::string trace_file_name) {
  trace_stream = new std::ofstream(trace_file_name, std::ios_base::out);
  if ((!trace_stream->is_open())) {
    std::ostringstream oss;
    oss << "[TraceWriter.TraceWriter] Could not open file: " << trace_file_name;
    throw std::runtime_error(oss.str());
  }
}

bool TraceWriter::write(Instruction &inst) {
  if (trace_stream == NULL) {
    throw std::runtime_error("[TraceWriter.write] empty trace_stream.");
    return false;
  }

  trace_stream->write(reinterpret_cast<char *> (&inst), sizeof(Instruction));
  return true;
}

bool TraceWriter::write(uint64_t pc, uint32_t instr) {
  if (trace_stream == NULL) {
    throw std::runtime_error("[TraceWriter.write] empty trace_stream.");
    return false;
  }

  Instruction inst;
  inst.instr_pc_va = pc;
  inst.instr = instr;

  trace_stream->write(reinterpret_cast<char *> (&inst), sizeof(Instruction));
  return true;
}

bool TraceWriter::write(Control &ctrl) {
  if (trace_stream == NULL) {
    throw std::runtime_error("[TraceWriter.write] empty trace_stream.");
    return false;
  }

  trace_stream->write(reinterpret_cast<char *> (&ctrl), sizeof(Control));
  return true;
}

void TraceWriter::traceOver() {
  if (trace_stream == NULL) {
    throw std::runtime_error("[TraceWriter.traceOver] empty trace_stream.");
    return;
  }

  trace_stream->close();
}