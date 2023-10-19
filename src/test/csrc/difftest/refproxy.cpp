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

void* SpikeProxy::handle = nullptr;
void (*SpikeProxy::sim_init)() = nullptr;
void (*SpikeProxy::sim_regcpy)(size_t coreid, void *dut, bool direction, bool on_demand) = nullptr;
void (*SpikeProxy::sim_csrcpy)(size_t coreid, void *dut, bool direction) = nullptr;
void (*SpikeProxy::sim_memcpy)(size_t coreid, uint64_t nemu_addr, void *dut_buf, size_t n, bool direction) = nullptr;
void (*SpikeProxy::sim_exec)(size_t coreid, uint64_t n) = nullptr;
void (*SpikeProxy::sim_reg_display)(size_t coreid) = nullptr;
void (*SpikeProxy::sim_update_config)(size_t coreid, void *config) = nullptr;
void (*SpikeProxy::sim_uarchstatus_cpy)(size_t coreid, void *dut, bool direction) = nullptr;
int (*SpikeProxy::sim_store_commit)(size_t coreid, uint64_t *saddr, uint64_t *sdata, uint8_t *smask) = nullptr;
void (*SpikeProxy::sim_raise_intr)(size_t coreid, uint64_t no) = nullptr;
void (*SpikeProxy::sim_load_flash_bin)(void *flash_bin, size_t size) = nullptr;
#ifdef ENABLE_RUNHEAD
  void (*SpikeProxy::sim_query)(size_t core_id, void *result_buffer, uint64_t type) = nullptr;
#endif
#ifdef DEBUG_MODE_DIFF
  void (*SpikeProxy::sim_debug_mem_sync)(paddr_t addr, void *bytes, size_t size) = nullptr;
#endif

// REF_OPTIONAL
void (*SpikeProxy::sim_close)(size_t) = nullptr; // w/ coreid
void (*SpikeProxy::sim_set_ramsize)(size_t) = nullptr; // w/o coreid
void (*SpikeProxy::sim_set_mhartid)(size_t, int) = nullptr; // w/ coreid
void (*SpikeProxy::sim_put_gmaddr)(void *) = nullptr; // w/o coreid
void (*SpikeProxy::sim_skip_one)(size_t, bool, bool, uint32_t, uint64_t) = nullptr; // w/ coreid
void (*SpikeProxy::sim_guided_exec)(size_t, void *) = nullptr; // w/ coreid
int (*SpikeProxy::sim_disambiguation_state)(size_t) = nullptr; // w/ coreid


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

Type1Proxy::Type1Proxy(int coreid, size_t ram_size, const char *env, const char *file_path) :
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

Type1Proxy::~Type1Proxy() {
  if (handler) {
    dlclose(handler);
  }
};

