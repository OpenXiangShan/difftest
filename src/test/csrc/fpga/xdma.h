/***************************************************************************************
* Copyright (c) 2025 Beijing Institute of Open Source Chip (BOSC)
* Copyright (c) 2020-2025 Institute of Computing Technology, Chinese Academy of Sciences
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
#include "mpool.h"
#include <atomic>
#include <queue>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/shm.h>
#include <thread>
#include <vector>
#ifdef FPGA_SIM
#include "xdma_sim.h"
#endif // FPGA_SIM

#define DMA_PACKGE_NUM 8

// DMA_PADDING (packge_idx(1) + difftest_data) send width to be calculated by mod up
#define DMA_PACKGE_LEN     (CONFIG_DIFFTEST_BATCH_BYTELEN + 1)
#define DMA_PACKGE_ALIGNED ((DMA_PACKGE_LEN + 63) / 64 * 64)
#define DMA_PACKGE_PADDING (DMA_PACKGE_ALIGNED - DMA_PACKGE_LEN)

typedef struct __attribute__((packed)) {
  uint8_t packge_idx; // idx of header packet is valid and idx of intermediate data is placeholder
  uint8_t diff_packge[CONFIG_DIFFTEST_BATCH_BYTELEN];
#if (DMA_PACKGE_PADDING > 0)
  uint8_t padding[DMA_PACKGE_PADDING];
#endif
} DmaDiffPackge;

typedef struct __attribute__((packed)) {
  DmaDiffPackge diff_packge[DMA_PACKGE_NUM];
} FpgaPackgeHead;

class FpgaXdma {
public:
  FpgaXdma();

  void start() {
    running = true;
#ifdef USE_THREAD_MEMPOOL
    std::unique_lock<std::mutex> lock(thread_mtx);
    start_transmit_thread();
    while (running) {
      thread_cv.wait(lock); // wait notify from stop
    }
    stop_thansmit_thread();
#else
    read_and_process();
#endif // USE_THREAD_MEMPOOL
  }
  void stop() {
    running = false;
#ifdef USE_THREAD_MEMPOOL
    thread_cv.notify_one();
#endif // USE_THREAD_MEMPOOL
  }
  void ddr_load_workload(const char *workload) {
    core_reset();
    device_write(true, workload, 0, 0);
    core_restart();
  }

  void fpga_reset_io(bool enable) {
    if (enable)
      device_write(false, nullptr, 0x0, 0x1);
    else
      device_write(false, nullptr, 0x0, 0x0);
  }

private:
  bool running = false;
  int xdma_c2h_fd[CONFIG_DMA_CHANNELS];
#ifdef CONFIG_USE_XDMA_H2C
  int xdma_h2c_fd;
#endif

  void device_write(bool is_bypass, const char *workload, uint64_t addr, uint64_t value);
  void core_reset() {
    device_write(false, nullptr, 0x20000, 0x1);
    device_write(false, nullptr, 0x100000, 0x1);
    device_write(false, nullptr, 0x10000, 0x8);
  }

  void core_restart() {
    device_write(false, nullptr, 0x20000, 0);
    device_write(false, nullptr, 0x100000, 0);
  }

#ifdef USE_THREAD_MEMPOOL
  std::mutex thread_mtx;
  std::condition_variable thread_cv;
  MemoryIdxPool xdma_mempool;
  std::thread receive_thread[CONFIG_DMA_CHANNELS];
  std::thread process_thread;
  // thread api
  void start_transmit_thread();
  void stop_thansmit_thread();
  void read_xdma_thread(int channel);
  void write_difftest_thread();
#else
  void read_and_process();
#endif // USE_THREAD_MEMPOOL
};

#endif
