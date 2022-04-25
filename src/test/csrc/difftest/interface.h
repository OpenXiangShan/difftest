/***************************************************************************************
* Copyright (c) 2020-2021 Institute of Computing Technology, Chinese Academy of Sciences
* Copyright (c) 2020-2021 Peng Cheng Laboratory
*
* XiangShan is licensed under Mulan PSL v2.
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

/**
 * Headers for Verilog DPI-C difftest interface
 */

#ifndef __DT_INTERFACE_H__
#define __DT_INTERFACE_H__

#include "difftest.h"
#include "runahead.h"

// #ifdef __cplusplus
// extern "C" {
// #endif

#define DIFFTEST_DPIC_FUNC_NAME(name) \
  v_difftest_##name

#define DIFFTEST_DPIC_FUNC_DECL(name) \
  extern "C" void DIFFTEST_DPIC_FUNC_NAME(name)

#define DPIC_ARG_BIT  uint8_t
#define DPIC_ARG_BYTE uint8_t
#define DPIC_ARG_INT  uint32_t
#define DPIC_ARG_LONG uint64_t

// #define DPIC_ARG_BIT  svBit
// #define DPIC_ARG_BYTE char
// #define DPIC_ARG_INT  int32_t
// #define DPIC_ARG_LONG int64_t

// v_difftest_init
extern "C" int v_difftest_init();
extern "C" int v_difftest_step();

// v_difftest_step
// extern "C" int
// #define INTERFACE_STEP
//   DIFFTEST_DPIC_FUNC_DECL(step) (
//   )

// v_difftest_ArchEvent
#define INTERFACE_ARCH_EVENT             \
  DIFFTEST_DPIC_FUNC_DECL(ArchEvent) (   \
    DPIC_ARG_BYTE coreid,                \
    DPIC_ARG_INT  intrNo,                \
    DPIC_ARG_INT  cause,                 \
    DPIC_ARG_LONG exceptionPC,           \
    DPIC_ARG_INT  exceptionInst          \
  )

// v_difftest_BasicInstrCommit
#define INTERFACE_BASIC_INSTR_COMMIT     \
  DIFFTEST_DPIC_FUNC_DECL(BasicInstrCommit) ( \
    DPIC_ARG_BYTE coreid,                \
    DPIC_ARG_BYTE index,                 \
    DPIC_ARG_BIT  valid,                 \
    DPIC_ARG_BYTE special,               \
    DPIC_ARG_BIT  skip,                  \
    DPIC_ARG_BIT  isRVC,                 \
    DPIC_ARG_BIT  rfwen,                 \
    DPIC_ARG_BIT  fpwen,                 \
    DPIC_ARG_BYTE wpdest,                \
    DPIC_ARG_BYTE wdest                  \
  )

// v_difftest_InstrCommit
#define INTERFACE_INSTR_COMMIT           \
  DIFFTEST_DPIC_FUNC_DECL(InstrCommit) ( \
    DPIC_ARG_BYTE coreid,                \
    DPIC_ARG_BYTE index,                 \
    DPIC_ARG_BIT  valid,                 \
    DPIC_ARG_BYTE special,               \
    DPIC_ARG_BIT  skip,                  \
    DPIC_ARG_BIT  isRVC,                 \
    DPIC_ARG_BIT  rfwen,                 \
    DPIC_ARG_BIT  fpwen,                 \
    DPIC_ARG_INT  wpdest,                \
    DPIC_ARG_BYTE wdest,                 \
    DPIC_ARG_LONG pc,                    \
    DPIC_ARG_INT  instr,                 \
    DPIC_ARG_LONG wdata                  \
  )

// v_difftest_BasicTrapEvent
#define INTERFACE_BASIC_TRAP_EVENT       \
  DIFFTEST_DPIC_FUNC_DECL(BasicTrapEvent) (   \
    DPIC_ARG_BYTE coreid,                \
    DPIC_ARG_BIT  valid,                 \
    DPIC_ARG_LONG cycleCnt,              \
    DPIC_ARG_LONG instrCnt,              \
    DPIC_ARG_BIT  hasWFI                 \
  )

// v_difftest_TrapEvent
#define INTERFACE_TRAP_EVENT             \
  DIFFTEST_DPIC_FUNC_DECL(TrapEvent) (   \
    DPIC_ARG_BYTE coreid,                \
    DPIC_ARG_BIT  valid,                 \
    DPIC_ARG_LONG cycleCnt,              \
    DPIC_ARG_LONG instrCnt,              \
    DPIC_ARG_BIT  hasWFI,                \
    DPIC_ARG_BYTE code,                  \
    DPIC_ARG_LONG pc                     \
  )

