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
  "tselect", "tdata1", "tinfo", "tcontol"
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
  f(raise_intr, difftest_raise_intr, void, uint64_t)                          \
  f(load_flash_bin, difftest_load_flash, void, void*, size_t)

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

#define REF_OPTIONAL(f)                                                       \
  f(ref_status, difftest_status, int, )                                       \
  f(ref_close, difftest_close, void, )                                        \
  f(ref_set_ramsize, difftest_set_ramsize, void, size_t)                      \
  f(ref_set_mhartid, difftest_set_mhartid, void, int)                         \
  f(ref_put_gmaddr, difftest_put_gmaddr, void, void *)                        \
  f(ref_skip_one, difftest_skip_one, void, bool, bool, uint32_t, uint64_t)    \
  f(ref_guided_exec, difftest_guided_exec, void, void*)                       \
  f(raise_nmi_intr, difftest_raise_nmi_intr, void, bool)                      \
  f(ref_virtual_interrupt_is_hvictl_inject, difftest_virtual_interrupt_is_hvictl_inject, void, bool)        \
  f(disambiguation_state, difftest_disambiguation_state, int, )

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

class RefProxy : public AbstractRefProxy {
public:
  RefProxy(int coreid, size_t ram_size) : AbstractRefProxy(coreid, ram_size, nullptr, nullptr) {}
  RefProxy(int coreid, size_t ram_size, const char *env, const char *file_path)
      : AbstractRefProxy(coreid, ram_size, env, file_path) {}
  ~RefProxy();

  DifftestArchIntRegState regs_int;
#ifdef CONFIG_DIFFTEST_ARCHFPREGSTATE
  DifftestArchFpRegState regs_fp;
#endif // CONFIG_DIFFTEST_ARCHFPREGSTATE
  DifftestCSRState csr;
  uint64_t pc;
#ifdef CONFIG_DIFFTEST_HCSRSTATE
  DifftestHCSRState hcsr;
#endif // CONFIG_DIFFTEST_HCSRSTATE
#ifdef CONFIG_DIFFTEST_ARCHVECREGSTATE
  DifftestArchVecRegState regs_vec;
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

  inline uint64_t *arch_reg(uint8_t src, bool is_fp = false) {
    return
#ifdef CONFIG_DIFFTEST_ARCHFPREGSTATE
        is_fp ? regs_fp.value + src :
#endif
              regs_int.value + src;
  }

  inline void sync(bool is_from_dut = false) {
    ref_regcpy(&regs_int, is_from_dut, is_from_dut);
  }

  void regcpy(DiffTestState *dut);
  int compare(DiffTestState *dut);
  void display(DiffTestState *dut = nullptr);

  inline void skip_one(bool isRVC, bool wen, uint32_t wdest, uint64_t wdata) {
    if (ref_skip_one) {
      ref_skip_one(isRVC, wen, wdest, wdata);
    } else {
      sync();
      pc += isRVC ? 2 : 4;
      // TODO: what if skip with fpwen?
      if (wen) {
        regs_int.value[wdest] = wdata;
      }
      sync(true);
    }
  }

  inline void trigger_nmi(bool hasNMI) {
    if (raise_nmi_intr) {
      raise_nmi_intr(hasNMI);
    } else {
      printf("No NMI interrupt is triggered.\n");
    }
  }

  inline void virtual_interrupt_is_hvictl_inject(bool virtualInterruptIsHvictlInject) {
    if (ref_virtual_interrupt_is_hvictl_inject) {
      ref_virtual_interrupt_is_hvictl_inject(virtualInterruptIsHvictlInject);
    } else {
      printf("Virtual interrupt without hvictl register injection.\n");
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

  inline void set_illegal_mem_access(bool ignored = false) {
    config.ignore_illegal_mem_access = ignored;
    sync_config();
  }

#ifdef ENABLE_STORE_LOG
  inline void set_store_log(bool enable = false) {
    config.enable_store_log = enable;
    sync_config();
  }
#endif // ENABLE_STORE_LOG

  inline int get_reg_size() {
    return sizeof(DifftestArchIntRegState) + sizeof(DifftestCSRState) + sizeof(uint64_t)
#ifdef CONFIG_DIFFTEST_ARCHFPREGSTATE
           + sizeof(DifftestArchFpRegState)
#endif // CONFIG_DIFFTEST_ARCHFPREGSTATE
#ifdef CONFIG_DIFFTEST_ARCHVECREGSTATE
           + sizeof(DifftestArchVecRegState)
#endif // CONFIG_DIFFTEST_ARCHVECREGSTATE
#ifdef CONFIG_DIFFTEST_VECCSRSTATE
           + sizeof(DifftestVecCSRState)
#endif // CONFIG_DIFFTEST_VECCSRSTATE
#ifdef CONFIG_DIFFTEST_FPCSRSTATE
           + sizeof(DifftestFpCSRState)
#endif // CONFIG_DIFFTEST_FPCSRSTATE
#ifdef CONFIG_DIFFTEST_HCSRSTATE
           + sizeof(DifftestHCSRState)
#endif // CONFIG_DIFFTEST_HCSRSTATE
#ifdef CONFIG_DIFFTEST_TRIGGERCSRSTATE
           + sizeof(DifftestTriggerCSRState)
#endif //CONFIG_DIFFTEST_TRIGGERCSRSTATE
        ;
  }

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

extern const char *difftest_ref_so;
extern uint8_t *ref_golden_mem;

#endif
