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

#ifndef __NEMU_PROXY_H
#define __NEMU_PROXY_H

#include "common.h"
#include <dlfcn.h>
#include <unistd.h>

/* clang-format off */
static const char *regs_name_int[] = {
  "$0",  "ra",  "sp",   "gp",   "tp",  "t0",  "t1",   "t2",
  "s0",  "s1",  "a0",   "a1",   "a2",  "a3",  "a4",   "a5",
  "a6",  "a7",  "s2",   "s3",   "s4",  "s5",  "s6",   "s7",
  "s8",  "s9",  "s10",  "s11",  "t3",  "t4",  "t5",   "t6"
};

static const char *regs_name_csr[] = {
  "mode",
  "mstatus", "sstatus", "mepc",
  "sepc", "mtval", "stval",
  "mtvec",
  "stvec", "mcause", "scause", "satp", "mip", "mie",
  "mscratch", "sscratch", "mideleg", "medeleg"
};

static const char *regs_name_hcsr[] = {
  "v",
  "mtval2", "mtinst", "hstatus", "hideleg", "hedeleg",
  "hcounteren", "htval", "htinst", "hgatp", "vsstatus",
  "vstvec","vsepc", "vscause", "vstval", "vsatp", "vsscratch"
};

static const char *regs_name_fp[] = {
  "ft0", "ft1", "ft2",  "ft3",  "ft4", "ft5", "ft6",  "ft7",
  "fs0", "fs1", "fa0",  "fa1",  "fa2", "fa3", "fa4",  "fa5",
  "fa6", "fa7", "fs2",  "fs3",  "fs4", "fs5", "fs6",  "fs7",
  "fs8", "fs9", "fs10", "fs11", "ft8", "ft9", "ft10", "ft11"
};

static const char *debug_regs_name[] = {
  "debug mode", "dcsr", "dpc", "dscratch0", "dscratch1",
};

static const char *regs_name_vec[] = {
  "v0_low",  "v0_high",  "v1_low",  "v1_high",  "v2_low",  "v2_high",  "v3_low",  "v3_high",
  "v4_low",  "v4_high",  "v5_low",  "v5_high",  "v6_low",  "v6_high",  "v7_low",  "v7_high",
  "v8_low",  "v8_high",  "v9_low",  "v9_high",  "v10_low", "v10_high", "v11_low", "v11_high",
  "v12_low", "v12_high", "v13_low", "v13_high", "v14_low", "v14_high", "v15_low", "v15_high",
  "v16_low", "v16_high", "v17_low", "v17_high", "v18_low", "v18_high", "v19_low", "v19_high",
  "v20_low", "v20_high", "v21_low", "v21_high", "v22_low", "v22_high", "v23_low", "v23_high",
  "v24_low", "v24_high", "v25_low", "v25_high", "v26_low", "v26_high", "v27_low", "v27_high",
  "v28_low", "v28_high", "v29_low", "v29_high", "v30_low", "v30_high", "v31_low", "v31_high"
};

static const char *regs_name_vec_csr[] = {
  "vstart", "vxsat", "vxrm", "vcsr", "vl", "vtype", "vlenb"
};

static const char *regs_name_fp_csr[] = {
  "fcsr"
};

static const char *regs_name_triggercsr[] = {
  "tselect", "tdata1", "tinfo"
};

/* clang-format on */

enum {
  REF_TO_DUT,
  DUT_TO_REF
};

class RefProxyConfig {
public:
  bool ignore_illegal_mem_access = false;
  bool debug_difftest = false;
  bool enable_store_log = false;
};

/* clang-format off */
#define REF_BASE(f)                                                           \
  f(ref_init, difftest_init, void, )                                          \
  f(ref_regcpy, difftest_regcpy, void, void*, bool, bool)                     \
  f(ref_csrcpy, difftest_csrcpy, void, void*, bool)                           \
  f(ref_memcpy, difftest_memcpy, void, uint64_t, void*, size_t, bool)         \
  f(ref_exec, difftest_exec, void, uint64_t)                                  \
  f(ref_reg_display, difftest_display, void, )                                \
  f(update_config, update_dynamic_config, void, void*)                        \
  f(uarchstatus_sync, difftest_uarchstatus_sync, void, void*)                 \
  f(store_commit, difftest_store_commit, int, uint64_t*, uint64_t*, uint8_t*) \
  f(raise_intr, difftest_raise_intr, void, uint64_t)

#ifdef ENABLE_RUNHEAD
#define REF_RUN_AHEAD(f)                                                      \
  f(query, difftest_query_ref, void, void *, uint64_t)
