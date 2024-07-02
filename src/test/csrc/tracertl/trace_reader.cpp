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
  // read a inst from redirect buffer
  if (redirectInstList.size() > 0) {
    inst = redirectInstList.front();
    redirectInstList.pop_front();
  } else {
    // read a inst from the source
    trace_stream->read(reinterpret_cast<char *> (&(inst.static_inst)), sizeof(TraceInstruction));
    inst.inst_id = current_inst_id.pop();
  }
  pendingInstList.push_back(inst); // for commit check
  driveInstInput.push_back(inst); // for ibuffer drive check

//  inst.dump();

  return true;
}

/**
  * Redirect the instruction to re-run from inst_id
  * 1. pop inst from pendingInstList(from oldest to inst_id) and push to redirectInstList
  *    pendingInstList : back(old) -> front(young). the front is the youngest inst.
  *    redirectInstList: back(old) -> front(young). the front is the youngest inst.
  *    younger inst has smaller inst_id.
  * 2. flush the driveInstInput
  */
void TraceReader::redirect(uint64_t inst_id) {
  redirectLog.push_back(inst_id);

  if (pendingInstList.size() > 0) {
    if (pendingInstList.back().inst_id < inst_id || pendingInstList.front().inst_id > inst_id) {
      setError();
      printf("Redirect Error: inst_id %lu not in the pendingInstList range [%lu, %lu]\n",
       inst_id, pendingInstList.back().inst_id, pendingInstList.front().inst_id);
      return;
    }
  }

  // redirect inst from pendingInstList to redirectInstList
  while(pendingInstList.size() > 0){
    Instruction inst = pendingInstList.back();
    if (inst.inst_id < inst_id) break;
    pendingInstList.pop_back();
    redirectInstList.push_front(inst);
  }
  // flush driveInstInput
  driveInstInput.clear();
}

bool TraceReader::check(uint64_t pc, uint32_t instn, uint8_t instNum) {
  if (isError() || isStuck() || isErrorDrive()) {
    return false;
  }
  if (pendingInstList.size() < instNum) {
    return false;
  }
  Instruction inst = pendingInstList.front();
  if ((inst.static_inst.instr_pc_va != pc) || (inst.static_inst.instr != instn)) {
    errorInst.instr = instn;
    errorInst.instr_pc = pc;
    errorInst.instNum = instNum;
    errorInst.instID = commit_inst_num.get();
    return false;
  }

//  printf("TraceCheck:=== [0x%08lx] pc 0x%08lx instn 0x%08x instNum 0x%x==\n", commit_inst_num.get(), pc, instn, instNum);
  for (int i = 0; i < instNum; i++) {
      auto tmp = pendingInstList.front();
//      tmp.dump();
      committedInstList.push_back(tmp);
      if (committedInstList.size() > CommittedInstSize) {
          committedInstList.pop_front();
      }
      pendingInstList.pop_front();
  }
//  printf("TraceCheck End================================\n");

  TraceCollectInstruction dut;
  dut.instr_pc = pc;
  dut.instr = instn;
  dut.instNum = instNum;
  dut.instID = commit_inst_num.get();
  dutCommittedInstList.push_back(dut);
  if (dutCommittedInstList.size() > DutCommittedInstSize) {
      dutCommittedInstList.pop_front();
  }

  setCommit();
  commit_inst_num.add(instNum);
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
  Instruction instBundle = driveInstInput.front();
  if (instBundle.static_inst.instr_pc_va != pc || instBundle.static_inst.instr != inst) {
    printf("DriveInstInput not match\n");
    errorInst.instr = inst;
    errorInst.instr_pc = pc;
    errorInst.instNum = 1;
    errorInst.instID = instBundle.inst_id;
    return false;
  }
  driveInstInput.pop_front();

  TraceCollectInstruction dut;
  dut.instr_pc = pc;
  dut.instr = inst;
  dut.instNum = 1;
  dut.instID = instBundle.inst_id;
  driveInstDecoded.push_back(dut);
  if (driveInstDecoded.size() > DriveInstDecodedSize ) {
    driveInstDecoded.pop_front();
  }
  return true;
}

void TraceReader::dump_uncommited_inst() {
  printf("UnCommitted Inst: ========================\n");
  for (auto inst : pendingInstList) {
    inst.dump();
  }
  printf("UnCommitted Inst End =======================\n");
}

void TraceReader::dump_committed_inst() {
  printf("Committed Inst: =======================\n");
  for (auto inst : committedInstList) {
    inst.dump();
  }
  printf("Committed Inst End =======================\n");
}

bool TraceReader::traceOver() {
  // end of file or add signal into the trace
  return redirectInstList.empty() && trace_stream->eof();
}

bool TraceReader::update_tick(uint64_t tick) {
  uint64_t threa;
  if (commit_inst_num.get() == 0) threa = FIRST_BLOCK_THREASHOLD;
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
  for (auto inst : dutCommittedInstList) {
    inst.dump();
  }
  printf("DUT Committed Inst End ==============\n");
}

void TraceReader::error_dump() {
  printf("\n");
  printf("TraceRTL Dump:\n");
  printf("commit_inst_num: %lu\n", commit_inst_num.get());
  printf("last_commit_tick: %lu\n", last_commit_tick);

  dump_committed_inst();
  dump_dut_committed_inst();
  printf("\n");
  if (isError()) {
    printf("========= TraceRTL Error at inst 0x%lx ===========\n", commit_inst_num.get());
    printf("DUT inst: ");
    errorInst.dump();
    printf("========= TraceRTL Error End ===========\n");
  } else {
    printf("========= TraceRTL Stuck at inst 0x%lx ===========\n", commit_inst_num.get());
  }
  dump_uncommited_inst();
}

void TraceReader::assert_dump() {
  printf("\n");
  printf("TraceRTL Dump:\n");
  printf("commit_inst_num: %lu\n", commit_inst_num.get());
  printf("last_commit_tick: %lu\n", last_commit_tick);

  dump_committed_inst();
  dump_dut_committed_inst();
  dump_uncommited_inst();
}

void TraceReader::success_dump() {
  printf("\n");
  printf("TraceRTL Dump:\n");
  printf("commit_inst_num: %lu\n", commit_inst_num.get());
  printf("last_commit_tick: %lu\n", last_commit_tick);
}

void TraceReader::error_drive_dump() {
  printf("\n");
  printf("Drive Decoded: =================\n");
  int i = 0;
  for (auto inst : driveInstDecoded) {
    inst.dump();
  }
  printf("Drive Decoded End================= \n");

  printf("DUT Inst");
  errorInst.dump();

  printf("Drive Not Decoded: =================\n");
  i = 0;
  for (auto inst : driveInstInput) {
    inst.dump();
  }
  printf("Drive Not Decoded End================= \n");
}