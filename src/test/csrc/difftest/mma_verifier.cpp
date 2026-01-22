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
#include <cstring>
#include <iostream>

MmaVerifier::MmaVerifier(REF_PROXY *proxy) : proxy(proxy), mma_thread_running(false) {
}

MmaVerifier::~MmaVerifier() {
  stop();
  
  // Free all buffers
  for (auto buffer : mma_buffers) {
    free_buffer(buffer);
  }
  mma_buffers.clear();
  
  // Clear queue (should be empty if thread was stopped properly)
  std::unique_lock<std::mutex> lock(mma_queue_mutex);
  while (!mma_verification_queue.empty()) {
    auto buffer = mma_verification_queue.front();
    mma_verification_queue.pop();
    free_buffer(buffer);
  }
}

void MmaVerifier::start() {
  if (!mma_thread_running) {
    mma_thread_running = true;
    mma_verification_thread = std::thread(&MmaVerifier::mma_verification_thread_func, this);
  }
}

void MmaVerifier::stop() {
  if (mma_thread_running) {
    mma_thread_running = false;
    mma_queue_cv.notify_all();
    
    if (mma_verification_thread.joinable()) {
      mma_verification_thread.join();
    }
  }
}

MmaVerificationBuffer* MmaVerifier::allocate_buffer(const DifftestAmuCtrlEvent *amu_event) {
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
  
  buffer->in_use = true;
  
  // Add to buffer list
  mma_buffers.push_back(buffer);
  
  return buffer;
}

void MmaVerifier::free_buffer(MmaVerificationBuffer *buffer) {
  if (buffer) {
    // Free the data buffers
    if (buffer->src1) {
      delete[] buffer->src1;
      delete[] buffer->src2;
      delete[] buffer->src3;
      delete[] buffer->dut_result;
    }
    
    // Remove from buffer list
    for (auto it = mma_buffers.begin(); it != mma_buffers.end(); ++it) {
      if (*it == buffer) {
        mma_buffers.erase(it);
        break;
      }
    }
    
    // Free the buffer structure
    delete buffer;
  }
}

void MmaVerifier::add_to_verification_queue(MmaVerificationBuffer *buffer) {
  std::unique_lock<std::mutex> lock(mma_queue_mutex);
  mma_verification_queue.push(buffer);
  mma_queue_cv.notify_one();
}

void MmaVerifier::mma_verification_thread_func() {
  while (mma_thread_running) {
    MmaVerificationBuffer *buffer = nullptr;
    
    // Check if there are buffers to process
    {
      std::unique_lock<std::mutex> lock(mma_queue_mutex);
      
      // Wait for a buffer to be available or thread to be stopped
      mma_queue_cv.wait(lock, [this] {
        return !mma_thread_running || !mma_verification_queue.empty();
      });
      
      if (!mma_thread_running) {
        break;
      }
      
      if (!mma_verification_queue.empty()) {
        buffer = mma_verification_queue.front();
        mma_verification_queue.pop();
      }
    }

    if (buffer) {
      // TODO: Perform MMA verification here (matrix multiplication and result comparison)
      uint8_t isfp = buffer->amu_event.isfp;
      uint8_t types1 = buffer->amu_event.types1;
      uint8_t types2 = buffer->amu_event.types2;
      uint8_t typed = buffer->amu_event.typed;
      
      if (isfp) {
        mfmacc(buffer);
      } else { // !isfp
        int op = ((types1 & 0x4) << 1) | (types2 & 0x4);
        switch (op) {
          case 0:
            mmaccu(buffer);
            break;
          case 1:
            mmaccus(buffer);
            break;
          case 2:
            mmaccsu(buffer);
            break;
          case 3:
            mmacc(buffer);
            break;
          default:
            break;
        }
      }

      // Free the buffer after verification
      free_buffer(buffer);
    }
  }
}

#define MMA_PROLOGUE \
  int tile_m = buffer->amu_event.mtilem; \
  int tile_k = buffer->amu_event.mtilek; \
  int tile_n = buffer->amu_event.mtilen; \
  uint8_t m_d_sz = buffer->amu_event.typed; \
  uint8_t m_s_sz = buffer->amu_event.types1; \

