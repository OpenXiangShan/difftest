/***************************************************************************************
* Copyright (c) 2020-2023 Institute of Computing Technology, Chinese Academy of Sciences
* Copyright (c) 2020-2021 Peng Cheng Laboratory
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

#include "mma_verifier.h"
#include "mma_backend/mma_backend_cpu.h"
#include "mma_backend/mma_backend_cuda.h"

#ifdef CONFIG_DIFFTEST_AMUCTRLEVENT
#include <cstring>

MmaVerifier::MmaVerifier() : mma_thread_running(false) {
#ifdef CONFIG_DIFFTEST_MMA_CUDA
  backend = new CudaMmaBackend();
#else
  backend = new CpuMmaBackend();
#endif
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
  if (backend) {
    delete backend;
    backend = nullptr;
  }
}

void MmaVerifier::start() {
  if (mma_thread_state.load(std::memory_order_acquire) == ThreadState::NotStarted) {
    mma_thread_running.store(true, std::memory_order_release);
    mma_thread_state.store(ThreadState::Running, std::memory_order_release);
    mma_verification_thread = std::thread(&MmaVerifier::mma_verification_thread_func, this);
  }
}

void MmaVerifier::stop() {
  if (mma_thread_state.load(std::memory_order_acquire) == ThreadState::Running) {
    mma_thread_running.store(false, std::memory_order_release);
    mma_queue_cv.notify_all();
  }
  if (mma_verification_thread.joinable()) {
    mma_verification_thread.join();
    mma_thread_state.store(ThreadState::NotStarted, std::memory_order_release);
  }
}

MmaVerificationBuffer *MmaVerifier::allocate_buffer(const DifftestAmuCtrlEvent *amu_event) {
  size_t m = amu_event->mtilem;
  size_t n = amu_event->mtilen;
  size_t k = amu_event->mtilek;
  size_t element_sz_s1 = get_element_size(amu_event->types1);
  size_t element_sz_s2 = get_element_size(amu_event->types2);
  size_t element_sz_d = get_element_size(amu_event->typed);

  size_t a_sz = element_sz_s1 * m * k;
  size_t b_sz = element_sz_s2 * k * n;
  size_t c_sz = element_sz_d * m * n;

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
  std::unique_lock<std::mutex> lock(mma_queue_mutex);
  mma_verification_queue.push(buffer);
  mma_pending_count++;
  mma_queue_cv.notify_one();
}

bool MmaVerifier::has_pending_mma_verifications() const {
  return mma_pending_count.load(std::memory_order_acquire) > 0;
}

bool MmaVerifier::has_mma_verification_error() const {
  return mma_has_error.load(std::memory_order_acquire);
}

const MmaVerificationBuffer *MmaVerifier::get_error_buffer() const {
  return mma_error_buffer.load(std::memory_order_acquire);
}

void MmaVerifier::mma_verification_thread_func() {
  while (mma_thread_running) {
    MmaVerificationBuffer *buffer = nullptr;

    // Check if there are buffers to process
    {
      std::unique_lock<std::mutex> lock(mma_queue_mutex);

      // Wait for a buffer to be available or thread to be stopped
      mma_queue_cv.wait(lock, [this] { return !mma_thread_running || !mma_verification_queue.empty(); });

      if (!mma_thread_running) {
        break;
      }

      if (!mma_verification_queue.empty()) {
        buffer = mma_verification_queue.front();
        mma_verification_queue.pop();
      }
    }

    if (buffer) {
      bool passed = backend ? backend->verify(buffer) : true;

      mma_pending_count--;
      if (passed) {
        free_buffer(buffer);
      } else {
        mma_has_error.store(true, std::memory_order_release);
        mma_error_buffer.store(buffer, std::memory_order_release);
        mma_thread_running.store(false, std::memory_order_release);
        mma_thread_state.store(ThreadState::StoppedNotJoined, std::memory_order_release);
        break; // Only care about first error, exit verification thread; buffer retained
      }
    }
  }
  // Thread exiting: mark as StoppedNotJoined so stop() knows to join
  mma_thread_state.store(ThreadState::StoppedNotJoined, std::memory_order_release);
}

#endif // CONFIG_DIFFTEST_AMUCTRLEVENT
