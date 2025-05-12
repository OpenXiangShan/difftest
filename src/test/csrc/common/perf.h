/***************************************************************************************
* Copyright (c) 2020-2024 Institute of Computing Technology, Chinese Academy of Sciences
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

#ifndef __PERF_H__
#define __PERF_H__

#include "common.h"

#ifdef CONFIG_DIFFTEST_PERFCNT
static inline void difftest_perfcnt_print(const char *name, long long calls, long long bytes, long long msec) {
  long long calls_mean = calls * 1000 / msec;
  long long bytes_mean = bytes * 1000 / msec;
  printf("%30s %15lld %15lld/s %15lldB %15lldB/s\n", name, calls, calls_mean, bytes, bytes_mean);
}
void difftest_perfcnt_init();
void difftest_perfcnt_finish(uint64_t cycleCnt);
enum DIFFTEST_PERF {
  perf_difftest_nstep,
  perf_difftest_ram_read,
  perf_difftest_ram_write,
  perf_flash_read,
  perf_sd_set_addr,
  perf_sd_read,
  perf_jtag_tick,
  perf_put_pixel,
  perf_vmem_sync,
  perf_pte_helper,
  perf_amo_helper,
  DIFFTEST_PERF_NUM
};
extern long long difftest_calls[DIFFTEST_PERF_NUM], difftest_bytes[DIFFTEST_PERF_NUM];
#endif // CONFIG_DIFFTEST_PERFCNT

#endif
