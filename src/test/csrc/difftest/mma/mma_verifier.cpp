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

#include "mma/mma_verifier.h"
#include "mma/backend/mma_backend_cpu.h"
#include "mma/backend/mma_backend_cuda.h"
#include "mma/mma_batch.h"

#ifdef CONFIG_DIFFTEST_AMUCTRLEVENT
#include <cstdio>
#include <cstring>

#ifndef CONFIG_DIFF_MMA_BATCH_SIZE
#define CONFIG_DIFF_MMA_BATCH_SIZE 64
#endif

#ifndef CONFIG_DIFF_MMA_BACKPRESSURE_FACTOR
#define CONFIG_DIFF_MMA_BACKPRESSURE_FACTOR 4
#endif

static constexpr size_t kMmaBackpressurePendingLimit =
    static_cast<size_t>(CONFIG_DIFF_MMA_BATCH_SIZE) * CONFIG_DIFF_MMA_BACKPRESSURE_FACTOR;

void bitmte_hash_bytes(MatrixHash128 &hash, const uint8_t *data, size_t bytes) {
  for (size_t byte = 0; byte < bytes; ++byte) {
    for (unsigned bit = 0; bit < 8; ++bit) {
      const bool top = (hash.hi >> 63) != 0;
      hash.hi = (hash.hi << 1) | (hash.lo >> 63);
      hash.lo <<= 1;
      if (top) hash.lo ^= 0x87;
    }
    hash.lo ^= data[byte];
    ++hash.bytes;
  }
}

MmaVerifier::MmaVerifier() {
  Assert(CONFIG_DIFF_MMA_BATCH_SIZE > 0, "CONFIG_DIFF_MMA_BATCH_SIZE must be positive");
  Assert(CONFIG_DIFF_MMA_BACKPRESSURE_FACTOR > 0, "CONFIG_DIFF_MMA_BACKPRESSURE_FACTOR must be positive");
#ifdef CONFIG_DIFFTEST_MMA_CUDA
  backend = new CudaMmaBackend();
  const char *backend_name = "CUDA";
#else
  backend = new CpuMmaBackend();
  const char *backend_name = "CPU";
#endif
  packed_backend = new CpuMmaBackend();
  Info("[INFO] MMA reference model backend: %s\n", backend_name);
}

MmaVerifier::~MmaVerifier() {
  stop();
  // Single-threaded here (verification thread already joined), no lock needed
  while (!mma_verification_queue.empty()) {
    auto buffer = mma_verification_queue.front();
    mma_verification_queue.pop();
    mma_pending_count--;
    free_buffer(buffer);
  }
  MmaVerificationBuffer *err_buf = mma_error_buffer.exchange(nullptr);
  if (err_buf) {
    free_buffer(err_buf);
  }
  delete packed_backend;
  if (backend) {
    delete backend;
    backend = nullptr;
  }
}

void MmaVerifier::start() {
  if (mma_verification_thread.joinable()) {
    return;
  }

  std::lock_guard<std::mutex> lock(mma_queue_mutex);
  mma_stop_requested = false;
  mma_verification_thread = std::thread(&MmaVerifier::mma_verification_thread_func, this);
}

void MmaVerifier::stop() {
  {
    std::lock_guard<std::mutex> lock(mma_queue_mutex);
    mma_stop_requested = true;
  }
  mma_worker_cv.notify_all();
  mma_flush_cv.notify_all();
  mma_backpressure_cv.notify_all();

  if (mma_verification_thread.joinable()) {
    mma_verification_thread.join();
  }
}

bool MmaVerifier::flush() {
  std::unique_lock<std::mutex> lock(mma_queue_mutex);
  Assert(mma_pending_count == 0 || (mma_verification_thread.joinable() && !mma_stop_requested),
         "MMA verifier is not running with %zu pending requests", mma_pending_count);

  mma_flush_cv.wait(lock, [this] { return mma_pending_count == 0 || mma_stop_requested; });

  if (mma_pending_count != 0) {
    Assert(false, "MMA verifier stopped with %zu pending requests", mma_pending_count);
    return false;
  }
  return !has_mma_verification_error();
}

