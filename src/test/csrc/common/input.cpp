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

#include "input.h"
#include <algorithm>
#include <fcntl.h>
#include <iostream>
#include <limits>
#include <sys/stat.h>

ssize_t InputReader::read(void *dest, size_t capacity) {
  if (failed)
    return -1;
  if (capacity == 0)
    return 0;
  size_t available = prefix.size() - prefix_offset;
  if (available) {
    size_t n = std::min(capacity, available);
    memcpy(dest, prefix.data() + prefix_offset, n);
    prefix_offset += n;
    if (prefix_offset == prefix.size()) {
      prefix.clear();
      prefix_offset = 0;
    }
    return n;
  }
  ssize_t n = read_some(dest, capacity);
  failed = n < 0;
  return n;
}

ssize_t InputReader::peek(void *dest, size_t size) {
  if (failed)
    return -1;
  if (prefix_offset) {
    prefix.erase(prefix.begin(), prefix.begin() + prefix_offset);
    prefix_offset = 0;
  }
  while (prefix.size() < size) {
    size_t old_size = prefix.size();
    prefix.resize(size);
    ssize_t n = read_some(prefix.data() + old_size, size - old_size);
    if (n < 0) {
      prefix.resize(old_size);
      failed = true;
      return -1;
    }
    prefix.resize(old_size + n);
    if (n == 0)
      break;
  }
  size_t n = std::min(size, prefix.size());
  if (n)
    memcpy(dest, prefix.data(), n);
  return n;
}

uint64_t InputReader::next() {
  uint64_t value = 0;
  size_t offset = 0;
  while (offset < sizeof(value)) {
    ssize_t n = read(reinterpret_cast<uint8_t *>(&value) + offset, sizeof(value) - offset);
    if (n < 0)
      throw std::runtime_error("Cannot read input word");
    if (n == 0)
      return 0;
    offset += n;
  }
  return value;
}

uint64_t InputReader::read_all(void *dest, uint64_t max_bytes) {
  uint64_t total = 0;
  while (total < max_bytes) {
    size_t chunk = std::min<uint64_t>(max_bytes - total, 1 << 16);
    ssize_t n = read(static_cast<uint8_t *>(dest) + total, chunk);
    if (n < 0)
      throw std::runtime_error("Cannot read input bytes");
    if (n == 0)
      break;
    total += n;
  }
  return total;
}

FileReader::FileReader(const char *filename) {
  fd = open(filename, O_RDONLY);
  if (fd < 0)
    throw std::runtime_error(std::string("Cannot open input '") + filename + "': " + strerror(errno));
  struct stat s;
  if (fstat(fd, &s) < 0) {
    close(fd);
    throw std::runtime_error("Cannot stat input");
  }
  file_size = s.st_size;
}

FileReader::~FileReader() {
  close(fd);
}

ssize_t FileReader::read_some(void *dest, size_t capacity) {
  ssize_t n;
  do {
    n = ::read(fd, dest, capacity);
  } while (n < 0 && errno == EINTR);
  if (n < 0)
    fprintf(stderr, "Cannot read input: %s\n", strerror(errno));
  return n;
}

StdinReader::StdinReader() {
  std::cin.read(reinterpret_cast<char *>(&n_bytes), sizeof(n_bytes));
  if (!std::cin)
    throw std::runtime_error("Cannot read stdin image length");
}

ssize_t StdinReader::read_some(void *dest, size_t capacity) {
  size_t n = std::min<uint64_t>(capacity, n_bytes);
  if (n == 0)
    return 0;
  std::cin.read(static_cast<char *>(dest), n);
  if (static_cast<size_t>(std::cin.gcount()) != n) {
    fprintf(stderr, "Truncated stdin image\n");
    return -1;
  }
  n_bytes -= n;
  return n;
}

ssize_t WimReader::read_some(void *dest, size_t capacity) {
  size_t n = std::min<uint64_t>(capacity, size - index);
  if (n)
    memcpy(dest, base_addr + index, n);
  index += n;
  return n;
}

std::unique_ptr<InputReader> open_input(const char *spec) {
  if (!strcmp(spec, "-"))
    return std::unique_ptr<InputReader>(new StdinReader());
  if (!strncmp(spec, "wim@", 4)) {
    char *end;
    errno = 0;
    uint64_t addr = strtoull(spec + 4, &end, 16);
    if (end == spec + 4 || *end != '+' || errno || addr % sizeof(uint64_t))
      throw std::runtime_error("Invalid wim input address");
    const char *start = end + 1;
    uint64_t size = strtoull(start, &end, 16);
    if (end == start || *end || errno || size > std::numeric_limits<uintptr_t>::max() - addr)
      throw std::runtime_error("Invalid wim input size");
    return std::unique_ptr<InputReader>(new WimReader(reinterpret_cast<uint64_t *>(addr), size));
  }
  return std::unique_ptr<InputReader>(new FileReader(spec));
}

long read_to_memory(InputReader &input, void *dest, long capacity, MemoryWriteMode mode, bool allow_truncate) {
  if (capacity < 0)
    return -1;
  alignas(uint64_t) uint8_t buffer[1 << 16];
  long total = 0;
  while (total < capacity) {
    size_t chunk = std::min<long>(sizeof(buffer), capacity - total);
    // Fill each block so zero-word skipping is independent of decoder chunking.
    size_t used = 0;
    while (used < chunk) {
      ssize_t n = input.read(buffer + used, chunk - used);
      if (n < 0)
        return -1;
      if (n == 0)
        break;
      used += n;
    }
    auto *out = static_cast<uint8_t *>(dest) + total;
    size_t offset = 0;
    if (mode == MemoryWriteMode::SkipZeroWords) {
      while (offset + sizeof(uint64_t) <= used) {
        uint64_t word;
        memcpy(&word, buffer + offset, sizeof(word));
        if (word)
          memcpy(out + offset, &word, sizeof(word));
        offset += sizeof(word);
      }
    }
    memcpy(out + offset, buffer + offset, used - offset);
    total += used;
    if (used < chunk)
      return total;
  }
  if (!allow_truncate) {
    uint8_t extra;
    ssize_t n = input.read(&extra, 1);
    if (n < 0)
      return -1;
    if (n > 0) {
      fprintf(stderr, "Image size exceeds memory capacity (%ld bytes)\n", capacity);
      return -1;
    }
  }
  return total;
}
