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
#include "tracertl.h"

TraceReader::TraceReader(std::string trace_file_name)
{
  // preread trace
  trace_stream_preread = new std::ifstream(trace_file_name, std::ios_base::in);
  if ((!trace_stream_preread->is_open())) {
    std::ostringstream oss;
    oss << "[TraceReader.TraceReader] preread. Could not open file: " << trace_file_name;
    throw std::runtime_error(oss.str());
  }
  inst_id_preread.pop(); // set init id to 1
  commit_inst_num.pop(); // set init id to 1

  printf("TraceRTL: preread tracefile...\n");
  fflush(stdout);
  while (!trace_stream_preread->eof()) {
    TraceInstruction static_inst;
    Instruction inst;
    trace_stream_preread->read(reinterpret_cast<char *> (&static_inst), sizeof(TraceInstruction));
    inst.fromTraceInst(static_inst);
    inst.inst_id = inst_id_preread.pop();

    if (!inst.legalInst()) {
      setError();
      printf("TraceRTL preread: read from trace file, but illegal inst. Dump the inst:\n");
      inst.dump();
    }

    // construct trace_icache
    trace_icache->constructICache(inst.instr_pc_va, inst.instr);
    if (inst.instr_pc_pa != 0)
      trace_icache->constructSoftTLB(inst.instr_pc_va, 0, 0, inst.instr_pc_pa);
    if (inst.memory_address_pa != 0)
      trace_icache->constructSoftTLB(inst.memory_address_va, 0, 0, inst.memory_address_pa);

    // construct trace
    instList_preread.push(inst);
  }
  printf("TraceRTL: preread tracefile finished total 0x%08lx(0d%08lu) insts...\n", inst_id_preread.get(), inst_id_preread.get());
  fflush(stdout);
  trace_stream_preread->close();
  delete trace_stream_preread;
}

bool TraceReader::readFromBuffer(Instruction &inst, uint8_t idx) {
  METHOD_TRACE();
  inst = readBuffer[idx];
  readBufferNeedReload = true;
//  printf("TraceReadBuffer %d+%d->newIdx:%d", idx, readBufferStartIdx, idx_inner);
//  inst.dump();
//  fflush(stdout);
  METHOD_TRACE();
  return true;
}

bool TraceReader::prepareRead() {
  METHOD_TRACE();
  if (!readBufferNeedReload) {
    METHOD_TRACE();
    return true;
  }
  readBufferNeedReload = false;

  for (int i = 0; i < TraceReadBufferSize; i++) {
    bool ret = read(readBuffer[i]);

    if (!ret) {
      METHOD_TRACE();
      return false;
    } else {
//      printf("Prepare read idx %d", i);
//      readBuffer[i].dump();
//      fflush(stdout);
    }
  }
  METHOD_TRACE();
  return true;
}

bool TraceReader::read(Instruction &inst) {
  // read a inst from redirect buffer
  METHOD_TRACE();
  if (!redirectInstList.empty()) {
    METHOD_TRACE();
    inst = redirectInstList.front();
    redirectInstList.pop_front();
    counterReadFromRedirect ++;
  } else if (!instList_preread.empty()) {
    METHOD_TRACE();
    inst = instList_preread.front();
    instList_preread.pop();
    counterReadFromInstList ++;
  } else {
    setOver();
    memset(reinterpret_cast<char *> (&inst), 0, sizeof(Instruction));
    METHOD_TRACE();
    return false;
  }


  pendingInstList.push_back(inst); // for commit check
  driveInstInput.push_back(inst); // for ibuffer drive check

  if (pendingInstList.size() > 500) {
    setError();
    printf("TraceRTL: pendingInstList has too many inst, more than 1000. Check it.\n");
  }

//  inst.dump();

  METHOD_TRACE();
  return true;
}

/**
  * Redirect the instruction to re-run from inst_id
  * 1. pop inst from pendingInstList(from oldest to inst_id) and push to redirectInstList
  *    pendingInstList : back(old) -> front(young). the front is the youngest inst.
  *    redirectInstList: back(old) -> front(young). the front is the youngest inst.
  *    younger inst has smaller inst_id.
  * 2. flush the driveInstInput
  * 3. re-prepare readBuffer
  */
void TraceReader::redirect(uint64_t inst_id) {
  METHOD_TRACE();
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

  readBufferNeedReload = true;
  METHOD_TRACE();
}

void TraceReader::collectCommit(uint64_t pc, uint32_t inst, uint8_t instNum, uint8_t idx) {
  METHOD_TRACE();
  commitBuffer[idx].valid = true;
  commitBuffer[idx].inst.instr_pc = pc;
  commitBuffer[idx].inst.instr = inst;
  commitBuffer[idx].inst.instNum = instNum;
//  printf("Trace CollectCommit %d", idx);
//  commitBuffer[idx].inst.dump();
//  fflush(stdout);
  METHOD_TRACE();
}

void TraceReader::collectDrive(uint64_t pc, uint32_t inst, uint8_t idx) {
  METHOD_TRACE();
  driveBuffer[idx].valid = true;
  driveBuffer[idx].inst.instr_pc = pc;
  driveBuffer[idx].inst.instr = inst;
  driveBuffer[idx].inst.instNum = 1; // dontCare
//  printf("Trace CollectDrive %d", idx);
//  driveBuffer[idx].inst.dump();
//  fflush(stdout);
  METHOD_TRACE();
}

void TraceReader::checkCommit() {
  METHOD_TRACE();
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
    if ((pendingInstList.front().instr_pc_va != pc) ||
        (pendingInstList.front().instr != instn)) {
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
  METHOD_TRACE();
}

void TraceReader::checkDrive() {
  METHOD_TRACE();
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
    if (instBundle.instr_pc_va != pc || instBundle.instr != inst) {
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
  METHOD_TRACE();
}

void TraceReader::dump_uncommited_inst() {
  printf("UnCommitted Inst: ========================\n");
  int count = 0;
  for (auto inst : pendingInstList) {
    if (count > 100) { break; }
    count ++;
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
  METHOD_TRACE();
  // end of file or add signal into the trace
  return redirectInstList.empty() && instList_preread.empty();
}

bool TraceReader::update_tick(uint64_t tick) {
  METHOD_TRACE();
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
  printf("inst read from instList: %lu from Redirect: %lu\n", counterReadFromInstList, counterReadFromRedirect);
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