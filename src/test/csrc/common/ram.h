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

#ifndef __RAM_H
#define __RAM_H

#include "common.h"
#include <fstream>
#include <functional>
#include <iostream>
#include <memory>
#include <set>
#include <unordered_map>
#include <vector>

#ifndef DEFAULT_EMU_RAM_SIZE
#define DEFAULT_EMU_RAM_SIZE (256 * 1024 * 1024UL)
#endif

uint64_t pmem_read(uint64_t raddr);
void pmem_write(uint64_t waddr, uint64_t wdata);
extern "C" uint64_t difftest_ram_read(uint64_t rIdx);
extern "C" void difftest_ram_write(uint64_t wIdx, uint64_t wdata, uint64_t wmask);

class InputReader {
public:
  virtual ~InputReader() {};
  virtual uint64_t len() {
    return 0;
  };
  virtual uint64_t next() = 0;
  virtual uint64_t read_all(void *, uint64_t) = 0;
};

class StdinReader : public InputReader {
public:
  StdinReader();
  ~StdinReader() {}
  uint64_t next();
  uint64_t read_all(void *dest, uint64_t max_bytes = -1ULL);

private:
  uint64_t n_bytes;
};

class WimReader : public InputReader {
public:
  WimReader(uint64_t *addr, uint64_t size) : base_addr(addr), size(size), index(0) {}
  ~WimReader() {}
  uint64_t len() {
    return size;
  };
  uint64_t next();
  uint64_t read_all(void *dest, uint64_t max_bytes = -1ULL);

private:
  uint64_t *base_addr;
  uint64_t size, index;
};

class FileReader : public InputReader {
public:
  FileReader(const char *filename);
  ~FileReader() {
    file.close();
  }
  uint64_t len() {
    return file_size;
  };
  uint64_t next();
  uint64_t read_all(void *dest, uint64_t max_bytes = -1ULL);

private:
  std::ifstream file;
  uint64_t file_size;
};

class SimMemory {
private:
  bool is_stdin(const char *image);
  uint64_t *is_wim(const char *image, uint64_t &wim_size);

protected:
  uint64_t memory_size; // in bytes
#ifdef FUZZING
  std::set<uint64_t> accessed_indices;
#endif
  InputReader *createInputReader(const char *image);
  void inline on_access(uint64_t index) {
#ifdef FUZZING
    accessed_indices.insert(index);
#endif
  }

public:
  SimMemory(uint64_t n_bytes) : memory_size(n_bytes) {}
  virtual ~SimMemory();
  virtual uint64_t get_img_size() = 0;
  uint64_t get_size() {
    return memory_size;
  }
  bool in_range_u8(uint64_t address) {
    return address < memory_size;
  }
  bool in_range_u64(uint64_t index) {
    return index < memory_size / sizeof(uint64_t);
  }

  virtual void clone(std::function<void(void *, size_t)> func, bool skip_zero = false) = 0;
  virtual void clone_on_demand(std::function<void(uint64_t, void *, size_t)> func, bool skip_zero = false) {
    clone([func](void *src, size_t n) { func(0, src, n); }, skip_zero);
  }
  virtual uint64_t *as_ptr() {
    return nullptr;
  }
  virtual uint64_t &at(uint64_t index) = 0;
  void display_stats();
};

class MmapMemory : public SimMemory {
private:
  uint64_t *ram;
  uint64_t img_size;

public:
  MmapMemory(const char *image, uint64_t n_bytes);
  virtual ~MmapMemory();
  void clone(std::function<void(void *, uint64_t)> func, bool skip_zero = false) {
    uint64_t n_bytes = skip_zero ? img_size : get_size();
    func(ram, n_bytes);
  }
  uint64_t &at(uint64_t index) {
    on_access(index);
    return ram[index];
  }

  uint64_t *as_ptr() {
    return ram;
  }
  virtual inline uint64_t get_img_size() {
    return img_size;
  }
};

class MmapMemoryWithFootprints : public MmapMemory {
private:
  uint8_t *touched;
  std::ofstream footprints_file;

public:
  MmapMemoryWithFootprints(const char *image, uint64_t n_bytes, const char *footprints_name);
  ~MmapMemoryWithFootprints();
  uint64_t &at(uint64_t index);
};

class FootprintsMemory : public SimMemory {
private:
  std::unordered_map<uint64_t, uint64_t> ram;
  std::ifstream footprints_file;
  std::vector<std::function<void(uint64_t, uint64_t)>> callbacks;
  InputReader *reader;
  uint64_t n_accessed;

protected:
  void add_callback(std::function<void(uint64_t, uint64_t)> func) {
    callbacks.push_back(func);
  }

public:
  FootprintsMemory(const char *footprints_name, uint64_t n_bytes);
  ~FootprintsMemory();
  uint64_t &at(uint64_t index);
  void clone(std::function<void(void *, uint64_t)> func, bool skip_zero = false) {
    printf("clone_instant not support by FootprintsMemory\n");
    assert(0);
  }
  void clone_on_demand(std::function<void(uint64_t, void *, size_t)> func, bool skip_zero = false) {
    auto cb = [func](uint64_t index, uint64_t value) { func(index * sizeof(uint64_t), &value, sizeof(uint64_t)); };
    for (auto i = ram.begin(); i != ram.end(); i++) {
      cb(i->first, i->second);
    }
    add_callback(cb);
  }
  virtual inline uint64_t get_img_size() {
    return reader->len();
  }
};

class LinearizedFootprintsMemory : public FootprintsMemory {
private:
  const char *linear_name;
  uint64_t *linear_memory;
  uint64_t n_touched; // for performance opt

public:
  LinearizedFootprintsMemory(const char *footprints_name, uint64_t n_bytes, const char *linear_name);
  ~LinearizedFootprintsMemory();
  void save_linear_memory(const char *filename);
};

extern SimMemory *simMemory;
// This is to initialize the common mmap RAM
void init_ram(const char *image, uint64_t n_bytes);
void overwrite_ram(const char *gcpt_restore, uint64_t overwrite_nbytes);
void copy_ram(uint64_t copy_ram_offset);
uint64_t parse_ramsize(const char *ramsize_str);

#ifdef WITH_DRAMSIM3

void dramsim3_init(const char *config_file, const char *out_dir);
void dramsim3_step();
void dramsim3_finish();

extern "C" uint64_t memory_response(bool isWrite);
extern "C" bool memory_request(uint64_t address, uint32_t id, bool isWrite);

struct dramsim3_meta {
  uint32_t id;
};

#endif

#endif
