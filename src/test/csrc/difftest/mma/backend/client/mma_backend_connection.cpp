/***************************************************************************************
* Copyright (c) 2020-2026 Institute of Computing Technology, Chinese Academy of Sciences
* Copyright (c) 2026 Beijing Institute of Open Source Chip (BOSC)
*
* DiffTest is licensed under Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
***************************************************************************************/

#include "mma/backend/client/mma_backend_connection.h"

#ifdef CONFIG_DIFFTEST_AMUCTRLEVENT
#include "mma/backend/mma_backend_ipc.h"
#include "mma/mma_verifier.h"
#include <linux/memfd.h>
#include <signal.h>
#include <spawn.h>
#include <sys/wait.h>
extern char **environ;

using namespace mma_ipc;

// The worker can outlive individual core and replay connections.
class MmaWorkerProcess {
public:
  MmaWorkerProcess() : owner(getpid()) {
    name = "difftest-mma." + std::to_string(owner) + "." + std::to_string(reinterpret_cast<uintptr_t>(this));
    if (!start()) {
      stop();
      throw std::runtime_error("Failed to start MMA service");
    }
  }
  ~MmaWorkerProcess() {
    stop();
  }
  const pid_t owner;
  std::string name;

private:
  int control = -1;
  pid_t worker = -1;

  bool start() {
    int listener = -1, child_listener = -1, child_control = -1;
    int sockets[2] = {-1, -1};
    posix_spawn_file_actions_t actions;
    if (posix_spawn_file_actions_init(&actions))
      return false;
    do {
      listener = socket(AF_UNIX, SOCK_SEQPACKET | SOCK_CLOEXEC, 0);
      const auto address = service_address(name);
      const auto bytes = offsetof(sockaddr_un, sun_path) + 1 + name.size();
      if (listener < 0 || bind(listener, reinterpret_cast<const sockaddr *>(&address), bytes) || listen(listener, 16) ||
          socketpair(AF_UNIX, SOCK_SEQPACKET | SOCK_CLOEXEC, 0, sockets))
        break;
      control = sockets[0];
      child_listener = fcntl(listener, F_DUPFD_CLOEXEC, 5);
      child_control = fcntl(sockets[1], F_DUPFD_CLOEXEC, 5);
      if (child_listener < 0 || child_control < 0)
        break;
      if (posix_spawn_file_actions_adddup2(&actions, child_listener, kWorkerSocket) ||
          posix_spawn_file_actions_adddup2(&actions, child_control, kWorkerControl))
        break;
      char executable[PATH_MAX];
      const ssize_t length = readlink("/proc/self/exe", executable, sizeof(executable));
      if (length < 0 || static_cast<size_t>(length) == sizeof(executable))
        break;
      std::string program(executable, length);
      program.erase(program.find_last_of('/') + 1);
      program += "difftest-mma-worker";
      std::string parent = std::to_string(owner);
      char *argv[] = {&program[0], &parent[0], nullptr};
      const int error = posix_spawn(&worker, program.c_str(), &actions, nullptr, argv, environ);
      if (error) {
        worker = -1;
        fprintf(stderr, "MmaBackend: cannot launch %s: %s\n", program.c_str(), strerror(error));
      }
    } while (false);
    posix_spawn_file_actions_destroy(&actions);
    for (int fd: {listener, child_listener, child_control, sockets[1]})
      if (fd >= 0)
        close(fd);
    Message ready = {};
    return worker > 0 && receive_message(control, ready, true) && !ready.sequence && !ready.count && !ready.status;
  }

  void stop() {
    if (control >= 0) {
      if (owner == getpid())
        shutdown(control, SHUT_RDWR);
      close(control);
      control = -1;
    }
    if (owner != getpid() || worker < 0)
      return;
    for (int attempt = 0; attempt < 250; ++attempt) {
      const pid_t result = waitpid(worker, nullptr, WNOHANG);
      if (result == worker || (result < 0 && errno == ECHILD))
        return;
      usleep(20000);
    }
    kill(worker, SIGKILL);
    while (waitpid(worker, nullptr, 0) < 0 && errno == EINTR) {}
  }
};

