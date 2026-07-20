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

bool CudaMmaBackend::verify(MmaVerificationBuffer *buffer) {
  CudaMmaType type;
  if (!select_cuda_mma_type(buffer->amu_event, &type)) {
    fprintf(stderr, "CudaMmaBackend: unsupported MMA type pc=0x%lx isfp=%u types1=%u types2=%u typed=%u\n",
            buffer->amu_event.pc, buffer->amu_event.isfp, buffer->amu_event.types1, buffer->amu_event.types2,
            buffer->amu_event.typed);
    return false;
  }

  cudaError_t err = cudaSetDevice(0);
  if (err != cudaSuccess && err != cudaErrorSetOnActiveProcess) {
    return report_cuda_error("cudaSetDevice", err);
  }

  bool passed = cuda_mma_backend_launch(type, buffer->amu_event.mtilem, buffer->amu_event.mtilek,
                                        buffer->amu_event.mtilen, buffer->amu_event.types1, buffer->amu_event.types2,
                                        buffer->src1, buffer->src2, buffer->src3, buffer->dut_result);
  return passed && report_cuda_error("post-launch", cudaGetLastError());
}

#else

#include <cassert>

bool CudaMmaBackend::verify(MmaVerificationBuffer *buffer) {
  (void)buffer;
  assert(false && "CudaMmaBackend is not implemented");
  return false;
}

#endif // CONFIG_DIFFTEST_MMA_CUDA

#endif // CONFIG_DIFFTEST_AMUCTRLEVENT
