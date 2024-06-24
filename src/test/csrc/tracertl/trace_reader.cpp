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
  trace_stream->read(reinterpret_cast<char *> (&inst), sizeof(TraceInstruction));
  instList.push_back(inst);

//  printf("[%08lu] ", read_inst_cnt++);
//  inst.dump();

  return true;
}

bool TraceReader::check(uint64_t pc, uint32_t instn, uint8_t instNum) {
    if (instList.size() < instNum) {
      return false;
    }
    Instruction inst = instList.front();
    if ((inst.instr_pc_va != pc) || (inst.instr != instn)) {
      return false;
    }

    for (int i = 0; i < instNum; i++) {
        instList.pop_front();
    }
    setCommit();
    commit_inst_num += instNum;
    return true;
}

void TraceReader::dump_uncommited_inst() {
  for (auto inst : instList) {
    printf("[%08lu] ", commit_inst_num++);
    inst.dump();
  }
}

bool TraceReader::traceOver() {
  // end of file or add signal into the trace
  return trace_stream->eof();
}

bool TraceReader::update_tick(uint64_t tick) {
  uint64_t threa;
  if (commit_inst_num == 0) threa = FIRST_BLOCK_THREASHOLD;
  else threa = BLOCK_THREASHOLD;

   if (isCommited()) {
     last_commit_tick = tick;
     clearCommit();
     return true;
   }
  return (tick - last_commit_tick) < threa;
}

void TraceReader::dump() {
  printf("\n");
  printf("TraceRTL Dump:\n");
  printf("commit_inst_num: %lu\n", commit_inst_num);
  printf("last_commit_tick: %lu\n", last_commit_tick);

  dump_uncommited_inst();
}