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
  TRACE_ERROR,
  TRACE_ERROR_DRIVE,
  TRACE_STUCK
};

class TraceReader {
  std::ifstream *trace_stream;

  TraceStatus status;
  TraceCollectInstruction errorInst;

  uint64_t commit_inst_num = 0;
  uint64_t last_commit_tick = 0;
  bool last_committed = false;
  uint64_t BLOCK_THREASHOLD = 2000;
  uint64_t FIRST_BLOCK_THREASHOLD = 5000;

  uint64_t read_inst_cnt = 0;
  std::deque<Instruction> instList;
  std::deque<Instruction> committedInst;
  std::deque<TraceCollectInstruction> dutCommittedInst;
  const int CommittedInstSize = 16;
  const int DutCommittedInstSize = 8;

  uint64_t decoded_inst_num = 0;
  std::deque<Instruction> driveInstInput;
  std::deque<TraceCollectInstruction> driveInstDecoded;
  const int DriveInstDecodedSize = 16;

public:
  TraceReader(std::string trace_file_name);
  ~TraceReader() {
    delete trace_stream;
  }
  /* get an instruction from file */
  bool read(Instruction &inst);
  bool check(uint64_t pc, uint32_t inst, uint8_t instNum);
  bool check_drive(uint64_t pc, uint32_t inst);
  /* if the trace is over */
  bool traceOver();

  bool update_tick(uint64_t tick);
  void dump_uncommited_inst();
  void dump_committed_inst();
  void dump_dut_committed_inst();
  void error_dump();
  void success_dump();
  void error_drive_dump();

  bool isOver() { return status == TRACE_EOF; }
  bool isError() { return status == TRACE_ERROR; }
  bool isErrorDrive() { return status == TRACE_ERROR_DRIVE; }
  bool isStuck() { return status == TRACE_STUCK; }

  void setOver() { status = TRACE_EOF; }
  void setError() { status = TRACE_ERROR; }
  void setErrorDrive() { status = TRACE_ERROR_DRIVE; }
  void setStuck() { status = TRACE_STUCK; }

  bool isCommited() { return last_committed;}
  void setCommit() { last_committed = true;}
  void clearCommit() { last_committed = false;}
};

#endif