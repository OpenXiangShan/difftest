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
#include "fork_ref/fork_ref.h"
#include "mpool.h"
#include "ram.h"
#include "reftrace/refdrive_trace.h"
#include "xdma_c2h_pool.h"
#include "xdma_receive.h"
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
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>

#define XDMA_USER       "/dev/xdma0_user"
#define XDMA_BYPASS     "/dev/xdma0_bypass"
#define XDMA_C2H_DEVICE "/dev/xdma0_c2h_"
#define XDMA_H2C_DEVICE "/dev/xdma0_h2c_0"
MemoryIdxPool *g_packet_pool = nullptr;

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

namespace {
bool receive_env_flag(const char *name) {
  const char *value = std::getenv(name);
  if (value && std::strcmp(value, "0") && std::strcmp(value, "1"))
    throw std::invalid_argument(std::string(name) + " must be 0 or 1");
  return value && std::strcmp(value, "1") == 0;
}

size_t receive_env_number(const char *name, size_t fallback) {
  const char *value = std::getenv(name);
  if (!value || !*value)
    return fallback;
  for (const char *p = value; *p; ++p)
    if (*p < '0' || *p > '9')
      throw std::invalid_argument(std::string(name) + " must be a positive integer");
  errno = 0;
  const unsigned long long number = std::strtoull(value, nullptr, 10);
  if (errno || number == 0 || number > 4096)
    throw std::invalid_argument(std::string(name) + " must be in [1, 4096]");
  return static_cast<size_t>(number);
}
} // namespace

extern void fpga_ref_fork_abort();

FpgaXdma::FpgaXdma(bool fork_readers, size_t pool_slots) {
  std::fill_n(xdma_c2h_fd, CONFIG_DMA_CHANNELS, -1);
  const char *pool_env = std::getenv("FPGA_XDMA_POOL");
  if (pool_env && std::strcmp(pool_env, "0") && std::strcmp(pool_env, "1"))
    throw std::invalid_argument("FPGA_XDMA_POOL must be 0 or 1");
  persistent_pool = pool_env ? std::strcmp(pool_env, "1") == 0 : std::getenv("FPGA_XDMA_POOL_SLOTS") != nullptr;
  this->pool_slots = static_cast<uint32_t>(receive_env_number("FPGA_XDMA_POOL_SLOTS", 128));
  read_packets = receive_env_number("FPGA_XDMA_READ_PACKETS", persistent_pool ? 32 : 1);
  adjacent_descriptors = receive_env_flag("FPGA_XDMA_ADJACENT");
  if (adjacent_descriptors && !persistent_pool)
    throw std::invalid_argument("FPGA_XDMA_ADJACENT=1 requires FPGA_XDMA_POOL=1");
  if (persistent_pool && this->pool_slots > 1024)
    throw std::invalid_argument("FPGA_XDMA_POOL_SLOTS must be at most 1024 for this driver");
  if (!persistent_pool && read_packets != 1)
    throw std::invalid_argument("FPGA_XDMA_READ_PACKETS greater than 1 requires FPGA_XDMA_POOL=1");
#ifdef FPGA_SIM
  if (persistent_pool || read_packets != 1)
    throw std::invalid_argument("Persistent/batched XDMA receive is unavailable in FPGA_SIM");
#endif
  if (persistent_pool && CONFIG_DMA_CHANNELS != 1)
    throw std::invalid_argument("Persistent XDMA receive currently requires DMA_CHANNELS=1");
#ifdef USE_THREAD_MEMPOOL
  this->fork_readers = fork_readers;
  packet_pool = std::make_unique<MemoryIdxPool>(sizeof(FpgaPackgeHead), fork_readers ? pool_slots : NUM_BLOCKS);
  if (fork_readers)
    g_packet_pool = packet_pool.get();
#endif

#ifndef FPGA_SIM
  if (XdmaRelayClient::enabled()) {
    if (persistent_pool || read_packets != 1)
      throw std::invalid_argument("Persistent/batched XDMA receive is unavailable over XDMA relay");
    relay = std::make_unique<XdmaRelayClient>(sizeof(FpgaPackgeHead), CONFIG_DMA_CHANNELS);
    return;
  }
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
  // start() joins receivers before returning. Never close a live reader's fd.
  finish_receive();
#ifdef USE_THREAD_MEMPOOL
  if (g_packet_pool == packet_pool.get())
    g_packet_pool = nullptr;
#endif
#ifdef FPGA_SIM
  for (int i = 0; i < CONFIG_DMA_CHANNELS; i++)
    xdma_sim_close(i);
  xdma_sim_workload_close(true);
  xdma_sim_h2c_close(0);
  xdma_sim_axilite_close(true);
#else
  for (int &fd: xdma_c2h_fd) {
    if (fd >= 0)
      close(fd);
    fd = -1;
  }
#ifdef CONFIG_USE_XDMA_H2C
  if (xdma_h2c_fd >= 0)
    close(xdma_h2c_fd);
  xdma_h2c_fd = -1;
#endif
#endif
}

