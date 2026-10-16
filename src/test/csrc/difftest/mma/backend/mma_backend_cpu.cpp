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
#include <cstdint>
#include <cstring>
#include <type_traits>

#ifdef CONFIG_DIFF_MMA_REDUCE_WIDTH_BYTES
static constexpr int kMmaReduceWidthBytes = CONFIG_DIFF_MMA_REDUCE_WIDTH_BYTES;
#else
static constexpr int kMmaReduceWidthBytes = 32;
#endif

static inline uint32_t read_u32(const uint8_t *data, size_t index0, size_t index1, size_t row_bytes) {
  const uint8_t *row_ptr = data + index0 * row_bytes;
  return reinterpret_cast<const uint32_t *>(row_ptr)[index1];
}

static inline void write_u32(uint8_t *data, size_t index0, size_t index1, size_t row_bytes, uint32_t value) {
  uint8_t *row_ptr = data + index0 * row_bytes;
  reinterpret_cast<uint32_t *>(row_ptr)[index1] = value;
}

void CpuMmaBackend::verify(const std::vector<MmaVerificationBuffer *> &buffers, std::vector<uint8_t> &passed) {
  passed.clear();
  passed.reserve(buffers.size());
  for (auto *buffer: buffers) {
    passed.push_back(verify_one(buffer) ? 1 : 0);
  }
}

bool CpuMmaBackend::verify_one(MmaVerificationBuffer *buffer) {
  if (!is_mma_32bit_result_type(buffer->amu_event.typed)) {
    return false;
  }

  if (buffer->amu_event.packedMode) return verify_packed_mma(buffer);

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

// Interpret the native packed rows independently of CUTE's reduction helpers.
// RTL keeps its existing wrap behavior for both values of the saturation flag.
bool CpuMmaBackend::verify_packed_mma(MmaVerificationBuffer *buffer) {
  const auto &event = buffer->amu_event;
  const unsigned aw = event.types1 == 7 ? 4 : 8;
  const unsigned bw = event.packedMode == 1 ? 2 : 1;
  const unsigned k = 512 / aw;
  if ((event.packedMode != 1 && event.packedMode != 2) || event.isfp ||
      (event.types1 != 4 && event.types1 != 7) || event.types2 != (bw == 2 ? 4 : 0) || event.typed != 2 ||
      event.mtilem != 128 || event.mtilen != 128 || event.mtilek != k || event.kslice >= aw / bw)
    return false;

  bool passed = true;
  MatrixHash128 partial_hash{};
  // C writes visit Mgroup, Ngroup, bank, then the eight adjacent columns.
  for (unsigned mg = 0; mg < 16; ++mg) {
    for (unsigned ng = 0; ng < 16; ++ng) {
      for (unsigned bank = 0; bank < 8; ++bank) {
        const unsigned m = mg * 8 + bank;
        const uint8_t *a = buffer->src1 + m * 64;
        for (unsigned column = 0; column < 8; ++column) {
          const unsigned n = ng * 8 + column;
          const uint8_t *b = buffer->src2 + n * 64;
          const size_t offset = m * 512 + n * 4;
          uint32_t value;
          memcpy(&value, buffer->src3 + offset, sizeof(value));
          for (unsigned lane = 0; lane < k; ++lane) {
            const unsigned a_bit = lane * aw;
            const unsigned a_code = (a[a_bit / 8] >> (a_bit % 8)) & ((1u << aw) - 1);
            const int32_t activation = a_code & (1u << (aw - 1)) ? int32_t(a_code) - int32_t(1u << aw) : int32_t(a_code);
            const unsigned b_bit = (event.kslice * k + lane) * bw;
            const unsigned b_code = (b[b_bit / 8] >> (b_bit % 8)) & ((1u << bw) - 1);
            // The reserved ternary code 10 has undefined results; choose zero.
            const int32_t weight = bw == 1 ? int32_t(b_code) : b_code == 1 ? 1 : b_code == 3 ? -1 : 0;
            value += uint32_t(int64_t(activation) * weight);
            if (lane + 1 == k / 2)
              bitmte_hash_bytes(partial_hash, reinterpret_cast<const uint8_t *>(&value), sizeof(value));
          }
          memcpy(buffer->src3 + offset, &value, sizeof(value));
          uint32_t dut_value;
          memcpy(&dut_value, buffer->dut_result + offset, sizeof(dut_value));
          if (dut_value != value) passed = false;
        }
      }
    }
  }
  return passed && partial_hash == buffer->packed_partial_hash;
}

template <cute_mma_model::FloatFormat Format> bool CpuMmaBackend::verify_fp_mma(MmaVerificationBuffer *buffer) {
  const int tile_m = buffer->amu_event.mtilem;
  const int tile_k = buffer->amu_event.mtilek;
  const int tile_n = buffer->amu_event.mtilen;
  const int src_elem_bytes = cute_mma_model::source_bytes(Format);
  const int reduce_chunk_elems = kMmaReduceWidthBytes / src_elem_bytes;
  const size_t src_row_bytes = static_cast<size_t>(tile_k) * src_elem_bytes;
  const size_t result_row_bytes = static_cast<size_t>(tile_n) * sizeof(uint32_t);

  // FP addition is not associative; reproduce CUTE's fixed-width reduction tree and FP32 normalization per K chunk.
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
        acc_bits = cute_mma_model::reduce_fp32_chunk(Format, src1_row + static_cast<size_t>(k_base) * src_elem_bytes,
                                                     src2_row + static_cast<size_t>(k_base) * src_elem_bytes,
                                                     valid_lanes, kMmaReduceWidthBytes, acc_bits);
      }

      write_u32(buffer->src3, i, j, result_row_bytes, acc_bits);
    }
  }

  return memcmp(buffer->dut_result, buffer->src3, static_cast<size_t>(tile_m) * tile_n * sizeof(uint32_t)) == 0;
}

template <class src1_t, class src2_t> bool CpuMmaBackend::verify_int_mma(MmaVerificationBuffer *buffer) {
  const int tile_m = buffer->amu_event.mtilem;
  const int tile_k = buffer->amu_event.mtilek;
  const int tile_n = buffer->amu_event.mtilen;
  const int reduce_chunk_elems = kMmaReduceWidthBytes;
  const size_t src_row_bytes = static_cast<size_t>(tile_k);
  const size_t result_row_bytes = static_cast<size_t>(tile_n) * sizeof(uint32_t);
  const bool src1_signed = std::is_signed<src1_t>::value;
  const bool src2_signed = std::is_signed<src2_t>::value;

  // Match CUTE's fixed-width integer reduction tree; the current RTL does not consume the MMA saturation flag.
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
        acc_bits = cute_mma_model::reduce_int32_chunk(src1_signed, src2_signed, src1_row + k_base, src2_row + k_base,
                                                      valid_lanes, kMmaReduceWidthBytes, acc_bits);
      }

      write_u32(buffer->src3, i, j, result_row_bytes, acc_bits);
    }
  }

  return memcmp(buffer->dut_result, buffer->src3, static_cast<size_t>(tile_m) * tile_n * sizeof(uint32_t)) == 0;
}

#endif // CONFIG_DIFFTEST_AMUCTRLEVENT
