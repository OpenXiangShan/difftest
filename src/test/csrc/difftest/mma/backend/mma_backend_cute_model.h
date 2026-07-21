/***************************************************************************************
* Copyright (c) 2020-2026 Institute of Computing Technology, Chinese Academy of Sciences
* Copyright (c) 2026 Beijing Institute of Open Source Chip (BOSC)
*
* DiffTest is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND.
***************************************************************************************/

#ifndef __MMA_BACKEND_CUTE_MODEL_H__
#define __MMA_BACKEND_CUTE_MODEL_H__

#include <stdint.h>

#ifdef __CUDACC__
#define CUTE_MMA_HOST_DEVICE __host__ __device__
#else
#define CUTE_MMA_HOST_DEVICE
#endif

namespace cute_mma_model {

enum class FloatFormat : uint8_t {
  Fp8E5M2,
  Fp8E4M3,
  Fp16,
  Bf16,
  Tf32,
};

struct RawFloat {
  bool sign;
  int exponent;
  uint32_t significand;
  bool is_nan;
  bool is_inf;
  bool is_zero;
};

struct ProductTerm {
  bool sign;
  int exponent_token;
  uint32_t significand;
  bool is_nan;
  bool is_inf;
};

struct ExceptionFlags {
  bool has_nan;
  bool has_pos_inf;
  bool has_neg_inf;
};

CUTE_MMA_HOST_DEVICE static inline int source_bytes(FloatFormat format) {
  switch (format) {
    case FloatFormat::Fp8E5M2:
    case FloatFormat::Fp8E4M3: return 1;
    case FloatFormat::Fp16:
    case FloatFormat::Bf16: return 2;
    case FloatFormat::Tf32: return 4;
  }
  return 0;
}

CUTE_MMA_HOST_DEVICE static inline bool uses_second_product_half(FloatFormat format) {
  return format == FloatFormat::Fp8E5M2 || format == FloatFormat::Fp8E4M3;
}

CUTE_MMA_HOST_DEVICE static inline uint32_t load_little_endian(const uint8_t *data, int bytes) {
  uint32_t value = 0;
  for (int i = 0; i < bytes; ++i) {
    value |= static_cast<uint32_t>(data[i]) << (i * 8);
  }
  return value;
}

CUTE_MMA_HOST_DEVICE static inline RawFloat decode_ieee(uint32_t bits, int exp_bits, int mantissa_bits) {
  RawFloat result = {};
  const uint32_t exp_mask = (1u << exp_bits) - 1u;
  const uint32_t mantissa_mask = (1u << mantissa_bits) - 1u;
  const uint32_t raw_exp = (bits >> mantissa_bits) & exp_mask;
  const uint32_t raw_mantissa = bits & mantissa_mask;
  const int bias = (1 << (exp_bits - 1)) - 1;

  result.sign = ((bits >> (exp_bits + mantissa_bits)) & 1u) != 0;
  result.exponent = static_cast<int>(raw_exp == 0 ? 1 : raw_exp) - bias;
  result.significand = (raw_exp == 0 ? 0u : (1u << mantissa_bits)) | raw_mantissa;
  result.is_nan = raw_exp == exp_mask && raw_mantissa != 0;
  result.is_inf = raw_exp == exp_mask && raw_mantissa == 0;
  result.is_zero = raw_exp == 0 && raw_mantissa == 0;
  return result;
}

CUTE_MMA_HOST_DEVICE static inline RawFloat decode_source(uint32_t bits, FloatFormat format) {
  RawFloat result = {};
  switch (format) {
    case FloatFormat::Fp8E5M2:
      result = decode_ieee(bits, 5, 2);
      result.significand <<= 8;
      break;
    case FloatFormat::Fp8E4M3:
      result = decode_ieee(bits, 4, 3);
      result.is_nan = (bits & 0x7fu) == 0x7fu;
      result.is_inf = false;
      result.significand <<= 7;
      break;
    case FloatFormat::Fp16: result = decode_ieee(bits, 5, 10); break;
    case FloatFormat::Bf16:
      result = decode_ieee(bits, 8, 7);
      result.significand <<= 3;
      break;
    case FloatFormat::Tf32: result = decode_ieee(bits >> 13, 8, 10); break;
  }
  return result;
}

CUTE_MMA_HOST_DEVICE static inline RawFloat decode_fp32(uint32_t bits) {
  return decode_ieee(bits, 8, 23);
}

CUTE_MMA_HOST_DEVICE static inline ProductTerm multiply(const RawFloat &a, const RawFloat &b) {
  ProductTerm result = {};
  result.sign = a.sign != b.sign;
  result.exponent_token = (a.is_zero || b.is_zero) ? 0 : a.exponent + b.exponent + 511;
  result.significand = a.significand * b.significand;
  result.is_nan = a.is_nan || b.is_nan || (a.is_inf && b.is_zero) || (b.is_inf && a.is_zero);
  result.is_inf = (!a.is_zero && b.is_inf) || (!b.is_zero && a.is_inf);
  return result;
}

CUTE_MMA_HOST_DEVICE static inline void update_exceptions(ExceptionFlags *flags, const ProductTerm &term) {
  flags->has_nan = flags->has_nan || term.is_nan;
  flags->has_pos_inf = flags->has_pos_inf || (term.is_inf && !term.sign);
  flags->has_neg_inf = flags->has_neg_inf || (term.is_inf && term.sign);
}

CUTE_MMA_HOST_DEVICE static inline int64_t wrap_signed(int64_t value, int width) {
  const uint64_t mask = (1ull << width) - 1ull;
  const uint64_t sign_bit = 1ull << (width - 1);
  const uint64_t bits = static_cast<uint64_t>(value) & mask;
  if ((bits & sign_bit) == 0) {
    return static_cast<int64_t>(bits);
  }
  return -static_cast<int64_t>(((~bits) & mask) + 1ull);
}

CUTE_MMA_HOST_DEVICE static inline int64_t fixed_add(int64_t lhs, int64_t rhs, int width) {
  return wrap_signed(lhs + rhs, width);
}

CUTE_MMA_HOST_DEVICE static inline int32_t decode_int8(uint8_t bits, bool is_signed) {
  return is_signed && (bits & 0x80u) != 0 ? static_cast<int32_t>(bits) - 256 : static_cast<int32_t>(bits);
}

// Functionally models the fixed-width integer reduction tree for one CUTE ReducePE input vector.
CUTE_MMA_HOST_DEVICE static inline uint32_t reduce_int32_chunk(bool src1_signed, bool src2_signed, const uint8_t *src1,
                                                               const uint8_t *src2, int valid_lanes,
                                                               int reduce_width_bytes, uint32_t c_bits) {
  const int lane_capacity = reduce_width_bytes;
  if (valid_lanes > lane_capacity) {
    valid_lanes = lane_capacity;
  }

  const int groups_per_half = reduce_width_bytes / 8;
  int64_t first_half = 0;
  int64_t second_half = 0;

  for (int group = 0; group < groups_per_half * 2; ++group) {
    int64_t group_sum = 0;
    for (int offset = 0; offset < 4; ++offset) {
      const int lane = group * 4 + offset;
      int64_t product = 0;
      if (lane < valid_lanes) {
        const int32_t a = decode_int8(src1[lane], src1_signed);
        const int32_t b = decode_int8(src2[lane], src2_signed);
        product = static_cast<int64_t>(a) * b;
      }
      group_sum = fixed_add(group_sum, product, 28);
    }

    if (group < groups_per_half) {
      first_half = fixed_add(first_half, group_sum, 32);
    } else {
      second_half = fixed_add(second_half, group_sum, 32);
    }
  }

  first_half = fixed_add(first_half, wrap_signed(c_bits, 32), 32);
  return static_cast<uint32_t>(fixed_add(first_half, second_half, 32));
}

CUTE_MMA_HOST_DEVICE static inline uint64_t shift_right(uint64_t value, int amount) {
  return amount >= 64 ? 0 : value >> amount;
}

CUTE_MMA_HOST_DEVICE static inline int64_t align_product(const ProductTerm &term, int max_exponent_token) {
  const int shift = max_exponent_token - term.exponent_token;
  const uint64_t magnitude = shift_right(static_cast<uint64_t>(term.significand) << 3, shift);
  return term.sign ? -static_cast<int64_t>(magnitude) : static_cast<int64_t>(magnitude);
}

CUTE_MMA_HOST_DEVICE static inline int64_t align_c(const RawFloat &c, int max_exponent_token) {
  const int shift = max_exponent_token - (c.exponent + 511);
  const uint64_t magnitude = shift_right(c.significand, shift);
  return c.sign ? -static_cast<int64_t>(magnitude) : static_cast<int64_t>(magnitude);
}

CUTE_MMA_HOST_DEVICE static inline int leading_zeros_32(uint32_t value) {
  int count = 0;
  for (int bit = 31; bit >= 0; --bit) {
    if (((value >> bit) & 1u) != 0) {
      break;
    }
    ++count;
  }
  return count;
}

CUTE_MMA_HOST_DEVICE static inline uint32_t normalize_fp32(uint32_t reduce_bits, int max_exponent_token) {
  const bool sign = (reduce_bits & 0x80000000u) != 0;
  const uint32_t unsigned_sig = sign ? (~reduce_bits + 1u) : reduce_bits;
  const int leading_zeros = leading_zeros_32(unsigned_sig);
  const int shift_exp = max_exponent_token - 511 - leading_zeros + 8;
  const uint32_t shift_mantissa =
      leading_zeros > 8 ? unsigned_sig << (leading_zeros - 8) : unsigned_sig >> (8 - leading_zeros);

  uint32_t result_exp;
  if (shift_exp < -126) {
    result_exp = 0;
  } else if (shift_exp > 127) {
    result_exp = 255;
  } else {
    result_exp = static_cast<uint32_t>(shift_exp + 127);
  }

  uint32_t result_sig;
  if (shift_exp < -149) {
    result_sig = 0;
  } else if (shift_exp < -126) {
    result_sig = (shift_mantissa & 0x00ffffffu) >> (-126 - shift_exp);
  } else if (shift_exp > 127) {
    result_sig = 0;
  } else {
    result_sig = shift_mantissa & 0x00ffffffu;
  }

  return (static_cast<uint32_t>(sign) << 31) | (result_exp << 23) | (result_sig & 0x007fffffu);
}

// Functionally models one CUTE ReducePE input vector. Missing tail lanes are +0.
CUTE_MMA_HOST_DEVICE static inline uint32_t reduce_fp32_chunk(FloatFormat format, const uint8_t *src1,
                                                              const uint8_t *src2, int valid_lanes,
                                                              int reduce_width_bytes, uint32_t c_bits) {
  const int elem_bytes = source_bytes(format);
  const int lane_capacity = reduce_width_bytes / elem_bytes;
  if (valid_lanes > lane_capacity) {
    valid_lanes = lane_capacity;
  }

  const RawFloat c = decode_fp32(c_bits);
  int max_exponent_token = c.exponent + 511;
  ExceptionFlags exceptions = {c.is_nan, c.is_inf && !c.sign, c.is_inf && c.sign};

  for (int lane = 0; lane < valid_lanes; ++lane) {
    const RawFloat a = decode_source(load_little_endian(src1 + lane * elem_bytes, elem_bytes), format);
    const RawFloat b = decode_source(load_little_endian(src2 + lane * elem_bytes, elem_bytes), format);
    const ProductTerm product = multiply(a, b);
    if (product.exponent_token > max_exponent_token) {
      max_exponent_token = product.exponent_token;
    }
    update_exceptions(&exceptions, product);
  }

  const int groups_per_half = reduce_width_bytes / 8;
  const int group_count = uses_second_product_half(format) ? groups_per_half * 2 : groups_per_half;
  int64_t first_half = 0;
  int64_t second_half = 0;

  for (int group = 0; group < group_count; ++group) {
    int64_t group_sum = 0;
    for (int offset = 0; offset < 4; ++offset) {
      const int lane = group * 4 + offset;
      int64_t aligned = 0;
      if (lane < valid_lanes) {
        const RawFloat a = decode_source(load_little_endian(src1 + lane * elem_bytes, elem_bytes), format);
        const RawFloat b = decode_source(load_little_endian(src2 + lane * elem_bytes, elem_bytes), format);
        aligned = align_product(multiply(a, b), max_exponent_token);
      }
      group_sum = fixed_add(group_sum, aligned, 28);
    }

    if (group < groups_per_half) {
      first_half = fixed_add(first_half, group_sum, 32);
    } else {
      second_half = fixed_add(second_half, group_sum, 32);
    }
  }

  first_half = fixed_add(first_half, align_c(c, max_exponent_token), 32);
  const int64_t reduce_result = uses_second_product_half(format) ? fixed_add(first_half, second_half, 32) : first_half;
  const uint32_t reduce_bits = static_cast<uint32_t>(reduce_result);

  if (exceptions.has_nan || (exceptions.has_pos_inf && exceptions.has_neg_inf)) {
    return 0x7fc00000u;
  }
  if (exceptions.has_pos_inf) {
    return 0x7f800000u;
  }
  if (exceptions.has_neg_inf) {
    return 0xff800000u;
  }
  if (reduce_bits == 0) {
    return 0;
  }
  return normalize_fp32(reduce_bits, max_exponent_token);
}

} // namespace cute_mma_model

#undef CUTE_MMA_HOST_DEVICE

#endif // __MMA_BACKEND_CUTE_MODEL_H__
