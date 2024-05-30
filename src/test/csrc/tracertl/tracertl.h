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

#ifndef __TRACERTL_H__
#define __TRACERTL_H__

#include "trace_format.h"

extern "C" void init_tracertl(const char *trace_file_name);

extern "C" void init_traceicache(const char *binary_name);
extern "C" void trace_icache_helper(uint64_t addr, uint8_t *res_valid, uint64_t *data0, uint64_t *data1, uint64_t *data2, uint64_t *data3, uint64_t *data4, uint64_t *data5, uint64_t *data6, uint64_t *data7);
extern "C" uint64_t trace_icache_dword_helper(uint64_t addr);
extern "C" uint8_t trace_icache_legal_addr(uint64_t addr);
extern "C" void trace_read_one_instr(uint8_t enable, uint64_t *pc, uint32_t *instr);


Instruction read_one_trace();
extern "C" bool read_one_trace_bare(uint64_t *pc, uint32_t *instr);

#endif