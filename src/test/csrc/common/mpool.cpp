/***************************************************************************************
* Copyright (c) 2024 Beijing Institute of Open Source Chip (BOSC)
* Copyright (c) 2020-2024 Institute of Computing Technology, Chinese Academy of Sciences
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