void *Type1Proxy::load_handler(const char *env, const char *file_path) {
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
T Type1Proxy::load_function(const char *func_name) {
  check_and_assert(handler);
  return reinterpret_cast<T>(dlsym(handler, func_name));
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

#define NEMU_ENV_VARIABLE "NEMU_HOME"
#define NEMU_SO_FILENAME  "build/riscv64-nemu-interpreter-so"
NemuProxy::NemuProxy(int coreid, size_t ram_size) :
  Type1Proxy(coreid, ram_size, NEMU_ENV_VARIABLE, NEMU_SO_FILENAME) {
}


LinkedProxy::LinkedProxy(int coreid, size_t ram_size) : Type1Proxy(coreid, ram_size) {
  if (NUM_CORES > 1) {
    printf("LinkedProxy does not support NUM_CORES(%d) > 1. Please use other REFs.\n", NUM_CORES);
    fflush(stdout);
    assert(0);
  }
}

#define SPIKE_ENV_VARIABLE "SPIKE_HOME"
#define SPIKE_SO_FILENAME  "difftest/build/riscv64-spike-so"

void SpikeProxy::ref_init() 
{
  if (difftest_ref_so == NULL) {
    printf("--diff is not given, "
           "try to use $(" SPIKE_ENV_VARIABLE ")/" SPIKE_SO_FILENAME
           " by default\n");
    const char *spike_home = getenv(SPIKE_ENV_VARIABLE);
    if (spike_home == NULL) {
      printf("FATAL: $(" SPIKE_ENV_VARIABLE ") is not defined!\n");
      exit(1);
    }
    const char *so = "/" SPIKE_SO_FILENAME;
    char *buf = (char *)malloc(strlen(spike_home) + strlen(so) + 1);
    strcpy(buf, spike_home);
    strcat(buf, so);
    difftest_ref_so = buf;
  }

  printf("SpikeProxy using %s\n", difftest_ref_so);

  handle = dlmopen(LM_ID_NEWLM, difftest_ref_so, RTLD_LAZY | RTLD_DEEPBIND);
  if (!handle) {
    printf("%s\n", dlerror());
    assert(0);
  }

  sim_init = (void (*)())dlsym(handle, "difftest_init");
  check_and_assert(sim_init);

  sim_regcpy = (void (*)(size_t, void *, bool, bool))dlsym(handle, "difftest_regcpy");
  check_and_assert(sim_regcpy);

  sim_csrcpy = (void (*)(size_t, void *, bool))dlsym(handle, "difftest_csrcpy");
  check_and_assert(sim_csrcpy);

  sim_memcpy = (void (*)(size_t, uint64_t, void *, size_t, bool))dlsym(handle, "difftest_memcpy");
  check_and_assert(sim_memcpy);

  sim_exec = (void (*)(size_t, uint64_t))dlsym(handle, "difftest_exec");
  check_and_assert(sim_exec);

  sim_reg_display = (void (*)(size_t))dlsym(handle, "difftest_display");
  check_and_assert(sim_reg_display);

  sim_update_config = (void (*)(size_t, void *))dlsym(handle, "update_dynamic_config");
  check_and_assert(sim_update_config);

  sim_uarchstatus_cpy = (void (*)(size_t, void *, bool))dlsym(handle, "difftest_uarchstatus_cpy");
  check_and_assert(sim_uarchstatus_cpy);

  sim_store_commit = (int (*)(size_t, uint64_t*, uint64_t*, uint8_t*))dlsym(handle, "difftest_store_commit");
  check_and_assert(sim_store_commit);

  sim_raise_intr = (void (*)(size_t, uint64_t))dlsym(handle, "difftest_raise_intr");
  check_and_assert(sim_raise_intr);

  // core independent
  sim_load_flash_bin = (void (*)(void*, size_t))dlsym(handle, "difftest_load_flash");;
  check_and_assert(sim_load_flash_bin);

#ifdef ENABLE_RUNHEAD
  sim_query = (void (*)(size_t, void*, uint64_t))dlsym(handle, "difftest_query_ref");
  check_and_assert(sim_query);
#endif
#ifdef DEBUG_MODE_DIFF
  // core independent
  sim_debug_mem_sync = (void (*)(uint64_t, void *, size_t))dlsym(handle, "debug_mem_sync");
  check_and_assert(sim_debug_mem_sync);
#endif

  // REF_OPTIONAL
  sim_put_gmaddr = (void (*)(void*))dlsym(handle, "difftest_put_gmaddr");
  if (NUM_CORES > 1) {
    check_and_assert(sim_put_gmaddr);
    assert(ref_golden_mem);
    sim_put_gmaddr(ref_golden_mem);
  }

  sim_guided_exec = (void (*)(size_t, void *))dlsym(handle, "difftest_guided_exec");
  check_and_assert(sim_guided_exec);
  
  sim_init();
}

void SpikeProxy::ref_regcpy(void *dut, bool direction, bool on_demand)
{
  sim_regcpy(coreid, dut, direction, on_demand);
}

void SpikeProxy::ref_csrcpy(void *dut, bool direction)
{
  sim_csrcpy(coreid, dut, direction);
}

void SpikeProxy::ref_memcpy(uint64_t nemu_addr, void *dut_buf, size_t n, bool direction)
{
  sim_memcpy(coreid, nemu_addr, dut_buf, n, direction);
}

void SpikeProxy::ref_exec(uint64_t n)
{
  sim_exec(coreid, n);
}

void SpikeProxy::ref_reg_display()
{
  sim_reg_display(coreid);
}

void SpikeProxy::update_config(void *config)
{
  sim_update_config(coreid, config);
}

void SpikeProxy::uarchstatus_sync(void *dut)
{
  sim_uarchstatus_cpy(coreid, dut, DIFFTEST_TO_REF);
}

int SpikeProxy::store_commit(uint64_t *saddr, uint64_t *sdata, uint8_t *smask)
{
  return sim_store_commit(coreid, saddr, sdata, smask);
}

void SpikeProxy::raise_intr(uint64_t no)
{
  sim_raise_intr(coreid, no);
}

void SpikeProxy::load_flash_bin(void *flash_bin, size_t size)
{
  sim_load_flash_bin(flash_bin, size);
}

#ifdef ENABLE_RUNHEAD
void SpikeProxy::query(void *result_buffer, uint64_t type)
{
  sim_query(coreid, result_buffer, type);
}
#endif

#ifdef DEBUG_MODE_DIFF
void SpikeProxy::debug_mem_sync(uint64_t *addr, void *bytes, size_t size)
{
  sim_debug_mem_sync(addr, bytes, size);
}
#endif

// REF_OPTIONAL
void SpikeProxy::ref_put_gmaddr(void *addr)
{
  sim_put_gmaddr(addr);
}
void SpikeProxy::ref_guided_exec(void *disambiguate_para)
{
  sim_guided_exec(coreid, disambiguate_para);
}







