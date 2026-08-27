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
#include "xdma.h"
#include "difftest-dpic.h"
#include "mpool.h"
#include "ram.h"
#include <algorithm>
#include <cstring>
#include <errno.h>
#include <execinfo.h>
#include <fcntl.h>
#include <fstream>
#include <inttypes.h>
#include <iostream>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <sys/mman.h>
#include <unistd.h>

#define XDMA_USER       "/dev/xdma0_user"
#define XDMA_BYPASS     "/dev/xdma0_bypass"
#define XDMA_C2H_DEVICE "/dev/xdma0_c2h_"
#define XDMA_H2C_DEVICE "/dev/xdma0_h2c_0"
static const size_t H2C_AXIS_BYTES = CONFIG_DIFFTEST_HOST_AXIS_BYTES;

void signal_handler(int sig) {
  void *array[20];
  size_t size;
  size = backtrace(array, 20);

  fprintf(stderr, "Error: signal %d:\n", sig);
  backtrace_symbols_fd(array, size, STDERR_FILENO);
  exit(1);
}

template <typename Func, typename Obj, typename... Args> void thread_wrapper(Func func, Obj obj, Args... args) {
  signal(SIGSEGV, signal_handler);
  (obj->*func)(args...);
}

FpgaXdma::FpgaXdma()
#ifdef USE_THREAD_MEMPOOL
    : xdma_mempool(sizeof(FpgaPackgeHead))
#endif // USE_THREAD_MEMPOOL
{
  for (int i = 0; i < CONFIG_DMA_CHANNELS; i++) {
    char c2h_device[64];
    sprintf(c2h_device, "%s%d", XDMA_C2H_DEVICE, i);
#ifdef FPGA_SIM
    xdma_sim_open(i, true);
#else
    xdma_c2h_fd[i] = open(c2h_device, O_RDONLY);
    if (xdma_c2h_fd[i] == -1) {
      std::cout << c2h_device << std::endl;
      perror("Failed to open XDMA device");
      exit(-1);
    }
    std::cout << "XDMA link " << c2h_device << std::endl;
#endif // FPGA_SIM
  }
#ifdef FPGA_SIM
  xdma_sim_axilite_open(true);
  xdma_sim_workload_open(true);
  xdma_sim_h2c_open(0, true);
#endif // FPGA_SIM
#if defined(CONFIG_USE_XDMA_H2C) && !defined(FPGA_SIM)
  xdma_h2c_fd = open(XDMA_H2C_DEVICE, O_WRONLY | O_TRUNC);
  if (xdma_h2c_fd == -1) {
    std::cout << XDMA_H2C_DEVICE << std::endl;
    perror("Failed to open XDMA device");
    exit(-1);
  }
  std::cout << "XDMA link " << XDMA_H2C_DEVICE << std::endl;
#endif
}

FpgaXdma::~FpgaXdma() {
#ifdef FPGA_SIM
  for (int i = 0; i < CONFIG_DMA_CHANNELS; i++) {
    xdma_sim_close(i);
  }
  xdma_sim_workload_close(true);
  xdma_sim_h2c_close(0);
  xdma_sim_axilite_close(true);
#endif // FPGA_SIM
}

void FpgaXdma::wait_fpga_io_done(uint64_t address, const char *tag) {
  const int max_retry = 600000; // 10 minute
  for (int retry = 0; retry < max_retry; retry++) {
    uint32_t status = fpga_io_read(address) & 0x3;
    if (status == 0x2) {
      return;
    }
    if (status == 0x3) {
      fprintf(stderr, "[fpga-host] %s failed: address range exceeds FPGA AXI address width\n", tag);
      exit(1);
    }
    usleep(1000);
  }
  fprintf(stderr, "[fpga-host] timeout waiting for %s\n", tag);
  exit(1);
}

#ifdef CONFIG_DIFFTEST_REPLAY_TRACE
bool FpgaXdma::queue_replay_trace(uint16_t trace_head, uint16_t trace_size) {
  if (replay_trace_pending || replay_trace_requested) {
    return true;
  }

  replay_trace_pending_head = trace_head;
  replay_trace_pending_size = trace_size;
  replay_trace_pending = true;
  return true;
}

