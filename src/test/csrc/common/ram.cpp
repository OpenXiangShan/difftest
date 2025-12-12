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

#include "ram.h"
#include "common.h"
#include "compress.h"
#include "elfloader.h"
#include <iostream>
#ifdef CONFIG_DIFFTEST_PERFCNT
#include "perf.h"
#endif // CONFIG_DIFFTEST_PERFCNT

// #define TLB_UNITTEST

#ifdef WITH_DRAMSIM3
#include "cosimulation.h"
CoDRAMsim3 *dram = NULL;
#endif

SimMemory *simMemory = nullptr;

void init_ram(const char *image, uint64_t ram_size) {
  simMemory = new MmapMemory(image, ram_size);
}

#ifdef TLB_UNITTEST
// Note: addpageSv39 only supports pmem base 0x80000000
void addpageSv39() {
//three layers
//addr range: 0x0000000080000000 - 0x0000000088000000 for 128MB from 2GB - 2GB128MB
//the first layer: one entry for 1GB. (512GB in total by 512 entries). need the 2th entries
//the second layer: one entry for 2MB. (1GB in total by 512 entries). need the 0th-63rd entries
//the third layer: one entry for 4KB (2MB in total by 512 entries). need 64 with each one all
#define TOPSIZE    (128 * 1024 * 1024)
#define PAGESIZE   (4 * 1024)            // 4KB = 2^12B
#define ENTRYNUM   (PAGESIZE / 8)        //512 2^9
#define PTEVOLUME  (PAGESIZE * ENTRYNUM) // 2MB
#define PTENUM     (TOPSIZE / PTEVOLUME) // 128MB / 2MB = 64
#define PDDENUM    1
#define PDENUM     1
#define PDDEADDR   (0x88000000 - (PAGESIZE * (PTENUM + 2)))            //0x88000000 - 0x1000*66
#define PDEADDR    (0x88000000 - (PAGESIZE * (PTENUM + 1)))            //0x88000000 - 0x1000*65
#define PTEADDR(i) (0x88000000 - (PAGESIZE * PTENUM) + (PAGESIZE * i)) //0x88000000 - 0x100*64
#define PTEMMIONUM 128
#define PDEMMIONUM 1
#define PTEDEVNUM  128
#define PDEDEVNUM  1

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
  pdde[0] = (((PDDEADDR - PAGESIZE * (PDEMMIONUM + PTEMMIONUM + PDEDEVNUM)) & 0xfffff000) >> 2) | 0x1;

  for (int i = 0; i < PTEDEVNUM; i++) {
    pdedev[ENTRYNUM - PTEDEVNUM + i] =
        (((PDDEADDR - PAGESIZE * (PDEMMIONUM + PTEMMIONUM + PDEDEVNUM + PTEDEVNUM - i)) & 0xfffff000) >> 2) | 0x1;
  }

  for (int outidx = 0; outidx < PTEDEVNUM; outidx++) {
    for (int inidx = 0; inidx < ENTRYNUM; inidx++) {
      ptedev[outidx][inidx] = (((0x30000000 + outidx * PTEVOLUME + inidx * PAGESIZE) & 0xfffff000) >> 2) | 0xf;
    }
  }

  // mmio: 0x40000000 - 0x4fffffff
  pdde[1] = (((PDDEADDR - PAGESIZE * PDEMMIONUM) & 0xfffff000) >> 2) | 0x1;

  for (int i = 0; i < PTEMMIONUM; i++) {
    pdemmio[i] = (((PDDEADDR - PAGESIZE * (PTEMMIONUM + PDEMMIONUM - i)) & 0xfffff000) >> 2) | 0x1;
  }

  for (int outidx = 0; outidx < PTEMMIONUM; outidx++) {
    for (int inidx = 0; inidx < ENTRYNUM; inidx++) {
      ptemmio[outidx][inidx] = (((0x40000000 + outidx * PTEVOLUME + inidx * PAGESIZE) & 0xfffff000) >> 2) | 0xf;
    }
  }

  //0x800000000 - 0x87ffffff
  pdde[2] = ((PDEADDR & 0xfffff000) >> 2) | 0x1;
  //pdde[2] = ((0x80000000&0xc0000000) >> 2) | 0xf;

  for (int i = 0; i < PTENUM; i++) {
    // pde[i] = ((PTEADDR(i)&0xfffff000)>>2) | 0x1;
    pde[i] = (((0x80000000 + i * 2 * 1024 * 1024) & 0xffe00000) >> 2) | 0xf;
  }

  for (int outidx = 0; outidx < PTENUM; outidx++) {
    for (int inidx = 0; inidx < ENTRYNUM; inidx++) {
      pte[outidx][inidx] = (((0x80000000 + outidx * PTEVOLUME + inidx * PAGESIZE) & 0xfffff000) >> 2) | 0xf;
    }
  }

  Info("try to add identical tlb page to ram\n");
  memcpy((char *)ram +
             (TOPSIZE - PAGESIZE * (PTENUM + PDDENUM + PDENUM + PDEMMIONUM + PTEMMIONUM + PDEDEVNUM + PTEDEVNUM)),
         ptedev, PAGESIZE * PTEDEVNUM);
  memcpy((char *)ram + (TOPSIZE - PAGESIZE * (PTENUM + PDDENUM + PDENUM + PDEMMIONUM + PTEMMIONUM + PDEDEVNUM)), pdedev,
         PAGESIZE * PDEDEVNUM);
  memcpy((char *)ram + (TOPSIZE - PAGESIZE * (PTENUM + PDDENUM + PDENUM + PDEMMIONUM + PTEMMIONUM)), ptemmio,
         PAGESIZE * PTEMMIONUM);
  memcpy((char *)ram + (TOPSIZE - PAGESIZE * (PTENUM + PDDENUM + PDENUM + PDEMMIONUM)), pdemmio, PAGESIZE * PDEMMIONUM);
  memcpy((char *)ram + (TOPSIZE - PAGESIZE * (PTENUM + PDDENUM + PDENUM)), pdde, PAGESIZE * PDDENUM);
  memcpy((char *)ram + (TOPSIZE - PAGESIZE * (PTENUM + PDENUM)), pde, PAGESIZE * PDENUM);
  memcpy((char *)ram + (TOPSIZE - PAGESIZE * PTENUM), pte, PAGESIZE * PTENUM);
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
  std::cin.read(reinterpret_cast<char *>(&value), sizeof(uint64_t));
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

// wim@[base_addr],[wim_size]
uint64_t *SimMemory::is_wim(const char *image, uint64_t &wim_size) {
  const char wim_prefix[] = "wim";
  const char *wim_info = strchr(image, '@');
  if (!wim_info || strncmp(wim_prefix, image, sizeof(wim_prefix) - 1)) {
    return nullptr;
  }
  wim_info++;
  uint64_t base_addr = strtoul(wim_info, (char **)&wim_info, 16);
  if (base_addr % sizeof(uint64_t) || *wim_info != '+') {
    return nullptr;
  }
  wim_info++;
  wim_size = strtoul(wim_info, (char **)&wim_info, 16);
  if (*wim_info) {
    return nullptr;
  }
  return (uint64_t *)base_addr;
}

uint64_t WimReader::next() {
  if (index + sizeof(uint64_t) > size) {
    return 0;
  }
  uint64_t value = base_addr[index / sizeof(uint64_t)];
  index += sizeof(uint64_t);
  return value;
}

uint64_t WimReader::read_all(void *dest, uint64_t max_bytes) {
  uint64_t n_read = size - index;
  if (n_read >= max_bytes) {
    n_read = max_bytes;
  }
  memcpy(dest, base_addr, n_read);
  index += n_read;
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
    file.read(reinterpret_cast<char *>(&value), sizeof(uint64_t));
    if (!file.fail()) {
      return value;
    }
  }
  return 0;
}

