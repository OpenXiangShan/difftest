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

#include "refproxy.h"
#include <dlfcn.h>
#include <fstream>
#include <iostream>
#include <unistd.h>
#include <vector>

uint8_t *ref_golden_mem = NULL;
const char *difftest_ref_so = NULL;

#define check_and_assert(func)                             \
  do {                                                     \
    if (!func) {                                           \
      printf("Failed loading %s: %s\n", #func, dlerror()); \
      fflush(stdout);                                      \
      assert(func);                                        \
    }                                                      \
  } while (0);

#ifdef LINKED_REFPROXY_LIB
#define DeclExtRefFunc(dummy, ref_func, ret, ...) \
  extern "C" {                                    \
  extern RefFunc(ref_func, ret, __VA_ARGS__);     \
  }
REF_ALL(DeclExtRefFunc)

// Special notes on these weak symbols:
// For static libraries, the functions are linked at compile-time, as well as for the symbols.
// For dynamic libraries, despite being loaded at run-time, the symbols are looked up at compile-time.
// Be careful if you change the symbols in dynamic libraries after compiling and want to load it at run-time.
#define DeclWeakExtRefFunc(dummy, ref_func, ret, ...)               \
  extern "C" {                                                      \
  extern RefFunc(ref_func, ret __attribute__((weak)), __VA_ARGS__); \
  }
REF_OPTIONAL(DeclWeakExtRefFunc)
#endif

AbstractRefProxy::AbstractRefProxy(int coreid, size_t ram_size, const char *env, const char *file_path)
    : handler(load_handler(env, file_path)) {
#ifdef LINKED_REFPROXY_LIB
#define GetRefFunc(dummy, ref_func, ret, ...) ref_func
#else
#define GetRefFunc(dummy, ref_func, ret, ...) load_function<RefFunc((*), ret, __VA_ARGS__)>(#ref_func)
#endif
#define LoadRefFunc(this_func, ref_func, ret, ...)  this_func = GetRefFunc(, ref_func, ret, __VA_ARGS__);
#define CheckRefFunc(this_func, ref_func, ret, ...) check_and_assert(this_func);

  REF_ALL(LoadRefFunc)
  REF_ALL(CheckRefFunc)
  REF_OPTIONAL(LoadRefFunc)

  if (NUM_CORES > 1) {
    check_and_assert(ref_set_mhartid);
    ref_set_mhartid(coreid);

    check_and_assert(ref_put_gmaddr);
    check_and_assert(ref_golden_mem);
    ref_put_gmaddr(ref_golden_mem);
  }

  // set ram_size before ref_init()
  if (ram_size > 0) {
    check_and_assert(ref_set_ramsize);
    ref_set_ramsize(ram_size);
  }

#ifdef ENABLE_STORE_LOG
  check_and_assert(ref_store_log_reset);
  check_and_assert(ref_store_log_restore);
#endif // ENABLE_STORE_LOG

  if (ref_init_v2) {
    ref_init_v2(sizeof(ref_state_t));
  } else {
    ref_init();
  }
}

AbstractRefProxy::~AbstractRefProxy() {
  if (handler) {
    dlclose(handler);
  }
};

void *AbstractRefProxy::load_handler(const char *env, const char *file_path) {
#ifdef LINKED_REFPROXY_LIB
  Info("The reference model (dynamically or statically linked at compile-time) is %s\n", LINKED_REFPROXY_LIB);
  return nullptr;
#endif

  bool use_given_diff = true;

  if (difftest_ref_so == NULL) {
    const char *ref_home = getenv(env);
    if (ref_home == NULL) {
#ifdef REF_HOME
      ref_home = REF_HOME;
#else
      printf("FATAL: $(%s) is not defined!\n", env);
      exit(1);
#endif // REF_HOME
    }
    char *buf = (char *)malloc(256);
    sprintf(buf, "%s/%s", ref_home, file_path);
    difftest_ref_so = buf;
    use_given_diff = false;
  }

  Info("The reference model is %s\n", difftest_ref_so);

  int mode = RTLD_LAZY | RTLD_DEEPBIND;
  void *so_handler = (NUM_CORES > 1) ? dlmopen(LM_ID_NEWLM, difftest_ref_so, mode) : dlopen(difftest_ref_so, mode);
  check_and_assert(so_handler);

  if (!use_given_diff) {
    free((void *)difftest_ref_so);
    difftest_ref_so = nullptr;
  }

  return so_handler;
}

template <typename T> T AbstractRefProxy::load_function(const char *func_name) {
  check_and_assert(handler);
  return reinterpret_cast<T>(dlsym(handler, func_name));
}

RefProxy::~RefProxy() {
  if (ref_close) {
    ref_close();
  }
}

void RefProxy::regcpy(const DiffTestRegState *regs, uint64_t pc) {
  memcpy(&state.xrf, regs->xrf.value, sizeof(state.xrf));
#ifdef CONFIG_DIFFTEST_ARCHFPREGSTATE
  memcpy(&state.frf, regs->frf.value, sizeof(state.frf));
#endif // CONFIG_DIFFTEST_ARCHFPREGSTATE
  memcpy(&state.csr, &regs->csr, sizeof(state.csr));
  state.pc = pc;
#ifdef CONFIG_DIFFTEST_HCSRSTATE
  memcpy(&state.hcsr, &regs->hcsr, sizeof(state.hcsr));
#endif // CONFIG_DIFFTEST_HCSRSTATE
#ifdef CONFIG_DIFFTEST_ARCHVECREGSTATE
  memcpy(&state.vrf, &regs->vrf.value, sizeof(state.vrf));
#endif // CONFIG_DIFFTEST_ARCHVECREGSTATE
#ifdef CONFIG_DIFFTEST_VECCSRSTATE
  memcpy(&state.vcsr, &regs->vcsr, sizeof(state.vcsr));
#endif // CONFIG_DIFFTEST_VECCSRSTATE
#ifdef CONFIG_DIFFTEST_FPCSRSTATE
  memcpy(&state.fcsr, &regs->fcsr, sizeof(state.fcsr));
#endif // CONFIG_DIFFTEST_FPCSRSTATE
#ifdef CONFIG_DIFFTEST_TRIGGERCSRSTATE
  memcpy(&state.triggercsr, &regs->triggercsr, sizeof(state.triggercsr));
#endif //CONFIG_DIFFTEST_TRIGGERCSRSTATE
  ref_regcpy(&state, DUT_TO_REF, false);
};

int RefProxy::compare(DiffTestState *dut) {
#define PROXY_COMPARE(field) memcmp(&(dut->regs.field), &(state.field), sizeof(state.field))

  const int results[] = {PROXY_COMPARE(xrf),
#ifdef CONFIG_DIFFTEST_ARCHFPREGSTATE
                         PROXY_COMPARE(frf),
#endif // CONFIG_DIFFTEST_ARCHFPREGSTATE
#ifdef CONFIG_DIFFTEST_ARCHVECREGSTATE
                         PROXY_COMPARE(vrf),
#endif // CONFIG_DIFFTEST_ARCHVECREGSTATE
#ifdef CONFIG_DIFFTEST_VECCSRSTATE
                         PROXY_COMPARE(vcsr),
#endif // CONFIG_DIFFTEST_VECCSRSTATE
#ifdef CONFIG_DIFFTEST_FPCSRSTATE
                         PROXY_COMPARE(fcsr),
#endif // CONFIG_DIFFTEST_FPCSRSTATE
#ifdef CONFIG_DIFFTEST_HCSRSTATE
                         PROXY_COMPARE(hcsr),
#endif // CONFIG_DIFFTEST_HCSRSTATE
#ifdef CONFIG_DIFFTEST_TRIGGERCSRSTATE
                         PROXY_COMPARE(triggercsr),
#endif // CONFIG_DIFFTEST_TRIGGERCSRSTATE
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

void RefProxy::display(DiffTestState *dut) {
  if (dut) {
#define PROXY_COMPARE_AND_DISPLAY(field, field_names)                                   \
  do {                                                                                  \
    uint64_t *_ptr_dut = (uint64_t *)(&((dut)->regs.field));                            \
    uint64_t *_ptr_ref = (uint64_t *)(&(state.field));                                  \
    for (int i = 0; i < sizeof(state.field) / sizeof(uint64_t); i++) {                  \
      if (_ptr_dut[i] != _ptr_ref[i]) {                                                 \
        REPORT_DIFFERENCE(field_names[i], dut->commit[0].pc, _ptr_ref[i], _ptr_dut[i]); \
      }                                                                                 \
    }                                                                                   \
  } while (0);

    PROXY_COMPARE_AND_DISPLAY(xrf, regs_name_int)
#ifdef CONFIG_DIFFTEST_ARCHFPREGSTATE
    PROXY_COMPARE_AND_DISPLAY(frf, regs_name_fp)
#endif // CONFIG_DIFFTEST_ARCHFPREGSTATE
    PROXY_COMPARE_AND_DISPLAY(csr, regs_name_csr)
#ifdef CONFIG_DIFFTEST_HCSRSTATE
    PROXY_COMPARE_AND_DISPLAY(hcsr, regs_name_hcsr)
#endif // CONFIG_DIFFTEST_HCSRSTATE
#ifdef CONFIG_DIFFTEST_ARCHVECREGSTATE
    PROXY_COMPARE_AND_DISPLAY(vrf, regs_name_vec)
#endif // CONFIG_DIFFTEST_ARCHVECREGSTATE
#ifdef CONFIG_DIFFTEST_VECCSRSTATE
    PROXY_COMPARE_AND_DISPLAY(vcsr, regs_name_vec_csr)
#endif // CONFIG_DIFFTEST_VECCSRSTATE
#ifdef CONFIG_DIFFTEST_FPCSRSTATE
    PROXY_COMPARE_AND_DISPLAY(fcsr, regs_name_fp_csr)
#endif // CONFIG_DIFFTEST_FPCSRSTATE
#ifdef CONFIG_DIFFTEST_TRIGGERCSRSTATE
    PROXY_COMPARE_AND_DISPLAY(triggercsr, regs_name_triggercsr)
#endif // CONFIG_DIFFTEST_TRIGGERCSRSTATE
  } else {
    ref_reg_display();
  }
};

void RefProxy::flash_init(const uint8_t *flash_base, size_t size, const char *flash_bin) {
  if (load_flash_bin_v2) {
    load_flash_bin_v2(flash_base, size);
  } else if (load_flash_bin) {
    // This API is deprecated because flash_bin may be an empty pointer
    load_flash_bin(flash_bin, size);
  } else {
    std::cout << "Require load_flash_bin or load_flash_bin_v2 to initialize the flash" << std::endl;
    assert(0);
  }
}

#ifdef CPU_ROCKET_CHIP
// similar function as encodeVirtualAddress@RocketCore.scala: 1151
static uint64_t encode_vaddr(uint64_t vaddr) {
  int64_t hi = (int64_t)vaddr >> 39;
  if (hi == 0 || hi == -1) {
    return vaddr;
  }
  hi = ((vaddr >> 38) & 0x1) ? 0 : -1;
  uint64_t mask = (1UL << 39) - 1;
  return (vaddr & mask) | (hi & (~mask));
}

static uint64_t sext_vaddr_40bit(uint64_t vaddr) {
  uint64_t hi = ((vaddr >> 39) & 0x1) ? -1UL : 0;
  uint64_t mask = (1UL << 40) - 1;
  return (vaddr & mask) | (hi & (~mask));
}

static uint64_t read_stvec(uint64_t vaddr) {
  vaddr = vaddr & ((1UL << 39) - 1);
  vaddr = vaddr & (~((vaddr & 0x1) ? 0xfeUL : 0x2UL));
  return ((int64_t)vaddr << 25) >> 25;
}

static uint64_t read_mtvec(uint64_t paddr) {
  paddr = paddr & ((1UL << 32) - 1);
  return paddr & (~((paddr & 0x1) ? 0xfeUL : 0x2UL));
}
#endif // CPU_ROCKET_CHIP

bool RefProxy::do_csr_waive(DiffTestState *dut) {
#define CSR_WAIVE(field, mapping)                                             \
  do {                                                                        \
    uint64_t v = mapping(state.csr.field);                                    \
    if (state.csr.field != dut->regs.csr.field && dut->regs.csr.field == v) { \
      state.csr.field = v;                                                    \
      has_waive = true;                                                       \
    }                                                                         \
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

#define NEMU_ENV_VARIABLE "NEMU_HOME"
#define NEMU_SO_FILENAME  "build/riscv64-nemu-interpreter-so"
NemuProxy::NemuProxy(int coreid, size_t ram_size) : RefProxy(coreid, ram_size, NEMU_ENV_VARIABLE, NEMU_SO_FILENAME) {}

#define SPIKE_ENV_VARIABLE "SPIKE_HOME"
#define SPIKE_SO_FILENAME  "difftest/build/riscv64-spike-so"
SpikeProxy::SpikeProxy(int coreid, size_t ram_size)
    : RefProxy(coreid, ram_size, SPIKE_ENV_VARIABLE, SPIKE_SO_FILENAME) {}

LinkedProxy::LinkedProxy(int coreid, size_t ram_size) : RefProxy(coreid, ram_size) {
  if (NUM_CORES > 1) {
    printf("LinkedProxy does not support NUM_CORES(%d) > 1. Please use other REFs.\n", NUM_CORES);
    fflush(stdout);
    assert(0);
  }
}
