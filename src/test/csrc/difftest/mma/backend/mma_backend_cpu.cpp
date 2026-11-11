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

#include "mma/backend/mma_backend_cpu.h"
#include "mma/backend/mma_backend_cute_model.h"

#ifdef CONFIG_DIFFTEST_AMUCTRLEVENT

#include "mma/mma_verifier.h"
#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <cstring>
#include <queue>
#include <thread>
#include <type_traits>

class CpuMmaThreadPool {
public:
  explicit CpuMmaThreadPool(unsigned limit) : limit(limit) {
    try {
      for (unsigned i = 0; i < limit; ++i)
        workers.emplace_back([this] {
          while (true) {
            std::function<void()> job;
            {
              std::unique_lock<std::mutex> lock(mutex);
              work.wait(lock, [this] { return stopping || !jobs.empty(); });
              if (jobs.empty())
                return;
              job = std::move(jobs.front());
              jobs.pop();
            }
            job();
            {
              std::lock_guard<std::mutex> lock(mutex);
              --pending;
            }
            available.notify_one();
          }
        });
    } catch (...) {
      join();
      throw;
    }
  }
  ~CpuMmaThreadPool() {
    join();
  }
  void submit(std::function<void()> job) {
    std::unique_lock<std::mutex> lock(mutex);
    available.wait(lock, [this] { return pending < limit; });
    jobs.push(std::move(job));
    ++pending;
    work.notify_one();
  }

private:
  void join() {
    {
      std::lock_guard<std::mutex> lock(mutex);
      stopping = true;
    }
    work.notify_all();
    for (auto &worker: workers)
      worker.join();
  }
  const unsigned limit;
  unsigned pending = 0;
  bool stopping = false;
  std::mutex mutex;
  std::condition_variable work, available;
  std::queue<std::function<void()>> jobs;
  std::vector<std::thread> workers;
};
CpuMmaEngine::CpuMmaEngine(unsigned threads, CpuMmaGranularity granularity)
    : threads(threads), granularity(granularity), pool(new CpuMmaThreadPool(threads)) {}

CpuMmaEngine::~CpuMmaEngine() = default;

bool CpuMmaEngine::verify_output_rows(MmaVerificationBuffer *buffer) {
  const size_t rows = buffer->amu_event.mtilem;
  if (rows == 0)
    return verify_cpu_mma(buffer);
  const size_t tasks = std::min<size_t>(threads, rows);
  std::vector<uint8_t> passed(tasks);
  std::mutex mutex;
  std::condition_variable completed;
  size_t remaining = tasks;
  size_t submitted = 0;
  try {
    for (; submitted < tasks; ++submitted) {
      const size_t part = submitted;
      const size_t begin = rows * part / tasks;
      const size_t end = rows * (part + 1) / tasks;
      pool->submit([&, part, begin, end] {
        // Borrow disjoint output rows; each keeps its complete original K reduction.
        MmaVerificationBuffer view{};
        view.amu_event = buffer->amu_event;
        view.amu_event.mtilem = end - begin;
        view.src1 = buffer->src1 + begin * kMmaSourceRowBytes;
        view.src2 = buffer->src2;
        view.src3 = buffer->src3 + begin * kMmaResultRowBytes;
        auto *dut = reinterpret_cast<uint8_t *>(buffer->dut_result.get()) + begin * kMmaResultRowBytes;
        MatrixResultDeleter deleter;
        deleter.shared_result = std::shared_ptr<uint8_t>(std::shared_ptr<uint8_t>(), dut);
        view.dut_result = MatrixResult(reinterpret_cast<uint64_t *>(dut), std::move(deleter));
        passed[part] = verify_cpu_mma(&view);
        std::lock_guard<std::mutex> lock(mutex);
        --remaining;
        completed.notify_one();
      });
    }
  } catch (...) {
    std::unique_lock<std::mutex> lock(mutex);
    remaining -= tasks - submitted;
    completed.wait(lock, [&] { return remaining == 0; });
    throw;
  }
  std::unique_lock<std::mutex> lock(mutex);
  completed.wait(lock, [&] { return remaining == 0; });
  return std::all_of(passed.begin(), passed.end(), [](uint8_t value) { return value != 0; });
}

