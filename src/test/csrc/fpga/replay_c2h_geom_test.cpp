/***************************************************************************************
* Copyright (c) 2026 Beijing Institute of Open Source Chip (BOSC)
***************************************************************************************/
#include "replay_range.h"
#include "xdma_bar.h"
#include <stdio.h>
#include <stdlib.h>

#ifndef CONFIG_DIFFTEST_BATCH_BYTELEN
#define CONFIG_DIFFTEST_BATCH_BYTELEN 64
#endif
#ifndef CONFIG_DIFFTEST_HOST_AXIS_BYTES
#define CONFIG_DIFFTEST_HOST_AXIS_BYTES 32
#endif

static void expect_eq(const char *name, uint32_t got, uint32_t want) {
  if (got != want) {
    fprintf(stderr, "FAIL %s: got %u want %u\n", name, got, want);
    exit(1);
  }
}

int main() {
  expect_eq("bar.size", HOST_IO_REPLAY_SIZE_MB, 0x2c);
  expect_eq("bar.base", HOST_IO_REPLAY_BASE, 0x30);
  expect_eq("bar.wr", HOST_IO_REPLAY_WR_PTR, 0x34);
  expect_eq("bar.wrap", HOST_IO_REPLAY_WRAP_CNT, 0x38);
  expect_eq("bar.dump", HOST_IO_REPLAY_DUMP, 0x3c);
  uint32_t geom = replay_c2h_range_bytes(512, 256);
  expect_eq("geom.768", geom, 768);
  ReplayAxiRange range = replay_axi_range(0, 768, 0, 1, geom);
  expect_eq("one.range", range.len, 768);
  printf("replay_c2h_geom_test PASS dump=0x%x geom=%u\n", HOST_IO_REPLAY_DUMP, geom);
  return 0;
}