MmaVerificationBuffer *MmaVerifier::allocate_buffer(const DifftestAmuCtrlEvent *amu_event) {
  size_t m = amu_event->mtilem;
  size_t n = amu_event->mtilen;
  size_t k = amu_event->mtilek;
  size_t element_sz_s1 = get_element_size(amu_event->types1);
  size_t element_sz_s2 = get_element_size(amu_event->types2);
  size_t element_sz_d = get_element_size(amu_event->typed);

  size_t a_sz = amu_event->packedMode ? kBitMteTrBytes : element_sz_s1 * m * k;
  size_t b_sz = amu_event->packedMode ? kBitMteTrBytes : element_sz_s2 * k * n;
  size_t c_sz = amu_event->packedMode ? kBitMteAccBytes : element_sz_d * m * n;

  // Allocate memory for the buffer structure and data
  MmaVerificationBuffer *buffer = new MmaVerificationBuffer();
  buffer->amu_event = *amu_event;

  // Allocate memory for each source matrix and result
  buffer->src1 = new uint8_t[a_sz];
  buffer->src2 = new uint8_t[b_sz];
  buffer->src3 = new uint8_t[c_sz];
  buffer->dut_result = new uint8_t[c_sz];

  return buffer;
}

void MmaVerifier::free_buffer(MmaVerificationBuffer *buffer) {
  if (buffer) {
    if (buffer->src1) {
      delete[] buffer->src1;
      delete[] buffer->src2;
      delete[] buffer->src3;
      delete[] buffer->dut_result;
    }
    delete buffer;
  }
}

void MmaVerifier::add_to_verification_queue(MmaVerificationBuffer *buffer) {
  {
    std::unique_lock<std::mutex> lock(mma_queue_mutex);
    mma_backpressure_cv.wait(lock,
                             [this] { return mma_stop_requested || mma_pending_count < kMmaBackpressurePendingLimit; });
    mma_verification_queue.push(buffer);
    mma_pending_count++;
  }
  mma_worker_cv.notify_one();
}

bool MmaVerifier::has_mma_verification_error() const {
  return mma_error_buffer.load(std::memory_order_acquire) != nullptr;
}

void MmaVerifier::pending_counts(size_t &queued, size_t &executing) {
  std::lock_guard<std::mutex> lock(mma_queue_mutex);
  queued = mma_verification_queue.size();
  executing = mma_pending_count - queued;
}

const MmaVerificationBuffer *MmaVerifier::get_error_buffer() const {
  return mma_error_buffer.load(std::memory_order_acquire);
}

void MmaVerifier::mma_verification_thread_func() {
  while (true) {
    std::vector<MmaVerificationBuffer *> buffers;
    MmaBackend *selected_backend = nullptr;

    // Wait for work, then remove only a consecutive compatible FIFO prefix.
    {
      std::unique_lock<std::mutex> lock(mma_queue_mutex);
      mma_worker_cv.wait(lock, [this] { return mma_stop_requested || !mma_verification_queue.empty(); });

      if (mma_stop_requested) {
        break;
      }

      selected_backend = mma_verification_queue.front()->amu_event.packedMode ? packed_backend : backend;
      buffers = take_contiguous_batch(mma_verification_queue, CONFIG_DIFF_MMA_BATCH_SIZE, *selected_backend);
    }

    std::vector<uint8_t> passed;
    selected_backend->verify(buffers, passed);
    Assert(passed.size() == buffers.size(), "MMA backend returned %zu results for a batch of %zu", passed.size(),
           buffers.size());

    // A batch is FIFO ordered, so publishing failures in vector order keeps
    // the first reported mismatch identical to singleton verification.
    for (size_t i = 0; i < buffers.size(); ++i) {
      auto *buffer = buffers[i];
      if (buffer->amu_event.packedMode) packed_checked.fetch_add(1, std::memory_order_release);
      if (passed[i]) {
        free_buffer(buffer);
      } else {
        MmaVerificationBuffer *expected = nullptr;
        if (!mma_error_buffer.compare_exchange_strong(expected, buffer, std::memory_order_acq_rel)) {
          free_buffer(buffer);
        }
      }
    }

    bool drained = false;
    {
      std::lock_guard<std::mutex> lock(mma_queue_mutex);
      Assert(mma_pending_count >= buffers.size(), "MMA pending count underflow: %zu < %zu", mma_pending_count,
             buffers.size());
      mma_pending_count -= buffers.size();
      drained = (mma_pending_count == 0);
    }

    mma_backpressure_cv.notify_all();
    if (drained) {
      mma_flush_cv.notify_all();
    }
  }
  mma_flush_cv.notify_all();
}

#endif // CONFIG_DIFFTEST_AMUCTRLEVENT
