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

/**
 * Headers for C/C++ REF models
 * Shared definitions for golden.cpp and tlb.cpp
 */
#ifndef __GOLDEN_H__
#define __GOLDEN_H__

#include "common.h"

/**
 * Page table walk constants
 */
#define PAGE_SHIFT 12
#define PAGE_SIZE  (1ul << PAGE_SHIFT)
#define PAGE_MASK  (PAGE_SIZE - 1)

/**
 * Two-stage translation mode constants
 * Aligned with MMUConst.scala definitions
 */
#define noS2xlate  0  // Single-stage translation using satp
#define onlyStage1 1  // VS-stage only using vsatp
#define onlyStage2 2  // G-stage only using hgatp
#define allStage   3  // Full two-stage translation (GVA -> GPA -> HPA)

/**
 * Page fault type constants
 * Return values from pte_helper function
 */
#define PF_NONE     0  // No page fault, translation successful
#define PF_VS_STAGE 1  // VS-stage page fault
#define PF_G_STAGE  2  // G-stage page fault (guest page fault)

/**
 * Page table index extraction macros
 */
#define VPNiSHFT(i) (12 + 9 * (i))
#define NAPOTSHFT   (12 + 4)  // only support 64kb page

// Extract VPN index at level i from VPN (9 bits each level)
#define VPNi(vpn, i) (((vpn) >> (9 * (i))) & 0x1ff)

// Extract GPN index for G-stage (11 bits for top level in Sv39x4/Sv48x4)
#define GVPNi(addr, i, max) (((addr) >> (9 * (i) + 12)) & ((i == 3 || (i == 2 && max == 2)) ? 0x7ff : 0x1ff))

/**
 * Page Table Entry (PTE) structure
 * RISC-V Sv39/Sv48 format
 */
typedef union PageTableEntry {
  struct {
    uint64_t v : 1;      // Valid bit
    uint64_t r : 1;      // Read permission
    uint64_t w : 1;      // Write permission
    uint64_t x : 1;      // Execute permission
    uint64_t u : 1;      // User mode accessible
    uint64_t g : 1;      // Global mapping
    uint64_t a : 1;      // Accessed bit
    uint64_t d : 1;      // Dirty bit
    uint64_t rsw : 2;    // Reserved for software
    uint64_t ppn : 44;   // Physical page number
    uint64_t rsvd : 7;   // Reserved
    uint64_t pbmt : 2;   // Page-based memory type
    uint64_t n : 1;      // NAPOT bit
  };
  struct {
    uint64_t difftest_v : 1;
    uint64_t difftest_perm : 7;
    uint64_t difftest_rsw : 2;
    uint64_t difftest_ppn : 44;
    uint64_t difftest_rsvd : 7;
    uint64_t difftest_pbmt : 2;
    uint64_t difftest_n : 1;
  };
  uint64_t val;
} PTE;

/**
 * Address Translation and Protection (ATP) register structure
 * Used for satp, vsatp, and hgatp registers
 */
typedef union atpStruct {
  struct {
    uint64_t ppn : 44;   // Page table base PPN
    uint32_t asid : 16;  // Address space identifier (or VMID for hgatp)
    uint32_t mode : 4;   // Translation mode (8=Sv39, 9=Sv48)
  };
  uint64_t val;
} Satp, Vsatp, Hgatp;

/**
 * G-stage translation result structure
 * Used by do_s2xlate() in tlb.cpp and do_g_stage() in golden.cpp
 */
typedef struct {
  PTE pte;
  uint8_t level;
} r_s2xlate;

/**
 * pte_helper: Software page table walk with H-extension support
 *
 * @param satp    Host SATP register (for noS2xlate mode)
 * @param vsatp   Guest VSATP register (for VS-stage translation)
 * @param hgatp   Hypervisor HGATP register (for G-stage translation)
 * @param vpn     Virtual page number to translate
 * @param s2xlate Translation mode (noS2xlate/onlyStage1/onlyStage2/allStage)
 * @param pte     Output: final PTE value
 * @param level   Output: page table level (0=4KB, 1=2MB, 2=1GB, 3=512GB)
 * @return        Page fault type (PF_NONE/PF_VS_STAGE/PF_G_STAGE)
 */
extern "C" uint8_t pte_helper(
    uint64_t satp,
    uint64_t vsatp,
    uint64_t hgatp,
    uint64_t vpn,
    uint8_t  s2xlate,
    uint64_t *pte,
    uint8_t  *level,
    uint64_t *s1_pte,
    uint64_t *s2_pte,
    uint8_t  *s1_level
);

extern "C" uint64_t amo_helper(uint8_t cmd, uint64_t addr, uint64_t wdata, uint8_t mask);

#endif // __GOLDEN_H__
