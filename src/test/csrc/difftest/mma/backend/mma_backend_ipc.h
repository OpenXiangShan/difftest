/***************************************************************************************
* Copyright (c) 2020-2026 Institute of Computing Technology, Chinese Academy of Sciences
* Copyright (c) 2026 Beijing Institute of Open Source Chip (BOSC)
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

#ifndef __MMA_BACKEND_IPC_H__
#define __MMA_BACKEND_IPC_H__

#include "mma/backend/mma_backend.h"

#ifdef CONFIG_DIFFTEST_AMUCTRLEVENT
#include <algorithm>
#include <cerrno>
#include <chrono>
#include <climits>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <dirent.h>
#include <fcntl.h>
#include <poll.h>
#include <stdexcept>
#include <string>
#include <sys/file.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/un.h>
#include <unistd.h>

namespace mma_process {

const int kWorkerSocket = 3;
const int kWorkerControl = 4;
const int kReplyTimeoutMs = 60000;
const size_t kPoolSlots = static_cast<size_t>(CONFIG_DIFF_MMA_BATCH_SIZE) * CONFIG_DIFF_MMA_BACKPRESSURE_FACTOR + 2;
// Ordinary-array fallback requests use a separate batch beyond the pool.
const size_t kSharedSlots = kPoolSlots + CONFIG_DIFF_MMA_BATCH_SIZE;

struct Message {
  uint64_t sequence;
  uint32_t count;
  int32_t status;
};

// Operands and canonical DUT results are shared with the MMA service.
// SharedStorage preserves private DUT snapshots for LightSSS replay.
struct SharedBatchOperands {
  pid_t owner;
  DifftestAmuCtrlEvent events[CONFIG_DIFF_MMA_BATCH_SIZE];
  uint8_t passed[CONFIG_DIFF_MMA_BATCH_SIZE];
  uint32_t slots[CONFIG_DIFF_MMA_BATCH_SIZE][4];
  uint64_t src1[kSharedSlots][CONFIG_DIFF_AMU_AB_REG_SIZE_BYTES / sizeof(uint64_t)];
  uint64_t src2[kSharedSlots][CONFIG_DIFF_AMU_AB_REG_SIZE_BYTES / sizeof(uint64_t)];
  uint64_t src3[kSharedSlots][CONFIG_DIFF_AMU_C_REG_SIZE_BYTES / sizeof(uint64_t)];
};

struct SharedBatch : SharedBatchOperands {
  uint8_t result_padding[65536 - sizeof(SharedBatchOperands) % 65536];
  uint64_t dut_results[kSharedSlots][CONFIG_DIFF_AMU_C_REG_SIZE_BYTES / sizeof(uint64_t)];
};

// The simulator writes DUT results directly into this shared tail region.
// LightSSS stops DUT writers before fork. Snapshots get private pages at
// the same addresses, so existing ROB pointers remain valid while the parent
// continues recycling shared slots.
class SharedStorage {
public:
  explicit SharedStorage(void *mapping) : mapping(mapping) {
    const long page_size = sysconf(_SC_PAGESIZE);
    if (page_size <= 0)
      throw std::runtime_error("Cannot determine MMA result page size");
    const size_t page = page_size;
    result = reinterpret_cast<uint8_t *>(static_cast<SharedBatch *>(mapping)->dut_results);
    bytes = (sizeof(static_cast<SharedBatch *>(mapping)->dut_results) + page - 1) / page * page;
    if (reinterpret_cast<uintptr_t>(result) % page)
      throw std::runtime_error("MMA result storage is not page-aligned");
    std::lock_guard<std::mutex> lock(mutex());
    static bool registered = false;
    if (!registered) {
      if (pthread_atfork(before_fork, after_parent, after_child))
        throw std::runtime_error("Cannot register MMA result snapshot handlers");
      registered = true;
    }
    mappings().push_back(this);
  }

  ~SharedStorage() {
    std::lock_guard<std::mutex> lock(mutex());
    auto &live = mappings();
    live.erase(std::find(live.begin(), live.end(), this));
    munmap(mapping, sizeof(SharedBatch));
  }

private:
  void *mapping;
  uint8_t *result;
  size_t bytes;
  void *snapshot = nullptr;
  bool private_results = false;

  // Fork handlers remain installed until process exit.
  static std::mutex &mutex() {
    static auto *value = new std::mutex;
    return *value;
  }
  static std::vector<SharedStorage *> &mappings() {
    static auto *value = new std::vector<SharedStorage *>;
    return *value;
  }
  static void fail() {
    const char message[] = "MMA backend: cannot preserve DUT results across fork\n";
    const ssize_t ignored = write(STDERR_FILENO, message, sizeof(message) - 1);
    (void)ignored;
    _exit(EXIT_FAILURE);
  }
  static void before_fork() {
    mutex().lock();
    for (auto *storage: mappings()) {
      if (storage->private_results)
        continue;
      storage->snapshot = mmap(nullptr, storage->bytes, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
      if (storage->snapshot == MAP_FAILED)
        fail();
      memcpy(storage->snapshot, storage->result, storage->bytes);
    }
  }
  static void after_parent() {
    for (auto *storage: mappings()) {
      if (!storage->snapshot)
        continue;
      munmap(storage->snapshot, storage->bytes);
      storage->snapshot = nullptr;
    }
    mutex().unlock();
  }
  static void after_child() {
    for (auto *storage: mappings()) {
      if (!storage->snapshot)
        continue;
      if (reinterpret_cast<void *>(syscall(SYS_mremap, storage->snapshot, storage->bytes, storage->bytes,
                                           MREMAP_MAYMOVE | MREMAP_FIXED, storage->result)) == MAP_FAILED)
        fail();
      storage->snapshot = nullptr;
      storage->private_results = true;
    }
    mutex().unlock();
  }
};

static uint8_t *operand_storage(SharedBatch *batch, size_t operand) {
  return reinterpret_cast<uint8_t *>(operand == 0 ? batch->src1[0] : operand == 1 ? batch->src2[0] : batch->src3[0]);
}

static size_t operand_bytes(size_t operand) {
  return operand == 2 ? CONFIG_DIFF_AMU_C_REG_SIZE_BYTES : CONFIG_DIFF_AMU_AB_REG_SIZE_BYTES;
}

static bool send_message(int fd, const Message &message) {
  ssize_t sent;
  do {
    sent = send(fd, &message, sizeof(message), MSG_NOSIGNAL);
  } while (sent < 0 && errno == EINTR);
  return sent == sizeof(message);
}

static bool receive_message(int fd, Message &message, bool timeout) {
  if (timeout) {
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(kReplyTimeoutMs);
    while (true) {
      const auto left =
          std::chrono::duration_cast<std::chrono::milliseconds>(deadline - std::chrono::steady_clock::now()).count();
      if (left <= 0) {
        errno = ETIMEDOUT;
        return false;
      }
      pollfd event = {fd, POLLIN, 0};
      const int ready = poll(&event, 1, static_cast<int>(left));
      if (ready < 0 && errno == EINTR)
        continue;
      if (ready <= 0 || !(event.revents & POLLIN)) {
        if (ready == 0)
          errno = ETIMEDOUT;
        else if (ready > 0)
          errno = ECONNRESET;
        return false;
      }
      break;
    }
  }
  ssize_t received;
  do {
    received = recv(fd, &message, sizeof(message), MSG_TRUNC);
  } while (received < 0 && errno == EINTR);
  if (received == sizeof(message))
    return true;
  if (received >= 0)
    errno = received == 0 ? EPIPE : EPROTO;
  return false;
}

static sockaddr_un service_address(const std::string &name) {
  sockaddr_un address = {};
  address.sun_family = AF_UNIX;
  Assert(name.size() < sizeof(address.sun_path) - 1, "MMA service socket name is too long");
  memcpy(address.sun_path + 1, name.data(), name.size());
  return address;
}

static bool send_storage(int socket, int storage) {
  char byte = 0;
  iovec data = {&byte, sizeof(byte)};
  alignas(cmsghdr) char control[CMSG_SPACE(sizeof(storage))] = {};
  msghdr message = {};
  message.msg_iov = &data;
  message.msg_iovlen = 1;
  message.msg_control = control;
  message.msg_controllen = sizeof(control);
  auto *header = CMSG_FIRSTHDR(&message);
  header->cmsg_level = SOL_SOCKET;
  header->cmsg_type = SCM_RIGHTS;
  header->cmsg_len = CMSG_LEN(sizeof(storage));
  memcpy(CMSG_DATA(header), &storage, sizeof(storage));
  ssize_t sent;
  do {
    sent = sendmsg(socket, &message, MSG_NOSIGNAL);
  } while (sent < 0 && errno == EINTR);
  return sent == sizeof(byte);
}

static int receive_storage(int socket) {
  char byte;
  iovec data = {&byte, sizeof(byte)};
  alignas(cmsghdr) char control[CMSG_SPACE(8 * sizeof(int))] = {};
  msghdr message = {};
  message.msg_iov = &data;
  message.msg_iovlen = 1;
  message.msg_control = control;
  message.msg_controllen = sizeof(control);
  ssize_t received;
  do {
    received = recvmsg(socket, &message, MSG_CMSG_CLOEXEC | MSG_TRUNC);
  } while (received < 0 && errno == EINTR);
  int storage = -1;
  bool valid = received == sizeof(byte) && !(message.msg_flags & (MSG_CTRUNC | MSG_TRUNC));
  for (auto *header = CMSG_FIRSTHDR(&message); header; header = CMSG_NXTHDR(&message, header)) {
    if (header->cmsg_level != SOL_SOCKET || header->cmsg_type != SCM_RIGHTS || header->cmsg_len < CMSG_LEN(0)) {
      valid = false;
      continue;
    }
    const size_t bytes = header->cmsg_len - CMSG_LEN(0);
    valid = valid && bytes == sizeof(int);
    for (size_t offset = 0; offset + sizeof(int) <= bytes; offset += sizeof(int)) {
      int fd;
      memcpy(&fd, CMSG_DATA(header) + offset, sizeof(fd));
      if (storage < 0)
        storage = fd;
      else {
        close(fd);
        valid = false;
      }
    }
  }
  if (!valid && storage >= 0) {
    close(storage);
    storage = -1;
  }
  return storage;
}

struct MappedClient {
  int socket;
  pid_t pid;
  SharedBatch *batch = nullptr;
  uint64_t sequence = 0;
  bool registered = false;
  int lease = -1;
  std::chrono::steady_clock::time_point accepted = std::chrono::steady_clock::now();

  MappedClient(int socket, pid_t pid) : socket(socket), pid(pid) {}
  ~MappedClient() {
    if (batch)
      munmap(batch, sizeof(SharedBatch));
    if (lease >= 0)
      close(lease);
    close(socket);
  }

  bool attach() {
    const int storage = receive_storage(socket);
    if (storage < 0)
      return false;
    // A separate open description lets the client wait until MMA has stopped
    // using its storage after a lost reply or a disconnected session.
    const std::string path = "/proc/self/fd/" + std::to_string(storage);
    lease = open(path.c_str(), O_RDWR | O_CLOEXEC);
    struct stat info;
    void *mapping = MAP_FAILED;
    if (lease >= 0 && !flock(lease, LOCK_EX | LOCK_NB) && !fstat(storage, &info) && info.st_size == sizeof(SharedBatch))
      mapping = mmap(nullptr, sizeof(SharedBatch), PROT_READ | PROT_WRITE, MAP_SHARED, storage, 0);
    close(storage);
    if (mapping == MAP_FAILED)
      return false;
    batch = static_cast<SharedBatch *>(mapping);
    if (batch->owner != pid)
      return false;
    registered = true;
    return true;
  }
};

} // namespace mma_process

#endif
#endif
