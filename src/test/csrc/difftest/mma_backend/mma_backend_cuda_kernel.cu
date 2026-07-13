/***************************************************************************************
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

#include "mma_backend/mma_backend_cuda_impl.h"
#include <cuda_runtime_api.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <type_traits>

#ifdef CONFIG_DIFF_MMA_REDUCE_WIDTH_BYTES
static constexpr int kMmaReduceWidthBytes = CONFIG_DIFF_MMA_REDUCE_WIDTH_BYTES;
#else
static constexpr int kMmaReduceWidthBytes = 32;
#endif

template <int total_bits>
struct BitsStorageType {
  typedef typename std::conditional<
      (total_bits <= 8),
      uint8_t,
      typename std::conditional<(total_bits <= 16), uint16_t, uint32_t>::type>::type Type;
};

template <int total_bits>
__device__ uint32_t read_bits_device(const uint8_t *data, size_t index0, size_t index1, size_t row_bytes) {
  typedef typename BitsStorageType<total_bits>::Type StorageType;
  const uint8_t *row_ptr = data + index0 * row_bytes;
  return static_cast<uint32_t>(reinterpret_cast<const StorageType *>(row_ptr)[index1]);
}

template <int total_bits>
__device__ void write_bits_device(uint8_t *data, size_t index0, size_t index1, size_t row_bytes, uint32_t value) {
  typedef typename BitsStorageType<total_bits>::Type StorageType;
  uint8_t *row_ptr = data + index0 * row_bytes;
  reinterpret_cast<StorageType *>(row_ptr)[index1] = static_cast<StorageType>(value);
}

template <int exp_bits, int mantissa_bits>
__device__ double parse_custom_float_device(uint32_t bits) {
  constexpr int total_bits = 1 + exp_bits + mantissa_bits;
  constexpr uint64_t exp_mask = (1ULL << exp_bits) - 1;
  constexpr uint64_t mantissa_mask = (1ULL << mantissa_bits) - 1;
  constexpr int bias = (1 << (exp_bits - 1)) - 1;
  constexpr uint64_t exp_max = (1ULL << exp_bits) - 1;

  uint64_t sign = (bits >> (total_bits - 1)) & 1;
  uint64_t exp = (bits >> mantissa_bits) & exp_mask;
  uint64_t mantissa = bits & mantissa_mask;
  uint64_t ull_value;

  if (exp == exp_max) {
    if (mantissa == 0) {
      ull_value = (sign << 63) | (0x7ffULL << 52);
    } else {
      ull_value = (sign << 63) | (0x7ffULL << 52) | 1;
    }
  } else if (exp == 0) {
    ull_value = (sign << 63) | (mantissa << (52 - mantissa_bits));
  } else {
    ull_value = (sign << 63) |
                (((int64_t)exp - bias + (1 << (11 - 1)) - 1) << 52) |
                (mantissa << (52 - mantissa_bits));
  }

  double value;
  memcpy(&value, &ull_value, sizeof(value));
  return value;
}

template <int exp_bits, int mantissa_bits>
__device__ uint32_t encode_custom_float_device(double value) {
  uint64_t ull_value;
  memcpy(&ull_value, &value, sizeof(ull_value));

  constexpr int total_bits = 1 + exp_bits + mantissa_bits;
  constexpr int bias = (1 << (exp_bits - 1)) - 1;
  constexpr uint64_t exp_max = (1ULL << exp_bits) - 1;

  uint64_t sign = (ull_value >> 63) & 1;
  uint64_t exp = (ull_value >> 52) & ((1ULL << 11) - 1);
  uint64_t mantissa = ull_value & ((1ULL << 52) - 1);

  if (isnan(value)) {
    return (sign << (total_bits - 1)) | (exp_max << mantissa_bits) | 1;
  } else if (isinf(value)) {
    return (sign << (total_bits - 1)) | (exp_max << mantissa_bits);
  } else if (exp == 0) {
    return (sign << (total_bits - 1)) | (mantissa >> (52 - mantissa_bits));
  } else {
    int64_t new_exp = (int64_t)exp - ((1 << (11 - 1)) - 1) + bias;
    if (new_exp > (int64_t)exp_max) {
      return (sign << (total_bits - 1)) | (exp_max << mantissa_bits);
    } else if (new_exp <= 0) {
      return (sign << (total_bits - 1)) | (((1ULL << 52) | mantissa) >> (52 - mantissa_bits - new_exp + 1));
    } else {
      return (sign << (total_bits - 1)) | (new_exp << mantissa_bits) | (mantissa >> (52 - mantissa_bits));
    }
  }
}

template <class src1_t, class src2_t>
struct MmaAccumTypesDevice {
  static const bool both_unsigned = !std::is_signed<src1_t>::value && !std::is_signed<src2_t>::value;
  typedef typename std::conditional<both_unsigned, uint32_t, int32_t>::type ResultType;
  typedef typename std::conditional<both_unsigned, uint64_t, int64_t>::type ComputeType;
};

template <typename compute_t>
__device__ compute_t saturate_unsigned_device(compute_t value) {
  return value > UINT32_MAX ? UINT32_MAX : value;
}

template <typename compute_t>
__device__ compute_t saturate_signed_device(compute_t value) {
  value = value > INT32_MAX ? INT32_MAX : value;
  return value < INT32_MIN ? INT32_MIN : value;
}

template <class src1_t, class src2_t>
__global__ void mmacc_kernel(
    const uint8_t *src1,
    const uint8_t *src2,
    uint8_t *src3,
    int tile_m,
    int tile_k,
    int tile_n,
    bool sat) {
  typedef MmaAccumTypesDevice<src1_t, src2_t> AccumTypes;
  typedef typename AccumTypes::ResultType result_type;
  typedef typename AccumTypes::ComputeType compute_type;

  int idx = blockIdx.x * blockDim.x + threadIdx.x;
  int total = tile_m * tile_n;
  if (idx >= total) {
    return;
  }

  int i = idx / tile_n;
  int j = idx % tile_n;
  compute_type src_3 = reinterpret_cast<const result_type *>(src3)[idx];
  constexpr int reduce_chunk_elems = kMmaReduceWidthBytes / sizeof(src1_t);
  for (int k_base = 0; k_base < tile_k; k_base += reduce_chunk_elems) {
    int k_end = k_base + reduce_chunk_elems;
    if (k_end > tile_k) {
      k_end = tile_k;
    }

    for (int k = k_base; k < k_end; k++) {
      compute_type src_1 = reinterpret_cast<const src1_t *>(src1)[i * tile_k + k];
      compute_type src_2 = reinterpret_cast<const src2_t *>(src2)[j * tile_k + k];
      src_3 += src_1 * src_2;

      if (sat) {
        if (AccumTypes::both_unsigned) {
          src_3 = saturate_unsigned_device(src_3);
        } else {
          src_3 = saturate_signed_device(src_3);
        }
      }
    }

    src_3 = static_cast<result_type>(src_3);
  }
  reinterpret_cast<result_type *>(src3)[idx] = static_cast<result_type>(src_3);
}

// Keep this path on CUDA cores instead of tensor cores: the verifier must
// preserve the architectural k-order and CUTE reduce-vector writeback points.
template <int src_exp_bits, int src_mantissa_bits, int result_exp_bits, int result_mantissa_bits>
__global__ void mfmacc_kernel(
    const uint8_t *src1,
    const uint8_t *src2,
    uint8_t *src3,
    int tile_m,
    int tile_k,
    int tile_n) {
  constexpr int src_total_bits = 1 + src_exp_bits + src_mantissa_bits;
  constexpr int result_total_bits = 1 + result_exp_bits + result_mantissa_bits;
  constexpr size_t src_elem_bytes = src_total_bits / 8;
  constexpr size_t result_elem_bytes = result_total_bits / 8;

  int idx = blockIdx.x * blockDim.x + threadIdx.x;
  int total = tile_m * tile_n;
  if (idx >= total) {
    return;
  }

  int i = idx / tile_n;
  int j = idx % tile_n;
  size_t src_row_bytes = static_cast<size_t>(tile_k) * src_elem_bytes;
  size_t result_row_bytes = static_cast<size_t>(tile_n) * result_elem_bytes;
  uint32_t acc_bits = read_bits_device<result_total_bits>(src3, i, j, result_row_bytes);

  constexpr int reduce_chunk_elems = kMmaReduceWidthBytes / src_elem_bytes;
  for (int k_base = 0; k_base < tile_k; k_base += reduce_chunk_elems) {
    double accumulator = parse_custom_float_device<result_exp_bits, result_mantissa_bits>(acc_bits);
    int k_end = k_base + reduce_chunk_elems;
    if (k_end > tile_k) {
      k_end = tile_k;
    }

    for (int k = k_base; k < k_end; k++) {
      uint32_t src1_bits = read_bits_device<src_total_bits>(src1, i, k, src_row_bytes);
      uint32_t src2_bits = read_bits_device<src_total_bits>(src2, j, k, src_row_bytes);
      double src1_value = parse_custom_float_device<src_exp_bits, src_mantissa_bits>(src1_bits);
      double src2_value = parse_custom_float_device<src_exp_bits, src_mantissa_bits>(src2_bits);
      double product = src1_value * src2_value;
      accumulator = accumulator + product;
    }

    acc_bits = encode_custom_float_device<result_exp_bits, result_mantissa_bits>(accumulator);
  }

  write_bits_device<result_total_bits>(src3, i, j, result_row_bytes, acc_bits);
}

static size_t element_size(uint8_t typed) {
  switch (typed & 3) {
    case 0:
      return 1;
    case 1:
      return 2;
    case 2:
      return 4;
    default:
      return 4;
  }
}

static bool report_error(const char *what, cudaError_t err) {
  if (err == cudaSuccess) {
    return true;
  }
  fprintf(stderr, "CudaMmaBackend: %s failed: %s\n", what, cudaGetErrorString(err));
  return false;
}

static bool launch_kernel(
    CudaMmaType type,
    uint16_t tile_m,
    uint16_t tile_k,
    uint16_t tile_n,
    uint8_t sat_value,
    uint8_t *src1,
    uint8_t *src2,
    uint8_t *src3) {
  int total = tile_m * tile_n;
  int block_size = 256;
  int grid_size = (total + block_size - 1) / block_size;
  bool sat = sat_value != 0;

  switch (type) {
    case CudaMmaType::U8U8:
      mmacc_kernel<uint8_t, uint8_t><<<grid_size, block_size>>>(src1, src2, src3, tile_m, tile_k, tile_n, sat);
      break;
    case CudaMmaType::U8S8:
      mmacc_kernel<uint8_t, int8_t><<<grid_size, block_size>>>(src1, src2, src3, tile_m, tile_k, tile_n, sat);
      break;
    case CudaMmaType::S8U8:
      mmacc_kernel<int8_t, uint8_t><<<grid_size, block_size>>>(src1, src2, src3, tile_m, tile_k, tile_n, sat);
      break;
    case CudaMmaType::S8S8:
      mmacc_kernel<int8_t, int8_t><<<grid_size, block_size>>>(src1, src2, src3, tile_m, tile_k, tile_n, sat);
      break;
    case CudaMmaType::Fp8E5M2ToFp16:
      mfmacc_kernel<5, 2, 5, 10><<<grid_size, block_size>>>(src1, src2, src3, tile_m, tile_k, tile_n);
      break;
    case CudaMmaType::Fp8E5M2ToFp32:
      mfmacc_kernel<5, 2, 8, 23><<<grid_size, block_size>>>(src1, src2, src3, tile_m, tile_k, tile_n);
      break;
    case CudaMmaType::Fp8E5M2ToBf16:
      mfmacc_kernel<5, 2, 8, 7><<<grid_size, block_size>>>(src1, src2, src3, tile_m, tile_k, tile_n);
      break;
    case CudaMmaType::Fp8E4M3ToFp16:
      mfmacc_kernel<4, 3, 5, 10><<<grid_size, block_size>>>(src1, src2, src3, tile_m, tile_k, tile_n);
      break;
    case CudaMmaType::Fp8E4M3ToFp32:
      mfmacc_kernel<4, 3, 8, 23><<<grid_size, block_size>>>(src1, src2, src3, tile_m, tile_k, tile_n);
      break;
    case CudaMmaType::Fp8E4M3ToBf16:
      mfmacc_kernel<4, 3, 8, 7><<<grid_size, block_size>>>(src1, src2, src3, tile_m, tile_k, tile_n);
      break;
    case CudaMmaType::Fp16ToFp16:
      mfmacc_kernel<5, 10, 5, 10><<<grid_size, block_size>>>(src1, src2, src3, tile_m, tile_k, tile_n);
      break;
    case CudaMmaType::Fp16ToFp32:
      mfmacc_kernel<5, 10, 8, 23><<<grid_size, block_size>>>(src1, src2, src3, tile_m, tile_k, tile_n);
      break;
    case CudaMmaType::Bf16ToFp32:
      mfmacc_kernel<8, 7, 8, 23><<<grid_size, block_size>>>(src1, src2, src3, tile_m, tile_k, tile_n);
      break;
    default:
      return false;
  }

  return report_error("kernel launch", cudaGetLastError());
}

extern "C" bool cuda_mma_backend_launch(
    CudaMmaType type,
    uint16_t tile_m,
    uint16_t tile_k,
    uint16_t tile_n,
    uint8_t types1,
    uint8_t types2,
    uint8_t typed,
    uint8_t sat,
    const uint8_t *src1,
    const uint8_t *src2,
    uint8_t *src3,
    const uint8_t *dut_result) {
  size_t src1_size = element_size(types1) * tile_m * tile_k;
  size_t src2_size = element_size(types2) * tile_k * tile_n;
  size_t result_size = element_size(typed) * tile_m * tile_n;

  uint8_t *dev_src1 = nullptr;
  uint8_t *dev_src2 = nullptr;
  uint8_t *dev_src3 = nullptr;
  bool passed = false;

  if (!report_error("cudaMalloc src1", cudaMalloc(&dev_src1, src1_size)) ||
      !report_error("cudaMalloc src2", cudaMalloc(&dev_src2, src2_size)) ||
      !report_error("cudaMalloc src3", cudaMalloc(&dev_src3, result_size))) {
    goto cleanup;
  }
  if (!report_error("copy src1 to device", cudaMemcpy(dev_src1, src1, src1_size, cudaMemcpyHostToDevice)) ||
      !report_error("copy src2 to device", cudaMemcpy(dev_src2, src2, src2_size, cudaMemcpyHostToDevice)) ||
      !report_error("copy src3 to device", cudaMemcpy(dev_src3, src3, result_size, cudaMemcpyHostToDevice))) {
    goto cleanup;
  }
  if (!launch_kernel(type, tile_m, tile_k, tile_n, sat, dev_src1, dev_src2, dev_src3)) {
    goto cleanup;
  }
  if (!report_error("kernel sync", cudaDeviceSynchronize())) {
    goto cleanup;
  }
  if (!report_error("copy src3 to host", cudaMemcpy(src3, dev_src3, result_size, cudaMemcpyDeviceToHost))) {
    goto cleanup;
  }

  passed = memcmp(dut_result, src3, result_size) == 0;

cleanup:
  if (dev_src1 != nullptr) {
    cudaFree(dev_src1);
  }
  if (dev_src2 != nullptr) {
    cudaFree(dev_src2);
  }
  if (dev_src3 != nullptr) {
    cudaFree(dev_src3);
  }
  return passed;
}
