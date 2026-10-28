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
#include "xdma_relay.h"
#include "wire.h"
#include <algorithm>
#include <cstdlib>
using namespace xdma_relay;

bool XdmaRelayClient::enabled() {
  return getenv("XDMA_RELAY_LISTEN") != nullptr;
}
XdmaRelayClient::XdmaRelayClient(size_t packet_bytes, unsigned channels) : owner(getpid()) {
  std::string token = token_file(getenv("XDMA_RELAY_TOKEN_FILE"));
  const char *port_string = getenv("XDMA_RELAY_PORT");
  require(port_string != nullptr, "XDMA_RELAY_PORT is required");
  char *end = nullptr;
  unsigned long port = strtoul(port_string, &end, 10);
  require(end && !*end && port <= 65535, "invalid XDMA_RELAY_PORT");
  sockaddr_in address = endpoint(getenv("XDMA_RELAY_LISTEN"), port);
  require(channels && channels <= 16 && packet_bytes && packet_bytes <= MAX_PACKET, "invalid relay profile");
  int listener = socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0);
  require(listener >= 0, "relay listener socket");
  try {
    int yes = 1;
    setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
    require(bind(listener, reinterpret_cast<sockaddr *>(&address), sizeof(address)) == 0 && listen(listener, 16) == 0,
            "relay listen");
    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(120);
    auto accept_peer = [&]() {
      int remaining =
          std::chrono::duration_cast<std::chrono::milliseconds>(deadline - std::chrono::steady_clock::now()).count();
      pollfd p{listener, POLLIN, 0};
      errno = ETIMEDOUT;
      require(remaining > 0 && poll(&p, 1, remaining) > 0, "relay connection deadline");
      int fd = accept4(listener, nullptr, nullptr, SOCK_CLOEXEC);
      require(fd >= 0, "relay accept");
      socket_timeout(fd, std::max(1, (remaining + 999) / 1000));
      if (!authenticate(fd, token, true)) {
        close(fd);
        errno = EACCES;
        require(false, "relay authentication");
      }
      return fd;
    };
    control = accept_peer();
    require(put(control, channels) && put(control, packet_bytes), "relay send profile");
    uint32_t status;
    require(get(control, status) && !status, "relay open control device");
    for (unsigned i = 0; i < channels; ++i) {
      int fd = accept_peer();
      streams.push_back(fd);
      require(get(fd, status) && status == i && get(fd, status) && !status, "relay open C2H device");
    }
    started.resize(channels);
    pending.resize(channels);
    offset.resize(channels);
    socket_timeout(control);
    for (int fd: streams)
      socket_timeout(fd);
    close(listener);
    printf("[fpga-host] XDMA relay connected: %u channels, %zu bytes/packet\n", channels, packet_bytes);
  } catch (...) {
    close(listener);
    stop();
    if (control >= 0)
      close(control);
    for (int fd: streams)
      close(fd);
    throw;
  }
}
XdmaRelayClient::~XdmaRelayClient() {
  stop();
  if (control >= 0)
    close(control);
  for (int fd: streams)
    close(fd);
}
void XdmaRelayClient::stop() {
  if (getpid() != owner)
    return;
  if (control >= 0)
    shutdown(control, SHUT_RDWR);
  for (int fd: streams)
    shutdown(fd, SHUT_RDWR);
}
ssize_t XdmaRelayClient::read(unsigned channel, void *buffer, size_t size) {
  int fd = streams.at(channel);
  if (!started[channel]) {
    if (!put(fd, 1))
      return -1;
    started[channel] = true;
  }
  if (offset[channel] == pending[channel].size()) {
    uint32_t error;
    if (!block(fd, pending[channel], error))
      return -1;
    offset[channel] = 0;
    if (error) {
      errno = error;
      return -1;
    }
    if (pending[channel].empty())
      return 0;
  }
  size_t n = std::min(size, pending[channel].size() - offset[channel]);
  memcpy(buffer, pending[channel].data() + offset[channel], n);
  offset[channel] += n;
  return n;
}
ssize_t XdmaRelayClient::write(const void *buffer, size_t size) {
  std::lock_guard<std::mutex> lock(commands);
  size = std::min<size_t>(size, MAX_BLOCK);
  uint32_t status, written;
  if (!put(control, H2C) || !put(control, size) || !io(control, const_cast<void *>(buffer), size, true) ||
      !get(control, status) || !get(control, written))
    return -1;
  if (status || written > size) {
    errno = status ? status : EPROTO;
    return -1;
  }
  return written;
}
uint32_t XdmaRelayClient::read32(uint64_t address) {
  std::lock_guard<std::mutex> lock(commands);
  uint32_t status, value;
  require(address <= UINT32_MAX && address % 4 == 0, "relay MMIO address");
  require(put(control, READ32) && put(control, address) && get(control, status) && get(control, value), "relay read32");
  errno = status;
  require(!status, "relay device read32");
  return value;
}
void XdmaRelayClient::write32(uint64_t address, uint32_t value) {
  std::lock_guard<std::mutex> lock(commands);
  uint32_t status;
  require(address <= UINT32_MAX && address % 4 == 0, "relay MMIO address");
  require(put(control, WRITE32) && put(control, address) && put(control, value) && get(control, status),
          "relay write32");
  errno = status;
  require(!status, "relay device write32");
}
