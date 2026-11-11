#include "checkpoint_writer.h"

#ifdef CONFIG_DIFFTEST_FORK_REF
#include <cerrno>
#include <climits>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <sched.h>
#include <sys/prctl.h>
#include <sys/syscall.h>
#include <sys/wait.h>
#include <unistd.h>

namespace {
struct Job {
  std::string base;
  uint64_t instr;
};
std::map<pid_t, Job> jobs;
// Maximum registered writers awaiting reaping (FPGA_CPT_WRITERS).
unsigned capacity = 16;
uint64_t requested = 0, reaped = 0, errors = 0;
bool failed = false;

bool configure_limit(const char *name, unsigned &limit) {
  const char *value = getenv(name);
  if (!value)
    return true;
  char *end;
  errno = 0;
  const auto parsed = strtoll(value, &end, 10);
  if (end == value || *end != '\0' || errno == ERANGE || parsed <= 0 ||
      static_cast<unsigned long long>(parsed) > UINT_MAX) {
    fprintf(stderr, "[ERROR] %s must be an integer in [1, %u]\n", name, UINT_MAX);
    return false;
  }
  limit = static_cast<unsigned>(parsed);
  return true;
}

// Poll registered writers. With wait_for_one, block on the first registered PID
// before polling the remaining jobs.
void reap(bool wait_for_one) {
  for (auto it = jobs.begin(); it != jobs.end();) {
    int status = 0;
    pid_t got;
    do {
      got = waitpid(it->first, &status, wait_for_one ? 0 : WNOHANG);
    } while (got < 0 && errno == EINTR);
    if (got == 0) {
      ++it;
      continue;
    }
    const auto &job = it->second;
    bool ok = got == it->first && WIFEXITED(status) && WEXITSTATUS(status) == 0;
    ++reaped;
    if (!ok) {
      fprintf(stderr, "Checkpoint writer failed: instr=%lu pid=%d status=%d\n", job.instr, int(it->first), status);
      failed = true;
      ++errors;
    }
    it = jobs.erase(it);
    wait_for_one = false;
  }
}
} // namespace

bool checkpoint_writer_configure() {
  return configure_limit("FPGA_CPT_WRITERS", capacity);
}

bool checkpoint_writer_submit(const std::string &base, uint64_t instr,
                              const std::function<void(const char *)> &serialize) {
  reap(false);
  while (!failed && jobs.size() >= capacity)
    reap(true);
  if (failed)
    return false;
  const pid_t pid = checkpoint_writer_fork(base, serialize);
  if (pid < 0) {
    failed = true;
    return false;
  }
  checkpoint_writer_adopt(pid, base, instr);
  return true;
}

bool checkpoint_writer_has_capacity() {
  return jobs.size() < capacity;
}

[[noreturn]] void checkpoint_writer_run(const std::string &base, const std::function<void(const char *)> &serialize) {
  // A writer must not run inherited FPGA/splitview signal cleanup handlers
  for (int sig: {SIGINT, SIGTERM, SIGABRT, SIGSEGV, SIGBUS})
    signal(sig, SIG_DFL);
  serialize(base.c_str());
  fflush(nullptr);
  _exit(0);
}

pid_t checkpoint_writer_fork(const std::string &base, const std::function<void(const char *)> &serialize,
                             bool sibling) {
  // Only single-threaded parsers/checkers call this, never the receiver
  const pid_t parent = sibling ? getppid() : getpid();
  fflush(nullptr);
  const pid_t pid = sibling ? syscall(SYS_clone, CLONE_PARENT | SIGCHLD, nullptr, nullptr, nullptr, 0) : fork();
  if (pid < 0) {
    perror("Checkpoint fork");
    return -1;
  }
  if (pid == 0) {
    if (prctl(PR_SET_PDEATHSIG, SIGKILL) != 0 || getppid() != parent)
      _exit(EXIT_FAILURE);
    checkpoint_writer_run(base, serialize);
  }
  return pid;
}

void checkpoint_writer_adopt(pid_t pid, const std::string &base, uint64_t instr) {
  jobs.emplace(pid, Job{base, instr});
  ++requested;
}

bool checkpoint_writer_poll() {
  reap(false);
  return !failed;
}

bool checkpoint_writer_drain() {
  while (!jobs.empty())
    reap(true);
  fprintf(stderr, "[CPT_DRAIN] requested=%lu reaped=%lu errors=%lu\n", requested, reaped, errors);
  return !failed;
}
#endif // CONFIG_DIFFTEST_FORK_REF
