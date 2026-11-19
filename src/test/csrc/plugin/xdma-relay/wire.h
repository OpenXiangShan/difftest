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
#ifndef __XDMA_RELAY_WIRE_H__
#define __XDMA_RELAY_WIRE_H__
#include <arpa/inet.h>
#include <cerrno>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <poll.h>
#include <signal.h>
#include <stdexcept>
#include <string>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>
#ifndef NO_ZSTD_COMPRESSION
#include <zstd.h>
#endif

namespace xdma_relay {
inline volatile sig_atomic_t cancelled = 0;
constexpr uint32_t MAGIC = 0x58445231;
constexpr uint32_t MAX_BLOCK = 1024 * 1024;
constexpr uint32_t MAX_PACKET = 1024 * 1024;
constexpr uint32_t READ32 = 1, WRITE32 = 2, H2C = 3;
inline void require(bool ok, const char *what) {
  if (!ok)
    throw std::runtime_error(std::string(what) + ": " + strerror(errno ? errno : EPROTO));
}
inline bool io(int fd, void *buffer, size_t size, bool writing) {
  char *p = static_cast<char *>(buffer);
  while (size) {
    if (cancelled) {
      errno = EINTR;
      return false;
    }
    ssize_t n = writing ? send(fd, p, size, MSG_NOSIGNAL) : recv(fd, p, size, 0);
    if (n < 0 && errno == EINTR)
      continue;
    if (n <= 0) {
      if (!n)
        errno = ECONNRESET;
      return false;
    }
    p += n;
    size -= n;
  }
  return true;
}
inline bool put(int fd, uint32_t value) {
  value = htonl(value);
  return io(fd, &value, sizeof(value), true);
}
inline bool get(int fd, uint32_t &value) {
  if (!io(fd, &value, sizeof(value), false))
    return false;
  value = ntohl(value);
  return true;
}
inline std::string token_file(const char *path) {
  require(path && *path, "XDMA relay token file is required");
  std::ifstream file(path, std::ios::binary);
  char bytes[131];
  file.read(bytes, sizeof(bytes));
  std::string token(bytes, file.gcount());
  while (!token.empty() && (token.back() == '\n' || token.back() == '\r'))
    token.pop_back();
  errno = EINVAL;
  require(file.is_open() && token.size() >= 32 && token.size() <= 128, "invalid XDMA relay token file");
  return token;
}
inline bool authenticate(int fd, const std::string &token, bool listener) {
  uint32_t magic = 0, size = 0;
  if (!listener)
    return put(fd, MAGIC) && put(fd, token.size()) && io(fd, const_cast<char *>(token.data()), token.size(), true) &&
           get(fd, magic) && magic == MAGIC && get(fd, size) && size == 0;
  char peer[128];
  bool valid = get(fd, magic) && magic == MAGIC && get(fd, size) && size == token.size() && io(fd, peer, size, false) &&
               memcmp(peer, token.data(), size) == 0;
  return valid && put(fd, MAGIC) && put(fd, 0);
}
inline void socket_timeout(int fd, unsigned seconds = 300) {
  timeval timeout{static_cast<time_t>(seconds), 0};
  require(setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) == 0 &&
              setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout)) == 0,
          "relay socket timeout");
}
inline sockaddr_in endpoint(const char *ip, unsigned port) {
  sockaddr_in address{};
  address.sin_family = AF_INET;
  address.sin_port = htons(port);
  errno = EINVAL;
  require(port && port <= 65535 && ip && inet_pton(AF_INET, ip, &address.sin_addr) == 1 &&
              address.sin_addr.s_addr != INADDR_ANY,
          "relay requires explicit IPv4 and valid port");
  return address;
}
inline bool block(int fd, const void *data, uint32_t size, bool compress, uint32_t error = 0) {
  std::vector<char> encoded;
#ifndef NO_ZSTD_COMPRESSION
  if (compress && size) {
    encoded.resize(ZSTD_compressBound(size));
    size_t n = ZSTD_compress(encoded.data(), encoded.size(), data, size, 1);
    if (ZSTD_isError(n)) {
      errno = EIO;
      return false;
    }
    if (n < size)
      encoded.resize(n);
    else
      encoded.clear();
  }
#else
  if (compress) {
    errno = ENOTSUP;
    return false;
  }
#endif
  return put(fd, error) && put(fd, size) && put(fd, encoded.size()) &&
         io(fd, encoded.empty() ? const_cast<void *>(data) : encoded.data(), encoded.empty() ? size : encoded.size(),
            true);
}
inline bool block(int fd, std::vector<char> &data, uint32_t &error) {
  uint32_t size, compressed;
  if (!get(fd, error) || !get(fd, size) || !get(fd, compressed))
    return false;
  if (size > MAX_BLOCK || compressed > MAX_BLOCK + MAX_BLOCK / 128 + 1024 || (error && (size || compressed))) {
    errno = EPROTO;
    return false;
  }
  data.resize(size);
  if (!compressed)
    return io(fd, data.data(), size, false);
#ifndef NO_ZSTD_COMPRESSION
  std::vector<char> encoded(compressed);
  if (!io(fd, encoded.data(), compressed, false))
    return false;
  size_t n = ZSTD_decompress(data.data(), size, encoded.data(), compressed);
  if (ZSTD_isError(n) || n != size) {
    errno = EPROTO;
    return false;
  }
  return true;
#else
  errno = ENOTSUP;
  return false;
#endif
}
} // namespace xdma_relay
#endif
