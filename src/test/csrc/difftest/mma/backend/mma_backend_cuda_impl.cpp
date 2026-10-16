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

#ifdef CONFIG_DIFFTEST_MMA_CUDA

#include <cuda_runtime_api.h>
#include <cstdio>
#include <cstring>
#include <memory>
#include <vector>

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

void CudaMmaHostDeleter::operator()(uint8_t *buffer) const {
  cudaFreeHost(buffer);
}

static bool allocate_host_buffer(std::unique_ptr<uint8_t[], CudaMmaHostDeleter> &buffer, size_t bytes,
                                 const char *what) {
  uint8_t *data = nullptr;
  if (!report_error(what, cudaMallocHost(reinterpret_cast<void **>(&data), bytes))) {
    return false;
  }
  buffer.reset(data);
  return true;
}

bool CudaMmaWorkspace::allocate(size_t batch_size, size_t ab_reg_bytes, size_t c_reg_bytes) {
  if (descriptors != nullptr || batch_size == 0 || batch_size > 65535 || ab_reg_bytes == 0 || c_reg_bytes == 0 ||
      batch_size > SIZE_MAX / sizeof(CudaMmaBatchDescriptor) || ab_reg_bytes > SIZE_MAX / batch_size ||
      c_reg_bytes > SIZE_MAX / batch_size) {
    fprintf(stderr, "CudaMmaBackend: invalid workspace capacities\n");
    return false;
  }

  host_descriptors.resize(batch_size);
  if (!allocate_host_buffer(host_src1, batch_size * ab_reg_bytes, "allocate pinned batch src1") ||
      !allocate_host_buffer(host_src2, batch_size * ab_reg_bytes, "allocate pinned batch src2") ||
      !allocate_host_buffer(host_src3, batch_size * c_reg_bytes, "allocate pinned batch src3")) {
    release();
    return false;
  }

  if (!report_error("allocate batch descriptors",
                    cudaMalloc(reinterpret_cast<void **>(&descriptors), batch_size * sizeof(CudaMmaBatchDescriptor))) ||
      !report_error("allocate batch src1", cudaMalloc(reinterpret_cast<void **>(&src1), batch_size * ab_reg_bytes)) ||
      !report_error("allocate batch src2", cudaMalloc(reinterpret_cast<void **>(&src2), batch_size * ab_reg_bytes)) ||
      !report_error("allocate batch src3", cudaMalloc(reinterpret_cast<void **>(&src3), batch_size * c_reg_bytes))) {
    release();
    return false;
  }
  max_batch_size = batch_size;
  ab_reg_size = ab_reg_bytes;
  c_reg_size = c_reg_bytes;
  return true;
}

size_t CudaMmaWorkspace::batch_capacity() const {
  return max_batch_size;
}

bool CudaMmaWorkspace::supports(size_t src1_bytes, size_t src2_bytes, size_t src3_bytes) const {
  return src1_bytes <= ab_reg_size && src2_bytes <= ab_reg_size && src3_bytes <= c_reg_size;
}

void CudaMmaWorkspace::release() {
  if (descriptors != nullptr) cudaFree(descriptors);
  if (src1 != nullptr) cudaFree(src1);
  if (src2 != nullptr) cudaFree(src2);
  if (src3 != nullptr) cudaFree(src3);
  descriptors = nullptr;
  src1 = nullptr;
  src2 = nullptr;
  src3 = nullptr;
  host_src1.reset();
  host_src2.reset();
  host_src3.reset();
  max_batch_size = 0;
  ab_reg_size = 0;
  c_reg_size = 0;
}

CudaMmaWorkspace *cuda_mma_backend_create(size_t batch_size, size_t ab_reg_bytes, size_t c_reg_bytes) {
  std::unique_ptr<CudaMmaWorkspace> workspace(new CudaMmaWorkspace());
  if (!workspace->allocate(batch_size, ab_reg_bytes, c_reg_bytes)) {
    return nullptr;
  }
  return workspace.release();
}

