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
 */
#ifndef __GOLDEN_H__
#define __GOLDEN_H__

#include "common.h"

// REF Models
extern "C" uint8_t pte_helper(uint64_t satp, uint64_t vpn, uint64_t *pte, uint8_t *level);
extern "C" uint64_t amo_helper(uint8_t cmd, uint64_t addr, uint64_t wdata, uint8_t mask);

typedef union PageTableEntry {
  struct {
    uint32_t v : 1;
    uint32_t r : 1;
    uint32_t w : 1;
    uint32_t x : 1;
    uint32_t u : 1;
    uint32_t g : 1;
    uint32_t a : 1;
    uint32_t d : 1;
    uint32_t rsw : 2;
    uint64_t ppn : 44;
    uint32_t rsvd : 7;
    uint32_t pbmt : 2;
    uint32_t n : 1;
  };
  struct {
    uint32_t difftest_v : 1;
    uint32_t difftest_perm : 7;
    uint32_t difftest_rsw : 2;
    uint64_t difftest_ppn : 44;
    uint32_t difftest_rsvd : 7;
    uint32_t difftest_pbmt : 2;
    uint32_t difftest_n : 1;
  };
  uint64_t val;
} PTE;

typedef union atpStruct {
  struct {
    uint64_t ppn : 44;
    uint32_t asid : 16;
    uint32_t mode : 4;
  };
  uint64_t val;
} Satp, Hgatp;
#define noS2xlate           0
#define onlyStage1          1
#define onlyStage2          2
#define allStage            3
#define VPNiSHFT(i)         (12 + 9 * (i))
#define GVPNi(addr, i, max) (((addr) >> (9 * (i) + 12)) & ((i == 3 || (i == 2 && max == 2)) ? 0x7ff : 0x1ff))
#define VPNi(vpn, i)        (((vpn) >> (9 * (i))) & 0x1ff)
#endif
