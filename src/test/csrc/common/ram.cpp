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

#include <sys/mman.h>

#include "common.h"
#include "ram.h"
#include "compress.h"

// #define TLB_UNITTEST

#ifdef WITH_DRAMSIM3
#include "cosimulation.h"
CoDRAMsim3 *dram = NULL;
#endif

static uint64_t *ram;
static long img_size = 0;
static pthread_mutex_t ram_mutex;

unsigned long EMU_RAM_SIZE = DEFAULT_EMU_RAM_SIZE;

void* get_img_start() { return &ram[0]; }
long get_img_size() { return img_size; }
void* get_ram_start() { return &ram[0]; }
long get_ram_size() { return EMU_RAM_SIZE; }

void init_ram(const char *img) {
  assert(img != NULL);

  printf("The image is %s\n", img);

  // initialize memory using Linux mmap
  ram = (uint64_t *)mmap(NULL, EMU_RAM_SIZE, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0);
  if (ram == (uint64_t *)MAP_FAILED) {
    printf("Warning: Insufficient phisical memory\n");
    EMU_RAM_SIZE = 128 * 1024 * 1024UL;
    ram = (uint64_t *)mmap(NULL, EMU_RAM_SIZE, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0);
    if (ram == (uint64_t *)MAP_FAILED) {
      printf("Error: Cound not mmap 0x%lx bytes\n", EMU_RAM_SIZE);
      assert(0);
    }
  }
  printf("Using simulated %luMB RAM\n", EMU_RAM_SIZE / (1024 * 1024));

#ifdef TLB_UNITTEST
  //new add
  addpageSv39();
  //new end
#endif

  int ret;
  if (isGzFile(img)) {
    printf("Gzip file detected and loading image from extracted gz file\n");
    img_size = readFromGz(ram, img, EMU_RAM_SIZE, LOAD_RAM);
    assert(img_size >= 0);
  }
  else {
    FILE *fp = fopen(img, "rb");
    if (fp == NULL) {
      printf("Can not open '%s'\n", img);
      assert(0);
    }

    fseek(fp, 0, SEEK_END);
    img_size = ftell(fp);
    if (img_size > EMU_RAM_SIZE) {
      img_size = EMU_RAM_SIZE;
    }

    fseek(fp, 0, SEEK_SET);
    ret = fread(ram, img_size, 1, fp);

    assert(ret == 1);
    fclose(fp);
  }

#ifdef WITH_DRAMSIM3
  dramsim3_init();
#endif

  pthread_mutex_init(&ram_mutex, 0);

}

void ram_finish() {
  munmap(ram, EMU_RAM_SIZE);
#ifdef WITH_DRAMSIM3
  dramsim3_finish();
#endif
  pthread_mutex_destroy(&ram_mutex);
}


extern "C" uint64_t ram_read_helper(uint8_t en, uint64_t rIdx) {
  if (!ram)
    return 0;
  rIdx %= EMU_RAM_SIZE / sizeof(uint64_t);
  uint64_t rdata = (en) ? ram[rIdx] : 0;
  return rdata;
}

extern "C" void ram_write_helper(uint64_t wIdx, uint64_t wdata, uint64_t wmask, uint8_t wen) {
  if (wen && ram) {
    if (wIdx >= EMU_RAM_SIZE / sizeof(uint64_t)) {
      printf("ERROR: ram wIdx = 0x%lx out of bound!\n", wIdx);
      return;
    }
    ram[wIdx] = (ram[wIdx] & ~wmask) | (wdata & wmask);
  }
}

uint64_t pmem_read(uint64_t raddr) {
  if (raddr % sizeof(uint64_t)) {
    printf("Warning: pmem_read only supports 64-bit aligned memory access\n");
  }
  raddr -= 0x2000000000UL;
  return ram_read_helper(1, raddr / sizeof(uint64_t));
}

void pmem_write(uint64_t waddr, uint64_t wdata) {
  if (waddr % sizeof(uint64_t)) {
    printf("Warning: pmem_write only supports 64-bit aligned memory access\n");
  }
  waddr -= 0x2000000000UL;
  return ram_write_helper(waddr / sizeof(uint64_t), wdata, -1UL, 1);
}

#ifdef WITH_DRAMSIM3
void dramsim3_init() {
#if !defined(DRAMSIM3_CONFIG) || !defined(DRAMSIM3_OUTDIR)
  #error DRAMSIM3_CONFIG or DRAMSIM3_OUTDIR is not defined
#endif

  assert(dram == NULL);
  dram = new ComplexCoDRAMsim3(DRAMSIM3_CONFIG, DRAMSIM3_OUTDIR);
  // dram = new SimpleCoDRAMsim3(90);
}

void dramsim3_step() {
  dram->tick();
}

void dramsim3_finish() {
  delete dram;
}

uint64_t memory_response(bool isWrite) {
  auto response = (isWrite) ? dram->check_write_response() : dram->check_read_response();
  if (response) {
    auto meta = static_cast<dramsim3_meta *>(response->req->meta);
    uint64_t response_value = meta->id | (1UL << 32);
    delete meta;
    delete response;
    return response_value;
  }
  return 0;
}

bool memory_request(uint64_t address, uint32_t id, bool isWrite) {
  if (dram->will_accept(address, isWrite)) {
    auto req = new CoDRAMRequest();
    auto meta = new dramsim3_meta;
    req->address = address;
    req->is_write = isWrite;
    meta->id = id;
    req->meta = meta;
    dram->add_request(req);
    return true;
  }
  return false;
}

#endif
