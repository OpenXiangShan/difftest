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

#include "mma/backend/cuda/int8.cuh"
#include "mma/backend/mma_backend_cuda_impl.h"
#include "mma/backend/mma_backend_cute_model.h"
#include <cuda_runtime_api.h>
#include <stdint.h>
#include <stdio.h>
#include <type_traits>

#ifdef CONFIG_DIFF_MMA_REDUCE_WIDTH_BYTES
static constexpr int kMmaReduceWidthBytes = CONFIG_DIFF_MMA_REDUCE_WIDTH_BYTES;
#else
static constexpr int kMmaReduceWidthBytes = 32;
#endif

__device__ __forceinline__ uint32_t read_u32_device(const uint8_t *data, size_t index0, size_t index1,
                                                    size_t row_bytes) {
  const uint8_t *row_ptr = data + index0 * row_bytes;
  return reinterpret_cast<const uint32_t *>(row_ptr)[index1];
}

__device__ __forceinline__ void write_u32_device(uint8_t *data, size_t index0, size_t index1, size_t row_bytes,
                                                 uint32_t value) {
  uint8_t *row_ptr = data + index0 * row_bytes;
  reinterpret_cast<uint32_t *>(row_ptr)[index1] = value;
}

template <class src1_t, class src2_t>
__device__ __forceinline__ void mmacc_cute_int32_element(const uint8_t *src1, const uint8_t *src2, uint8_t *src3,
                                                         int tile_m, int tile_k, int tile_n, int idx) {
  static_assert(sizeof(src1_t) == 1 && sizeof(src2_t) == 1, "CUTE integer MMA expects 8-bit source elements");
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

__device__ __forceinline__ void mfmacc_cute_fp32_element(cute_mma_model::FloatFormat format, const uint8_t *src1,
                                                         const uint8_t *src2, uint8_t *src3, int tile_m, int tile_k,
                                                         int tile_n, int idx) {
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

template <class src1_t, class src2_t>
__global__ void mmacc_cute_int32_batch_kernel(const CudaMmaBatchDescriptor *descriptors, const uint8_t *src1,
                                              const uint8_t *src2, uint8_t *src3) {
  const CudaMmaBatchDescriptor &desc = descriptors[blockIdx.y];
#if defined(__CUDA_ARCH__) && __CUDA_ARCH__ >= 800
  mmacc_int8_tensor<std::is_signed<src1_t>::value, std::is_signed<src2_t>::value>(desc, src1, src2, src3);
#else
  for (int idx = blockIdx.x * blockDim.x + threadIdx.x; idx < desc.tile_m * desc.tile_n;
       idx += blockDim.x * gridDim.x) {
    mmacc_cute_int32_element<src1_t, src2_t>(src1 + desc.src1_offset, src2 + desc.src2_offset, src3 + desc.src3_offset,
                                             desc.tile_m, desc.tile_k, desc.tile_n, idx);
  }
#endif
}

__global__ void mfmacc_cute_fp32_batch_kernel(cute_mma_model::FloatFormat format,
                                              const CudaMmaBatchDescriptor *descriptors, const uint8_t *src1,
                                              const uint8_t *src2, uint8_t *src3) {
  const CudaMmaBatchDescriptor &desc = descriptors[blockIdx.y];
  int idx = blockIdx.x * blockDim.x + threadIdx.x;
  mfmacc_cute_fp32_element(format, src1 + desc.src1_offset, src2 + desc.src2_offset, src3 + desc.src3_offset,
                           desc.tile_m, desc.tile_k, desc.tile_n, idx);
}

static bool report_error(const char *what, cudaError_t err) {
  if (err == cudaSuccess) {
    return true;
  }
  fprintf(stderr, "CudaMmaBackend: %s failed: %s\n", what, cudaGetErrorString(err));
  return false;
}

bool cuda_mma_backend_launch_kernel(CudaMmaType type, const CudaMmaBatchDescriptor *descriptors, size_t count,
                                    size_t max_output_elements, uint8_t *src1, uint8_t *src2, uint8_t *src3) {
  if (count == 0 || count > 65535 || max_output_elements == 0) {
    return false;
  }
  const int block_size = 256;
  const int grid_size = static_cast<int>((max_output_elements + block_size - 1) / block_size);
  dim3 grid(grid_size, static_cast<unsigned int>(count));
  dim3 int_grid((max_output_elements + 1023) / 1024, static_cast<unsigned int>(count));

  switch (type) {
    case CudaMmaType::U8U8:
      mmacc_cute_int32_batch_kernel<uint8_t, uint8_t><<<int_grid, block_size>>>(descriptors, src1, src2, src3);
      break;
    case CudaMmaType::U8S8:
      mmacc_cute_int32_batch_kernel<uint8_t, int8_t><<<int_grid, block_size>>>(descriptors, src1, src2, src3);
      break;
    case CudaMmaType::S8U8:
      mmacc_cute_int32_batch_kernel<int8_t, uint8_t><<<int_grid, block_size>>>(descriptors, src1, src2, src3);
      break;
    case CudaMmaType::S8S8:
      mmacc_cute_int32_batch_kernel<int8_t, int8_t><<<int_grid, block_size>>>(descriptors, src1, src2, src3);
      break;
    case CudaMmaType::Fp8E5M2ToFp32:
      mfmacc_cute_fp32_batch_kernel<<<grid, block_size>>>(cute_mma_model::FloatFormat::Fp8E5M2, descriptors, src1, src2,
                                                          src3);
      break;
    case CudaMmaType::Fp8E4M3ToFp32:
      mfmacc_cute_fp32_batch_kernel<<<grid, block_size>>>(cute_mma_model::FloatFormat::Fp8E4M3, descriptors, src1, src2,
                                                          src3);
      break;
    case CudaMmaType::Fp16ToFp32:
      mfmacc_cute_fp32_batch_kernel<<<grid, block_size>>>(cute_mma_model::FloatFormat::Fp16, descriptors, src1, src2,
                                                          src3);
      break;
    case CudaMmaType::Bf16ToFp32:
      mfmacc_cute_fp32_batch_kernel<<<grid, block_size>>>(cute_mma_model::FloatFormat::Bf16, descriptors, src1, src2,
                                                          src3);
      break;
    case CudaMmaType::Tf32ToFp32:
      mfmacc_cute_fp32_batch_kernel<<<grid, block_size>>>(cute_mma_model::FloatFormat::Tf32, descriptors, src1, src2,
                                                          src3);
      break;
    default: return false;
  }

  return report_error("batch kernel launch", cudaGetLastError());
}
