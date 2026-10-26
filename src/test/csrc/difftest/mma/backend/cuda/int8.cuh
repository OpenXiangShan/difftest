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

#ifndef __MMA_CUDA_INT8_CUH__
#define __MMA_CUDA_INT8_CUH__

#include "mma/backend/mma_backend_cuda_impl.h"
#include <stdint.h>

#if defined(__CUDA_ARCH__) && __CUDA_ARCH__ >= 800

__device__ __forceinline__ uint32_t load_int8_fragment(const uint8_t *data, int row, int rows, int k, int stride) {
  uint32_t value = 0;
#pragma unroll
  for (int byte = 0; byte < 4; ++byte) {
    if (row < rows && k + byte < stride) {
      value |= uint32_t(data[size_t(row) * stride + k + byte]) << (byte * 8);
    }
  }
  return value;
}

template <bool SignedA, bool SignedB>
__device__ __forceinline__ void mma_int8(uint32_t (&c)[4], const uint32_t (&a)[4], const uint32_t (&b)[2]) {
// Half dots start from zero; the caller applies the selected INT32 overflow mode.
#define MMA_INT8(A, B)                                                           \
  asm volatile("mma.sync.aligned.m16n8k32.row.col.s32." A "." B                  \
               ".s32 "                                                           \
               "{%0, %1, %2, %3}, {%4, %5, %6, %7}, {%8, %9}, {%0, %1, %2, %3};" \
               : "+r"(c[0]), "+r"(c[1]), "+r"(c[2]), "+r"(c[3])                  \
               : "r"(a[0]), "r"(a[1]), "r"(a[2]), "r"(a[3]), "r"(b[0]), "r"(b[1]))
  if (SignedA && SignedB) {
    MMA_INT8("s8", "s8");
  } else if (SignedA) {
    MMA_INT8("s8", "u8");
  } else if (SignedB) {
    MMA_INT8("u8", "s8");
  } else {
    MMA_INT8("u8", "u8");
  }
#undef MMA_INT8
}

template <bool SignedA, bool SignedB, int ReduceWidthBytes>
__device__ __forceinline__ void mmacc_int8_tensor(const CudaMmaBatchDescriptor &desc, const uint8_t *src1,
                                                  const uint8_t *src2, uint8_t *src3) {
  const int lane = threadIdx.x % 32;
  const int group = lane / 4;
  const int thread = lane % 4;
  const int n_tiles = (desc.tile_n + 7) / 8;
  const int tiles = ((desc.tile_m + 15) / 16) * n_tiles;
  const uint8_t *a_data = src1 + desc.src1_offset;
  const uint8_t *b_data = src2 + desc.src2_offset;
  uint32_t *c_data = reinterpret_cast<uint32_t *>(src3 + desc.src3_offset);

  for (int tile = blockIdx.x * (blockDim.x / 32) + threadIdx.x / 32; tile < tiles;
       tile += gridDim.x * (blockDim.x / 32)) {
    const int row = (tile / n_tiles) * 16 + group;
    const int col = (tile % n_tiles) * 8;
    uint32_t c[4] = {};
#pragma unroll
    for (int i = 0; i < 4; ++i) {
      const int r = row + (i / 2) * 8;
      const int n = col + thread * 2 + i % 2;
      if (r < desc.tile_m && n < desc.tile_n) {
        c[i] = c_data[r * desc.tile_n + n];
      }
    }

    // Add the low half to C, then the high half, matching Pipe3 and Pipe4.
    for (int base = 0; base < desc.tile_k; base += ReduceWidthBytes / 2) {
      const int k = base + thread * 4;
      const uint32_t a[4] = {
        load_int8_fragment(a_data, row, desc.tile_m, k, desc.tile_k),
        load_int8_fragment(a_data, row + 8, desc.tile_m, k, desc.tile_k),
        ReduceWidthBytes == 64 ? load_int8_fragment(a_data, row, desc.tile_m, k + 16, desc.tile_k) : 0,
        ReduceWidthBytes == 64 ? load_int8_fragment(a_data, row + 8, desc.tile_m, k + 16, desc.tile_k) : 0};
      // src2 already stores each output column's K elements contiguously.
      const uint32_t b[2] = {
        load_int8_fragment(b_data, col + group, desc.tile_n, k, desc.tile_k),
        ReduceWidthBytes == 64 ? load_int8_fragment(b_data, col + group, desc.tile_n, k + 16, desc.tile_k) : 0};
      // A pure half dot is bounded by 32 * 255 * 255, so its internal
      // reduction order cannot overflow or affect the exact signed32 sum.
      uint32_t partial[4] = {};
      mma_int8<SignedA, SignedB>(partial, a, b);
#pragma unroll
      for (int i = 0; i < 4; ++i) {
        if (desc.sat) {
          asm volatile("add.sat.s32 %0, %0, %1;" : "+r"(c[i]) : "r"(partial[i]));
        } else {
          c[i] += partial[i];
        }
      }
    }

#pragma unroll
    for (int i = 0; i < 4; ++i) {
      const int r = row + (i / 2) * 8;
      const int n = col + thread * 2 + i % 2;
      if (r < desc.tile_m && n < desc.tile_n) {
        c_data[r * desc.tile_n + n] = c[i];
      }
    }
  }
}

#endif
#endif
