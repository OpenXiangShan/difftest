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
#include <vector>

#ifdef CONFIG_DIFF_MMA_REDUCE_WIDTH_BYTES
static constexpr int kMmaReduceWidthBytes = CONFIG_DIFF_MMA_REDUCE_WIDTH_BYTES;
#else
static constexpr int kMmaReduceWidthBytes = 32;
#endif

struct CudaMmaBatchDescriptor {
  uint64_t src1_offset;
  uint64_t src2_offset;
  uint64_t src3_offset;
  uint16_t tile_m;
  uint16_t tile_k;
  uint16_t tile_n;
};

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
  int idx = blockIdx.x * blockDim.x + threadIdx.x;
  mmacc_cute_int32_element<src1_t, src2_t>(src1 + desc.src1_offset, src2 + desc.src2_offset, src3 + desc.src3_offset,
                                           desc.tile_m, desc.tile_k, desc.tile_n, idx);
}

__global__ void mfmacc_cute_fp32_batch_kernel(cute_mma_model::FloatFormat format,
                                              const CudaMmaBatchDescriptor *descriptors, const uint8_t *src1,
                                              const uint8_t *src2, uint8_t *src3) {
  const CudaMmaBatchDescriptor &desc = descriptors[blockIdx.y];
  int idx = blockIdx.x * blockDim.x + threadIdx.x;
  mfmacc_cute_fp32_element(format, src1 + desc.src1_offset, src2 + desc.src2_offset, src3 + desc.src3_offset,
                           desc.tile_m, desc.tile_k, desc.tile_n, idx);
}

static size_t element_size(uint8_t typed) {
  switch (typed & 3) {
    case 0: return 1;
    case 1: return 2;
    case 2: return 4;
    default: return 4;
  }
}

static bool report_error(const char *what, cudaError_t err) {
  if (err == cudaSuccess) {
    return true;
  }
  fprintf(stderr, "CudaMmaBackend: %s failed: %s\n", what, cudaGetErrorString(err));
  return false;
}

