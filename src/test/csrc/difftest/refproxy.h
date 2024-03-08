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

#include <cstddef>
#include <unistd.h>
#include <dlfcn.h>
#include <type_traits>

#include "common.h"

enum { DIFFTEST_TO_DUT, DIFFTEST_TO_REF };
enum { REF_TO_DUT, DUT_TO_REF };
// DIFFTEST_TO_DUT ~ REF_TO_DUT ~ REF_TO_DIFFTEST
// DIFFTEST_TO_REF ~ DUT_TO_REF ~ DUT_TO_DIFFTEST

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
  "mscratch", "sscratch", "mideleg", "medeleg",
#ifdef FDI_DIFF
  "fdiMainCfg", "fdiUMBoundLo", "fdiUMBoundHi",
  "fdiLibCfg",
  "fdiLibBound0", "fdiLibBound1", "fdiLibBound2", "fdiLibBound3",
  "fdiLibBound4", "fdiLibBound5", "fdiLibBound6", "fdiLibBound7",
  "fdiMainCall", "fdiReturnPC",
  "fdiJumpCfg", "fdiJumpBound0", "fdiJumpBound1",
#endif  // FDI_DIFF
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


class RefProxyConfig {
public:
  bool ignore_illegal_mem_access = false;
  bool debug_difftest = false;
};

template<typename DerivedT>
class RefProxy{
public:
  RefProxy() {
    static_assert(std::is_base_of<RefProxy, DerivedT>::value,
                  "Must pass the derived type as the template argument!");
  }
  ~RefProxy() {
    if (derived.ref_close) {
      derived.ref_close();
    }
  }

  DifftestArchIntRegState regs_int;
#ifdef CONFIG_DIFFTEST_ARCHFPREGSTATE
  DifftestArchFpRegState regs_fp;
#endif // CONFIG_DIFFTEST_ARCHFPREGSTATE
  DifftestCSRState csr;
  uint64_t pc;
#ifdef CONFIG_DIFFTEST_ARCHVECREGSTATE
  DifftestArchVecRegState regs_vec;
#endif // CONFIG_DIFFTEST_ARCHVECREGSTATE
#ifdef CONFIG_DIFFTEST_VECCSRSTATE
  DifftestVecCSRState vcsr;
#endif // CONFIG_DIFFTEST_VECCSRSTATE

  inline uint64_t *arch_reg(uint8_t src, bool is_fp = false) {
    return
#ifdef CONFIG_DIFFTEST_ARCHFPREGSTATE
      is_fp ? regs_fp.value + src :
#endif
      regs_int.value + src;
  }

  inline void sync(bool is_from_dut = false) {
    derived.ref_regcpy(&regs_int, is_from_dut, is_from_dut);
  }

  void regcpy(DiffTestState *dut) {
    memcpy(&regs_int, dut->regs_int.value, 32 * sizeof(uint64_t));
#ifdef CONFIG_DIFFTEST_ARCHFPREGSTATE
    memcpy(&regs_fp, dut->regs_fp.value, 32 * sizeof(uint64_t));
#endif // CONFIG_DIFFTEST_ARCHFPREGSTATE
    memcpy(&csr, &dut->csr, sizeof(csr));
    pc = dut->commit[0].pc;
#ifdef CONFIG_DIFFTEST_ARCHVECREGSTATE
    memcpy(&regs_vec, &dut->regs_vec.value, sizeof(regs_vec));
#endif // CONFIG_DIFFTEST_ARCHVECREGSTATE
#ifdef CONFIG_DIFFTEST_VECCSRSTATE
    memcpy(&vcsr, &dut->vcsr, sizeof(vcsr));
#endif // CONFIG_DIFFTEST_VECCSRSTATE
    derived.ref_regcpy(&regs_int, DUT_TO_REF, false);
  };

