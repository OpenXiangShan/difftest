#include <cstdint>
#include <cstring>
#include <mutex>

extern "C" {
#include <softfloat.h>
}

namespace {

constexpr uint16_t kDefaultNaNF16 = 0x7e00;
constexpr uint32_t kDefaultNaNF32 = 0x7fc00000;
constexpr uint64_t kDefaultNaNF64 = 0x7ff8000000000000ULL;

std::mutex &softfloat_mutex() {
  static std::mutex mutex;
  return mutex;
}

uint64_t read_bits(const uint32_t *bits, unsigned bit, unsigned width) {
  const unsigned word = bit / 32;
  const unsigned shift = bit % 32;
  uint64_t value = static_cast<uint64_t>(bits[word]) >> shift;
  if (shift + width > 32) {
    value |= static_cast<uint64_t>(bits[word + 1]) << (32 - shift);
  }
  if (width < 64) {
    value &= (uint64_t(1) << width) - 1;
  }
  return value;
}

void write_bits(uint32_t *bits, unsigned bit, unsigned width, uint64_t value) {
  const unsigned word = bit / 32;
  const unsigned shift = bit % 32;
  const uint64_t mask = width == 64 ? ~uint64_t(0) : ((uint64_t(1) << width) - 1);
  value &= mask;
  bits[word] |= static_cast<uint32_t>(value << shift);
  if (shift + width > 32) {
    bits[word + 1] |= static_cast<uint32_t>(value >> (32 - shift));
  }
}

bool is_nan_f16(uint16_t value) {
  return (value & 0x7c00) == 0x7c00 && (value & 0x03ff) != 0;
}

bool is_snan_f16(uint16_t value) {
  return is_nan_f16(value) && (value & 0x0200) == 0;
}

bool is_nan_f32(uint32_t value) {
  return (value & 0x7f800000) == 0x7f800000 && (value & 0x007fffff) != 0;
}

bool is_snan_f32(uint32_t value) {
  return is_nan_f32(value) && (value & 0x00400000) == 0;
}

bool is_nan_f64(uint64_t value) {
  return (value & 0x7ff0000000000000ULL) == 0x7ff0000000000000ULL &&
         (value & 0x000fffffffffffffULL) != 0;
}

bool is_snan_f64(uint64_t value) {
  return is_nan_f64(value) && (value & 0x0008000000000000ULL) == 0;
}

uint_fast8_t softfloat_rm(uint8_t rm) {
  // RISC-V's five standard rounding encodings match SoftFloat's encodings.
  return rm <= 4 ? rm : softfloat_round_near_even;
}

template <typename Fn>
uint64_t calculate(uint8_t format, uint64_t opa, uint64_t opb, bool is_sqrt,
                   bool opa_canonical_nan, bool opb_canonical_nan, Fn fn,
                   uint8_t &flags) {
  switch (format) {
  case 1: {
    const uint16_t a = static_cast<uint16_t>(opa);
    const uint16_t b = static_cast<uint16_t>(opb);
    const bool a_nan = opa_canonical_nan || is_nan_f16(a);
    const bool b_nan = !is_sqrt && (opb_canonical_nan || is_nan_f16(b));
    if (a_nan || b_nan) {
      flags = ((!opa_canonical_nan && is_snan_f16(a)) ||
               (!is_sqrt && !opb_canonical_nan && is_snan_f16(b)))
                  ? softfloat_flag_invalid
                  : 0;
      return kDefaultNaNF16;
    }
    float16_t fa{a};
    float16_t result = is_sqrt ? f16_sqrt(fa) : f16_div(fa, float16_t{b});
    flags = softfloat_exceptionFlags & 0x1f;
    return result.v;
  }
  case 2: {
    const uint32_t a = static_cast<uint32_t>(opa);
    const uint32_t b = static_cast<uint32_t>(opb);
    const bool a_nan = opa_canonical_nan || is_nan_f32(a);
    const bool b_nan = !is_sqrt && (opb_canonical_nan || is_nan_f32(b));
    if (a_nan || b_nan) {
      flags = ((!opa_canonical_nan && is_snan_f32(a)) ||
               (!is_sqrt && !opb_canonical_nan && is_snan_f32(b)))
                  ? softfloat_flag_invalid
                  : 0;
      return kDefaultNaNF32;
    }
    float32_t fa{a};
    float32_t result = is_sqrt ? f32_sqrt(fa) : f32_div(fa, float32_t{b});
    flags = softfloat_exceptionFlags & 0x1f;
    return result.v;
  }
  case 3: {
    const bool a_nan = is_nan_f64(opa);
    const bool b_nan = !is_sqrt && is_nan_f64(opb);
    if (a_nan || b_nan) {
      flags = (is_snan_f64(opa) || (!is_sqrt && is_snan_f64(opb))) ? softfloat_flag_invalid : 0;
      return kDefaultNaNF64;
    }
    float64_t fa{opa};
    float64_t result = is_sqrt ? f64_sqrt(fa) : f64_div(fa, float64_t{opb});
    flags = softfloat_exceptionFlags & 0x1f;
    return result.v;
  }
  default:
    flags = softfloat_flag_invalid;
    return kDefaultNaNF64;
  }
}

} // namespace

extern "C" void fdivsqrt_dpic(const uint32_t *in_bits, uint32_t *out_bits) {
  std::memset(out_bits, 0, 3 * sizeof(uint32_t));

  const bool valid = read_bits(in_bits, 128, 1) != 0;
  if (!valid) {
    return;
  }

  const uint64_t opa = read_bits(in_bits, 0, 64);
  const uint64_t opb = read_bits(in_bits, 64, 64);
  const uint8_t format = static_cast<uint8_t>(read_bits(in_bits, 136, 2));
  const uint8_t rm = static_cast<uint8_t>(read_bits(in_bits, 144, 3));
  const bool is_sqrt = read_bits(in_bits, 152, 1) != 0;
  const bool opa_canonical_nan = read_bits(in_bits, 160, 1) != 0;
  const bool opb_canonical_nan = read_bits(in_bits, 168, 1) != 0;

  std::lock_guard<std::mutex> lock(softfloat_mutex());
  softfloat_roundingMode = softfloat_rm(rm);
  softfloat_exceptionFlags = 0;
  uint8_t flags = 0;
  const uint64_t result = calculate(
      format, opa, opb, is_sqrt, opa_canonical_nan, opb_canonical_nan,
      [](auto) {}, flags);

  write_bits(out_bits, 0, 64, result);
  write_bits(out_bits, 64, 5, flags);
}
