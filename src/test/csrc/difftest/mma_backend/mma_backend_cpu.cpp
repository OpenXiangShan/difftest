/***************************************************************************************
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

#include "mma_backend/mma_backend_cpu.h"

#ifdef CONFIG_DIFFTEST_AMUCTRLEVENT

#include "mma_verifier.h"
#include <climits>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <type_traits>

#ifdef CONFIG_DIFF_MMA_REDUCE_WIDTH_BYTES
static constexpr int kMmaReduceWidthBytes = CONFIG_DIFF_MMA_REDUCE_WIDTH_BYTES;
#else
static constexpr int kMmaReduceWidthBytes = 32;
#endif

template <int total_bits> struct BitsStorageType {
  typedef typename std::conditional<(total_bits <= 8), uint8_t,
                                    typename std::conditional<(total_bits <= 16), uint16_t, uint32_t>::type>::type Type;
};

template <int total_bits>
static uint32_t read_bits(const uint8_t *data, size_t index0, size_t index1, size_t row_bytes) {
  typedef typename BitsStorageType<total_bits>::Type StorageType;
  const uint8_t *row_ptr = data + index0 * row_bytes;
  return static_cast<uint32_t>(reinterpret_cast<const StorageType *>(row_ptr)[index1]);
}

template <int total_bits>
static void write_bits(uint8_t *data, size_t index0, size_t index1, size_t row_bytes, uint32_t value) {
  typedef typename BitsStorageType<total_bits>::Type StorageType;
  uint8_t *row_ptr = data + index0 * row_bytes;
  reinterpret_cast<StorageType *>(row_ptr)[index1] = static_cast<StorageType>(value);
}

template <class src1_t, class src2_t> struct MmaAccumTypes {
  static const bool both_unsigned = !std::is_signed<src1_t>::value && !std::is_signed<src2_t>::value;
  typedef typename std::conditional<both_unsigned, uint32_t, int32_t>::type ResultType;
  typedef typename std::conditional<both_unsigned, uint64_t, int64_t>::type ComputeType;
};

template <typename compute_t> static compute_t saturate_accum(compute_t value, std::true_type) {
  return value > UINT32_MAX ? UINT32_MAX : value;
}

template <typename compute_t> static compute_t saturate_accum(compute_t value, std::false_type) {
  value = value > INT32_MAX ? INT32_MAX : value;
  return value < INT32_MIN ? INT32_MIN : value;
}

template <int exp_bits, int mantissa_bits> static double parse_custom_float(uint32_t bits) {
  constexpr int total_bits = 1 + exp_bits + mantissa_bits;
  constexpr uint64_t exp_mask = (1ULL << exp_bits) - 1;
  constexpr uint64_t mantissa_mask = (1ULL << mantissa_bits) - 1;
  constexpr int bias = (1 << (exp_bits - 1)) - 1;
  constexpr uint64_t exp_max = (1ULL << exp_bits) - 1;

  uint64_t sign = (bits >> (total_bits - 1)) & 1;
  uint64_t exp = (bits >> mantissa_bits) & exp_mask;
  uint64_t mantissa = bits & mantissa_mask;

  if (exp == exp_max) {
    if (mantissa == 0) {
      return sign ? -INFINITY : INFINITY;
    } else {
      return NAN;
    }
  } else if (exp == 0) {
    uint64_t ull_value = (sign << 63) | (mantissa << (52 - mantissa_bits));
    return reinterpret_cast<double *>(&ull_value)[0];
  } else {
    uint64_t ull_value =
        ((sign << 63) | (((int64_t)exp - bias + (1 << (11 - 1)) - 1) << 52) | (mantissa << (52 - mantissa_bits)));
    return reinterpret_cast<double *>(&ull_value)[0];
  }
}

template <int exp_bits, int mantissa_bits> static uint32_t encode_custom_float(double value) {
  uint64_t ull_value = reinterpret_cast<uint64_t *>(&value)[0];

  constexpr int total_bits = 1 + exp_bits + mantissa_bits;
  constexpr int bias = (1 << (exp_bits - 1)) - 1;
  constexpr uint64_t exp_max = (1ULL << exp_bits) - 1;

  uint64_t sign = (ull_value >> 63) & 1;
  uint64_t exp = (ull_value >> 52) & ((1ULL << 11) - 1);
  uint64_t mantissa = ull_value & ((1ULL << 52) - 1);

  if (std::isnan(value)) {
    return (sign << (total_bits - 1)) | (exp_max << mantissa_bits) | 1;
  } else if (std::isinf(value)) {
    return (sign << (total_bits - 1)) | (exp_max << mantissa_bits);
  } else if (exp == 0) {
    return (sign << (total_bits - 1)) | (mantissa >> (52 - mantissa_bits));
  } else {
    int64_t new_exp = (int64_t)exp - ((1 << (11 - 1)) - 1) + bias;
    if (new_exp > (int64_t)exp_max) {
      return (sign << (total_bits - 1)) | (exp_max << mantissa_bits);
    } else if (new_exp <= 0) {
      return ((sign << (total_bits - 1)) | (((1ULL << 52) | mantissa) >> (52 - mantissa_bits - new_exp + 1)));
    } else {
      return ((sign << (total_bits - 1)) | (new_exp << mantissa_bits) | (mantissa >> (52 - mantissa_bits)));
    }
  }
}

bool CpuMmaBackend::verify(MmaVerificationBuffer *buffer) {
  bool passed = true;
  uint8_t isfp = buffer->amu_event.isfp;

  if (isfp) {
    uint8_t types = buffer->amu_event.types1;
    uint8_t typed = buffer->amu_event.typed;
    switch (types) {
      case 0:
        switch (typed) {
          case 1: passed = mfmacc_template<5, 2, 5, 10>(buffer); break;
          case 2: passed = mfmacc_template<5, 2, 8, 23>(buffer); break;
          case 5: passed = mfmacc_template<5, 2, 8, 7>(buffer); break;
          default: break;
        }
        break;
      case 4:
        switch (typed) {
          case 1: passed = mfmacc_template<4, 3, 5, 10>(buffer); break;
          case 2: passed = mfmacc_template<4, 3, 8, 23>(buffer); break;
          case 5: passed = mfmacc_template<4, 3, 8, 7>(buffer); break;
          default: break;
        }
        break;
      case 1:
        switch (typed) {
          case 1: passed = mfmacc_template<5, 10, 5, 10>(buffer); break;
          case 2: passed = mfmacc_template<5, 10, 8, 23>(buffer); break;
          default: break;
        }
        break;
      case 5:
        switch (typed) {
          case 2: passed = mfmacc_template<8, 7, 8, 23>(buffer); break;
          default: break;
        }
        break;
      default: break;
    }
  } else {
    uint8_t types1 = buffer->amu_event.types1;
    uint8_t types2 = buffer->amu_event.types2;
    int op = ((types1 & 0x4) >> 1) | ((types2 & 0x4) >> 2);
    switch (op) {
      case 0: passed = mmacc_template<uint8_t, uint8_t>(buffer); break;
      case 1: passed = mmacc_template<uint8_t, int8_t>(buffer); break;
      case 2: passed = mmacc_template<int8_t, uint8_t>(buffer); break;
      case 3: passed = mmacc_template<int8_t, int8_t>(buffer); break;
      default: break;
    }
  }

  return passed;
}

template <int src_exp_bits, int src_mantissa_bits, int result_exp_bits, int result_mantissa_bits>
bool CpuMmaBackend::mfmacc_template(MmaVerificationBuffer *buffer) {
  int tile_m = buffer->amu_event.mtilem;
  int tile_k = buffer->amu_event.mtilek;
  int tile_n = buffer->amu_event.mtilen;

  constexpr int src_total_bits = 1 + src_exp_bits + src_mantissa_bits;
  constexpr int result_total_bits = 1 + result_exp_bits + result_mantissa_bits;
  constexpr size_t src_elem_bytes = src_total_bits / 8;
  constexpr size_t result_elem_bytes = result_total_bits / 8;
  constexpr int reduce_chunk_elems = kMmaReduceWidthBytes / src_elem_bytes;
  const size_t src_row_bytes = static_cast<size_t>(tile_k) * src_elem_bytes;
  const size_t result_row_bytes = static_cast<size_t>(tile_n) * result_elem_bytes;

  for (int i = 0; i < tile_m; i++) {
    for (int j = 0; j < tile_n; j++) {
      uint32_t acc_bits = read_bits<result_total_bits>(buffer->src3, i, j, result_row_bytes);

      for (int k_base = 0; k_base < tile_k; k_base += reduce_chunk_elems) {
        double accumulator = parse_custom_float<result_exp_bits, result_mantissa_bits>(acc_bits);
        int k_end = k_base + reduce_chunk_elems;
        if (k_end > tile_k) {
          k_end = tile_k;
        }

        for (int k = k_base; k < k_end; k++) {
          uint32_t src1_bits = read_bits<src_total_bits>(buffer->src1, i, k, src_row_bytes);
          uint32_t src2_bits = read_bits<src_total_bits>(buffer->src2, j, k, src_row_bytes);
          double src1 = parse_custom_float<src_exp_bits, src_mantissa_bits>(src1_bits);
          double src2 = parse_custom_float<src_exp_bits, src_mantissa_bits>(src2_bits);

          double product = src1 * src2;
          accumulator = accumulator + product;
        }

        acc_bits = encode_custom_float<result_exp_bits, result_mantissa_bits>(accumulator);
      }

      write_bits<result_total_bits>(buffer->src3, i, j, result_row_bytes, acc_bits);
    }
  }

  size_t result_size = (result_total_bits <= 8)    ? sizeof(uint8_t)
                       : (result_total_bits <= 16) ? sizeof(uint16_t)
                                                   : sizeof(uint32_t);
  return memcmp(buffer->dut_result, buffer->src3, tile_m * tile_n * result_size) == 0;
}

template <class src1_t, class src2_t> bool CpuMmaBackend::mmacc_template(MmaVerificationBuffer *buffer) {
  int tile_m = buffer->amu_event.mtilem;
  int tile_k = buffer->amu_event.mtilek;
  int tile_n = buffer->amu_event.mtilen;

  typedef MmaAccumTypes<src1_t, src2_t> AccumTypes;
  typedef typename AccumTypes::ResultType result_type;
  typedef typename AccumTypes::ComputeType compute_type;
  constexpr int reduce_chunk_elems = kMmaReduceWidthBytes / sizeof(src1_t);

  for (int i = 0; i < tile_m; i++) {
    for (int j = 0; j < tile_n; j++) {
      compute_type src_3 = ((result_type *)(buffer->src3))[(i * tile_n + j)];
      for (int k_base = 0; k_base < tile_k; k_base += reduce_chunk_elems) {
        int k_end = k_base + reduce_chunk_elems;
        if (k_end > tile_k) {
          k_end = tile_k;
        }

        for (int k = k_base; k < k_end; k++) {
          compute_type src_1 = ((src1_t *)(buffer->src1))[(i * tile_k + k)];
          compute_type src_2 = ((src2_t *)(buffer->src2))[(j * tile_k + k)];
          src_3 += src_1 * src_2;

          if (buffer->amu_event.sat) {
            src_3 = saturate_accum(src_3, std::integral_constant<bool, AccumTypes::both_unsigned>());
          }
        }

        src_3 = static_cast<result_type>(src_3);
      }
      ((result_type *)(buffer->src3))[(i * tile_n + j)] = static_cast<result_type>(src_3);
    }
  }

  return memcmp(buffer->dut_result, buffer->src3, tile_m * tile_n * sizeof(result_type)) == 0;
}

#endif // CONFIG_DIFFTEST_AMUCTRLEVENT
