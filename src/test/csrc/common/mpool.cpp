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
#include "mpool.h"
#include <algorithm>
#include <limits>
#include <new>
#include <thread>

void MemoryPool::init_memory_pool() {
  memory_pool.reserve(NUM_BLOCKS);
  for (size_t i = 0; i < NUM_BLOCKS; ++i) {
    memory_pool.emplace_back();
    block_mutexes[i].unlock();
  }
}

void MemoryPool::cleanup_memory_pool() {
  cv_empty.notify_all();
  cv_filled.notify_all();
  memory_pool.clear();
}

void MemoryPool::unlock_thread() {
  cv_empty.notify_all();
  cv_filled.notify_all();
}

char *MemoryPool::get_free_chunk() {
  page_head = (write_index++) & REM_NUM_BLOCKS;
  {
    std::unique_lock<std::mutex> lock(block_mutexes[page_head]);
    cv_empty.wait(lock, [this] { return empty_blocks > 0; });
  }

  --empty_blocks;
  block_mutexes[page_head].lock();
  return memory_pool[page_head].data.get();
}

void MemoryPool::set_busy_chunk() {
  memory_pool[page_head].is_free = false;
  block_mutexes[page_head].unlock();
  cv_filled.notify_one();
  ++filled_blocks;
}

const char *MemoryPool::get_busy_chunk() {
  page_end = (read_index++) & REM_NUM_BLOCKS;
  {
    std::unique_lock<std::mutex> lock(block_mutexes[page_end]);
    cv_filled.wait(lock, [this] { return filled_blocks > 0; });
  }
  --filled_blocks;
  block_mutexes[page_end].lock();
  return memory_pool[page_end].data.get();
}

void MemoryPool::set_free_chunk() {
  memory_pool[page_end].is_free = true;
  block_mutexes[page_end].unlock();
  cv_empty.notify_one();
  ++empty_blocks;
}

