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
#include <chrono>
#include <cstring>
#include <errno.h>
#include <execinfo.h>
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <limits>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <sys/mman.h>
#include <unistd.h>
#include <vector>

#define XDMA_USER       "/dev/xdma0_user"
#define XDMA_C2H_DEVICE "/dev/xdma0_c2h_"
#define XDMA_H2C_DEVICE "/dev/xdma0_h2c_0"

static constexpr uint32_t H2C_DDR_ARB_SEL_MASK = 0x1;
static constexpr uint32_t H2C_STATUS_DONE_MASK = 0x4;
static constexpr uint32_t H2C_STATUS_DONE_FALLBACK_MASK = 0x1;
static constexpr uint32_t DEFAULT_H2C_BEAT_BYTES = 8;
static constexpr int XDMA_ERR_ERESTARTSYS = 512;
static constexpr int DEFAULT_H2C_CFG_TIMEOUT_MS = 5000;
static constexpr int DEFAULT_H2C_DONE_TIMEOUT_MS = 30000;
static constexpr int DEFAULT_H2C_ACTIVE_TIMEOUT_MS = 200;
static constexpr int H2C_POLL_INTERVAL_US = 100;
static constexpr uint64_t XDMA_USER_MAP_SIZE = 0x1000;

static int get_timeout_ms_from_env(const char *name, int default_ms) {
  const char *value = getenv(name);
  if (value == nullptr || value[0] == '\0') {
    return default_ms;
  }
  char *end = nullptr;
  long parsed = strtol(value, &end, 10);
  if (end == value || *end != '\0' || parsed <= 0 || parsed > std::numeric_limits<int>::max()) {
    fprintf(stderr, "Invalid %s=%s, fallback to %d ms\n", name, value, default_ms);
    return default_ms;
  }
  return static_cast<int>(parsed);
}

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

void handle_sigint(int sig) {
  printf("handle sigint unlink pcie success, exit fpga-host!\n");
  exit(1);
}

FpgaXdma::FpgaXdma()
#ifdef USE_THREAD_MEMPOOL
    : xdma_mempool(sizeof(FpgaPackgeHead))
