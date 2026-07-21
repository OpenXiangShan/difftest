/***************************************************************************************
* Copyright (c) 2020-2023 Institute of Computing Technology, Chinese Academy of Sciences
* Copyright (c) 2020-2021 Peng Cheng Laboratory
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

#include "golden.h"
#include "ram.h"
#ifndef CONFIG_NO_DIFFTEST
#include "goldenmem.h"
#endif // CONFIG_NO_DIFFTEST
#ifdef CONFIG_DIFFTEST_PERFCNT
#include "perf.h"
#endif // CONFIG_DIFFTEST_PERFCNT

uint8_t pte_helper(uint64_t satp, uint64_t vpn, uint64_t *pte, uint8_t *level) {
#ifdef CONFIG_DIFFTEST_PERFCNT
  difftest_calls[perf_pte_helper]++;
  difftest_bytes[perf_pte_helper] += 25;
#endif // CONFIG_DIFFTEST_PERFCNT
  uint64_t pg_base = satp << 12, pte_addr;
  PTE *pte_p = (PTE *)pte;
  for (*level = 0; *level < 3; (*level)++) {
    pte_addr = pg_base + VPNi(vpn, *level) * sizeof(uint64_t);
#ifdef CONFIG_NO_DIFFTEST
    pte_p->val = pmem_read(pte_addr);
#else
    read_goldenmem(pte_addr, &pte_p->val, 8);
#endif // CONFIG_NO_DIFFTEST
    pg_base = pte_p->ppn << 12;
    // pf
    if (!pte_p->v) {
      return 1;
    }
    // leaf pte
    if (pte_p->r || pte_p->x) {
      return 0;
    }
  }
  return 1;
}

enum {
  M_XA_SWAP = 4,
  M_XLR = 6,
  M_XSC = 7,
  M_XA_ADD = 8,
  M_XA_XOR = 9,
  M_XA_OR = 10,
  M_XA_AND = 11,
  M_XA_MIN = 12,
  M_XA_MAX = 13,
  M_XA_MINU = 14,
  M_XA_MAXU = 15
};

uint64_t amo_helper(uint8_t cmd, uint64_t addr, uint64_t wdata, uint8_t mask) {
#ifdef CONFIG_DIFFTEST_PERFCNT
  difftest_calls[perf_amo_helper]++;
  difftest_bytes[perf_amo_helper] += 18;
#endif // CONFIG_DIFFTEST_PERFCNT
  const unsigned byte_offset = mask == 0 ? 0 : __builtin_ctz(mask);
  const unsigned bytes = __builtin_popcount(mask);
  const uint8_t contiguous_mask = bytes == 8 ? 0xff : ((1U << bytes) - 1) << byte_offset;
  if ((bytes != 1 && bytes != 2 && bytes != 4 && bytes != 8) || mask != contiguous_mask) {
    printf("warning: invalid amo data mask %x\n", mask);
    return 0;
  }
  if ((addr & 0x7) != byte_offset || (addr & (bytes - 1)) != 0) {
    printf("warning: amo address %lx does not match mask %x\n", addr, mask);
  }

  const uint64_t aligned_addr = addr & ~0x7ULL;
  const unsigned bits = bytes * 8;
  const unsigned bit_shift = byte_offset * 8;
  const uint64_t value_mask = bits == 64 ? ~0ULL : (1ULL << bits) - 1;
  static uint64_t lr_addr = 0, lr_valid = 0;
  uint64_t rdata = pmem_read(aligned_addr);
  uint64_t rop = (rdata >> bit_shift) & value_mask;
  uint64_t wop = (wdata >> bit_shift) & value_mask;
  uint64_t result = 0;
  const auto as_signed = [bits](uint64_t value) -> int64_t {
    if (bits == 64)
      return static_cast<int64_t>(value);
    const uint64_t sign = 1ULL << (bits - 1);
    return static_cast<int64_t>((value ^ sign) - sign);
  };
  switch (cmd) {
    case M_XA_SWAP: result = wop; break;
    case M_XLR:
      result = rop;
      lr_valid = 1;
      lr_addr = aligned_addr;
      break;
    // always succeed
    case M_XSC:
      rop = !(lr_valid && lr_addr == aligned_addr);
      lr_valid = 0;
      result = wop;
      break;
    case M_XA_ADD: result = wop + rop; break;
    case M_XA_XOR: result = wop ^ rop; break;
    case M_XA_OR: result = wop | rop; break;
    case M_XA_AND: result = wop & rop; break;
    case M_XA_MIN: result = as_signed(wop) > as_signed(rop) ? rop : wop; break;
    case M_XA_MAX: result = as_signed(wop) > as_signed(rop) ? wop : rop; break;
    case M_XA_MINU: result = wop > rop ? rop : wop; break;
    case M_XA_MAXU: result = wop > rop ? wop : rop; break;
    default: printf("unknown atomic op %d!!\n", cmd); break;
  }
  result = (rdata & ~(value_mask << bit_shift)) | ((result & value_mask) << bit_shift);
  pmem_write(aligned_addr, result);
  return rop << bit_shift;
}
