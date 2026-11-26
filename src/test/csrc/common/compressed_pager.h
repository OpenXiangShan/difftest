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

#ifndef DIFFTEST_COMPRESSED_PAGER_H
#define DIFFTEST_COMPRESSED_PAGER_H

#if defined(__linux__) && !defined(NO_ZSTD_COMPRESSION)
#include <atomic>
#include <cstdint>
#include <mutex>
#include <thread>
#include <vector>

class CompressedPager {
private:
  struct Entry {
    uint64_t offset;
    uint32_t compressed, decoded;
  };
  int data_fd, root_fd, fault_fd;
  uint8_t *canonical;
  uint64_t length, page_size;
  static constexpr uint64_t chunk_size = 64 * 1024;
  std::vector<Entry> entries;
  std::vector<uint8_t> loaded;
  std::vector<uint64_t> aliases;
  std::mutex lock;
  std::atomic<bool> stop{false};
  std::thread worker;

  void serve();

public:
  CompressedPager(const char *index_path, uint64_t expected_size);
  ~CompressedPager();
  void *map_private_branch();
  // Called only during initialization, before private consumers are created.
  void write(uint64_t offset, const void *data, uint64_t size);
};
#endif

#endif // DIFFTEST_COMPRESSED_PAGER_H