#else
#define REF_RUN_AHEAD(f)
#endif

#ifdef ENABLE_STORE_LOG
#define REF_STORE_LOG(f)                                                      \
  f(ref_store_log_reset, difftest_store_log_reset, void, )                    \
  f(ref_store_log_restore, difftest_store_log_restore, void, )
#else
#define REF_STORE_LOG(f)
#endif

#ifdef DEBUG_MODE_DIFF
#define REF_DEBUG_MODE(f)                                                     \
  f(debug_mem_sync, debug_mem_sync, void, uint64_t, void *, size_t)
#else
#define REF_DEBUG_MODE(f)
#endif

#define REF_ALL(f)  \
  REF_BASE(f)       \
  REF_RUN_AHEAD(f)  \
  REF_STORE_LOG(f)  \
  REF_DEBUG_MODE(f)

#define REF_OPTIONAL(f)                                                                                     \
  f(ref_init_v2, difftest_init_v2, void, unsigned)                                                          \
  f(load_flash_bin, difftest_load_flash, void, const char*, size_t)                                         \
  f(load_flash_bin_v2, difftest_load_flash_v2, void, const uint8_t*, size_t)                                \
  f(ref_status, difftest_status, int, )                                                                     \
  f(ref_close, difftest_close, void, )                                                                      \
  f(ref_set_ramsize, difftest_set_ramsize, void, size_t)                                                    \
  f(ref_set_mhartid, difftest_set_mhartid, void, int)                                                       \
  f(ref_put_gmaddr, difftest_put_gmaddr, void, void *)                                                      \
  f(ref_skip_one, difftest_skip_one, void, bool, bool, uint32_t, uint64_t)                                  \
  f(ref_guided_exec, difftest_guided_exec, void, void*)                                                     \
  f(ref_memcpy_init, difftest_memcpy_init, void, uint64_t, void*, size_t, bool)                             \
  f(raise_nmi_intr, difftest_raise_nmi_intr, void, bool)                                                    \
  f(ref_virtual_interrupt_is_hvictl_inject, difftest_virtual_interrupt_is_hvictl_inject, void, bool)        \
  f(ref_interrupt_delegate, difftest_interrupt_delegate, void, void*)                                       \
  f(disambiguation_state, difftest_disambiguation_state, int, )                                             \
  f(ref_non_reg_interrupt_pending, difftest_non_reg_interrupt_pending, void, void*)                         \
  f(raise_mhpmevent_overflow, difftest_raise_mhpmevent_overflow, void, uint64_t)                            \
  f(ref_raise_critical_error, difftest_raise_critical_error, bool)                                          \
  f(ref_get_store_event_other_info, difftest_get_store_event_other_info, void, void*)                       \
  f(ref_sync_aia, difftest_sync_aia, void, void*)                                                           \
  f(ref_sync_custom_mflushpwr, difftest_sync_custom_mflushpwr, void, bool)                                  \
  f(ref_get_vec_load_vdNum, difftest_get_vec_load_vdNum, int, )                                             \
  f(ref_get_vec_load_dual_goldenmem_reg, difftest_get_vec_load_dual_goldenmem_reg, void*, )                 \
  f(ref_update_vec_load_goldenmen, difftest_update_vec_load_pmem, void, )
#define RefFunc(func, ret, ...) ret func(__VA_ARGS__)
#define DeclRefFunc(this_func, dummy, ret, ...) RefFunc((*this_func), ret, __VA_ARGS__);
/* clang-format on */

// This class only loads the functions. It should never call anything.
class AbstractRefProxy {
public:
  REF_ALL(DeclRefFunc)

  AbstractRefProxy(int coreid, size_t ram_size, const char *env, const char *file_path);
  ~AbstractRefProxy();

protected:
  REF_OPTIONAL(DeclRefFunc)

private:
  void *const handler;
  void *load_handler(const char *env, const char *file_path);
  template <typename T> T load_function(const char *func_name);
};

typedef struct __attribute__((packed)) {
  DifftestArchIntRegState xrf;
#ifdef CONFIG_DIFFTEST_ARCHFPREGSTATE
  DifftestArchFpRegState frf;
#endif // CONFIG_DIFFTEST_ARCHFPREGSTATE
  DifftestCSRState csr;
  uint64_t pc;
#ifdef CONFIG_DIFFTEST_HCSRSTATE
  DifftestHCSRState hcsr;
#endif // CONFIG_DIFFTEST_HCSRSTATE
#ifdef CONFIG_DIFFTEST_ARCHVECREGSTATE
  DifftestArchVecRegState vrf;
#endif // CONFIG_DIFFTEST_ARCHVECREGSTATE
#ifdef CONFIG_DIFFTEST_VECCSRSTATE
  DifftestVecCSRState vcsr;
#endif // CONFIG_DIFFTEST_VECCSRSTATE
#ifdef CONFIG_DIFFTEST_FPCSRSTATE
  DifftestFpCSRState fcsr;
#endif // CONFIG_DIFFTEST_FPCSRSTATE
#ifdef CONFIG_DIFFTEST_TRIGGERCSRSTATE
  DifftestTriggerCSRState triggercsr;
#endif // CONFIG_DIFFTEST_TRIGGERCSRSTATE
} ref_state_t;

