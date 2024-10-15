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

#ifndef __TRACE_COMMON_H__
#define __TRACE_COMMON_H__

// when address translation's vpn is not in trace, give 0xa0000000 as paddr
// page (0xa0000000, 0xfff) for out of trace mmu
#define TRACE_PAGE_SIZE (4096)
#define TRACE_PAGE_SHIFT (12)
#define OUTOF_TRACE_PAGE_PADDR (0xa0000000)
#define OUTOF_TRACE_PPN (OUTOF_TRACE_PAGE_PADDR >> TRACE_PAGE_SHIFT)
#define DYN_PAGE_TABLE_BASE_PADDR (0x90000000)
#define TRACE_SATP64_PPN  (0x00000FFFFFFFFFFF)

#endif // __TRACE_COMMON_H__