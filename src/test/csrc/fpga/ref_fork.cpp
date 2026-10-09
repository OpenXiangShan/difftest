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
#include "ref_fork.h"
#include "difftest.h"
#include "shared_packet_pool.h"
#include <atomic>
#include <cerrno>
#include <chrono>
#include <csignal>
#include <new>
#include <sched.h>
#include <sys/prctl.h>
#include <sys/syscall.h>
#include <sys/wait.h>
#include <unistd.h>

#ifdef CONFIG_DIFFTEST_FORK_REF
namespace {
// Bound outstanding checkpoints, not the lifetime number of segments.
constexpr unsigned MAX_SEGMENTS = SharedPacketPool::MAX_READERS;
constexpr uint64_t OPEN_END = UINT64_MAX;
constexpr uint64_t TIMEOUT_NS = 300ULL * 1000000000;
enum class Role {
  SUPERVISOR,
  LEADER,
  CHECKER
};
enum class Status {
  EMPTY,
  RUNNING,
  MATCH,
  DIVERGED,
  FAILED,
  DISCARDED,
  PROMOTED
};
enum class Command {
  WAIT,
  DISCARD,
  PROMOTE
};
struct Segment {
  uint64_t generation, start_window, start_packet, start_ns;
  unsigned reader;
  std::atomic<pid_t> pid{0};
  std::atomic<uint64_t> end_window{OPEN_END};
  uint64_t end_instr, fast_end_ns;
  int fast_stamp, slow_stamp;
  DifftestStateHash fast_hash{}, slow_hash{};
  uint64_t checked_windows, child_start_ns, child_end_ns, end_packet;
  std::atomic<Status> status{Status::EMPTY};
  std::atomic<Command> command{Command::WAIT};
  bool reaped = false; // Written only by the supervisor.
};
struct Shared {
  std::atomic<uint64_t> generation{0}, head{0}, retired{0}, published_windows{0};
  std::atomic<bool> pause{false}, paused{false}, endpoint{false};
  std::atomic<int> complete{0}; // 0 running, 1 checked endpoint, 2 failure.
  Segment segments[MAX_SEGMENTS];
};
Shared *control = nullptr;
bool enabled = false;
Role role = Role::SUPERVISOR;
uint64_t interval_ns = 0, generation = 0, current_segment = OPEN_END;
uint64_t windows = 0, last_instr = 0, deadline_ns = 0, child_window = 0;
uint64_t trusted_segment = 0, trusted_windows = 0, checked_windows = 0;
uint64_t accepted_segments = 0, recoveries = 0, drain_start_ns = 0;
pid_t supervisor_pid = 0, leader_pid = 0;
unsigned leader_reader = 0;
bool leader_reaped = false, summary_printed = false;

uint64_t now_ns() {
  return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch())
      .count();
}
Segment &segment(uint64_t seq) {
  return control->segments[seq % MAX_SEGMENTS];
}
bool hashes_match(const DifftestStateHash &a, const DifftestStateHash &b) {
  return a.state_lo == b.state_lo && a.state_hi == b.state_hi && a.store_lo == b.store_lo && a.store_hi == b.store_hi &&
         a.store_count == b.store_count;
}
void fail() {
  g_shared_packet_pool->fail();
  control->complete.store(2, std::memory_order_release);
}
// Called only at parser boundaries, outside checkpoint construction/publication.
// The supervisor waits for this acknowledgement before discarding descendants.
void pause_leader() {
  if (role == Role::LEADER && control->pause.load(std::memory_order_acquire)) {
    control->paused.store(true, std::memory_order_release);
    while (true)
      usleep(1000); // Supervisor replaces and kills this leader.
  }
}
void discard_checker() {
  segment(current_segment).status.store(Status::DISCARDED, std::memory_order_release);
  g_shared_packet_pool->finish_reader();
  _exit(0);
}
void complete_checker(Difftest *self, bool checks_ok) {
  Segment &s = segment(current_segment);
  if (s.command.load(std::memory_order_acquire) == Command::DISCARD)
    discard_checker();
  s.checked_windows = child_window - s.start_window;
  s.end_packet = g_shared_packet_pool->cursor();
  s.child_end_ns = now_ns();
  self->proxy->sync();
  checks_ok =
      checks_ok && child_window == s.end_window.load(std::memory_order_acquire) && self->proxy->state_hash(s.slow_hash);
  s.slow_stamp = self->fork_commit_stamp();
  const bool match = checks_ok && hashes_match(s.fast_hash, s.slow_hash) && s.fast_stamp == s.slow_stamp;
  s.status.store(!checks_ok ? Status::FAILED : (match ? Status::MATCH : Status::DIVERGED), std::memory_order_release);
  if (!checks_ok) {
    // Its starting snapshot is authoritative only after preceding segments pass.
    g_shared_packet_pool->finish_reader();
    fflush(nullptr);
    _exit(2);
  }
  if (match) {
    g_shared_packet_pool->finish_reader();
    _exit(0);
  }
  // A locally valid slow endpoint remains alive and pins its boundary packet.
  // Only the supervisor, after trusting all predecessors, can promote it.
  while (true) {
    const Command cmd = s.command.load(std::memory_order_acquire);
    if (cmd == Command::DISCARD || g_shared_packet_pool->aborted())
      discard_checker();
    if (cmd == Command::PROMOTE) {
      windows = child_window;
      last_instr = s.end_instr;
      generation = control->generation.load(std::memory_order_acquire);
      current_segment = OPEN_END;
      role = Role::LEADER;
      if (!self->set_ref_mode(REF_EXEC_FAST)) {
        fail();
        _exit(2);
      }
      deadline_ns = now_ns() + interval_ns;
      s.status.store(Status::PROMOTED, std::memory_order_release);
      printf("RefLeaderPromoted PID=%d Generation=%lu Window=%lu Packet=%lu\n", int(getpid()),
             (unsigned long)generation, (unsigned long)windows, (unsigned long)g_shared_packet_pool->cursor());
      fflush(stdout);
      return;
    }
    usleep(50);
  }
}
void close_segment(Difftest *self) {
  if (current_segment == OPEN_END)
    return;
  Segment &s = segment(current_segment);
  if (s.end_window.load(std::memory_order_acquire) != OPEN_END)
    return;
  self->proxy->sync();
  if (!self->proxy->state_hash(s.fast_hash))
    fail();
  s.fast_stamp = self->fork_commit_stamp();
  s.end_instr = last_instr;
  s.fast_end_ns = now_ns();
  s.end_window.store(windows, std::memory_order_release);
  printf("RefForkClose Generation=%lu Segment=%lu StartWindow=%lu EndWindow=%lu EndInstr=%lu FastSpanMs=%.3f\n",
         (unsigned long)generation, (unsigned long)current_segment, (unsigned long)s.start_window,
         (unsigned long)windows, (unsigned long)last_instr, double(s.fast_end_ns - s.start_ns) / 1e6);
  fflush(stdout);
}
bool reap(pid_t pid, bool &done, bool expected_kill = false) {
  if (done || pid <= 0)
    return true;
  int status = 0;
  const pid_t got = waitpid(pid, &status, WNOHANG);
  if (got == 0 || (got < 0 && errno == EINTR))
    return true;
  done = true;
  return got == pid && (expected_kill || (WIFEXITED(status) && WEXITSTATUS(status) == 0));
}
void poll_checkers() {
  const uint64_t end = control->head.load(std::memory_order_acquire);
  for (uint64_t i = control->retired.load(std::memory_order_relaxed); i < end; ++i) {
    Segment &s = segment(i);
    const pid_t pid = s.pid.load(std::memory_order_acquire);
    if (pid <= 0 || s.reaped || s.status.load(std::memory_order_acquire) == Status::PROMOTED)
      continue;
    const bool ok = reap(pid, s.reaped, s.command.load(std::memory_order_acquire) == Command::DISCARD);
    if (s.reaped) {
      g_shared_packet_pool->retire_reader(s.reader);
      const auto status = s.status.load(std::memory_order_acquire);
      if (!ok || (status != Status::MATCH && status != Status::DISCARDED))
        s.status.store(Status::FAILED, std::memory_order_release);
    }
  }
}
void retire_prefix() {
  uint64_t retired = control->retired.load(std::memory_order_relaxed);
  while (retired < trusted_segment) {
    Segment &s = segment(retired);
    if (!s.reaped && s.status.load(std::memory_order_acquire) != Status::PROMOTED)
      break;
    ++retired;
  }
  control->retired.store(retired, std::memory_order_release);
}
void accept(uint64_t seq, bool recovered) {
  Segment &s = segment(seq);
  checked_windows += s.checked_windows;
  trusted_windows = s.end_window.load(std::memory_order_acquire);
  ++trusted_segment;
  ++accepted_segments;
  printf(
      "RefForkResult Generation=%lu Segment=%lu %s StartWindow=%lu EndWindow=%lu Checked=%lu "
      "FastMs=%.3f SlowMs=%.3f LagMs=%.3f StartPacket=%lu EndPacket=%lu Stores=%lu\n",
      (unsigned long)s.generation, (unsigned long)seq, recovered ? "RECOVERED" : "MATCH", (unsigned long)s.start_window,
      (unsigned long)trusted_windows, (unsigned long)s.checked_windows, double(s.fast_end_ns - s.start_ns) / 1e6,
      double(s.child_end_ns - s.child_start_ns) / 1e6,
      s.child_end_ns > s.fast_end_ns ? double(s.child_end_ns - s.fast_end_ns) / 1e6 : 0.0,
      (unsigned long)s.start_packet, (unsigned long)s.end_packet, (unsigned long)s.slow_hash.store_count);
  fflush(stdout);
}
bool recover(uint64_t seq) {
  Segment &candidate = segment(seq);
  control->pause.store(true, std::memory_order_release);
  const uint64_t begin = now_ns();
  while (!control->paused.load(std::memory_order_acquire)) {
    if (g_shared_packet_pool->aborted() || now_ns() - begin >= TIMEOUT_NS)
      return false;
    if (!reap(leader_pid, leader_reaped) || leader_reaped)
      return false;
    usleep(1000);
  }
  // Publication is stopped, so no unseen fork or reader can appear now.
  const uint64_t end = control->head.load(std::memory_order_acquire);
  kill(leader_pid, SIGKILL);
  for (uint64_t i = seq + 1; i < end; ++i) {
    Segment &s = segment(i);
    s.command.store(Command::DISCARD, std::memory_order_release);
    const pid_t pid = s.pid.load(std::memory_order_acquire);
    if (!s.reaped && pid > 0)
      kill(pid, SIGKILL);
  }
  while (true) {
    if (!reap(leader_pid, leader_reaped, true))
      return false;
    bool done = leader_reaped;
    for (uint64_t i = seq + 1; i < end; ++i) {
      Segment &s = segment(i);
      if (!reap(s.pid.load(std::memory_order_acquire), s.reaped, true))
        return false;
      done &= s.reaped;
    }
    if (done)
      break;
    if (now_ns() - begin >= TIMEOUT_NS)
      return false;
    usleep(1000);
  }
  g_shared_packet_pool->retire_reader(leader_reader);
  for (uint64_t i = seq + 1; i < end; ++i)
    g_shared_packet_pool->retire_reader(segment(i).reader);
  leader_pid = candidate.pid.load(std::memory_order_acquire);
  leader_reader = candidate.reader;
  leader_reaped = false;
  control->head.store(seq + 1, std::memory_order_release);
  control->published_windows.store(candidate.end_window.load(std::memory_order_acquire), std::memory_order_release);
  control->endpoint.store(false, std::memory_order_release);
  drain_start_ns = 0;
  control->generation.fetch_add(1, std::memory_order_release);
  control->pause.store(false, std::memory_order_release);
  control->paused.store(false, std::memory_order_release);
  ++recoveries;
  printf("RefLeaderRecovery PID=%d Generation=%lu DiscardedSegments=%lu\n", int(leader_pid),
         (unsigned long)control->generation.load(), (unsigned long)(end - seq - 1));
  fflush(stdout);
  candidate.command.store(Command::PROMOTE, std::memory_order_release);
  while (candidate.status.load(std::memory_order_acquire) != Status::PROMOTED) {
    if (g_shared_packet_pool->aborted() || now_ns() - begin >= TIMEOUT_NS || !reap(leader_pid, leader_reaped) ||
        leader_reaped)
      return false;
    usleep(50);
  }
  accept(seq, true);
  return true;
}
bool all_reaped() {
  bool done = leader_reaped;
  for (uint64_t i = control->retired.load(); i < control->head.load(); ++i) {
    const Segment &s = segment(i);
    done &= s.reaped || s.status.load(std::memory_order_acquire) == Status::PROMOTED;
  }
  return done;
}
} // namespace