void CpuMmaEngine::compute(const std::vector<MmaVerificationBuffer *> &buffers, uint8_t *passed,
                           uint8_t *const operands[3], const Completion &completed) {
  if (granularity == CpuMmaGranularity::OutputRows) {
    for (size_t i = 0; i < buffers.size(); ++i)
      passed[i] = verify_output_rows(buffers[i]);
    completed(buffers.size());
    return;
  }
  struct Pending {
    std::atomic<size_t> remaining;
    const size_t count;
    Completion completed;
    Pending(size_t count, const Completion &completed) : remaining(count), count(count), completed(completed) {}
  };
  const auto pending = std::make_shared<Pending>(buffers.size(), completed);
  for (size_t i = 0; i < buffers.size(); ++i) {
    auto *buffer = buffers[i];
    pool->submit([buffer, passed, i, pending] {
      passed[i] = verify_cpu_mma(buffer);
      if (pending->remaining.fetch_sub(1, std::memory_order_acq_rel) == 1)
        pending->completed(pending->count);
    });
  }
}

void CpuMmaEngine::shutdown() {
  pool.reset();
}

#ifdef CONFIG_DIFF_MMA_REDUCE_WIDTH_BYTES
static constexpr int kMmaReduceWidthBytes = CONFIG_DIFF_MMA_REDUCE_WIDTH_BYTES;
#else
static constexpr int kMmaReduceWidthBytes = 32;
#endif

static uint32_t reduce_int32_chunk_cpu(bool src1_signed, bool src2_signed, const uint8_t *src1, const uint8_t *src2,
                                       int valid_lanes, uint32_t c_bits, bool saturate) {
  using namespace cute_mma_model;
  int32_t products[kMmaReduceWidthBytes] = {};
  // Independent products can vectorize without changing the reduction order.
  for (int lane = 0; lane < valid_lanes; ++lane) {
    products[lane] = decode_int8(src1[lane], src1_signed) * decode_int8(src2[lane], src2_signed);
  }

  const int groups_per_half = kMmaReduceWidthBytes / 8;
  int64_t first_half = 0;
  int64_t second_half = 0;
  for (int group = 0; group < groups_per_half * 2; ++group) {
    int64_t group_sum = 0;
    for (int offset = 0; offset < 4; ++offset)
      group_sum += products[group * 4 + offset];
    if (group < groups_per_half)
      first_half = fixed_add(first_half, group_sum, 32);
    else
      second_half = fixed_add(second_half, group_sum, 32);
  }
  first_half = add_int32(first_half, wrap_signed(c_bits, 32), saturate);
  return static_cast<uint32_t>(add_int32(first_half, second_half, saturate));
}

static constexpr int kFpOutputBatch = 8;

struct FpChunkBatch {
  uint32_t reduce_bits[kFpOutputBatch];
  int max_exponent_tokens[kFpOutputBatch];
  uint32_t exception_masks[kFpOutputBatch];
};

struct FpPartialBatch {
  int64_t group_sums[kMmaReduceWidthBytes / 4][kFpOutputBatch];
  int max_exponent_tokens[kFpOutputBatch];
  uint32_t exception_masks[kFpOutputBatch];
  int c_exponent_tokens[kFpOutputBatch];
  uint32_t c_significands[kFpOutputBatch];
  uint32_t c_signs[kFpOutputBatch];
};

static void initialize_fp32_chunks_cpu(const uint32_t *c_bits, FpPartialBatch &batch) {
  uint32_t bits[kFpOutputBatch];
  memcpy(bits, c_bits, sizeof(bits));
  for (int output = 0; output < kFpOutputBatch; ++output) {
    const cute_mma_model::RawFloat c = cute_mma_model::decode_fp32(bits[output]);
    batch.max_exponent_tokens[output] = c.exponent + 511;
    batch.exception_masks[output] = static_cast<uint32_t>(c.is_nan) |
                                    (static_cast<uint32_t>(c.is_inf && !c.sign) << 1) |
                                    (static_cast<uint32_t>(c.is_inf && c.sign) << 2);
    batch.c_exponent_tokens[output] = c.exponent + 511;
    batch.c_significands[output] = c.significand;
    batch.c_signs[output] = c.sign;
  }
}

