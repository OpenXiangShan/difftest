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
#include <cerrno>
#include <chrono>
#include <cinttypes>
#include <cstdint>
#include <cstring>
#include <execinfo.h>
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <sys/mman.h>
#include <unistd.h>
#include <vector>

#define XDMA_USER       "/dev/xdma0_user"
#define XDMA_BYPASS     "/dev/xdma0_bypass"
#define XDMA_C2H_DEVICE "/dev/xdma0_c2h_"
#define XDMA_H2C_DEVICE "/dev/xdma0_h2c_0"

static int getenv_int(const char *name, int default_value) {
  const char *value = getenv(name);
  if (!value || !value[0]) {
    return default_value;
  }

  char *end = nullptr;
  errno = 0;
  long parsed = strtol(value, &end, 0);
  if (errno != 0 || end == value || *end != '\0' || parsed <= 0 || parsed > INT32_MAX) {
    fprintf(stderr, "[fpga-host] invalid integer %s=%s, using %d\n", name, value, default_value);
    return default_value;
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
  signal(SIGINT, handle_sigint);
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
#ifdef CONFIG_USE_XDMA_H2C
  xdma_h2c_fd = open(XDMA_H2C_DEVICE, O_WRONLY);
  if (xdma_h2c_fd == -1) {
    std::cout << XDMA_H2C_DEVICE << std::endl;
    perror("Failed to open XDMA device");
    exit(-1);
  }
  std::cout << "XDMA link " << XDMA_H2C_DEVICE << std::endl;
#endif
}

void FpgaXdma::bar_write32(uint64_t addr, uint32_t value) {
  device_write(false, nullptr, addr, value);
}

uint32_t FpgaXdma::bar_read32(uint64_t addr) {
  uint64_t pg_size = sysconf(_SC_PAGE_SIZE);
  uint64_t size = 0x1000;
  uint64_t aligned_size = (size + 0xffful) & ~0xffful;
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

  uint32_t value = ((volatile uint32_t *)m_ptr)[offset >> 2];
  munmap(m_ptr, aligned_size);
  close(fd);
  return value;
}

bool FpgaXdma::drain_c2h_until_idle(int channel, int idle_timeout_ms, size_t *drained_bytes) {
  if (drained_bytes) {
    *drained_bytes = 0;
  }
#ifdef FPGA_SIM
  (void)channel;
  (void)idle_timeout_ms;
  return true;
#else
  int fd = xdma_c2h_fd[channel];
  int old_flags = fcntl(fd, F_GETFL, 0);
  if (old_flags < 0) {
    perror("failed to get XDMA C2H fd flags");
    return false;
  }
  if (fcntl(fd, F_SETFL, old_flags | O_NONBLOCK) < 0) {
    perror("failed to set XDMA C2H nonblocking mode");
    return false;
  }
  auto restore_flags = [&]() {
    if (fcntl(fd, F_SETFL, old_flags) < 0) {
      perror("failed to restore XDMA C2H fd flags");
    }
  };

  std::vector<uint8_t> discard(1u << 20);
  int idle_ms = 0;
  while (true) {
    ssize_t n = read(fd, discard.data(), discard.size());
    if (n > 0) {
      if (drained_bytes) {
        *drained_bytes += static_cast<size_t>(n);
      }
      idle_ms = 0;
      continue;
    }
    if (n == 0) {
      restore_flags();
      return true;
    }
    if (errno == EINTR) {
      continue;
    }
    if (errno == ETIMEDOUT) {
      restore_flags();
      return true;
    }
    if (errno == EAGAIN || errno == EWOULDBLOCK) {
      int poll_ms = idle_timeout_ms - idle_ms;
      if (poll_ms > 10) {
        poll_ms = 10;
      }
      if (poll_ms <= 0) {
        restore_flags();
        return true;
      }

      struct pollfd pfd;
      pfd.fd = fd;
      pfd.events = POLLIN;
      pfd.revents = 0;
      int ret = poll(&pfd, 1, poll_ms);
      if (ret > 0) {
        if (pfd.revents & (POLLERR | POLLHUP | POLLNVAL)) {
          fprintf(stderr, "XDMA C2H drain poll error revents=0x%x\n", pfd.revents);
          restore_flags();
          return false;
        }
        if (pfd.revents & POLLIN) {
          idle_ms = 0;
          continue;
        }
      }
      if (ret < 0) {
        if (errno == EINTR) {
          continue;
        }
        perror("XDMA C2H drain poll failed");
        restore_flags();
        return false;
      }
      idle_ms += poll_ms;
      continue;
    }

    perror("XDMA C2H drain read failed");
    restore_flags();
    return false;
  }
#endif // FPGA_SIM
}

bool FpgaXdma::read_c2h_exact(int channel, void *buf, size_t n_bytes, int idle_timeout_ms) {
  uint8_t *dst = reinterpret_cast<uint8_t *>(buf);
  size_t done = 0;

#ifndef FPGA_SIM
  int fd = xdma_c2h_fd[channel];
  int old_flags = fcntl(fd, F_GETFL, 0);
  if (old_flags < 0) {
    perror("failed to get XDMA C2H fd flags");
    return false;
  }
  if (fcntl(fd, F_SETFL, old_flags | O_NONBLOCK) < 0) {
    perror("failed to set XDMA C2H nonblocking mode");
    return false;
  }
  auto restore_flags = [&]() {
    if (fcntl(fd, F_SETFL, old_flags) < 0) {
      perror("failed to restore XDMA C2H fd flags");
    }
  };
  int idle_ms = 0;
#endif // FPGA_SIM

  while (done < n_bytes) {
    size_t want = n_bytes - done;
    if (want > (1u << 20)) {
      want = 1u << 20;
    }
#ifdef FPGA_SIM
    ssize_t n = xdma_sim_read(channel, reinterpret_cast<char *>(dst + done), want);
#else
    ssize_t n = read(fd, dst + done, want);
#endif // FPGA_SIM
    if (n < 0) {
      if (errno == EINTR) {
        continue;
      }
#ifndef FPGA_SIM
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        struct pollfd pfd;
        pfd.fd = fd;
        pfd.events = POLLIN;
        pfd.revents = 0;
        int poll_ms = idle_timeout_ms - idle_ms;
        if (poll_ms > 100) {
          poll_ms = 100;
        }
        if (poll_ms <= 0) {
          fprintf(stderr, "XDMA C2H read idle timeout after %zu/%zu bytes\n", done, n_bytes);
          restore_flags();
          return false;
        }
        int ret = poll(&pfd, 1, poll_ms);
        if (ret > 0) {
          if (pfd.revents & (POLLERR | POLLHUP | POLLNVAL)) {
            fprintf(stderr, "XDMA C2H poll error revents=0x%x after %zu/%zu bytes\n", pfd.revents, done, n_bytes);
            restore_flags();
            return false;
          }
          if (pfd.revents & POLLIN) {
            idle_ms = 0;
            continue;
          }
        }
        if (ret < 0) {
          if (errno == EINTR) {
            continue;
          }
          perror("XDMA C2H poll failed");
          restore_flags();
          return false;
        }
        idle_ms += poll_ms;
        if (idle_ms >= idle_timeout_ms) {
          fprintf(stderr, "XDMA C2H read idle timeout after %zu/%zu bytes\n", done, n_bytes);
          restore_flags();
          return false;
        }
        continue;
      }
#endif // FPGA_SIM
      perror("XDMA C2H read failed");
#ifndef FPGA_SIM
      restore_flags();
#endif // FPGA_SIM
      return false;
    }
    if (n == 0) {
      fprintf(stderr, "XDMA C2H read returned EOF after %zu/%zu bytes\n", done, n_bytes);
#ifndef FPGA_SIM
      restore_flags();
#endif // FPGA_SIM
      return false;
    }
    done += static_cast<size_t>(n);
#ifndef FPGA_SIM
    idle_ms = 0;
#endif // FPGA_SIM
  }
#ifndef FPGA_SIM
  restore_flags();
#endif // FPGA_SIM
  return true;
}

