/***************************************************************************************
* Copyright (c) 2025-2026 Beijing Institute of Open Source Chip (BOSC)
* Copyright (c) 2020-2026 Institute of Computing Technology, Chinese Academy of Sciences
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
#include "ref_fork.h"
#include <algorithm>
#include <chrono>
#include <cstring>
#include <errno.h>
#include <execinfo.h>
#include <fcntl.h>
#include <fstream>
#include <inttypes.h>
#include <iostream>
#include <pthread.h>
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
SharedPacketPool *g_shared_packet_pool = nullptr;

static const size_t H2C_AXIS_BYTES = CONFIG_DIFFTEST_HOST_AXIS_BYTES;

void signal_handler(int sig) {
  if (difftest_ref_fork_is_child())
    _Exit(128 + sig);
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
{
#ifdef USE_THREAD_MEMPOOL
  const char *shared = getenv("DIFFTEST_SHARED_PACKET_POOL");
  if ((shared && strcmp(shared, "1") == 0) || difftest_ref_fork_enabled()) {
    if (CONFIG_DMA_CHANNELS != 1)
      throw std::runtime_error("Shared packet pool requires one DMA channel");
    size_t packets = NUM_BLOCKS;
    const char *value = getenv("DIFFTEST_PACKET_POOL_SLOTS");
    if (value) {
      char *end = nullptr;
      errno = 0;
      packets = strtoull(value, &end, 0);
      if (errno || end == value || *end)
        throw std::runtime_error("Invalid shared packet pool packet count");
    }
    shared_packet_pool = std::make_unique<SharedPacketPool>(packets, sizeof(FpgaPackgeHead));
    g_shared_packet_pool = shared_packet_pool.get();
    printf("SharedPacketPool Slots=%zu PacketBytes=%zu Mapping=MAP_SHARED\n", packets, sizeof(FpgaPackgeHead));
  } else
    indexed_packet_pool = std::make_unique<MemoryIdxPool>(sizeof(FpgaPackgeHead));
#endif

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
#else
void FpgaXdma::h2c_load_workload(const void *payload, uint64_t size) {
  (void)payload;
  (void)size;
  fprintf(stderr, "[fpga-host] XDMA H2C workload support is disabled (CONFIG_USE_XDMA_H2C=0)\n");
}
#endif // CONFIG_USE_XDMA_H2C

// write xdma_bypass memory or xdma_user
void FpgaXdma::device_write(bool is_bypass, const char *workload, uint64_t addr, uint64_t value) {
  (void)workload;
#ifdef FPGA_SIM
  if (is_bypass) {
    fprintf(stderr, "[fpga-host] FPGA_SIM XDMA bypass write is unsupported\n");
    exit(-1);
  }
  if (xdma_sim_axilite_write(static_cast<uint32_t>(addr), static_cast<uint32_t>(value), 0xf) != 0) {
    fprintf(stderr, "[fpga-host] FPGA_SIM AXI-Lite command queue is full, addr=0x%lx value=0x%lx\n", addr, value);
    exit(-1);
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
  if (is_bypass) {
    fprintf(stderr, "[fpga-host] FPGA_SIM XDMA bypass read is unsupported\n");
    exit(-1);
  }
  uint32_t data = 0;
  if (xdma_sim_axilite_read(static_cast<uint32_t>(addr), &data) != 0) {
    fprintf(stderr, "[fpga-host] FPGA_SIM AXI-Lite read failed, addr=0x%lx\n", addr);
    exit(-1);
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
extern void fpga_ref_fork_abort();

static void xdma_wakeup_handler(int) {}
void FpgaXdma::start_transmit_thread() {
  struct sigaction sa {};
  sa.sa_handler = xdma_wakeup_handler;
  sigemptyset(&sa.sa_mask);
  sigaction(SIGUSR1, &sa, nullptr); // Wake blocking read without SA_RESTART.

  for (int i = 0; i < CONFIG_DMA_CHANNELS; i++) {
    printf("start channel %d \n", i);
    receive_finished[i].store(false);
    receive_thread[i] = std::thread(thread_wrapper<decltype(&FpgaXdma::read_xdma_thread), FpgaXdma *, int>,
                                    &FpgaXdma::read_xdma_thread, this, i);
  }
  process_thread = std::thread(thread_wrapper<decltype(&FpgaXdma::write_difftest_thread), FpgaXdma *>,
                               &FpgaXdma::write_difftest_thread, this);
}

void FpgaXdma::stop_thansmit_thread() {
  stop();
  for (int i = 0; i < CONFIG_DMA_CHANNELS; ++i) {
    while (receive_thread[i].joinable() && !receive_finished[i].load(std::memory_order_acquire)) {
      pthread_kill(receive_thread[i].native_handle(), SIGUSR1);
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
  }

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
    char *mem = shared_packet_pool ? shared_packet_pool->get_free() : indexed_packet_pool->get_free_chunk(&mem_get_idx);
    if (!mem) {
      if (shared_packet_pool && shared_packet_pool->aborted())
        break;
      std::this_thread::yield();
      continue;
    }
    // Ordinary read fills a pool slot directly. Short reads accumulate before
    // publication; a partial packet is never visible to any parser.
    size_t received = 0;
    while (running && signal_num == 0 && received < sizeof(FpgaPackgeHead)) {
#ifdef FPGA_SIM
      ssize_t size = static_cast<ssize_t>(xdma_sim_read(channel, mem + received, sizeof(FpgaPackgeHead) - received));
#else
      ssize_t size = read(xdma_c2h_fd[channel], mem + received, sizeof(FpgaPackgeHead) - received);
#endif
      if (size < 0 && errno == EINTR)
        continue;
      if (size <= 0) {
        fprintf(stderr, "XDMA receive failed or ended inside a packet\n");
        if (shared_packet_pool)
          shared_packet_pool->fail();
        running = false;
        break;
      }
      received += size;
    }
    if (received != sizeof(FpgaPackgeHead))
      break;
    if (shared_packet_pool)
      shared_packet_pool->publish();
    else if (!indexed_packet_pool->write_free_chunk(mem[0], mem_get_idx)) {
      fprintf(stderr, "XDMA pool publication failed\n");
      running = false;
    }
  }
  receive_finished[channel].store(true, std::memory_order_release);
}

void FpgaXdma::write_difftest_thread() {
  auto abort = [] {
    difftest_ref_fork_abort_child();
    fpga_ref_fork_abort();
  };
  uint8_t recv_count = 0;
  if (indexed_packet_pool)
    indexed_packet_pool->wait_mempool_start();
  while (running && signal_num == 0) {
    if (shared_packet_pool && shared_packet_pool->aborted()) {
      abort();
      return;
    }
    auto *packet =
        reinterpret_cast<FpgaPackgeHead *>(shared_packet_pool ? shared_packet_pool->get_busy() : indexed_packet_pool->read_busy_chunk());
    if (!packet) {
      if (shared_packet_pool && difftest_ref_fork_idle()) {
        abort();
        return;
      }
      std::this_thread::yield();
      continue;
    }
    if (packet->diff_packge[0].packge_idx != recv_count++) {
      fprintf(stderr, "XDMA packet sequence mismatch\n");
      if (shared_packet_pool)
        shared_packet_pool->fail();
      abort();
      return;
    }
    for (size_t i = 0; i < DMA_PACKGE_NUM; ++i)
      v_difftest_Batch(packet->diff_packge[i].diff_packge);
    if (shared_packet_pool) {
      shared_packet_pool->release();
      if (difftest_ref_fork_idle()) {
        abort();
        return;
      }
    } else
      indexed_packet_pool->set_free_chunk();
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
      v_difftest_Batch(packge->diff_packge[i].diff_packge);
    }
  }
  free(packge);
}
#endif // USE_THREAD_MEMPOOL