bool FpgaXdma::service_replay_trace_request() {
  if (!replay_trace_pending || replay_trace_requested) {
    return true;
  }

  fpga_io(HOST_IO_DIFFTEST_ENABLE, false);
#ifdef FPGA_SIM
  // Let already queued Path-A C2H ranges drain/discard before switching the
  // shared channel to the replay dump stream.
  xdma_sim_drain(0, 100);
#endif
  fpga_io(HOST_IO_REPLAY_TRACE_HEAD, static_cast<uint32_t>(replay_trace_pending_head));
  fpga_io(HOST_IO_REPLAY_TRACE_SIZE, static_cast<uint32_t>(replay_trace_pending_size));
  fpga_io(HOST_IO_REPLAY_TRACE_FREEZE, true);

  const int max_retry = 1000;
  for (int retry = 0; retry < max_retry; retry++) {
    if (fpga_io_read(HOST_IO_REPLAY_TRACE_STATUS) & REPLAY_TRACE_STATUS_FROZEN) {
      break;
    }
    usleep(1000);
    if (retry == max_retry - 1) {
      fprintf(stderr, "[fpga-host] timeout freezing replay trace buffer\n");
      return false;
    }
  }

  replay_trace_requested = true;
  replay_trace_header_seen = false;
  replay_trace_discarded = 0;
  replay_trace_remaining = 0;
  replay_trace_packets = 0;
  const char *path = std::getenv("FPGA_REPLAY_TRACE_DUMP");
  if (path && path[0]) {
    replay_trace_file.open(path, std::ios::binary | std::ios::trunc);
    if (!replay_trace_file.is_open()) {
      fprintf(stderr, "[fpga-host] failed to open replay trace file %s\n", path);
      replay_trace_requested = false;
      return false;
    }
  }
  fpga_io(HOST_IO_REPLAY_TRACE_DUMP, true);
  printf("[fpga-host] replay trace requested: head=%u size=%u\n", replay_trace_pending_head,
         replay_trace_pending_size);
  replay_trace_pending = false;
  return true;
}

bool FpgaXdma::consume_replay_trace_packet(const uint8_t *payload) {
  if ((!replay_trace_pending && !replay_trace_requested) || !payload) {
    return false;
  }
  if (replay_trace_pending) {
    return true;
  }

  if (!replay_trace_header_seen) {
    ReplayTraceDumpHeader header;
    memcpy(&header, payload, sizeof(header));
    static bool replay_trace_debug_dumped = false;
    if (!replay_trace_debug_dumped) {
      replay_trace_debug_dumped = true;
      fprintf(stderr, "[fpga-host] replay trace first beat:");
      for (size_t i = 0; i < sizeof(header); i++) fprintf(stderr, " %02x", payload[i]);
      fprintf(stderr, "\n");
    }
    if (header.magic != UINT64_C(0x5254524143453031)) {
      replay_trace_discarded++;
      return true;
    }
    if (header.version != 1 || header.trace_head >= CONFIG_DIFFTEST_REPLAY_TRACE_DEPTH ||
        header.trace_size > CONFIG_DIFFTEST_REPLAY_TRACE_DEPTH ||
        header.dump_words > CONFIG_DIFFTEST_REPLAY_TRACE_DEPTH ||
        (header.dump_words != 0 && header.dump_start >= CONFIG_DIFFTEST_REPLAY_TRACE_DEPTH)) {
      fprintf(stderr, "[fpga-host] invalid replay trace header\n");
      replay_trace_requested = false;
      running = false;
      return true;
    }
    replay_trace_header_seen = true;
    if (replay_trace_discarded != 0) {
      printf("[fpga-host] discarded %u queued Batch beats before replay trace\n", replay_trace_discarded);
    }
    replay_trace_remaining = header.dump_words;
    replay_trace_packets = 1 + header.dump_words;
    replay_trace_packets += (8 - (replay_trace_packets & 7)) & 7;
    if (replay_trace_file.is_open()) {
      replay_trace_file.write(reinterpret_cast<const char *>(&header), sizeof(header));
    }
    if (header.flags & REPLAY_TRACE_HEADER_RANGE_LOST) {
      fprintf(stderr, "[fpga-host] replay trace range was overwritten before freeze\n");
      replay_trace_requested = false;
      running = false;
      return true;
    }
  } else if (replay_trace_remaining != 0) {
    if (replay_trace_file.is_open()) {
      replay_trace_file.write(reinterpret_cast<const char *>(payload), CONFIG_DIFFTEST_BATCH_BYTELEN);
    }
    replay_trace_remaining--;
    v_difftest_Batch(const_cast<uint8_t *>(payload));
  }

  if (replay_trace_packets != 0) {
    replay_trace_packets--;
  }
  if (replay_trace_packets == 0) {
    if (replay_trace_file.is_open()) {
      replay_trace_file.flush();
      replay_trace_file.close();
    }
    replay_trace_requested = false;
    if (running) {
      fprintf(stderr, "[fpga-host] replay range ended without reproducing the error\n");
      running = false;
    }
  }
  return true;
}
#endif // CONFIG_DIFFTEST_REPLAY_TRACE