// v_difftest_CSRState
#define INTERFACE_CSR_STATE              \
  DIFFTEST_DPIC_FUNC_DECL(CSRState) (    \
    DPIC_ARG_BYTE coreid,                \
    DPIC_ARG_BYTE priviledgeMode,        \
    DPIC_ARG_LONG mstatus,               \
    DPIC_ARG_LONG sstatus,               \
    DPIC_ARG_LONG mepc,                  \
    DPIC_ARG_LONG sepc,                  \
    DPIC_ARG_LONG mtval,                 \
    DPIC_ARG_LONG stval,                 \
    DPIC_ARG_LONG mtvec,                 \
    DPIC_ARG_LONG stvec,                 \
    DPIC_ARG_LONG mcause,                \
    DPIC_ARG_LONG scause,                \
    DPIC_ARG_LONG satp,                  \
    DPIC_ARG_LONG mip,                   \
    DPIC_ARG_LONG mie,                   \
    DPIC_ARG_LONG mscratch,              \
    DPIC_ARG_LONG sscratch,              \
    DPIC_ARG_LONG mideleg,               \
    DPIC_ARG_LONG medeleg                \
  )

// v_difftest_DebugMode
#define INTERFACE_DM_STATE               \
  DIFFTEST_DPIC_FUNC_DECL(DebugMode) (   \
    DPIC_ARG_BYTE coreid,                \
    DPIC_ARG_BIT  dMode,                 \
    DPIC_ARG_LONG dcsr,                  \
    DPIC_ARG_LONG dpc,                   \
    DPIC_ARG_LONG dscratch0,             \
    DPIC_ARG_LONG dscratch1              \
  )

// v_difftest_IntWriteback
#define INTERFACE_INT_WRITEBACK          \
  DIFFTEST_DPIC_FUNC_DECL(IntWriteback) (\
    DPIC_ARG_BYTE coreid,                \
    DPIC_ARG_BIT  valid,                 \
    DPIC_ARG_INT  dest,                  \
    DPIC_ARG_LONG data                   \
  )

// v_difftest_ArchIntRegState
#define INTERFACE_INT_REG_STATE          \
  DIFFTEST_DPIC_FUNC_DECL(ArchIntRegState) ( \
    DPIC_ARG_BYTE coreid,                \
    DPIC_ARG_LONG gpr_0,                 \
    DPIC_ARG_LONG gpr_1,                 \
    DPIC_ARG_LONG gpr_2,                 \
    DPIC_ARG_LONG gpr_3,                 \
    DPIC_ARG_LONG gpr_4,                 \
    DPIC_ARG_LONG gpr_5,                 \
    DPIC_ARG_LONG gpr_6,                 \
    DPIC_ARG_LONG gpr_7,                 \
    DPIC_ARG_LONG gpr_8,                 \
    DPIC_ARG_LONG gpr_9,                 \
    DPIC_ARG_LONG gpr_10,                \
    DPIC_ARG_LONG gpr_11,                \
    DPIC_ARG_LONG gpr_12,                \
    DPIC_ARG_LONG gpr_13,                \
    DPIC_ARG_LONG gpr_14,                \
    DPIC_ARG_LONG gpr_15,                \
    DPIC_ARG_LONG gpr_16,                \
    DPIC_ARG_LONG gpr_17,                \
    DPIC_ARG_LONG gpr_18,                \
    DPIC_ARG_LONG gpr_19,                \
    DPIC_ARG_LONG gpr_20,                \
    DPIC_ARG_LONG gpr_21,                \
    DPIC_ARG_LONG gpr_22,                \
    DPIC_ARG_LONG gpr_23,                \
    DPIC_ARG_LONG gpr_24,                \
    DPIC_ARG_LONG gpr_25,                \
    DPIC_ARG_LONG gpr_26,                \
    DPIC_ARG_LONG gpr_27,                \
    DPIC_ARG_LONG gpr_28,                \
    DPIC_ARG_LONG gpr_29,                \
    DPIC_ARG_LONG gpr_30,                \
    DPIC_ARG_LONG gpr_31                 \
  )

