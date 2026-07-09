/***************************************************************************************
* Copyright (c) 2020-2026 Institute of Computing Technology, Chinese Academy of Sciences
* Copyright (c) 2026 Beijing Institute of Open Source Chip
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

#ifndef __TOPDOWN_DPI_H__
#define __TOPDOWN_DPI_H__

#include <cstddef>
#include <cstdint>
#include <cstring>

#if defined(__has_include)
#if __has_include("svdpi.h")
#include "svdpi.h"
#else
typedef uint32_t svBitVecVal;
#endif
#else
#include "svdpi.h"
#endif

static inline void topdown_zero_bytes(svBitVecVal *data, size_t bytes) {
  const size_t words = (bytes + sizeof(svBitVecVal) - 1) / sizeof(svBitVecVal);
  std::memset(data, 0, words * sizeof(svBitVecVal));
}

#endif // __TOPDOWN_DPI_H__
