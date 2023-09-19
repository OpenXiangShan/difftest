#ifndef __DIFFTEST_DPIC_H__
#define __DIFFTEST_DPIC_H__

#include <cstdint>


extern "C" void v_difftest_RefillEvent (
  uint8_t  io_coreid,
  uint8_t  io_valid,
  uint8_t  io_index,
  uint64_t io_addr,
  uint64_t io_data_0,
  uint64_t io_data_1,
  uint64_t io_data_2,
  uint64_t io_data_3,
  uint64_t io_data_4,
  uint64_t io_data_5,
  uint64_t io_data_6,
  uint64_t io_data_7,
  uint8_t  io_idtfr
);

extern "C" void v_difftest_ArchEvent (
  uint8_t  io_coreid,
  uint8_t  io_valid,
  uint32_t io_interrupt,
  uint32_t io_exception,
  uint64_t io_exceptionPC,
  uint32_t io_exceptionInst
);

extern "C" void v_difftest_CSRState (
  uint8_t  io_coreid,
  uint64_t io_priviledgeMode,
  uint64_t io_mstatus,
  uint64_t io_sstatus,
  uint64_t io_mepc,
  uint64_t io_sepc,
  uint64_t io_mtval,
  uint64_t io_stval,
  uint64_t io_mtvec,
  uint64_t io_stvec,
  uint64_t io_mcause,
  uint64_t io_scause,
  uint64_t io_satp,
  uint64_t io_mip,
  uint64_t io_mie,
  uint64_t io_mscratch,
  uint64_t io_sscratch,
  uint64_t io_mideleg,
  uint64_t io_medeleg
);

extern "C" void v_difftest_DebugMode (
  uint8_t  io_coreid,
  uint8_t  io_debugMode,
  uint64_t io_dcsr,
  uint64_t io_dpc,
  uint64_t io_dscratch0,
  uint64_t io_dscratch1
);

extern "C" void v_difftest_AtomicEvent (
  uint8_t  io_coreid,
  uint8_t  io_valid,
  uint64_t io_addr,
  uint64_t io_data,
  uint8_t  io_mask,
  uint8_t  io_fuop,
  uint64_t io_out
);

extern "C" void v_difftest_LrScEvent (
  uint8_t  io_coreid,
  uint8_t  io_valid,
  uint8_t  io_success
);

extern "C" void v_difftest_StoreEvent (
  uint8_t  io_coreid,
  uint8_t  io_index,
  uint8_t  io_valid,
  uint64_t io_addr,
  uint64_t io_data,
  uint8_t  io_mask
);

extern "C" void v_difftest_SbufferEvent (
  uint8_t  io_coreid,
  uint8_t  io_index,
  uint8_t  io_valid,
  uint64_t io_addr,
  uint8_t  io_data_0,
  uint8_t  io_data_1,
  uint8_t  io_data_2,
  uint8_t  io_data_3,
  uint8_t  io_data_4,
  uint8_t  io_data_5,
  uint8_t  io_data_6,
  uint8_t  io_data_7,
  uint8_t  io_data_8,
  uint8_t  io_data_9,
  uint8_t  io_data_10,
  uint8_t  io_data_11,
  uint8_t  io_data_12,
  uint8_t  io_data_13,
  uint8_t  io_data_14,
  uint8_t  io_data_15,
  uint8_t  io_data_16,
  uint8_t  io_data_17,
  uint8_t  io_data_18,
  uint8_t  io_data_19,
  uint8_t  io_data_20,
  uint8_t  io_data_21,
  uint8_t  io_data_22,
  uint8_t  io_data_23,
  uint8_t  io_data_24,
  uint8_t  io_data_25,
  uint8_t  io_data_26,
  uint8_t  io_data_27,
  uint8_t  io_data_28,
  uint8_t  io_data_29,
  uint8_t  io_data_30,
  uint8_t  io_data_31,
  uint8_t  io_data_32,
  uint8_t  io_data_33,
  uint8_t  io_data_34,
  uint8_t  io_data_35,
  uint8_t  io_data_36,
  uint8_t  io_data_37,
  uint8_t  io_data_38,
  uint8_t  io_data_39,
  uint8_t  io_data_40,
  uint8_t  io_data_41,
  uint8_t  io_data_42,
  uint8_t  io_data_43,
  uint8_t  io_data_44,
  uint8_t  io_data_45,
  uint8_t  io_data_46,
  uint8_t  io_data_47,
  uint8_t  io_data_48,
  uint8_t  io_data_49,
  uint8_t  io_data_50,
  uint8_t  io_data_51,
  uint8_t  io_data_52,
  uint8_t  io_data_53,
  uint8_t  io_data_54,
  uint8_t  io_data_55,
  uint8_t  io_data_56,
  uint8_t  io_data_57,
  uint8_t  io_data_58,
  uint8_t  io_data_59,
  uint8_t  io_data_60,
  uint8_t  io_data_61,
  uint8_t  io_data_62,
  uint8_t  io_data_63,
  uint64_t io_mask
);

