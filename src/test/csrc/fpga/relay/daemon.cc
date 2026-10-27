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
#include "wire.h"
#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <cstdlib>
#include <fcntl.h>
#include <mutex>
#include <signal.h>
#include <sys/mman.h>
#include <thread>
using namespace xdma_relay;

static volatile sig_atomic_t interrupted = 0;
static void interrupt(int) {
  interrupted = 1;
  cancelled = 1;
}
static void wake(int) {}
static std::atomic<bool> stopping{false};

static int connect_peer(const sockaddr_in &address, std::chrono::steady_clock::time_point deadline) {
  while (!interrupted && std::chrono::steady_clock::now() < deadline) {
    int fd = socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC | SOCK_NONBLOCK, 0);
    require(fd >= 0, "relay socket");
    int result = connect(fd, reinterpret_cast<const sockaddr *>(&address), sizeof(address));
    if (result < 0 && errno == EINPROGRESS) {
      pollfd p{fd, POLLOUT, 0};
      if (poll(&p, 1, 100) > 0) {
        int error = 0;
        socklen_t len = sizeof(error);
        if (!getsockopt(fd, SOL_SOCKET, SO_ERROR, &error, &len) && !error)
          result = 0;
      }
    }
    if (!result) {
      require(fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) & ~O_NONBLOCK) == 0, "relay blocking socket");
      auto remaining =
          std::chrono::duration_cast<std::chrono::seconds>(deadline - std::chrono::steady_clock::now()).count();
      socket_timeout(fd, std::max<long>(1, remaining));
      return fd;
    }
    close(fd);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
  errno = ETIMEDOUT;
  require(false, "relay connect deadline");
  return -1;
}

