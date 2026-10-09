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

void MemoryIdxPool::cleanup() {
  if (memory_base)
    munmap(memory_base, capacity * mem_block_size);
  if (shared)
    munmap(shared, sizeof(Shared));
}

bool MemoryIdxPool::write_free_chunk(uint8_t idx, size_t mem_idx) {
  if (shared->stopped.load(std::memory_order_acquire)) {
#if (CONFIG_DMA_CHANNELS > 1)
    chunk_semaphore.store(false, std::memory_order_release);
#endif
    return !aborted();
  }
#if (CONFIG_DMA_CHANNELS <= 1)
  (void)mem_idx;
  if (idx != uint8_t(producer)) {
    fprintf(stderr, "Unexpected packet idx %u, expected %u\n", idx, uint8_t(producer));
    fail();
    return false;
  }
  shared->head.store(++producer, std::memory_order_release);
#else
  size_t page_w_idx = idx + group_w_offset.load(std::memory_order_relaxed);
  uint64_t sequence = (group_w_idx.load(std::memory_order_relaxed) - 1) * MAX_IDX + idx;
  if (memory_order_ptr[page_w_idx].is_free.load(std::memory_order_relaxed) == false) {
    size_t this_group = group_w_idx.load(std::memory_order_relaxed);
    size_t offset = ((this_group & REM_MAX_GROUPING_IDX) * MAX_IDX);
    page_w_idx = idx + offset;
    sequence += MAX_IDX;
    if (memory_order_ptr[page_w_idx].is_free.load(std::memory_order_relaxed) == false) {
      fprintf(stderr, "Duplicate packet idx %u\n", idx);
      fail();
      chunk_semaphore.store(false, std::memory_order_release);
      return false;
    }
    write_next_count.fetch_add(1, std::memory_order_relaxed);
  } else {
    write_count.fetch_add(1, std::memory_order_relaxed);
  }
  // Payload and physical index must be visible before the sorted entry is ready.
  memory_order_ptr[page_w_idx].memblock_idx.store(mem_idx, std::memory_order_relaxed);
  memory_order_ptr[page_w_idx].is_free.store(false, std::memory_order_release);
  if (sequence >= shared->head.load(std::memory_order_relaxed))
    shared->head.store(sequence + 1, std::memory_order_release);
  if (write_count.load(std::memory_order_relaxed) == MAX_IDX) {
    size_t next_w_idx = wait_next_free_group();
    group_w_offset.store((next_w_idx & REM_MAX_GROUPING_IDX) * MAX_IDX);
    write_count.store(write_next_count);
    write_next_count.store(0);
  }
  chunk_semaphore.store(false, std::memory_order_release);
#endif
  return !aborted();
}

char *MemoryIdxPool::get_free_chunk(size_t *mem_idx) {
  if (shared->stopped.load(std::memory_order_acquire))
    return nullptr;
#if (CONFIG_DMA_CHANNELS <= 1)
  if (producer >= safe_until) {
    safe_until = retained_from() + capacity;
    if (producer >= safe_until)
      return nullptr;
  }
  *mem_idx = producer & (capacity - 1);
#else
  while (chunk_semaphore.exchange(true, std::memory_order_acquire)) {
    if (shared->stopped.load(std::memory_order_acquire))
      return nullptr;
    std::this_thread::yield();
  }
  if (shared->stopped.load(std::memory_order_acquire)) {
    chunk_semaphore.store(false, std::memory_order_release);
    return nullptr;
  }
  size_t page_w_idx = mem_chunk_idx.load(std::memory_order_relaxed);
  if (memory_pool_is_free[page_w_idx].load(std::memory_order_relaxed) == false)
    reclaim();
  if (memory_pool_is_free[page_w_idx].load(std::memory_order_relaxed) == false) {
    chunk_semaphore.store(false, std::memory_order_release);
    return nullptr;
  }
  memory_pool_is_free[page_w_idx].store(false);
  mem_chunk_idx.store((page_w_idx + 1) & (capacity - 1), std::memory_order_relaxed);
  *mem_idx = page_w_idx;
#endif
  return memory_base + *mem_idx * mem_block_size;
}

char *MemoryIdxPool::read_busy_chunk() {
  if (aborted() || consumer >= shared->head.load(std::memory_order_acquire))
    return nullptr;
  size_t page_r_idx = consumer & (capacity - 1);
#if (CONFIG_DMA_CHANNELS > 1)
  if (memory_order_ptr[page_r_idx].is_free.load(std::memory_order_acquire))
    return nullptr;
  page_r_idx = memory_order_ptr[page_r_idx].memblock_idx.load(std::memory_order_relaxed);
#endif
  return memory_base + page_r_idx * mem_block_size;
}

#if (CONFIG_DMA_CHANNELS > 1)
void MemoryIdxPool::reclaim() {
  const uint64_t retained = retained_from();
  while (reclaimed < retained) {
    size_t idx = reclaimed & (capacity - 1);
    if (memory_order_ptr[idx].is_free.load(std::memory_order_acquire))
      break;
    size_t physical = memory_order_ptr[idx].memblock_idx.load(std::memory_order_relaxed);
    memory_pool_is_free[physical].store(true, std::memory_order_relaxed);
    if (++reclaimed % MAX_IDX == 0) {
      // Keep occupied entries until the whole group has left every reader.
      size_t offset = (reclaimed - MAX_IDX) & (capacity - 1);
      for (size_t i = 0; i < MAX_IDX; ++i)
        memory_order_ptr[offset + i].is_free.store(true, std::memory_order_relaxed);
      empty_blocks.fetch_add(1, std::memory_order_relaxed);
    }
  }
}

size_t MemoryIdxPool::wait_next_free_group() {
  size_t free_num = empty_blocks.fetch_sub(1, std::memory_order_relaxed) - 1;
  if (free_num <= 2) {
    reclaim();
    while (!shared->stopped.load(std::memory_order_acquire) && empty_blocks.load(std::memory_order_acquire) <= 1) {
      std::this_thread::yield();
      reclaim();
    }
  }
  return group_w_idx.fetch_add(1);
}
#endif

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

void MemoryIdxPool::retire_reader(unsigned id) {
  assert(id < MAX_READERS);
  shared->readers[id].active.store(false, std::memory_order_release);
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