#endif // USE_THREAD_MEMPOOL
{
  xdma_user_fd = -1;
  xdma_h2c_fd = -1;

  signal(SIGINT, handle_sigint);
  for (int i = 0; i < CONFIG_DMA_CHANNELS; i++) {
    xdma_c2h_fd[i] = -1;
    char c2h_device[64];
    sprintf(c2h_device, "%s%d", XDMA_C2H_DEVICE, i);
#ifdef FPGA_SIM
    xdma_c2h_sim_open(i, true);
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
}

FpgaXdma::~FpgaXdma() {
  close_device_fds();
}

void FpgaXdma::close_device_fds() {
#ifndef FPGA_SIM
  for (int i = 0; i < CONFIG_DMA_CHANNELS; i++) {
    if (xdma_c2h_fd[i] >= 0) {
      close(xdma_c2h_fd[i]);
      xdma_c2h_fd[i] = -1;
    }
  }
  if (xdma_h2c_fd >= 0) {
    close(xdma_h2c_fd);
    xdma_h2c_fd = -1;
  }
  if (xdma_user_fd >= 0) {
    close(xdma_user_fd);
    xdma_user_fd = -1;
  }
#endif // FPGA_SIM
}

void FpgaXdma::fpga_io(uint64_t addr, bool enable) {
#ifndef FPGA_SIM
  uint32_t value = enable ? 0x1U : 0x0U;
  if (!config_bar_write32(static_cast<uint32_t>(addr), value)) {
    fprintf(stderr, "fpga_io write failed: addr=0x%lx value=0x%x\n", addr, value);
  }
#else
  device_write(addr, enable ? 0x1 : 0x0);
#endif // FPGA_SIM
}

void FpgaXdma::device_write(uint64_t addr, uint64_t value) {
  uint64_t pg_size = sysconf(_SC_PAGE_SIZE);
  uint64_t aligned_size = (XDMA_USER_MAP_SIZE + 0xffful) & ~0xffful;
  uint64_t base = addr & ~0xffful;
  uint32_t offset = addr & 0xfffu;

  if (base % pg_size != 0) {
    printf("base must be a multiple of system page size\n");
    exit(-1);
  }

  int fd = open(XDMA_USER, O_RDWR | O_SYNC);
  if (fd < 0) {
    printf("Failed to open %s\n", XDMA_USER);
    exit(-1);
  }

  void *m_ptr = mmap(nullptr, aligned_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, base);
  if (m_ptr == MAP_FAILED) {
    close(fd);
    printf("failed to mmap\n");
    exit(-1);
  }

  ((volatile uint32_t *)m_ptr)[offset >> 2] = value;

  munmap(m_ptr, aligned_size);
  close(fd);
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
  for (int i = 0; i < CONFIG_DMA_CHANNELS; i++) {
    if (receive_thread[i].joinable())
      receive_thread[i].join();
#ifdef FPGA_SIM
    xdma_c2h_sim_close(i);
#endif // FPGA_SIM
  }

  if (process_thread.joinable())
    process_thread.join();
  close_device_fds();
}

void FpgaXdma::read_xdma_thread(int channel) {
  size_t mem_get_idx = 0;
  while (running) {
    char *mem = xdma_mempool.get_free_chunk(&mem_get_idx);
    if (mem == nullptr) {
      std::this_thread::yield();
      continue;
    }
#ifdef FPGA_SIM
    size_t size = xdma_c2h_sim_read(channel, mem, sizeof(FpgaPackgeHead));
#else
    size_t size = read(xdma_c2h_fd[channel], mem, sizeof(FpgaPackgeHead));
#endif // FPGA_SIM
    if (size != sizeof(FpgaPackgeHead)) {
      printf("read_xdma_thread channel %d short read: %zu / %zu\n", channel, size, sizeof(FpgaPackgeHead));
      assert(0);
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
  while (running) {
    packge = reinterpret_cast<FpgaPackgeHead *>(xdma_mempool.read_busy_chunk());
    if (packge->diff_packge[0].packge_idx != recv_count) {
      printf("read mempool idx failed, packge_idx %d need_idx %d\n", packge->diff_packge[0].packge_idx, recv_count);
      assert(0);
    }
    recv_count++;
    // packge unpack
    for (size_t i = 0; i < DMA_PACKGE_NUM; i++) {
      v_difftest_Batch(packge->diff_packge[i].diff_packge);
    }
    xdma_mempool.set_free_chunk();
  }
}

#else
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
  while (running) {
#ifdef FPGA_SIM
    size_t size = xdma_c2h_sim_read(0, (char *)packge, sizeof(FpgaPackgeHead));
#else
    size_t size = read(xdma_c2h_fd[0], packge, sizeof(FpgaPackgeHead));
#endif // FPGA_SIM
    for (size_t i = 0; i < DMA_PACKGE_NUM; i++) {
      v_difftest_Batch(packge->diff_packge[i].diff_packge);
    }
  }
  free(packge);
}
#endif // USE_THREAD_MEMPOOL

#ifndef FPGA_SIM
bool FpgaXdma::config_bar_access32(uint32_t offset, uint32_t *value, bool is_write) {
  if (value == nullptr) {
    return false;
  }
  if ((offset & 0x3U) != 0) {
    fprintf(stderr, "Config BAR %s offset 0x%x is not 4-byte aligned\n", is_write ? "write" : "read", offset);
    return false;
  }
  if (!ensure_user_fd_open()) {
    return false;
  }

  const uint64_t base = offset & ~0xFFFULL;
  const uint32_t page_offset = offset & 0xFFFU;
  void *map_ptr = mmap(nullptr, XDMA_USER_MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, xdma_user_fd, base);
  if (map_ptr == MAP_FAILED) {
    fprintf(stderr, "Config BAR %s mmap failed at 0x%x: %s\n", is_write ? "write" : "read", offset, strerror(errno));
    return false;
  }

  volatile uint32_t *reg = reinterpret_cast<volatile uint32_t *>(reinterpret_cast<char *>(map_ptr) + page_offset);
  if (is_write) {
    *reg = *value;
  } else {
    *value = *reg;
  }
  __sync_synchronize();
  munmap(map_ptr, XDMA_USER_MAP_SIZE);
  return true;
}

bool FpgaXdma::config_bar_wait_mask(uint32_t offset, uint32_t mask, uint32_t expect, int timeout_ms,
                                    uint32_t *readback) {
  if (readback == nullptr) {
    return false;
  }
  *readback = 0;
  auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeout_ms);
  while (std::chrono::steady_clock::now() < deadline) {
    if (config_bar_read32(offset, readback) && ((*readback & mask) == expect)) {
      return true;
    }
    usleep(H2C_POLL_INTERVAL_US);
  }
  return false;
}

bool FpgaXdma::ensure_user_fd_open() {
  if (xdma_user_fd >= 0) {
    return true;
  }
  xdma_user_fd = open(XDMA_USER, O_RDWR | O_SYNC);
  if (xdma_user_fd < 0) {
    fprintf(stderr, "Failed to open %s: %s\n", XDMA_USER, strerror(errno));
    return false;
  }
  std::cout << "XDMA link " << XDMA_USER << std::endl;
  return true;
}

bool FpgaXdma::ensure_h2c_fd_open() {
  if (xdma_h2c_fd >= 0) {
    return true;
  }
  xdma_h2c_fd = open(XDMA_H2C_DEVICE, O_WRONLY);
  if (xdma_h2c_fd < 0) {
    fprintf(stderr, "Failed to open %s: %s\n", XDMA_H2C_DEVICE, strerror(errno));
    return false;
  }
  std::cout << "XDMA link " << XDMA_H2C_DEVICE << std::endl;
  return true;
}

bool FpgaXdma::config_bar_write32(uint32_t offset, uint32_t value) {
  return config_bar_access32(offset, &value, true);
}

bool FpgaXdma::config_bar_read32(uint32_t offset, uint32_t *value) {
  return config_bar_access32(offset, value, false);
}

void FpgaXdma::dump_h2c_debug_regs(const char *tag) {
  uint32_t arb = 0;
  uint32_t length = 0;
  uint32_t status = 0;
  uint32_t beat_count = 0;
  uint32_t beat_bytes = 0;
  bool ok = true;
  ok &= config_bar_read32(HOST_IO_DDR_ARB_SEL, &arb);
  ok &= config_bar_read32(HOST_IO_H2C_LENGTH, &length);
  ok &= config_bar_read32(HOST_IO_H2C_STATUS, &status);
  ok &= config_bar_read32(HOST_IO_H2C_BEAT_CNT, &beat_count);
  ok &= config_bar_read32(HOST_IO_H2C_BEAT_BYTES, &beat_bytes);
  if (!ok) {
    fprintf(stderr, "H2C debug[%s]: failed to read one or more config BAR regs\n", tag ? tag : "unknown");
    return;
  }
  fprintf(stderr,
          "H2C debug[%s]: DDR_ARB_SEL=0x%x H2C_LENGTH=%u H2C_STATUS=0x%x(active=%u done=%u) H2C_BEAT_CNT=%u "
          "H2C_BEAT_BYTES=%u\n",
          tag ? tag : "unknown", arb, length, status, (status & 0x2U) ? 1 : 0, (status & 0x4U) ? 1 : 0, beat_count,
          beat_bytes);
}

bool FpgaXdma::h2c_stream_write_all(const uint8_t *buf, size_t len, size_t *written_out) {
  if (!ensure_h2c_fd_open()) {
    if (written_out) {
      *written_out = 0;
    }
    return false;
  }

  size_t written_total = 0;
  int transient_retry_left = get_timeout_ms_from_env("H2C_WRITE_TRANSIENT_RETRY", 3);
  while (written_total < len) {
    ssize_t wrote = write(xdma_h2c_fd, buf + written_total, len - written_total);
    if (wrote < 0) {
      const int err = errno;
      if (errno == EINTR) {
        continue;
      }
      // Some XDMA driver paths may surface kernel-internal ERESTARTSYS (512) to userspace.
      if ((err == XDMA_ERR_ERESTARTSYS) && transient_retry_left > 0) {
        --transient_retry_left;
        fprintf(stderr,
                "H2C stream write transient failure at %zu/%zu: errno=%d (%s), retry left=%d\n",
                written_total, len, err, strerror(err), transient_retry_left);
        usleep(1000);
        continue;
      }
      fprintf(stderr, "H2C stream write failed at %zu/%zu: errno=%d (%s)\n", written_total, len, err, strerror(err));
      dump_h2c_debug_regs("write_failed");
      if (written_out) {
        *written_out = written_total;
      }
      return false;
    }
    if (wrote == 0) {
      fprintf(stderr, "H2C stream write short progress at %zu/%zu\n", written_total, len);
      if (written_out) {
        *written_out = written_total;
      }
      return false;
    }
    written_total += static_cast<size_t>(wrote);
  }
  if (written_out) {
    *written_out = written_total;
  }
  return true;
}

bool FpgaXdma::h2c_init_sequence(uint32_t beats) {
  const int cfg_timeout_ms = get_timeout_ms_from_env("H2C_CFG_TIMEOUT_MS", DEFAULT_H2C_CFG_TIMEOUT_MS);
  const int active_timeout_ms = get_timeout_ms_from_env("H2C_ACTIVE_TIMEOUT_MS", DEFAULT_H2C_ACTIVE_TIMEOUT_MS);
  printf("H2C init: beats=%u\n", beats);

  if (!config_bar_write32(HOST_IO_H2C_LENGTH, beats)) {
    fprintf(stderr, "H2C init failed: cannot write H2C_LENGTH\n");
    return false;
  }

  uint32_t readback = 0;
  if (!config_bar_wait_mask(HOST_IO_H2C_LENGTH, 0xFFFFFFFFU, beats, cfg_timeout_ms, &readback)) {
    fprintf(stderr, "H2C init failed: H2C_LENGTH readback mismatch exp=%u got=%u\n", beats, readback);
    return false;
  }

  fpga_io(HOST_IO_DDR_ARB_SEL, true);
  readback = 0;
  if (!config_bar_wait_mask(HOST_IO_DDR_ARB_SEL, H2C_DDR_ARB_SEL_MASK, H2C_DDR_ARB_SEL_MASK, cfg_timeout_ms,
                            &readback)) {
    fprintf(stderr, "H2C init failed: DDR_ARB_SEL not enabled (readback=0x%x)\n", readback);
    return false;
  }
  printf("H2C arbitration enabled\n");
  dump_h2c_debug_regs("post_init");

  // In normal case active should become 1 quickly after arbitration is enabled.
  // If this times out, H2C logic clock/reset/handshake is likely blocked.
  auto active_deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(active_timeout_ms);
  bool active_seen = false;
  uint32_t status = 0;
  while (std::chrono::steady_clock::now() < active_deadline) {
    if (config_bar_read32(HOST_IO_H2C_STATUS, &status) && (status & 0x2U)) {
      active_seen = true;
      break;
    }
    usleep(H2C_POLL_INTERVAL_US);
  }
  if (!active_seen) {
    fprintf(stderr, "H2C warning: active bit not observed within %d ms after init\n", active_timeout_ms);
    dump_h2c_debug_regs("active_timeout");
  } else {
    dump_h2c_debug_regs("active_seen");
  }
  return true;
}

bool FpgaXdma::h2c_complete_sequence(uint32_t expect_beats) {
  const int done_timeout_ms = get_timeout_ms_from_env("H2C_DONE_TIMEOUT_MS", DEFAULT_H2C_DONE_TIMEOUT_MS);
  const int cfg_timeout_ms = get_timeout_ms_from_env("H2C_CFG_TIMEOUT_MS", DEFAULT_H2C_CFG_TIMEOUT_MS);
  const int status_log_interval_ms = get_timeout_ms_from_env("H2C_STATUS_LOG_INTERVAL_MS", 500);
  const int stall_warn_ms = get_timeout_ms_from_env("H2C_STALL_WARN_MS", 2000);
  auto done_deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(done_timeout_ms);
  auto start_time = std::chrono::steady_clock::now();
  auto next_log_time = start_time + std::chrono::milliseconds(status_log_interval_ms);
  auto last_progress_time = start_time;

  uint32_t status = 0;
  uint32_t beat_count = 0;
  bool done_reached = false;
  bool beat_count_ok = false;
  uint32_t last_beat_count = 0;
  bool beat_count_initialized = false;
  bool stall_warned = false;

  while (std::chrono::steady_clock::now() < done_deadline) {
    if (!config_bar_read32(HOST_IO_H2C_STATUS, &status) || !config_bar_read32(HOST_IO_H2C_BEAT_CNT, &beat_count)) {
      fprintf(stderr, "H2C complete failed: cannot read status registers\n");
      break;
    }

    auto now = std::chrono::steady_clock::now();
    const bool active = (status & 0x2U) != 0;
    const bool done = (status & H2C_STATUS_DONE_MASK) || (status & H2C_STATUS_DONE_FALLBACK_MASK);
    const auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - start_time).count();

    if (!beat_count_initialized || beat_count != last_beat_count) {
      beat_count_initialized = true;
      last_beat_count = beat_count;
      last_progress_time = now;
      stall_warned = false;
    }

    if (now >= next_log_time) {
      printf("H2C poll: t=%lldms status=0x%x active=%u done=%u beat_count=%u expect=%u\n",
             static_cast<long long>(elapsed_ms), status, active ? 1 : 0, done ? 1 : 0, beat_count, expect_beats);
      next_log_time = now + std::chrono::milliseconds(status_log_interval_ms);
    }

    const auto stalled_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_progress_time).count();
    if (!stall_warned && active && beat_count < expect_beats && stalled_ms >= stall_warn_ms) {
      fprintf(stderr, "H2C warning: status active but beat_count stalled for %lldms (status=0x%x beat_count=%u "
                      "expect=%u)\n",
              static_cast<long long>(stalled_ms), status, beat_count, expect_beats);
      stall_warned = true;
    }

    if (done) {
      done_reached = true;
      if (beat_count >= expect_beats) {
        beat_count_ok = true;
        break;
      }
    }
    usleep(H2C_POLL_INTERVAL_US);
  }

  if (!done_reached) {
    fprintf(stderr, "H2C complete timeout: status=0x%x beat_count=%u expect=%u\n", status, beat_count, expect_beats);
    return false;
  }
  if (!beat_count_ok) {
    fprintf(stderr, "H2C complete beat_count mismatch: status=0x%x beat_count=%u expect=%u\n", status, beat_count,
            expect_beats);
    return false;
  }

  fpga_io(HOST_IO_DDR_ARB_SEL, false);
  uint32_t arb_sel = 0x1;
  if (!config_bar_wait_mask(HOST_IO_DDR_ARB_SEL, H2C_DDR_ARB_SEL_MASK, 0, cfg_timeout_ms, &arb_sel)) {
    fprintf(stderr, "H2C complete failed: cannot restore DDR arbitration (arb=0x%x)\n", arb_sel);
    return false;
  }

  printf("H2C complete: status=0x%x beat_count=%u\n", status, beat_count);
  return true;
}