uint64_t FileReader::read_all(void *dest, uint64_t max_bytes) {
  uint64_t read_size = (file_size > max_bytes) ? max_bytes : file_size;
  file.read(static_cast<char *>(dest), read_size);
  return read_size;
}

InputReader *SimMemory::createInputReader(const char *image) {
  if (is_stdin(image)) {
    return new StdinReader();
  }
  uint64_t n_bytes;
  if (uint64_t *ptr = is_wim(image, n_bytes)) {
    return new WimReader(ptr, n_bytes);
  }
  return new FileReader(image);
}

void SimMemory::display_stats() {
#ifdef FUZZING
  uint64_t req_in_range = 0;
  auto const img_indices = get_img_size() / sizeof(uint64_t);
  for (auto index: accessed_indices) {
    if (index < img_indices) {
      req_in_range++;
    }
  }
  auto req_all = accessed_indices.size();
  printf("SimMemory: img_size %lu, req_all %lu, req_in_range %lu\n", img_indices, req_all, req_in_range);
#endif // FUZZING
}

MmapMemory::MmapMemory(const char *image, uint64_t n_bytes) : SimMemory(n_bytes) {
  // initialize memory using Linux mmap
  ram = (uint64_t *)mmap(NULL, memory_size, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE | MAP_NORESERVE, -1, 0);
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
  } else if (isZstdFile(image)) {
    Info("Zstd file detected and loading image from extracted zstd file\n");
    img_size = readFromZstd(ram, image, memory_size, LOAD_RAM);
    assert(img_size >= 0);
  } else if (isElfFile(image)) {
    Info("ELF file detected and loading image from extracted elf file\n");
    img_size = readFromElf(ram, image, memory_size);
    assert(img_size >= 0);
  } else {
    InputReader *reader = createInputReader(image);
    img_size = reader->read_all(ram, memory_size);
    delete reader;
  }
}