class RefProxy : public AbstractRefProxy {
public:
  RefProxy(int coreid, size_t ram_size) : AbstractRefProxy(coreid, ram_size, nullptr, nullptr) {}
  RefProxy(int coreid, size_t ram_size, const char *env, const char *file_path)
      : AbstractRefProxy(coreid, ram_size, env, file_path) {}
  ~RefProxy();

  ref_state_t state;

  inline uint64_t *arch_reg(uint8_t src, bool is_fp = false) {
    return
#ifdef CONFIG_DIFFTEST_ARCHFPREGSTATE
        is_fp ? state.frf.value + src :
#endif
              state.xrf.value + src;
  }

#ifdef CONFIG_DIFFTEST_ARCHVECREGSTATE
  inline uint64_t *arch_vecreg(uint8_t src) {
    return state.vrf.value + src;
  }
#endif // CONFIG_DIFFTEST_ARCHVECREGSTATE
  inline void sync(bool is_from_dut = false) {
    ref_regcpy(&state.xrf, is_from_dut, is_from_dut);
  }

  void regcpy(const DiffTestRegState *regs, uint64_t pc);
  int compare(DiffTestState *dut);
  void display(DiffTestState *dut = nullptr);

  inline void skip_one(bool isRVC, bool rfwen, bool fpwen, bool vecwen, uint32_t wdest, uint64_t wdata) {
    bool wen = rfwen | fpwen;
    if (ref_skip_one) {
      ref_skip_one(isRVC, wen, wdest, wdata);
    } else {
      sync();
      state.pc += isRVC ? 2 : 4;

      if (rfwen)
        state.xrf.value[wdest] = wdata;
#ifdef CONFIG_DIFFTEST_ARCHFPREGSTATE
      if (fpwen)
        state.frf.value[wdest] = wdata;
#endif // CONFIG_DIFFTEST_ARCHFPREGSTATE
#ifdef CONFIG_DIFFTEST_ARCHVECREGSTATE
      // TODO: vec skip is not supported at this time.
      if (vecwen)
        assert(0);
#endif // CONFIG_DIFFTEST_ARCHVECREGSTATE

      sync(true);
    }
  }

  inline void trigger_nmi(bool hasNMI) {
    if (raise_nmi_intr) {
      raise_nmi_intr(hasNMI);
    } else {
      Info("No NMI interrupt is triggered.\n");
    }
  }

  inline void virtual_interrupt_is_hvictl_inject(bool virtualInterruptIsHvictlInject) {
    if (ref_virtual_interrupt_is_hvictl_inject) {
      ref_virtual_interrupt_is_hvictl_inject(virtualInterruptIsHvictlInject);
    } else {
      Info("Virtual interrupt without hvictl register injection.\n");
    }
  }

  inline void intr_delegate(struct InterruptDelegate &intrDeleg) {
    if (ref_interrupt_delegate) {
      ref_interrupt_delegate(&intrDeleg);
    }
  }

  inline void non_reg_interrupt_pending(struct NonRegInterruptPending &ip) {
    if (ref_non_reg_interrupt_pending) {
      ref_non_reg_interrupt_pending(&ip);
    }
  }

  inline void mhpmevent_overflow(uint64_t mhpmeventOverflow) {
    if (raise_mhpmevent_overflow) {
      raise_mhpmevent_overflow(mhpmeventOverflow);
    }
  }

  inline bool raise_critical_error() {
    return ref_raise_critical_error ? ref_raise_critical_error() : false;
  }

  inline void sync_aia(struct FromAIA &src) {
    if (ref_sync_aia) {
      ref_sync_aia(&src);
    } else {
      Info("Does not support the out-of-core part of AIA.\n");
    }
  }

  inline void sync_custom_mflushpwr(bool l2FlushDone) {
    if (ref_sync_custom_mflushpwr) {
      ref_sync_custom_mflushpwr(l2FlushDone);
    } else {
      printf("Does not support sync custom CSR mflushpwr.\n");
    }
  }