// v_difftest_FpWriteback
#define INTERFACE_FP_WRITEBACK           \
  DIFFTEST_DPIC_FUNC_DECL(FpWriteback) ( \
    DPIC_ARG_BYTE coreid,                \
    DPIC_ARG_BIT  valid,                 \
    DPIC_ARG_INT  dest,                  \
    DPIC_ARG_LONG data                   \
  )

// v_difftest_ArchFpRegState
#define INTERFACE_FP_REG_STATE           \
  DIFFTEST_DPIC_FUNC_DECL(ArchFpRegState) ( \
    DPIC_ARG_BYTE coreid,                \
    DPIC_ARG_LONG fpr_0,                 \
    DPIC_ARG_LONG fpr_1,                 \
    DPIC_ARG_LONG fpr_2,                 \
    DPIC_ARG_LONG fpr_3,                 \
    DPIC_ARG_LONG fpr_4,                 \
    DPIC_ARG_LONG fpr_5,                 \
    DPIC_ARG_LONG fpr_6,                 \
    DPIC_ARG_LONG fpr_7,                 \
    DPIC_ARG_LONG fpr_8,                 \
    DPIC_ARG_LONG fpr_9,                 \
    DPIC_ARG_LONG fpr_10,                \
    DPIC_ARG_LONG fpr_11,                \
    DPIC_ARG_LONG fpr_12,                \
    DPIC_ARG_LONG fpr_13,                \
    DPIC_ARG_LONG fpr_14,                \
    DPIC_ARG_LONG fpr_15,                \
    DPIC_ARG_LONG fpr_16,                \
    DPIC_ARG_LONG fpr_17,                \
    DPIC_ARG_LONG fpr_18,                \
    DPIC_ARG_LONG fpr_19,                \
    DPIC_ARG_LONG fpr_20,                \
    DPIC_ARG_LONG fpr_21,                \
    DPIC_ARG_LONG fpr_22,                \
    DPIC_ARG_LONG fpr_23,                \
    DPIC_ARG_LONG fpr_24,                \
    DPIC_ARG_LONG fpr_25,                \
    DPIC_ARG_LONG fpr_26,                \
    DPIC_ARG_LONG fpr_27,                \
    DPIC_ARG_LONG fpr_28,                \
    DPIC_ARG_LONG fpr_29,                \
    DPIC_ARG_LONG fpr_30,                \
    DPIC_ARG_LONG fpr_31                 \
  )

