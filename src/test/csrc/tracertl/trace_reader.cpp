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
  instList.push_back(inst); // for commit check
  driveInstInput.push_back(inst); // for ibuffer drive check

//  printf("[%08lu] ", read_inst_cnt++);
//  inst.dump();

  return true;
}

bool TraceReader::check(uint64_t pc, uint32_t instn, uint8_t instNum) {
  if (isError() || isStuck() || isErrorDrive()) {
    return false;
  }
  if (instList.size() < instNum) {
    return false;
  }
  Instruction inst = instList.front();
  if ((inst.instr_pc_va != pc) || (inst.instr != instn)) {
    errorInst.instr = instn;
    errorInst.instr_pc = pc;
    errorInst.instNum = instNum;
    errorInst.instID = commit_inst_num;
    return false;
  }

  printf("TraceCheck:=== [%08lu] pc 0x%08lx instn 0x%08x instNum %d==\n", commit_inst_num, pc, instn, instNum);
  for (int i = 0; i < instNum; i++) {
      auto tmp = instList.front();
      tmp.dump();
      committedInst.push_back(tmp);
      if (committedInst.size() > CommittedInstSize) {
          committedInst.pop_front();
      }
      instList.pop_front();
  }
  printf("TraceCheck End================================\n");

  TraceCollectInstruction dut;
  dut.instr_pc = pc;
  dut.instr = instn;
  dut.instNum = instNum;
  dut.instID = commit_inst_num;
  dutCommittedInst.push_back(dut);
  if (dutCommittedInst.size() > DutCommittedInstSize) {
      dutCommittedInst.pop_front();
  }

  setCommit();
  commit_inst_num += instNum;
  return true;
}

bool TraceReader::check_drive(uint64_t pc, uint32_t inst) {
  if (isError() || isStuck() || isErrorDrive()) {
    return false;
  }
  if (driveInstInput.empty()) {
    printf("DriveInstInput empty\n");
    return false;
  }
  if (driveInstInput.front().instr_pc_va != pc || driveInstInput.front().instr != inst) {
    printf("DriveInstInput not match\n");
    errorInst.instr = inst;
    errorInst.instr_pc = pc;
    errorInst.instNum = 1;
    errorInst.instID = decoded_inst_num;
    return false;
  }
  driveInstInput.pop_front();

  TraceCollectInstruction dut;
  dut.instr_pc = pc;
  dut.instr = inst;
  dut.instNum = 1;
  dut.instID = decoded_inst_num;
  driveInstDecoded.push_back(dut);
  if (driveInstDecoded.size() > DriveInstDecodedSize ) {
    driveInstDecoded.pop_front();
  }
  decoded_inst_num++;
  return true;
}

void TraceReader::dump_uncommited_inst() {
  printf("UnCommitted Inst: ========================\n");
  int i = 0;
  for (auto inst : instList) {
    printf("[%08lu] ", commit_inst_num + (i++));
    inst.dump();
  }
  printf("UnCommitted Inst End =======================\n");
}

void TraceReader::dump_committed_inst() {
  uint64_t base_idx = commit_inst_num - committedInst.size();
  printf("Committed Inst: =======================\n");
  for (auto inst : committedInst) {
    printf("[%08lu] ", base_idx++);
    inst.dump();
  }
  printf("Committed Inst End =======================\n");
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
  if ((tick - last_commit_tick) >= threa) {
    setStuck();
    return false;
  }

  return true;
}

void TraceReader::dump_dut_committed_inst() {
  printf("DUT Committed Inst:=====================\n");
  for (auto inst : dutCommittedInst) {
    inst.dump();
  }
  printf("DUT Committed Inst End ==============\n");
}

void TraceReader::error_dump() {
  printf("\n");
  printf("TraceRTL Dump:\n");
  printf("commit_inst_num: %lu\n", commit_inst_num);
  printf("last_commit_tick: %lu\n", last_commit_tick);

  dump_committed_inst();
  dump_dut_committed_inst();
  printf("\n\n\n");
  if (isError()) {
    printf("========= TraceRTL Error at inst %lu ===========\n", commit_inst_num);
    printf("DUT inst: ");
    errorInst.dump();
    printf("========= TraceRTL Error End ===========\n");
  } else {
    printf("========= TraceRTL Stuck at inst %lu ===========\n", commit_inst_num);
  }
  dump_uncommited_inst();
}

void TraceReader::success_dump() {
  printf("\n");
  printf("TraceRTL Dump:\n");
  printf("commit_inst_num: %lu\n", commit_inst_num);
  printf("last_commit_tick: %lu\n", last_commit_tick);
}

void TraceReader::error_drive_dump() {
  printf("\n");
  printf("Drive Decoded: =================\n");
  int i = 0;
  for (auto inst : driveInstDecoded) {
//    printf("[%08lu]:", decoded_inst_num - driveInstDecoded.size() + (i++));
    inst.dump();
  }
  printf("Drive Decoded End================= \n");

  printf("DUT Inst");
  errorInst.dump();

  printf("Drive Not Decoded: =================\n");
  i = 0;
  for (auto inst : driveInstInput) {
    printf("[%08lu]", decoded_inst_num + (i++));
    inst.dump();
  }
  printf("Drive Not Decoded End================= \n");
}