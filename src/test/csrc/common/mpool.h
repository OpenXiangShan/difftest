/***************************************************************************************
* Copyright (c) 2025-2026 Beijing Institute of Open Source Chip (BOSC)
* Copyright (c) 2020-2026 Institute of Computing Technology, Chinese Academy of Sciences
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
#if defined(__x86_64__) || defined(__i386__)
#include <xmmintrin.h>
#endif

#ifndef MEMPOOL_SIZE
#define MEMPOOL_SIZE 16384 * 1024 // Nominal size; MemoryIdxPool allocates packet-sized slots.
#endif
#define MEMBLOCK_SIZE   4096 // 4K packge
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
    while (locked.test_and_set(std::memory_order_acquire)) {
#if defined(__x86_64__) || defined(__i386__)
      _mm_pause();
#elif defined(__aarch64__) || defined(__arm__)
      __asm__ volatile("yield" ::: "memory");
#endif
    }
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

// Shared packet storage with serialized idx sorting and local parser cursors.
class MemoryIdxPool {
public:
  static constexpr unsigned MAX_READERS = 128;
  MemoryIdxPool(uint64_t block_size, size_t count = NUM_BLOCKS);
  ~MemoryIdxPool();
  MemoryIdxPool(const MemoryIdxPool &) = delete;
  MemoryIdxPool &operator=(const MemoryIdxPool &) = delete;

  char *get_free_chunk(size_t *mem_idx);
  bool write_free_chunk(uint8_t idx, size_t mem_idx);
  char *read_busy_chunk();
  void set_free_chunk();
  size_t wait_next_free_group();

  unsigned add_reader();
  void enter_reader(unsigned id);
  void finish_reader();
  void retire_reader(unsigned id);
  uint64_t cursor() const;
  uint64_t retained_from() const;
  bool aborted() const;
  void fail();
  void stop_waiting();

  const size_t capacity;
  const size_t packet_bytes;

private:
  static constexpr size_t MAX_IDX = 256;
  struct alignas(64) Reader {
    std::atomic<uint64_t> cursor{0};
    std::atomic<bool> active{false};
  };
  struct Shared {
    std::atomic<bool> chunk_semaphore{false};
    std::atomic<bool> stopped{false};
    std::atomic<bool> failed{false};
    std::atomic<uint64_t> head{0};
    std::atomic<uint64_t> reader_epoch{0};
    Reader readers[MAX_READERS];
    // Only the serialized producer changes sorting and reclamation state.
    size_t mem_chunk_idx = 0;
    size_t group_w_offset = 0;
    size_t write_count = 0;
    size_t write_next_count = 0;
    size_t empty_blocks = 0;
    uint64_t group_w_idx = 1;
    uint64_t reclaimed = 0;
  };
  static void *map(size_t bytes);
  void reclaim();
  void cleanup();

  Shared *shared = nullptr;
  char *memory_base = nullptr;
  std::atomic<bool> *memory_pool_is_free = nullptr;
  MemoryChunk *memory_order_ptr = nullptr;
  uint64_t consumer = 0;
  unsigned reader_id = 0;
};

extern MemoryIdxPool *g_packet_pool;

#endif