MmapMemory::~MmapMemory() {
  munmap(ram, memory_size);
#ifdef WITH_DRAMSIM3
  dramsim3_finish();
#endif
}

uint64_t difftest_ram_read(uint64_t rIdx) {
#ifdef CONFIG_DIFFTEST_PERFCNT
  difftest_calls[perf_difftest_ram_read]++;
  difftest_bytes[perf_difftest_ram_read] += 8;
#endif // CONFIG_DIFFTEST_PERFCNT
  if (!simMemory)
    return 0;
#ifdef PMEM_CHECK
  if (!simMemory->in_range_u64(rIdx)) {
    printf("ERROR: ram rIdx = 0x%lx out of bound!\n", rIdx);
    return 0;
  }
#endif // PMEM_CHECK
  rIdx %= simMemory->get_size() / sizeof(uint64_t);
  uint64_t rdata = simMemory->at(rIdx);
  return rdata;
}

void difftest_ram_write(uint64_t wIdx, uint64_t wdata, uint64_t wmask) {
#ifdef CONFIG_DIFFTEST_PERFCNT
  difftest_calls[perf_difftest_ram_write]++;
  difftest_bytes[perf_difftest_ram_write] += 24;
#endif // CONFIG_DIFFTEST_PERFCNT
  if (simMemory) {
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
  return difftest_ram_read(raddr / sizeof(uint64_t));
}

void pmem_write(uint64_t waddr, uint64_t wdata) {
  if (waddr % sizeof(uint64_t)) {
    printf("Warning: pmem_write only supports 64-bit aligned memory access\n");
  }
  waddr -= PMEM_BASE;
  return difftest_ram_write(waddr / sizeof(uint64_t), wdata, -1UL);
}

MmapMemoryWithFootprints::MmapMemoryWithFootprints(const char *image, uint64_t n_bytes, const char *footprints_name)
    : MmapMemory(image, n_bytes) {
  uint64_t n_touched = memory_size / sizeof(uint64_t);
  touched = (uint8_t *)mmap(NULL, n_touched, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0);
  footprints_file.open(footprints_name, std::ios::binary);
  if (!footprints_file.is_open()) {
    printf("Cannot open %s as the footprints file\n", footprints_name);
    assert(0);
  }
}

MmapMemoryWithFootprints::~MmapMemoryWithFootprints() {
  munmap(touched, memory_size / sizeof(uint64_t));
  footprints_file.close();
}

uint64_t &MmapMemoryWithFootprints::at(uint64_t index) {
  uint64_t &data = MmapMemory::at(index);
  if (!touched[index]) {
    footprints_file.write(reinterpret_cast<const char *>(&data), sizeof(data));
    touched[index] = 1;
  }
  return data;
}

FootprintsMemory::FootprintsMemory(const char *footprints_name, uint64_t n_bytes)
    : SimMemory(n_bytes), reader(createInputReader(footprints_name)), n_accessed(0) {
  printf("The image is %s\n", footprints_name);
  add_callback([this](uint64_t, uint64_t) { this->on_access(this->n_accessed / sizeof(uint64_t)); });
}

FootprintsMemory::~FootprintsMemory() {
  delete reader;
}

uint64_t &FootprintsMemory::at(uint64_t index) {
  if (ram.find(index) == ram.end()) {
    uint64_t value = reader->next();
    ram[index] = value;
    for (auto &cb: callbacks) {
      cb(index, value);
    }
    n_accessed += sizeof(uint64_t);
  }
  return ram[index];
}

LinearizedFootprintsMemory::LinearizedFootprintsMemory(const char *footprints_name, uint64_t n_bytes,
                                                       const char *linear_name)
    : FootprintsMemory(footprints_name, n_bytes), linear_name(linear_name), n_touched(0) {
  linear_memory = (uint64_t *)mmap(nullptr, n_bytes, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
  if (linear_memory == MAP_FAILED) {
    perror("mmap");
    exit(EXIT_FAILURE);
  }
  add_callback([this](uint64_t index, uint64_t value) {
    if (value) {
      linear_memory[index] = value;
      n_touched++;
    }
  });
}

LinearizedFootprintsMemory::~LinearizedFootprintsMemory() {
  save_linear_memory(linear_name);
  munmap(linear_memory, get_size());
}

void LinearizedFootprintsMemory::save_linear_memory(const char *filename) {
  std::ofstream out_file(filename, std::ios::out | std::ios::binary);
  if (!out_file) {
    std::cerr << "Cannot open output file: " << filename << std::endl;
    return;
  }
  // Find the position of the last non-zero element
  uint64_t last_nonzero_index = 0, nonzero_count = 0;
  for (uint64_t i = 0; i < get_size() / sizeof(uint64_t) && nonzero_count < n_touched; ++i) {
    if (linear_memory[i] != 0) {
      last_nonzero_index = i;
      nonzero_count++;
    }
  }
  // Even if all all zeros, we still write one uint64_t.
  size_t n_bytes = (last_nonzero_index + 1) * sizeof(uint64_t);
  out_file.write(reinterpret_cast<char *>(linear_memory), n_bytes);
  out_file.close();
}

void overwrite_ram(const char *gcpt_restore, uint64_t overwrite_nbytes) {
  InputReader *reader = new FileReader(gcpt_restore);
  int overwrite_size = reader->read_all(simMemory->as_ptr(), overwrite_nbytes);
  Info("Overwrite %d bytes from file %s.\n", overwrite_size, gcpt_restore);
  delete reader;
}

void copy_ram(uint64_t copy_ram_offset) {
  uint64_t n_word = (copy_ram_offset + sizeof(uint64_t) - 1) / sizeof(uint64_t);
  for (uint64_t src = 0; src < n_word; src++) {
    uint64_t dest = n_word + src;
    assert(simMemory->in_range_u64(src) && simMemory->in_range_u64(dest));
    auto src_val = simMemory->at(src);
    if (src_val != 0) {
      simMemory->at(dest) = src_val;
    }
  }
  Info("Copying the RAM contents to 0x%lx.\n", copy_ram_offset);
}

uint64_t parse_ramsize(const char *ramsize_str) {
  unsigned long ram_size_value = 0;
  char ram_size_unit[64] = {'\0'};
  sscanf(ramsize_str, "%ld%s", &ram_size_value, (char *)&ram_size_unit);
  assert(ram_size_value > 0);

  if (!strcmp(ram_size_unit, "GB") || !strcmp(ram_size_unit, "gb")) {
    return ram_size_value * 1024 * 1024 * 1024;
  }
  if (!strcmp(ram_size_unit, "MB") || !strcmp(ram_size_unit, "mb")) {
    return ram_size_value * 1024 * 1024;
  }
  if (!strcmp(ram_size_unit, "KB") || !strcmp(ram_size_unit, "kb")) {
    return ram_size_value * 1024;
  }
  if (!strcmp(ram_size_unit, "B") || ram_size_unit[0] == '\0') {
    return ram_size_value;
  }
  printf("Invalid ram size %s\n", ram_size_unit);
  assert(false);
  return 0;
}

#ifdef WITH_DRAMSIM3
void dramsim3_init(const char *config_file, const char *out_dir) {
#if !defined(DRAMSIM3_CONFIG) || !defined(DRAMSIM3_OUTDIR)
#error DRAMSIM3_CONFIG or DRAMSIM3_OUTDIR is not defined
#endif

  config_file = (config_file == nullptr) ? DRAMSIM3_CONFIG : config_file;

  assert(dram == NULL);
  // check config_file is valid
  std::ifstream ifs(config_file);
  if (!ifs) {
    std::cerr << "Cannot open DRAMSIM3 config file: " << config_file << std::endl;
    exit(1);
  }
  ifs.close();

  out_dir = (out_dir == nullptr) ? DRAMSIM3_OUTDIR : out_dir;

  std::cout << "DRAMSIM3 config: " << config_file << std::endl;
  std::cout << "DRAMSIM3 outdir: " << out_dir << std::endl;
  dram = new ComplexCoDRAMsim3(config_file, out_dir);
  // dram = new SimpleCoDRAMsim3(90);
}

void dramsim3_step() {
  if (dram == NULL)
    return;
  dram->tick();
}

void dramsim3_finish() {
  delete dram;
  dram = NULL;
}

uint64_t memory_response(bool isWrite) {
  if (dram == NULL)
    return 0;
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
  if (dram == NULL)
    return false;
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