MmaBackendConnection::MmaBackendConnection(MmaHostBuffer (&host)[3]) : host(host), client_pid(getpid()) {
  try {
    worker_process = std::make_shared<MmaWorkerProcess>();
    prepare();
  } catch (...) {
    release();
    throw;
  }
}

MmaBackendConnection::~MmaBackendConnection() {
  release();
}

void MmaBackendConnection::disconnect() {
  if (socket_fd >= 0) {
    if (client_pid == getpid())
      shutdown(socket_fd, SHUT_RDWR);
    close(socket_fd);
  }
  socket_fd = -1;
  sequence = 0;
  if (client_pid == getpid() && storage_fd >= 0) {
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(kReplyTimeoutMs);
    while (flock(storage_fd, LOCK_EX | LOCK_NB)) {
      if (errno == EINTR)
        continue;
      if (errno != EWOULDBLOCK || std::chrono::steady_clock::now() >= deadline) {
        fprintf(stderr, "MmaBackend: service did not release client storage\n");
        _exit(EXIT_FAILURE);
      }
      usleep(1000);
    }
    flock(storage_fd, LOCK_UN);
  }
}

void MmaBackendConnection::release() {
  disconnect();
  dut_host.reset();
  for (auto &pool: host)
    pool.release();
  if (shared && !shared_storage)
    munmap(shared, sizeof(SharedBatch));
  shared_storage.reset();
  if (storage_fd >= 0)
    close(storage_fd);
  storage_fd = -1;
  shared = nullptr;
}

void MmaBackendConnection::prepare() {
  if (client_pid != getpid()) {
    for (auto &pool: host)
      Assert(pool.all_recycled(), "MMA operands must be released before fork");
    release();
    client_pid = getpid();
  }
  if (socket_fd >= 0)
    return;
  if (!shared) {
    storage_fd = static_cast<int>(syscall(SYS_memfd_create, "difftest-mma", MFD_CLOEXEC));
    if (storage_fd < 0 || ftruncate(storage_fd, sizeof(SharedBatch)))
      throw std::runtime_error("Cannot allocate MMA shared storage");
    shared = mmap(nullptr, sizeof(SharedBatch), PROT_READ | PROT_WRITE, MAP_SHARED, storage_fd, 0);
    if (shared == MAP_FAILED) {
      shared = nullptr;
      throw std::runtime_error("Cannot map MMA shared storage");
    }
    shared_storage = std::make_shared<SharedStorage>(shared);
    for (size_t i = 0; i < 3; ++i)
      host[i].bind(operand_storage(static_cast<SharedBatch *>(shared), i), operand_bytes(i), kPoolSlots);
    dut_host = std::make_shared<MmaHostBuffer>();
    dut_host->bind(reinterpret_cast<uint8_t *>(static_cast<SharedBatch *>(shared)->dut_results),
                   CONFIG_DIFF_AMU_C_REG_SIZE_BYTES, kPoolSlots);
  }
  static_cast<SharedBatch *>(shared)->owner = client_pid;
  const auto address = service_address(worker_process->name);
  const auto bytes = offsetof(sockaddr_un, sun_path) + 1 + worker_process->name.size();
  socket_fd = socket(AF_UNIX, SOCK_SEQPACKET | SOCK_CLOEXEC, 0);
  timeval timeout = {kReplyTimeoutMs / 1000, (kReplyTimeoutMs % 1000) * 1000};
  Message ready = {};
  if (socket_fd < 0 || setsockopt(socket_fd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout)) ||
      connect(socket_fd, reinterpret_cast<const sockaddr *>(&address), bytes) || !send_storage(socket_fd, storage_fd) ||
      !receive_message(socket_fd, ready, true) || ready.sequence || ready.count || ready.status) {
    disconnect();
    throw std::runtime_error("Cannot connect MMA service");
  }
}

