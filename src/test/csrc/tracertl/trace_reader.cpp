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
#include "trace_decompress.h"

TraceReader::TraceReader(const char *trace_file_name)
{
  printf("TraceRTL: check file exists %s...\n", trace_file_name);
  fflush(stdout);
  // preread trace
  trace_stream = new std::ifstream(trace_file_name, std::ios_base::in);
  if ((!trace_stream->is_open())) {
    printf("[TraceReader.TraceReader] Could not open file.\n");
    exit(1);
  }

  // read file into buffer;
  printf("TraceRTL: read tracefile...\n");
  trace_stream->seekg(0, std::ios::end);
  std::streampos fileSize = trace_stream->tellg();
  trace_stream->seekg(0, std::ios::beg);
  char *fileBuffer = new char[fileSize];
  trace_stream->read(fileBuffer, fileSize);
  trace_stream->close();
  delete trace_stream;

  if (fileSize == 0 ) {
    std::cerr << "FileSize Error, should not be zero" << std::endl;
    exit(1);
  }

  // decompress
  printf("TraceRTL: decompress tracefile...\n");
  size_t sizeAfterDC = traceDecompressSizeZSTD(fileBuffer, fileSize);
  if ((sizeAfterDC % sizeof(TraceInstruction)) != 0) {
    printf("Trace file decompress result wrong. sizeAfterDC cannot be divide exactly.\n");
    exit(0);
  }

  TraceInstruction *instDecompressBuffer = new TraceInstruction[sizeAfterDC / sizeof(TraceInstruction)];
  uint64_t decompressedSize =
    traceDecompressZSTD((char *)instDecompressBuffer, sizeAfterDC, fileBuffer, fileSize);
  uint64_t traceInstNum = decompressedSize / sizeof(TraceInstruction);
  delete[] fileBuffer;

  if (decompressedSize != sizeAfterDC) {
    std::cerr << "TraceRTL: Error of Decompress. Decompress size not match "
              << sizeAfterDC << " " << decompressedSize << std::endl;
    exit(1);
  }

  // pre-process the trace inst;
  printf("TraceRTL: preparse/precheck tracefile...\n");
  TraceCounter inst_id_preread;
  inst_id_preread.pop(); // set init id to 1
  commit_inst_num.pop(); // set init id to 1

  if (instDecompressBuffer[0].instr_pc_va != RESET_VECTOR) {
    TraceInstruction static_inst;
    static_inst.setToForceJump(RESET_VECTOR, instDecompressBuffer[0].instr_pc_va);
    Instruction inst;
    inst.fromTraceInst(static_inst);
    inst.inst_id = inst_id_preread.pop();

    if (!inst.legalInst()) {
      setError();
      printf("TraceRTL preread: read from trace file, but illegal inst. Dump the inst:\n");
      inst.dump();
    }

    // construct trace
    instList_preread.push(inst);
  }

  for (uint64_t idx = 0; idx < traceInstNum; idx ++) {
    TraceInstruction static_inst = instDecompressBuffer[idx];
    Instruction inst;
    inst.fromTraceInst(static_inst);
    inst.inst_id = inst_id_preread.pop();

    if (!inst.legalInst()) {
      setError();
      printf("TraceRTL preread: read from trace file, but illegal inst. Dump the inst:\n");
      inst.dump();
    }

    // construct trace_icache
    trace_icache->constructICache(inst.instr_pc_va, inst.instr);
    trace_icache->constructSoftTLB(inst.instr_pc_va, 0, 0, inst.instr_pc_pa);
    if (inst.memory_type != MEM_TYPE_None) {
      trace_icache->constructSoftTLB(inst.exu_data.memory_address.va, 0, 0, inst.exu_data.memory_address.pa);
    }

    // construct trace
    instList_preread.push(inst);
  }
  printf("TraceRTL: preparse tracefile finished total 0x%08lx(0d%08lu) insts...\n", inst_id_preread.get(), inst_id_preread.get());
  fflush(stdout);
  delete[] instDecompressBuffer;

  last_interval_time = gen_cur_time();
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

  if (pendingInstList.size() > 2000) {
    setError();
    printf("TraceRTL: pendingInstList has too many inst, more than 2000. Check it.\n");
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
      printf("Redirect Error: inst_id 0x%lx not in the pendingInstList range [0x%lx, 0x%lx]\n",
       inst_id, pendingInstList.front().inst_id, pendingInstList.back().inst_id);
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

void TraceReader::checkCommit(uint64_t tick) {
  METHOD_TRACE();
  for (int i = 0; i < TraceCommitBufferSize && commitBuffer[i].valid; i ++) {
    // RTL should not commit exception/interrupt/traceCtrl
    // So skip the check.
    if (i == 0) {
      while (!pendingInstList.empty()) {
        if (pendingInstList.front().isCtrlForceJump()) {
          pendingInstList.pop_front();
        } else {
          break;
        }
      }
    }
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
    if ((!pendingInstList.front().pc_va_match(pc)) ||
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

  update_tick(tick);

#ifdef PRINT_SIMULATION_SPEED
  if (commit_inst_num.get() > next_print_inst) {
    uint64_t cur_time = gen_cur_time();
    uint64_t time_delta = cur_time - last_interval_time + 1; // plus 1 to avoid divid 0
    uint64_t cycle_delta = tick - last_interval_tick;
    uint64_t inst_delta = commit_inst_num.get() - last_interval_inst;

    uint64_t cycle_per_second = cycle_delta / time_delta;
    uint64_t inst_per_second = inst_delta / time_delta;

    last_interval_time = cur_time;
    last_interval_tick = tick;
    last_interval_inst = commit_inst_num.get();
    next_print_inst += PRINT_INST_INTERVAL;

    printf("Current Finished Inst Num %ld. Simulation Speed: %ld cycle/s %ld inst/s\r ", commit_inst_num.get(), cycle_per_second, inst_per_second);
    fflush(stdout);

  }
#endif
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
    if (!instBundle.pc_va_match(pc) || instBundle.instr != inst) {
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
  if (commit_inst_num.get() == 1) threa = FIRST_BLOCK_THREASHOLD;
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
