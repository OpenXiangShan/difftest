/***************************************************************************************
* Copyright (c) 2026 Beijing Institute of Open Source Chip
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

#ifndef DIFFTEST_INPUT_H
#define DIFFTEST_INPUT_H

#include "common.h"
#include <memory>
#include <vector>

class InputReader {
public:
  InputReader() = default;
  InputReader(const InputReader &) = delete;
  InputReader &operator=(const InputReader &) = delete;
  virtual ~InputReader() = default;

  // read consumes bytes; peek retains them for the next read. Zero means EOF,
  // negative means failure. peek fills the requested prefix unless EOF occurs.
  ssize_t read(void *dest, size_t capacity);
  ssize_t peek(void *dest, size_t size);

  // Compatibility interface for footprint readers and existing memory consumers.
  virtual uint64_t len() {
    return 0;
  }
  virtual uint64_t next();
  virtual uint64_t read_all(void *dest, uint64_t max_bytes = -1ULL);

protected:
  virtual ssize_t read_some(void *dest, size_t capacity) = 0;

private:
  std::vector<uint8_t> prefix;
  size_t prefix_offset = 0;
  bool failed = false;
};

class FileReader : public InputReader {
public:
  explicit FileReader(const char *filename);
  ~FileReader() override;
  uint64_t len() override {
    return file_size;
  }

protected:
  ssize_t read_some(void *dest, size_t capacity) override;

private:
  int fd;
  uint64_t file_size;
};

class StdinReader : public InputReader {
public:
  // Preserve the { total_bytes: uint64_t, bytes: uint8_t[] } input protocol.
  StdinReader();

protected:
  ssize_t read_some(void *dest, size_t capacity) override;

private:
  uint64_t n_bytes;
};

class WimReader : public InputReader {
public:
  WimReader(uint64_t *addr, uint64_t size) : base_addr(reinterpret_cast<uint8_t *>(addr)), size(size) {}
  uint64_t len() override {
    return size;
  }

protected:
  ssize_t read_some(void *dest, size_t capacity) override;

private:
  const uint8_t *base_addr;
  uint64_t size;
  uint64_t index = 0;
};

// Open a filename, framed stdin ("-"), or memory input ("wim@ADDR+SIZE").
std::unique_ptr<InputReader> open_input(const char *spec);

enum class MemoryWriteMode {
  Overwrite,
  SkipZeroWords
};

// Bounded byte transfer shared by raw image and compressed snapshot consumers.
// Only complete zero words are skipped; trailing bytes are always copied.
long read_to_memory(InputReader &input, void *dest, long capacity, MemoryWriteMode mode, bool allow_truncate = false);

#endif // DIFFTEST_INPUT_H