static bool launch_batch_kernel(CudaMmaType type, const CudaMmaBatchDescriptor *descriptors, size_t count,
                                size_t max_output_elements, uint8_t *src1, uint8_t *src2, uint8_t *src3) {
  if (count == 0 || count > 65535 || max_output_elements == 0) {
    return false;
  }
  const int block_size = 256;
  const int grid_size = static_cast<int>((max_output_elements + block_size - 1) / block_size);
  dim3 grid(grid_size, static_cast<unsigned int>(count));

  switch (type) {
    case CudaMmaType::U8U8:
      mmacc_cute_int32_batch_kernel<uint8_t, uint8_t><<<grid, block_size>>>(descriptors, src1, src2, src3);
      break;
    case CudaMmaType::U8S8:
      mmacc_cute_int32_batch_kernel<uint8_t, int8_t><<<grid, block_size>>>(descriptors, src1, src2, src3);
      break;
    case CudaMmaType::S8U8:
      mmacc_cute_int32_batch_kernel<int8_t, uint8_t><<<grid, block_size>>>(descriptors, src1, src2, src3);
      break;
    case CudaMmaType::S8S8:
      mmacc_cute_int32_batch_kernel<int8_t, int8_t><<<grid, block_size>>>(descriptors, src1, src2, src3);
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

extern "C" bool cuda_mma_backend_launch(CudaMmaType type, const CudaMmaBatchItem *items, size_t count,
                                        uint8_t *passed) {
  if (count == 0) {
    return true;
  }
  if (!items || !passed || count > 65535) {
    return false;
  }
  memset(passed, 0, count * sizeof(*passed));

  std::vector<CudaMmaBatchDescriptor> descriptors(count);
  size_t total_src1_size = 0;
  size_t total_src2_size = 0;
  size_t total_src3_size = 0;
  size_t max_output_elements = 0;
  for (size_t i = 0; i < count; ++i) {
    const CudaMmaBatchItem &item = items[i];
    if (item.tile_m == 0 || item.tile_k == 0 || item.tile_n == 0 || !item.src1 || !item.src2 || !item.src3 ||
        !item.dut_result) {
      return false;
    }

    CudaMmaBatchDescriptor &desc = descriptors[i];
    desc.src1_offset = total_src1_size;
    desc.src2_offset = total_src2_size;
    desc.src3_offset = total_src3_size;
    desc.tile_m = item.tile_m;
    desc.tile_k = item.tile_k;
    desc.tile_n = item.tile_n;

    total_src1_size += element_size(item.types1) * item.tile_m * item.tile_k;
    total_src2_size += element_size(item.types2) * item.tile_k * item.tile_n;
    total_src3_size += sizeof(uint32_t) * item.tile_m * item.tile_n;
    size_t output_elements = static_cast<size_t>(item.tile_m) * item.tile_n;
    if (output_elements > max_output_elements) {
      max_output_elements = output_elements;
    }
  }

  std::vector<uint8_t> packed_src1(total_src1_size);
  std::vector<uint8_t> packed_src2(total_src2_size);
  std::vector<uint8_t> packed_src3(total_src3_size);
  for (size_t i = 0; i < count; ++i) {
    const CudaMmaBatchItem &item = items[i];
    const CudaMmaBatchDescriptor &desc = descriptors[i];
    size_t src1_size = element_size(item.types1) * item.tile_m * item.tile_k;
    size_t src2_size = element_size(item.types2) * item.tile_k * item.tile_n;
    size_t src3_size = sizeof(uint32_t) * item.tile_m * item.tile_n;
    memcpy(packed_src1.data() + desc.src1_offset, item.src1, src1_size);
    memcpy(packed_src2.data() + desc.src2_offset, item.src2, src2_size);
    memcpy(packed_src3.data() + desc.src3_offset, item.src3, src3_size);
  }

  CudaMmaBatchDescriptor *dev_descriptors = nullptr;
  uint8_t *dev_src1 = nullptr;
  uint8_t *dev_src2 = nullptr;
  uint8_t *dev_src3 = nullptr;
  bool completed = false;

  if (!report_error("cudaMalloc batch descriptors",
                    cudaMalloc(&dev_descriptors, descriptors.size() * sizeof(descriptors[0]))) ||
      !report_error("cudaMalloc batch src1", cudaMalloc(&dev_src1, packed_src1.size())) ||
      !report_error("cudaMalloc batch src2", cudaMalloc(&dev_src2, packed_src2.size())) ||
      !report_error("cudaMalloc batch src3", cudaMalloc(&dev_src3, packed_src3.size()))) {
    goto cleanup;
  }
  if (!report_error("copy batch descriptors to device",
                    cudaMemcpy(dev_descriptors, descriptors.data(), descriptors.size() * sizeof(descriptors[0]),
                               cudaMemcpyHostToDevice)) ||
      !report_error("copy batch src1 to device",
                    cudaMemcpy(dev_src1, packed_src1.data(), packed_src1.size(), cudaMemcpyHostToDevice)) ||
      !report_error("copy batch src2 to device",
                    cudaMemcpy(dev_src2, packed_src2.data(), packed_src2.size(), cudaMemcpyHostToDevice)) ||
      !report_error("copy batch src3 to device",
                    cudaMemcpy(dev_src3, packed_src3.data(), packed_src3.size(), cudaMemcpyHostToDevice))) {
    goto cleanup;
  }
  if (!launch_batch_kernel(type, dev_descriptors, count, max_output_elements, dev_src1, dev_src2, dev_src3)) {
    goto cleanup;
  }
  if (!report_error("batch kernel sync", cudaDeviceSynchronize())) {
    goto cleanup;
  }
  if (!report_error("copy batch src3 to host",
                    cudaMemcpy(packed_src3.data(), dev_src3, packed_src3.size(), cudaMemcpyDeviceToHost))) {
    goto cleanup;
  }

  for (size_t i = 0; i < count; ++i) {
    const CudaMmaBatchItem &item = items[i];
    const CudaMmaBatchDescriptor &desc = descriptors[i];
    size_t result_size = sizeof(uint32_t) * item.tile_m * item.tile_n;
    memcpy(item.src3, packed_src3.data() + desc.src3_offset, result_size);
    passed[i] = memcmp(item.dut_result, item.src3, result_size) == 0 ? 1 : 0;
  }
  completed = true;

cleanup:
  if (dev_descriptors != nullptr) {
    cudaFree(dev_descriptors);
  }
  if (dev_src1 != nullptr) {
    cudaFree(dev_src1);
  }
  if (dev_src2 != nullptr) {
    cudaFree(dev_src2);
  }
  if (dev_src3 != nullptr) {
    cudaFree(dev_src3);
  }
  return completed;
}
