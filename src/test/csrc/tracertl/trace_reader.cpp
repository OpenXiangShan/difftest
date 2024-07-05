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

bool TraceReader::readFromBuffer(Instruction &inst, uint8_t idx) {
  uint8_t idx_inner = (readBufferStartIdx + idx) % TraceReadBufferSize;
  if (readBuffer[idx_inner].valid) {
    inst = readBuffer[idx_inner].inst;
    readBuffer[idx_inner].valid = false;

//    printf("TraceReadBuffer %d+%d->newIdx:%d", idx, readBufferStartIdx, idx_inner);
//    inst.dump();
//    fflush(stdout);

    return true;
  } else {
    // traceOver
    Log();
//    printf("TraceRTL: Trace Over for readBuffer not enough");
//    fflush(stdout);
    return false;
  }
}

bool TraceReader::prepareRead() {
  // update startIdx
  uint8_t oldStartIdx = readBufferStartIdx;
//  bool updated = false;
  for (int i = 0; i < TraceReadBufferSize; i++) {
    uint8_t idx = (i + oldStartIdx) % TraceReadBufferSize;
    if (!readBuffer[idx].valid) {
//      updated = true;
      readBufferStartIdx = (idx + 1) % TraceReadBufferSize;
    } else { break; }
  }

//  if (updated) {
//    printf("Update startIdx old %d -> new %d\n", oldStartIdx, readBufferStartIdx);
//    fflush(stdout);
//  }

  for (int i = 0; i < TraceReadBufferSize; i++) {
    int idx = (readBufferStartIdx + i) % TraceReadBufferSize;
    if (!readBuffer[idx].valid) {
      readBuffer[idx].valid = true;
      bool ret = read(readBuffer[idx].inst);

      if (!ret) { return false; }
      else {
//        printf("Prepare read idx %d", idx);
//        readBuffer[idx].inst.dump();
//        fflush(stdout);
      }
    }
  }
  return true;
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
  * 3. clear readBuffer
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

  // clear readBuffer
  readBufferStartIdx = 0;
  for (int i = 0; i < TraceReadBufferSize; i++) {
    readBuffer[i].valid = false;
  }
}

void TraceReader::collectCommit(uint64_t pc, uint32_t inst, uint8_t instNum, uint8_t idx) {
  commitBuffer[idx].valid = true;
  commitBuffer[idx].inst.instr_pc = pc;
  commitBuffer[idx].inst.instr = inst;
  commitBuffer[idx].inst.instNum = instNum;
//  printf("Trace CollectCommit %d", idx);
//  commitBuffer[idx].inst.dump();
//  fflush(stdout);
}

void TraceReader::collectDrive(uint64_t pc, uint32_t inst, uint8_t idx) {
  driveBuffer[idx].valid = true;
  driveBuffer[idx].inst.instr_pc = pc;
  driveBuffer[idx].inst.instr = inst;
  driveBuffer[idx].inst.instNum = 1; // dontCare
//  printf("Trace CollectDrive %d", idx);
//  driveBuffer[idx].inst.dump();
//  fflush(stdout);
}

void TraceReader::checkCommit() {
  for (int i = 0; i < TraceCommitBufferSize && commitBuffer[i].valid; i ++) {
    commitBuffer[i].valid = false;
    uint64_t pc = commitBuffer[i].inst.instr_pc;
    uint32_t instn = commitBuffer[i].inst.instr;
    uint8_t instNum = commitBuffer[i].inst.instNum;

    if (isError() || isStuck() || isErrorDrive()) {
      return ;
    }

    if (pendingInstList.size() < instNum) {
      setError();
      printf("TraceRTL pendingInstList size %zu less then instNum %d\n", pendingInstList.size(), instNum);
    }
    if ((pendingInstList.front().static_inst.instr_pc_va != pc) ||
        (pendingInstList.front().static_inst.instr != instn)) {
      setError();
      printf("TraceRTL Commit Mismatch\n");
    }

    if (isError()) {
      errorInst.instr = instn;
      errorInst.instr_pc = pc;
      errorInst.instNum = instNum;
      errorInst.instID = commit_inst_num.get();
      return ;
    }

    for (int i = 0; i < instNum; i++) {
      committedInstList.push_back(pendingInstList.front());
      if (committedInstList.size() > CommittedInstSize) {
        committedInstList.pop_front();
      }
      pendingInstList.pop_front();
    }

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
  }
}

void TraceReader::checkDrive() {
  for (int i = 0; (i < TraceDriveBufferSize) && driveBuffer[i].valid; i ++) {
    driveBuffer[i].valid = false;
    uint64_t pc = driveBuffer[i].inst.instr_pc;
    uint32_t inst = driveBuffer[i].inst.instr;

    if (isError() || isStuck() || isErrorDrive()) {
      return ;
    }
    if (driveInstInput.empty()) {
      setErrorDrive();
      printf("DriveInstInput empty\n");
      fflush(stdout);
      return ;
    }
    Instruction instBundle = driveInstInput.front();
    if (instBundle.static_inst.instr_pc_va != pc || instBundle.static_inst.instr != inst) {
      setErrorDrive();
      printf("DriveInstInput not match\n");
      fflush(stdout);
      errorInst.instr = inst;
      errorInst.instr_pc = pc;
      errorInst.instNum = 1;
      errorInst.instID = instBundle.inst_id;
      return ;
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
  }
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