// Keep GCC from cloning this helper for each constant format at its call sites.
#if defined(__GNUC__) && !defined(__clang__)
__attribute__((noipa))
#endif
static void
prepare_fp32_chunk_cpu(cute_mma_model::FloatFormat format, const uint8_t *src1, const uint8_t *src2, int valid_lanes,
                       FpPartialBatch &batch, int output) {
  using namespace cute_mma_model;
  const int elem_bytes = source_bytes(format);
  const int lane_capacity = kMmaReduceWidthBytes / elem_bytes;
  if (valid_lanes > lane_capacity) {
    valid_lanes = lane_capacity;
  }

  int max_exponent_token = batch.max_exponent_tokens[output];
  uint32_t exception_mask = batch.exception_masks[output];
  int first_a_exponents[kMmaReduceWidthBytes];
  int first_b_exponents[kMmaReduceWidthBytes];
  uint32_t a_significands[kMmaReduceWidthBytes];
  uint32_t b_significands[kMmaReduceWidthBytes];
  // Cache decoded fields for metadata and alignment; bits 0..3 are sign, zero, NaN and Inf.
  uint32_t first_a_flags[kMmaReduceWidthBytes];
  uint32_t first_b_flags[kMmaReduceWidthBytes];
  for (int lane = 0; lane < valid_lanes; ++lane) {
    const RawFloat a = decode_source(load_little_endian(src1 + lane * elem_bytes, elem_bytes), format);
    const RawFloat b = decode_source(load_little_endian(src2 + lane * elem_bytes, elem_bytes), format);
    first_a_exponents[lane] = a.exponent;
    first_b_exponents[lane] = b.exponent;
    a_significands[lane] = a.significand;
    b_significands[lane] = b.significand;
    first_a_flags[lane] = static_cast<uint32_t>(a.sign) | (static_cast<uint32_t>(a.is_zero) << 1) |
                          (static_cast<uint32_t>(a.is_nan) << 2) | (static_cast<uint32_t>(a.is_inf) << 3);
    first_b_flags[lane] = static_cast<uint32_t>(b.sign) | (static_cast<uint32_t>(b.is_zero) << 1) |
                          (static_cast<uint32_t>(b.is_nan) << 2) | (static_cast<uint32_t>(b.is_inf) << 3);
  }
  // Match multiply()'s metadata, independently of decoding and MMA accumulation.
  for (int lane = 0; lane < valid_lanes; ++lane) {
    const uint32_t a = first_a_flags[lane];
    const uint32_t b = first_b_flags[lane];
    const uint32_t sign = (a ^ b) & 1u;
    const bool zero = ((a | b) & 2u) != 0;
    const int exponent = zero ? 0 : first_a_exponents[lane] + first_b_exponents[lane] + 511;
    const uint32_t a_zero = (a >> 1) & 1u;
    const uint32_t b_zero = (b >> 1) & 1u;
    const uint32_t a_inf = (a >> 3) & 1u;
    const uint32_t b_inf = (b >> 3) & 1u;
    const uint32_t is_nan = (((a | b) >> 2) & 1u) | (a_inf & b_zero) | (b_inf & a_zero);
    const uint32_t is_inf = ((a_zero ^ 1u) & b_inf) | ((b_zero ^ 1u) & a_inf);
    if (exponent > max_exponent_token) {
      max_exponent_token = exponent;
    }
    exception_mask |= is_nan | ((is_inf & (sign ^ 1u)) << 1) | ((is_inf & sign) << 2);
  }
  int64_t aligned[kMmaReduceWidthBytes] = {};
  for (int lane = 0; lane < valid_lanes; ++lane) {
    // Match multiply()'s integer fields; exceptions were collected in the first pass.
    const bool sign = ((first_a_flags[lane] ^ first_b_flags[lane]) & 1u) != 0;
    const bool zero = ((first_a_flags[lane] | first_b_flags[lane]) & 2u) != 0;
    const ProductTerm product = {sign, zero ? 0 : first_a_exponents[lane] + first_b_exponents[lane] + 511,
                                 a_significands[lane] * b_significands[lane], false, false};
    aligned[lane] = align_product(product, max_exponent_token);
  }

  const int groups_per_half = kMmaReduceWidthBytes / 8;
  const int group_count = uses_second_product_half(format) ? groups_per_half * 2 : groups_per_half;
  // Four aligned magnitudes sum to at most 4 * 2047 * 2047 * 8, which fits in 28 signed bits.
  for (int group = 0; group < group_count; ++group) {
    int64_t group_sum = 0;
    for (int offset = 0; offset < 4; ++offset) {
      group_sum += aligned[group * 4 + offset];
    }
    batch.group_sums[group][output] = group_sum;
  }

  batch.max_exponent_tokens[output] = max_exponent_token;
  batch.exception_masks[output] = exception_mask;
}

