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

#ifdef CONFIG_DIFFTEST_PERFCNT
#include "perf.h"
#include <inttypes.h>
#include <time.h>

long long perf_run_msec = 0;
long long difftest_calls[DIFFTEST_PERF_NUM] = {0}, difftest_bytes[DIFFTEST_PERF_NUM] = {0};

void difftest_perfcnt_init() {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  perf_run_msec = ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
  for (int i = 0; i < DIFFTEST_PERF_NUM; i++) {
    difftest_calls[i] = 0;
    difftest_bytes[i] = 0;
  }
  diffstate_perfcnt_init();
}

void difftest_perfcnt_finish(uint64_t cycleCnt, uint64_t instrCnt, const char *name, bool print_dpic) {
  printf("==================== %s PerfCnt ====================\n", name ? name : "Difftest");
  if (name)
    printf("instrCnt = %" PRIu64 ", cycleCnt = %" PRIu64 ", IPC = %.6f\n", instrCnt, cycleCnt,
           cycleCnt ? double(instrCnt) / cycleCnt : 0);
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  const long long elapsed_msec = ts.tv_sec * 1000 + ts.tv_nsec / 1000000 - perf_run_msec;
  const long long rate_msec = elapsed_msec > 0 ? elapsed_msec : 1;
  printf("Run time: %lld s %lld ms\n", elapsed_msec / 1000, elapsed_msec % 1000);
  float khz = (float)cycleCnt / (float)rate_msec;
  float kips = (float)instrCnt / (float)rate_msec;
  printf("Simulation speed: ");
  if (khz < 1000) {
    printf("%.2f KHz, %.2f KIPS\n", khz, kips);
  } else {
    printf("%.2f MHz, %.2f MIPS\n", khz / 1000, kips / 1000);
  }
  if (name && !print_dpic)
    return;
  printf("%30s %15s %17s %16s %18s\n", "DPIC_FUNC", "DPIC_CALLS", "DPIC_CALLS/s", "DPIC_BYTES", "DPIC_BYTES/s");
  printf(">>> DiffState Func\n");
  diffstate_perfcnt_finish(rate_msec);
  printf(">>> Other Difftest Func\n");
  const char *func_name[DIFFTEST_PERF_NUM] = {
    "difftest_nstep", "difftest_ram_read", "difftest_ram_write", "flash_read", "sd_set_addr", "sd_read",
    "jtag_tick",      "put_pixel",         "vmem_sync",          "pte_helper", "amo_helper",
  };
  for (int i = 0; i < DIFFTEST_PERF_NUM; i++) {
    difftest_perfcnt_print(func_name[i], difftest_calls[i], difftest_bytes[i], rate_msec);
  }
}
#endif
