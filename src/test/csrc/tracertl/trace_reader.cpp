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
#include <cstdlib>
#include "trace_reader.h"
#include "tracertl.h"
#include "trace_decompress.h"

#include <tracertl_dut_info.h>

TraceReader::TraceReader(const char *trace_file_name, bool enable_gen_paddr, uint64_t max_insts, uint64_t skip_traceinstr)
{
  pre_readfile(trace_file_name, skip_traceinstr);
  mid_construct(max_insts, enable_gen_paddr);
  post_opt();

  printf("[TraceRTL] TraceReader Finished.\n");
  fflush(stdout);

  last_interval_time = gen_cur_time();
}

void TraceReader::pre_readfile(const char *trace_file_name, uint64_t skip_traceinstr) {
  printf("[TraceRTL] check file exists...\n");
  fflush(stdout);
  // preread trace
  std::ifstream *trace_stream;
  trace_stream = new std::ifstream(trace_file_name, std::ios_base::in);
  if ((!trace_stream->is_open())) {
    printf("[TraceReader.TraceReader] Could not open file.\n");
    exit(1);
  }

  // read file into buffer;
  printf("[TraceRTL] read tracefile...\n");
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
  printf("[TraceRTL] decompress tracefile...\n");
  size_t sizeAfterDC = traceDecompressSizeZSTD(fileBuffer, fileSize);
  if ((sizeAfterDC % sizeof(TraceInstruction)) != 0) {
    printf("Error: Trace file decompress result wrong. sizeAfterDC cannot be divide exactly.\n");
    exit(1);
  }

  TraceInstruction *instDecompressBuffer = new TraceInstruction[sizeAfterDC / sizeof(TraceInstruction)];
  uint64_t decompressedSize =
    traceDecompressZSTD((char *)instDecompressBuffer, sizeAfterDC, fileBuffer, fileSize);
  uint64_t traceInstNum = decompressedSize / sizeof(TraceInstruction);
  delete[] fileBuffer;

  printf("[TraceRTL] Read %lu instructions from trace file.\n", traceInstNum);
  if (skip_traceinstr > 0) {
    printf("[TraceRTL] Skip %lu instructions.\n", skip_traceinstr);
  }
  if (traceInstNum <= skip_traceinstr) {
    printf("[TraceRTL] Skip all the instructions. Exit\n");
    exit(1);
  }

  if (decompressedSize != sizeAfterDC) {
    std::cerr << "[TraceRTL] Error of Decompress. Decompress size not match "
              << sizeAfterDC << " " << decompressedSize << std::endl;
    exit(1);
  }

  // pre-process the trace inst;
  printf("[TraceRTL] preparse/precheck tracefile...\n");
  TraceCounter inst_id_preread;
  inst_id_preread.pop(); // set init id to 1
  commit_inst_num.pop(); // set init id to 1

  size_t start_instr_index = skip_traceinstr;

  if (instDecompressBuffer[start_instr_index].instr_pc_va != RESET_VECTOR) {
    // gen jump instr
    TraceInstruction static_inst;
    static_inst.setToForceJump(RESET_VECTOR, instDecompressBuffer[start_instr_index].instr_pc_va);
    Instruction inst;
    inst.fromTraceInst(static_inst);
    inst.inst_id = inst_id_preread.pop();
    instList_preread.push_back(inst);
  }

  for (uint64_t idx = start_instr_index; idx < traceInstNum; idx ++) {
    // trans to XS Trace Format
    TraceInstruction static_inst = instDecompressBuffer[idx];
    Instruction inst;
    inst.fromTraceInst(static_inst);
    inst.inst_id = inst_id_preread.pop();
    instList_preread.push_back(inst);
  }
  printf("[TraceRTL] preread trace file finished.\n");
  fflush(stdout);

  delete[] instDecompressBuffer;
}

