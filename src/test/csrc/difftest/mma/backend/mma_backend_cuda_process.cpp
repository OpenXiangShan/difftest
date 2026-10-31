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

#include "mma/backend/mma_backend_cuda_process.h"

#if defined(CONFIG_DIFFTEST_AMUCTRLEVENT) && defined(CONFIG_DIFFTEST_MMA_CUDA)
#include "mma/backend/mma_backend_cuda_impl.h"
#include "mma/mma_verifier.h"
#include <algorithm>
#include <cerrno>
#include <chrono>
#include <climits>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cuda_runtime_api.h>
#include <dirent.h>
#include <fcntl.h>
#include <linux/memfd.h>
#include <poll.h>
#include <signal.h>
#include <spawn.h>
#include <stdexcept>
#include <string>
#include <sys/file.h>
#include <sys/mman.h>
#include <sys/prctl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <unistd.h>

extern char **environ;

namespace {
const int kWorkerSocket = 3;
const int kWorkerStorage = 4;
const int kReplyTimeoutMs = 60000;
const size_t kPoolSlots = static_cast<size_t>(CONFIG_DIFF_MMA_BATCH_SIZE) * CONFIG_DIFF_MMA_BACKPRESSURE_FACTOR + 2;
// Ordinary-array fallback requests use a separate batch beyond the pool.
const size_t kSharedSlots = kPoolSlots + CONFIG_DIFF_MMA_BATCH_SIZE;

struct Message {
  uint64_t sequence;
  uint32_t count;
  int32_t status;
};

// Only temporary operand snapshots are shared. ROB results and reference architectural state
// remain ordinary COW memory in the simulator; DUT results are compared there.
struct SharedBatch {
  pid_t owner;
  DifftestAmuCtrlEvent events[CONFIG_DIFF_MMA_BATCH_SIZE];
  uint8_t passed[CONFIG_DIFF_MMA_BATCH_SIZE];
  uint32_t slots[CONFIG_DIFF_MMA_BATCH_SIZE][3];
  uint64_t src1[kSharedSlots][CONFIG_DIFF_AMU_AB_REG_SIZE_BYTES / sizeof(uint64_t)];
  uint64_t src2[kSharedSlots][CONFIG_DIFF_AMU_AB_REG_SIZE_BYTES / sizeof(uint64_t)];
  uint64_t src3[kSharedSlots][CONFIG_DIFF_AMU_C_REG_SIZE_BYTES / sizeof(uint64_t)];
};

uint8_t *operand_storage(SharedBatch *batch, size_t operand) {
  return reinterpret_cast<uint8_t *>(operand == 0 ? batch->src1[0] : operand == 1 ? batch->src2[0] : batch->src3[0]);
}

size_t operand_bytes(size_t operand) {
  return operand == 2 ? CONFIG_DIFF_AMU_C_REG_SIZE_BYTES : CONFIG_DIFF_AMU_AB_REG_SIZE_BYTES;
}

bool send_message(int fd, const Message &message) {
  ssize_t sent;
  do {
    sent = send(fd, &message, sizeof(message), MSG_NOSIGNAL);
  } while (sent < 0 && errno == EINTR);
  return sent == sizeof(message);
}

bool receive_message(int fd, Message &message, bool timeout) {
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

sockaddr_un service_address(const std::string &name) {
  sockaddr_un address = {};
  address.sun_family = AF_UNIX;
  Assert(name.size() < sizeof(address.sun_path) - 1, "CUDA service socket name is too long");
  memcpy(address.sun_path + 1, name.data(), name.size());
  return address;
}

bool send_storage(int socket, int storage) {
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

int receive_storage(int socket) {
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

struct Client {
  int socket;
  pid_t pid;
  SharedBatch *batch = nullptr;
  uint64_t sequence = 0;
  bool registered = false;
  bool root = false;
  int lease = -1;
  std::chrono::steady_clock::time_point accepted = std::chrono::steady_clock::now();

  Client(int socket, pid_t pid) : socket(socket), pid(pid) {}
  ~Client() {
    if (registered)
      cudaHostUnregister(batch);
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
    // A separate open description lets the client wait until CUDA has stopped
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
    const auto error = cudaHostRegister(batch, sizeof(SharedBatch), cudaHostRegisterDefault);
    if (error != cudaSuccess) {
      fprintf(stderr, "CudaMmaBackend service: register client failed: %s\n", cudaGetErrorString(error));
      return false;
    }
    registered = true;
    return true;
  }
};

void close_other_fds() {
  DIR *directory = opendir("/proc/self/fd");
  if (!directory)
    throw std::runtime_error("Cannot enumerate CUDA worker file descriptors");
  while (auto *entry = readdir(directory)) {
    char *end;
    const long fd = strtol(entry->d_name, &end, 10);
    if (!*end && fd > kWorkerStorage && fd != dirfd(directory))
      close(static_cast<int>(fd));
  }
  closedir(directory);
}

} // namespace

CudaMmaProcess::CudaMmaProcess(MmaHostBuffer (&host)[3]) : host(host) {
  try {
    if (!start())
      throw std::runtime_error("Failed to start CUDA MMA worker");
  } catch (...) {
    release();
    throw;
  }
}

CudaMmaProcess::~CudaMmaProcess() {
  release();
}

void CudaMmaProcess::detach_inherited() {
  // close(), not shutdown(): the parent's endpoint and worker remain live.
  if (socket_fd >= 0)
    close(socket_fd);
  for (auto &pool: host) {
    Assert(pool.all_recycled(), "MMA operand snapshots must be released before fork");
    pool.release();
  }
  if (shared)
    munmap(shared, sizeof(SharedBatch));
  if (storage_fd >= 0)
    close(storage_fd);
  storage_fd = -1;
  socket_fd = -1;
  client_pid = -1;
  shared = nullptr;
  sequence = 0;
}

void CudaMmaProcess::release() {
  disconnect();
  for (auto &pool: host)
    pool.release();
  if (shared)
    munmap(shared, sizeof(SharedBatch));
  if (storage_fd >= 0)
    close(storage_fd);
  shared = nullptr;
  storage_fd = -1;
}

void CudaMmaProcess::disconnect() {
  const bool owner = owner_pid == getpid();
  const pid_t worker = owner ? worker_pid : -1;
  if (socket_fd >= 0) {
    // Only this session's client may shut it down. A snapshot that never
    // reconnects merely closes its inherited reference.
    if (client_pid == getpid())
      shutdown(socket_fd, SHUT_RDWR);
    close(socket_fd);
  }
  socket_fd = -1;
  worker_pid = -1;
  sequence = 0;
  if (worker <= 0) {
    if (client_pid == getpid() && storage_fd >= 0) {
      const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(kReplyTimeoutMs);
      while (flock(storage_fd, LOCK_EX | LOCK_NB)) {
        if (errno == EINTR)
          continue;
        if (errno != EWOULDBLOCK || std::chrono::steady_clock::now() >= deadline) {
          fprintf(stderr, "CudaMmaBackend: service did not release client storage\n");
          // Continuing could recycle a slot while the service still writes C.
          _exit(EXIT_FAILURE);
        }
        usleep(1000);
      }
      flock(storage_fd, LOCK_UN);
    }
    return;
  }
  // The root connection owns service lifetime; replay clients cannot reap it.
  for (int attempt = 0; attempt < 250; ++attempt) {
    const pid_t result = waitpid(worker, nullptr, WNOHANG);
    if (result == worker || (result < 0 && errno == ECHILD))
      return;
    usleep(20000);
  }
  kill(worker, SIGKILL);
  while (waitpid(worker, nullptr, 0) < 0 && errno == EINTR) {}
}

bool CudaMmaProcess::start() {
  const bool owner = owner_pid < 0 || owner_pid == getpid();
  if (owner)
    owner_pid = getpid();
  client_pid = getpid();
  int listener = -1;
  int child_socket = -1, child_storage = -1;
  sequence = 0;
  posix_spawn_file_actions_t actions;
  bool actions_ready = false;
  bool success = false;
  do {
    if (!shared) {
      storage_fd = static_cast<int>(syscall(SYS_memfd_create, "difftest-mma", MFD_CLOEXEC));
      if (storage_fd < 0 || ftruncate(storage_fd, sizeof(SharedBatch)) < 0)
        break;
      shared = mmap(nullptr, sizeof(SharedBatch), PROT_READ | PROT_WRITE, MAP_SHARED, storage_fd, 0);
      if (shared == MAP_FAILED) {
        shared = nullptr;
        break;
      }
      for (size_t operand = 0; operand < 3; ++operand)
        host[operand].bind(operand_storage(static_cast<SharedBatch *>(shared), operand), operand_bytes(operand),
                           kPoolSlots);
    }
    static_cast<SharedBatch *>(shared)->owner = client_pid;
    if (!owner) {
      success = true;
      break;
    }
    if (service_name.empty())
      service_name =
          "difftest-mma." + std::to_string(owner_pid) + "." + std::to_string(reinterpret_cast<uintptr_t>(this));
    listener = socket(AF_UNIX, SOCK_SEQPACKET | SOCK_CLOEXEC, 0);
    if (listener < 0)
      break;
    const auto address = service_address(service_name);
    const auto address_bytes = offsetof(sockaddr_un, sun_path) + 1 + service_name.size();
    if (bind(listener, reinterpret_cast<const sockaddr *>(&address), address_bytes) || listen(listener, 16))
      break;
    // Use high duplicate sources, so dup2 to descriptors 3/4 cannot clobber
    // another source when the simulator has only a few open descriptors.
    child_socket = fcntl(listener, F_DUPFD_CLOEXEC, 5);
    child_storage = fcntl(storage_fd, F_DUPFD_CLOEXEC, 5);
    if (child_socket < 0 || child_storage < 0)
      break;
    int action_error = posix_spawn_file_actions_init(&actions);
    if (action_error) {
      errno = action_error;
      break;
    }
    actions_ready = true;
    action_error = posix_spawn_file_actions_adddup2(&actions, child_socket, kWorkerSocket);
    if (!action_error)
      action_error = posix_spawn_file_actions_adddup2(&actions, child_storage, kWorkerStorage);
    if (action_error) {
      errno = action_error;
      break;
    }
    char executable[PATH_MAX];
    const ssize_t length = readlink("/proc/self/exe", executable, sizeof(executable));
    if (length < 0)
      break;
    if (static_cast<size_t>(length) == sizeof(executable)) {
      errno = ENAMETOOLONG;
      break;
    }
    std::string program(executable, length);
    program.erase(program.find_last_of('/') + 1);
    program += "difftest-mma-worker";
    char *argv[] = {&program[0], nullptr};
    const int error = posix_spawn(&worker_pid, program.c_str(), &actions, nullptr, argv, environ);
    if (error) {
      fprintf(stderr, "CudaMmaBackend: cannot launch %s: %s\n", program.c_str(), strerror(error));
      worker_pid = -1;
      errno = error;
      break;
    }
    success = true;
  } while (false);
  if (actions_ready)
    posix_spawn_file_actions_destroy(&actions);
  for (int fd: {listener, child_socket, child_storage})
    if (fd >= 0)
      close(fd);
  Message ready = {};
  if (success) {
    socket_fd = socket(AF_UNIX, SOCK_SEQPACKET | SOCK_CLOEXEC, 0);
    const auto address = service_address(service_name);
    const auto address_bytes = offsetof(sockaddr_un, sun_path) + 1 + service_name.size();
    timeval timeout = {kReplyTimeoutMs / 1000, (kReplyTimeoutMs % 1000) * 1000};
    success = socket_fd >= 0 && !setsockopt(socket_fd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout)) &&
              !connect(socket_fd, reinterpret_cast<const sockaddr *>(&address), address_bytes) &&
              send_storage(socket_fd, storage_fd) && receive_message(socket_fd, ready, true);
    if (success && (ready.sequence || ready.count || ready.status)) {
      errno = EPROTO;
      success = false;
    }
  }
  if (!success) {
    fprintf(stderr, "CudaMmaBackend: worker startup failed: %s\n", strerror(errno));
    disconnect();
  }
  return success;
}

void CudaMmaProcess::prepare() {
  if (client_pid != getpid())
    detach_inherited();
  // Spawn from the producer: PR_SET_PDEATHSIG follows the spawning thread,
  // which must outlive stop()/restart() of the verifier worker.
  if (socket_fd < 0 && !start())
    throw std::runtime_error("Failed to restart CUDA MMA worker");
}

void CudaMmaProcess::verify(const std::vector<MmaVerificationBuffer *> &buffers, std::vector<uint8_t> &passed) {
  passed.assign(buffers.size(), 0);
  if (buffers.empty())
    return;
  if (buffers.size() > CONFIG_DIFF_MMA_BATCH_SIZE)
    return;
  if (client_pid != getpid() || socket_fd < 0)
    return;
  auto *batch = static_cast<SharedBatch *>(shared);
  for (size_t i = 0; i < buffers.size(); ++i) {
    const auto &buffer = *buffers[i];
    batch->events[i] = buffer.amu_event;
    const uint8_t *sources[] = {buffer.src1, buffer.src2, buffer.src3};
    for (size_t operand = 0; operand < 3; ++operand) {
      size_t slot = host[operand].slot(sources[operand]);
      if (slot == MmaHostBuffer::invalid_slot) {
        slot = kPoolSlots + i;
        if (buffer.amu_event.mtilem && buffer.amu_event.mtilen)
          memcpy(operand_storage(batch, operand) + slot * operand_bytes(operand), sources[operand],
                 operand_bytes(operand));
      }
      batch->slots[i][operand] = slot;
    }
  }
  Message request = {++sequence, static_cast<uint32_t>(buffers.size()), 0}, reply = {};
  if (!send_message(socket_fd, request)) {
    disconnect();
    return;
  }
  size_t completed = 0;
  while (completed < buffers.size()) {
    if (!receive_message(socket_fd, reply, true) || reply.sequence != request.sequence || reply.count <= completed ||
        reply.count > request.count || reply.status != 0) {
      fprintf(stderr, "CudaMmaBackend: worker failed while verifying batch %llu\n",
              static_cast<unsigned long long>(sequence));
      // Outstanding buffers still borrow this mapping, including the first error.
      disconnect();
      return;
    }
    for (size_t i = completed; i < reply.count; ++i) {
      const auto &event = batch->events[i];
      const auto *result = batch->src3[batch->slots[i][2]];
      passed[i] = batch->passed[i] &&
                  compare_mma_rows(buffers[i]->dut_result.get(), result, event.mtilem, sizeof(uint32_t) * event.mtilen);
      if (!passed[i] && event.mtilem && event.mtilen && buffers[i]->src3 != reinterpret_cast<const uint8_t *>(result))
        memcpy(buffers[i]->src3, result, sizeof(batch->src3[0]));
    }
    completed = reply.count;
  }
}

int cuda_mma_worker_main() {
  try {
    close_other_fds();
    pid_t owner;
    struct stat bootstrap;
    if (fstat(kWorkerStorage, &bootstrap) || bootstrap.st_size != sizeof(SharedBatch) ||
        pread(kWorkerStorage, &owner, sizeof(owner), 0) != sizeof(owner))
      throw std::runtime_error("Invalid CUDA service owner");
    close(kWorkerStorage);
    if (prctl(PR_SET_PDEATHSIG, SIGKILL) || getppid() != owner)
      throw std::runtime_error("CUDA service owner disappeared");
    const auto error = cudaSetDevice(0);
    if (error != cudaSuccess)
      throw std::runtime_error(cudaGetErrorString(error));
    MmaHostBuffer host[3];
    std::vector<std::unique_ptr<Client>> clients;
    std::unique_ptr<CudaMmaWorkspace> workspace;
    std::vector<CudaMmaBatchItem> items;
    items.reserve(CONFIG_DIFF_MMA_BATCH_SIZE);
    while (true) {
      std::vector<pollfd> events = {{kWorkerSocket, POLLIN, 0}};
      bool awaiting_client = false;
      for (const auto &client: clients) {
        events.push_back({client->socket, POLLIN, 0});
        awaiting_client = awaiting_client || !client->registered;
      }
      int ready;
      do {
        ready = poll(events.data(), events.size(), awaiting_client ? 1000 : -1);
      } while (ready < 0 && errno == EINTR);
      if (ready < 0)
        throw std::runtime_error("Cannot poll CUDA service clients");
      if (events[0].revents & POLLIN) {
        const int socket = accept4(kWorkerSocket, nullptr, nullptr, SOCK_CLOEXEC);
        if (socket < 0)
          throw std::runtime_error("Cannot accept CUDA service client");
        ucred peer;
        socklen_t length = sizeof(peer);
        timeval timeout = {kReplyTimeoutMs / 1000, (kReplyTimeoutMs % 1000) * 1000};
        if (getsockopt(socket, SOL_SOCKET, SO_PEERCRED, &peer, &length) || peer.uid != getuid() ||
            clients.size() >= 16 || setsockopt(socket, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout))) {
          close(socket);
        } else {
          clients.emplace_back(new Client(socket, peer.pid));
        }
      }
      // Reverse erasure leaves the poll indices of earlier clients unchanged.
      for (size_t index = events.size() - 1; index > 0; --index) {
        auto &client = *clients[index - 1];
        const bool expired = !client.registered && std::chrono::steady_clock::now() - client.accepted >
                                                       std::chrono::milliseconds(kReplyTimeoutMs);
        if (!events[index].revents && !expired)
          continue;
        bool connected =
            !expired && (events[index].revents & POLLIN) && !(events[index].revents & (POLLHUP | POLLERR | POLLNVAL));
        if (connected && !client.registered) {
          connected = client.attach();
          if (connected && client.pid == owner) {
            bool has_root = false;
            for (const auto &other: clients)
              has_root = has_root || other->root;
            client.root = !has_root;
          }
          if (connected && !workspace) {
            uint8_t *shared_host[] = {operand_storage(client.batch, 0), operand_storage(client.batch, 1),
                                      operand_storage(client.batch, 2)};
            workspace.reset(cuda_mma_backend_create(host, CONFIG_DIFF_MMA_BATCH_SIZE, CONFIG_DIFF_AMU_AB_REG_SIZE_BYTES,
                                                    CONFIG_DIFF_AMU_C_REG_SIZE_BYTES, shared_host));
            if (!workspace)
              throw std::runtime_error("Cannot allocate CUDA service workspace");
          }
          Message ready = {};
          connected = connected && send_message(client.socket, ready);
        } else if (connected) {
          Message request;
          connected = receive_message(client.socket, request, false);
          if (connected) {
            connected = !request.status && request.sequence == client.sequence + 1 && request.count &&
                        request.count <= CONFIG_DIFF_MMA_BATCH_SIZE;
            auto *batch = client.batch;
            items.clear();
            for (size_t i = 0; connected && i < request.count; ++i) {
              const auto &event = batch->events[i];
              for (size_t operand = 0; operand < 3; ++operand)
                if (batch->slots[i][operand] >= kSharedSlots)
                  connected = false;
              if (!connected)
                break;
              CudaMmaBatchItem item;
              item.tile_m = event.mtilem;
              item.tile_k = event.mtilek;
              item.tile_n = event.mtilen;
              item.sat = event.sat;
              item.src1 = reinterpret_cast<const uint8_t *>(batch->src1[batch->slots[i][0]]);
              item.src2 = reinterpret_cast<const uint8_t *>(batch->src2[batch->slots[i][1]]);
              item.src3 = reinterpret_cast<uint8_t *>(batch->src3[batch->slots[i][2]]);
              items.push_back(item);
            }
            if (connected) {
              client.sequence = request.sequence;
              for (size_t operand = 0; operand < 3; ++operand)
                workspace->operands[operand].shared_host = operand_storage(batch, operand);
              CudaMmaType type;
              const bool success = select_cuda_mma_type(batch->events[0], &type);
              if (success && (!cuda_mma_backend_enqueue(workspace.get(), type, items.data(), items.size()) ||
                              cudaGetLastError() != cudaSuccess))
                throw std::runtime_error("CUDA service computation failed");
              memset(batch->passed, success, request.count);
              // Publish completed prefixes while later ranges remain on the GPU.
              // Keep the batch and its slots leased until the final reply is consumed.
              const size_t completion_size = kMmaCompletionSize;
              for (size_t begin = 0; connected && begin < request.count; begin += completion_size) {
                const size_t end = std::min(begin + completion_size, size_t(request.count));
                if (success &&
                    !(request.count <= completion_size ? workspace->synchronize() : workspace->wait(begin, end)))
                  throw std::runtime_error("CUDA service computation failed");
                Message reply = {request.sequence, static_cast<uint32_t>(end), 0};
                connected = send_message(client.socket, reply);
              }
              if (!connected && !workspace->synchronize())
                throw std::runtime_error("CUDA service computation failed");
            }
          }
        }
        if (!connected) {
          const bool root = client.root;
          clients.erase(clients.begin() + index - 1);
          if (root) {
            close(kWorkerSocket);
            return 0;
          }
        }
      }
    }
  } catch (const std::exception &error) {
    fprintf(stderr, "CudaMmaBackend service: %s\n", error.what());
    close(kWorkerSocket);
    return 1;
  }
}
#endif
