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
#include "trace_format.h"
#include "emu.h"

enum TraceStatus {
  TRACE_IDLE,
  TRACE_EOF,
  TRACE_ERROR
};

class TraceReader {
  std::ifstream *trace_stream;
  std::deque<Instruction> instList;
  TraceStatus status;
  uint64_t commit_inst_num = 0;
  uint64_t last_commit_tick = 0;
  uint64_t BLOCK_THREASHOLD = 2000;
  uint64_t FIRST_BLOCK_THREASHOLD = 5000;

  bool last_committed = false;

public:
  TraceReader(std::string trace_file_name);
  ~TraceReader() {
    delete trace_stream;
  }
  /* get an instruction from file */
  bool read(Instruction &inst);
  bool check(uint64_t pc, uint32_t inst, uint8_t instNum);
  /* if the trace is over */
  bool traceOver();

  bool update_tick(uint64_t tick);
  void dump();

  bool isOver() { return status == TRACE_EOF; }
  bool isError() { return status == TRACE_ERROR; }
  void setOver() { status = TRACE_EOF; }
  void setError() { status = TRACE_ERROR; }

  bool isCommited() { return last_committed;}
  void setCommit() { last_committed = true;}
  void clearCommit() { last_committed = false;}
};

#endif