static void reduce_fp32_chunks_cpu(const FpPartialBatch &partial, FpChunkBatch &batch, bool use_second_half) {
  using namespace cute_mma_model;
  const int groups_per_half = kMmaReduceWidthBytes / 8;
  int64_t first_halves[kFpOutputBatch] = {};
  int64_t second_halves[kFpOutputBatch] = {};
  // Parallelize independent outputs, keeping each half's group order and wrap32.
  for (int group = 0; group < groups_per_half; ++group) {
    for (int output = 0; output < kFpOutputBatch; ++output) {
      first_halves[output] = fixed_add(first_halves[output], partial.group_sums[group][output], 32);
      if (use_second_half) {
        second_halves[output] =
            fixed_add(second_halves[output], partial.group_sums[groups_per_half + group][output], 32);
      }
    }
  }
  uint32_t c_magnitudes[kFpOutputBatch];
  for (int output = 0; output < kFpOutputBatch; ++output) {
    const unsigned shift = partial.max_exponent_tokens[output] - partial.c_exponent_tokens[output];
    // C's zero-extended 32-bit significand is zero after any right shift of 32 or more.
    c_magnitudes[output] =
        (partial.c_significands[output] >> (shift & 31u)) & (0u - static_cast<uint32_t>(shift < 32u));
  }
  int64_t aligned_cs[kFpOutputBatch];
  for (int output = 0; output < kFpOutputBatch; ++output) {
    const int64_t magnitude = c_magnitudes[output];
    aligned_cs[output] = partial.c_signs[output] != 0 ? -magnitude : magnitude;
  }
  for (int output = 0; output < kFpOutputBatch; ++output) {
    first_halves[output] = fixed_add(first_halves[output], aligned_cs[output], 32);
  }
  if (use_second_half) {
    for (int output = 0; output < kFpOutputBatch; ++output) {
      first_halves[output] = fixed_add(first_halves[output], second_halves[output], 32);
    }
  }
  for (int output = 0; output < kFpOutputBatch; ++output) {
    batch.reduce_bits[output] = static_cast<uint32_t>(first_halves[output]);
    batch.max_exponent_tokens[output] = partial.max_exponent_tokens[output];
    batch.exception_masks[output] = partial.exception_masks[output];
  }
}