  int compare(DiffTestState *dut) {
#define PROXY_COMPARE(field) memcmp(&(dut->field), &(field), sizeof(field))

    const int results[] = {PROXY_COMPARE(regs_int),
#ifdef CONFIG_DIFFTEST_ARCHFPREGSTATE
                           PROXY_COMPARE(regs_fp),
#endif // CONFIG_DIFFTEST_ARCHFPREGSTATE
#ifdef CONFIG_DIFFTEST_ARCHVECREGSTATE
                           PROXY_COMPARE(regs_vec),
#endif // CONFIG_DIFFTEST_ARCHVECREGSTATE
#ifdef CONFIG_DIFFTEST_VECCSRSTATE
                           PROXY_COMPARE(vcsr),
#endif // CONFIG_DIFFTEST_VECCSRSTATE
                           PROXY_COMPARE(csr)

    };
    for (int i = 0; i < sizeof(results) / sizeof(int); i++) {
      if (results[i]) {
        // There may be some waive rules for CSRs
        if (i == sizeof(results) / sizeof(int) - 1) {
          // If mismatches are cleared, we sync the states back to REF.
          if (do_csr_waive(dut) && !PROXY_COMPARE(csr)) {
            sync(true);
            return 0;
          }
        }
        return 1;
      }
    }
    return 0;
  };

  void display(DiffTestState *dut = nullptr) {
    if (dut) {
#define PROXY_COMPARE_AND_DISPLAY(field, field_names)                          \
  do {                                                                         \
    uint64_t *_ptr_dut = (uint64_t *)(&((dut)->field));                        \
    uint64_t *_ptr_ref = (uint64_t *)(&(field));                               \
    for (int i = 0; i < sizeof(field) / sizeof(uint64_t); i++) {               \
      if (_ptr_dut[i] != _ptr_ref[i]) {                                        \
        printf("%7s different at pc = 0x%010lx, right= 0x%016lx, "             \
               "wrong = 0x%016lx\n",                                           \
               field_names[i], dut->commit[0].pc, _ptr_ref[i], _ptr_dut[i]);   \
      }                                                                        \
    }                                                                          \
  } while (0);

      PROXY_COMPARE_AND_DISPLAY(regs_int, regs_name_int)
#ifdef CONFIG_DIFFTEST_ARCHFPREGSTATE
      PROXY_COMPARE_AND_DISPLAY(regs_fp, regs_name_fp)
#endif // CONFIG_DIFFTEST_ARCHFPREGSTATE
      PROXY_COMPARE_AND_DISPLAY(csr, regs_name_csr)
#ifdef CONFIG_DIFFTEST_ARCHVECREGSTATE
      PROXY_COMPARE_AND_DISPLAY(regs_vec, regs_name_vec)
#endif // CONFIG_DIFFTEST_ARCHVECREGSTATE
#ifdef CONFIG_DIFFTEST_VECCSRSTATE
      PROXY_COMPARE_AND_DISPLAY(vcsr, regs_name_vec_csr)
#endif // CONFIG_DIFFTEST_VECCSRSTATE
    } else {
      derived.ref_reg_display();
    }
  };

  inline void skip_one(bool isRVC, bool wen, uint32_t wdest, uint64_t wdata) {
    if (derived.ref_skip_one) {
      derived.ref_skip_one(isRVC, wen, wdest, wdata);
    }
    else {
      sync();
      pc += isRVC ? 2 : 4;
      // TODO: what if skip with fpwen?
      if (wen) {
        regs_int.value[wdest] = wdata;
      }
      sync(true);
    }
  }

  inline void guided_exec(struct ExecutionGuide &guide) {
  #if defined (SELECTEDSpike)
    derived.ref_guided_exec(&guide);
  #else
    derived.ref_guided_exec ? derived.ref_guided_exec(&guide) : derived.ref_exec(1);
  #endif
  }

  virtual inline bool in_disambiguation_state() {
    return derived.disambiguation_state ? derived.disambiguation_state() : false;
  }