void TraceReader::mid_construct(uint64_t max_insts, bool enable_gen_paddr) {
  // gen paddr
  if (enable_gen_paddr) {
    for (auto &inst : instList_preread) {
      inst.instr_pc_pa = iPaddrAllocator.va2pa(inst.instr_pc_va);
      if (inst.memory_type != MEM_TYPE_None) {
        inst.exu_data.memory_address.pa = dPaddrAllocator.va2pa(inst.exu_data.memory_address.va);
      }
    }
    printf("[TraceRTL] gen paddr finished.\n");
    fflush(stdout);
  }

#ifdef TRACE_VERBOSE
  iPaddrAllocator.dump();
  dPaddrAllocator.dump();
#endif

  // check legal
  for (auto inst : instList_preread) {
    if (!inst.legalInst()) {
      printf("[TraceRTL] read from trace file, but illegal inst. Dump the inst:\n");
      inst.dump();
      exit(1);
    }
  }
  printf("[TraceRTL] check legal inst finished.\n");
  fflush(stdout);

  TraceLegalFlowChecker flowChecker;
  flowChecker.check(instList_preread);
  printf("[TraceRTL] flow checker finished.\n");
  fflush(stdout);


  auto traceInstNum = instList_preread.size();
  printf("[TraceRTL] preparse tracefile finished total 0x%08lx(0d%08lu) insts.\n", traceInstNum, traceInstNum);
  if (traceInstNum < max_insts) {
    printf("[TraceRTL] inst_id_preread %lu < max_insts %lu, adjust max_insts\n", traceInstNum, max_insts);
  }
  act_max_insts = std::min(act_max_insts, traceInstNum);
  fflush(stdout);

  trace_icache->construct(instList_preread);
  printf("[TraceRTL] TraceICache Constructed.\n");
  fflush(stdout);

}

void TraceReader::post_opt() {
  if (!trace_fastsim->isFastSimMemoryFinished()) {
    trace_fastsim->mergeMemAddr(instList_preread, act_max_insts);
    printf("[TraceRTL] FastSim Memory Part Finished.\n");
    fflush(stdout);
  }
  if (!trace_fastsim->isFastSimInstFinished()) {
    trace_fastsim->instDedup.dedup(instList_preread,
      0, trace_fastsim->getWarmupInstNum());
    printf("[TraceRTL] FastSim Instruction Part Finished.\n");
    fflush(stdout);
  }
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
#ifdef TRACERTL_FPGA
  printf("[TraceRTL] prepareRead should not be called in FPGA mode\n");
  exit(1);
#endif

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
  } else {
    while(instReadIdx < instList_preread.size() && instList_preread[instReadIdx].is_squashed) {
      instReadIdx ++;
    }
    if (instReadIdx < instList_preread.size()) {
      METHOD_TRACE();
      // inst = instList_preread.front();
      inst = instList_preread[instReadIdx++];
      // inst.fast_simulation = trace_fastsim->isFastSimInstFinished() ? 0 : 1;
      inst.fast_simulation = trace_fastsim->isFastSimInstByIdx(instReadIdx) ? 1 : 0;

      if (!trace_fastsim->isFastSimInstFinished() && (instReadIdx >= trace_fastsim->getWarmupInstNum())) {
        printf("Set FastSim Inst Finish at Fetch end\n");
        trace_fastsim->setFastsimInstFinish();
      }

      // instList_preread.pop();
      counterReadFromInstList ++;

      // dynamic update constructedICache to hanlder thread changes
      trace_icache->constructICache(inst.instr_pc_va, inst.instr);
    } else {
      setOver();
      memset(reinterpret_cast<char *> (&inst), 0, sizeof(Instruction));
      METHOD_TRACE();
      return false;
    }
  }

  pendingInstList.push_back(inst); // for commit check
  driveInstInput.push_back(inst); // for ibuffer drive check

#ifndef TRACERTL_FPGA
  if (pendingInstList.size() > 2000) {
    setError();
    printf("[TraceRTL] pendingInstList has too many inst, more than 2000. Check it.\n");
  }
#endif

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
          commit_inst_num.add(1); // force jump also has unique inst_id
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

  static bool lastAvoidStuck = false;
  if (trace_fastsim->avoidInstStuck()) lastAvoidStuck = true;
  if (lastAvoidStuck && !trace_fastsim->avoidInstStuck()) {
    printf("[TraceRTL] FastSim avoidInstStuck finished tick: %ld\n", tick);
    last_commit_tick = tick; // when fastsim finished, update the last_commit_tick, to pass stuck check
    lastAvoidStuck = false;
  }
  if (!trace_fastsim->avoidInstStuck()) {
    check_tracertl_timeout(tick);
  }

