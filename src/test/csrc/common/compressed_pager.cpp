/***************************************************************************************
* Copyright (c) 2026 Beijing Institute of Open Source Chip
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

#include "compressed_pager.h"

#if defined(__linux__) && !defined(NO_ZSTD_COMPRESSION)
#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <linux/memfd.h>
#include <linux/userfaultfd.h>
#include <poll.h>
#include <string>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <zstd.h>

namespace {
[[noreturn]] void fail(const char *what) {
  perror(what);
  exit(EXIT_FAILURE);
}

void read_exact(int fd, void *dest, size_t size, uint64_t offset) {
  auto *p = static_cast<uint8_t *>(dest);
  while (size) {
    ssize_t n = pread(fd, p, size, offset);
    if (n < 0 && errno == EINTR)
      continue;
    if (n <= 0) {
      if (n == 0)
        errno = EIO;
      fail("read compressed RAM");
    }
    p += n;
    offset += n;
    size -= n;
  }
}
} // namespace

CompressedPager::CompressedPager(const char *path, uint64_t expected_size) {
  static_assert(sizeof(Entry) == 16, "RTLCMPV2 entry layout");
  int index = open(path, O_RDONLY | O_CLOEXEC);
  if (index < 0)
    fail("open compressed RAM index");
  struct {
    char magic[8];
    uint64_t chunk, bytes, count;
  } header;
  read_exact(index, &header, sizeof(header), 0);
  page_size = sysconf(_SC_PAGESIZE);
  if (memcmp(header.magic, "RTLCMPV2", 8) || header.bytes != expected_size || header.chunk != chunk_size ||
      header.bytes % page_size || chunk_size % page_size ||
      header.count != (header.bytes + chunk_size - 1) / chunk_size) {
    fprintf(stderr, "Invalid RTLCMPV2 RAM index or RAM size\n");
    exit(EXIT_FAILURE);
  }
  length = header.bytes;
  entries.resize(header.count);
  loaded.resize(header.count);
  read_exact(index, entries.data(), entries.size() * sizeof(Entry), sizeof(header));
  close(index);
  std::string name(path);
  name = name.substr(0, name.find_last_of('/') + 1) + "frames.zst";
  data_fd = open(name.c_str(), O_RDONLY | O_CLOEXEC);
  if (data_fd < 0)
    fail("open compressed RAM frames");
  root_fd = memfd_create("difftest-ram-compressed", MFD_CLOEXEC);
  if (root_fd < 0 || ftruncate(root_fd, length))
    fail("create compressed RAM backing");
  canonical = static_cast<uint8_t *>(mmap(nullptr, length, PROT_READ | PROT_WRITE, MAP_SHARED, root_fd, 0));
  if (canonical == MAP_FAILED)
    fail("map compressed RAM backing");
  fault_fd = syscall(SYS_userfaultfd, O_CLOEXEC | O_NONBLOCK | UFFD_USER_MODE_ONLY);
  if (fault_fd < 0)
    fail("create compressed RAM userfaultfd");
  uffdio_api api{};
  api.api = UFFD_API;
  api.features = UFFD_FEATURE_MISSING_SHMEM | UFFD_FEATURE_MINOR_SHMEM;
  if (ioctl(fault_fd, UFFDIO_API, &api))
    fail("initialize compressed RAM userfaultfd");
  worker = std::thread(&CompressedPager::serve, this);
}

void *CompressedPager::map_private_branch() {
  void *p = mmap(nullptr, length, PROT_READ | PROT_WRITE, MAP_PRIVATE, root_fd, 0);
  if (p == MAP_FAILED)
    fail("map compressed RAM branch");
  std::lock_guard<std::mutex> guard(lock);
  aliases.push_back(reinterpret_cast<uint64_t>(p));
  uffdio_register reg{};
  reg.range.start = reinterpret_cast<uint64_t>(p);
  reg.range.len = length;
  reg.mode = UFFDIO_REGISTER_MODE_MISSING | UFFDIO_REGISTER_MODE_MINOR;
  if (ioctl(fault_fd, UFFDIO_REGISTER, &reg))
    fail("register compressed RAM branch");
  return p;
}

void CompressedPager::write(uint64_t offset, const void *data, uint64_t size) {
  // The DUT write has already faulted in every destination chunk.
  std::lock_guard<std::mutex> guard(lock);
  memcpy(canonical + offset, data, size);
}

void CompressedPager::serve() {
  std::vector<uint8_t> packed(ZSTD_compressBound(chunk_size)), decoded(chunk_size);
  while (!stop.load()) {
    pollfd p{fault_fd, POLLIN, 0};
    int ready = poll(&p, 1, 50);
    if (ready < 0) {
      if (errno == EINTR)
        continue;
      fail("poll compressed RAM faults");
    }
    if (!ready)
      continue;
    uffd_msg msg{};
    ssize_t n = read(fault_fd, &msg, sizeof(msg));
    if (n < 0 && (errno == EAGAIN || errno == EINTR))
      continue;
    if (n != sizeof(msg) || msg.event != UFFD_EVENT_PAGEFAULT) {
      fprintf(stderr, "Invalid compressed RAM fault event\n");
      exit(EXIT_FAILURE);
    }
    uint64_t address = msg.arg.pagefault.address & ~(page_size - 1);
    std::lock_guard<std::mutex> guard(lock);
    auto alias = std::find_if(aliases.begin(), aliases.end(),
                              [=](uint64_t base) { return address >= base && address - base < length; });
    if (alias == aliases.end()) {
      fprintf(stderr, "Unknown compressed RAM fault address\n");
      exit(EXIT_FAILURE);
    }
    uint64_t offset = address - *alias;
    uint64_t i = offset / chunk_size;
    const auto &entry = entries[i];
    if (entry.compressed && !loaded[i]) {
      if (entry.compressed > packed.size() || entry.decoded != std::min(chunk_size, length - i * chunk_size)) {
        fprintf(stderr, "Invalid compressed RAM frame size\n");
        exit(EXIT_FAILURE);
      }
      read_exact(data_fd, packed.data(), entry.compressed, entry.offset);
      size_t size = ZSTD_decompress(decoded.data(), decoded.size(), packed.data(), entry.compressed);
      if (ZSTD_isError(size) || size != entry.decoded) {
        fprintf(stderr, "Cannot decompress RAM frame %lu\n", i);
        exit(EXIT_FAILURE);
      }
      memcpy(canonical + i * chunk_size, decoded.data(), size);
      loaded[i] = 1;
    } else {
      // Populate zero holes without overwriting earlier initialization writes.
      (void)*static_cast<volatile uint8_t *>(canonical + offset);
    }
    uffdio_continue cont{};
    cont.range.start = address;
    cont.range.len = page_size;
    // A queued fault for the same page may already have installed its PTE.
    if (ioctl(fault_fd, UFFDIO_CONTINUE, &cont) && errno != EEXIST)
      fail("continue compressed RAM fault");
  }
}

CompressedPager::~CompressedPager() {
  stop.store(true);
  worker.join();
  close(fault_fd);
  munmap(canonical, length);
  close(root_fd);
  close(data_fd);
}
#endif
