/***************************************************************************************
* Copyright (c) 2026 Institute of Computing Technology, Chinese Academy of Sciences
* Copyright (c) 2026 Beijing Institute of Open Source Chip (BOSC)
*
* DiffTest is licensed under Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
***************************************************************************************/

#ifndef __MATRIX_WB_REBUILD_H__
#define __MATRIX_WB_REBUILD_H__

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace matrix_wb_rebuild {

// A complete C-register write normally carries the same entry address for each
// bank. Compute its interleave once and issue fixed-size copies. The caller
// selects the unmasked C-register/MMA path; every other event uses the ordinary
// recorder. No state or pointers survive this call.
template <size_t Banks, size_t Words, size_t EventWords, size_t RowBits, size_t MatrixBytes, typename Probe>
inline bool try_common_address(uint64_t *result, const Probe &probe) {
  static_assert(Words > 0 && Words <= EventWords, "invalid matrix writeback entry width");
  static_assert(RowBits % (Words * sizeof(uint64_t) * 8) == 0, "invalid matrix row/entry geometry");
  static_assert(MatrixBytes % (Banks * Words * sizeof(uint64_t)) == 0, "invalid matrix bank geometry");
  // Other geometries remain supported by the reference recorder. Four copies
  // avoid a per-bank loop and allow packed, unaligned event data safely.
  if (Banks != 4 || (Words != 2 && Words != 4)) {
    return false;
  }
  if ((probe.bankValid & 0xfU) != 0xfU) {
    return false;
  }
  const size_t address = probe.bankAddr[0];
  if (probe.bankAddr[1] != address || probe.bankAddr[2] != address || probe.bankAddr[3] != address) {
    return false;
  }
  constexpr size_t bank_bytes = Words * sizeof(uint64_t);
  constexpr size_t entries_per_bank = MatrixBytes / Banks / bank_bytes;
  constexpr size_t stride = RowBits / (bank_bytes * 8);
  if (entries_per_bank % stride != 0) {
    return false;
  }
  // Reject before copying any bank. The generic recorder then retains its
  // normal invalid-address assertion instead of silently discarding the event.
  if (address >= entries_per_bank) {
    return false;
  }
  constexpr size_t bank_stride_bytes = stride * bank_bytes;
  const size_t offset = (address / stride * stride * Banks + address % stride) * bank_bytes;
  assert(offset + (Banks - 1) * bank_stride_bytes + bank_bytes <= MatrixBytes);
  auto *dst = reinterpret_cast<uint8_t *>(result) + offset;
  const auto *src = reinterpret_cast<const uint8_t *>(probe.data);
  std::memcpy(dst, src, bank_bytes);
  std::memcpy(dst + bank_stride_bytes, src + EventWords * sizeof(uint64_t), bank_bytes);
  std::memcpy(dst + 2 * bank_stride_bytes, src + 2 * EventWords * sizeof(uint64_t), bank_bytes);
  std::memcpy(dst + 3 * bank_stride_bytes, src + 3 * EventWords * sizeof(uint64_t), bank_bytes);
  return true;
}

} // namespace matrix_wb_rebuild

#endif // __MATRIX_WB_REBUILD_H__