static void finish_fp32_chunks_cpu(const FpChunkBatch &batch, uint32_t *results) {
  uint32_t magnitudes[kFpOutputBatch];
  int leading_zeros[kFpOutputBatch];
  for (int output = 0; output < kFpOutputBatch; ++output) {
    const uint32_t bits = batch.reduce_bits[output];
    magnitudes[output] = (bits & 0x80000000u) != 0 ? ~bits + 1u : bits;
    leading_zeros[output] = cute_mma_model::leading_zeros_32(magnitudes[output]);
  }

  // Match normalize_fp32(), with CLZ separated from the independent field arithmetic.
  for (int output = 0; output < kFpOutputBatch; ++output) {
    const uint32_t bits = batch.reduce_bits[output];
    const int zeros = leading_zeros[output];
    const int shift_exp = batch.max_exponent_tokens[output] - 511 - zeros + 8;
    const uint32_t shift_mantissa = zeros > 8 ? magnitudes[output] << (zeros - 8) : magnitudes[output] >> (8 - zeros);
    const uint32_t exponent = shift_exp < -126 ? 0u : shift_exp > 127 ? 255u : static_cast<uint32_t>(shift_exp + 127);
    const uint32_t significand = shift_exp < -149 || shift_exp > 127 ? 0u
                                 : shift_exp < -126 ? (shift_mantissa & 0x00ffffffu) >> (-126 - shift_exp)
                                                    : shift_mantissa & 0x00ffffffu;
    uint32_t result = bits == 0 ? 0 : (bits & 0x80000000u) | (exponent << 23) | (significand & 0x007fffffu);
    const uint32_t exceptions = batch.exception_masks[output];
    if ((exceptions & 4u) != 0) {
      result = 0xff800000u;
    }
    if ((exceptions & 2u) != 0) {
      result = 0x7f800000u;
    }
    if ((exceptions & 1u) != 0 || (exceptions & 6u) == 6u) {
      result = 0x7fc00000u;
    }
    results[output] = result;
  }
}

static inline uint32_t read_u32(const uint8_t *data, size_t index0, size_t index1, size_t row_bytes) {
  const uint8_t *row_ptr = data + index0 * row_bytes;
  return reinterpret_cast<const uint32_t *>(row_ptr)[index1];
}

static inline void write_u32(uint8_t *data, size_t index0, size_t index1, size_t row_bytes, uint32_t value) {
  uint8_t *row_ptr = data + index0 * row_bytes;
  reinterpret_cast<uint32_t *>(row_ptr)[index1] = value;
}

template <cute_mma_model::FloatFormat Format> static bool verify_fp_mma(MmaVerificationBuffer *buffer);
template <class src1_t, class src2_t> static bool verify_int_mma(MmaVerificationBuffer *buffer);

bool verify_cpu_mma(MmaVerificationBuffer *buffer) {
  if (!is_mma_32bit_result_type(buffer->amu_event.typed)) {
    return false;
  }

  if (buffer->amu_event.isfp) { // Floating-point MMA
    const MmaElementType source_type = static_cast<MmaElementType>(buffer->amu_event.types1);
    switch (source_type) {
      case MmaElementType::Fp8E5M2: return verify_fp_mma<cute_mma_model::FloatFormat::Fp8E5M2>(buffer);
      case MmaElementType::Fp8E4M3: return verify_fp_mma<cute_mma_model::FloatFormat::Fp8E4M3>(buffer);
      case MmaElementType::Fp16: return verify_fp_mma<cute_mma_model::FloatFormat::Fp16>(buffer);
      case MmaElementType::Bf16: return verify_fp_mma<cute_mma_model::FloatFormat::Bf16>(buffer);
      case MmaElementType::Tf32: return verify_fp_mma<cute_mma_model::FloatFormat::Tf32>(buffer);
      default: return false;
    }
  } else { // Integer MMA
    uint8_t types1 = buffer->amu_event.types1;
    uint8_t types2 = buffer->amu_event.types2;
    int op = ((types1 & 0x4) >> 1) | ((types2 & 0x4) >> 2);
    switch (op) {
      case 0: return verify_int_mma<uint8_t, uint8_t>(buffer);
      case 1: return verify_int_mma<uint8_t, int8_t>(buffer);
      case 2: return verify_int_mma<int8_t, uint8_t>(buffer);
      case 3: return verify_int_mma<int8_t, int8_t>(buffer);
      default: return false;
    }
  }
}

