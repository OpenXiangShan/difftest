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

#ifndef __MMA_CUDA_FP32_H__
#define __MMA_CUDA_FP32_H__

#include "mma/backend/mma_backend_cute_model.h"

namespace cute_mma_cuda {

using namespace cute_mma_model;

// Remove the source significand scaling and reserve exponent code zero for zero products.
// The remaining significand, exponent and sign fit in one word, including TF32.
template <FloatFormat Format> struct ProductEncoding {
  enum {
    source_fraction_bits = Format == FloatFormat::Fp8E5M2   ? 2
                           : Format == FloatFormat::Fp8E4M3 ? 3
                           : Format == FloatFormat::Bf16    ? 7
                                                            : 10,
    source_exponent_bits = Format == FloatFormat::Fp8E4M3                                 ? 4
                           : (Format == FloatFormat::Bf16 || Format == FloatFormat::Tf32) ? 8
                                                                                          : 5,
    bytes = (Format == FloatFormat::Fp8E5M2 || Format == FloatFormat::Fp8E4M3) ? 1
            : Format == FloatFormat::Tf32                                      ? 4
                                                                               : 2,
    bias = (1 << (source_exponent_bits - 1)) - 1,
    product_bits = 2 * (source_fraction_bits + 1),
    exponent_bits = source_exponent_bits + 1,
    product_shift = 2 * (10 - source_fraction_bits),
    exponent_base = 510 + 2 * (1 - bias)
  };
};

template <FloatFormat Format> __device__ __forceinline__ uint32_t pack_product(const ProductTerm &product) {
  typedef ProductEncoding<Format> Bits;
  const uint32_t exponent = product.exponent_token ? product.exponent_token - Bits::exponent_base : 0;
  return (product.significand >> Bits::product_shift) | (exponent << Bits::product_bits) |
         (static_cast<uint32_t>(product.sign) << (Bits::product_bits + Bits::exponent_bits));
}

// Exceptions are collected on the first pass; the second needs only these three fields.
template <FloatFormat Format> __device__ __forceinline__ ProductTerm unpack_product(uint32_t packed) {
  typedef ProductEncoding<Format> Bits;
  ProductTerm product = {};
  const uint32_t exponent = (packed >> Bits::product_bits) & ((1u << Bits::exponent_bits) - 1);
  product.exponent_token = exponent ? exponent + Bits::exponent_base : 0;
  product.significand = (packed & ((1u << Bits::product_bits) - 1)) << Bits::product_shift;
  product.sign = (packed >> (Bits::product_bits + Bits::exponent_bits)) != 0;
  return product;
}

template <FloatFormat Format>
__device__ __forceinline__ ProductTerm make_product(const uint8_t *src1, const uint8_t *src2) {
  uint32_t a = load_little_endian(src1, source_bytes(Format));
  uint32_t b = load_little_endian(src2, source_bytes(Format));
  typedef ProductEncoding<Format> Bits;
  if (Format == FloatFormat::Tf32) {
    a >>= 13;
    b >>= 13;
  }
  const uint32_t sign_mask = 1u << (Bits::source_fraction_bits + Bits::source_exponent_bits);
  const uint32_t a_abs = a & (sign_mask - 1), b_abs = b & (sign_mask - 1);
  const uint32_t exp_mask = (1u << Bits::source_exponent_bits) - 1;
  const uint32_t a_exp = a_abs >> Bits::source_fraction_bits, b_exp = b_abs >> Bits::source_fraction_bits;
  const uint32_t fraction_mask = (1u << Bits::source_fraction_bits) - 1;
  const uint32_t a_sig = (a_abs & fraction_mask) | (a_exp ? 1u << Bits::source_fraction_bits : 0);
  const uint32_t b_sig = (b_abs & fraction_mask) | (b_exp ? 1u << Bits::source_fraction_bits : 0);
  ProductTerm product = {};
  product.sign = ((a ^ b) & sign_mask) != 0;
  product.exponent_token =
      (!a_abs || !b_abs) ? 0 : static_cast<int>((a_exp ? a_exp : 1) + (b_exp ? b_exp : 1)) + 511 - 2 * Bits::bias;
  product.significand = (a_sig * b_sig) << Bits::product_shift;
  if (Format == FloatFormat::Fp8E4M3) {
    product.is_nan = a_abs == 0x7f || b_abs == 0x7f;
  } else {
    const uint32_t infinity = exp_mask << Bits::source_fraction_bits;
    if (a_abs >= infinity || b_abs >= infinity) {
      const bool a_inf = a_abs == infinity, b_inf = b_abs == infinity;
      product.is_nan = a_abs > infinity || b_abs > infinity || (a_inf && !b_abs) || (b_inf && !a_abs);
      product.is_inf = (a_inf && b_abs) || (b_inf && a_abs);
    }
  }
  return product;
}

template <FloatFormat Format, int Width>
__device__ __forceinline__ uint32_t reduce_chunk(const uint8_t *a, const uint8_t *b, int valid_lanes, uint32_t c_bits) {
  const int elem_bytes = ProductEncoding<Format>::bytes;
  const int capacity = Width / elem_bytes;
  uint32_t products[capacity];
  // A fixed unrolled bound keeps the packed products in registers, including partial tiles.
  if (valid_lanes > capacity)
    valid_lanes = capacity;
  const RawFloat c = decode_fp32(c_bits);
  int max_exponent_token = c.exponent + 511;
  // Bit 0 is NaN; bits 1 and 2 record positive and negative infinity.
  uint32_t special = static_cast<uint32_t>(c.is_nan) | (c.is_inf ? (2u << c.sign) : 0);
#pragma unroll
  for (int lane = 0; lane < capacity; ++lane) {
    if (lane < valid_lanes) {
      const auto product = make_product<Format>(a + lane * elem_bytes, b + lane * elem_bytes);
      if (product.exponent_token > max_exponent_token)
        max_exponent_token = product.exponent_token;
      if (product.is_nan || product.is_inf)
        special |= static_cast<uint32_t>(product.is_nan) | (product.is_inf ? (2u << product.sign) : 0);
      products[lane] = pack_product<Format>(product);
    }
  }
  const int groups_per_half = Width / 8;
  const int group_count = uses_second_product_half(Format) ? groups_per_half * 2 : groups_per_half;
  int64_t first_half = 0, second_half = 0;
#pragma unroll
  for (int group = 0; group < group_count; ++group) {
    int64_t group_sum = 0;
#pragma unroll
    for (int offset = 0; offset < 4; ++offset) {
      const int lane = group * 4 + offset;
      int64_t aligned = 0;
      if (lane < valid_lanes && lane < capacity) {
        const auto product = unpack_product<Format>(products[lane]);
        aligned = align_product(product, max_exponent_token);
      }
      group_sum += aligned;
    }
    if (group < groups_per_half)
      first_half = fixed_add(first_half, group_sum, 32);
    else
      second_half = fixed_add(second_half, group_sum, 32);
  }
  first_half = fixed_add(first_half, align_c(c, max_exponent_token), 32);
  const uint32_t result =
      static_cast<uint32_t>(uses_second_product_half(Format) ? fixed_add(first_half, second_half, 32) : first_half);
  if ((special & 1) || (special & 6) == 6)
    return 0x7fc00000u;
  if (special & 2)
    return 0x7f800000u;
  if (special & 4)
    return 0xff800000u;
  return result ? normalize_fp32(result, max_exponent_token) : 0;
}

template <int Width>
__device__ __forceinline__ uint32_t reduce_chunk(FloatFormat format, const uint8_t *a, const uint8_t *b,
                                                 int valid_lanes, uint32_t c_bits) {
  switch (format) {
    case FloatFormat::Fp8E5M2: return reduce_chunk<FloatFormat::Fp8E5M2, Width>(a, b, valid_lanes, c_bits);
    case FloatFormat::Fp8E4M3: return reduce_chunk<FloatFormat::Fp8E4M3, Width>(a, b, valid_lanes, c_bits);
    case FloatFormat::Fp16: return reduce_chunk<FloatFormat::Fp16, Width>(a, b, valid_lanes, c_bits);
    case FloatFormat::Bf16: return reduce_chunk<FloatFormat::Bf16, Width>(a, b, valid_lanes, c_bits);
    case FloatFormat::Tf32: return reduce_chunk<FloatFormat::Tf32, Width>(a, b, valid_lanes, c_bits);
  }
  return reduce_fp32_chunk(format, a, b, valid_lanes, Width, c_bits);
}

} // namespace cute_mma_cuda

#endif