#ifdef CONFIG_USE_XDMA_H2C
void FpgaXdma::h2c_load_workload(const void *payload, uint64_t size) {
  if (payload == nullptr) {
    fprintf(stderr, "[fpga-host] H2C load requires mmap-backed memory image\n");
    exit(-1);
  }
  if (size == 0) {
    fprintf(stderr, "[fpga-host] H2C workload size must be non-zero\n");
    exit(-1);
  }

#ifdef FPGA_SIM
  uint64_t offset = 0;
  while (offset < size) {
    size_t beatBytes = std::min<uint64_t>(H2C_AXIS_BYTES, size - offset);
    char beat[H2C_AXIS_BYTES] = {};
    memcpy(beat, reinterpret_cast<const uint8_t *>(payload) + offset, beatBytes);
    uint64_t tkeep = beatBytes == H2C_AXIS_BYTES ? UINT64_MAX : ((1ULL << beatBytes) - 1);
    if (xdma_sim_h2c_write(0, beat, tkeep, offset + beatBytes >= size, sizeof(beat)) != (int)sizeof(beat)) {
      fprintf(stderr, "[fpga-host] FPGA_SIM H2C shared-memory write failed\n");
      exit(-1);
    }
    offset += beatBytes;
  }
  printf("[fpga-host] FPGA_SIM H2C queued %" PRIu64 " bytes\n", size);
#else
  const char *buf = reinterpret_cast<const char *>(payload);
  uint64_t offset = 0;
  while (offset < size) {
    uint64_t remaining = size - offset;
    size_t request = std::min<uint64_t>(64ull * 1024ull * 1024ull, remaining); // 64MB per XDMA transfer
    ssize_t written = write(xdma_h2c_fd, buf + offset, request);
    if (written < 0) {
      if (errno == EINTR) {
        continue;
      }
      perror("[fpga-host] XDMA H2C write failed");
      exit(-1);
    }
    if (written == 0) {
      fprintf(stderr, "[fpga-host] XDMA H2C zero write at offset=%" PRIu64 "\n", offset);
      exit(-1);
    }
    offset += written;
  }
  printf("[fpga-host] XDMA H2C queued %" PRIu64 " bytes\n", size);
#endif // FPGA_SIM
}
#endif // CONFIG_USE_XDMA_H2C

// write xdma_bypass memory or xdma_user
void FpgaXdma::device_write(bool is_bypass, const char *workload, uint64_t addr, uint64_t value) {
  (void)workload;
#ifdef FPGA_SIM
  static const bool debug_axilite = std::getenv("FPGA_DEBUG_AXILITE") != nullptr;
  if (debug_axilite) {
    fprintf(stderr, "[fpga-host] AXIL write begin addr=0x%lx value=0x%lx\n", addr, value);
    fflush(stderr);
  }
  if (is_bypass) {
    fprintf(stderr, "[fpga-host] FPGA_SIM XDMA bypass write is unsupported\n");
    exit(-1);
  }
  if (xdma_sim_axilite_write(static_cast<uint32_t>(addr), static_cast<uint32_t>(value), 0xf) != 0) {
    fprintf(stderr, "[fpga-host] FPGA_SIM AXI-Lite command queue is full, addr=0x%lx value=0x%lx\n", addr, value);
    exit(-1);
  }
  if (debug_axilite) {
    fprintf(stderr, "[fpga-host] AXIL write done addr=0x%lx\n", addr);
    fflush(stderr);
  }
  return;
#endif // FPGA_SIM

  uint64_t pg_size = sysconf(_SC_PAGE_SIZE);
  uint64_t size = !is_bypass ? 0x1000 : 0x100000;
  uint64_t aligned_size = (size + 0xffful) & ~0xffful;
  uint64_t base = addr & ~0xffful;
  uint32_t offset = addr & 0xfffu;
  int fd = -1;

  if (base % pg_size != 0) {
    printf("base must be a multiple of system page size\n");
    exit(-1);
  }

  if (is_bypass)
    fd = open(XDMA_BYPASS, O_RDWR | O_SYNC);
  else
    fd = open(XDMA_USER, O_RDWR | O_SYNC);
  if (fd < 0) {
    printf("Failed to open %s\n", is_bypass ? XDMA_BYPASS : XDMA_USER);
    exit(-1);
  }

  void *m_ptr = mmap(nullptr, aligned_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, base);
  if (m_ptr == MAP_FAILED) {
    close(fd);
    printf("failed to mmap\n");
    exit(-1);
  }

  if (is_bypass) {
    if (simMemory->get_img_size() > aligned_size) {
      printf("The loaded workload size exceeds the xdma bypass size");
      exit(-1);
    }
    memcpy(static_cast<char *>(m_ptr) + offset, static_cast<const void *>(simMemory->as_ptr()),
           simMemory->get_img_size());
  } else {
    ((volatile uint32_t *)m_ptr)[offset >> 2] = value;
  }

  munmap(m_ptr, aligned_size);
  close(fd);
}

