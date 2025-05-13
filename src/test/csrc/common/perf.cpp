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

void difftest_perfcnt_finish(uint64_t cycleCnt) {
  printf("==================== Difftest PerfCnt ====================\n");
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  perf_run_msec = ts.tv_sec * 1000 + ts.tv_nsec / 1000000 - perf_run_msec;
  printf("Run time: %lld s %lld ms\n", perf_run_msec / 1000, perf_run_msec % 1000);
  float speed = (float)cycleCnt / (float)perf_run_msec;
  printf("Simulation speed: %.2f KHz\n", speed);
  printf("%30s %15s %17s %16s %18s\n", "DPIC_FUNC", "DPIC_CALLS", "DPIC_CALLS/s", "DPIC_BYTES", "DPIC_BYTES/s");
  printf(">>> DiffState Func\n");
  diffstate_perfcnt_finish(perf_run_msec);
  printf(">>> Other Difftest Func\n");
  const char *func_name[DIFFTEST_PERF_NUM] = {
    "difftest_nstep", "difftest_ram_read", "difftest_ram_write", "flash_read", "sd_set_addr", "sd_read",
    "jtag_tick",      "put_pixel",         "vmem_sync",          "pte_helper", "amo_helper",
  };
  for (int i = 0; i < DIFFTEST_PERF_NUM; i++) {
    difftest_perfcnt_print(func_name[i], difftest_calls[i], difftest_bytes[i], perf_run_msec);
  }
}
#endif
