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

static constexpr size_t kRangeSize = 32;

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

  capacity = batch_size;
  dut_spans.reserve(batch_size + 1);
  if (!report_error("allocate host batch descriptors", cudaMallocHost(reinterpret_cast<void **>(&host_descriptors),
                                                                      batch_size * sizeof(CudaMmaBatchDescriptor)))) {
    release();
    return false;
  }
  for (auto &stream: streams) {
    if (!report_error("create batch stream", cudaStreamCreateWithFlags(&stream, cudaStreamNonBlocking))) {
      release();
      return false;
    }
  }
  if (!report_error("create DUT upload stream", cudaStreamCreateWithFlags(&dut_stream, cudaStreamNonBlocking))) {
    release();
    return false;
  }
  completed.resize((batch_size + kRangeSize - 1) / kRangeSize);
  for (auto &event: completed) {
    if (!report_error("create range completion",
                      cudaEventCreateWithFlags(&event, cudaEventBlockingSync | cudaEventDisableTiming))) {
      release();
      return false;
    }
  }
  dut_uploaded.resize(completed.size());
  for (auto &event: dut_uploaded) {
    if (!report_error("create DUT upload completion", cudaEventCreateWithFlags(&event, cudaEventDisableTiming))) {
      release();
      return false;
    }
  }
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
  if (!report_error("allocate device DUT results",
                    cudaMalloc(reinterpret_cast<void **>(&device_dut), batch_size * c_reg_bytes)) ||
      !report_error("allocate device comparison flags",
                    cudaMalloc(reinterpret_cast<void **>(&device_passed), batch_size))) {
    release();
    return false;
  }
  return true;
}

size_t CudaMmaWorkspace::batch_capacity() const {
  return capacity;
}

bool CudaMmaWorkspace::synchronize() {
  bool success = true;
  for (auto stream: streams) {
    if (stream && !report_error("batch stream sync", cudaStreamSynchronize(stream)))
      success = false;
  }
  if (dut_stream && !report_error("DUT stream sync", cudaStreamSynchronize(dut_stream)))
    success = false;
  return success;
}

bool CudaMmaWorkspace::wait(size_t begin, size_t end) {
  for (size_t i = begin / kRangeSize; i < (end + kRangeSize - 1) / kRangeSize; ++i) {
    if (!report_error("wait for range completion", cudaEventSynchronize(completed[i])))
      return false;
  }
  return true;
}

