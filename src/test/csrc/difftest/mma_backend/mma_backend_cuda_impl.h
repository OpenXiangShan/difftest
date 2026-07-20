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

#ifndef __MMA_BACKEND_CUDA_IMPL_H__
#define __MMA_BACKEND_CUDA_IMPL_H__

#include "mma_backend/mma_backend.h"
#include <cstdint>

enum class CudaMmaType : uint8_t {
  U8U8 = 0,
  U8S8 = 1,
  S8U8 = 2,
  S8S8 = 3,
  Fp8E5M2ToFp32 = 5,
  Fp8E4M3ToFp32 = 8,
  Fp16ToFp32 = 11,
  Bf16ToFp32 = 12,
  Tf32ToFp32 = 13,
};

extern "C" bool cuda_mma_backend_launch(CudaMmaType type, uint16_t tile_m, uint16_t tile_k, uint16_t tile_n,
                                        uint8_t types1, uint8_t types2, const uint8_t *src1, const uint8_t *src2,
                                        uint8_t *src3, const uint8_t *dut_result);

#endif // __MMA_BACKEND_CUDA_IMPL_H__
