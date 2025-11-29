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
#include <atomic>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <vector>
#include <xmmintrin.h>

#define MEMPOOL_SIZE    16384 * 1024 // 16M memory
#define MEMBLOCK_SIZE   4096         // 4K packge
#define NUM_BLOCKS      (MEMPOOL_SIZE / MEMBLOCK_SIZE)
#define REM_NUM_BLOCKS  (NUM_BLOCKS - 1)
#define MAX_WINDOW_SIZE 256

class MemoryChunk {
public:
  std::atomic<size_t> memblock_idx;
  std::atomic<bool> is_free;
  MemoryChunk() : memblock_idx(0), is_free(true) {}

  MemoryChunk(MemoryChunk &&other) noexcept : memblock_idx(other.memblock_idx.load()), is_free(other.is_free.load()) {}

  MemoryChunk(const MemoryChunk &other) : memblock_idx(other.memblock_idx.load()), is_free(other.is_free.load()) {}
};

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

class SpinLock {
  std::atomic_flag locked = ATOMIC_FLAG_INIT;

public:
  void lock() {
    while (locked.test_and_set(std::memory_order_acquire))
      _mm_pause();
  }
  void unlock() {
    locked.clear(std::memory_order_release);
  }
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
  uint64_t mem_block_size = MEMBLOCK_SIZE;

public:
  MemoryIdxPool(uint64_t block_size) : mem_block_size(block_size) {
    size_t total_size = NUM_BLOCKS * mem_block_size;
    void *base = nullptr;
    if (posix_memalign(&base, 4096, total_size) != 0) {
      throw std::runtime_error("Failed to allocate large aligned memory block");
    }
    memset(base, 0, total_size);
    memory_base = static_cast<char *>(base);
    for (size_t i = 0; i < NUM_BLOCKS; ++i) {
      memory_pool[i] = (memory_base + i * mem_block_size);
      memory_order_ptr[i].is_free.store(true);
      memory_pool_is_free[i].store(true);
    }

    printf("MemoryIdxPool using contiguous memory block\n");
  }
  ~MemoryIdxPool() {}

  // Get free block pointer increment is returned from the heap
  char *get_free_chunk(size_t *mem_idx);
  // Write a specified free block of a free window
  bool write_free_chunk(uint8_t idx, size_t mem_idx);

  // Get the head memory
  char *read_busy_chunk();

  // Set the block data valid and locked
  void set_free_chunk();

  // Wait for the data to be free
  size_t wait_next_free_group();

  // Wait for the data to be readable
  size_t wait_next_full_group();

  // Check if there is a window to read
  bool check_group();
  // Wait mempool have data
  void wait_mempool_start();

private:
  char *memory_base = nullptr;
  char *memory_pool[NUM_BLOCKS];                     // Mempool
  std::atomic<bool> memory_pool_is_free[NUM_BLOCKS]; // Mempool free status
  MemoryChunk memory_order_ptr[NUM_BLOCKS];
  std::atomic<bool> chunk_semaphore{false};

  size_t group_r_offset = 0; // The offset used by the current consumer

  std::atomic<size_t> wait_setfree_mem_idx{0};
  std::atomic<size_t> wait_setfree_ptr_idx{0};
  std::atomic<size_t> read_count{0};
  std::atomic<size_t> mem_chunk_idx{0};
  std::atomic<size_t> group_w_offset{0}; // The offset used by the current producer
  std::atomic<size_t> write_count{0};
  std::atomic<size_t> write_next_count{0};
  std::atomic<size_t> empty_blocks{MAX_GROUP_READ};
  std::atomic<size_t> group_w_idx{1};
  std::atomic<size_t> group_r_idx{1};
};

#endif
