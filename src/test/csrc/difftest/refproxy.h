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

#ifndef __NEMU_PROXY_H
#define __NEMU_PROXY_H

#include <unistd.h>
#include <dlfcn.h>

#include "common.h"

class RefProxy {
public:
  // public callable functions
  void (*memcpy)(paddr_t nemu_addr, void *dut_buf, size_t n, bool direction) = NULL;
  void (*regcpy)(void *dut, bool direction) = NULL;
  void (*csrcpy)(void *dut, bool direction) = NULL;
  void (*uarchstatus_cpy)(void *dut, bool direction) = NULL;
  int (*store_commit)(uint64_t *saddr, uint64_t *sdata, uint8_t *smask) = NULL;
  void (*exec)(uint64_t n) = NULL;
  vaddr_t (*guided_exec)(void *disambiguate_para) = NULL;
  void (*update_config)(void *config) = NULL;
  void (*raise_intr)(uint64_t no) = NULL;
  void (*isa_reg_display)() = NULL;
  void (*query)(void *result_buffer, uint64_t type) = NULL;
  void (*debug_mem_sync)(paddr_t addr, void *bytes, size_t size) = NULL;
  void (*load_flash_bin)(void *flash_bin, size_t size) = NULL;
  void (*set_ramsize)(size_t size) = NULL;
};
extern const char *difftest_ref_so;

#define NEMU_ENV_VARIABLE "NEMU_HOME"
#define NEMU_SO_FILENAME  "build/riscv64-nemu-interpreter-so"
class NemuProxy : public RefProxy {
public:
  NemuProxy(int coreid, size_t ram_size);
private:
};

#define SPIKE_ENV_VARIABLE "SPIKE_HOME"
#define SPIKE_SO_FILENAME  "difftest/build/riscv64-spike-so"
class SpikeProxy : public RefProxy {
public:
  SpikeProxy(int coreid, size_t ram_size);
private:
};

struct SyncState {
  uint64_t lrscValid;
  uint64_t lrscAddr;
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

typedef struct DynamicConfig {
  bool ignore_illegal_mem_access = false;
  bool debug_difftest = false;
} DynamicSimulatorConfig;

void ref_misc_put_gmaddr(uint8_t* ptr);

#endif