void FpgaXdma::configure_receive() {
#ifndef FPGA_SIM
  if (!persistent_pool)
    return;
  for (int channel = 0; channel < CONFIG_DMA_CHANNELS; ++channel) {
    const xdma_c2h_pool_config config = {XDMA_C2H_POOL_ABI, static_cast<uint32_t>(sizeof(FpgaPackgeHead)), pool_slots,
                                         adjacent_descriptors ? XDMA_C2H_POOL_ADJACENT : 0U};
    if (ioctl(xdma_c2h_fd[channel], XDMA_IOCTL_C2H_POOL_SETUP, &config) < 0)
      throw std::system_error(errno, std::generic_category(),
                              "FPGA_XDMA_POOL requires a compatible XDMA C2H pool driver (SETUP)");
    pool_configured[channel] = true;
    printf("[fpga-host] XDMA C2H pool channel=%d packet_bytes=%zu slots=%u read_packets=%zu adjacent=%d\n", channel,
           sizeof(FpgaPackgeHead), pool_slots, read_packets, adjacent_descriptors ? 1 : 0);
  }
#endif
}

bool FpgaXdma::finish_receive() {
  bool passed = true;
#ifndef FPGA_SIM
  for (int channel = 0; channel < CONFIG_DMA_CHANNELS; ++channel) {
    if (!pool_configured[channel])
      continue;
    xdma_c2h_pool_stats stats = {};
    if (ioctl(xdma_c2h_fd[channel], XDMA_IOCTL_C2H_POOL_STATS, &stats) < 0) {
      perror("XDMA C2H pool STATS");
      passed = false;
    } else {
      printf(
          "[XDMA_C2H_POOL] channel=%d launches=%llu reads=%llu packets=%llu bytes=%llu errors=%llu "
          "partial_reads=%llu packet_bytes=%u slots=%u\n",
          channel, static_cast<unsigned long long>(stats.launches), static_cast<unsigned long long>(stats.reads),
          static_cast<unsigned long long>(stats.packets), static_cast<unsigned long long>(stats.bytes),
          static_cast<unsigned long long>(stats.errors), static_cast<unsigned long long>(stats.partial_reads),
          stats.packet_bytes, stats.slots);
      if (stats.errors || stats.packet_bytes != sizeof(FpgaPackgeHead) || stats.slots != pool_slots)
        passed = false;
    }
    // STOP is serialized with reads in the driver; all readers have joined here.
    if (ioctl(xdma_c2h_fd[channel], XDMA_IOCTL_C2H_POOL_STOP) < 0) {
      perror("XDMA C2H pool STOP");
      passed = false;
    }
    pool_configured[channel] = false;
  }
#endif
  return passed;
}

void FpgaXdma::receive_failure(const char *message) {
  fprintf(stderr, "[fpga-host] %s\n", message);
  running = false;
#ifdef USE_THREAD_MEMPOOL
  if (packet_pool)
    packet_pool->fail();
#endif
}

ssize_t FpgaXdma::read_stream(int channel, void *buffer, size_t bytes) {
#ifdef FPGA_SIM
  return static_cast<ssize_t>(xdma_sim_read(channel, static_cast<char *>(buffer), bytes));
#else
  return relay ? relay->read(channel, buffer, bytes) : read(xdma_c2h_fd[channel], buffer, bytes);
#endif
}

void FpgaXdma::dispatch_packet(const FpgaPackgeHead &packet) {
  xdma_receive::dispatch(
      DMA_PACKGE_NUM, [this] { return keep_receiving(); },
      [&packet](size_t i) {
        if (!dma_packet_is_padding(packet.diff_packge[i]))
          v_difftest_Batch(const_cast<uint8_t *>(packet.diff_packge[i].diff_packge));
      });
}

void FpgaXdma::start(bool enable_diff) {
  running = true;
  if (!enable_diff) {
    while (signal_num == 0)
      usleep(10000);
    running = false;
    return;
  }
  bool failed = false;
  try {
    // Configure only after fpga_init has reset the FPGA packet stream, and before
    // any receiver starts. DMA resources belong to this open file until cleanup.
    configure_receive();
#ifdef USE_THREAD_MEMPOOL
    start_transmit_thread();
    while (keep_receiving()) {
      if (packet_idle && packet_idle())
        break;
      usleep(1000);
    }
    running = false;
    stop_thansmit_thread();
    failed = packet_pool->aborted();
#else
    read_and_process();
#endif
  } catch (const std::exception &error) {
    receive_failure(error.what());
    failed = true;
#ifdef USE_THREAD_MEMPOOL
    stop_thansmit_thread();
#endif
  }
  if (!finish_receive())
    failed = true;
  if (failed)
    fpga_ref_fork_abort();
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
    ssize_t written = relay ? relay->write(buf + offset, request) : write(xdma_h2c_fd, buf + offset, request);
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
  if (relay) {
    if (is_bypass) {
      fprintf(stderr, "XDMA relay does not support bypass writes\n");
      exit(1);
    }
    relay->write32(addr, value);
    return;
  }
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
  if (relay) {
    if (is_bypass) {
      fprintf(stderr, "XDMA relay does not support bypass reads\n");
      exit(1);
    }
    return relay->read32(addr);
  }
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
  if (!fork_readers || difftest_refdrive_trace_pending())
    process_thread =
        std::thread(thread_wrapper<decltype(&FpgaXdma::process_packets), FpgaXdma *>, &FpgaXdma::process_packets, this);
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
  }
  if (process_thread.joinable())
    process_thread.join();
}

