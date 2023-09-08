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
#include <unistd.h>
#include <dlfcn.h>

uint8_t* ref_golden_mem = NULL;
const char *difftest_ref_so = NULL;

#define check_and_assert(func)                                \
  do {                                                        \
    if (!func) {                                              \
      printf("Failed loading %s: %s\n", #func, dlerror());    \
      fflush(stdout); assert(func);                           \
    }                                                         \
  } while (0);

#ifdef LINKED_REFPROXY_LIB
#define DeclExtRefFunc(dummy, ref_func, ret, ...)             \
  extern "C" { extern RefFunc(ref_func, ret, __VA_ARGS__); }
REF_ALL(DeclExtRefFunc)

// Special notes on these weak symbols:
// For static libraries, the functions are linked at compile-time, as well as for the symbols.
// For dynamic libraries, despite being loaded at run-time, the symbols are looked up at compile-time.
// Be careful if you change the symbols in dynamic libraries after compiling and want to load it at run-time.
#define DeclWeakExtRefFunc(dummy, ref_func, ret, ...)             \
  extern "C" { extern RefFunc(ref_func, ret __attribute__((weak)), __VA_ARGS__); }
REF_OPTIONAL(DeclWeakExtRefFunc)
#endif

AbstractRefProxy::AbstractRefProxy(int coreid, size_t ram_size, const char *env, const char *file_path) :
  handler(load_handler(env, file_path)) {
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

  ref_init();
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

template <typename T>
T AbstractRefProxy::load_function(const char *func_name) {
  check_and_assert(handler);
  return reinterpret_cast<T>(dlsym(handler, func_name));
}

RefProxy::~RefProxy() {
  if (ref_close) {
    ref_close();
  }
}


void RefProxy::regcpy(DiffTestState *dut) {
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
  ref_regcpy(&regs_int, DUT_TO_REF, false);
};

int RefProxy::compare(DiffTestState *dut) {
#define PROXY_COMPARE(field) memcmp(&(dut->field), &(field), sizeof(field))

  const int results[] = {
    PROXY_COMPARE(regs_int),
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

void RefProxy::display(DiffTestState *dut) {
  if (dut) {
#define PROXY_COMPARE_AND_DISPLAY(field, field_names)                   \
do {                                                                  \
  uint64_t *_ptr_dut = (uint64_t *)(&((dut)->field));                 \
  uint64_t *_ptr_ref = (uint64_t *)(&(field));                    \
  for (int i = 0; i < sizeof(field) / sizeof(uint64_t); i ++) {   \
    if (_ptr_dut[i] != _ptr_ref[i]) {                                 \
      printf("%7s different at pc = 0x%010lx, right= 0x%016lx, "      \
              "wrong = 0x%016lx\n", field_names[i], dut->commit[0].pc, \
              _ptr_ref[i], _ptr_dut[i]);                               \
    }                                                                 \
  }                                                                   \
} while(0);

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
  }
  else {
    ref_reg_display();
  }
};

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
#define CSR_WAIVE(field, mapping)                             \
  do {                                                        \
    uint64_t v = mapping(csr.field);                          \
    if (csr.field != dut->csr.field && dut->csr.field == v) { \
      csr.field = v;                                          \
      has_waive = true;                                       \
    }                                                         \
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
NemuProxy::NemuProxy(int coreid, size_t ram_size) :
  RefProxy(coreid, ram_size, NEMU_ENV_VARIABLE, NEMU_SO_FILENAME) {
}

#define SPIKE_ENV_VARIABLE "SPIKE_HOME"
#define SPIKE_SO_FILENAME  "difftest/build/riscv64-spike-so"
SpikeProxy::SpikeProxy(int coreid, size_t ram_size) :
  RefProxy(coreid, ram_size, SPIKE_ENV_VARIABLE, SPIKE_SO_FILENAME) {
}

LinkedProxy::LinkedProxy(int coreid, size_t ram_size) : RefProxy(coreid, ram_size) {
  if (NUM_CORES > 1) {
    printf("LinkedProxy does not support NUM_CORES(%d) > 1. Please use other REFs.\n", NUM_CORES);
    fflush(stdout);
    assert(0);
  }
}
