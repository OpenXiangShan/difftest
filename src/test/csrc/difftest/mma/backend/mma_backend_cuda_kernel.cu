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

#include "mma/backend/mma_backend_cuda_impl.h"
#include "mma/backend/mma_backend_cute_model.h"
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

__device__ __forceinline__ uint32_t read_u32_device(
    const uint8_t *data, size_t index0, size_t index1, size_t row_bytes) {
  const uint8_t *row_ptr = data + index0 * row_bytes;
  return reinterpret_cast<const uint32_t *>(row_ptr)[index1];
}

__device__ __forceinline__ void write_u32_device(
    uint8_t *data, size_t index0, size_t index1, size_t row_bytes, uint32_t value) {
  uint8_t *row_ptr = data + index0 * row_bytes;
  reinterpret_cast<uint32_t *>(row_ptr)[index1] = value;
}

template <class src1_t, class src2_t>
__global__ void mmacc_cute_int32_kernel(
    const uint8_t *src1, const uint8_t *src2, uint8_t *src3, int tile_m, int tile_k, int tile_n) {
  static_assert(sizeof(src1_t) == 1 && sizeof(src2_t) == 1, "CUTE integer MMA expects 8-bit source elements");
  int idx = blockIdx.x * blockDim.x + threadIdx.x;
  int total = tile_m * tile_n;
  if (idx >= total) {
    return;
  }

  int i = idx / tile_n;
  int j = idx % tile_n;
  int reduce_chunk_elems = kMmaReduceWidthBytes;
  size_t src_row_bytes = static_cast<size_t>(tile_k);
  size_t result_row_bytes = static_cast<size_t>(tile_n) * sizeof(uint32_t);
  const uint8_t *src1_row = src1 + static_cast<size_t>(i) * src_row_bytes;
  const uint8_t *src2_row = src2 + static_cast<size_t>(j) * src_row_bytes;
  uint32_t acc_bits = read_u32_device(src3, i, j, result_row_bytes);

  for (int k_base = 0; k_base < tile_k; k_base += reduce_chunk_elems) {
    int valid_lanes = tile_k - k_base;
    if (valid_lanes > reduce_chunk_elems) {
      valid_lanes = reduce_chunk_elems;
    }
    acc_bits = cute_mma_model::reduce_int32_chunk(std::is_signed<src1_t>::value, std::is_signed<src2_t>::value,
                                                  src1_row + k_base, src2_row + k_base, valid_lanes,
                                                  kMmaReduceWidthBytes, acc_bits);
  }

  write_u32_device(src3, i, j, result_row_bytes, acc_bits);
}

__global__ void mfmacc_cute_fp32_kernel(cute_mma_model::FloatFormat format, const uint8_t *src1, const uint8_t *src2,
                                        uint8_t *src3, int tile_m, int tile_k, int tile_n) {
  int idx = blockIdx.x * blockDim.x + threadIdx.x;
  int total = tile_m * tile_n;
  if (idx >= total) {
    return;
  }

  int i = idx / tile_n;
  int j = idx % tile_n;
  int src_elem_bytes = cute_mma_model::source_bytes(format);
  int reduce_chunk_elems = kMmaReduceWidthBytes / src_elem_bytes;
  size_t src_row_bytes = static_cast<size_t>(tile_k) * src_elem_bytes;
  size_t result_row_bytes = static_cast<size_t>(tile_n) * sizeof(uint32_t);
  const uint8_t *src1_row = src1 + static_cast<size_t>(i) * src_row_bytes;
  const uint8_t *src2_row = src2 + static_cast<size_t>(j) * src_row_bytes;
  uint32_t acc_bits = read_u32_device(src3, i, j, result_row_bytes);

  for (int k_base = 0; k_base < tile_k; k_base += reduce_chunk_elems) {
    int valid_lanes = tile_k - k_base;
    if (valid_lanes > reduce_chunk_elems) {
      valid_lanes = reduce_chunk_elems;
    }
    acc_bits = cute_mma_model::reduce_fp32_chunk(format, src1_row + static_cast<size_t>(k_base) * src_elem_bytes,
                                                 src2_row + static_cast<size_t>(k_base) * src_elem_bytes, valid_lanes,
                                                 kMmaReduceWidthBytes, acc_bits);
  }

  write_u32_device(src3, i, j, result_row_bytes, acc_bits);
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
    uint8_t *src1,
    uint8_t *src2,
    uint8_t *src3) {
  int total = tile_m * tile_n;
  int block_size = 256;
  int grid_size = (total + block_size - 1) / block_size;

  switch (type) {
    case CudaMmaType::U8U8:
      mmacc_cute_int32_kernel<uint8_t, uint8_t><<<grid_size, block_size>>>(src1, src2, src3, tile_m, tile_k, tile_n);
      break;
    case CudaMmaType::U8S8:
      mmacc_cute_int32_kernel<uint8_t, int8_t><<<grid_size, block_size>>>(src1, src2, src3, tile_m, tile_k, tile_n);
      break;
    case CudaMmaType::S8U8:
      mmacc_cute_int32_kernel<int8_t, uint8_t><<<grid_size, block_size>>>(src1, src2, src3, tile_m, tile_k, tile_n);
      break;
    case CudaMmaType::S8S8:
      mmacc_cute_int32_kernel<int8_t, int8_t><<<grid_size, block_size>>>(src1, src2, src3, tile_m, tile_k, tile_n);
      break;
    case CudaMmaType::Fp8E5M2ToFp32:
      mfmacc_cute_fp32_kernel<<<grid_size, block_size>>>(cute_mma_model::FloatFormat::Fp8E5M2, src1, src2, src3, tile_m,
                                                         tile_k, tile_n);
      break;
    case CudaMmaType::Fp8E4M3ToFp32:
      mfmacc_cute_fp32_kernel<<<grid_size, block_size>>>(cute_mma_model::FloatFormat::Fp8E4M3, src1, src2, src3, tile_m,
                                                         tile_k, tile_n);
      break;
    case CudaMmaType::Fp16ToFp32:
      mfmacc_cute_fp32_kernel<<<grid_size, block_size>>>(cute_mma_model::FloatFormat::Fp16, src1, src2, src3, tile_m,
                                                         tile_k, tile_n);
      break;
    case CudaMmaType::Bf16ToFp32:
      mfmacc_cute_fp32_kernel<<<grid_size, block_size>>>(cute_mma_model::FloatFormat::Bf16, src1, src2, src3, tile_m,
                                                         tile_k, tile_n);
      break;
    case CudaMmaType::Tf32ToFp32:
      mfmacc_cute_fp32_kernel<<<grid_size, block_size>>>(cute_mma_model::FloatFormat::Tf32, src1, src2, src3, tile_m,
                                                         tile_k, tile_n);
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
    const uint8_t *src1,
    const uint8_t *src2,
    uint8_t *src3,
    const uint8_t *dut_result) {
  size_t src1_size = element_size(types1) * tile_m * tile_k;
  size_t src2_size = element_size(types2) * tile_k * tile_n;
  size_t result_size = sizeof(uint32_t) * tile_m * tile_n;

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
  if (!launch_kernel(type, tile_m, tile_k, tile_n, dev_src1, dev_src2, dev_src3)) {
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
