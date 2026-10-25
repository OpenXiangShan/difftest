/***************************************************************************************
* Copyright (c) 2020-2026 Institute of Computing Technology, Chinese Academy of Sciences
* Copyright (c) 2026 Beijing Institute of Open Source Chip (BOSC)
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

#include "mma/backend/mma_backend.h"

void MmaHostDeleter::operator()(uint8_t *buffer) const {
  if (free_host) {
    free_host(buffer);
  } else {
    delete[] buffer;
  }
}

bool MmaHostBuffer::allocate(size_t bytes, size_t slot_count, uint8_t *(*allocate_host)(size_t),
                             void (*free_host)(uint8_t *)) {
  release();
  stride = bytes;
  this->slot_count = slot_count;
  slots.get_deleter().free_host = free_host;
  if (slot_count != 0) {
    slots.reset(allocate_host ? allocate_host(slot_count * bytes) : new uint8_t[slot_count * bytes]);
    if (!slots) {
      return false;
    }
  }
  for (size_t i = 0; i < slot_count; ++i) {
    free_slots.push(slots.get() + i * bytes);
  }
  return true;
}

size_t MmaHostBuffer::slot(const uint8_t *buffer) const {
  if (!slots) {
    return invalid_slot;
  }
  const uintptr_t base = reinterpret_cast<uintptr_t>(slots.get());
  const uintptr_t address = reinterpret_cast<uintptr_t>(buffer);
  if (address < base) {
    return invalid_slot;
  }
  const size_t offset = address - base;
  if (offset >= slot_count * stride || offset % stride != 0) {
    return invalid_slot;
  }
  return offset / stride;
}

uint8_t *MmaHostBuffer::acquire(size_t bytes) {
  if (bytes == stride && slots) {
    std::lock_guard<std::mutex> lock(mutex);
    if (!free_slots.empty()) {
      auto *buffer = free_slots.front();
      free_slots.pop();
      return buffer;
    }
  }
  // Preserve submission backpressure; exhausted pools do not wait.
  return new uint8_t[bytes];
}

void MmaHostBuffer::recycle(uint8_t *buffer) {
  if (slot(buffer) == invalid_slot) {
    delete[] buffer;
    return;
  }
  std::lock_guard<std::mutex> lock(mutex);
  free_slots.push(buffer);
}

void MmaHostBuffer::release() {
  slots.reset();
  while (!free_slots.empty()) {
    free_slots.pop();
  }
  slot_count = 0;
  stride = 0;
}
