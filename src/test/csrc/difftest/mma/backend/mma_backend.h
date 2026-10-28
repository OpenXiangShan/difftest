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

#ifndef __MMA_BACKEND_H__
#define __MMA_BACKEND_H__

#include "common.h"
#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <queue>
#include <vector>

struct MmaHostDeleter {
  void (*free_host)(uint8_t *) = nullptr;
  void operator()(uint8_t *buffer) const;
};

// Default empty pools use ordinary arrays. A backend may initialize FIFO slots
// with its own allocator; acquire/recycle are thread-safe.
// allocate/release require all users to stop and all acquired storage to return.
class MmaHostBuffer {
public:
  static constexpr size_t invalid_slot = SIZE_MAX;

  std::unique_ptr<uint8_t[], MmaHostDeleter> slots;
  size_t slot_count = 0;
  size_t stride = 0;

  bool allocate(size_t bytes, size_t slot_count, uint8_t *(*allocate_host)(size_t) = nullptr,
                void (*free_host)(uint8_t *) = nullptr);

  size_t slot(const uint8_t *buffer) const;

  uint8_t *acquire(size_t bytes);

  void recycle(uint8_t *buffer);

  void release();

private:
  std::mutex mutex;
  std::queue<uint8_t *> free_slots;
};

#ifdef CONFIG_DIFFTEST_AMUCTRLEVENT

static constexpr size_t kMmaSourceRowBytes = CONFIG_DIFF_AMU_TRLEN / 8;
static constexpr size_t kMmaResultRowBytes = CONFIG_DIFF_AMU_ARLEN / 8;

bool compare_mma_rows(const void *lhs, const void *rhs, size_t rows, size_t row_bytes);

class MmaResultPool;

struct MatrixResultDeleter {
  std::shared_ptr<MmaResultPool> pool;
  void operator()(uint64_t *buffer) const;
};

// Move the whole lease when transferring ownership. To adopt an ordinary array,
// assign a fresh MatrixResult; reset(new[]) would retain a pooled lease's deleter.
using MatrixResult = std::unique_ptr<uint64_t[], MatrixResultDeleter>;

// Typed ordinary-memory ROB storage. A result lease keeps this pool alive even
// when DiffState outlives its verifier, or a first mismatch is retained.
class MmaResultPool : public std::enable_shared_from_this<MmaResultPool> {
public:
  MmaResultPool(size_t bytes, size_t capacity);
  MatrixResult acquire();
  void recycle(uint64_t *buffer);

private:
  const size_t words;
  const size_t capacity;
  std::mutex mutex;
  std::vector<std::unique_ptr<uint64_t[]>> storage;
  std::vector<uint64_t *> free_slots;
};

struct MmaVerificationBuffer;

enum MmaElementType : uint8_t {
  Fp8E5M2 = 0,
  Fp16 = 1,
  Fp32 = 2,
  Fp8E4M3 = 4,
  Bf16 = 5,
  Tf32 = 6,
};

static inline bool is_mma_32bit_result_type(uint8_t type) {
  return (type & 3) == 2;
}

class MmaBackend {
public:
  virtual ~MmaBackend() = default;
  MmaHostBuffer host[3];

  /**
   * @brief Verifies a FIFO-ordered batch and writes one result per request.
   *
   * On a numerical mismatch, src3 contains the computed reference result for
   * diagnostics. Successful requests need only a passing result; callers must
   * not depend on src3 being updated after successful verification.
   *
   * A singleton batch is the non-batching execution mode.
   */
  virtual void verify(const std::vector<MmaVerificationBuffer *> &buffers, std::vector<uint8_t> &passed) = 0;

  /**
   * @brief Returns whether candidate can share one backend batch with first.
   *
   * The default keeps the existing singleton behavior. Backends may override
   * this when they can execute a homogeneous consecutive batch efficiently.
   */
  virtual bool is_batch_compatible(const MmaVerificationBuffer *first, const MmaVerificationBuffer *candidate) const {
    (void)first;
    (void)candidate;
    return false;
  }
};

#endif // CONFIG_DIFFTEST_AMUCTRLEVENT

#endif // __MMA_BACKEND_H__
