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

bool CudaMmaWorkspace::allocate(size_t batch_size, size_t ab_reg_bytes, size_t c_reg_bytes) {
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
    op.spans.reserve(batch_size + 1);
    op.packed.get_deleter().free_host = free_pinned;
    op.packed.reset(allocate_pinned(batch_size * bytes));
    if (!op.packed || !host[i].allocate(bytes, slots, allocate_pinned, free_pinned)) {
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
                      cudaMalloc(reinterpret_cast<void **>(&operands[i].device), batch_size * host[i].stride))) {
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
    op.packed.reset();
    op.spans.clear();
    op.direct_bytes = 0;
  }
  host_descriptors.clear();
}

CudaMmaWorkspace *cuda_mma_backend_create(MmaHostBuffer (&host)[3], size_t batch_size, size_t ab_reg_bytes,
                                          size_t c_reg_bytes) {
  std::unique_ptr<CudaMmaWorkspace> workspace(new CudaMmaWorkspace(host));
  if (!workspace->allocate(batch_size, ab_reg_bytes, c_reg_bytes)) {
    return nullptr;
  }
  return workspace.release();
}

static const uint8_t *operand_data(const CudaMmaBatchItem &item, size_t operand) {
  return operand == 0 ? item.src1 : operand == 1 ? item.src2 : item.src3;
}

static size_t operand_bytes(const CudaMmaBatchItem &item, size_t operand) {
  if (operand == 2) {
    return sizeof(uint32_t) * item.tile_m * item.tile_n;
  }
  return element_size(operand == 0 ? item.types1 : item.types2) * item.tile_k *
         (operand == 0 ? item.tile_m : item.tile_n);
}

static uint64_t &operand_offset(CudaMmaBatchDescriptor &desc, size_t operand) {
  return operand == 0 ? desc.src1_offset : operand == 1 ? desc.src2_offset : desc.src3_offset;
}

static void pack_operand(CudaMmaWorkspace &workspace, const CudaMmaBatchItem *items, size_t count, size_t operand) {
  auto &host = workspace.host[operand];
  auto &descriptors = workspace.host_descriptors;
  auto &op = workspace.operands[operand];
  op.spans.clear();
  size_t first = 0, full_count = 0;
  bool direct = true;
  for (size_t i = 0; i < count; ++i) {
    const size_t slot = host.slot(operand_data(items[i], operand));
    if (slot == MmaHostBuffer::invalid_slot)
      continue;
    if (operand_bytes(items[i], operand) != host.stride ||
        (full_count != 0 && slot != (first + full_count) % host.slot_count)) {
      direct = false;
      break;
    }
    if (full_count == 0)
      first = slot;
    ++full_count;
  }
  if (!direct)
    full_count = 0;
  op.direct_bytes = full_count * host.stride;
  if (op.direct_bytes != 0) {
    const size_t head = std::min(full_count, host.slot_count - first) * host.stride;
    op.spans.push_back({host.slots.get() + first * host.stride, head});
    if (head < op.direct_bytes)
      op.spans.push_back({host.slots.get(), op.direct_bytes - head});
  }
  size_t direct_offset = 0, packed_bytes = 0;
  for (size_t i = 0; i < count; ++i) {
    const auto *source = operand_data(items[i], operand);
    const size_t bytes = operand_bytes(items[i], operand);
    auto &offset = operand_offset(descriptors[i], operand);
    if (op.direct_bytes && host.slot(source) != MmaHostBuffer::invalid_slot) {
      offset = direct_offset;
      direct_offset += bytes;
    } else {
      offset = op.direct_bytes + packed_bytes;
      memcpy(op.packed.get() + packed_bytes, source, bytes);
      packed_bytes += bytes;
    }
  }
  if (packed_bytes != 0)
    op.spans.push_back({op.packed.get(), packed_bytes});
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

bool cuda_mma_backend_launch(CudaMmaWorkspace *workspace, CudaMmaType type, const CudaMmaBatchItem *items, size_t count,
                             uint8_t *passed) {
  if (count == 0) {
    return true;
  }
  if (!workspace || !items || !passed || count > 65535) {
    return false;
  }
  memset(passed, 0, count * sizeof(*passed));
  if (count > workspace->batch_capacity()) {
    fprintf(stderr, "CudaMmaBackend: batch of %zu exceeds workspace capacity %zu\n", count,
            workspace->batch_capacity());
    return false;
  }

  size_t max_output_elements = 0;
  for (size_t i = 0; i < count; ++i) {
    const auto &item = items[i];
    if (item.tile_m == 0 || item.tile_k == 0 || item.tile_n == 0 || !item.src1 || !item.src2 || !item.src3 ||
        !item.dut_result) {
      return false;
    }
    auto &desc = workspace->host_descriptors[i];
    desc.tile_m = item.tile_m;
    desc.tile_k = item.tile_k;
    desc.tile_n = item.tile_n;
    desc.sat = item.sat;
    max_output_elements = std::max(max_output_elements, size_t(item.tile_m) * item.tile_n);
  }
  for (size_t operand = 0; operand < 3; ++operand) {
    pack_operand(*workspace, items, count, operand);
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

  const auto &output = workspace->operands[2];
  for (size_t i = 0; i < count; ++i) {
    const auto &item = items[i];
    const size_t offset = workspace->host_descriptors[i].src3_offset;
    const size_t bytes = operand_bytes(item, 2);
    const uint8_t *result =
        offset < output.direct_bytes ? item.src3 : output.packed.get() + offset - output.direct_bytes;
    passed[i] = memcmp(item.dut_result, result, bytes) == 0 ? 1 : 0;
    if (!passed[i] && result != item.src3) {
      // Retain the computed REF matrix for mismatch diagnostics.
      memcpy(item.src3, result, bytes);
    }
  }
  return true;
}

void cuda_mma_backend_release(CudaMmaWorkspace *workspace) {
  delete workspace;
}

#endif // CONFIG_DIFFTEST_MMA_CUDA
