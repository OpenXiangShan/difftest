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

#include "checker.h"
#include "golden.h"

#ifdef CONFIG_DIFFTEST_L1TLBEVENT

bool L1TLBChecker::get_valid(const DifftestL1TLBEvent &probe) {
  return probe.valid;
}

void L1TLBChecker::clear_valid(DifftestL1TLBEvent &probe) {
  probe.valid = 0;
}

int L1TLBChecker::check(const DifftestL1TLBEvent &probe, const DiffTestRegState &regs) {
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
  if (isNapot) {
    probe.ppn = probe.ppn >> 4 << 4;
    pte.difftest_ppn = pte.difftest_ppn >> 4 << 4;
  } else {
    probe.ppn = probe.ppn >> difftest_level * 9 << difftest_level * 9;
  }

  if (pte.difftest_ppn != probe.ppn) {
    Info("Warning: l1tlb resp test of core %d index %d failed! vpn = %lx\n", id, i, probe.vpn);
    Info("  REF commits pte.val: 0x%lx, dut s2xlate: %d\n", pte.val, probe.s2xlate);
    Info("  REF commits ppn 0x%lx, DUT commits ppn 0x%lx\n", pte.difftest_ppn, probe.ppn);
    Info("  REF commits perm 0x%02x, level %d, pf %d\n", pte.difftest_perm, difftest_level, !pte.difftest_v);
  }

  return 0;
}

#endif // CONFIG_DIFFTEST_L1TLBEVENT