MatrixResult MmaBackendConnection::allocate_result() {
  auto *data = dut_host->acquire(CONFIG_DIFF_AMU_C_REG_SIZE_BYTES);
  if (dut_host->slot(data) == MmaHostBuffer::invalid_slot) {
    dut_host->recycle(data);
    return MatrixResult();
  }
  const auto pool = dut_host;
  const auto storage = shared_storage;
  MatrixResultDeleter deleter;
  deleter.shared_result = std::shared_ptr<uint8_t>(data, [pool, storage](uint8_t *p) { pool->recycle(p); });
  return MatrixResult(reinterpret_cast<uint64_t *>(data), std::move(deleter));
}

void MmaBackendConnection::prepare_result(MmaVerificationBuffer *buffer) {
  if (!buffer->amu_event.mtilem || !buffer->amu_event.mtilen ||
      dut_host->slot(reinterpret_cast<uint8_t *>(buffer->dut_result.get())) != MmaHostBuffer::invalid_slot)
    return;
  if (!buffer->dut_snapshot)
    buffer->dut_snapshot = dut_host->acquire(CONFIG_DIFF_AMU_C_REG_SIZE_BYTES);
  memcpy(buffer->dut_snapshot, buffer->dut_result.get(), CONFIG_DIFF_AMU_C_REG_SIZE_BYTES);
}

void MmaBackendConnection::release_result(MmaVerificationBuffer *buffer) {
  if (buffer->dut_snapshot) {
    dut_host->recycle(buffer->dut_snapshot);
    buffer->dut_snapshot = nullptr;
  }
}

void MmaBackendConnection::verify(const std::vector<MmaVerificationBuffer *> &buffers, std::vector<uint8_t> &passed) {
  passed.assign(buffers.size(), 0);
  if (buffers.empty() || buffers.size() > CONFIG_DIFF_MMA_BATCH_SIZE || client_pid != getpid() || socket_fd < 0)
    return;
  auto *batch = static_cast<SharedBatch *>(shared);
  for (size_t i = 0; i < buffers.size(); ++i) {
    const auto &buffer = *buffers[i];
    batch->events[i] = buffer.amu_event;
    const bool has_output = buffer.amu_event.mtilem && buffer.amu_event.mtilen;
    const uint8_t *sources[] = {buffer.src1, buffer.src2, buffer.src3};
    for (size_t operand = 0; operand < 3; ++operand) {
      size_t slot = host[operand].slot(sources[operand]);
      if (slot == MmaHostBuffer::invalid_slot) {
        slot = kPoolSlots + i;
        if (has_output)
          memcpy(operand_storage(batch, operand) + slot * operand_bytes(operand), sources[operand],
                 operand_bytes(operand));
      }
      batch->slots[i][operand] = slot;
    }
    const auto *dut =
        buffer.dut_snapshot ? buffer.dut_snapshot : reinterpret_cast<const uint8_t *>(buffer.dut_result.get());
    size_t slot = dut_host->slot(dut);
    if (slot == MmaHostBuffer::invalid_slot) {
      slot = kPoolSlots + i;
      if (has_output)
        memcpy(batch->dut_results[slot], dut, CONFIG_DIFF_AMU_C_REG_SIZE_BYTES);
    }
    batch->slots[i][3] = slot;
  }
  const Message request = {++sequence, static_cast<uint32_t>(buffers.size()), 0};
  Message reply = {};
  if (!send_message(socket_fd, request)) {
    disconnect();
    return;
  }
  size_t completed = 0;
  while (completed < buffers.size()) {
    if (!receive_message(socket_fd, reply, true) || reply.sequence != request.sequence || reply.count <= completed ||
        reply.count > request.count || reply.status) {
      disconnect();
      return;
    }
    for (size_t i = completed; i < reply.count; ++i) {
      passed[i] = batch->passed[i];
      const auto &event = batch->events[i];
      auto *result = batch->src3[batch->slots[i][2]];
      if (!passed[i] && event.mtilem && event.mtilen && buffers[i]->src3 != reinterpret_cast<uint8_t *>(result))
        memcpy(buffers[i]->src3, result, CONFIG_DIFF_AMU_C_REG_SIZE_BYTES);
    }
    completed = reply.count;
  }
}

#endif