  inline bool check_ref_vec_load_goldenmem() {
    return ref_get_vec_load_vdNum && ref_get_vec_load_dual_goldenmem_reg && ref_update_vec_load_goldenmen;
  }

  inline int get_ref_vdNum() {
    if (ref_get_vec_load_vdNum) {
      return ref_get_vec_load_vdNum();
    } else {
      Info("Does not support the get vec load vd num.\n");
      return 0;
    }
  }

  inline void *get_vec_goldenmem_reg() {
    if (ref_get_vec_load_dual_goldenmem_reg) {
      return ref_get_vec_load_dual_goldenmem_reg();
    } else {
      Info("Does not support the get vec load goldenmem reg.\n");
      return nullptr;
    }
  }

  inline void vec_update_goldenmem() {
    if (ref_update_vec_load_goldenmen) {
      ref_update_vec_load_goldenmen();
    } else {
      Info("Does not support the get vec update goldenmem.\n");
    }
  }

  inline void guided_exec(struct ExecutionGuide &guide) {
    return ref_guided_exec ? ref_guided_exec(&guide) : ref_exec(1);
  }

  virtual inline bool in_disambiguation_state() {
    return disambiguation_state ? disambiguation_state() : false;
  }

  inline void set_debug(bool enabled = false) {
    config.debug_difftest = enabled;
    sync_config();
  }

  inline bool get_debug() {
    return config.debug_difftest;
  }

  inline void set_illegal_mem_access(bool ignored = false) {
    config.ignore_illegal_mem_access = ignored;
    sync_config();
  }

  inline void mem_init(uint64_t dest, void *src, size_t n, bool direction) {
    if (ref_memcpy_init) {
      ref_memcpy_init(dest, src, n, direction);
    } else {
      ref_memcpy(dest, src, n, direction);
    }
  }

  void flash_init(const uint8_t *flash_base, size_t size, const char *flash_bin);

  inline void get_store_event_other_info(void *info) {
    if (ref_get_store_event_other_info) {
      ref_get_store_event_other_info(info);
    } else {
      Info(
          "This version of 'REF' does not support the 'PC' value of store commit event. Please use a newer version of "
          "'REF'.\n");
    }
  }

#ifdef ENABLE_STORE_LOG
  inline void set_store_log(bool enable = false) {
    config.enable_store_log = enable;
    sync_config();
  }
#endif // ENABLE_STORE_LOG

  inline int get_status() {
    return ref_status ? ref_status() : 0;
  }

private:
  RefProxyConfig config;

  inline void sync_config() {
    update_config(&config);
  }
  bool do_csr_waive(DiffTestState *dut);
};

class NemuProxy : public RefProxy {
public:
  NemuProxy(int coreid, size_t ram_size = 0);
  ~NemuProxy() {}
};

class SpikeProxy : public RefProxy {
public:
  SpikeProxy(int coreid, size_t ram_size = 0);
  ~SpikeProxy() {}
};

class LinkedProxy : public RefProxy {
public:
  LinkedProxy(int coreid, size_t ram_size = 0);
  ~LinkedProxy() {}
};

struct SyncState {
  uint64_t sc_fail;
};

struct ExecutionGuide {
  // force raise exception
  bool force_raise_exception;
  uint64_t exception_num;
  uint64_t mtval;
  uint64_t stval;
#ifdef CONFIG_DIFFTEST_HCSRSTATE
  uint64_t mtval2;
  uint64_t htval;
  uint64_t vstval;
#endif // CONFIG_DIFFTEST_HCSRSTATE
  // force set jump target
  bool force_set_jump_target;
  uint64_t jump_target;
};

struct NonRegInterruptPending {
  bool platformIRPMeip;
  bool platformIRPMtip;
  bool platformIRPMsip;
  bool platformIRPSeip;
  bool platformIRPStip;
  bool platformIRPVseip;
  bool platformIRPVstip;
  bool fromAIAMeip;
  bool fromAIASeip;
  bool localCounterOverflowInterruptReq;
};

struct FromAIA {
  uint64_t mtopei;
  uint64_t stopei;
  uint64_t vstopei;
  uint64_t hgeip;
};

struct InterruptDelegate {
  bool irToHS;
  bool irToVS;
};

extern const char *difftest_ref_so;
extern uint8_t *ref_golden_mem;

#define REPORT_DIFFERENCE(name, pc_val, right_val, wrong_val) \
  Info("%7s different at pc = 0x%010lx, right = 0x%016lx, wrong = 0x%016lx\n", name, pc_val, right_val, wrong_val);

#endif
