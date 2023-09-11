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

#include "perfhelper.h"

static const std::regex reg(R"(\.helper_?[0-9]*\.inner$)");

static std::atomic_int allocator(0);
static uint64_t perfCounters[50000] = {};

static uint64_t *cycles = nullptr;
static uint64_t begin = 0;
static uint64_t end = 0;
static bool clean = false;
static bool dump = false;

void perf_init(uint64_t perf_begin, uint64_t perf_end, uint64_t *perf_cycles) {
  begin = perf_begin;
  end = perf_end;
  cycles = perf_cycles;
}

uint64_t perf_get_begin() {
  return begin;
}

uint64_t perf_get_end() {
  return end;
}

void perf_set_begin(uint64_t perf_begin) {
  begin = perf_begin;
}

void perf_set_end(uint64_t perf_end) {
  end = perf_end;
}

void perf_set_clean() {
  clean = true;
}

void perf_set_dump() {
  dump = true;
}

void perf_unset_clean() {
  clean = false;
}

void perf_unset_dump() {
  dump = false;
}

extern "C" int register_perf_accumulate() {
  return allocator.fetch_add(1);
}

extern "C" int register_perf_histogram(int start, int stop, int step) {
  return allocator.fetch_add((stop - start) / step + 4);
}

extern "C" int register_perf_max() {
  return allocator.fetch_add(1);
}

extern "C" void handle_perf_accumulate(int id, int inc, const char *mod, const char *name) {
  uint64_t next = perfCounters[id] + inc;
  perfCounters[id] = clean ? 0 : next;
  if (dump) {
    std::string smod = std::regex_replace(mod, reg, "");
    fprintf(stderr, "[PERF ][time=%lu] %s: %s, %lu\n", *cycles, smod.c_str(), name, next);
  }
}

extern "C" void handle_perf_histogram(int id, int inc, unsigned char enable,
                                      int start, int stop, int step, int nBins,
                                      unsigned char left_strict, unsigned char right_strict,
                                      const char *mod, const char *name) {
  uint64_t *counters = perfCounters + id;
  uint64_t *left_out_counter = perfCounters + id + nBins;
  uint64_t *right_out_counter = perfCounters + id + nBins + 1;
  uint64_t *sum = perfCounters + id + nBins + 2;
  uint64_t *nSamples = perfCounters + id + nBins + 3;

  if (dump) {
    std::string smod = std::regex_replace(mod, reg, "");
    fprintf(stderr, "[PERF ][time=%lu] %s: %s_mean, %lu\n", *cycles, smod.c_str(), name, *nSamples ? *sum / *nSamples : 0);
    for (int i = 0; i < nBins; i++) {
      int range_start = start + i * step;
      int range_end = start + (i + 1) * step;
      uint64_t ctr;
      if (i == 0 && !left_strict)
        ctr = counters[i] + *left_out_counter;
      else if (i == nBins - 2 && !right_strict)
        ctr = counters[i] + *right_out_counter;
      else
        ctr = counters[i];
      fprintf(stderr, "[PERF ][time=%lu] %s: %s_%d_%d, %lu\n", *cycles, smod.c_str(), name, range_start, range_end, ctr);
    }
  }

  for (int i = 0; i < nBins; i++) {
    int range_start = start + i * step;
    int range_end = start + (i + 1) * step;
    bool in_range = inc >= range_start && inc < range_end;
    if (clean)
      counters[i] = 0;
    else if (enable && in_range)
      counters[i]++;
  }

  bool left_out = left_strict ? false : inc < start;
  if (clean)
    *left_out_counter = 0;
  else if (enable && left_out)
    *left_out_counter += 1;

  bool right_out = right_strict ? false : inc >= stop;
  if (clean)
    *right_out_counter = 0;
  else if (enable && right_out)
    *right_out_counter += 1;

  if (clean)
    *sum = *nSamples = 0;
  else if (enable) {
    *sum += inc;
    *nSamples += 1;
  }
}

extern "C" void handle_perf_max(int id, int ctr, unsigned char enable, const char *mod, const char *name) {
  uint64_t next = enable && ctr > perfCounters[id] ? ctr : perfCounters[id];
  perfCounters[id] = clean ? 0 : next;
  if (dump) {
    std::string smod = std::regex_replace(mod, reg, "");
    fprintf(stderr, "[PERF ][time=%lu] %s: %s_max, %lu\n", *cycles, smod.c_str(), name, next);
  }
}