// v_difftest_SbufferEvent
#define INTERFACE_SBUFFER_EVENT          \
  DIFFTEST_DPIC_FUNC_DECL(SbufferEvent) ( \
    DPIC_ARG_BYTE coreid,                \
    DPIC_ARG_BYTE index,                 \
    DPIC_ARG_BIT  sbufferResp,           \
    DPIC_ARG_LONG sbufferAddr,           \
    DPIC_ARG_BYTE sbufferData_0,         \
    DPIC_ARG_BYTE sbufferData_1,         \
    DPIC_ARG_BYTE sbufferData_2,         \
    DPIC_ARG_BYTE sbufferData_3,         \
    DPIC_ARG_BYTE sbufferData_4,         \
    DPIC_ARG_BYTE sbufferData_5,         \
    DPIC_ARG_BYTE sbufferData_6,         \
    DPIC_ARG_BYTE sbufferData_7,         \
    DPIC_ARG_BYTE sbufferData_8,         \
    DPIC_ARG_BYTE sbufferData_9,         \
    DPIC_ARG_BYTE sbufferData_10,        \
    DPIC_ARG_BYTE sbufferData_11,        \
    DPIC_ARG_BYTE sbufferData_12,        \
    DPIC_ARG_BYTE sbufferData_13,        \
    DPIC_ARG_BYTE sbufferData_14,        \
    DPIC_ARG_BYTE sbufferData_15,        \
    DPIC_ARG_BYTE sbufferData_16,        \
    DPIC_ARG_BYTE sbufferData_17,        \
    DPIC_ARG_BYTE sbufferData_18,        \
    DPIC_ARG_BYTE sbufferData_19,        \
    DPIC_ARG_BYTE sbufferData_20,        \
    DPIC_ARG_BYTE sbufferData_21,        \
    DPIC_ARG_BYTE sbufferData_22,        \
    DPIC_ARG_BYTE sbufferData_23,        \
    DPIC_ARG_BYTE sbufferData_24,        \
    DPIC_ARG_BYTE sbufferData_25,        \
    DPIC_ARG_BYTE sbufferData_26,        \
    DPIC_ARG_BYTE sbufferData_27,        \
    DPIC_ARG_BYTE sbufferData_28,        \
    DPIC_ARG_BYTE sbufferData_29,        \
    DPIC_ARG_BYTE sbufferData_30,        \
    DPIC_ARG_BYTE sbufferData_31,        \
    DPIC_ARG_BYTE sbufferData_32,        \
    DPIC_ARG_BYTE sbufferData_33,        \
    DPIC_ARG_BYTE sbufferData_34,        \
    DPIC_ARG_BYTE sbufferData_35,        \
    DPIC_ARG_BYTE sbufferData_36,        \
    DPIC_ARG_BYTE sbufferData_37,        \
    DPIC_ARG_BYTE sbufferData_38,        \
    DPIC_ARG_BYTE sbufferData_39,        \
    DPIC_ARG_BYTE sbufferData_40,        \
    DPIC_ARG_BYTE sbufferData_41,        \
    DPIC_ARG_BYTE sbufferData_42,        \
    DPIC_ARG_BYTE sbufferData_43,        \
    DPIC_ARG_BYTE sbufferData_44,        \
    DPIC_ARG_BYTE sbufferData_45,        \
    DPIC_ARG_BYTE sbufferData_46,        \
    DPIC_ARG_BYTE sbufferData_47,        \
    DPIC_ARG_BYTE sbufferData_48,        \
    DPIC_ARG_BYTE sbufferData_49,        \
    DPIC_ARG_BYTE sbufferData_50,        \
    DPIC_ARG_BYTE sbufferData_51,        \
    DPIC_ARG_BYTE sbufferData_52,        \
    DPIC_ARG_BYTE sbufferData_53,        \
    DPIC_ARG_BYTE sbufferData_54,        \
    DPIC_ARG_BYTE sbufferData_55,        \
    DPIC_ARG_BYTE sbufferData_56,        \
    DPIC_ARG_BYTE sbufferData_57,        \
    DPIC_ARG_BYTE sbufferData_58,        \
    DPIC_ARG_BYTE sbufferData_59,        \
    DPIC_ARG_BYTE sbufferData_60,        \
    DPIC_ARG_BYTE sbufferData_61,        \
    DPIC_ARG_BYTE sbufferData_62,        \
    DPIC_ARG_BYTE sbufferData_63,        \
    DPIC_ARG_LONG sbufferMask            \
  )

// v_difftest_StoreEvent
#define INTERFACE_STORE_EVENT            \
  DIFFTEST_DPIC_FUNC_DECL(StoreEvent) (  \
    DPIC_ARG_BYTE coreid,                \
    DPIC_ARG_BYTE index,                 \
    DPIC_ARG_BIT  valid,                 \
    DPIC_ARG_LONG storeAddr,             \
    DPIC_ARG_LONG storeData,             \
    DPIC_ARG_BYTE storeMask              \
  )

// v_difftest_LoadEvent
#define INTERFACE_LOAD_EVENT             \
  DIFFTEST_DPIC_FUNC_DECL(LoadEvent) (   \
    DPIC_ARG_BYTE coreid,                \
    DPIC_ARG_BYTE index,                 \
    DPIC_ARG_BIT  valid,                 \
    DPIC_ARG_LONG paddr,                 \
    DPIC_ARG_BYTE opType,                \
    DPIC_ARG_BYTE fuType                 \
  )

// v_difftest_AtomicEvent
#define INTERFACE_ATOMIC_EVENT           \
  DIFFTEST_DPIC_FUNC_DECL(AtomicEvent) ( \
    DPIC_ARG_BYTE coreid,                \
    DPIC_ARG_BIT  resp,            \
    DPIC_ARG_LONG addr,            \
    DPIC_ARG_LONG data,            \
    DPIC_ARG_BYTE mask,            \
    DPIC_ARG_BYTE fuop,            \
    DPIC_ARG_LONG out              \
  )

// v_difftest_PtwEvent
#define INTERFACE_PTW_EVENT              \
  DIFFTEST_DPIC_FUNC_DECL(PtwEvent) (    \
    DPIC_ARG_BYTE coreid,                \
    DPIC_ARG_BIT  resp,                  \
    DPIC_ARG_LONG addr,                  \
    DPIC_ARG_LONG data_0,                \
    DPIC_ARG_LONG data_1,                \
    DPIC_ARG_LONG data_2,                \
    DPIC_ARG_LONG data_3                 \
  )