bool cuda_mma_backend_launch(CudaMmaWorkspace *workspace, CudaMmaType type, const CudaMmaBatchItem *items,
                             size_t count, uint8_t *passed) {
  if (count == 0) {
    return true;
  }
  if (!workspace || !items || !passed || count > 65535) {
    return false;
  }
  memset(passed, 0, count * sizeof(*passed));
  if (count > workspace->batch_capacity()) {
    fprintf(stderr, "CudaMmaBackend: batch of %zu exceeds workspace capacity %zu\n", count, workspace->batch_capacity());
    return false;
  }

  auto &descriptors = workspace->host_descriptors;
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

    const size_t src1_size = element_size(item.types1) * item.tile_m * item.tile_k;
    const size_t src2_size = element_size(item.types2) * item.tile_k * item.tile_n;
    const size_t src3_size = sizeof(uint32_t) * item.tile_m * item.tile_n;
    if (!workspace->supports(src1_size, src2_size, src3_size)) {
      fprintf(stderr, "CudaMmaBackend: request %zu exceeds configured matrix register capacities\n", i);
      return false;
    }
    total_src1_size += src1_size;
    total_src2_size += src2_size;
    total_src3_size += src3_size;
    size_t output_elements = static_cast<size_t>(item.tile_m) * item.tile_n;
    if (output_elements > max_output_elements) {
      max_output_elements = output_elements;
    }
  }

  uint8_t *packed_src1 = workspace->host_src1.get();
  uint8_t *packed_src2 = workspace->host_src2.get();
  uint8_t *packed_src3 = workspace->host_src3.get();
  for (size_t i = 0; i < count; ++i) {
    const CudaMmaBatchItem &item = items[i];
    const CudaMmaBatchDescriptor &desc = descriptors[i];
    size_t src1_size = element_size(item.types1) * item.tile_m * item.tile_k;
    size_t src2_size = element_size(item.types2) * item.tile_k * item.tile_n;
    size_t src3_size = sizeof(uint32_t) * item.tile_m * item.tile_n;
    memcpy(packed_src1 + desc.src1_offset, item.src1, src1_size);
    memcpy(packed_src2 + desc.src2_offset, item.src2, src2_size);
    memcpy(packed_src3 + desc.src3_offset, item.src3, src3_size);
  }

  const size_t descriptors_bytes = count * sizeof(descriptors[0]);
  if (!report_error("copy batch descriptors to device", cudaMemcpy(workspace->descriptors, descriptors.data(),
                                                                   descriptors_bytes, cudaMemcpyHostToDevice)) ||
      !report_error("copy batch src1 to device",
                    cudaMemcpy(workspace->src1, packed_src1, total_src1_size, cudaMemcpyHostToDevice)) ||
      !report_error("copy batch src2 to device",
                    cudaMemcpy(workspace->src2, packed_src2, total_src2_size, cudaMemcpyHostToDevice)) ||
      !report_error("copy batch src3 to device",
                    cudaMemcpy(workspace->src3, packed_src3, total_src3_size, cudaMemcpyHostToDevice))) {
    return false;
  }
  if (!cuda_mma_backend_launch_kernel(type, workspace->descriptors, count, max_output_elements,
                                      workspace->src1, workspace->src2, workspace->src3)) {
    return false;
  }
  if (!report_error("batch kernel sync", cudaDeviceSynchronize())) {
    return false;
  }
  if (!report_error("copy batch src3 to host",
                    cudaMemcpy(packed_src3, workspace->src3, total_src3_size, cudaMemcpyDeviceToHost))) {
    return false;
  }

  for (size_t i = 0; i < count; ++i) {
    const CudaMmaBatchItem &item = items[i];
    const CudaMmaBatchDescriptor &desc = descriptors[i];
    size_t result_size = sizeof(uint32_t) * item.tile_m * item.tile_n;
    memcpy(item.src3, packed_src3 + desc.src3_offset, result_size);
    passed[i] = memcmp(item.dut_result, item.src3, result_size) == 0 ? 1 : 0;
  }
  return true;
}

void cuda_mma_backend_release(CudaMmaWorkspace *workspace) {
  if (workspace != nullptr) {
    workspace->release();
    delete workspace;
  }
}

#endif // CONFIG_DIFFTEST_MMA_CUDA
