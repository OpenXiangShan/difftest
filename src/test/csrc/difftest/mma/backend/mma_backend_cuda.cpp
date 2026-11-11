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

#include "mma/backend/mma_backend_cuda.h"

#ifdef CONFIG_DIFFTEST_AMUCTRLEVENT

#ifdef CONFIG_DIFFTEST_MMA_CUDA
#ifndef CONFIG_DIFFTEST_HAS_CUDA_TOOLCHAIN
#error "CONFIG_DIFFTEST_MMA_CUDA requires CUDA toolchain, but none was detected"
#endif

#include "mma/backend/mma_backend_cuda_impl.h"
#include "mma/mma_verifier.h"
#include <algorithm>
#include <cstring>
#include <cuda_runtime_api.h>
#include <stdexcept>

bool select_cuda_mma_type(const DifftestAmuCtrlEvent &event, CudaMmaType *type) {
  if (!is_mma_32bit_result_type(event.typed)) {
    return false;
  }

  if (event.isfp) {
    const MmaElementType source_type = static_cast<MmaElementType>(event.types1);
    switch (source_type) {
      case MmaElementType::Fp8E5M2: *type = CudaMmaType::Fp8E5M2ToFp32; return true;
      case MmaElementType::Fp8E4M3: *type = CudaMmaType::Fp8E4M3ToFp32; return true;
      case MmaElementType::Fp16: *type = CudaMmaType::Fp16ToFp32; return true;
      case MmaElementType::Bf16: *type = CudaMmaType::Bf16ToFp32; return true;
      case MmaElementType::Tf32: *type = CudaMmaType::Tf32ToFp32; return true;
      default: return false;
    }
  }

  int op = ((event.types1 & 0x4) >> 1) | ((event.types2 & 0x4) >> 2);
  switch (op) {
    case 0: *type = CudaMmaType::U8U8; return true;
    case 1: *type = CudaMmaType::U8S8; return true;
    case 2: *type = CudaMmaType::S8U8; return true;
    case 3: *type = CudaMmaType::S8S8; return true;
    default: return false;
  }
}

CudaMmaEngine::CudaMmaEngine() {
  const auto error = cudaSetDevice(0);
  if (error != cudaSuccess)
    throw std::runtime_error(cudaGetErrorString(error));
  items.reserve(CONFIG_DIFF_MMA_BATCH_SIZE);
}

CudaMmaEngine::~CudaMmaEngine() = default;

bool CudaMmaEngine::register_storage(void *storage, size_t bytes, uint8_t *const operands[3]) {
  const auto error = cudaHostRegister(storage, bytes, cudaHostRegisterDefault);
  if (error != cudaSuccess) {
    fprintf(stderr, "CudaMmaBackend: register client failed: %s\n", cudaGetErrorString(error));
    return false;
  }
  if (!workspace) {
    workspace.reset(cuda_mma_backend_create(host, CONFIG_DIFF_MMA_BATCH_SIZE, CONFIG_DIFF_AMU_AB_REG_SIZE_BYTES,
                                            CONFIG_DIFF_AMU_C_REG_SIZE_BYTES, operands));
    if (!workspace) {
      cudaHostUnregister(storage);
      return false;
    }
  }
  return true;
}

void CudaMmaEngine::unregister_storage(void *storage) {
  cudaHostUnregister(storage);
}

void CudaMmaEngine::compute(const std::vector<MmaVerificationBuffer *> &buffers, uint8_t *passed,
                            uint8_t *const operands[3], const Completion &completed) {
  items.clear();
  for (const auto *buffer: buffers) {
    const auto &event = buffer->amu_event;
    CudaMmaBatchItem item;
    item.tile_m = event.mtilem;
    item.tile_k = event.mtilek;
    item.tile_n = event.mtilen;
    item.sat = event.sat;
    item.src1 = buffer->src1;
    item.src2 = buffer->src2;
    item.src3 = buffer->src3;
    item.dut = reinterpret_cast<const uint8_t *>(buffer->dut_result.get());
    items.push_back(item);
  }
  for (size_t operand = 0; operand < 3; ++operand)
    workspace->operands[operand].shared_host = operands[operand];
  CudaMmaType type;
  const bool success = select_cuda_mma_type(buffers[0]->amu_event, &type);
  if (success && (!cuda_mma_backend_enqueue_references(workspace.get(), type, items.data(), items.size()) ||
                  cudaGetLastError() != cudaSuccess))
    throw std::runtime_error("CUDA service computation failed");
  if (!success)
    memset(passed, 0, buffers.size());
  if (success && !cuda_mma_backend_enqueue_checks(workspace.get(), items.data(), items.size(), passed))
    throw std::runtime_error("CUDA service comparison failed");
  for (size_t begin = 0; begin < buffers.size(); begin += kMmaCompletionSize) {
    const size_t end = std::min(begin + kMmaCompletionSize, buffers.size());
    if (success && !(buffers.size() <= kMmaCompletionSize ? workspace->synchronize() : workspace->wait(begin, end)))
      throw std::runtime_error("CUDA service computation failed");
    for (size_t i = begin; success && i < end; ++i)
      if (!passed[i] && !cuda_mma_backend_read_result(workspace.get(), i, items[i].src3))
        throw std::runtime_error("CUDA service result readback failed");
    if (!completed(end)) {
      if (!workspace->synchronize())
        throw std::runtime_error("CUDA service computation failed");
      return;
    }
  }
}

void CudaMmaEngine::shutdown() {
  workspace.reset();
}
#endif // CONFIG_DIFFTEST_MMA_CUDA

#endif // CONFIG_DIFFTEST_AMUCTRLEVENT
