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

#ifdef CONFIG_DIFFTEST_L2TLBEVENT

bool L2TLBChecker::get_valid(const DifftestL2TLBEvent &probe) {
  return probe.valid;
}

void L2TLBChecker::clear_valid(DifftestL2TLBEvent &probe) {
  probe.valid = 0;
}

int L2TLBChecker::check(const DifftestL2TLBEvent &probe, const DiffTestRegState &regs) {
  Satp *satp = (Satp *)&probe.satp;
  Satp *vsatp = (Satp *)&probe.vsatp;
  Hgatp *hgatp = (Hgatp *)&probe.hgatp;
  PTE pte;
  r_s2xlate r_s2;
  r_s2xlate check_s2;
  uint64_t paddr;
  uint8_t difftest_level;
  for (int j = 0; j < 8; j++) {
    if (probe.valididx[j]) {
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
        paddr = pg_base + VPNi(probe.vpn + j, difftest_level) * sizeof(uint64_t);
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
        if (probe.pteidx[j])
          check_s2 = r_s2;
      }
      bool difftest_gpf = !r_s2.pte.v || (!r_s2.pte.r && r_s2.pte.w);
      bool difftest_pf = !pte.v || (!pte.r && pte.w);
      bool s1_check_fail = pte.difftest_ppn != probe.ppn[j] || pte.difftest_perm != probe.perm ||
                            pte.difftest_pbmt != probe.pbmt || difftest_level != probe.level ||
                            difftest_pf != probe.pf;
      bool s2_check_fail = hasS2xlate ? r_s2.pte.difftest_ppn != probe.s2ppn ||
                                            r_s2.pte.difftest_perm != probe.g_perm ||
                                            r_s2.pte.difftest_pbmt != probe.g_pbmt ||
                                            r_s2.level != probe.g_level || difftest_gpf != probe.gpf
                                      : false;
      if (s1_check_fail || s2_check_fail) {
        Info("Warning: L2TLB resp test of core %d index %d sector %d failed! vpn = %lx\n", id, i, j,
              probe.vpn + j);
        Info("  REF commits ppn 0x%lx, perm 0x%02x, level %d, pf %d\n", pte.difftest_ppn, pte.difftest_perm,
              difftest_level, difftest_pf);
        if (hasS2xlate)
          Info("      s2_ppn 0x%lx, g_perm 0x%02x, g_level %d, gpf %d\n", r_s2.pte.difftest_ppn,
                r_s2.pte.difftest_perm, r_s2.level, difftest_gpf);
        Info("  DUT commits ppn 0x%lx, perm 0x%02x, level %d, pf %d\n", probe.ppn[j], probe.perm,
              probe.level, probe.pf);
        if (hasS2xlate)
          Info("      s2_ppn 0x%lx, g_perm 0x%02x, g_level %d, gpf %d\n", probe.s2ppn, probe.g_perm,
                probe.g_level, probe.gpf);
        return 1;
      }
    }
  }

  return 0;
}

#endif // CONFIG_DIFFTEST_L2TLBEVENT