// v_difftest_RefillEvent
#define INTERFACE_REFILL_EVENT           \
  DIFFTEST_DPIC_FUNC_DECL(RefillEvent) ( \
    DPIC_ARG_BYTE coreid,                \
    DPIC_ARG_BIT  valid,                 \
    DPIC_ARG_LONG addr,                  \
    DPIC_ARG_LONG data_0,                \
    DPIC_ARG_LONG data_1,                \
    DPIC_ARG_LONG data_2,                \
    DPIC_ARG_LONG data_3,                \
    DPIC_ARG_LONG data_4,                \
    DPIC_ARG_LONG data_5,                \
    DPIC_ARG_LONG data_6,                \
    DPIC_ARG_LONG data_7,                \
    DPIC_ARG_BIT  cacheid                \
  )

// v_difftest_RefillEvent
#define INTERFACE_LR_SC_EVENT            \
  DIFFTEST_DPIC_FUNC_DECL(LrScEvent) (   \
    DPIC_ARG_BYTE coreid,                \
    DPIC_ARG_BIT  valid,                 \
    DPIC_ARG_BIT  success                \
  )

// v_difftest_RunaheadEvent
#define INTERFACE_RUNAHEAD_EVENT           \
  DIFFTEST_DPIC_FUNC_DECL(RunaheadEvent) ( \
    DPIC_ARG_BYTE coreid,                  \
    DPIC_ARG_BYTE index,                   \
    DPIC_ARG_BIT  valid,                   \
    DPIC_ARG_BIT  branch,                  \
    DPIC_ARG_BIT  may_replay,              \
    DPIC_ARG_LONG pc,                      \
    DPIC_ARG_LONG checkpoint_id            \
  )

// v_difftest_RunaheadCommitEvent
#define INTERFACE_RUNAHEAD_COMMIT_EVENT          \
  DIFFTEST_DPIC_FUNC_DECL(RunaheadCommitEvent) ( \
    DPIC_ARG_BYTE coreid,                        \
    DPIC_ARG_BYTE index,                         \
    DPIC_ARG_BIT  valid,                         \
    DPIC_ARG_LONG pc                             \
  )

// v_difftest_RunaheadRedirectEvent
#define INTERFACE_RUNAHEAD_REDIRECT_EVENT          \
  DIFFTEST_DPIC_FUNC_DECL(RunaheadRedirectEvent) ( \
    DPIC_ARG_BYTE coreid,                          \
    DPIC_ARG_BIT  valid,                           \
    DPIC_ARG_LONG pc,                              \
    DPIC_ARG_LONG target_pc,                       \
    DPIC_ARG_LONG checkpoint_id                    \
  )

// v_difftest_RunaheadMemdepPred
#define INTERFACE_RUNAHEAD_MEMDEP_PRED             \
  DIFFTEST_DPIC_FUNC_DECL(RunaheadMemdepPred) (    \
    DPIC_ARG_BYTE coreid,                          \
    DPIC_ARG_BYTE index,                           \
    DPIC_ARG_BIT  valid,                           \
    DPIC_ARG_BIT  is_load,                         \
    DPIC_ARG_BIT  need_wait,                       \
    DPIC_ARG_LONG pc,                              \
    uint64_t* oracle_vaddr                         \
  )

INTERFACE_BASIC_INSTR_COMMIT;
INTERFACE_ARCH_EVENT;
INTERFACE_INSTR_COMMIT;
INTERFACE_BASIC_TRAP_EVENT;
INTERFACE_TRAP_EVENT;
INTERFACE_CSR_STATE;
INTERFACE_INT_WRITEBACK;
INTERFACE_INT_REG_STATE;
INTERFACE_FP_WRITEBACK;
INTERFACE_FP_REG_STATE;
INTERFACE_SBUFFER_EVENT;
INTERFACE_STORE_EVENT;
INTERFACE_LOAD_EVENT;
INTERFACE_ATOMIC_EVENT;
INTERFACE_PTW_EVENT;
INTERFACE_REFILL_EVENT;
INTERFACE_LR_SC_EVENT;
INTERFACE_RUNAHEAD_EVENT;
INTERFACE_RUNAHEAD_COMMIT_EVENT;
INTERFACE_RUNAHEAD_REDIRECT_EVENT;
INTERFACE_RUNAHEAD_MEMDEP_PRED;

#endif