void *MemoryIdxPool::map(size_t bytes) {
  void *ptr = mmap(nullptr, bytes, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
  if (ptr == MAP_FAILED)
    throw std::runtime_error("Indexed pool mmap failed");
  return ptr;
}

MemoryIdxPool::MemoryIdxPool(uint64_t block_size, size_t count) : capacity(count), packet_bytes(block_size) {
  const size_t limit = std::numeric_limits<size_t>::max();
  if (count < 2048 || (count & (count - 1)) || block_size == 0 || block_size > limit || count > limit / block_size ||
      count > limit / sizeof(MemoryChunk) || count > limit / sizeof(std::atomic<bool>))
    throw std::runtime_error("Indexed pool requires valid packet size and power of two >=2048 slots");
  try {
    shared = static_cast<Shared *>(map(sizeof(Shared)));
    new (shared) Shared();
    shared->empty_blocks = capacity / MAX_IDX - 2;
    if (!shared->head.is_lock_free() || !shared->reader_epoch.is_lock_free() ||
        !shared->chunk_semaphore.is_lock_free() || !shared->readers[0].cursor.is_lock_free())
      throw std::runtime_error("Indexed pool requires lock-free shared atomics");
    memory_base = static_cast<char *>(map(capacity * packet_bytes));
    memory_pool_is_free = static_cast<std::atomic<bool> *>(map(capacity * sizeof(std::atomic<bool>)));
    memory_order_ptr = static_cast<MemoryChunk *>(map(capacity * sizeof(MemoryChunk)));
    for (size_t i = 0; i < capacity; ++i) {
      new (&memory_pool_is_free[i]) std::atomic<bool>(true);
      new (&memory_order_ptr[i]) MemoryChunk();
    }
    if (!memory_order_ptr[0].memblock_idx.is_lock_free())
      throw std::runtime_error("Indexed pool requires lock-free shared indices");
    shared->readers[0].active.store(true, std::memory_order_release);
  } catch (...) {
    cleanup();
    throw;
  }
}

void MemoryIdxPool::cleanup() {
  if (memory_order_ptr)
    munmap(memory_order_ptr, capacity * sizeof(MemoryChunk));
  if (memory_pool_is_free)
    munmap(memory_pool_is_free, capacity * sizeof(std::atomic<bool>));
  if (memory_base)
    munmap(memory_base, capacity * packet_bytes);
  if (shared)
    munmap(shared, sizeof(Shared));
}

MemoryIdxPool::~MemoryIdxPool() {
  cleanup();
}

bool MemoryIdxPool::write_free_chunk(uint8_t idx, size_t mem_idx) {
  if (shared->stopped.load(std::memory_order_acquire)) {
    shared->chunk_semaphore.store(false, std::memory_order_release);
    return !aborted();
  }
  size_t page_w_idx = idx + shared->group_w_offset;
  uint64_t sequence = (shared->group_w_idx - 1) * MAX_IDX + idx;
#if (CONFIG_DMA_CHANNELS <= 1)
  if (idx != shared->write_count) {
    fprintf(stderr, "Unexpected packet idx %u, expected %zu\n", idx, shared->write_count);
    fail();
    shared->chunk_semaphore.store(false, std::memory_order_release);
    return false;
  }
#endif
#if (CONFIG_DMA_CHANNELS > 1)
  // A repeated idx belongs to the next group while the current group fills.
  if (!memory_order_ptr[page_w_idx].is_free.load(std::memory_order_relaxed)) {
    page_w_idx = idx + ((shared->group_w_idx & (capacity / MAX_IDX - 1)) * MAX_IDX);
    sequence += MAX_IDX;
    if (!memory_order_ptr[page_w_idx].is_free.load(std::memory_order_relaxed)) {
      fprintf(stderr, "Duplicate packet idx %u\n", idx);
      fail();
      shared->chunk_semaphore.store(false, std::memory_order_release);
      return false;
    }
    ++shared->write_next_count;
  } else {
#endif
    ++shared->write_count;
#if (CONFIG_DMA_CHANNELS > 1)
  }
#endif
  // Payload and physical index must be visible before the sorted entry is ready.
  memory_order_ptr[page_w_idx].memblock_idx.store(mem_idx, std::memory_order_relaxed);
  memory_order_ptr[page_w_idx].is_free.store(false, std::memory_order_release);
  if (sequence >= shared->head.load(std::memory_order_relaxed))
    shared->head.store(sequence + 1, std::memory_order_release);
  if (shared->write_count == MAX_IDX) {
    size_t next_w_idx = wait_next_free_group();
    shared->group_w_offset = (next_w_idx & (capacity / MAX_IDX - 1)) * MAX_IDX;
    shared->write_count = shared->write_next_count;
    shared->write_next_count = 0;
  }
  shared->chunk_semaphore.store(false, std::memory_order_release);
  return !aborted();
}

char *MemoryIdxPool::get_free_chunk(size_t *mem_idx) {
  if (shared->stopped.load(std::memory_order_acquire))
    return nullptr;
  while (shared->chunk_semaphore.exchange(true, std::memory_order_acquire)) {
    if (shared->stopped.load(std::memory_order_acquire))
      return nullptr;
    std::this_thread::yield();
  }
  if (shared->stopped.load(std::memory_order_acquire)) {
    shared->chunk_semaphore.store(false, std::memory_order_release);
    return nullptr;
  }
  size_t page_w_idx = shared->mem_chunk_idx;
  if (!memory_pool_is_free[page_w_idx].load(std::memory_order_relaxed))
    reclaim();
  if (!memory_pool_is_free[page_w_idx].load(std::memory_order_relaxed)) {
    shared->chunk_semaphore.store(false, std::memory_order_release);
    return nullptr;
  }
  memory_pool_is_free[page_w_idx].store(false, std::memory_order_relaxed);
  shared->mem_chunk_idx = (page_w_idx + 1) & (capacity - 1);
  *mem_idx = page_w_idx;
  return memory_base + page_w_idx * packet_bytes;
}

void MemoryIdxPool::reclaim() {
  const uint64_t retained = retained_from();
  while (shared->reclaimed < retained) {
    size_t idx = shared->reclaimed & (capacity - 1);
    if (memory_order_ptr[idx].is_free.load(std::memory_order_acquire))
      break;
    size_t physical = memory_order_ptr[idx].memblock_idx.load(std::memory_order_relaxed);
    memory_pool_is_free[physical].store(true, std::memory_order_relaxed);
    if (++shared->reclaimed % MAX_IDX == 0) {
      // Keep occupied entries until the whole group has left every reader.
      size_t offset = (shared->reclaimed - MAX_IDX) & (capacity - 1);
      for (size_t i = 0; i < MAX_IDX; ++i)
        memory_order_ptr[offset + i].is_free.store(true, std::memory_order_relaxed);
      ++shared->empty_blocks;
    }
  }
}

size_t MemoryIdxPool::wait_next_free_group() {
  --shared->empty_blocks;
  if (shared->empty_blocks <= 2) {
    reclaim();
    while (!shared->stopped.load(std::memory_order_acquire) && shared->empty_blocks <= 1) {
      std::this_thread::yield();
      reclaim();
    }
  }
  return shared->group_w_idx++;
}

char *MemoryIdxPool::read_busy_chunk() {
  if (aborted() || consumer >= shared->head.load(std::memory_order_acquire))
    return nullptr;
  const size_t idx = consumer & (capacity - 1);
  if (memory_order_ptr[idx].is_free.load(std::memory_order_acquire))
    return nullptr;
  size_t physical = memory_order_ptr[idx].memblock_idx.load(std::memory_order_relaxed);
  return memory_base + physical * packet_bytes;
}

void MemoryIdxPool::set_free_chunk() {
  shared->readers[reader_id].cursor.store(++consumer, std::memory_order_release);
}

unsigned MemoryIdxPool::add_reader() {
  for (unsigned i = 0; i < MAX_READERS; ++i) {
    if (!shared->readers[i].active.load(std::memory_order_acquire)) {
      shared->readers[i].cursor.store(consumer, std::memory_order_relaxed);
      shared->readers[i].active.store(true, std::memory_order_release);
      shared->reader_epoch.fetch_add(1, std::memory_order_release);
      return i;
    }
  }
  return MAX_READERS;
}

void MemoryIdxPool::enter_reader(unsigned id) {
  assert(id < MAX_READERS);
  reader_id = id;
  consumer = shared->readers[id].cursor.load(std::memory_order_acquire);
}

void MemoryIdxPool::finish_reader() {
  shared->readers[reader_id].cursor.store(UINT64_MAX, std::memory_order_release);
}

void MemoryIdxPool::retire_reader(unsigned id) {
  assert(id < MAX_READERS);
  shared->readers[id].active.store(false, std::memory_order_release);
}

uint64_t MemoryIdxPool::cursor() const {
  return consumer;
}

uint64_t MemoryIdxPool::retained_from() const {
  while (true) {
    const uint64_t epoch = shared->reader_epoch.load(std::memory_order_acquire);
    uint64_t oldest = shared->head.load(std::memory_order_acquire);
    for (unsigned i = 0; i < MAX_READERS; ++i) {
      if (shared->readers[i].active.load(std::memory_order_acquire))
        oldest = std::min(oldest, shared->readers[i].cursor.load(std::memory_order_acquire));
    }
    if (epoch == shared->reader_epoch.load(std::memory_order_acquire))
      return oldest;
  }
}

bool MemoryIdxPool::aborted() const {
  return shared->failed.load(std::memory_order_acquire);
}

void MemoryIdxPool::fail() {
  shared->failed.store(true, std::memory_order_release);
  shared->stopped.store(true, std::memory_order_release);
}

void MemoryIdxPool::stop_waiting() {
  shared->stopped.store(true, std::memory_order_release);
}
