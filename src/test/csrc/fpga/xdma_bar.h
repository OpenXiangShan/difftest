/***************************************************************************************
* Copyright (c) 2025-2026 Beijing Institute of Open Source Chip (BOSC)
* Copyright (c) 2020-2026 Institute of Computing Technology, Chinese Academy of Sciences
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
#ifndef __XDMA_BAR_H__
#define __XDMA_BAR_H__

#define HOST_IO_CFG_RESET       0x0
#define HOST_IO_RESET           0x4
#define HOST_IO_DIFFTEST_ENABLE 0x8
#define HOST_IO_ILA_TRIGGER     0xc
#define HOST_IO_SQUASH_ENABLE   0x10
#define HOST_IO_SEED            0x14
#define HOST_IO_RAM_SIZE_MB     0x18
#define HOST_IO_MEM_INIT        0x1c
#define HOST_IO_MEM_CPU         0x20
#define HOST_IO_MEM_H2C         0x24
#define HOST_IO_H2C_SIZE_MB     0x28
#define HOST_IO_REPLAY_SIZE_MB  0x2c
#define HOST_IO_REPLAY_BASE     0x30
#define HOST_IO_REPLAY_WR_PTR   0x34
#define HOST_IO_REPLAY_WRAP_CNT 0x38
#define HOST_IO_REPLAY_DUMP     0x3c

#endif // __XDMA_BAR_H__
