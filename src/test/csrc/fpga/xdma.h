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
  BatchInfo difftest_batch_info;
  uint8_t packge_idx;
} FpgaPackgeHead;

class FpgaXdma {
public:
  struct FpgaPackgeHead *shmadd_recv;

  MemoryIdxPool xdma_mempool;

  int xdma_c2h_fd[CONFIG_DMA_CHANNELS];
  int xdma_h2c_fd;

  bool running = false;

  std::condition_variable diff_filled_cv;
  std::condition_variable diff_empile_cv;

  std::atomic<uint32_t> diff_packge_count{0};

  FpgaXdma();
  ~FpgaXdma() {
    stop_thansmit_thread();
  };

  // thread api
  void start_transmit_thread();
  void stop_thansmit_thread();
  void read_xdma_thread(int channel);
  void write_difftest_thread();

private:
  std::thread receive_thread[CONFIG_DMA_CHANNELS];
  std::thread process_thread;

  static void handle_sigint(int sig);
};

#endif
