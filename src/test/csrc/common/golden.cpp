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

/**
 * Read PTE from memory (physical address)
 * Uses goldenmem in difftest mode, pmem_read otherwise
 */
static inline uint64_t read_pte(uint64_t pte_addr) {
  uint64_t pte_val;
#ifdef CONFIG_NO_DIFFTEST
  pte_val = pmem_read(pte_addr);
#else
  read_goldenmem(pte_addr, &pte_val, 8);
#endif // CONFIG_NO_DIFFTEST
  return pte_val;
}

/**
 * G-Stage page table walk (GPA -> HPA)
 * Implementation aligned with do_s2xlate() in tlb.cpp
 *
 * @param hgatp  HGATP register value
 * @param gpaddr Guest physical address to translate
 * @return       r_s2xlate containing PTE and level
 */
static r_s2xlate do_g_stage(Hgatp *hgatp, uint64_t gpaddr) {
  PTE pte;
  uint64_t hpaddr;
  uint8_t level;
  uint64_t pg_base = hgatp->ppn << 12;
  r_s2xlate result;

  // If hgatp.mode == 0, G-stage is disabled, return GPA directly
  if (hgatp->mode == 0) {
    result.pte.val = 0;
    result.pte.ppn = gpaddr >> 12;
    result.pte.v = 1;  // Mark as valid for pass-through
    result.level = 0;
    return result;
  }

  // Determine max level: Sv39x4 (mode=8) -> 2, Sv48x4 (mode=9) -> 3
  int max_level = (hgatp->mode == 8) ? 2 : 3;

  for (level = max_level; level >= 0; level--) {
    // Use GVPNi macro: top level uses 11 bits, others use 9 bits
    hpaddr = pg_base + GVPNi(gpaddr, level, max_level) * sizeof(uint64_t);
    pte.val = read_pte(hpaddr);
    pg_base = pte.ppn << 12;

    // Stop if: invalid, leaf PTE (r/x/w set), or reached level 0
    if (!pte.v || pte.r || pte.x || pte.w || level == 0) {
      break;
    }
  }

  result.pte = pte;
  result.level = level;
  return result;
}

/**
 * Main pte_helper function with H-extension two-stage translation support
 * Implementation aligned with L1TLBChecker::check() in tlb.cpp
 *
 * Translation modes:
 * - noS2xlate:  Single-stage using satp (host mode)
 * - onlyStage1: VS-stage only using vsatp (PTE addresses are physical)
 * - onlyStage2: G-stage only using hgatp (vpn is GPA)
 * - allStage:   Full two-stage (GVA -> GPA -> HPA)
 */
