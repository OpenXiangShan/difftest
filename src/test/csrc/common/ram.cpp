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

#include <iostream>
#include <sys/mman.h>

#include "common.h"
#include "ram.h"
#include "compress.h"

// #define TLB_UNITTEST

#ifdef WITH_DRAMSIM3
#include "cosimulation.h"
CoDRAMsim3 *dram = NULL;
#endif

SimMemory *simMemory = nullptr;

#ifdef TLB_UNITTEST
// Note: addpageSv39 only supports pmem base 0x80000000
void addpageSv39() {
//three layers
//addr range: 0x0000000080000000 - 0x0000000088000000 for 128MB from 2GB - 2GB128MB
//the first layer: one entry for 1GB. (512GB in total by 512 entries). need the 2th entries
//the second layer: one entry for 2MB. (1GB in total by 512 entries). need the 0th-63rd entries
//the third layer: one entry for 4KB (2MB in total by 512 entries). need 64 with each one all
#define TOPSIZE (128 * 1024 * 1024)
#define PAGESIZE (4 * 1024)  // 4KB = 2^12B
#define ENTRYNUM (PAGESIZE / 8) //512 2^9
#define PTEVOLUME (PAGESIZE * ENTRYNUM) // 2MB
#define PTENUM (TOPSIZE / PTEVOLUME) // 128MB / 2MB = 64
#define PDDENUM 1
#define PDENUM 1
#define PDDEADDR (0x88000000 - (PAGESIZE * (PTENUM + 2))) //0x88000000 - 0x1000*66
#define PDEADDR (0x88000000 - (PAGESIZE * (PTENUM + 1))) //0x88000000 - 0x1000*65
#define PTEADDR(i) (0x88000000 - (PAGESIZE * PTENUM) + (PAGESIZE * i)) //0x88000000 - 0x100*64
#define PTEMMIONUM 128
#define PDEMMIONUM 1
#define PTEDEVNUM 128
#define PDEDEVNUM 1

  uint64_t pdde[ENTRYNUM];
  uint64_t pde[ENTRYNUM];
  uint64_t pte[PTENUM][ENTRYNUM];

  // special addr for mmio 0x40000000 - 0x4fffffff
  uint64_t pdemmio[ENTRYNUM];
  uint64_t ptemmio[PTEMMIONUM][ENTRYNUM];

  // special addr for internal devices 0x30000000-0x3fffffff
  uint64_t pdedev[ENTRYNUM];
  uint64_t ptedev[PTEDEVNUM][ENTRYNUM];

  // dev: 0x30000000-0x3fffffff
  pdde[0] = (((PDDEADDR-PAGESIZE*(PDEMMIONUM+PTEMMIONUM+PDEDEVNUM)) & 0xfffff000) >> 2) | 0x1;

  for (int i = 0; i < PTEDEVNUM; i++) {
    pdedev[ENTRYNUM-PTEDEVNUM+i] = (((PDDEADDR-PAGESIZE*(PDEMMIONUM+PTEMMIONUM+PDEDEVNUM+PTEDEVNUM-i)) & 0xfffff000) >> 2) | 0x1;
  }

  for(int outidx = 0; outidx < PTEDEVNUM; outidx++) {
    for(int inidx = 0; inidx < ENTRYNUM; inidx++) {
      ptedev[outidx][inidx] = (((0x30000000 + outidx*PTEVOLUME + inidx*PAGESIZE) & 0xfffff000) >> 2) | 0xf;
    }
  }

  // mmio: 0x40000000 - 0x4fffffff
  pdde[1] = (((PDDEADDR-PAGESIZE*PDEMMIONUM) & 0xfffff000) >> 2) | 0x1;

  for(int i = 0; i < PTEMMIONUM; i++) {
    pdemmio[i] = (((PDDEADDR-PAGESIZE*(PTEMMIONUM+PDEMMIONUM-i)) & 0xfffff000) >> 2) | 0x1;
  }

  for(int outidx = 0; outidx < PTEMMIONUM; outidx++) {
    for(int inidx = 0; inidx < ENTRYNUM; inidx++) {
      ptemmio[outidx][inidx] = (((0x40000000 + outidx*PTEVOLUME + inidx*PAGESIZE) & 0xfffff000) >> 2) | 0xf;
    }
  }

  //0x800000000 - 0x87ffffff
  pdde[2] = ((PDEADDR & 0xfffff000) >> 2) | 0x1;
  //pdde[2] = ((0x80000000&0xc0000000) >> 2) | 0xf;

  for(int i = 0; i < PTENUM ;i++) {
    // pde[i] = ((PTEADDR(i)&0xfffff000)>>2) | 0x1;
    pde[i] = (((0x80000000+i*2*1024*1024)&0xffe00000)>>2) | 0xf;
  }

  for(int outidx = 0; outidx < PTENUM; outidx++ ) {
    for(int inidx = 0; inidx < ENTRYNUM; inidx++ ) {
      pte[outidx][inidx] = (((0x80000000 + outidx*PTEVOLUME + inidx*PAGESIZE) & 0xfffff000)>>2) | 0xf;
    }
  }

  Info("try to add identical tlb page to ram\n");
  memcpy((char *)ram+(TOPSIZE-PAGESIZE*(PTENUM+PDDENUM+PDENUM+PDEMMIONUM+PTEMMIONUM+PDEDEVNUM+PTEDEVNUM)),ptedev,PAGESIZE*PTEDEVNUM);
  memcpy((char *)ram+(TOPSIZE-PAGESIZE*(PTENUM+PDDENUM+PDENUM+PDEMMIONUM+PTEMMIONUM+PDEDEVNUM)),pdedev,PAGESIZE*PDEDEVNUM);
  memcpy((char *)ram+(TOPSIZE-PAGESIZE*(PTENUM+PDDENUM+PDENUM+PDEMMIONUM+PTEMMIONUM)),ptemmio, PAGESIZE*PTEMMIONUM);
  memcpy((char *)ram+(TOPSIZE-PAGESIZE*(PTENUM+PDDENUM+PDENUM+PDEMMIONUM)), pdemmio, PAGESIZE*PDEMMIONUM);
  memcpy((char *)ram+(TOPSIZE-PAGESIZE*(PTENUM+PDDENUM+PDENUM)), pdde, PAGESIZE*PDDENUM);
  memcpy((char *)ram+(TOPSIZE-PAGESIZE*(PTENUM+PDENUM)), pde, PAGESIZE*PDENUM);
  memcpy((char *)ram+(TOPSIZE-PAGESIZE*PTENUM), pte, PAGESIZE*PTENUM);
}
#endif