uint32_t FpgaXdma::device_read(bool is_bypass, uint64_t addr) {
#ifdef FPGA_SIM
  static const bool debug_axilite = std::getenv("FPGA_DEBUG_AXILITE") != nullptr;
  if (debug_axilite) {
    fprintf(stderr, "[fpga-host] AXIL read begin addr=0x%lx\n", addr);
    fflush(stderr);
  }
  if (is_bypass) {
    fprintf(stderr, "[fpga-host] FPGA_SIM XDMA bypass read is unsupported\n");
    exit(-1);
  }
  uint32_t data = 0;
  if (xdma_sim_axilite_read(static_cast<uint32_t>(addr), &data) != 0) {
    fprintf(stderr, "[fpga-host] FPGA_SIM AXI-Lite read failed, addr=0x%lx\n", addr);
    exit(-1);
  }
  if (debug_axilite) {
    fprintf(stderr, "[fpga-host] AXIL read done addr=0x%lx data=0x%x\n", addr, data);
    fflush(stderr);
  }
  return data;
#endif // FPGA_SIM

  uint64_t pg_size = sysconf(_SC_PAGE_SIZE);
  uint64_t size = !is_bypass ? 0x1000 : 0x100000;
  uint64_t aligned_size = (size + 0xffful) & ~0xffful;
  uint64_t base = addr & ~0xffful;
  uint32_t offset = addr & 0xfffu;

  if (base % pg_size != 0) {
    printf("base must be a multiple of system page size\n");
    exit(-1);
  }

  int fd = open(is_bypass ? XDMA_BYPASS : XDMA_USER, O_RDWR | O_SYNC);
  if (fd < 0) {
    printf("Failed to open %s\n", is_bypass ? XDMA_BYPASS : XDMA_USER);
    exit(-1);
  }

  void *m_ptr = mmap(nullptr, aligned_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, base);
  if (m_ptr == MAP_FAILED) {
    close(fd);
    printf("failed to mmap\n");
    exit(-1);
  }

  uint32_t value = ((volatile uint32_t *)m_ptr)[offset >> 2];
  munmap(m_ptr, aligned_size);
  close(fd);
  return value;
}

#ifdef USE_THREAD_MEMPOOL
void FpgaXdma::start_transmit_thread() {
  for (int i = 0; i < CONFIG_DMA_CHANNELS; i++) {
    printf("start channel %d \n", i);
    receive_thread[i] = std::thread(thread_wrapper<decltype(&FpgaXdma::read_xdma_thread), FpgaXdma *, int>,
                                    &FpgaXdma::read_xdma_thread, this, i);
  }
  process_thread = std::thread(thread_wrapper<decltype(&FpgaXdma::write_difftest_thread), FpgaXdma *>,
                               &FpgaXdma::write_difftest_thread, this);
}

void FpgaXdma::stop_thansmit_thread() {
#ifdef FPGA_SIM
  for (int i = 0; i < CONFIG_DMA_CHANNELS; i++) {
    xdma_sim_cancel(i);
  }
#endif // FPGA_SIM
  for (int i = 0; i < CONFIG_DMA_CHANNELS; i++) {
    if (receive_thread[i].joinable())
      receive_thread[i].join();
#ifdef FPGA_SIM
    xdma_sim_close(i);
#else
    close(xdma_c2h_fd[i]);
#endif // FPGA_SIM
  }

  if (process_thread.joinable())
    process_thread.join();
#if defined(CONFIG_USE_XDMA_H2C) && !defined(FPGA_SIM)
  close(xdma_h2c_fd);
#endif
}