#define MMA_LOOP_BEGIN \
  for (int i = 0; i < tile_m; i++) { \
    for (int j = 0; j < tile_n; j++) { \
      for (int k = 0; k < tile_k; k++) { \

#define MMA_LOOP_END \
      } \
    } \
  } \

#define MMA_EPILOGUE \
  return memcpy(buffer->dut_result, buffer->src3, tile_m * tile_n * sizeof(int32_t)) == 0;

bool MmaVerifier::mmacc(MmaVerificationBuffer *buffer) {
  MMA_PROLOGUE
  MMA_LOOP_BEGIN
    int64_t src_1 = ((int8_t *)(buffer->src1))[(i * tile_k + k)];
    int64_t src_2 = ((int8_t *)(buffer->src2))[(j * tile_k + k)];
    int64_t src_3 = ((int32_t *)(buffer->src3))[(i * tile_n + j)];
    int64_t result = src_1 * src_2 + src_3;
    if (buffer->amu_event.sat) {
      result = result > INT32_MAX ? INT32_MAX : result;
      result = result < INT32_MIN ? INT32_MIN : result;
    }
    ((int32_t *)(buffer->src3))[(i * tile_n + j)] += result;
  MMA_LOOP_END
  MMA_EPILOGUE
}

bool MmaVerifier::mmaccu(MmaVerificationBuffer *buffer) {
  MMA_PROLOGUE
  MMA_LOOP_BEGIN
    uint64_t src_1 = ((uint8_t *)(buffer->src1))[(i * tile_k + k)];
    uint64_t src_2 = ((uint8_t *)(buffer->src2))[(j * tile_k + k)];
    uint64_t src_3 = ((uint32_t *)(buffer->src3))[(i * tile_n + j)];
    uint64_t result = src_1 * src_2 + src_3;
    if (buffer->amu_event.sat) {
      result = result > UINT32_MAX ? UINT32_MAX : result;
    }
    ((uint32_t *)(buffer->src3))[(i * tile_n + j)] += result;
  MMA_LOOP_END
  MMA_EPILOGUE
}

bool MmaVerifier::mmaccus(MmaVerificationBuffer *buffer) {
  MMA_PROLOGUE
  MMA_LOOP_BEGIN
    uint64_t src_1 = ((uint8_t *)(buffer->src1))[(i * tile_k + k)];
    int64_t src_2 = ((int8_t *)(buffer->src2))[(j * tile_k + k)];
    int64_t src_3 = ((int32_t *)(buffer->src3))[(i * tile_n + j)];
    int64_t result = src_1 * src_2 + src_3;
    if (buffer->amu_event.sat) {
      result = result > INT32_MAX ? INT32_MAX : result;
      result = result < INT32_MIN ? INT32_MIN : result;
    }
    ((int32_t *)(buffer->src3))[(i * tile_n + j)] += result;
  MMA_LOOP_END
  MMA_EPILOGUE
}

bool MmaVerifier::mmaccsu(MmaVerificationBuffer *buffer) {
  MMA_PROLOGUE
  MMA_LOOP_BEGIN
    int64_t src_1 = ((int8_t *)(buffer->src1))[(i * tile_k + k)];
    uint64_t src_2 = ((uint8_t *)(buffer->src2))[(j * tile_k + k)];
    int64_t src_3 = ((int32_t *)(buffer->src3))[(i * tile_n + j)];
    int64_t result = src_1 * src_2 + src_3;
    if (buffer->amu_event.sat) {
      result = result > INT32_MAX ? INT32_MAX : result;
      result = result < INT32_MIN ? INT32_MIN : result;
    }
    ((int32_t *)(buffer->src3))[(i * tile_n + j)] += result;
  MMA_LOOP_END
  MMA_EPILOGUE
}

bool MmaVerifier::mfmacc(MmaVerificationBuffer *buffer) {
  // TODO: Implement FP MMA verification
  return true;
}
