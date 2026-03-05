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

#ifdef CONFIG_DIFFTEST_AMUCTRLEVENT
#include <cstring>
#include <type_traits>
#include <climits>
#include <cstdint>
#include <cmath>
#include <cfloat>

MmaVerifier::MmaVerifier() : mma_thread_running(false) {
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
  MmaVerificationBuffer* err_buf = mma_error_buffer.exchange(nullptr);
  if (err_buf) {
    free_buffer(err_buf);
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

const MmaVerificationBuffer* MmaVerifier::get_error_buffer() const {
  return mma_error_buffer.load(std::memory_order_acquire);
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
      bool passed = true;
      uint8_t isfp = buffer->amu_event.isfp;
      
      if (isfp) {
        uint8_t types = buffer->amu_event.types1;
        uint8_t typed = buffer->amu_event.typed;
        switch (types) {
          case 0: // E5M2
            switch (typed) {
              case 1: // FP16
                passed = mfmacc_template<5, 2, 5, 10>(buffer);
                break;
              case 2: // FP32
                passed = mfmacc_template<5, 2, 8, 23>(buffer);
                break;
              case 5: // BF16
                passed = mfmacc_template<5, 2, 8, 7>(buffer);
                break;
              default:
                break;
            }
            break;
          case 4: // E4M3
            switch (typed) {
              case 1: // FP16
                passed = mfmacc_template<4, 3, 5, 10>(buffer);
                break;
              case 2: // FP32
                passed = mfmacc_template<4, 3, 8, 23>(buffer);
                break;
              case 5: // BF16
                passed = mfmacc_template<4, 3, 8, 7>(buffer);
                break;
              default:
                break;
            }
            break;
          case 1: // FP16
            switch (typed) {
              case 1: // FP16
                passed = mfmacc_template<5, 10, 5, 10>(buffer);
                break;
              case 2: // FP32
                passed = mfmacc_template<5, 10, 8, 23>(buffer);
                break;
              default:
                break;
            }
            break;
          case 5: // BF16
            switch (typed) {
              case 2: // FP32
                passed = mfmacc_template<8, 7, 8, 23>(buffer);
                break;
              default:
                break;
            }
            break;
          default:
            break;
        }
      } else { // !isfp
        uint8_t types1 = buffer->amu_event.types1;
        uint8_t types2 = buffer->amu_event.types2;
        int op = ((types1 & 0x4) << 1) | (types2 & 0x4);
        switch (op) {
          case 0:
            passed = mmacc_template<uint8_t, uint8_t>(buffer);
            break;
          case 1:
            passed = mmacc_template<uint8_t, int8_t>(buffer);
            break;
          case 2:
            passed = mmacc_template<int8_t, uint8_t>(buffer);
            break;
          case 3:
            passed = mmacc_template<int8_t, int8_t>(buffer);
            break;
          default:
            break;
        }
      }

      mma_pending_count--;
      if (passed) {
        free_buffer(buffer);
      } else {
        mma_has_error.store(true, std::memory_order_release);
        mma_error_buffer.store(buffer, std::memory_order_release);
        mma_thread_running.store(false, std::memory_order_release);
        mma_thread_state.store(ThreadState::StoppedNotJoined, std::memory_order_release);
        break;  // Only care about first error, exit verification thread; buffer retained
      }
    }
  }
  // Thread exiting: mark as StoppedNotJoined so stop() knows to join
  mma_thread_state.store(ThreadState::StoppedNotJoined, std::memory_order_release);
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

// Helper function to read bits from byte array
template<int total_bits, int rlenb>
static uint32_t read_bits(const uint8_t* data, size_t index0, size_t index1) {
  if constexpr (total_bits <= 8) {
    return data[index0 * rlenb + index1];
  } else if constexpr (total_bits <= 16) {
    return reinterpret_cast<const uint16_t*>(data)[index0 * rlenb / 2 + index1];
  } else {
    return reinterpret_cast<const uint32_t*>(data)[index0 * rlenb / 4 + index1];
  }
}

// Helper function to write bits to byte array
template<int total_bits, int rlenb>
static void write_bits(uint8_t* data, size_t index0, size_t index1, uint32_t value) {
  if constexpr (total_bits <= 8) {
    data[index0 * rlenb + index1] = static_cast<uint8_t>(value);
  } else if constexpr (total_bits <= 16) {
    reinterpret_cast<uint16_t*>(data)[index0 * rlenb / 2 + index1] = static_cast<uint16_t>(value);
  } else {
    reinterpret_cast<uint32_t*>(data)[index0 * rlenb / 4 + index1] = value;
  }
}

// Helper function to parse custom floating point format to double
template<int exp_bits, int mantissa_bits>
static double parse_custom_float(uint32_t bits) {
  constexpr int total_bits = 1 + exp_bits + mantissa_bits;
  constexpr uint64_t exp_mask = (1ULL << exp_bits) - 1;
  constexpr uint64_t mantissa_mask = (1ULL << mantissa_bits) - 1;
  constexpr int bias = (1 << (exp_bits - 1)) - 1;
  constexpr uint64_t exp_max = (1ULL << exp_bits) - 1;
  
  // Extract fields
  uint64_t sign = (bits >> (total_bits - 1)) & 1;
  uint64_t exp = (bits >> mantissa_bits) & exp_mask;
  uint64_t mantissa = bits & mantissa_mask;
  
  if (exp == exp_max) {
    // Handle special cases: Infinity or NaN
    if (mantissa == 0) {
      return sign ? -INFINITY : INFINITY;
    } else {
      return NAN;
    }
  } else if (exp == 0) {
    // Zero or subnormal
    uint64_t ull_value = (sign << 63) | (mantissa << (52 - mantissa_bits));
    return reinterpret_cast<double*>(&ull_value)[0];
  } else {
    // Normal number: value = (-1)^sign * 2^(exp-bias) * (1 + mantissa / 2^mantissa_bits)
    uint64_t ull_value = (
      (sign << 63)
      | (((int64_t)exp - bias + (1 << (11 - 1)) - 1) << 52)
      | (mantissa << (52 - mantissa_bits))
    );
    return reinterpret_cast<double*>(&ull_value)[0];
  }
}

// Helper function to convert double to custom floating point format
template<int exp_bits, int mantissa_bits>
static uint32_t encode_custom_float(double value) {
  uint64_t ull_value = reinterpret_cast<uint64_t*>(&value)[0];

  constexpr int total_bits = 1 + exp_bits + mantissa_bits;
  constexpr int bias = (1 << (exp_bits - 1)) - 1;
  constexpr uint64_t exp_max = (1ULL << exp_bits) - 1;
  // constexpr uint32_t mantissa_mask = (1U << mantissa_bits) - 1;
  
  uint64_t sign = (ull_value >> 63) & 1;
  uint64_t exp = (ull_value >> 52) & ((1ULL << 11) - 1);
  uint64_t mantissa = ull_value & ((1ULL << 52) - 1);
  double abs_value = std::abs(value);

  // Handle special cases
  if (std::isnan(value)) {
    // Return canonical NaN: sign=0, exp=max, mantissa=1
    return (sign << (total_bits - 1)) | (exp_max << mantissa_bits) | 1;
  } else if (std::isinf(value)) {
    // Return infinity: sign=0, exp=max, mantissa=0
    return (sign << (total_bits - 1)) | (exp_max << mantissa_bits) | 0;
  } else if (exp == 0) {
    // Zero or subnormal
    return (sign << (total_bits - 1)) | (mantissa >> (52 - mantissa_bits));
  } else {
    // Normal float number
    int64_t new_exp = (int64_t)exp - ((1 << (11 - 1)) - 1) + bias;
    if (new_exp > exp_max) {
      // To infinity
      return (sign << (total_bits - 1)) | (exp_max << mantissa_bits) | 0;
    } else if (new_exp <= 0) {
      // To subnormal
      return (
        (sign << (total_bits - 1))
        | (((1ULL << 52) | mantissa) >> (52 - mantissa_bits - new_exp + 1))
      );
    } else {
      return (
        (sign << (total_bits - 1))
        | (new_exp << mantissa_bits)
        | (mantissa >> (52 - mantissa_bits))
      );
    }
  }
}

template<int src_exp_bits, int src_mantissa_bits, int result_exp_bits, int result_mantissa_bits>
bool MmaVerifier::mfmacc_template(MmaVerificationBuffer *buffer) {
  int tile_m = buffer->amu_event.mtilem;
  int tile_k = buffer->amu_event.mtilek;
  int tile_n = buffer->amu_event.mtilen;
  
  constexpr int src_total_bits = 1 + src_exp_bits + src_mantissa_bits;
  constexpr int result_total_bits = 1 + result_exp_bits + result_mantissa_bits;
  
  for (int i = 0; i < tile_m; i++) {
    for (int j = 0; j < tile_n; j++) {
      // Read initial accumulator value
      // TODO: Do not hardcode 128 here
      uint32_t acc_bits = read_bits<result_total_bits, 512>(buffer->src3, i, j);
      double accumulator = parse_custom_float<result_exp_bits, result_mantissa_bits>(acc_bits);
      
      for (int k = 0; k < tile_k; k++) {
        // Parse source operands
        // TODO: Do not hardcode 64 here
        uint32_t src1_bits = read_bits<src_total_bits, 64>(buffer->src1, i, k);
        uint32_t src2_bits = read_bits<src_total_bits, 64>(buffer->src2, j, k);
        double src1 = parse_custom_float<src_exp_bits, src_mantissa_bits>(src1_bits);
        double src2 = parse_custom_float<src_exp_bits, src_mantissa_bits>(src2_bits);

        // Perform multiplication and accumulation
        double product = src1 * src2;
        accumulator = accumulator + product;
      }
      // Encode result back to target format
      uint32_t result_bits = encode_custom_float<result_exp_bits, result_mantissa_bits>(accumulator);
      write_bits<result_total_bits, 512>(buffer->src3, i, j, result_bits);
    }
  }
  
  // Calculate size for memcmp based on result bit width
  size_t result_size = (result_total_bits <= 8) ? sizeof(uint8_t) : 
                       (result_total_bits <= 16) ? sizeof(uint16_t) : sizeof(uint32_t);
  return memcmp(buffer->dut_result, buffer->src3, tile_m * tile_n * result_size) == 0;
}

// Integer matrix multiplication template
template<class src1_t, class src2_t>
bool MmaVerifier::mmacc_template(MmaVerificationBuffer *buffer) {
  int tile_m = buffer->amu_event.mtilem;
  int tile_k = buffer->amu_event.mtilek;
  int tile_n = buffer->amu_event.mtilen;
  
  // Determine if both source types are unsigned
  constexpr bool both_unsigned = !std::is_signed_v<src1_t> && !std::is_signed_v<src2_t>;
  
  // Result type: uint32_t if both are unsigned, int32_t otherwise
  using result_type = std::conditional_t<both_unsigned, uint32_t, int32_t>;
  // Compute type: uint64_t if both are unsigned, int64_t otherwise
  using compute_type = std::conditional_t<both_unsigned, uint64_t, int64_t>;
  
  for (int i = 0; i < tile_m; i++) {
    for (int j = 0; j < tile_n; j++) {
      compute_type src_3 = ((result_type *)(buffer->src3))[(i * tile_n + j)];
      for (int k = 0; k < tile_k; k++) {
        compute_type src_1 = ((src1_t *)(buffer->src1))[(i * tile_k + k)];
        compute_type src_2 = ((src2_t *)(buffer->src2))[(j * tile_k + k)];
        src_3 += src_1 * src_2;
        
        // Saturation handling
        if (buffer->amu_event.sat) {
          if constexpr (both_unsigned) {
            src_3 = src_3 > UINT32_MAX ? UINT32_MAX : src_3;
          } else {
            // At least one signed: clamp to INT32_MIN/INT32_MAX
            src_3 = src_3 > INT32_MAX ? INT32_MAX : src_3;
            src_3 = src_3 < INT32_MIN ? INT32_MIN : src_3;
          }
        }
      }
      ((result_type *)(buffer->src3))[(i * tile_n + j)] = static_cast<result_type>(src_3);
    }
  }
  
  return memcmp(buffer->dut_result, buffer->src3, tile_m * tile_n * sizeof(result_type)) == 0;
}

#endif // CONFIG_DIFFTEST_AMUCTRLEVENT
