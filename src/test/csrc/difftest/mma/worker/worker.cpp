/***************************************************************************************
* Copyright (c) 2026 Beijing Institute of Open Source Chip (BOSC)
*
* DiffTest is licensed under Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
***************************************************************************************/

#include "mma/worker/worker.h"

#ifdef CONFIG_DIFFTEST_AMUCTRLEVENT
#include "mma/backend/mma_backend_cpu.h"
#include "mma/backend/mma_backend_cuda.h"
#include "mma/backend/mma_backend_ipc.h"
#include "mma/mma_verifier.h"
#include <atomic>
#include <signal.h>
#include <sys/prctl.h>

using namespace mma_process;

namespace {
struct Client : MappedClient {
  MmaEngine &engine;
  std::atomic<bool> active{false};
  bool prepared = false;
  std::vector<MmaVerificationBuffer> buffers;
  std::vector<MmaVerificationBuffer *> items;

  Client(int socket, pid_t pid, MmaEngine &engine) : MappedClient(socket, pid), engine(engine) {}
  ~Client() {
    if (prepared)
      engine.unregister_storage(batch);
  }
  bool attach() {
    if (!MappedClient::attach())
      return false;
    uint8_t *operands[] = {operand_storage(batch, 0), operand_storage(batch, 1), operand_storage(batch, 2)};
    prepared = engine.register_storage(batch, sizeof(SharedBatch), operands);
    return prepared;
  }
  void compute(size_t count, const MmaEngine::Completion &completed) {
    buffers.resize(count);
    items.resize(count);
    for (size_t i = 0; i < count; ++i) {
      auto &buffer = buffers[i];
      buffer.amu_event = batch->events[i];
      buffer.src1 = reinterpret_cast<uint8_t *>(batch->src1[batch->slots[i][0]]);
      buffer.src2 = reinterpret_cast<uint8_t *>(batch->src2[batch->slots[i][1]]);
      buffer.src3 = reinterpret_cast<uint8_t *>(batch->src3[batch->slots[i][2]]);
      auto *dut = reinterpret_cast<uint8_t *>(batch->dut_results[batch->slots[i][3]]);
      // Empty-owner aliases borrow results while this client retains the mapping.
      MatrixResultDeleter deleter;
      deleter.shared_result = std::shared_ptr<uint8_t>(std::shared_ptr<uint8_t>(), dut);
      buffer.dut_result = MatrixResult(reinterpret_cast<uint64_t *>(dut), std::move(deleter));
      items[i] = &buffer;
    }
    uint8_t *operands[] = {operand_storage(batch, 0), operand_storage(batch, 1), operand_storage(batch, 2)};
    engine.compute(items, batch->passed, operands, completed);
  }
};
} // namespace

int mma_worker_main(unsigned threads, pid_t owner) {
  try {
    DIR *directory = opendir("/proc/self/fd");
    if (!directory)
      throw std::runtime_error("Cannot enumerate MMA worker file descriptors");
    while (auto *entry = readdir(directory)) {
      char *end;
      const long fd = strtol(entry->d_name, &end, 10);
      if (!*end && fd > kWorkerControl && fd != dirfd(directory))
        close(static_cast<int>(fd));
    }
    closedir(directory);
    if (!threads || prctl(PR_SET_PDEATHSIG, SIGKILL) || getppid() != owner)
      throw std::runtime_error("MMA service owner disappeared");
#ifdef CONFIG_DIFFTEST_MMA_CUDA
    std::unique_ptr<MmaEngine> engine(new CudaMmaEngine());
#else
    std::unique_ptr<MmaEngine> engine(new CpuMmaEngine(threads));
#endif
    std::vector<std::shared_ptr<Client>> clients;
    // Release compute resources before unpinning clients, including error exits.
    struct Cleanup {
      MmaEngine &engine;
      ~Cleanup() {
        engine.shutdown();
      }
    } cleanup{*engine};
    if (!send_message(kWorkerControl, Message{}))
      return 1;
    while (true) {
      std::vector<pollfd> events = {{kWorkerSocket, POLLIN, 0}, {kWorkerControl, POLLIN, 0}};
      for (const auto &client: clients)
        events.push_back({client->socket, POLLIN, 0});
      int ready;
      do {
        ready = poll(events.data(), events.size(), 1000);
      } while (ready < 0 && errno == EINTR);
      if (ready < 0)
        throw std::runtime_error("Cannot poll MMA clients");
      if (events[1].revents)
        return 0;
      if (events[0].revents & POLLIN) {
        const int socket = accept4(kWorkerSocket, nullptr, nullptr, SOCK_CLOEXEC);
        if (socket < 0)
          throw std::runtime_error("Cannot accept MMA client");
        ucred peer;
        socklen_t size = sizeof(peer);
        timeval timeout = {kReplyTimeoutMs / 1000, (kReplyTimeoutMs % 1000) * 1000};
        if (getsockopt(socket, SOL_SOCKET, SO_PEERCRED, &peer, &size) || peer.uid != getuid() ||
            setsockopt(socket, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout)))
          close(socket);
        else
          clients.emplace_back(std::make_shared<Client>(socket, peer.pid, *engine));
      }
      for (size_t index = events.size(); index-- > 2;) {
        const auto client = clients[index - 2];
        const auto flags = events[index].revents;
        const bool expired = !client->registered && std::chrono::steady_clock::now() - client->accepted >
                                                        std::chrono::milliseconds(kReplyTimeoutMs);
        if (!flags && !expired)
          continue;
        bool connected = !expired && (flags & POLLIN) && !(flags & (POLLHUP | POLLERR | POLLNVAL));
        if (connected && !client->registered) {
          connected = client->attach() && send_message(client->socket, Message{});
        } else if (connected) {
          Message request;
          connected = receive_message(client->socket, request, false) && !client->active.load() && !request.status &&
                      request.sequence == client->sequence + 1 && request.count &&
                      request.count <= CONFIG_DIFF_MMA_BATCH_SIZE;
          for (size_t i = 0; connected && i < request.count; ++i)
            for (size_t operand = 0; operand < 4; ++operand)
              connected = connected && client->batch->slots[i][operand] < kSharedSlots;
          if (connected) {
            client->sequence = request.sequence;
            client->active = true;
            client->compute(request.count, [client, request](size_t end) {
              if (end == request.count)
                client->active.store(false, std::memory_order_release);
              return send_message(client->socket, Message{request.sequence, static_cast<uint32_t>(end), 0});
            });
          }
        }
        if (!connected)
          clients.erase(clients.begin() + index - 2);
      }
    }
  } catch (const std::exception &error) {
    fprintf(stderr, "MMA service: %s\n", error.what());
    return 1;
  }
}

#endif // CONFIG_DIFFTEST_AMUCTRLEVENT
