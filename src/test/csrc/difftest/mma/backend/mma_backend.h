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

#ifndef __MMA_BACKEND_H__
#define __MMA_BACKEND_H__

#include "common.h"

#ifdef CONFIG_DIFFTEST_AMUCTRLEVENT

struct MmaVerificationBuffer;

enum MmaElementType : uint8_t {
  Fp8E5M2 = 0,
  Fp16 = 1,
  Fp32 = 2,
  Fp8E4M3 = 4,
  Bf16 = 5,
  Tf32 = 6,
};

static inline bool is_mma_32bit_result_type(uint8_t type) {
  return (type & 3) == 2;
}

class MmaBackend {
public:
  virtual ~MmaBackend() = default;
  virtual bool verify(MmaVerificationBuffer *buffer) = 0;
};

#endif // CONFIG_DIFFTEST_AMUCTRLEVENT

#endif // __MMA_BACKEND_H__