bool FpgaXdma::sync_ddr_to_sim_memory(uint64_t ddr_addr, uint64_t n_bytes, bool compare_with_image,
                                      bool strict_compare) {
#ifdef FPGA_SIM
  (void)ddr_addr;
  (void)n_bytes;
  (void)compare_with_image;
  (void)strict_compare;
  printf("[fpga-host] XDMA DDR sync is not available in FPGA_SIM\n");
  return false;
#else
  if (n_bytes == 0) {
    return true;
  }
  if (!simMemory || !simMemory->as_ptr()) {
    fprintf(stderr, "[fpga-host] XDMA DDR sync requires mmap-backed simMemory\n");
    return false;
  }
  if ((ddr_addr & 63ull) != 0) {
    fprintf(stderr, "[fpga-host] XDMA DDR sync address must be 64-byte aligned: 0x%" PRIx64 "\n", ddr_addr);
    return false;
  }
  static constexpr uint64_t ddr_sync_addr_limit = 1ull << 40;
  if (ddr_addr >= ddr_sync_addr_limit) {
    fprintf(stderr, "[fpga-host] XDMA DDR sync address exceeds 40-bit DDR port: 0x%" PRIx64 "\n", ddr_addr);
    return false;
  }
  if (n_bytes > simMemory->get_size()) {
    fprintf(stderr, "[fpga-host] XDMA DDR sync size 0x%" PRIx64 " exceeds simMemory size 0x%" PRIx64 "\n", n_bytes,
            simMemory->get_size());
    return false;
  }

  uint64_t aligned_bytes = (n_bytes + 63ull) & ~63ull;
  if (aligned_bytes > UINT32_MAX) {
    fprintf(stderr, "[fpga-host] XDMA DDR sync size 0x%" PRIx64 " exceeds 32-bit BAR size field\n", aligned_bytes);
    return false;
  }
  if (aligned_bytes > ddr_sync_addr_limit - ddr_addr) {
    fprintf(stderr, "[fpga-host] XDMA DDR sync range [0x%" PRIx64 ", +0x%" PRIx64 "] exceeds 40-bit DDR port\n",
            ddr_addr, aligned_bytes);
    return false;
  }

  uint32_t status = bar_read32(HOST_IO_DDR_SYNC_CTRL);
  if ((status & HOST_IO_DDR_SYNC_SUPPORTED) == 0) {
    fprintf(stderr, "[fpga-host] bitstream does not report XDMA DDR sync support, status=0x%08x\n", status);
    return false;
  }

  void *buffer = nullptr;
  int ret = posix_memalign(&buffer, 4096, static_cast<size_t>(aligned_bytes));
  if (ret != 0 || !buffer) {
    errno = ret;
    perror("posix_memalign failed");
    return false;
  }
  memset(buffer, 0, static_cast<size_t>(aligned_bytes));

  printf("[fpga-host] syncing DDR[0x%" PRIx64 ", +0x%" PRIx64 "] to simMemory via XDMA C2H\n", ddr_addr, n_bytes);
  fflush(stdout);

  int drain_idle_ms = getenv_int("FPGA_XDMA_SYNC_DDR_DRAIN_IDLE_MS", 50);
  size_t drained_bytes = 0;
  if (!drain_c2h_until_idle(0, drain_idle_ms, &drained_bytes)) {
    free(buffer);
    return false;
  }
  if (drained_bytes != 0) {
    printf("[fpga-host] drained %zu stale C2H bytes before XDMA DDR sync\n", drained_bytes);
  }

  const auto sync_begin = std::chrono::steady_clock::now();
  bar_write32(HOST_IO_DDR_SYNC_CTRL, HOST_IO_DDR_SYNC_CLEAR);
  bar_write32(HOST_IO_DDR_SYNC_ADDR_LO, static_cast<uint32_t>(ddr_addr));
  bar_write32(HOST_IO_DDR_SYNC_ADDR_HI, static_cast<uint32_t>(ddr_addr >> 32));
  bar_write32(HOST_IO_DDR_SYNC_SIZE, static_cast<uint32_t>(n_bytes));
  bar_write32(HOST_IO_DDR_SYNC_CTRL, HOST_IO_DDR_SYNC_START);

  int idle_timeout_ms = getenv_int("FPGA_XDMA_SYNC_DDR_IDLE_TIMEOUT_MS", 10000);
  const auto c2h_begin = std::chrono::steady_clock::now();
  bool ok = read_c2h_exact(0, buffer, static_cast<size_t>(aligned_bytes), idle_timeout_ms);
  const auto c2h_end = std::chrono::steady_clock::now();
  if (ok) {
    bool done = false;
    for (int i = 0; i < 10000; i++) {
      status = bar_read32(HOST_IO_DDR_SYNC_CTRL);
      if (status & HOST_IO_DDR_SYNC_ERROR) {
        fprintf(stderr, "[fpga-host] XDMA DDR sync hardware error, status=0x%08x\n", status);
        ok = false;
        break;
      }
      if (status & HOST_IO_DDR_SYNC_DONE) {
        done = true;
        break;
      }
      usleep(1000);
    }
    if (!done) {
      fprintf(stderr, "[fpga-host] XDMA DDR sync timed out, status=0x%08x\n", status);
      ok = false;
    }
  }
  const auto sync_end = std::chrono::steady_clock::now();

  if (ok) {
    double c2h_seconds = std::chrono::duration<double>(c2h_end - c2h_begin).count();
    double total_seconds = std::chrono::duration<double>(sync_end - sync_begin).count();
    double aligned_mib = static_cast<double>(aligned_bytes) / (1024.0 * 1024.0);
    double c2h_mib_s = c2h_seconds > 0.0 ? aligned_mib / c2h_seconds : 0.0;
    double total_mib_s = total_seconds > 0.0 ? aligned_mib / total_seconds : 0.0;
    printf("[fpga-host] XDMA DDR sync C2H read: 0x%" PRIx64 " aligned bytes in %.6f s, %.3f MiB/s\n", aligned_bytes,
           c2h_seconds, c2h_mib_s);
    printf("[fpga-host] XDMA DDR sync total hardware phase: %.6f s, %.3f MiB/s\n", total_seconds, total_mib_s);
  }

  if (ok && compare_with_image) {
    const uint8_t *expected = reinterpret_cast<const uint8_t *>(simMemory->as_ptr());
    const uint8_t *actual = reinterpret_cast<const uint8_t *>(buffer);
    uint64_t mismatch_count = 0;
    uint64_t first_mismatch = 0;
    for (uint64_t i = 0; i < n_bytes; i++) {
      if (expected[i] != actual[i]) {
        if (mismatch_count == 0) {
          first_mismatch = i;
        }
        mismatch_count++;
      }
    }
    if (mismatch_count != 0) {
      fprintf(stderr,
              "[fpga-host] XDMA DDR sync differs from init_ram image: "
              "%" PRIu64 " mismatches, first offset=0x%" PRIx64 "\n",
              mismatch_count, first_mismatch);
      if (strict_compare) {
        ok = false;
      }
    } else {
      printf("[fpga-host] XDMA DDR sync matches init_ram image for 0x%" PRIx64 " bytes\n", n_bytes);
    }
  }

  if (ok) {
    memcpy(simMemory->as_ptr(), buffer, static_cast<size_t>(n_bytes));
    simMemory->extend_img_size(n_bytes);
    printf("[fpga-host] copied 0x%" PRIx64 " bytes from XDMA DDR sync buffer into simMemory\n", n_bytes);
  }

  bar_write32(HOST_IO_DDR_SYNC_CTRL, HOST_IO_DDR_SYNC_CLEAR);
  free(buffer);
  return ok;
#endif // FPGA_SIM
}

// write xdma_bypass memory or xdma_user
void FpgaXdma::device_write(bool is_bypass, const char *workload, uint64_t addr, uint64_t value) {
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
    xdma_sim_close(i);
#else
    close(xdma_c2h_fd[i]);
#endif // FPGA_SIM
  }

  if (process_thread.joinable())
    process_thread.join();
#ifdef CONFIG_USE_XDMA_H2C
  close(xdma_h2c_fd);
#endif
}

void FpgaXdma::read_xdma_thread(int channel) {
  size_t mem_get_idx = 0;
  while (running) {
    char *mem = xdma_mempool.get_free_chunk(&mem_get_idx);
#ifdef FPGA_SIM
    size_t size = xdma_sim_read(channel, mem, sizeof(FpgaPackgeHead));
#else
    size_t size = read(xdma_c2h_fd[channel], mem, sizeof(FpgaPackgeHead));
#endif // FPGA_SIM
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
    size_t size = xdma_sim_read(0, (char *)packge, sizeof(FpgaPackgeHead));
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