uint8_t pte_helper(uint64_t satp, uint64_t vsatp, uint64_t hgatp,
                   uint64_t vpn, uint8_t s2xlate,
                   uint64_t *pte, uint8_t *level,
                   uint64_t *s1_pte, uint64_t *s2_pte, uint8_t *s1_level) {
#ifdef CONFIG_DIFFTEST_PERFCNT
  difftest_calls[perf_pte_helper]++;
  difftest_bytes[perf_pte_helper] += 41;
#endif // CONFIG_DIFFTEST_PERFCNT

  PTE result_pte;
  uint64_t paddr;
  uint8_t result_level;
  r_s2xlate g_result;
  bool isNapot = false;

  Satp *satp_p = (Satp *)&satp;
  Vsatp *vsatp_p = (Vsatp *)&vsatp;
  Hgatp *hgatp_p = (Hgatp *)&hgatp;

  uint8_t hasS2xlate = (s2xlate != noS2xlate);
  uint8_t onlyS2 = (s2xlate == onlyStage2);
  uint8_t hasAllStage = (s2xlate == allStage);

  // Select satp based on translation mode
  uint64_t pg_base = (hasS2xlate ? vsatp_p->ppn : satp_p->ppn) << 12;
  int mode = hasS2xlate ? vsatp_p->mode : satp_p->mode;
  int max_level = (mode == 8) ? 2 : 3;

  if (onlyS2) {
    // G-Stage only: vpn is treated as GPA
    g_result = do_g_stage(hgatp_p, vpn << 12);
    result_pte = g_result.pte;
    result_level = g_result.level;

    // Check for G-stage page fault: invalid or illegal permission (w && !r)
    if (!result_pte.v || (!result_pte.r && result_pte.w)) {
      *pte = result_pte.val;
      *level = result_level;
      *s1_pte = 0;
      *s2_pte = 0;
      *s1_level = 0;
      return PF_G_STAGE;
    }
  } else {
    // VS-stage walk (with optional G-stage for PTE addresses)
    for (result_level = max_level; result_level >= 0; result_level--) {
      paddr = pg_base + VPNi(vpn, result_level) * sizeof(uint64_t);

      if (hasAllStage) {
        // Translate PTE address through G-stage
        g_result = do_g_stage(hgatp_p, paddr);

        // Check for G-stage page fault during PTE access: invalid or illegal permission
        if (!g_result.pte.v || (!g_result.pte.r && g_result.pte.w)) {
          *pte = g_result.pte.val;
          *level = g_result.level;
          *s1_pte = 0;
          *s2_pte = 0;
          *s1_level = 0;
          return PF_G_STAGE;
        }

        // Calculate HPA from G-stage result (handle superpage)
        uint64_t pg_mask = ((1ull << VPNiSHFT(g_result.level)) - 1);
        if (g_result.level == 0 && g_result.pte.n) {
          pg_mask = ((1ull << NAPOTSHFT) - 1);
        }
        pg_base = (g_result.pte.ppn << 12 & ~pg_mask) | (paddr & pg_mask & ~PAGE_MASK);
        paddr = pg_base | (paddr & PAGE_MASK);
      }

      result_pte.val = read_pte(paddr);
      pg_base = result_pte.ppn << 12;

      // Stop if: invalid, leaf PTE, or reached level 0
      if (!result_pte.v || result_pte.r || result_pte.x || result_pte.w || result_level == 0) {
        break;
      }
    }

    // Check for VS-stage page fault: invalid or illegal permission (w && !r)
    if (!result_pte.v || (!result_pte.r && result_pte.w)) {
      *pte = result_pte.val;
      *level = result_level;
      *s1_pte = 0;
      *s2_pte = 0;
      *s1_level = 0;
      return PF_VS_STAGE;
    }

    // Save VS-stage PTE and level before G-stage final translation overwrites them
    if (hasAllStage) {
      *s1_pte = result_pte.val;
      *s1_level = result_level;
    }

    // Handle superpage: calculate final pg_base
    if (result_level > 0 && result_pte.v) {
      uint64_t pg_mask = ((1ull << VPNiSHFT(result_level)) - 1);
      pg_base = (result_pte.ppn << 12 & ~pg_mask) | (vpn << 12 & pg_mask & ~PAGE_MASK);
    } else if (result_level == 0 && result_pte.n) {
      isNapot = true;
      uint64_t pg_mask = ((1ull << NAPOTSHFT) - 1);
      pg_base = (result_pte.ppn << 12 & ~pg_mask) | (vpn << 12 & pg_mask & ~PAGE_MASK);
    }

    // Final G-stage translation for the leaf PTE's PPN
    if (hasAllStage && result_pte.v) {
      g_result = do_g_stage(hgatp_p, pg_base);

      // Check for G-stage page fault: invalid or illegal permission
      if (!g_result.pte.v || (!g_result.pte.r && g_result.pte.w)) {
        *pte = g_result.pte.val;
        *level = g_result.level;
        *s2_pte = 0;
        return PF_G_STAGE;
      }

      result_pte = g_result.pte;
      result_level = g_result.level;
      // save G-stage PTE for allStage mode
      *s2_pte = g_result.pte.val;
      if (result_level == 0 && result_pte.n) {
        isNapot = true;
      }
    }
  }

  // Handle NAPOT: adjust PPN
  if (isNapot) {
    result_pte.ppn = result_pte.ppn >> 4 << 4;
  }

  *pte = result_pte.val;
  *level = result_level;
  // For non-allStage modes, set s1_pte/s2_pte/s1_level to 0
  if (!hasAllStage) {
    *s1_pte = 0;
    *s2_pte = 0;
    *s1_level = 0;
  }
  return PF_NONE;
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

#define SEXT32(data)      ((uint64_t)(data) | ((data >> 31) ? (0xffffffffUL << 32) : 0))
#define GET_LOWER32(data) ((data) & ((1UL << 32) - 1))
#define GET_UPPER32(data) ((data) >> 32)

uint64_t amo_helper(uint8_t cmd, uint64_t addr, uint64_t wdata, uint8_t mask) {
#ifdef CONFIG_DIFFTEST_PERFCNT
  difftest_calls[perf_amo_helper]++;
  difftest_bytes[perf_amo_helper] += 18;
#endif // CONFIG_DIFFTEST_PERFCNT
  if (addr % 8 == 4 && mask == 0xf0) {
    addr -= 4;
  } else if (addr % 8 != 0) {
    printf("warning: amo address %lx not naturally aligned to %x!!\n", addr, mask);
  }
  if (mask != 0xff && mask != 0xf && mask != 0xf0) {
    printf("warning: amo data mask %x not aligned to 32-bit\n", mask);
  }
  static uint64_t lr_addr = 0, lr_valid = 0;
  uint64_t rdata = pmem_read(addr);
  uint64_t upper_r = GET_UPPER32(rdata), upper_w = GET_UPPER32(wdata);
  uint64_t lower_r = GET_LOWER32(rdata), lower_w = GET_LOWER32(wdata);
  uint64_t rop = (mask == 0xff) ? rdata : (mask == 0xf) ? lower_r : upper_r;
  uint64_t wop = (mask == 0xff) ? wdata : (mask == 0xf) ? lower_w : upper_w;
  uint64_t result = 0;
  switch (cmd) {
    case M_XA_SWAP: result = wop; break;
    case M_XLR:
      result = rdata;
      lr_valid = 1;
      lr_addr = addr;
      break;
    // always succeed
    case M_XSC:
      rop = !(lr_valid && lr_addr == addr);
      lr_valid = 0;
      result = wop;
      break;
    case M_XA_ADD: result = wop + rop; break;
    case M_XA_XOR: result = wop ^ rop; break;
    case M_XA_OR: result = wop | rop; break;
    case M_XA_AND: result = wop & rop; break;
    case M_XA_MIN:
      if (mask == 0xff)
        result = ((int64_t)wop > (int64_t)rop) ? rop : wop;
      else
        result = ((int32_t)((uint32_t)wop) > (int32_t)((uint32_t)rop)) ? rop : wop;
      break;
    case M_XA_MAX:
      if (mask == 0xff)
        result = ((int64_t)wop > (int64_t)rop) ? wop : rop;
      else
        result = ((int32_t)((uint32_t)wop) > (int32_t)((uint32_t)rop)) ? wop : rop;
      break;
    case M_XA_MINU: result = (wop > rdata) ? rop : wop; break;
    case M_XA_MAXU: result = (wop > rdata) ? wop : rop; break;
    default: printf("unknown atomic op %d!!\n", cmd); break;
  }
  if (mask == 0xf) {
    result = (upper_r << 32) | (result & 0xffffffffUL);
  } else if (mask == 0xf0) {
    result = (result << 32) | lower_r;
  }
  pmem_write(addr, result);
  return (mask == 0xf0) ? rop << 32 : rop;
}