SimMemory::~SimMemory() {}

bool SimMemory::is_stdin(const char *image) {
  return !strcmp(image, "-");
}

// Read memory image from the standard input.
// The stdin is formatted as { total_bytes: uint64_t, bytes: uint8_t[] }
StdinReader::StdinReader() : n_bytes(next()) {}

uint64_t StdinReader::next() {
  uint64_t value;
  std::cin.read(reinterpret_cast<char*>(&value), sizeof(uint64_t));
  if (std::cin.fail()) {
    return 0;
  }
  return value;
}

uint64_t StdinReader::read_all(void *dest, uint64_t max_bytes) {
  uint64_t n_read = n_bytes;
  if (n_read >= max_bytes) {
    n_read = max_bytes;
  }
  std::cin.get((char *)dest, n_read);
  n_bytes -= n_read;
  return n_read;
}

FileReader::FileReader(const char *filename) : file(filename, std::ios::binary) {
  if (!file.is_open()) {
    std::cerr << "Cannot open '" << filename << "'\n";
    assert(0);
  }

  // Get the size of the file
  file.seekg(0, std::ios::end);
  file_size = file.tellg();
  file.seekg(0, std::ios::beg);
}

uint64_t FileReader::next() {
  if (!file.eof()) {
    uint64_t value;
    file.read(reinterpret_cast<char*>(&value), sizeof(uint64_t));
    if (!file.fail()) {
      return value;
    }
  }
  return 0;
}