void CudaMmaWorkspace::release() {
  synchronize();
  for (auto event: completed) {
    if (event)
      cudaEventDestroy(event);
  }
  completed.clear();
  for (auto event: dut_uploaded) {
    if (event)
      cudaEventDestroy(event);
  }
  dut_uploaded.clear();
  if (dut_stream) {
    cudaStreamDestroy(dut_stream);
    dut_stream = nullptr;
  }
  if (device_dut) {
    cudaFree(device_dut);
    device_dut = nullptr;
  }
  if (device_passed) {
    cudaFree(device_passed);
    device_passed = nullptr;
  }
  for (auto &stream: streams) {
    if (stream) {
      cudaStreamDestroy(stream);
      stream = nullptr;
    }
  }
  if (host_descriptors) {
    cudaFreeHost(host_descriptors);
    host_descriptors = nullptr;
  }
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
  capacity = 0;
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

static void map_operand(CudaMmaWorkspace &workspace, const CudaMmaBatchItem *items, size_t begin, size_t end,
                        size_t operand) {
  const auto &host = workspace.host[operand];
  auto &spans = workspace.operands[operand].spans;
  spans.clear();
  const auto &op = workspace.operands[operand];
  size_t previous_slot = MmaHostBuffer::invalid_slot;
  for (size_t i = begin; i < end; ++i) {
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

static bool copy_operand(CudaMmaWorkspace &workspace, size_t operand, size_t begin, cudaStream_t stream) {
  auto &op = workspace.operands[operand];
  size_t offset = begin * op.stride;
  for (const auto &span: op.spans) {
    void *device = op.device + offset;
    if (!report_error("copy batch operand",
                      cudaMemcpyAsync(device, span.host, span.bytes, cudaMemcpyHostToDevice, stream))) {
      return false;
    }
    offset += span.bytes;
  }
  return true;
}

static bool validate_batch(CudaMmaWorkspace *workspace, const CudaMmaBatchItem *items, size_t count) {
  if (!workspace || !items || count > 65535)
    return false;
  if (count > workspace->batch_capacity()) {
    fprintf(stderr, "CudaMmaBackend: batch of %zu exceeds workspace capacity %zu\n", count,
            workspace->batch_capacity());
    return false;
  }
  return true;
}

static bool enqueue_range(CudaMmaWorkspace &workspace, CudaMmaType type, const CudaMmaBatchItem *items, size_t begin,
                          size_t end, cudaStream_t stream) {
  size_t max_output_elements = 0;
  for (size_t i = begin; i < end; ++i) {
    const auto &item = items[i];
    auto &desc = workspace.host_descriptors[i];
    desc.tile_m = item.tile_m;
    desc.tile_k = item.tile_k;
    desc.tile_n = item.tile_n;
    desc.sat = item.sat;
    max_output_elements = std::max(max_output_elements, size_t(item.tile_m) * item.tile_n);
  }
  if (max_output_elements == 0)
    return true;
  for (size_t operand = 0; operand < 3; ++operand)
    map_operand(workspace, items, begin, end, operand);
  if (!report_error("copy batch descriptors to device",
                    cudaMemcpyAsync(workspace.descriptors + begin, workspace.host_descriptors + begin,
                                    (end - begin) * sizeof(CudaMmaBatchDescriptor), cudaMemcpyHostToDevice, stream)))
    return false;
  for (size_t operand = 0; operand < 3; ++operand)
    if (!copy_operand(workspace, operand, begin, stream))
      return false;
  return cuda_mma_backend_launch_kernel(type, workspace.descriptors + begin, end - begin, max_output_elements,
                                        workspace.operands[0].device, workspace.operands[1].device,
                                        workspace.operands[2].device, stream);
}

bool cuda_mma_backend_enqueue_references(CudaMmaWorkspace *workspace, CudaMmaType type, const CudaMmaBatchItem *items,
                                         size_t count) {
  if (count == 0)
    return true;
  if (!validate_batch(workspace, items, count))
    return false;
  for (size_t begin = 0; begin < count; begin += kRangeSize) {
    const size_t end = std::min(begin + kRangeSize, count);
    const auto stream = workspace->streams[(begin / kRangeSize) % 4];
    if (!enqueue_range(*workspace, type, items, begin, end, stream))
      return false;
  }
  return true;
}

bool cuda_mma_backend_enqueue_checks(CudaMmaWorkspace *workspace, const CudaMmaBatchItem *items, size_t count,
                                     uint8_t *passed) {
  if (count == 0)
    return true;
  if (!validate_batch(workspace, items, count) || !passed)
    return false;
  const size_t result_bytes = workspace->operands[2].stride;
  for (size_t begin = 0; begin < count; begin += kRangeSize) {
    const size_t end = std::min(begin + kRangeSize, count);
    const auto stream = workspace->streams[(begin / kRangeSize) % 4];
    size_t max_output_elements = 0;
    for (size_t i = begin; i < end; ++i)
      max_output_elements = std::max(max_output_elements, size_t(items[i].tile_m) * items[i].tile_n);
    if (max_output_elements) {
      // Coalesce adjacent prepared slots without repacking on the caller.
      auto &spans = workspace->dut_spans;
      spans.clear();
      for (size_t i = begin; i < end; ++i) {
        auto *data = const_cast<uint8_t *>(items[i].dut);
        if (!spans.empty() &&
            reinterpret_cast<uintptr_t>(data) == reinterpret_cast<uintptr_t>(spans.back().host) + spans.back().bytes)
          spans.back().bytes += result_bytes;
        else
          spans.push_back({data, result_bytes});
      }
#if CUDART_VERSION >= 13030
      if (spans.size() > 1) {
        void *destinations[kRangeSize];
        const void *sources[kRangeSize];
        size_t sizes[kRangeSize];
        size_t offset = begin * result_bytes;
        for (size_t i = 0; i < spans.size(); ++i) {
          destinations[i] = workspace->device_dut + offset;
          sources[i] = spans[i].host;
          sizes[i] = spans[i].bytes;
          offset += sizes[i];
        }
        cudaMemcpyAttributes attributes = {};
        attributes.srcAccessOrder = cudaMemcpySrcAccessOrderStream;
        attributes.flags = cudaMemcpyFlagPreferOverlapWithCompute;
        size_t index = 0;
        if (!report_error("upload DUT result batch",
                          cudaMemcpyBatchAsync(destinations, sources, sizes, spans.size(), &attributes, &index, 1,
                                               workspace->dut_stream)))
          return false;
      } else
#endif
      {
        size_t offset = begin * result_bytes;
        for (const auto &span: spans) {
          if (!report_error("upload DUT results", cudaMemcpyAsync(workspace->device_dut + offset, span.host, span.bytes,
                                                                  cudaMemcpyHostToDevice, workspace->dut_stream)))
            return false;
          offset += span.bytes;
        }
      }
      const auto uploaded = workspace->dut_uploaded[begin / kRangeSize];
      if (!report_error("record DUT upload", cudaEventRecord(uploaded, workspace->dut_stream)) ||
          !report_error("wait for DUT upload", cudaStreamWaitEvent(stream, uploaded, 0)) ||
          !cuda_mma_backend_compare_kernel(workspace->descriptors + begin, end - begin, workspace->operands[2].device,
                                           workspace->device_dut, workspace->device_passed + begin, stream) ||
          !report_error("download comparison flags", cudaMemcpyAsync(passed + begin, workspace->device_passed + begin,
                                                                     end - begin, cudaMemcpyDeviceToHost, stream)))
        return false;
    } else {
      memset(passed + begin, 1, end - begin);
    }
    if (count > kMmaCompletionSize &&
        !report_error("record range completion", cudaEventRecord(workspace->completed[begin / kRangeSize], stream)))
      return false;
  }
  return true;
}

bool cuda_mma_backend_read_result(CudaMmaWorkspace *workspace, size_t index, uint8_t *result) {
  const auto &operand = workspace->operands[2];
  return report_error("read mismatched reference result", cudaMemcpy(result, operand.device + index * operand.stride,
                                                                     operand.stride, cudaMemcpyDeviceToHost));
}

#endif // CONFIG_DIFFTEST_MMA_CUDA
