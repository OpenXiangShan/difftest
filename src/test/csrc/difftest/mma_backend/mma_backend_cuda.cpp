/***************************************************************************************
* Copyright (c) 2020-2025 Institute of Computing Technology, Chinese Academy of Sciences
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

#include "mma_backend/mma_backend_cuda.h"

#ifdef CONFIG_DIFFTEST_AMUCTRLEVENT

#ifdef CONFIG_DIFFTEST_MMA_CUDA
#ifndef CONFIG_DIFFTEST_HAS_CUDA_TOOLCHAIN
#error "CONFIG_DIFFTEST_MMA_CUDA requires CUDA toolchain, but none was detected"
#endif
#endif

#include <cassert>

bool CudaMmaBackend::verify(MmaVerificationBuffer *buffer) {
  (void)buffer;
  assert(false && "CudaMmaBackend is not implemented");
  return false;
}

#endif // CONFIG_DIFFTEST_AMUCTRLEVENT
