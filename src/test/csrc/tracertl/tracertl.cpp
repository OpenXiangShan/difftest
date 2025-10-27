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

#include <cstdio>
#include <cstdlib>
#include "tracertl.h"
#include "trace_format.h"
#include "trace_reader.h"
#include "trace_writer.h"
#include "trace_icache.h"
#include "trace_fastsim.h"

TraceReader *trace_reader = NULL;

/** Used By Emulator */

void init_tracertl(const char *trace_file_name, bool enable_gen_paddr, uint64_t max_insts, uint64_t skip_traceinstr) {
  printf("init_tracertl: %s\n", trace_file_name);
  printf("dynamic generate paddr(for instruction and data): %s\n", enable_gen_paddr ? "enable" : "disable");
  printf("skip-insts %lu left_max_insts: %lu\n", skip_traceinstr, max_insts);
  fflush(stdout);
  trace_reader = new TraceReader(trace_file_name, enable_gen_paddr, max_insts, skip_traceinstr);

#ifndef TRACERTL_FPGA
  printf("prepare read at init_tracertl\n");
  fflush(stdout);
  trace_reader->prepareRead();
#endif
}
bool tracertl_prepare_read() {
  return trace_reader->prepareRead();
}

void tracertl_check_commit(uint64_t tick) {
  trace_reader->checkCommit(tick);
}

void tracertl_check_commit_fpga(uint64_t tick) {
  trace_reader->checkCommitFPGA(tick);
}

void tracertl_check_drive() {
  trace_reader->checkDrive();
}

bool tracertl_over() {
  return trace_reader->isOver();
}

bool tracertl_error() {
  return trace_reader->isError();
}

bool tracertl_error_drive() {
  return trace_reader->isErrorDrive();
}

bool tracertl_stuck() {
  return trace_reader->isStuck();
}

bool tracertl_emu_conflict() {
  return trace_reader->isEmuConflict();
};

void tracertl_error_dump() {
  trace_reader->error_dump();
}

void tracertl_assert_dump() {
  trace_reader->assert_dump();
}

void tracertl_error_drive_dump() {
  trace_reader->error_drive_dump();
}

void tracertl_success_dump() {
  trace_reader->success_dump();
}

/*
 * TraceICache init and DPI-C Helper
 **/
// Not Used
void __attribute__((noinline))  trace_read_insts(uint8_t enable, ManyInstruction_t *manyInsts) {
  METHOD_TRACE();
  // ManyInstruction_t *manyInsts = insts;
  if (enable != 0) {
    for (int i = 0; i < TraceFetchWidth; i ++) {
      if (trace_reader->traceOver()) {
        printf("trace_read_one_instr: traceOver. Finish\n");
        trace_reader->setOver();
        // TODO: insert nop
        return ;
        // return manyInsts;
      }
      trace_reader->readFromBuffer(manyInsts->insts[i], i);
      manyInsts->insts[i].instr_pc_pa = manyInsts->insts[i].instr_pc_pa == 0 ?
        manyInsts->insts[i].instr_pc_va : manyInsts->insts[i].instr_pc_pa;
      manyInsts->insts[i].exu_data.memory_address.pa = manyInsts->insts[i].exu_data.memory_address.pa == 0 ?
        manyInsts->insts[i].exu_data.memory_address.va : manyInsts->insts[i].exu_data.memory_address.pa;
    }
  }
  METHOD_TRACE();
  return ;
}

extern "C" void trace_read_one_instr(
  uint64_t *pc_va, uint64_t *pc_pa, uint64_t *memory_addr_va, uint64_t *memory_addr_pa,
  uint64_t *target, uint32_t *instr,
  uint8_t *memory_type, uint8_t *memory_size,
  uint8_t *branch_type, uint8_t *branch_taken,
  uint8_t *exception, uint8_t *fast_simulation,
  uint64_t *InstID, uint8_t idx) {
  METHOD_TRACE();

  Instruction inst;
  trace_reader->readFromBuffer(inst, idx);
  // printf("TraceRead idx %d", idx);
  // inst.dump();
  // fflush(stdout);

  *pc_va = inst.instr_pc_va;
  *pc_pa = inst.instr_pc_pa == 0 ? inst.instr_pc_va : inst.instr_pc_pa;
  *memory_addr_va = inst.exu_data.memory_address.va;
  *memory_addr_pa = inst.exu_data.memory_address.pa == 0 ? inst.exu_data.memory_address.va : inst.exu_data.memory_address.pa;
  *target = inst.target;
  *instr = inst.instr;
  *memory_type = inst.memory_type;
  *memory_size = inst.memory_size;
  *branch_type = inst.branch_type;
  *branch_taken = inst.branch_taken;
  *exception = inst.exception;
  *fast_simulation = inst.fast_simulation;
  *InstID = inst.inst_id;
  METHOD_TRACE();
}