  inline void set_debug(bool enabled = false) {
    config.debug_difftest = enabled;
    sync_config();
  }

  inline void set_illegal_mem_access(bool ignored = false) {
    config.ignore_illegal_mem_access = ignored;
    sync_config();
  }

private:
  DerivedT& derived = static_cast<DerivedT&>(*this);

  RefProxyConfig config;

  inline void sync_config() {
    derived.update_config(&config);
  }

  bool do_csr_waive(DiffTestState *dut) {
#define CSR_WAIVE(field, mapping)                                              \
  do {                                                                         \
    uint64_t v = mapping(csr.field);                                           \
    if (csr.field != dut->csr.field && dut->csr.field == v) {                  \
      csr.field = v;                                                           \
      has_waive = true;                                                        \
    }                                                                          \
  } while (0);

    bool has_waive = false;
#ifdef CPU_ROCKET_CHIP
    CSR_WAIVE(mtval, encode_vaddr);
    CSR_WAIVE(mtval, sext_vaddr_40bit);
    CSR_WAIVE(stval, encode_vaddr);
    CSR_WAIVE(stval, sext_vaddr_40bit);
    CSR_WAIVE(mtvec, read_mtvec);
    CSR_WAIVE(stvec, read_stvec);
#endif // CPU_ROCKET_CHIP
    return has_waive;
  }
};

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
#define REF_RUN_AHEAD(f) \
  f(query, difftest_query_ref, void, void *, uint64_t)
#else
#define REF_RUN_AHEAD(f)
#endif

#ifdef DEBUG_MODE_DIFF
#define REF_DEBUG_MODE(f) \
  f(debug_mem_sync, debug_mem_sync, void, uint64_t, void *, size_t)
#else
#define REF_DEBUG_MODE(f)
#endif

#define REF_ALL(f) \
  REF_BASE(f)       \
  REF_RUN_AHEAD(f)  \
  REF_DEBUG_MODE(f)

#define REF_OPTIONAL(f)                                                       \
  f(ref_close, difftest_close, void, )                                        \
  f(ref_set_ramsize, difftest_set_ramsize, void, size_t)                      \
  f(ref_set_mhartid, difftest_set_mhartid, void, int)                         \
  f(ref_put_gmaddr, difftest_put_gmaddr, void, void *)                        \
  f(ref_skip_one, difftest_skip_one, void, bool, bool, uint32_t, uint64_t)    \
  f(ref_guided_exec, difftest_guided_exec, void, void*)                       \
  f(disambiguation_state, difftest_disambiguation_state, int, )

#define RefFunc(func, ret, ...) ret func(__VA_ARGS__)
#define DeclRefFunc(this_func, dummy, ret, ...) RefFunc((*this_func), ret, __VA_ARGS__);

// This class only loads the functions. It should never call anything.
class AbstractRefProxy {
public:
  REF_ALL(DeclRefFunc)

  AbstractRefProxy(int coreid, size_t ram_size, const char *env, const char *file_path);
  ~AbstractRefProxy();

protected:
  REF_OPTIONAL(DeclRefFunc)

private:
  void * const handler;
  void *load_handler(const char *env, const char *file_path);
  template <typename T>
  T load_function(const char *func_name);
};

class Type1Proxy : public RefProxy<Type1Proxy> {
public:
  REF_ALL(DeclRefFunc)

  Type1Proxy(int coreid, size_t ram_size, const char *env = nullptr, const char *file_path = nullptr);
  ~Type1Proxy();

// protected:
  REF_OPTIONAL(DeclRefFunc)

private:
  void * const handler;
  void *load_handler(const char *env, const char *file_path);
  template <typename T>
  T load_function(const char *func_name);
};

// If a functions of REF_OPTIONAL is not implemented, make it a 
// function pointer that's nullptr. Otherwise, make it normal member function.
// TODO: update regcpy to the latest version
// TODO: implement ref_set_ramsize and update ctor
class SpikeProxy : public RefProxy<SpikeProxy> {
public:
  SpikeProxy(int coreid, size_t ram_size = 0) : coreid(coreid) {};
  ~SpikeProxy() {}

