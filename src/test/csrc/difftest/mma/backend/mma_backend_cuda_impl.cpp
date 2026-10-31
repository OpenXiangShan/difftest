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

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <cuda_runtime_api.h>
#include <memory>
#include <vector>

static bool report_error(const char *what, cudaError_t err) {
  if (err == cudaSuccess) {
    return true;
  }
  fprintf(stderr, "CudaMmaBackend: %s failed: %s\n", what, cudaGetErrorString(err));
  return false;
}

static uint8_t *allocate_pinned(size_t bytes) {
  uint8_t *buffer = nullptr;
  if (!report_error("allocate pinned host buffer", cudaMallocHost(reinterpret_cast<void **>(&buffer), bytes))) {
    return nullptr;
  }
  return buffer;
}

static void free_pinned(uint8_t *buffer) {
  cudaFreeHost(buffer);
}

CudaMmaWorkspace::~CudaMmaWorkspace() {
  release();
}

bool CudaMmaWorkspace::allocate(size_t batch_size, size_t ab_reg_bytes, size_t c_reg_bytes,
                                uint8_t *const shared_host[3]) {
  // Pending requests, one producer snapshot and one retained first mismatch.
  const size_t slots = batch_size * CONFIG_DIFF_MMA_BACKPRESSURE_FACTOR + 2;
  if (descriptors != nullptr || batch_size == 0 || batch_size > 65535 || ab_reg_bytes == 0 || c_reg_bytes == 0 ||
      batch_size > SIZE_MAX / sizeof(CudaMmaBatchDescriptor) || ab_reg_bytes > SIZE_MAX / batch_size ||
      c_reg_bytes > SIZE_MAX / batch_size) {
    fprintf(stderr, "CudaMmaBackend: invalid workspace capacities\n");
    return false;
  }

  host_descriptors.resize(batch_size);
  for (size_t i = 0; i < 3; ++i) {
    auto &op = operands[i];
    const size_t bytes = i == 2 ? c_reg_bytes : ab_reg_bytes;
    op.stride = bytes;
    op.shared_host = shared_host ? shared_host[i] : nullptr;
    op.spans.reserve(batch_size + 1);
    if (!op.shared_host && !host[i].allocate(bytes, slots, allocate_pinned, free_pinned)) {
      release();
      return false;
    }
  }
  if (!report_error("allocate batch descriptors",
                    cudaMalloc(reinterpret_cast<void **>(&descriptors), batch_size * sizeof(CudaMmaBatchDescriptor)))) {
    release();
    return false;
  }
  for (size_t i = 0; i < 3; ++i) {
    if (!report_error("allocate batch operand",
                      cudaMalloc(reinterpret_cast<void **>(&operands[i].device), batch_size * operands[i].stride))) {
      release();
      return false;
    }
  }
  return true;
}

size_t CudaMmaWorkspace::batch_capacity() const {
  return host_descriptors.size();
}

void CudaMmaWorkspace::release() {
  if (descriptors != nullptr) {
    cudaFree(descriptors);
    descriptors = nullptr;
  }
  for (size_t i = 0; i < 3; ++i) {
    auto &op = operands[i];
    if (op.device != nullptr) {
      cudaFree(op.device);
      op.device = nullptr;
    }
    host[i].release();
    op.spans.clear();
  }
  host_descriptors.clear();
}

CudaMmaWorkspace *cuda_mma_backend_create(MmaHostBuffer (&host)[3], size_t batch_size, size_t ab_reg_bytes,
                                          size_t c_reg_bytes, uint8_t *const shared_host[3]) {
  std::unique_ptr<CudaMmaWorkspace> workspace(new CudaMmaWorkspace(host));
  if (!workspace->allocate(batch_size, ab_reg_bytes, c_reg_bytes, shared_host)) {
    return nullptr;
  }
  return workspace.release();
}

static const uint8_t *operand_data(const CudaMmaBatchItem &item, size_t operand) {
  return operand == 0 ? item.src1 : operand == 1 ? item.src2 : item.src3;
}

