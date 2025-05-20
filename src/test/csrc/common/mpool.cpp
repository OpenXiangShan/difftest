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
#include "mpool.h"
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

// Write a specified free block of a free window
bool MemoryIdxPool::write_free_chunk(uint8_t idx, size_t mem_idx) {
  size_t page_w_idx;

  page_w_idx = idx + group_w_offset.load(std::memory_order_relaxed);
#if (CONFIG_DMA_CHANNELS > 1)
  // Processing of winding data at the boundary
  if (memory_order_ptr[page_w_idx].is_free.load(std::memory_order_relaxed) == false) {
    size_t this_group = group_w_idx.load(std::memory_order_relaxed);
    size_t offset = ((this_group & REM_MAX_GROUPING_IDX) * MAX_IDX);
    page_w_idx = idx + offset;
    write_next_count.fetch_add(1, std::memory_order_relaxed);
    // Lookup failed
    if (memory_order_ptr[page_w_idx].is_free.load(std::memory_order_relaxed) == false) {
      printf("This block has been written, and there is a duplicate packge idx %d\n", idx);
      return false;
    }
  } else {
#endif
    write_count.fetch_add(1, std::memory_order_relaxed);
    // Proceed to the next group
    if (write_count.load(std::memory_order_relaxed) == MAX_IDX) {
      memory_order_ptr[page_w_idx].is_free.store(false, std::memory_order_relaxed);
      memory_order_ptr[page_w_idx].memblock_idx.store(mem_idx, std::memory_order_relaxed);
      size_t next_w_idx = wait_next_free_group();
      group_w_offset.store((next_w_idx & REM_MAX_GROUPING_IDX) * MAX_IDX);
      write_count.store(write_next_count);
      write_next_count.store(0);
      chunk_semaphore.store(false, std::memory_order_release);
      return true;
    }
#if (CONFIG_DMA_CHANNELS > 1)
  }
#endif
  memory_order_ptr[page_w_idx].is_free.store(false);
  memory_order_ptr[page_w_idx].memblock_idx.store(mem_idx);
  chunk_semaphore.store(false, std::memory_order_release);
  return true;
}

char *MemoryIdxPool::get_free_chunk(size_t *mem_idx) {
  while (chunk_semaphore.exchange(true, std::memory_order_acquire)) {
    std::this_thread::yield(); // sleep_for
  }

  size_t page_w_idx = mem_chunk_idx.fetch_add(1, std::memory_order_relaxed);
  if (page_w_idx == NUM_BLOCKS - 1)
    mem_chunk_idx.store(0, std::memory_order_relaxed);

  if (memory_pool_is_free[page_w_idx].load(std::memory_order_relaxed) == true) {
    memory_pool_is_free[page_w_idx].store(false);
    *mem_idx = page_w_idx;
    return memory_pool[page_w_idx];
  }
  return nullptr;
}

void MemoryIdxPool::wait_mempool_start() {
  while (check_group() == false) {}
}

char *MemoryIdxPool::read_busy_chunk() {
  size_t page_r_idx = read_count + group_r_offset;
  if (memory_order_ptr[page_r_idx].is_free.load() == true) {
    printf("An attempt was made to read the block of free %zu\n", page_r_idx);
    return nullptr;
  }
  size_t memory_pool_idx = memory_order_ptr[page_r_idx].memblock_idx.load();
  char *data = memory_pool[memory_pool_idx];
  wait_setfree_mem_idx.store(memory_pool_idx, std::memory_order_relaxed);
  wait_setfree_ptr_idx.store(page_r_idx, std::memory_order_relaxed);

  return data;
}

void MemoryIdxPool::set_free_chunk() {
  memory_order_ptr[wait_setfree_ptr_idx].is_free.store(true, std::memory_order_relaxed);
  memory_pool_is_free[wait_setfree_mem_idx].store(true, std::memory_order_relaxed);
  if (++read_count == MAX_IDX) {
    size_t next_r_idx = wait_next_full_group();
    group_r_offset = ((next_r_idx & REM_MAX_GROUPING_IDX) * MAX_IDX);
    read_count.store(0, std::memory_order_relaxed);
  }
}

size_t MemoryIdxPool::wait_next_free_group() {
  size_t free_num = empty_blocks.fetch_sub(1, std::memory_order_relaxed) - 1;
  //Reserve at least two free blocks
  if (free_num <= 2) {
    while (empty_blocks.load(std::memory_order_acquire) <= 1) {
      std::this_thread::sleep_for(std::chrono::nanoseconds(10));
    }
  }
  return group_w_idx.fetch_add(1);
}

size_t MemoryIdxPool::wait_next_full_group() {
  size_t free_num = empty_blocks.fetch_add(1, std::memory_order_relaxed) + 1;

  if (free_num >= MAX_GROUP_READ) {
    while (empty_blocks.load(std::memory_order_acquire) >= MAX_GROUP_READ) {
      std::this_thread::sleep_for(std::chrono::nanoseconds(10));
    }
  }
  return group_r_idx.fetch_add(1);
}

bool MemoryIdxPool::check_group() {
  bool result = (group_w_idx.load() > group_r_idx.load()) ? true : false;
  return result;
}