uint64_t FileReader::read_all(void *dest, uint64_t max_bytes) {
  uint64_t read_size = (file_size > max_bytes) ? max_bytes : file_size;
  file.read(static_cast<char*>(dest), read_size);
  return read_size;
}

InputReader *SimMemory::createInputReader(const char *image) {
  if (is_stdin(image)) {
    return new StdinReader();
  }
  return new FileReader(image);
}

void SimMemory::display_stats() {
  uint64_t req_in_range = 0;
  auto const img_indices = get_img_size() / sizeof(uint64_t);
  for (auto index : accessed_indices) {
    if (index < img_indices) {
      req_in_range++;
    }
  }
  auto req_all = accessed_indices.size();
  printf("SimMemory: img_size %lu, req_all %lu, req_in_range %lu\n", img_indices, req_all, req_in_range);
}

MmapMemory::MmapMemory(const char *image, uint64_t n_bytes) : SimMemory(n_bytes) {
  // initialize memory using Linux mmap
  ram = (uint64_t *)mmap(NULL, memory_size, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0);
  if (ram == (uint64_t *)MAP_FAILED) {
    printf("Warning: Insufficient phisical memory\n");
    memory_size = 128 * 1024 * 1024UL;
    ram = (uint64_t *)mmap(NULL, memory_size, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0);
    if (ram == (uint64_t *)MAP_FAILED) {
      printf("Error: Cound not mmap 0x%lx bytes\n", memory_size);
      assert(0);
    }
  }
  Info("Using simulated %luMB RAM\n", memory_size / (1024 * 1024));

#ifdef TLB_UNITTEST
  //new add
  addpageSv39();
  //new end
#endif

  if (image == NULL) {
    img_size = 0;
    return;
  }

  printf("The image is %s\n", image);
  if (isGzFile(image)) {
    Info("Gzip file detected and loading image from extracted gz file\n");
    img_size = readFromGz(ram, image, memory_size, LOAD_RAM);
    assert(img_size >= 0);
  }
  else {
    InputReader *reader = createInputReader(image);
    img_size = reader->read_all(ram, memory_size);
    delete reader;
  }

#ifdef WITH_DRAMSIM3
  dramsim3_init();
#endif
}

MmapMemory::~MmapMemory() {
  munmap(ram, memory_size);
#ifdef WITH_DRAMSIM3
  dramsim3_finish();
#endif
}

extern "C" uint64_t ram_read_helper(uint8_t en, uint64_t rIdx) {
  if (!en || !simMemory)
    return 0;
  rIdx %= simMemory->get_size() / sizeof(uint64_t);
  uint64_t rdata = simMemory->at(rIdx);
  return rdata;
}

extern "C" void ram_write_helper(uint64_t wIdx, uint64_t wdata, uint64_t wmask, uint8_t wen) {
  if (wen && simMemory) {
    if (!simMemory->in_range_u64(wIdx)) {
      printf("ERROR: ram wIdx = 0x%lx out of bound!\n", wIdx);
      return;
    }
    simMemory->at(wIdx) = (simMemory->at(wIdx) & ~wmask) | (wdata & wmask);
  }
}

uint64_t pmem_read(uint64_t raddr) {
  if (raddr % sizeof(uint64_t)) {
    printf("Warning: pmem_read only supports 64-bit aligned memory access\n");
  }
  raddr -= PMEM_BASE;
  return ram_read_helper(1, raddr / sizeof(uint64_t));
}

void pmem_write(uint64_t waddr, uint64_t wdata) {
  if (waddr % sizeof(uint64_t)) {
    printf("Warning: pmem_write only supports 64-bit aligned memory access\n");
  }
  waddr -= PMEM_BASE;
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