static uint64_t &operand_offset(CudaMmaBatchDescriptor &desc, size_t operand) {
  return operand == 0 ? desc.src1_offset : operand == 1 ? desc.src2_offset : desc.src3_offset;
}

static void map_operand(CudaMmaWorkspace &workspace, const CudaMmaBatchItem *items, size_t count, size_t operand) {
  const auto &host = workspace.host[operand];
  auto &spans = workspace.operands[operand].spans;
  spans.clear();
  const auto &op = workspace.operands[operand];
  size_t previous_slot = MmaHostBuffer::invalid_slot;
  for (size_t i = 0; i < count; ++i) {
    auto *data = const_cast<uint8_t *>(operand_data(items[i], operand));
    const size_t slot =
        op.shared_host ? (reinterpret_cast<uintptr_t>(data) - reinterpret_cast<uintptr_t>(op.shared_host)) / op.stride
                       : host.slot(data);
    operand_offset(workspace.host_descriptors[i], operand) = i * op.stride;
    if (slot != MmaHostBuffer::invalid_slot && previous_slot != MmaHostBuffer::invalid_slot &&
        slot == previous_slot + 1) {
      spans.back().bytes += op.stride;
    } else {
      spans.push_back({data, op.stride});
    }
    previous_slot = slot;
  }
}

static bool copy_operand(CudaMmaWorkspace &workspace, size_t operand, cudaMemcpyKind direction) {
  auto &op = workspace.operands[operand];
  size_t offset = 0;
  for (const auto &span: op.spans) {
    void *device = op.device + offset;
    const bool to_device = direction == cudaMemcpyHostToDevice;
    if (!report_error("copy batch operand", cudaMemcpy(to_device ? device : span.host, to_device ? span.host : device,
                                                       span.bytes, direction))) {
      return false;
    }
    offset += span.bytes;
  }
  return true;
}

bool cuda_mma_backend_launch(CudaMmaWorkspace *workspace, CudaMmaType type, const CudaMmaBatchItem *items,
                             size_t count) {
  if (count == 0) {
    return true;
  }
  if (!workspace || !items || count > 65535) {
    return false;
  }
  if (count > workspace->batch_capacity()) {
    fprintf(stderr, "CudaMmaBackend: batch of %zu exceeds workspace capacity %zu\n", count,
            workspace->batch_capacity());
    return false;
  }

  size_t max_output_elements = 0;
  for (size_t i = 0; i < count; ++i) {
    const auto &item = items[i];
    auto &desc = workspace->host_descriptors[i];
    desc.tile_m = item.tile_m;
    desc.tile_k = item.tile_k;
    desc.tile_n = item.tile_n;
    desc.sat = item.sat;
    max_output_elements = std::max(max_output_elements, size_t(item.tile_m) * item.tile_n);
  }
  if (max_output_elements == 0) {
    return true;
  }
  for (size_t operand = 0; operand < 3; ++operand) {
    map_operand(*workspace, items, count, operand);
  }
  const size_t descriptors_bytes = count * sizeof(CudaMmaBatchDescriptor);
  if (!report_error("copy batch descriptors to device",
                    cudaMemcpy(workspace->descriptors, workspace->host_descriptors.data(), descriptors_bytes,
                               cudaMemcpyHostToDevice))) {
    return false;
  }
  for (size_t operand = 0; operand < 3; ++operand) {
    if (!copy_operand(*workspace, operand, cudaMemcpyHostToDevice)) {
      return false;
    }
  }
  if (!cuda_mma_backend_launch_kernel(type, workspace->descriptors, count, max_output_elements,
                                      workspace->operands[0].device, workspace->operands[1].device,
                                      workspace->operands[2].device) ||
      !report_error("batch kernel sync", cudaDeviceSynchronize()) ||
      !copy_operand(*workspace, 2, cudaMemcpyDeviceToHost)) {
    return false;
  }

  return true;
}

void cuda_mma_backend_release(CudaMmaWorkspace *workspace) {
  delete workspace;
}

#endif // CONFIG_DIFFTEST_MMA_CUDA
