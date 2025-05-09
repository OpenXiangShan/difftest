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
#include "diffstate.h"
#include "mpool.h"
#include <atomic>
#include <queue>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/shm.h>
#include <thread>
#include <vector>

#ifdef CONFIG_DIFFTEST_BATCH
#define DMA_DIFF_PACKGE_LEN (CONFIG_DIFFTEST_BATCH_BYTELEN)
#elif defined(CONFIG_DIFFTEST_SQUASH)
#define DMA_DIFF_PACKGE_LEN 1280 // XDMA Min size
#endif
// DMA_PADDING (packge_idx(1) + difftest_data) send width to be calculated by mod up
#define DMA_PADDING (((1 + DMA_DIFF_PACKGE_LEN + 63) / 64) * 64 - (DMA_DIFF_PACKGE_LEN + 1))

typedef struct __attribute__((packed)) {
  uint8_t idx;
  uint8_t diff_packge[DMA_DIFF_PACKGE_LEN];
#if (DMA_PADDING != 0)
  uint8_t padding[DMA_PADDING];
#endif
} FpgaPackgeHead;

class FpgaXdma {
public:
  MemoryIdxPool xdma_mempool;

  bool running = false;

  FpgaXdma();
  ~FpgaXdma() {
    stop_thansmit_thread();
  };

  void core_reset() {
    device_write(false, nullptr, 0x20000, 0x1);
    device_write(false, nullptr, 0x100000, 0x1);
    device_write(false, nullptr, 0x10000, 0x8);
  }

  void core_restart() {
    device_write(false, nullptr, 0x20000, 0);
    device_write(false, nullptr, 0x100000, 0);
  }

  void ddr_load_workload(const char *workload) {
    core_reset();
    device_write(true, workload, 0, 0);
    core_restart();
  }

  void device_write(bool is_bypass, const char *workload, uint64_t addr, uint64_t value);

  void *posix_memalignd_malloc(size_t size) {
    void *ptr = nullptr;
    int ret = posix_memalign(&ptr, 4096, size);
    if (ret != 0) {
      perror("posix_memalign failed");
      return nullptr;
    }
    return ptr;
  }

  // thread api
  void start_transmit_thread();
  void stop_thansmit_thread();
  void read_xdma_thread(int channel);
  void write_difftest_thread();

private:
  std::thread receive_thread[CONFIG_DMA_CHANNELS];
  std::thread process_thread;

  int xdma_c2h_fd[CONFIG_DMA_CHANNELS];
#ifdef CONFIG_USE_XDMA_H2C
  int xdma_h2c_fd;
#endif

  static void handle_sigint(int sig);
};

#endif
