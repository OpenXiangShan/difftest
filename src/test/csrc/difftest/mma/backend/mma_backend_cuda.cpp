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
#include <cstdio>
#include <cuda_runtime_api.h>

static bool report_cuda_error(const char *what, cudaError_t err) {
  if (err == cudaSuccess) {
    return true;
  }
  fprintf(stderr, "CudaMmaBackend: %s failed: %s\n", what, cudaGetErrorString(err));
  return false;
}

static bool select_cuda_mma_type(const DifftestAmuCtrlEvent &event, CudaMmaType *type) {
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

static bool select_cuda_device() {
  cudaError_t err = cudaSetDevice(0);
  return (err == cudaSuccess || err == cudaErrorSetOnActiveProcess) || report_cuda_error("cudaSetDevice", err);
}

bool CudaMmaBackend::is_batch_compatible(const MmaVerificationBuffer *first,
                                         const MmaVerificationBuffer *candidate) const {
  CudaMmaType first_type;
  CudaMmaType candidate_type;
  if (!first || !candidate || !select_cuda_mma_type(first->amu_event, &first_type) ||
      !select_cuda_mma_type(candidate->amu_event, &candidate_type)) {
    return false;
  }

  // Require exact operand/result encodings in addition to the selected kernel
  // variant. Matrix dimensions may differ because each batch item carries its
  // own geometry.
  return first_type == candidate_type && first->amu_event.isfp == candidate->amu_event.isfp &&
         first->amu_event.types1 == candidate->amu_event.types1 &&
         first->amu_event.types2 == candidate->amu_event.types2 && first->amu_event.typed == candidate->amu_event.typed;
}

void CudaMmaBackend::verify(const std::vector<MmaVerificationBuffer *> &buffers, std::vector<uint8_t> &passed) {
  passed.assign(buffers.size(), 0);
  if (buffers.empty()) {
    return;
  }

  CudaMmaType type;
  if (!select_cuda_mma_type(buffers.front()->amu_event, &type)) {
    const auto &event = buffers.front()->amu_event;
    fprintf(stderr, "CudaMmaBackend: unsupported MMA type pc=0x%lx isfp=%u types1=%u types2=%u typed=%u\n", event.pc,
            event.isfp, event.types1, event.types2, event.typed);
    return;
  }

  std::vector<CudaMmaBatchItem> items;
  items.reserve(buffers.size());
  for (auto *buffer: buffers) {
    CudaMmaBatchItem item;
    item.tile_m = buffer->amu_event.mtilem;
    item.tile_k = buffer->amu_event.mtilek;
    item.tile_n = buffer->amu_event.mtilen;
    item.types1 = buffer->amu_event.types1;
    item.types2 = buffer->amu_event.types2;
    item.src1 = buffer->src1;
    item.src2 = buffer->src2;
    item.src3 = buffer->src3;
    item.dut_result = buffer->dut_result;
    items.push_back(item);
  }

  if (!select_cuda_device()) {
    return;
  }
  if (!cuda_mma_backend_launch(type, items.data(), items.size(), passed.data())) {
    return;
  }
  if (!report_cuda_error("post-batch-launch", cudaGetLastError())) {
    passed.assign(buffers.size(), 0);
  }
}

#else

#include <cassert>

void CudaMmaBackend::verify(const std::vector<MmaVerificationBuffer *> &buffers, std::vector<uint8_t> &passed) {
  passed.assign(buffers.size(), 0);
  assert(false && "CudaMmaBackend is not implemented");
}

bool CudaMmaBackend::is_batch_compatible(const MmaVerificationBuffer *first,
                                         const MmaVerificationBuffer *candidate) const {
  (void)first;
  (void)candidate;
  return false;
}

#endif // CONFIG_DIFFTEST_MMA_CUDA

#endif // CONFIG_DIFFTEST_AMUCTRLEVENT