extern "C" void v_difftest_ArchIntRegState (
  uint8_t  io_coreid,
  uint64_t io_value_0,
  uint64_t io_value_1,
  uint64_t io_value_2,
  uint64_t io_value_3,
  uint64_t io_value_4,
  uint64_t io_value_5,
  uint64_t io_value_6,
  uint64_t io_value_7,
  uint64_t io_value_8,
  uint64_t io_value_9,
  uint64_t io_value_10,
  uint64_t io_value_11,
  uint64_t io_value_12,
  uint64_t io_value_13,
  uint64_t io_value_14,
  uint64_t io_value_15,
  uint64_t io_value_16,
  uint64_t io_value_17,
  uint64_t io_value_18,
  uint64_t io_value_19,
  uint64_t io_value_20,
  uint64_t io_value_21,
  uint64_t io_value_22,
  uint64_t io_value_23,
  uint64_t io_value_24,
  uint64_t io_value_25,
  uint64_t io_value_26,
  uint64_t io_value_27,
  uint64_t io_value_28,
  uint64_t io_value_29,
  uint64_t io_value_30,
  uint64_t io_value_31
);

extern "C" void v_difftest_ArchFpRegState (
  uint8_t  io_coreid,
  uint64_t io_value_0,
  uint64_t io_value_1,
  uint64_t io_value_2,
  uint64_t io_value_3,
  uint64_t io_value_4,
  uint64_t io_value_5,
  uint64_t io_value_6,
  uint64_t io_value_7,
  uint64_t io_value_8,
  uint64_t io_value_9,
  uint64_t io_value_10,
  uint64_t io_value_11,
  uint64_t io_value_12,
  uint64_t io_value_13,
  uint64_t io_value_14,
  uint64_t io_value_15,
  uint64_t io_value_16,
  uint64_t io_value_17,
  uint64_t io_value_18,
  uint64_t io_value_19,
  uint64_t io_value_20,
  uint64_t io_value_21,
  uint64_t io_value_22,
  uint64_t io_value_23,
  uint64_t io_value_24,
  uint64_t io_value_25,
  uint64_t io_value_26,
  uint64_t io_value_27,
  uint64_t io_value_28,
  uint64_t io_value_29,
  uint64_t io_value_30,
  uint64_t io_value_31
);

extern "C" void v_difftest_FpWriteback (
  uint8_t  io_coreid,
  uint8_t  io_valid,
  uint8_t  io_address,
  uint64_t io_data
);

extern "C" void v_difftest_IntWriteback (
  uint8_t  io_coreid,
  uint8_t  io_valid,
  uint8_t  io_address,
  uint64_t io_data
);

extern "C" void v_difftest_InstrCommit (
  uint8_t  io_coreid,
  uint8_t  io_index,
  uint8_t  io_valid,
  uint8_t  io_skip,
  uint8_t  io_isRVC,
  uint8_t  io_rfwen,
  uint8_t  io_fpwen,
  uint8_t  io_vecwen,
  uint8_t  io_wpdest,
  uint8_t  io_wdest,
  uint64_t io_pc,
  uint32_t io_instr,
  uint32_t io_robIdx,
  uint8_t  io_lqIdx,
  uint8_t  io_sqIdx,
  uint8_t  io_isLoad,
  uint8_t  io_isStore,
  uint8_t  io_nFused,
  uint8_t  io_special
);

extern "C" void v_difftest_RunaheadCommitEvent (
  uint8_t  io_coreid,
  uint8_t  io_index,
  uint8_t  io_valid,
  uint64_t io_pc
);

extern "C" void v_difftest_LoadEvent (
  uint8_t  io_coreid,
  uint8_t  io_index,
  uint8_t  io_valid,
  uint64_t io_paddr,
  uint8_t  io_opType,
  uint8_t  io_fuType
);

extern "C" void v_difftest_TrapEvent (
  uint8_t  io_coreid,
  uint8_t  io_hasTrap,
  uint64_t io_cycleCnt,
  uint64_t io_instrCnt,
  uint8_t  io_hasWFI,
  uint8_t  io_code,
  uint64_t io_pc
);

#endif // __DIFFTEST_DPIC_H__