void FpgaXdma::read_xdma_thread(int channel) {
  size_t mem_get_idx = 0;
  while (running && signal_num == 0) {
    char *mem = xdma_mempool.get_free_chunk(&mem_get_idx);
#ifdef FPGA_SIM
    ssize_t size = static_cast<ssize_t>(xdma_sim_read(channel, mem, sizeof(FpgaPackgeHead)));
#else
    struct pollfd poll_fd = {xdma_c2h_fd[channel], POLLIN, 0};
    int poll_result;
    do {
      poll_result = poll(&poll_fd, 1, 100);
    } while (poll_result < 0 && errno == EINTR);
    if (poll_result == 0) {
      continue;
    }
    if (poll_result < 0 || (poll_fd.revents & (POLLERR | POLLHUP | POLLNVAL))) {
      break;
    }
    if (!running || signal_num != 0) {
      break;
    }
    ssize_t size = read(xdma_c2h_fd[channel], mem, sizeof(FpgaPackgeHead));
#endif // FPGA_SIM
    if (size <= 0) {
      if (signal_num != 0 || (size < 0 && errno == EINTR)) {
        break;
      }
      continue;
    }
    if (xdma_mempool.write_free_chunk(mem[0], mem_get_idx) == false) {
      printf("It should not be the case that no available block can be found\n");
      assert(0);
    }
  }
}

void FpgaXdma::write_difftest_thread() {
  FpgaPackgeHead *packge;
  uint8_t recv_count = 0;
  xdma_mempool.wait_mempool_start();
  while (running && signal_num == 0) {
    packge = reinterpret_cast<FpgaPackgeHead *>(xdma_mempool.read_busy_chunk());
    if (packge == nullptr) {
      printf("Failed to read data from the XDMA memory pool\n");
      assert(0);
    }
    if (packge->diff_packge[0].packge_idx != recv_count) {
      printf("read mempool idx failed, packge_idx %d need_idx %d\n", packge->diff_packge[0].packge_idx, recv_count);
      assert(0);
    }
    recv_count++;
    // packge unpack
    for (size_t i = 0; i < DMA_PACKGE_NUM; i++) {
#ifdef CONFIG_DIFFTEST_REPLAY_TRACE
      if (!consume_replay_trace_packet(packge->diff_packge[i].diff_packge)) {
        v_difftest_Batch(packge->diff_packge[i].diff_packge);
      }
#else
      v_difftest_Batch(packge->diff_packge[i].diff_packge);
#endif // CONFIG_DIFFTEST_REPLAY_TRACE
    }
    xdma_mempool.set_free_chunk();
#ifdef CONFIG_DIFFTEST_REPLAY_TRACE
    if (!service_replay_trace_request()) {
      running = false;
    }
#endif // CONFIG_DIFFTEST_REPLAY_TRACE
  }
}

#else // !USE_THREAD_MEMPOOL

void *posix_memalignd_malloc(size_t size) {
  void *ptr = nullptr;
  int ret = posix_memalign(&ptr, 4096, size);
  if (ret != 0) {
    perror("posix_memalign failed");
    return nullptr;
  }
  return ptr;
}
void FpgaXdma::read_and_process() {
  printf("start channel 0\n");
  FpgaPackgeHead *packge = (FpgaPackgeHead *)posix_memalignd_malloc(sizeof(FpgaPackgeHead));
  memset(packge, 0, sizeof(FpgaPackgeHead));
  while (running && signal_num == 0) {
#ifdef FPGA_SIM
    ssize_t size = static_cast<ssize_t>(xdma_sim_read(0, (char *)packge, sizeof(FpgaPackgeHead)));
#else
    ssize_t size = read(xdma_c2h_fd[0], packge, sizeof(FpgaPackgeHead));
#endif // FPGA_SIM
    if (size <= 0) {
      if (signal_num != 0 || (size < 0 && errno == EINTR)) {
        break;
      }
      continue;
    }
    for (size_t i = 0; i < DMA_PACKGE_NUM; i++) {
#ifdef CONFIG_DIFFTEST_REPLAY_TRACE
      if (!consume_replay_trace_packet(packge->diff_packge[i].diff_packge)) {
        v_difftest_Batch(packge->diff_packge[i].diff_packge);
      }
#else
      v_difftest_Batch(packge->diff_packge[i].diff_packge);
#endif // CONFIG_DIFFTEST_REPLAY_TRACE
    }
#ifdef CONFIG_DIFFTEST_REPLAY_TRACE
    if (!service_replay_trace_request()) {
      running = false;
    }
#endif // CONFIG_DIFFTEST_REPLAY_TRACE
  }
  free(packge);
}
#endif // USE_THREAD_MEMPOOL
