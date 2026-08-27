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
#include <cstdint>
#include <fstream>
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

#define HOST_IO_CFG_RESET        0x0
#define HOST_IO_RESET            0x4
#define HOST_IO_DIFFTEST_ENABLE  0x8
#define HOST_IO_ILA_TRIGGER      0xc
#define HOST_IO_SQUASH_ENABLE    0x10
#define HOST_IO_SQUASH_MAX_FUSED 0x14
#define HOST_IO_SEED             0x18
#define HOST_IO_RAM_SIZE_MB      0x1c
#define HOST_IO_MEM_INIT         0x20
#define HOST_IO_MEM_CPU          0x24
#define HOST_IO_MEM_H2C          0x28
#define HOST_IO_H2C_SIZE_MB      0x2c

#define HOST_IO_REPLAY_TRACE_FREEZE     0x30
#define HOST_IO_REPLAY_TRACE_HEAD       0x34
#define HOST_IO_REPLAY_TRACE_SIZE       0x38
#define HOST_IO_REPLAY_TRACE_DUMP       0x3c
#define HOST_IO_REPLAY_TRACE_REARM      0x40
#define HOST_IO_REPLAY_TRACE_STATUS     0x44
#define HOST_IO_REPLAY_TRACE_WRITE_PTR  0x48
#define HOST_IO_REPLAY_TRACE_WRITE_SEQ  0x4c
#define HOST_IO_REPLAY_TRACE_DUMP_START 0x50
#define HOST_IO_REPLAY_TRACE_DUMP_BEATS 0x54

#define REPLAY_TRACE_STATUS_FROZEN       (1u << 0)
#define REPLAY_TRACE_STATUS_DUMP_ACTIVE (1u << 1)
#define REPLAY_TRACE_STATUS_DUMP_DONE   (1u << 2)
#define REPLAY_TRACE_STATUS_RANGE_LOST  (1u << 3)
#define REPLAY_TRACE_HEADER_RANGE_LOST  (1u << 0)

#ifdef CONFIG_DIFFTEST_REPLAY_TRACE
typedef struct __attribute__((packed)) {
  uint64_t magic;
  uint32_t version;
  uint32_t flags;
  uint16_t trace_head;
  uint16_t trace_size;
  uint32_t dump_start;
  uint32_t dump_words;
  uint32_t snapshot_ptr;
  uint32_t snapshot_seq;
  uint8_t reserved[28];
} ReplayTraceDumpHeader;

static_assert(sizeof(ReplayTraceDumpHeader) == CONFIG_DIFFTEST_BATCH_BYTELEN,
              "Replay trace header must occupy one batch beat");
#endif // CONFIG_DIFFTEST_REPLAY_TRACE

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
#ifdef FPGA_SIM
    for (int i = 0; i < CONFIG_DMA_CHANNELS; i++) {
      xdma_sim_cancel(i);
    }
#endif // FPGA_SIM
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

#ifdef CONFIG_DIFFTEST_REPLAY_TRACE
  bool queue_replay_trace(uint16_t trace_head, uint16_t trace_size);
  bool replay_trace_in_progress() const {
    return replay_trace_pending || replay_trace_requested;
  }
  bool consume_replay_trace_packet(const uint8_t *payload);
#endif // CONFIG_DIFFTEST_REPLAY_TRACE

  void wait_fpga_io_done(uint64_t address, const char *tag);
#ifdef CONFIG_USE_XDMA_H2C
  void h2c_load_workload(const void *payload, uint64_t size);
#endif

private:
  std::atomic<bool> running{false};
  int xdma_c2h_fd[CONFIG_DMA_CHANNELS];
#ifdef CONFIG_USE_XDMA_H2C
  int xdma_h2c_fd;
#endif

  void device_write(bool is_bypass, const char *workload, uint64_t addr, uint64_t value);
  uint32_t device_read(bool is_bypass, uint64_t addr);

#ifdef CONFIG_DIFFTEST_REPLAY_TRACE
  bool replay_trace_pending = false;
  bool replay_trace_requested = false;
  bool replay_trace_header_seen = false;
  uint16_t replay_trace_pending_head = 0;
  uint16_t replay_trace_pending_size = 0;
  uint32_t replay_trace_discarded = 0;
  uint32_t replay_trace_remaining = 0;
  uint32_t replay_trace_packets = 0;
  std::ofstream replay_trace_file;

  bool service_replay_trace_request();
#endif // CONFIG_DIFFTEST_REPLAY_TRACE

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
