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
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/shm.h>
#include <thread>
#include <unistd.h>
#include <vector>
#ifdef FPGA_SIM
#include "xdma_sim.h"
#endif // FPGA_SIM

// Config BAR register map
#define HOST_IO_RESET           0x00
#define HOST_IO_DIFFTEST_ENABLE 0x04
#define HOST_IO_DDR_ARB_SEL     0x08
#define HOST_IO_H2C_LENGTH      0x0C
#define HOST_IO_H2C_STATUS      0x10
#define HOST_IO_H2C_BEAT_CNT    0x14
#define HOST_IO_H2C_BEAT_BYTES  0x18

#ifndef CONFIG_DMA_CHANNELS
#define CONFIG_DMA_CHANNELS 1
#endif

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
  ~FpgaXdma();

  void start(bool enable_diff) {
    running = true;
#ifndef FPGA_SIM
    fpga_io(HOST_IO_DIFFTEST_ENABLE, enable_diff);
#endif // FPGA_SIM
    if (enable_diff == false) {
      static volatile sig_atomic_t signal_received = 0;

      auto handler = [](int sig) {
        signal_received = sig;
        printf("\nReceived signal %d, terminating...\n", sig);
        exit(0);
      };

      signal(SIGINT, handler);
      signal(SIGTERM, handler);

      while (signal_received == 0) {
        usleep(10000);
      }
    } else {
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
  }

  void stop() {
    running = false;
#ifdef USE_THREAD_MEMPOOL
    thread_cv.notify_one();
#endif // USE_THREAD_MEMPOOL
  }
#ifndef FPGA_SIM
  bool h2c_load_workload();
#endif // FPGA_SIM

  void fpga_io(uint64_t address, bool enable);

private:
  bool running = false;
  int xdma_c2h_fd[CONFIG_DMA_CHANNELS];
  int xdma_user_fd;
  int xdma_h2c_fd;

  void device_write(uint64_t addr, uint64_t value);
  void close_device_fds();
#ifndef FPGA_SIM
  bool ensure_user_fd_open();
  bool ensure_h2c_fd_open();
  bool config_bar_access32(uint32_t offset, uint32_t *value, bool is_write);
  bool config_bar_wait_mask(uint32_t offset, uint32_t mask, uint32_t expect, int timeout_ms, uint32_t *readback);
  bool config_bar_write32(uint32_t offset, uint32_t value);
  bool config_bar_read32(uint32_t offset, uint32_t *value);
  void dump_h2c_debug_regs(const char *tag);
  bool h2c_stream_write_all(const uint8_t *buf, size_t len, size_t *written_out = nullptr);
  bool h2c_init_sequence(uint32_t beats);
  bool h2c_complete_sequence(uint32_t expect_beats);
#endif // FPGA_SIM
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
