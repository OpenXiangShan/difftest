/***************************************************************************************
* Copyright (c) 2020-2025 Institute of Computing Technology, Chinese Academy of Sciences
* Copyright (c) 2025 Beijing Institute of Open Source Chip
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

#include "checkers.h"
#include "golden.h"
#include "goldenmem.h"

#define PAGE_SHIFT 12
#define PAGE_SIZE  (1ul << PAGE_SHIFT)
#define PAGE_MASK  (PAGE_SIZE - 1)

#define noS2xlate           0
#define onlyStage1          1
#define onlyStage2          2
#define allStage            3
#define VPNiSHFT(i)         (12 + 9 * (i))
#define GVPNi(addr, i, max) (((addr) >> (9 * (i) + 12)) & ((i == 3 || (i == 2 && max == 2)) ? 0x7ff : 0x1ff))
#define NAPOTSHFT           (12 + 4) // only support 64kb page

typedef union atpStruct {
  struct {
    uint64_t ppn : 44;
    uint32_t asid : 16;
    uint32_t mode : 4;
  };
  uint64_t val;
} Satp, Hgatp;

typedef struct {
  PTE pte;
  uint8_t level;
} r_s2xlate;

static r_s2xlate do_s2xlate(Hgatp *hgatp, uint64_t gpaddr) {
  PTE pte;
  uint64_t hpaddr;
  uint8_t level;
  uint64_t pg_base = hgatp->ppn << 12;
  r_s2xlate r_s2;
  if (hgatp->mode == 0) {
    r_s2.pte.ppn = gpaddr >> 12;
    r_s2.level = 0;
    return r_s2;
  }
  int max_level = hgatp->mode == 8 ? 2 : 3;
  for (level = max_level; level >= 0; level--) {
    hpaddr = pg_base + GVPNi(gpaddr, level, max_level) * sizeof(uint64_t);
    read_goldenmem(hpaddr, &pte.val, 8);
    pg_base = pte.ppn << 12;
    if (!pte.v || pte.r || pte.x || pte.w || level == 0) {
      break;
    }
  }
  r_s2.pte = pte;
  r_s2.level = level;
  return r_s2;
}

#ifdef CONFIG_DIFFTEST_L1TLBEVENT
bool L1TLBChecker::get_valid(const DifftestL1TLBEvent &probe) {
  return probe.valid;
}

void L1TLBChecker::clear_valid(DifftestL1TLBEvent &probe) {
  probe.valid = 0;
}

int L1TLBChecker::check(const DifftestL1TLBEvent &probe) {
  PTE pte;
  uint64_t paddr;
  uint8_t difftest_level;
  r_s2xlate r_s2;
  bool isNapot = false;

  Satp *satp = (Satp *)&probe.satp;
  Satp *vsatp = (Satp *)&probe.vsatp;
  Hgatp *hgatp = (Hgatp *)&probe.hgatp;
  uint8_t hasS2xlate = probe.s2xlate != noS2xlate;
  uint8_t onlyS2 = probe.s2xlate == onlyStage2;
  uint8_t hasAllStage = probe.s2xlate == allStage;
  uint64_t pg_base = (hasS2xlate ? vsatp->ppn : satp->ppn) << 12;
  int mode = hasS2xlate ? vsatp->mode : satp->mode;
  int max_level = mode == 8 ? 2 : 3;
  if (onlyS2) {
    r_s2 = do_s2xlate(hgatp, probe.vpn << 12);
    pte = r_s2.pte;
    difftest_level = r_s2.level;
  } else {
    for (difftest_level = max_level; difftest_level >= 0; difftest_level--) {
      paddr = pg_base + VPNi(probe.vpn, difftest_level) * sizeof(uint64_t);
      if (hasAllStage) {
        r_s2 = do_s2xlate(hgatp, paddr);
        uint64_t pg_mask = ((1ull << VPNiSHFT(r_s2.level)) - 1);
        if (r_s2.level == 0 && r_s2.pte.n) {
          pg_mask = ((1ull << NAPOTSHFT) - 1);
        }
        pg_base = (r_s2.pte.ppn << 12 & ~pg_mask) | (paddr & pg_mask & ~PAGE_MASK);
        paddr = pg_base | (paddr & PAGE_MASK);
      }
      read_goldenmem(paddr, &pte.val, 8);
      pg_base = pte.ppn << 12;
      if (!pte.v || pte.r || pte.x || pte.w || difftest_level == 0) {
        break;
      }
    }
    if (difftest_level > 0 && pte.v) {
      uint64_t pg_mask = ((1ull << VPNiSHFT(difftest_level)) - 1);
      pg_base = (pte.ppn << 12 & ~pg_mask) | (probe.vpn << 12 & pg_mask & ~PAGE_MASK);
    } else if (difftest_level == 0 && pte.n) {
      isNapot = true;
      uint64_t pg_mask = ((1ull << NAPOTSHFT) - 1);
      pg_base = (pte.ppn << 12 & ~pg_mask) | (probe.vpn << 12 & pg_mask & ~PAGE_MASK);
    }
    if (hasAllStage && pte.v) {
      r_s2 = do_s2xlate(hgatp, pg_base);
      pte = r_s2.pte;
      difftest_level = r_s2.level;
      if (difftest_level == 0 && pte.n) {
        isNapot = true;
      }
    }
  }

  uint64_t ppn = probe.ppn;
  if (isNapot) {
    ppn = probe.ppn >> 4 << 4;
    pte.difftest_ppn = pte.difftest_ppn >> 4 << 4;
  } else {
    ppn = probe.ppn >> difftest_level * 9 << difftest_level * 9;
  }

  if (pte.difftest_ppn != ppn) {
    Info("Warning: l1tlb resp test of core %d failed! vpn = %lx\n", state->coreid, probe.vpn);
    Info("  REF commits pte.val: 0x%lx, dut s2xlate: %d\n", pte.val, probe.s2xlate);
    Info("  REF commits ppn 0x%lx, DUT commits ppn 0x%lx\n", pte.difftest_ppn, ppn);
    Info("  REF commits perm 0x%02x, level %d, pf %d\n", pte.difftest_perm, difftest_level, !pte.difftest_v);
  }

  return STATE_OK;
}

#endif // CONFIG_DIFFTEST_L1TLBEVENT

#ifdef CONFIG_DIFFTEST_L2TLBEVENT
bool L2TLBChecker::get_valid(const DifftestL2TLBEvent &probe) {
  return probe.valid;
}

void L2TLBChecker::clear_valid(DifftestL2TLBEvent &probe) {
  probe.valid = 0;
}

int L2TLBChecker::check(const DifftestL2TLBEvent &probe) {
  Satp *satp = (Satp *)&probe.satp;
  Satp *vsatp = (Satp *)&probe.vsatp;
  Hgatp *hgatp = (Hgatp *)&probe.hgatp;
  PTE pte;
  r_s2xlate r_s2;
  r_s2xlate check_s2;
  uint64_t paddr;
  uint8_t difftest_level;
  for (int i = 0; i < 8; i++) {
    if (probe.valididx[i]) {
      uint8_t hasS2xlate = probe.s2xlate != noS2xlate;
      uint8_t onlyS2 = probe.s2xlate == onlyStage2;
      uint64_t pg_base = (hasS2xlate ? vsatp->ppn : satp->ppn) << 12;
      int mode = hasS2xlate ? vsatp->mode : satp->mode;
      int max_level = mode == 8 ? 2 : 3;
      if (onlyS2) {
        r_s2 = do_s2xlate(hgatp, probe.vpn << 12);
        uint64_t pg_mask = ((1ull << VPNiSHFT(r_s2.level)) - 1);
        uint64_t s2_pg_base = r_s2.pte.ppn << 12;
        pg_base = (s2_pg_base & ~pg_mask) | (paddr & pg_mask & ~PAGE_MASK);
        paddr = pg_base | (paddr & PAGE_MASK);
      }
      for (difftest_level = max_level; difftest_level >= 0; difftest_level--) {
        paddr = pg_base + VPNi(probe.vpn + i, difftest_level) * sizeof(uint64_t);
        if (hasS2xlate) {
          r_s2 = do_s2xlate(hgatp, paddr);
          uint64_t pg_mask = ((1ull << VPNiSHFT(r_s2.level)) - 1);
          pg_base = (r_s2.pte.ppn << 12 & ~pg_mask) | (paddr & pg_mask & ~PAGE_MASK);
          paddr = pg_base | (paddr & PAGE_MASK);
        }
        read_goldenmem(paddr, &pte.val, 8);
        if (!pte.v || pte.r || pte.x || pte.w || difftest_level == 0) {
          break;
        }
        pg_base = pte.ppn << 12;
      }

      if (hasS2xlate) {
        r_s2 = do_s2xlate(hgatp, pg_base);
        if (probe.pteidx[i])
          check_s2 = r_s2;
      }
      bool difftest_gpf = !r_s2.pte.v || (!r_s2.pte.r && r_s2.pte.w);
      bool difftest_pf = !pte.v || (!pte.r && pte.w);
      bool s1_check_fail = pte.difftest_ppn != probe.ppn[i] || pte.difftest_perm != probe.perm ||
                           pte.difftest_pbmt != probe.pbmt || difftest_level != probe.level || difftest_pf != probe.pf;
      bool s2_check_fail = hasS2xlate
                               ? r_s2.pte.difftest_ppn != probe.s2ppn || r_s2.pte.difftest_perm != probe.g_perm ||
                                     r_s2.pte.difftest_pbmt != probe.g_pbmt || r_s2.level != probe.g_level ||
                                     difftest_gpf != probe.gpf
                               : false;
      if (s1_check_fail || s2_check_fail) {
        Info("Warning: L2TLB resp test of core %d sector %d failed! vpn = %lx\n", state->coreid, i, probe.vpn + i);
        Info("  REF commits ppn 0x%lx, perm 0x%02x, level %d, pf %d\n", pte.difftest_ppn, pte.difftest_perm,
             difftest_level, difftest_pf);
        if (hasS2xlate)
          Info("      s2_ppn 0x%lx, g_perm 0x%02x, g_level %d, gpf %d\n", r_s2.pte.difftest_ppn, r_s2.pte.difftest_perm,
               r_s2.level, difftest_gpf);
        Info("  DUT commits ppn 0x%lx, perm 0x%02x, level %d, pf %d\n", probe.ppn[i], probe.perm, probe.level,
             probe.pf);
        if (hasS2xlate)
          Info("      s2_ppn 0x%lx, g_perm 0x%02x, g_level %d, gpf %d\n", probe.s2ppn, probe.g_perm, probe.g_level,
               probe.gpf);
        return STATE_ERROR;
      }
    }
  }

  return STATE_OK;
}
#endif // CONFIG_DIFFTEST_L2TLBEVENT