bool difftest_ref_fork_init(uint64_t interval_ms) {
  if (enabled || (interval_ms != 0 && interval_ms < 3000) || interval_ms > UINT64_MAX / 1000000 || NUM_CORES != 1 ||
      CONFIG_DMA_CHANNELS != 1) {
    fprintf(stderr, "REF fork requires one core/channel and interval 0 or >=3 seconds\n");
    return false;
  }
  interval_ns = interval_ms * 1000000;
  enabled = true;
  return true;
}
bool difftest_ref_fork_enabled() {
  return enabled;
}
bool difftest_ref_fork_is_child() {
  return role == Role::CHECKER;
}
int difftest_ref_fork_start() {
  if (!enabled || !g_shared_packet_pool || control)
    return -1;
#if defined(CONFIG_DIFFTEST_LOADEVENT) || defined(CONFIG_DIFFTEST_STOREEVENT) ||                     \
    defined(CONFIG_DIFFTEST_ARCHINTDELAYEDUPDATE) || defined(CONFIG_DIFFTEST_ARCHFPDELAYEDUPDATE) || \
    defined(CONFIG_DIFFTEST_AMUCTRLEVENT) || defined(CONFIG_DIFFTEST_CMOINVALEVENT) ||               \
    defined(CONFIG_DIFFTEST_MSYNCEVENT) || defined(CONFIG_DIFFTEST_REFILLEVENT) ||                   \
    defined(CONFIG_DIFFTEST_L1TLBEVENT) || defined(CONFIG_DIFFTEST_L2TLBEVENT)
  fprintf(stderr, "REF fork checkpoints do not yet preserve history-dependent checker queues\n");
  return -1;
#endif
  void *mem = mmap(nullptr, sizeof(Shared), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
  if (mem == MAP_FAILED)
    return -1;
  control = new (mem) Shared();
  supervisor_pid = getpid();
  fflush(nullptr);
  leader_pid = fork();
  if (leader_pid < 0)
    return -1;
  if (leader_pid == 0) {
    role = Role::LEADER;
    if (prctl(PR_SET_PDEATHSIG, SIGKILL) != 0 || getppid() != supervisor_pid)
      _exit(2);
    return 1;
  }
  printf("RefSupervisor PID=%d Leader=%d PoolSlots=%zu\n", int(supervisor_pid), int(leader_pid),
         g_shared_packet_pool->capacity);
  fflush(stdout);
  return 0;
}
int difftest_ref_fork_prepare(Difftest *self) {
  if (!enabled)
    return 0;
  if (!control || role != Role::LEADER)
    return 2;
  pause_leader();
  if (g_shared_packet_pool->aborted())
    return 2;
  const bool next = current_segment == OPEN_END || (interval_ns && now_ns() >= deadline_ns && !self->dut->trap.hasTrap);
  if (!next)
    return 0;
  close_segment(self);
  uint64_t seq = control->head.load(std::memory_order_relaxed);
  while (seq - control->retired.load(std::memory_order_acquire) >= MAX_SEGMENTS) {
    pause_leader();
    if (g_shared_packet_pool->aborted())
      return 2;
    usleep(50);
  }
  unsigned reader;
  while ((reader = g_shared_packet_pool->add_reader()) == SharedPacketPool::MAX_READERS) {
    pause_leader();
    if (g_shared_packet_pool->aborted())
      return 2;
    usleep(50);
  }
  Segment &s = segment(seq);
  s.generation = generation;
  s.start_window = windows;
  s.start_packet = g_shared_packet_pool->cursor();
  s.start_ns = now_ns();
  s.reader = reader;
  s.pid.store(0, std::memory_order_relaxed);
  s.end_window.store(OPEN_END, std::memory_order_relaxed);
  s.command.store(Command::WAIT, std::memory_order_relaxed);
  s.reaped = false;
  s.status.store(Status::RUNNING, std::memory_order_relaxed);
  current_segment = seq;
  // Linux fork semantics, with the supervisor as parent of both processes.
  // No CLONE_VM/CLONE_FILES: REF memory remains COW and the parser is single-threaded.
  fflush(nullptr);
  const pid_t pid = syscall(SYS_clone, CLONE_PARENT | SIGCHLD, nullptr, nullptr, nullptr, 0);
  if (pid < 0) {
    g_shared_packet_pool->retire_reader(reader);
    fail();
    return 2;
  }
  if (pid == 0) {
    role = Role::CHECKER;
    child_window = s.start_window;
    g_shared_packet_pool->enter_reader(reader);
    if (prctl(PR_SET_PDEATHSIG, SIGKILL) != 0 || getppid() != supervisor_pid)
      _exit(2);
    s.child_start_ns = now_ns();
    if (!self->set_ref_mode(REF_EXEC_SLOW)) {
      fail();
      _exit(2);
    }
    self->proxy->sync();
    return 1;
  }
  s.pid.store(pid, std::memory_order_release);
  control->head.store(seq + 1, std::memory_order_release);
  deadline_ns = now_ns() + interval_ns;
  printf("RefForkStart Generation=%lu Segment=%lu Child=%d StartWindow=%lu StartInstr=%lu Packet=%lu InitMs=%.3f\n",
         (unsigned long)generation, (unsigned long)seq, int(pid), (unsigned long)windows, (unsigned long)last_instr,
         (unsigned long)s.start_packet, double(now_ns() - s.start_ns) / 1e6);
  fflush(stdout);
  return 0;
}
int difftest_ref_fork_check(Difftest *self) {
  Segment &s = segment(current_segment);
  while (control->published_windows.load(std::memory_order_acquire) <= child_window) {
    if (s.command.load(std::memory_order_acquire) == Command::DISCARD)
      discard_checker();
    if (g_shared_packet_pool->aborted())
      complete_checker(self, false);
    if (s.end_window.load(std::memory_order_acquire) == child_window)
      break;
    usleep(50);
  }
  if (s.end_window.load(std::memory_order_acquire) == child_window) {
    complete_checker(self, true);
    return self->step();
  }
  const int ret = self->fork_check_step();
  const bool terminal = ret == DiffTestChecker::STATE_TRAP && self->get_trap_code() == STATE_GOODTRAP;
  if (ret != DiffTestChecker::STATE_OK && !terminal) {
    fprintf(stderr, "RefForkLocalFailure Generation=%lu Segment=%lu Window=%lu Ret=%d\n", (unsigned long)generation,
            (unsigned long)current_segment, (unsigned long)child_window, ret);
    self->proxy->display(self->dut);
    complete_checker(self, false);
  }
  ++child_window;
  // CriticalError is a legitimate terminal window when the checker agrees.
  // Wait for FAST to close it rather than rejecting STATE_TRAP as an error.
  while (terminal && s.end_window.load(std::memory_order_acquire) == OPEN_END) {
    if (g_shared_packet_pool->aborted())
      complete_checker(self, false);
    usleep(50);
  }
  if (s.end_window.load(std::memory_order_acquire) == child_window)
    complete_checker(self, true);
  return ret;
}
void difftest_ref_fork_publish(Difftest *self) {
  if (!enabled || role != Role::LEADER)
    return;
  last_instr = self->dut->trap.instrCnt;
  control->published_windows.store(++windows, std::memory_order_release);
}
void difftest_ref_fork_abort_child() {
  if (role == Role::CHECKER)
    complete_checker(difftest[0], false);
}
int difftest_ref_fork_idle() {
  if (!control)
    return 0;
  if (role == Role::CHECKER) {
    Segment &s = segment(current_segment);
    if (s.command.load(std::memory_order_acquire) == Command::DISCARD)
      discard_checker();
    if (g_shared_packet_pool->aborted())
      complete_checker(difftest[0], false);
    if (s.end_window.load(std::memory_order_acquire) == child_window)
      complete_checker(difftest[0], true);
    return 0;
  }
  if (role == Role::LEADER) {
    pause_leader();
    return g_shared_packet_pool->aborted() ? 2 : 0;
  }
  poll_checkers();
  if (!reap(leader_pid, leader_reaped))
    fail();
  if (leader_reaped) {
    g_shared_packet_pool->retire_reader(leader_reader);
    if (control->complete.load(std::memory_order_acquire) != 1)
      fail();
  }
  if (!g_shared_packet_pool->aborted()) {
    const uint64_t end = control->head.load(std::memory_order_acquire);
    while (trusted_segment < end) {
      Segment &s = segment(trusted_segment);
      const auto status = s.status.load(std::memory_order_acquire);
      if (status == Status::FAILED) {
        fprintf(stderr, "REF trusted checkpoint failed Generation=%lu Segment=%lu Window=%lu\n",
                (unsigned long)s.generation, (unsigned long)trusted_segment, (unsigned long)trusted_windows);
        fail();
        break;
      }
      if (status != Status::MATCH && status != Status::DIVERGED)
        break;
      const uint64_t stop = s.end_window.load(std::memory_order_acquire);
      if (s.start_window != trusted_windows || stop == OPEN_END || s.checked_windows != stop - s.start_window) {
        fail();
        break;
      }
      if (status == Status::DIVERGED) {
        if (!recover(trusted_segment))
          fail();
        break;
      }
      accept(trusted_segment, false);
    }
    retire_prefix();
    if (control->endpoint.load(std::memory_order_acquire)) {
      if (!drain_start_ns)
        drain_start_ns = now_ns();
      if (now_ns() - drain_start_ns >= TIMEOUT_NS)
        fail();
      if (trusted_segment == control->head.load(std::memory_order_acquire) &&
          trusted_windows == control->published_windows.load(std::memory_order_acquire))
        control->complete.store(1, std::memory_order_release);
    }
  }
  if (g_shared_packet_pool->aborted()) {
    fail();
    if (!leader_reaped)
      kill(leader_pid, SIGKILL);
    for (uint64_t i = control->retired.load(); i < control->head.load(); ++i) {
      Segment &s = segment(i);
      if (!s.reaped && s.status.load() != Status::PROMOTED) {
        s.command.store(Command::DISCARD, std::memory_order_release);
        if (s.pid.load() > 0)
          kill(s.pid.load(), SIGKILL);
      }
    }
  }
  if (control->complete.load(std::memory_order_acquire) && all_reaped()) {
    const int result = control->complete.load();
    if (!summary_printed) {
      printf(
          "RefForkSummary %s Segments=%lu FastWindows=%lu CheckedWindows=%lu TrustedWindows=%lu "
          "Recoveries=%lu SnapshotCopyBytes=0\n",
          result == 1 ? "MATCH" : "MISMATCH", (unsigned long)accepted_segments,
          (unsigned long)control->published_windows.load(), (unsigned long)checked_windows,
          (unsigned long)trusted_windows, (unsigned long)recoveries);
      fflush(stdout);
      summary_printed = true;
    }
    return result;
  }
  return 0;
}
int difftest_ref_fork_finish() {
  if (!control)
    return 0;
  if (role == Role::LEADER) {
    pause_leader();
    close_segment(difftest[0]);
    control->endpoint.store(true, std::memory_order_release);
    while (!control->complete.load(std::memory_order_acquire)) {
      pause_leader();
      usleep(1000);
    }
    return control->complete.load() == 1 ? 0 : 2;
  }
  if (role == Role::CHECKER)
    return 0;
  int result;
  while (!(result = difftest_ref_fork_idle()))
    usleep(1000);
  munmap(control, sizeof(Shared));
  control = nullptr;
  return result == 1 ? 0 : 2;
}
void difftest_ref_fork_leader_exit(int result) {
  if (role == Role::CHECKER)
    complete_checker(difftest[0], false);
  if (result)
    fail();
  _exit(result ? 2 : 0);
}
#else
bool difftest_ref_fork_init(uint64_t) {
  return false;
}
bool difftest_ref_fork_enabled() {
  return false;
}
bool difftest_ref_fork_is_child() {
  return false;
}
int difftest_ref_fork_start() {
  return -1;
}
int difftest_ref_fork_prepare(Difftest *) {
  return 0;
}
int difftest_ref_fork_check(Difftest *) {
  return 0;
}
void difftest_ref_fork_publish(Difftest *) {}
int difftest_ref_fork_finish() {
  return 0;
}
int difftest_ref_fork_idle() {
  return 0;
}
void difftest_ref_fork_abort_child() {}
void difftest_ref_fork_leader_exit(int) {}
#endif
