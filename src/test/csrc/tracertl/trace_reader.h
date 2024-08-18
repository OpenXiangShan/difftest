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
#include <queue>
#include "trace_format.h"
#include "emu.h"

enum TraceStatus {
  TRACE_IDLE,
  TRACE_EOF,
  TRACE_ERROR,
  TRACE_ERROR_DRIVE,
  TRACE_STUCK
};

class TraceCounter {
  uint64_t value = 0;

public:
  inline uint64_t pop() { return value++; }
  inline uint64_t get() { return value; }
  inline void set(uint64_t id) { value = id; }
  inline void reset() { value = 0; }
  inline void inc() { value++; }
  inline void dec() { value++; }
  inline void add(uint64_t id) { value += id; }
  inline void sub(uint64_t id) { value -= id; }
};

struct TraceCollectBufferEntry {
  bool valid = false;
  TraceCollectInstruction inst;
};

#define DriveInstDecodedSize 16
#define CommittedInstSize 16
#define DutCommittedInstSize 16
#define RedirectLogSize 16
#define TraceCommitBufferSize 8
#define TraceDriveBufferSize 8
#define TraceReadBufferSize TraceFetchWidth

class TraceReader {
  std::ifstream *trace_stream_preread;
  TraceCounter inst_id_preread;
  std::queue<Instruction> instList_preread;

  TraceStatus status;
  TraceCollectInstruction errorInst;

  TraceCounter commit_inst_num;
  uint64_t last_commit_tick = 0;
  bool last_committed = false;
  uint64_t BLOCK_THREASHOLD = 5000;
  uint64_t FIRST_BLOCK_THREASHOLD = 15000;

  // process print
  uint64_t PRINT_INST_INTERVAL = 100000;
  uint64_t next_print_inst = 0;
  uint64_t last_interval_time = 0;
  uint64_t last_interval_tick = 0;
  uint64_t last_interval_inst = 0;

  // for all the inst queue:
  // back(old) -> front(young)
  // normally, push_back to insert, pop_front to pop out
  // but, for the redirect, we need to pop_back to get the older(bigger inst_id) inst

  // Commit Check
  // pending: inserted to dut.
  // Usage1: used by check method to difftest with the dut's commit info
  // Usage2: used by redirect method to re-insert to dut
  std::deque<Instruction> pendingInstList;
  // committed from the dut, recording the trace inst info. Print it to debug.
  std::deque<Instruction> committedInstList;
  // committed from the dut, recording the dut inst info. Print it to debug.
  std::deque<TraceCollectInstruction> dutCommittedInstList;
  // control the size(num of entry) of of above list to print

  // Drive Check
  // Similar with the commit check, but use for dirve.
  // "drive" is put after ibuffer's out
  std::deque<Instruction> driveInstInput;
  std::deque<TraceCollectInstruction> driveInstDecoded;

  // Redirect support
  std::deque<Instruction> redirectInstList;
  std::deque<uint64_t> redirectLog;

  // Read buffer for multi-thread support
  // Problem: when verilator multi-thread, dpic should be thread-safe.
  //   But trace-read is serialized. So we need a buffer to make it thread-isolated to achieve thread-safe
//  uint8_t readBufferStartIdx = 0;
  volatile bool readBufferNeedReload = true; // when dut read, set it true
  Instruction readBuffer[TraceReadBufferSize]; // Keep the same with PredictWidth(== TraceReadWidth)
  TraceCollectBufferEntry commitBuffer[TraceCommitBufferSize];
  TraceCollectBufferEntry driveBuffer[TraceDriveBufferSize];

  // performance counter
  uint64_t counterReadFromRedirect = 0;
  uint64_t counterReadFromInstList = 0;

  // time
  uint64_t gen_cur_time() {
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    return std::chrono::duration_cast<std::chrono::seconds>(duration).count();
  }

public:
  TraceReader(std::string trace_file_name);
  ~TraceReader() {
  }
  /* get an instruction from file */
  // Used by dut to read
  bool readFromBuffer(Instruction &inst, uint8_t idx);
  bool prepareRead();
  // Used by prepareRead
  bool read(Instruction &inst);

  void redirect(uint64_t inst_id);
  void collectCommit(uint64_t pc, uint32_t inst, uint8_t instNum, uint8_t idx);
  void collectDrive(uint64_t pc, uint32_t inst, uint8_t idx);
  void checkCommit(uint64_t tick);
  void checkDrive();
  /* if the trace is over */
  bool traceOver();

  bool update_tick(uint64_t tick);
  void dump_uncommited_inst();
  void dump_committed_inst();
  void dump_dut_committed_inst();
  void error_dump();
  void assert_dump();
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
