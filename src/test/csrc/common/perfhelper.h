/***************************************************************************************
* Copyright (c) 2020-2021 Institute of Computing Technology, Chinese Academy of Sciences
* Copyright (c) 2020-2021 Peng Cheng Laboratory
*
* XiangShan is licensed under Mulan PSL v2.
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

#ifndef __PERFHELPER_H
#define __PERFHELPER_H

#include <stdio.h>
#include <stdint.h>
#include <atomic>
#include <string.h>
#include <regex>

void perf_init(uint64_t perf_begin, uint64_t perf_end, uint64_t *perf_cycles);

uint64_t perf_get_begin();

uint64_t perf_get_end();

void perf_set_begin(uint64_t perf_begin);

void perf_set_end(uint64_t perf_end);

void perf_set_clean();

void perf_set_dump();

void perf_unset_clean();

void perf_unset_dump();

#endif