#ifdef PRINT_SIMULATION_SPEED
  if (commit_inst_num.get() > next_print_inst) {
    uint64_t cur_time = gen_cur_time();
    uint64_t time_delta = cur_time - last_interval_time + 1; // plus 1 to avoid divid 0
    uint64_t cycle_delta = tick - last_interval_tick;
    uint64_t inst_delta = commit_inst_num.get() - last_interval_inst;

    uint64_t cycle_per_second = cycle_delta / time_delta;
    uint64_t inst_per_second = inst_delta / time_delta;

    static int slow_count = 0;
    if (cycle_per_second < 500) { slow_count ++; }
    else { slow_count = 0; }

    last_interval_time = cur_time;
    last_interval_tick = tick;
    last_interval_inst = commit_inst_num.get();
    next_print_inst += PRINT_INST_INTERVAL;

    printf("\rCurrent Finished Inst Num %ld. Simulation Speed: %ld cycle/s %ld inst/s", commit_inst_num.get(), cycle_per_second, inst_per_second);
    fflush(stdout);

    if (slow_count > 3) {
      printf("\n");
      printf("  Slow Simulation Speed, may conflict with other process\n");
      setEmuConflict();
    }
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

void TraceReader::read_by_axis(char *tvalid, char *last, uint64_t *validVec,
    uint64_t *data0, uint64_t *data1, uint64_t *data2, uint64_t *data3,
    uint64_t *data4, uint64_t *data5, uint64_t *data6, uint64_t *data7) {

  static_assert(TRACERTL_FPGA_AXIS_WIDTH == 512, "FPGA AXIS width should be 512");

  // 1. read many instruction into a large array. one array, one complete axis-stream packet
  // 2. send the pakcet through multiply transport
  const int busBits = TRACERTL_FPGA_AXIS_WIDTH;
  const int packetInstNum = TRACERTL_FPGA_PACKET_INST_NUM; // instructions number per packet
  const int packetBits = packetInstNum * TRACERTL_INST_BIT_WIDTH; // bytes per packet
  const int maxReadCycles = (packetBits + busBits - 1) / busBits; // cycles per packet
  const int maxReadBytes = (maxReadCycles * busBits) / 8;
  const uint64_t lastValidVec = ~0ULL;
  static_assert(TRACERTL_FPGA_PACKET_CYCLE_NUM == maxReadCycles, "maxReadCycles should be equal to TRACERTL_FPGA_PACKET_CYCLE_NUM");

  static bool array_valid = false;
  static Instruction arrayRaw[TRACERTL_FPGA_PACKET_INST_NUM];
  static uint64_t arrayLongType[maxReadBytes / 8];
  static int read_cycle_num = 0;
  static uint64_t packet_count = 0;
  static uint64_t total_read_cycle = 0;
  TraceFpgaInstruction *arrayFPGA = (TraceFpgaInstruction *)arrayLongType;

  static int first_time_dump = true;

  if (!array_valid) {
    array_valid = true;

    for (int i = 0; i < packetInstNum; i ++) {
      read(arrayRaw[i]);
      arrayFPGA[i].genFrom(arrayRaw[i]);
    }
    if (first_time_dump) {
      first_time_dump = false;
    }
  }

  int index = read_cycle_num << 3;
  *data0 = arrayLongType[index + 0];
  *data1 = arrayLongType[index + 1];
  *data2 = arrayLongType[index + 2];
  *data3 = arrayLongType[index + 3];
  *data4 = arrayLongType[index + 4];
  *data5 = arrayLongType[index + 5];
  *data6 = arrayLongType[index + 6];
  *data7 = arrayLongType[index + 7];
  read_cycle_num += 1;
  bool lastCycle = read_cycle_num == maxReadCycles;
  *validVec = lastCycle ? lastValidVec : ~0ULL;
  *last = lastCycle;
  *tvalid = 1;

  if (lastCycle) {
    array_valid = false;
    read_cycle_num = 0;
    packet_count ++;
  }
  total_read_cycle ++;
}

void TraceReader::check_by_axis(char last, uint64_t valid,
  uint64_t data0, uint64_t data1, uint64_t data2, uint64_t data3,
  uint64_t data4, uint64_t data5, uint64_t data6, uint64_t data7) {

  const int collectBits = TRACERTL_FPGA_COLLECT_INST_WIDTH * TRACERTL_FPGA_COLLECT_INST_NUM;
  const int maxCycles = (collectBits + TRACERTL_FPGA_AXIS_WIDTH - 1) / TRACERTL_FPGA_AXIS_WIDTH;
  const int maxDWords = (maxCycles * TRACERTL_FPGA_AXIS_WIDTH) / 64;

  static_assert(TRACERTL_FPGA_AXIS_WIDTH == 512, "FPGA AXIS width should be 512");
  static_assert(TRACERTL_FPGA_COLLECT_CYCLE_NUM == maxCycles, "maxCycles should be equal to TRACERTL_FPGA_COLLECT_CYCLE_NUM");

  static int cycle_count = 0;
  static int actual_validBits_num = 0;
  static uint64_t collectBuffer[maxDWords];
  static uint64_t packet_count = 0;

  int index = cycle_count << 3;
  collectBuffer[index + 0] = data0;
  collectBuffer[index + 1] = data1;
  collectBuffer[index + 2] = data2;
  collectBuffer[index + 3] = data3;
  collectBuffer[index + 4] = data4;
  collectBuffer[index + 5] = data5;
  collectBuffer[index + 6] = data6;
  collectBuffer[index + 7] = data7;
  cycle_count += 1;
  bool lastCycle = (cycle_count == maxCycles);

  if (cycle_count > maxCycles) {
    printf("TraceRTL Commit cycle_count %d exceed max %d\n", cycle_count, maxCycles);
    fflush(stdout);
    setError();
    return ;
  }

  if (last) {
    TraceFpgaCollectStruct *committedInst = (TraceFpgaCollectStruct *)collectBuffer;

    packet_count ++;
    cycle_count = 0;
    actual_validBits_num = 0;

    // below is same with the old checkCommit

    for (int i = 0; i < TRACERTL_FPGA_COLLECT_INST_NUM; i ++) {
      if (i == 0) {
        while (!pendingInstList.empty()) {
          if (pendingInstList.front().isCtrlForceJump()) {
            pendingInstList.pop_front();
          } else {
            break;
          }
        }
      }

      uint64_t pc = committedInst[i].pcVA;
      uint64_t instNum = committedInst[i].instNum;

      if (isError() || isStuck() || isErrorDrive()) {
        return ;
      }

      if (pendingInstList.size() < instNum) {
        setError();
        printf("TraceRTL pendingInstList size %zu less then instNum %lu\n", pendingInstList.size(), instNum);
      }
      if ((!pendingInstList.front().pc_va_match(pc))) {
        setError();
        printf("TraceRTL Commit PC Mismatch\n");
        printf("Expect PC %lx, Get PC %lx\n", pendingInstList.front().instr_pc_va, pc);
      }

      if (isError()) {
        errorInst.instr = 0; // invalid
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
      dut.instr = 0;
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
}

// replace CheckCommit in FPGA mode
void TraceReader::checkCommitFPGA(uint64_t tick) {

  // when error, may segmentation fault, not resolved
  if (isError()) {
    return ;
  }

  if (isCommited()) {
    last_commit_tick = tick;
    clearCommit();
  } else {
    if ((tick - last_commit_tick) >= BLOCK_THREASHOLD) {
      setStuck();
    }
  }

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
}


void TraceReader::dump_uncommited_inst() {
  printf("UnCommitted Inst: ========================\n");
  int count = 0;
  for (auto inst : pendingInstList) {
    if (count > 400) { break; }
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
  return redirectInstList.empty() && (instReadIdx == instList_preread.size());
}

bool TraceReader::check_tracertl_timeout(uint64_t tick) {
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
    printf("========= TraceRTL Error at inst 0x%lx (may invalid)===========\n", commit_inst_num.get());
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
