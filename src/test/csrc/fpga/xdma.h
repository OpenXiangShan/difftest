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

#define HOST_IO_CFG_RESET       0x0
#define HOST_IO_RESET           0x4
#define HOST_IO_DIFFTEST_ENABLE 0x8
#define HOST_IO_ILA_TRIGGER     0xc
#define HOST_IO_SQUASH_ENABLE   0x10
#define HOST_IO_SEED            0x14
#define HOST_IO_RAM_SIZE_MB     0x18
#define HOST_IO_MEM_INIT        0x1c
#define HOST_IO_MEM_CPU         0x20
#define HOST_IO_MEM_H2C         0x24
#define HOST_IO_H2C_SIZE_MB     0x28

#define DMA_PACKGE_NUM 8
// DMA_PADDING (packge_idx(1) + difftest_data) send width to be calculated by mod up
#define DMA_PACKGE_LEN (CONFIG_DIFFTEST_BATCH_BYTELEN + 1)
#define DMA_PACKGE_ALIGNED                                                                    \
  ((DMA_PACKGE_LEN + CONFIG_DIFFTEST_HOST_AXIS_BYTES - 1) / CONFIG_DIFFTEST_HOST_AXIS_BYTES * \
   CONFIG_DIFFTEST_HOST_AXIS_BYTES)
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
    if (enable_diff == false) {
      while (signal_num == 0) {
        usleep(10000);
      }
      running = false;
    } else {
#ifdef USE_THREAD_MEMPOOL
      start_transmit_thread();
      while (running && signal_num == 0) {
        usleep(10000);
      }
      running = false;
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

  void fpga_io(uint64_t address, uint32_t value) {
    device_write(false, nullptr, address, value);
  }

  void fpga_io(uint64_t address, bool enable) {
    fpga_io(address, enable ? 1u : 0u);
  }

  uint32_t fpga_io_read(uint64_t address) {
    return device_read(false, address);
  }

  void wait_fpga_io_done(uint64_t address, const char *tag);
#ifdef CONFIG_USE_XDMA_H2C
  void h2c_load_workload(const void *payload, uint64_t size);
#endif

private:
  bool running = false;
  int xdma_c2h_fd[CONFIG_DMA_CHANNELS];
#ifdef CONFIG_USE_XDMA_H2C
  int xdma_h2c_fd;
#endif

  void device_write(bool is_bypass, const char *workload, uint64_t addr, uint64_t value);
  uint32_t device_read(bool is_bypass, uint64_t addr);

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
