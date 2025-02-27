/***************************************************************************************
* Copyright (c) 2025 Beijing Institute of Open Source Chip (BOSC)
* Copyright (c) 2020-2025 Institute of Computing Technology, Chinese Academy of Sciences
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
#ifndef __MPOOL_H__
#define __MPOOL_H__

#include "common.h"
#include "diffstate.h"
#include <atomic>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <vector>

#define MEMPOOL_SIZE   16384 * 1024 // 16M memory
#define MEMBLOCK_SIZE  4096         // 4K packge
#define NUM_BLOCKS     (MEMPOOL_SIZE / MEMBLOCK_SIZE)
#define REM_NUM_BLOCKS (NUM_BLOCKS - 1)

class MemoryBlock {
public:
  std::unique_ptr<char[], std::function<void(char *)>> data;
  std::atomic<bool> is_free;
  uint64_t mem_block_size = MEMBLOCK_SIZE;
  // Default constructor
  MemoryBlock() : MemoryBlock(MEMBLOCK_SIZE) {}

  // Parameterized constructor
  MemoryBlock(uint64_t size) : is_free(true) {
    mem_block_size = size < MEMBLOCK_SIZE ? MEMBLOCK_SIZE : size;
    void *ptr = nullptr;
    if (posix_memalign(&ptr, 4096, mem_block_size) != 0) {
      throw std::runtime_error("Failed to allocate aligned memory");
    }
    memset(ptr, 0, mem_block_size);
    data = std::unique_ptr<char[], std::function<void(char *)>>(static_cast<char *>(ptr), [](char *p) { free(p); });
  }
  ~MemoryBlock() {
    data.reset();
  }
  // Move constructors
  MemoryBlock(MemoryBlock &&other) noexcept : data(std::move(other.data)), is_free(other.is_free.load()) {}

  // Move assignment operator
  MemoryBlock &operator=(MemoryBlock &&other) noexcept {
    if (this != &other) {
      data = std::move(other.data);
      is_free.store(other.is_free.load());
    }
    return *this;
  }

  // Disable the copy constructor and copy assignment operator
  MemoryBlock(const MemoryBlock &) = delete;
  MemoryBlock &operator=(const MemoryBlock &) = delete;
};

class MemoryPool {
public:
  // Constructor to allocate aligned memory blocks
  MemoryPool() {
    init_memory_pool();
  }

  ~MemoryPool() {
    cleanup_memory_pool();
  }
  // Disable copy constructors and copy assignment operators
  MemoryPool(const MemoryPool &) = delete;
  MemoryPool &operator=(const MemoryPool &) = delete;

  void init_memory_pool();

  // Cleaning up memory pools
  void cleanup_memory_pool();
  // Releasing locks manually
  void unlock_thread();

  // Detect a free block and lock the memory that returns the free block
  char *get_free_chunk();
  // Set block data valid and locked
  void set_busy_chunk();

  // Gets the latest block of memory
  const char *get_busy_chunk();
  // Invalidate and lock the block
  void set_free_chunk();

private:
  std::vector<MemoryBlock> memory_pool;              // Mempool
  std::vector<std::mutex> block_mutexes{NUM_BLOCKS}; // Partition lock array
  std::atomic<size_t> empty_blocks{NUM_BLOCKS};      // Free block count
  std::atomic<size_t> filled_blocks;                 // Filled blocks count
  std::atomic<size_t> write_index;
  std::atomic<size_t> read_index;
  std::condition_variable cv_empty;  // Free block condition variable
  std::condition_variable cv_filled; // Filled block condition variable
  size_t page_head = 0;
  size_t page_end = 0;
};

// Split the memory pool into sliding Windows based on the index width
// Support multi-thread out-of-order write sequential read
class MemoryIdxPool {
private:
  const size_t MAX_IDX = 256;
  const size_t MAX_GROUPING_IDX = NUM_BLOCKS / MAX_IDX;
  const size_t MAX_GROUP_READ = MAX_GROUPING_IDX - 2; //The window needs to reserve two free Spaces
  const size_t REM_MAX_IDX = (MAX_IDX - 1);
  const size_t REM_MAX_GROUPING_IDX = (MAX_GROUPING_IDX - 1);
  uint64_t mem_block_size = 4096;

public:
  MemoryIdxPool(uint64_t block_size) {
    mem_block_size = block_size;
    initMemoryPool();
  }
  MemoryIdxPool() : MemoryIdxPool(4096) {}
  ~MemoryIdxPool() {
    cleanupMemoryPool();
  }
  // Disable copy constructors and copy assignment operators
  MemoryIdxPool(const MemoryIdxPool &) = delete;
  MemoryIdxPool &operator=(const MemoryIdxPool &) = delete;

  void initMemoryPool() {
    memory_pool.clear();
    memory_pool.reserve(MEMBLOCK_SIZE);
    for (size_t i = 0; i < MEMBLOCK_SIZE; ++i) {
      memory_pool.emplace_back(mem_block_size);
    }
  }

  // Cleaning up memory pools
  void cleanupMemoryPool();

  // Write a specified free block of a free window
  bool write_free_chunk(uint8_t idx, const char *data);

  // Get the head memory
  bool read_busy_chunk(char *data);

  // Wait for the data to be free
  size_t wait_next_free_group();

  // Wait for the data to be readable
  size_t wait_next_full_group();

  // Check if there is a window to read
  bool check_group();

  // Wait mempool have data
  void wait_mempool_start();

private:
  std::vector<MemoryBlock> memory_pool; // Mempool
  std::mutex window_mutexes;            // window sliding protection
  std::mutex offset_mutexes;            // w/r offset protection
  std::condition_variable cv_empty;     // Free block condition variable
  std::condition_variable cv_filled;    // Filled block condition variable

  size_t group_r_offset = 0; // The offset used by the current consumer
  size_t group_w_offset = 0; // The offset used by the current producer
  size_t read_count = 0;
  size_t write_count = 0;
  size_t write_next_count = 0;

  std::atomic<size_t> empty_blocks{MAX_GROUP_READ};
  std::atomic<size_t> group_w_idx{1};
  std::atomic<size_t> group_r_idx{1};
};

#endif
