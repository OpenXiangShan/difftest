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
#include "mma/backend/mma_backend_cuda_process.h"
#include "mma/mma_verifier.h"

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

CudaMmaBackend::CudaMmaBackend() : process(new CudaMmaProcess(host)) {}

CudaMmaBackend::~CudaMmaBackend() = default;

void CudaMmaBackend::start() {
  process->prepare();
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
  process->verify(buffers, passed);
}

#else

#include <cassert>

CudaMmaBackend::CudaMmaBackend() = default;

CudaMmaBackend::~CudaMmaBackend() = default;

void CudaMmaBackend::start() {}

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