void FpgaXdma::read_xdma_thread(int channel) {
  try {
    xdma_receive::RecordReader reader(sizeof(FpgaPackgeHead), read_packets, persistent_pool);
#ifdef CONFIG_DIFFTEST_FORK_REF
    std::chrono::steady_clock::time_point timeout_start{};
#endif
    auto read_batch = [this, channel
#ifdef CONFIG_DIFFTEST_FORK_REF
                       ,
                       &timeout_start
#endif
    ](void *buffer, size_t bytes) -> ssize_t {
      while (true) {
        const ssize_t size = read_stream(channel, buffer, bytes);
#ifdef CONFIG_DIFFTEST_FORK_REF
        if (size < 0 && errno == ETIMEDOUT && fork_readers && difftest_ref_fork_draining() && keep_receiving()) {
          const auto now = std::chrono::steady_clock::now();
          if (timeout_start == std::chrono::steady_clock::time_point{})
            timeout_start = now;
          if (now - timeout_start < std::chrono::minutes(5)) {
            fprintf(stderr, "XDMA C2H idle timeout; retrying while fork checkers drain\n");
            continue;
          }
        }
#endif
        return size;
      }
    };
    while (keep_receiving()) {
      const uint8_t *packet = reader.next(read_batch, [this] { return keep_receiving(); });
      if (!packet)
        break;
      char *memory = nullptr;
      size_t slot = 0;
      // Retain the rest of this read batch while pool consumers catch up.
      while (keep_receiving() && !(memory = packet_pool->get_free_chunk(&slot))) {
        if (packet_pool->aborted())
          break;
        std::this_thread::yield();
      }
      if (!memory)
        break;
      memcpy(memory, packet, sizeof(FpgaPackgeHead));
      if (!packet_pool->write_free_chunk(packet[0], slot)) {
        receive_failure("XDMA pool publication failed");
        break;
      }
    }
  } catch (const std::exception &error) {
    receive_failure(error.what());
  }
  receive_finished[channel].store(true, std::memory_order_release);
}

void FpgaXdma::process_packets() {
  const bool poll_callbacks = !fork_readers || !difftest_refdrive_trace_pending();
  // The fork leader enters this parser without start(); normal receiver threads
  // already have running set and must not undo an earlier receive failure.
  if (fork_readers && !receive_thread[0].joinable() && !packet_pool->aborted() && signal_num == 0)
    running = true;
  auto abort = [this, poll_callbacks] {
    if (!poll_callbacks)
      running = false;
    else if (packet_abort)
      packet_abort();
  };
  xdma_receive::PacketSequence sequence;
  while (running && signal_num == 0) {
    if (packet_pool->aborted()) {
      abort();
      return;
    }
    auto *packet = reinterpret_cast<FpgaPackgeHead *>(packet_pool->read_busy_chunk());
    if (!packet) {
      if (fork_readers && poll_callbacks && packet_idle && packet_idle()) {
        abort();
        return;
      }
      std::this_thread::yield();
      continue;
    }
    try {
      sequence.accept(packet->diff_packge[0].packge_idx);
      dispatch_packet(*packet);
    } catch (const std::exception &error) {
      receive_failure(error.what());
      abort();
      return;
    }
    packet_pool->set_free_chunk();
    if (fork_readers && poll_callbacks && packet_idle && packet_idle()) {
      abort();
      return;
    }
  }
}

#else // !USE_THREAD_MEMPOOL

void FpgaXdma::read_and_process() {
  printf("start channel 0\n");
  xdma_receive::RecordReader reader(sizeof(FpgaPackgeHead), read_packets, persistent_pool);
  xdma_receive::PacketSequence sequence;
  auto read_batch = [this](void *buffer, size_t bytes) { return read_stream(0, buffer, bytes); };
  while (keep_receiving()) {
    const uint8_t *bytes = reader.next(read_batch, [this] { return keep_receiving(); });
    if (!bytes)
      break;
    const auto *packet = reinterpret_cast<const FpgaPackgeHead *>(bytes);
    sequence.accept(packet->diff_packge[0].packge_idx);
    dispatch_packet(*packet);
  }
}

#endif // USE_THREAD_MEMPOOL