  // REF_ALL
  static void ref_init();
  void ref_regcpy(void *dut, bool direction, bool on_demand);
  void ref_csrcpy(void *dut, bool direction);
  void ref_memcpy(uint64_t nemu_addr, void *dut_buf, size_t n, bool direction);
  void ref_exec(uint64_t n);
  void ref_reg_display();
  void update_config(void *config);
  void uarchstatus_sync(void *dut);
  int store_commit(uint64_t *saddr, uint64_t *sdata, uint8_t *smask);
  void raise_intr(uint64_t no);
  void load_flash_bin(void *flash_bin, size_t size);
#ifdef ENABLE_RUNHEAD
  void query(void *result_buffer, uint64_t type);
#endif
#ifdef DEBUG_MODE_DIFF
  void debug_mem_sync(uint64_t addr, void *bytes, size_t size);
#endif

  // REF_OPTIONAL
  void (*ref_close)() = nullptr;
  void (*ref_set_ramsize)(size_t) = nullptr;
  void (*ref_set_mhartid)(int) = nullptr;
  void ref_put_gmaddr(void *addr);
  void (*ref_skip_one)(bool, bool, uint32_t, uint64_t) = nullptr;
  void ref_guided_exec(void *disambiguate_para); // implemented
  int (*disambiguation_state)() = nullptr;
  
  
private:
  size_t coreid;
  static void* handle;
  // REF_ALL
  static void (*sim_init)();
  static void (*sim_regcpy)(size_t coreid, void *dut, bool direction, bool on_demand);
  static void (*sim_csrcpy)(size_t coreid, void *dut, bool direction);
  static void (*sim_memcpy)(size_t coreid, uint64_t nemu_addr, void *dut_buf, size_t n, bool direction);
  static void (*sim_exec)(size_t coreid, uint64_t n);
  static void (*sim_reg_display)(size_t coreid);
  static void (*sim_update_config)(size_t coreid, void *config);
  static void (*sim_uarchstatus_cpy)(size_t coreid, void *dut, bool direction);
  static int (*sim_store_commit)(size_t coreid, uint64_t *saddr, uint64_t *sdata, uint8_t *smask);
  static void (*sim_raise_intr)(size_t coreid, uint64_t no);
  static void (*sim_load_flash_bin)(void *flash_bin, size_t size);
#ifdef ENABLE_RUNHEAD
  static void (*sim_query)(size_t core_id, void *result_buffer, uint64_t type);
#endif
#ifdef DEBUG_MODE_DIFF
  static void (*sim_debug_mem_sync)(uint64_t addr, void *bytes, size_t size);
#endif

  // REF_OPTIONAL
  static void (*sim_close)(size_t); // w/ coreid
  static void (*sim_set_ramsize)(size_t); // w/o coreid
  static void (*sim_set_mhartid)(size_t, int); // w/ coreid
  static void (*sim_put_gmaddr)(void *); // w/o coreid
  static void (*sim_skip_one)(size_t, bool, bool, uint32_t, uint64_t); // w/ coreid
  static void (*sim_guided_exec)(size_t, void *); // w/ coreid
  static int (*sim_disambiguation_state)(size_t); // w/ coreid
};
 


class NemuProxy : public Type1Proxy {
public:
  NemuProxy(int coreid, size_t ram_size = 0);
  ~NemuProxy() {}
};

// class SpikeProxy : public RefProxy {
// public:
//   SpikeProxy(int coreid, size_t ram_size = 0);
//   ~SpikeProxy() {}
// };

class LinkedProxy : public Type1Proxy {
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
  // force set jump target
  bool force_set_jump_target;
  uint64_t jump_target;
};

extern const char *difftest_ref_so;
extern uint8_t* ref_golden_mem;

#endif