extern "C" void trace_redirect(uint64_t inst_id) {
  METHOD_TRACE();
  trace_reader->redirect(inst_id);
}

extern "C" void trace_collect_commit(uint64_t pc, uint32_t instr, uint8_t instNum, uint8_t idx) {
  METHOD_TRACE();
  trace_reader->collectCommit(pc, instr, instNum, idx);
}

extern "C" void trace_collect_drive(uint64_t pc, uint32_t instr, uint8_t idx) {
  METHOD_TRACE();
  trace_reader->collectDrive(pc, instr, idx);
}

// when not in fpga mode, disable functin declare
#ifdef TRACERTL_FPGA
extern "C" void trace_axis_master_helper(
  char *tvalid, char *last, uint64_t *valid,
  uint64_t *data0, uint64_t *data1, uint64_t *data2, uint64_t *data3,
  uint64_t *data4, uint64_t *data5, uint64_t *data6, uint64_t *data7
) {
  METHOD_TRACE();
  trace_reader->read_by_axis(
    tvalid, last, valid,
    data0, data1, data2, data3,
    data4, data5, data6, data7
  );
}

extern "C" void trace_axis_slave_helper(
  char last, uint64_t valid,
  uint64_t data0, uint64_t data1, uint64_t data2, uint64_t data3,
  uint64_t data4, uint64_t data5, uint64_t data6, uint64_t data7
) {
  METHOD_TRACE();
  trace_reader->check_by_axis(
    last, valid,
    data0, data1, data2, data3,
    data4, data5, data6, data7
  );
}
#endif // TRACERTL_FPGA

/** Fake ICache */
TraceICache *trace_icache = NULL;

void init_traceicache(const char *tracept_file) {
  trace_icache = new TraceICache(tracept_file);
}

extern "C" uint64_t trace_icache_dword_helper(uint64_t addr) {
  uint64_t data;
  METHOD_TRACE();
  trace_icache->readDWord(data, addr);
  return data;
}

extern "C" uint64_t trace_dyn_pt_dword_helper(uint64_t addr) {
  METHOD_TRACE();
  uint64_t data = trace_icache->dynPageRead(addr);
  // printf("[TraceDynPT] addr: 0x%lx, data: 0x%016lx\n", addr, data);
  return data;
}

extern "C" uint8_t trace_icache_legal_addr(uint64_t addr) {
  return 1;
}

extern "C" uint64_t trace_tlb_ats(uint64_t vaddr, uint16_t asid, uint16_t vmid) {
  // asid/vmid is not used.
  METHOD_TRACE();
  return trace_icache->addrTrans(vaddr, 0, 0);
}

extern "C" bool trace_tlb_ats_hit(uint64_t vaddr, uint16_t asid, uint16_t vmid) {
  // asid/vmid is not used.
  METHOD_TRACE();
  return trace_icache->addrTrans_hit(vaddr, 0, 0);
}

extern "C" uint64_t trace_get_satp_ppn() {
  METHOD_TRACE();
  return trace_icache->getSatpPpn();
};

/******* Trace Fast Sim Checker ******/
TraceFastSimManager *trace_fastsim = NULL;

void init_tracefastsim(bool fastwarmup_enable, uint64_t warmup_inst) {
  trace_fastsim = new TraceFastSimManager(fastwarmup_enable, warmup_inst);
}

/** Called by dut*/
extern "C" uint8_t tracertl_get_fastsim_state() {
  return (uint8_t)trace_fastsim->get_fastsim_state();
}

extern "C" void trace_fastsim_mem_addr_reader(
  uint8_t idx,
  uint8_t* valid,
  uint64_t* vaddr, uint64_t* paddr) {
  METHOD_TRACE();

  trace_fastsim->read_mem_addr(idx, valid, vaddr, paddr);
}

/** Called by Emulator */
void tracertl_prepare_fastsim_memaddr() {
  METHOD_TRACE();
  trace_fastsim->prepareMemAddrBuffer();
}