bool FpgaXdma::h2c_load_workload() {
  if (simMemory == nullptr) {
    fprintf(stderr, "H2C load failed: simMemory is null\n");
    return false;
  }

  size_t img_size = simMemory->get_img_size();
  if (img_size == 0) {
    fprintf(stderr, "H2C load failed: empty workload image\n");
    return false;
  }
  if (simMemory->as_ptr() == nullptr) {
    fprintf(stderr, "H2C load failed: invalid workload pointer\n");
    return false;
  }

  uint32_t beat_bytes = 0;
  if (!config_bar_read32(HOST_IO_H2C_BEAT_BYTES, &beat_bytes) || beat_bytes == 0) {
    beat_bytes = DEFAULT_H2C_BEAT_BYTES;
    fprintf(stderr, "H2C warning: invalid H2C_BEAT_BYTES register, fallback to %u\n", beat_bytes);
  }

  const uint64_t beats64 = (img_size + beat_bytes - 1) / beat_bytes;
  if (beats64 > std::numeric_limits<uint32_t>::max()) {
    fprintf(stderr, "H2C load failed: workload beats overflow (%lu)\n", beats64);
    return false;
  }
  const uint32_t beats = static_cast<uint32_t>(beats64);
  const size_t padded_size = static_cast<size_t>(beats) * beat_bytes;
  const size_t pad_size = padded_size - img_size;
  std::vector<uint8_t> aligned_payload;
  const uint8_t *send_ptr = reinterpret_cast<const uint8_t *>(simMemory->as_ptr());
  size_t send_size = img_size;
  if (pad_size > 0) {
    aligned_payload.resize(padded_size, 0);
    std::memcpy(aligned_payload.data(), simMemory->as_ptr(), img_size);
    send_ptr = aligned_payload.data();
    send_size = aligned_payload.size();
  }

  printf("Loading workload via real-board H2C stream...\n");
  if (!ensure_h2c_fd_open()) {
    return false;
  }
  if (!h2c_init_sequence(beats)) {
    return false;
  }

  size_t payload_written = 0;
  if (!h2c_stream_write_all(send_ptr, send_size, &payload_written) || payload_written != send_size) {
    fprintf(stderr, "H2C load failed: payload short write expect=%zu got=%zu\n", send_size, payload_written);
    return false;
  }

  if (!h2c_complete_sequence(beats)) {
    return false;
  }

  printf("Workload loaded: %zu bytes (%u beats, sent=%zu bytes)\n", img_size, beats, send_size);
  return true;
}
#endif // FPGA_SIM
