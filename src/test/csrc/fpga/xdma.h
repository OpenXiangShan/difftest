/***************************************************************************************
* Copyright (c) 2024 Beijing Institute of Open Source Chip (BOSC)
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
#ifndef __XDMA_H__
#define __XDMA_H__

#include "common.h"
#include "diffstate.h"
#include "difftest-dpic.h"
#include "mpool.h"
#include <atomic>
#include <queue>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/shm.h>
#include <thread>
#include <vector>

#define WITH_FPGA

typedef struct FpgaPackgeHead {
  uint8_t packge_idx;
  char diff_batch_pack[CONFIG_DIFFTEST_BATCH_BYTELEN];
} FpgaPackgeHead;

class FpgaXdma {
public:
  MemoryIdxPool xdma_mempool;

  bool running = false;

  std::atomic<uint32_t> diff_packge_count{0};

  FpgaXdma(const char *workload);
  ~FpgaXdma() {
    stop_thansmit_thread();
  };

  void core_reset() {
    device_write(false, nullptr, 0x100000, 0x1);
    device_write(false, nullptr, 0x10000, 0x8);
  }

  void core_restart() {
    device_write(false, nullptr, 0x100000, 0);
  }

  void ddr_load_workload(const char *workload) {
    core_reset();
    device_write(true, workload, 0, 0);
    core_restart();
  }

  void device_write(bool is_bypass, const char *workload, uint64_t addr, uint64_t value);

  // thread api
  void start_transmit_thread();
  void stop_thansmit_thread();
  void read_xdma_thread(int channel);
  void write_difftest_thread();

private:
  std::thread receive_thread[CONFIG_DMA_CHANNELS];
  std::thread process_thread;

  int xdma_c2h_fd[CONFIG_DMA_CHANNELS];
  int xdma_h2c_fd;

  static void handle_sigint(int sig);
};

#endif
