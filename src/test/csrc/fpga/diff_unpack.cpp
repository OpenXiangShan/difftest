/***************************************************************************************
* Copyright (c) 2024 Beijing Institute of Open Source Chip (BOSC)
* Copyright (c) 2020-2024 Institute of Computing Technology, Chinese Academy of Sciences
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
#include <string.h>
#include "diffstate.h"
#include "difftest-dpic.h"
#include "diff_unpack.h"

typedef struct  {
  uint8_t  io_index;
  uint8_t  io_coreid;
  uint64_t io_data;
  uint8_t  io_valid;
} SquashCommitData;

typedef struct  {
  uint8_t io_coreid;
  uint8_t io_success;
  uint8_t io_bits_valid;
  uint8_t io_valid;
} SquashLrScEvent;

typedef struct  {
  uint8_t  io_coreid;
  uint64_t io_vsscratch;
  uint64_t io_vsatp;
  uint64_t io_vstval;
  uint64_t io_vscause;
  uint64_t io_vsepc;
  uint64_t io_vstvec;
  uint64_t io_vsstatus;
  uint64_t io_hgatp;
  uint64_t io_htinst;
  uint64_t io_htval;
  uint64_t io_hcounteren;
  uint64_t io_hedele;
  uint64_t io_hideleg;
  uint64_t io_hstatus;
  uint64_t io_mtinst;
  uint64_t io_mtval2;
  uint64_t io_virtMode;
  uint8_t  io_valid;
} SquashHCSRState;

typedef struct  {
  uint8_t  io_coreid;
  uint64_t io_fcsr;
  uint8_t  io_valid;
} SquashFpCSRState;

typedef struct  {
  uint8_t  io_coreid;
  uint64_t io_value[32];
  uint8_t  io_valid;
} SquashArchFpRegState;

typedef struct  {
  uint8_t  io_coreid;
  uint64_t io_value[32];
  uint8_t  io_valid;
} SquashArchIntRegState;

typedef struct  {
  uint8_t  io_coreid;
  uint64_t io_medeleg;
  uint64_t io_mideleg;
  uint64_t io_sscratch;
  uint64_t io_mscratch;
  uint64_t io_mie;
  uint64_t io_mip;
  uint64_t io_satp;
  uint64_t io_scause;
  uint64_t io_mcause;
  uint64_t io_stvec;
  uint64_t io_mtvec;
  uint64_t io_stval;
  uint64_t io_mtval;
  uint64_t io_sepc;
  uint64_t io_mepc;
  uint64_t io_sstatus;
  uint64_t io_mstatus;
  uint64_t io_privilegeMode;
  uint8_t  io_valid;
} SquashCSRState;

typedef struct  {
  uint8_t  io_coreid;
  uint32_t io_interrupt;
  uint32_t io_exception;
  uint64_t io_exceptionPC;
  uint32_t io_exceptionInst;
  uint8_t  io_hasNMI;
  uint8_t  io_virtualInterruptIsHvictlInject;
  uint8_t  io_bits_valid;
  uint8_t  io_valid;
} SquashArchEvent;

typedef struct  {
  uint8_t  coreid;
  uint64_t pc;
  uint64_t code;
  uint8_t  hasWFI;
  uint64_t instrCnt;
  uint64_t cycleCnt;
  uint8_t  hasTrap;
  uint8_t  io_valid;
} SquashTrapEvent;

typedef struct  {
  uint8_t  index;
  uint8_t  coreid;
  uint8_t  special;
  uint8_t  nFused;
  uint8_t  isStore;
  uint8_t  isLoad;
  uint8_t  sqIdx;
  uint8_t  lqIdx;
  uint16_t robIdx;
  uint32_t instr;
  uint64_t pc;
  uint8_t  wdest;
  uint8_t  wpdest;
  uint8_t  vecwen;
  uint8_t  fpwen;
  uint8_t  rfwen;
  uint8_t  isRVC;
  uint8_t  skip;
  uint8_t  valid;
} SquashInstrCommit;


void squash_unpackge(uint8_t *packge) {
  for (size_t i = 0; i < CONFIG_DIFF_COMMIT_DATA_WIDTH; i++) {
    SquashCommitData temp;
    memcpy(&temp, packge, sizeof(SquashCommitData));
    packge += sizeof(SquashCommitData);
    v_difftest_CommitData(temp.io_data, temp.io_coreid, temp.io_index);
  }
  {
    SquashLrScEvent temp;
    memcpy(&temp, packge, sizeof(SquashLrScEvent));
    packge += sizeof(SquashLrScEvent);
    v_difftest_LrScEvent(temp.io_success, temp.io_coreid);
  }
  {
    SquashHCSRState temp;
    memcpy(&temp, packge, sizeof(SquashHCSRState));
    packge += sizeof(SquashHCSRState);
    v_difftest_HCSRState(temp.io_virtMode, temp.io_mtval2, temp.io_mtinst,
    temp.io_hstatus, temp.io_hideleg, temp.io_hedele, temp.io_hcounteren,
    temp. io_htval, temp.io_htinst, temp.io_hgatp, temp.io_vsstatus,
    temp.io_vstvec, temp.io_vsepc, temp.io_vscause, temp.io_vstval,
    temp.io_vsatp, temp.io_vsscratch, temp.io_coreid);
  }
  {
    SquashFpCSRState temp;
    memcpy(&temp, packge, sizeof(SquashFpCSRState));
    packge += sizeof(SquashFpCSRState);
    v_difftest_FpCSRState(temp.io_fcsr, temp.io_coreid);
  }
  {
    SquashCSRState temp;
    memcpy(&temp, packge, sizeof(SquashCSRState));
    packge += sizeof(SquashCSRState);
    v_difftest_CSRState(temp.io_privilegeMode, temp.io_mstatus, temp.io_sstatus, temp.io_mepc,
    temp.io_sepc, temp.io_mtval, temp.io_stval, temp.io_mtvec, temp.io_stvec, temp.io_mcause, temp.io_scause,
    temp.io_satp, temp.io_mip, temp.io_mie, temp.io_mscratch, temp.io_sscratch, temp.io_mideleg, temp.io_medeleg,
    temp.io_coreid);
  }
  {
    SquashArchEvent temp;
    memcpy(&temp, packge, sizeof(SquashArchEvent));
    packge += sizeof(SquashArchEvent);
    v_difftest_ArchEvent(temp.io_interrupt, temp.io_exception, temp.io_exceptionPC, temp.io_exceptionInst,
    temp.io_hasNMI, temp.io_virtualInterruptIsHvictlInject, temp.io_coreid);
  }
  {
    SquashArchIntRegState temp;
    memcpy(&temp, packge, sizeof(SquashArchIntRegState));
    packge += sizeof(SquashArchIntRegState);
    v_difftest_ArchIntRegState(temp.io_value[0], temp.io_value[1], temp.io_value[2], temp.io_value[3], temp.io_value[4], temp.io_value[5],
    temp.io_value[6], temp.io_value[7], temp.io_value[8], temp.io_value[9], temp.io_value[10], temp.io_value[11], temp.io_value[12],
    temp.io_value[13], temp.io_value[14], temp.io_value[15], temp.io_value[16], temp.io_value[17], temp.io_value[18], temp.io_value[19],
    temp.io_value[20], temp.io_value[21], temp.io_value[22], temp.io_value[23], temp.io_value[24], temp.io_value[25], temp.io_value[26],
    temp.io_value[27], temp.io_value[28], temp.io_value[29], temp.io_value[30], temp.io_value[31], temp.io_coreid);
  }
  {
    SquashArchFpRegState temp;
    memcpy(&temp, packge, sizeof(SquashArchFpRegState));
    packge += sizeof(SquashArchFpRegState);
    v_difftest_ArchFpRegState(temp.io_value[0], temp.io_value[1], temp.io_value[2], temp.io_value[3], temp.io_value[4], temp.io_value[5],
    temp.io_value[6], temp.io_value[7], temp.io_value[8], temp.io_value[9], temp.io_value[10], temp.io_value[11], temp.io_value[12],
    temp.io_value[13], temp.io_value[14], temp.io_value[15], temp.io_value[16], temp.io_value[17], temp.io_value[18], temp.io_value[19],
    temp.io_value[20], temp.io_value[21], temp.io_value[22], temp.io_value[23], temp.io_value[24], temp.io_value[25], temp.io_value[26],
    temp.io_value[27], temp.io_value[28], temp.io_value[29], temp.io_value[30], temp.io_value[31], temp.io_coreid);
  }
  {
    SquashTrapEvent temp;
    memcpy(&temp, packge, sizeof(SquashTrapEvent));
    packge += sizeof(SquashTrapEvent);
    v_difftest_TrapEvent(temp.hasTrap, temp.cycleCnt, temp.instrCnt, temp.hasWFI, temp.code,
    temp.pc, temp.coreid);
  }
  for (size_t i = 0; i < CONFIG_DIFF_COMMIT_WIDTH; i++) {
    SquashInstrCommit temp;
    memcpy(&temp, packge, sizeof(SquashInstrCommit));
    packge += sizeof(SquashInstrCommit);
    v_difftest_InstrCommit(temp.skip, temp.isRVC, temp.rfwen, temp.fpwen, temp.vecwen, temp.wpdest, temp.wdest,
    temp.pc, temp.instr, temp.robIdx, temp.lqIdx, temp.sqIdx, temp.isLoad, temp.isStore, temp.nFused, temp.special,
    temp.coreid, temp.index);
  }
}