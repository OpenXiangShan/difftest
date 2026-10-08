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
#ifndef DIFFTEST_SHARED_PACKET_POOL_H
#define DIFFTEST_SHARED_PACKET_POOL_H

#include <algorithm>
#include <atomic>
#include <cassert>
#include <cstdint>
#include <limits>
#include <new>
#include <stdexcept>
#include <sys/mman.h>

// One producer, one fast reader and forked readers. Cursors name the first
// packet still in use; publishing a cursor releases every preceding packet.
// Slow segments have disjoint windows, but can share a boundary packet.
// Out-of-order completion can leave holes, so only a contiguous prefix is reusable.
class SharedPacketPool {
public:
  static constexpr unsigned MAX_READERS = 128;
  struct alignas(64) Reader {
    std::atomic<uint64_t> cursor{0};
    std::atomic<bool> active{false};
  };
  struct Shared {
    alignas(64) std::atomic<uint64_t> head{0};
    std::atomic<bool> abort{false};
    Reader readers[MAX_READERS];
  };

  SharedPacketPool(size_t count, size_t bytes) : capacity(count), packet_bytes(bytes) {
    if (count < 2 || (count & (count - 1)) != 0 || bytes == 0 || count > std::numeric_limits<size_t>::max() / bytes) {
      throw std::runtime_error("Invalid shared packet pool dimensions");
    }
    shared = static_cast<Shared *>(map(sizeof(Shared)));
    new (shared) Shared();
    if (!shared->head.is_lock_free() || !shared->abort.is_lock_free()) {
      munmap(shared, sizeof(Shared));
      throw std::runtime_error("Shared packet cursors require lock-free atomics");
    }
    try {
      data = static_cast<char *>(map(capacity * packet_bytes));
    } catch (...) {
      munmap(shared, sizeof(Shared));
      throw;
    }
    // mmap supplies zero pages; the producer fills a whole slot before publish.
    shared->readers[0].active.store(true);
    safe_until = capacity;
  }
  ~SharedPacketPool() {
    munmap(data, capacity * packet_bytes);
    munmap(shared, sizeof(Shared));
  }
  SharedPacketPool(const SharedPacketPool &) = delete;
  SharedPacketPool &operator=(const SharedPacketPool &) = delete;

  char *get_free() {
    if (aborted())
      return nullptr;
    if (producer >= safe_until) {
      safe_until = retained_from() + capacity;
      if (producer >= safe_until)
        return nullptr;
    }
    return data + (producer & (capacity - 1)) * packet_bytes;
  }
  void publish() {
    shared->head.store(++producer, std::memory_order_release);
  }
  char *get_busy() const {
    if (aborted() || shared->head.load(std::memory_order_acquire) <= consumer)
      return nullptr;
    return data + (consumer & (capacity - 1)) * packet_bytes;
  }
  void release() {
    shared->readers[reader_id].cursor.store(++consumer, std::memory_order_release);
  }
  void finish_reader() {
    assert(reader_id != 0);
    // No further payload access. Reserve the ID until the parent reaps us,
    // but stop retaining the final packet when exiting from inside its parser.
    shared->readers[reader_id].cursor.store(UINT64_MAX, std::memory_order_release);
  }
  unsigned add_reader() {
    for (unsigned i = 1; i < MAX_READERS; ++i) {
      if (!shared->readers[i].active.load(std::memory_order_acquire)) {
        shared->readers[i].cursor.store(consumer, std::memory_order_relaxed);
        shared->readers[i].active.store(true, std::memory_order_release);
        return i;
      }
    }
    return MAX_READERS;
  }
  void enter_reader(unsigned id) {
    reader_id = id;
    consumer = shared->readers[id].cursor.load(std::memory_order_acquire);
  }
  void retire_reader(unsigned id) {
    shared->readers[id].active.store(false, std::memory_order_release);
  }
  // Includes the fast reader: new fork readers start at its retained packet.
  uint64_t retained_from() const {
    uint64_t oldest = shared->readers[0].cursor.load(std::memory_order_acquire);
    for (unsigned i = 1; i < MAX_READERS; ++i) {
      if (shared->readers[i].active.load(std::memory_order_acquire)) {
        oldest = std::min(oldest, shared->readers[i].cursor.load(std::memory_order_acquire));
      }
    }
    return oldest;
  }
  uint64_t cursor() const {
    return consumer;
  }
  bool aborted() const {
    return shared->abort.load(std::memory_order_acquire);
  }
  void fail() {
    shared->abort.store(true, std::memory_order_release);
  }

  const size_t capacity;
  const size_t packet_bytes;

private:
  static void *map(size_t size) {
    void *p = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (p == MAP_FAILED)
      throw std::runtime_error("Shared packet mmap failed");
    return p;
  }
  Shared *shared;
  char *data;
  uint64_t producer = 0;
  uint64_t safe_until = 0;
  uint64_t consumer = 0;
  unsigned reader_id = 0;
};

extern SharedPacketPool *g_shared_packet_pool;

#endif