static void stream(int socket, int device, size_t packet, bool compress) {
  uint32_t start;
  if (!get(socket, start) || start != 1) {
    shutdown(socket, SHUT_RDWR);
    return;
  }
  std::mutex mutex;
  std::condition_variable ready;
  std::vector<char> queued;
  uint32_t error = 0;
  bool ended = false;
  std::atomic<bool> stop{false}, done{false};
  std::thread reader([&] {
    void *allocation = nullptr;
    int status = posix_memalign(&allocation, 4096, packet);
    if (status) {
      std::lock_guard<std::mutex> lock(mutex);
      error = status;
      ended = true;
      done = true;
      ready.notify_all();
      return;
    }
    while (!stop && !stopping && !interrupted) {
      ssize_t n = read(device, allocation, packet);
      if (n < 0 && errno == EINTR)
        continue;
      int device_error = n < 0 ? errno : 0;
      std::unique_lock<std::mutex> lock(mutex);
      ready.wait(lock, [&] { return stop || queued.size() + packet <= MAX_BLOCK; });
      if (stop)
        break;
      if (n <= 0) {
        error = device_error;
        ended = n == 0 || device_error != ETIMEDOUT;
        ready.notify_all();
        if (ended)
          break;
        ready.wait(lock, [&] { return stop || !error; });
      } else {
        queued.insert(queued.end(), static_cast<char *>(allocation), static_cast<char *>(allocation) + n);
        ready.notify_all();
      }
    }
    free(allocation);
    done = true;
    ready.notify_all();
  });
  while (!interrupted && !stopping) {
    std::vector<char> batch;
    uint32_t event_error;
    bool eof;
    {
      std::unique_lock<std::mutex> lock(mutex);
      ready.wait_for(lock, std::chrono::milliseconds(1), [&] { return queued.size() >= 65536 || error || ended; });
      size_t count = std::min<size_t>(queued.size(), 65536);
      batch.assign(queued.begin(), queued.begin() + count);
      queued.erase(queued.begin(), queued.begin() + count);
      event_error = batch.empty() ? error : 0;
      if (batch.empty())
        error = 0;
      eof = ended && batch.empty();
      ready.notify_all();
    }
    pollfd p{socket, POLLIN, 0};
    if (poll(&p, 1, 0) > 0)
      break;
    if ((!batch.empty() || event_error || eof) && !block(socket, batch.data(), batch.size(), compress, event_error))
      break;
    if (eof)
      break;
  }
  {
    std::lock_guard<std::mutex> lock(mutex);
    stop = true;
  }
  ready.notify_all();
  while (!done) {
    pthread_kill(reader.native_handle(), SIGUSR1);
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
  reader.join();
  shutdown(socket, SHUT_WR);
}

int main(int argc, char **argv) {
  const char *ip = nullptr, *token_path = nullptr, *prefix = "/dev/xdma0";
  unsigned port = 0, timeout = 120;
  bool compress = false;
  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];
    if (arg == "--zstd") {
      compress = true;
      continue;
    }
    if (arg == "--help") {
      puts(
          "fpga-xdma-relay --connect IPv4 --port N --token-file PATH [--zstd] [--connect-timeout SECONDS] "
          "[--device-prefix PATH]");
      return 0;
    }
    if (++i == argc) {
      fprintf(stderr, "missing relay option value\n");
      return 2;
    }
    if (arg == "--connect")
      ip = argv[i];
    else if (arg == "--token-file")
      token_path = argv[i];
    else if (arg == "--device-prefix")
      prefix = argv[i];
    else if (arg == "--port" || arg == "--connect-timeout") {
      char *end = nullptr;
      unsigned long value = strtoul(argv[i], &end, 10);
      if (!end || *end || !value || value > 65535)
        return 2;
      if (arg == "--port")
        port = value;
      else
        timeout = value;
    } else {
      fprintf(stderr, "unknown relay option\n");
      return 2;
    }
  }
  struct sigaction sa {};
  sa.sa_handler = interrupt;
  sigaction(SIGINT, &sa, nullptr);
  sigaction(SIGTERM, &sa, nullptr);
  sa.sa_handler = wake;
  sigaction(SIGUSR1, &sa, nullptr);
  int control = -1, user = -1, h2c = -1;
  std::vector<int> sockets, devices;
  std::vector<std::thread> workers;
  int result = 0;
  try {
    std::string token = token_file(token_path);
    sockaddr_in address = endpoint(ip, port);
    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(timeout);
    control = connect_peer(address, deadline);
    require(authenticate(control, token, false), "relay authenticate control");
    uint32_t channels, packet;
    require(get(control, channels) && get(control, packet) && channels && channels <= 16 && packet &&
                packet <= MAX_PACKET,
            "relay profile");
    std::string device_prefix = prefix;
    user = open((device_prefix + "_user").c_str(), O_RDWR | O_SYNC | O_CLOEXEC);
    require(put(control, user < 0 ? errno : 0) && user >= 0, "relay open user");
    for (unsigned i = 0; i < channels; ++i) {
      int socket = connect_peer(address, deadline);
      sockets.push_back(socket);
      require(authenticate(socket, token, false), "relay authenticate stream");
      int device = open((device_prefix + "_c2h_" + std::to_string(i)).c_str(), O_RDONLY | O_CLOEXEC);
      devices.push_back(device);
      require(put(socket, i) && put(socket, device < 0 ? errno : 0) && device >= 0, "relay open C2H");
    }
    socket_timeout(control);
    for (int fd: sockets)
      socket_timeout(fd);
    for (unsigned i = 0; i < channels; ++i)
      workers.emplace_back(stream, sockets[i], devices[i], packet, compress);
    while (!interrupted) {
      pollfd p{control, POLLIN, 0};
      if (poll(&p, 1, 100) <= 0)
        continue;
      uint32_t op, address_or_size, value = 0;
      if (!get(control, op) || !get(control, address_or_size))
        break;
      if (op == READ32 || op == WRITE32) {
        require(address_or_size % 4 == 0, "relay unaligned MMIO");
        if (op == WRITE32)
          require(get(control, value), "relay write32 value");
        off_t base = address_or_size & ~uint32_t(4095);
        void *mapped = mmap(nullptr, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, user, base);
        uint32_t status = mapped == MAP_FAILED ? errno : 0;
        if (!status) {
          volatile uint32_t *reg =
              reinterpret_cast<volatile uint32_t *>(static_cast<char *>(mapped) + (address_or_size & 4095));
          if (op == WRITE32)
            *reg = value;
          else
            value = *reg;
          munmap(mapped, 4096);
        }
        if (!put(control, status) || (op == READ32 && !put(control, value)))
          break;
      } else if (op == H2C) {
        require(address_or_size && address_or_size <= MAX_BLOCK, "relay H2C frame limit");
        void *buffer = nullptr;
        int status = posix_memalign(&buffer, 4096, address_or_size);
        require(!status, "relay H2C buffer");
        if (!io(control, buffer, address_or_size, false)) {
          free(buffer);
          break;
        }
        if (h2c < 0)
          h2c = open((device_prefix + "_h2c_0").c_str(), O_WRONLY | O_CLOEXEC);
        status = h2c < 0 ? errno : 0;
        size_t written = 0;
        while (!status && written < address_or_size && !interrupted) {
          ssize_t n = write(h2c, static_cast<char *>(buffer) + written, address_or_size - written);
          if (n < 0 && errno == EINTR)
            continue;
          if (n <= 0) {
            status = n < 0 ? errno : EIO;
            break;
          }
          written += n;
        }
        free(buffer);
        if (!put(control, status) || !put(control, written))
          break;
      } else {
        errno = EPROTO;
        require(false, "relay unknown command");
      }
    }
  } catch (const std::exception &e) {
    fprintf(stderr, "XDMA relay: %s\n", e.what());
    result = 1;
  }
  stopping = true;
  if (control >= 0) {
    shutdown(control, SHUT_RDWR);
    close(control);
  }
  for (int socket: sockets)
    shutdown(socket, SHUT_RDWR);
  for (auto &worker: workers)
    worker.join();
  for (int socket: sockets)
    close(socket);
  for (int device: devices)
    if (device >= 0)
      close(device);
  if (user >= 0)
    close(user);
  if (h2c >= 0)
    close(h2c);
  return result;
}