template <cute_mma_model::FloatFormat Format> static bool verify_fp_mma(MmaVerificationBuffer *buffer) {
  const int tile_m = buffer->amu_event.mtilem;
  const int tile_k = buffer->amu_event.mtilek;
  const int tile_n = buffer->amu_event.mtilen;
  const int src_elem_bytes = cute_mma_model::source_bytes(Format);
  const int reduce_chunk_elems = kMmaReduceWidthBytes / src_elem_bytes;
  const size_t src_row_bytes = kMmaSourceRowBytes;
  const size_t result_row_bytes = kMmaResultRowBytes;

  // FP addition is not associative; reproduce CUTE's fixed-width reduction tree and FP32 normalization per K chunk.
  for (int i = 0; i < tile_m; ++i) {
    const uint8_t *src1_row = buffer->src1 + static_cast<size_t>(i) * src_row_bytes;
    for (int j_base = 0; j_base < tile_n; j_base += kFpOutputBatch) {
      const int outputs = tile_n - j_base < kFpOutputBatch ? tile_n - j_base : kFpOutputBatch;
      uint32_t acc_bits[kFpOutputBatch] = {};
      for (int output = 0; output < outputs; ++output) {
        acc_bits[output] = read_u32(buffer->src3, i, j_base + output, result_row_bytes);
      }

      for (int k_base = 0; k_base < tile_k; k_base += reduce_chunk_elems) {
        int valid_lanes = tile_k - k_base;
        if (valid_lanes > reduce_chunk_elems) {
          valid_lanes = reduce_chunk_elems;
        }
        // Inactive outputs stay zero and never load or decode a source row.
        FpPartialBatch partial = {};
        FpChunkBatch batch;
        initialize_fp32_chunks_cpu(acc_bits, partial);
        for (int output = 0; output < outputs; ++output) {
          const uint8_t *src2_row = buffer->src2 + static_cast<size_t>(j_base + output) * src_row_bytes;
          prepare_fp32_chunk_cpu(Format, src1_row + static_cast<size_t>(k_base) * src_elem_bytes,
                                 src2_row + static_cast<size_t>(k_base) * src_elem_bytes, valid_lanes, partial, output);
        }
        reduce_fp32_chunks_cpu(partial, batch, cute_mma_model::uses_second_product_half(Format));
        finish_fp32_chunks_cpu(batch, acc_bits);
      }

      for (int output = 0; output < outputs; ++output) {
        write_u32(buffer->src3, i, j_base + output, result_row_bytes, acc_bits[output]);
      }
    }
  }

  return compare_mma_rows(buffer->dut_result.get(), buffer->src3, tile_m, tile_n * sizeof(uint32_t));
}

template <class src1_t, class src2_t> static bool verify_int_mma(MmaVerificationBuffer *buffer) {
  const int tile_m = buffer->amu_event.mtilem;
  const int tile_k = buffer->amu_event.mtilek;
  const int tile_n = buffer->amu_event.mtilen;
  const int reduce_chunk_elems = kMmaReduceWidthBytes;
  const size_t src_row_bytes = kMmaSourceRowBytes;
  const size_t result_row_bytes = kMmaResultRowBytes;
  const bool src1_signed = std::is_signed<src1_t>::value;
  const bool src2_signed = std::is_signed<src2_t>::value;

  // Follow the same ReducePE chunk boundaries for wrapping and saturation.
  for (int i = 0; i < tile_m; ++i) {
    const uint8_t *src1_row = buffer->src1 + static_cast<size_t>(i) * src_row_bytes;
    for (int j = 0; j < tile_n; ++j) {
      const uint8_t *src2_row = buffer->src2 + static_cast<size_t>(j) * src_row_bytes;
      uint32_t acc_bits = read_u32(buffer->src3, i, j, result_row_bytes);

      for (int k_base = 0; k_base < tile_k; k_base += reduce_chunk_elems) {
        int valid_lanes = tile_k - k_base;
        if (valid_lanes > reduce_chunk_elems) {
          valid_lanes = reduce_chunk_elems;
        }
        acc_bits = reduce_int32_chunk_cpu(src1_signed, src2_signed, src1_row + k_base, src2_row + k_base, valid_lanes,
                                          acc_bits, buffer->amu_event.sat);
      }

      write_u32(buffer->src3, i, j, result_row_bytes, acc_bits);
    }
  }

  return compare_mma_rows(buffer->dut_result.get(), buffer->src3, tile_m, tile_n * sizeof(uint32_t));
}

#endif // CONFIG_DIFFTEST_AMUCTRLEVENT
