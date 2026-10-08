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
#include <cstdlib>
#include <new>
#include <sys/prctl.h>
#include <sys/wait.h>
#include <unistd.h>

#ifdef CONFIG_DIFFTEST_FORK

namespace {
constexpr unsigned MAX_SEGMENTS = 256;
constexpr uint64_t OPEN_END = UINT64_MAX;
uint64_t now_ns() {
  return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch())
      .count();
}
bool enabled = false;
uint64_t interval_ns = 0;
constexpr uint64_t DRAIN_TIMEOUT_NS = 300ULL * 1000000000;

enum class SegmentStatus {
  EMPTY,
  RUNNING,
  CHECKED,
  FAILED
};

struct Segment {
  uint64_t start_window, start_instr, start_packet, start_ns;
  unsigned reader;
  std::atomic<uint64_t> end_window{OPEN_END};
  uint64_t end_instr, fast_end_ns;
  int fast_commit_stamp = 0, slow_commit_stamp = 0;
  DifftestStateHash fast_hash{}, slow_hash{};
  uint64_t checked_windows, child_start_ns, child_end_ns, end_packet;
  std::atomic<SegmentStatus> status{SegmentStatus::EMPTY};
};
struct Shared {
  std::atomic<uint64_t> published_windows{0};
  Segment segments[MAX_SEGMENTS];
};
Shared *control = nullptr;
pid_t children[MAX_SEGMENTS]{};
bool reaped[MAX_SEGMENTS]{};
int exits[MAX_SEGMENTS]{};
unsigned segment_count = 0, child_segment = 0;
uint64_t windows = 0, last_instr = 0, deadline_ns = 0, fork_ns = 0;
uint64_t child_window = 0;
bool child = false;

bool hashes_match(const DifftestStateHash &a, const DifftestStateHash &b) {
  return a.state_lo == b.state_lo && a.state_hi == b.state_hi && a.store_lo == b.store_lo && a.store_hi == b.store_hi &&
         a.store_count == b.store_count;
}

void child_exit(Difftest *self, bool ok) {
  Segment &s = control->segments[child_segment];
  s.checked_windows = child_window - s.start_window;
  s.end_packet = g_shared_packet_pool->cursor();
  if (ok) {
    self->proxy->sync();
    s.slow_hash = self->proxy->state_hash();
    s.slow_commit_stamp = self->fork_commit_stamp();
    ok = child_window == s.end_window.load(std::memory_order_acquire) && hashes_match(s.fast_hash, s.slow_hash) &&
         s.fast_commit_stamp == s.slow_commit_stamp;
  }
  s.child_end_ns = now_ns();
  s.status.store(ok ? SegmentStatus::CHECKED : SegmentStatus::FAILED, std::memory_order_release);
  if (!ok)
    g_shared_packet_pool->fail();
  g_shared_packet_pool->finish_reader();
  // Only the parent retires readers after waitpid, so a slot cannot be reused
  // before the old child's reap and accidentally retired a second time.
  _exit(ok ? 0 : 2);
}

void close_segment(Difftest *self) {
  Segment &s = control->segments[segment_count - 1];
  if (s.end_window.load(std::memory_order_acquire) != OPEN_END)
    return;
  if (self->fast_ref_status())
    g_shared_packet_pool->fail();
  self->proxy->sync();
  s.fast_hash = self->proxy->state_hash();
  s.fast_commit_stamp = self->fork_commit_stamp();

  s.end_instr = last_instr;
  s.fast_end_ns = now_ns();
  s.end_window.store(windows, std::memory_order_release);
  printf("RefForkClose Segment=%u StartWindow=%lu EndWindow=%lu EndInstr=%lu FastSpanMs=%.3f\n", segment_count - 1,
         (unsigned long)s.start_window, (unsigned long)windows, (unsigned long)last_instr,
         double(s.fast_end_ns - s.start_ns) / 1e6);
  fflush(stdout);
}
} // namespace

bool difftest_ref_fork_init(uint64_t interval_ms) {
  if (enabled || (interval_ms != 0 && interval_ms < 3000) || interval_ms > UINT64_MAX / 1000000 ||
      NUM_CORES != 1 || CONFIG_DMA_CHANNELS != 1) {
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
  return child;
}

static int poll_children() {
  if (!control || child)
    return 0;
  int failure = 0;
  for (unsigned i = 0; i < segment_count; ++i) {
    if (reaped[i]) {
      if (exits[i] != 0)
        failure = 2;
      continue;
    }
    int status = 0;
    const pid_t got = waitpid(children[i], &status, WNOHANG);
    if (got < 0 && errno == EINTR)
      continue;
    if (got == 0)
      continue;
    reaped[i] = true;
    g_shared_packet_pool->retire_reader(control->segments[i].reader);
    exits[i] = got == children[i] && WIFEXITED(status) ? WEXITSTATUS(status) : 2;
    if (exits[i] != 0 || control->segments[i].status.load(std::memory_order_acquire) != SegmentStatus::CHECKED) {
      g_shared_packet_pool->fail();
      failure = 2;
    }
  }
  return failure || g_shared_packet_pool->aborted() ? 2 : 0;
}

int difftest_ref_fork_prepare(Difftest *self) {
  if (!enabled)
    return 0;
#if defined(CONFIG_DIFFTEST_LOADEVENT) || defined(CONFIG_DIFFTEST_STOREEVENT) ||                     \
    defined(CONFIG_DIFFTEST_ARCHINTDELAYEDUPDATE) || defined(CONFIG_DIFFTEST_ARCHFPDELAYEDUPDATE) || \
    defined(CONFIG_DIFFTEST_AMUCTRLEVENT) || defined(CONFIG_DIFFTEST_CMOINVALEVENT) ||               \
    defined(CONFIG_DIFFTEST_MSYNCEVENT) || defined(CONFIG_DIFFTEST_REFILLEVENT) ||                   \
    defined(CONFIG_DIFFTEST_L1TLBEVENT) || defined(CONFIG_DIFFTEST_L2TLBEVENT)
  fprintf(stderr, "REF fork checkpoints do not yet preserve history-dependent checker queues\n");
  return 2;
#endif
  if (!g_shared_packet_pool) {
    fprintf(stderr, "REF fork requires shared packets\n");
    return 2;
  }
  if (!control) {
    void *mem = mmap(nullptr, sizeof(Shared), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (mem == MAP_FAILED)
      return 2;
    control = new (mem) Shared();
  }
  if ((windows & 1023) == 0 && poll_children())
    return 2;
  const bool next = segment_count == 0 || (interval_ns != 0 && now_ns() >= deadline_ns && !self->dut->trap.hasTrap);
  if (!next)
    return 0;
  if (segment_count >= MAX_SEGMENTS) {
    fprintf(stderr, "REF fork segment limit reached\n");
    return 2;
  }
  const uint64_t begin = now_ns();
  if (segment_count != 0)
    close_segment(self);
  const unsigned idx = segment_count;
  Segment &s = control->segments[idx];
  s.start_window = windows;
  s.start_instr = last_instr;
  s.start_packet = g_shared_packet_pool->cursor();
  while ((s.reader = g_shared_packet_pool->add_reader()) == SharedPacketPool::MAX_READERS) {
    if (poll_children())
      return 2;
    usleep(50);
  }
  s.start_ns = now_ns();
  s.status.store(SegmentStatus::RUNNING, std::memory_order_release);
  const pid_t parent_pid = getpid();
  const pid_t pid = fork();
  if (pid < 0) {
    g_shared_packet_pool->retire_reader(s.reader);
    g_shared_packet_pool->fail();
    return 2;
  }
  if (pid == 0) {
    child = true;
    child_segment = idx;
    child_window = s.start_window;
    g_shared_packet_pool->enter_reader(s.reader);
    if (prctl(PR_SET_PDEATHSIG, SIGKILL) != 0)
      _exit(2);
    if (getppid() != parent_pid)
      _exit(2);
    s.child_start_ns = now_ns();
    // NEMU's mode switch also refreshes MMU and permission caches.
    self->set_ref_mode(REF_EXEC_SLOW);
    self->proxy->sync();
    return 1;
  }
  children[idx] = pid;
  ++segment_count;
  const uint64_t elapsed = now_ns() - begin;
  fork_ns += elapsed;
  deadline_ns = now_ns() + interval_ns;
  printf("RefForkStart Segment=%u Child=%d StartWindow=%lu StartInstr=%lu Packet=%lu InitMs=%.3f IntervalMs=%lu\n", idx,
         int(pid), (unsigned long)windows, (unsigned long)last_instr, (unsigned long)s.start_packet,
         double(elapsed) / 1e6, (unsigned long)(interval_ns / 1000000));
  fflush(stdout);
  return 0;
}

int difftest_ref_fork_check(Difftest *self) {
  Segment &s = control->segments[child_segment];
  while (control->published_windows.load(std::memory_order_acquire) <= child_window) {
    if (g_shared_packet_pool->aborted())
      child_exit(self, false);
    if (s.end_window.load(std::memory_order_acquire) == child_window)
      child_exit(self, true);
    usleep(50);
  }
  // The publication of the next window follows closure of the old segment.
  if (s.end_window.load(std::memory_order_acquire) == child_window)
    child_exit(self, true);
  const int ret = self->fork_check_step();
  if (ret != DiffTestChecker::STATE_OK) {
    self->proxy->sync();
    fprintf(stderr, "RefFork child check failed Segment=%u Window=%lu Ret=%d Instr=%lu RefPC=0x%lx\n", child_segment,
            (unsigned long)child_window, ret, (unsigned long)self->dut->trap.instrCnt,
            (unsigned long)self->proxy->state.pc);
    self->proxy->display(self->dut);
    fflush(stdout);
    child_exit(self, false);
  }
  ++child_window;
  if (s.end_window.load(std::memory_order_acquire) == child_window)
    child_exit(self, true);
  return ret;
}

void difftest_ref_fork_publish(Difftest *self) {
  if (!control || child)
    return;
  last_instr = self->dut->trap.instrCnt;
  control->published_windows.store(++windows, std::memory_order_release);
}

void difftest_ref_fork_abort_child() {
  if (child)
    child_exit(difftest[0], false);
}

int difftest_ref_fork_idle() {
  if (!control)
    return 0;
  if (!child)
    return poll_children();
  if (g_shared_packet_pool->aborted())
    child_exit(difftest[0], false);
  if (control->segments[child_segment].end_window.load(std::memory_order_acquire) == child_window) {
    child_exit(difftest[0], true);
  }
  return 0;
}

int difftest_ref_fork_finish() {
  static bool finished = false;
  static int final_result = 0;
  if (finished)
    return final_result;
  if (!control || child)
    return 0;
  if (segment_count == 0)
    return 2;
  close_segment(difftest[0]);
  const uint64_t begin = now_ns();
  int ret = 0;
  uint64_t expected = 0, checked = 0, trusted = 0;
  bool prefix_ok = true;
  bool killed = false;
  while (true) {
    const bool failed = poll_children() != 0;
    if (!killed && (failed || now_ns() - begin >= DRAIN_TIMEOUT_NS)) {
      fprintf(stderr, "REF fork failed or exceeded final drain timeout\n");
      g_shared_packet_pool->fail();
      ret = 2;
      for (unsigned i = 0; i < segment_count; ++i)
        if (!reaped[i])
          kill(children[i], SIGKILL);
      killed = true;
    }
    bool done = true;
    for (unsigned i = 0; i < segment_count; ++i)
      done &= reaped[i];
    if (done)
      break;
    usleep(1000);
  }
  for (unsigned i = 0; i < segment_count; ++i) {
    Segment &s = control->segments[i];
    const uint64_t end = s.end_window.load(std::memory_order_acquire);
    const bool match = exits[i] == 0 && s.status.load(std::memory_order_acquire) == SegmentStatus::CHECKED &&
                       s.start_window == expected && s.checked_windows == end - s.start_window &&
                       hashes_match(s.fast_hash, s.slow_hash) && s.fast_commit_stamp == s.slow_commit_stamp;
    printf(
        "RefForkResult Segment=%u %s StartWindow=%lu EndWindow=%lu Checked=%lu StartInstr=%lu EndInstr=%lu "
        "FastMs=%.3f SlowMs=%.3f LagMs=%.3f StartPacket=%lu EndPacket=%lu Exit=%d Stores=%lu\n",
        i, prefix_ok && match ? "MATCH" : (match ? "UNTRUSTED" : "MISMATCH"), (unsigned long)s.start_window,
        (unsigned long)end, (unsigned long)s.checked_windows, (unsigned long)s.start_instr, (unsigned long)s.end_instr,
        double(s.fast_end_ns - s.start_ns) / 1e6,
        s.child_end_ns >= s.child_start_ns ? double(s.child_end_ns - s.child_start_ns) / 1e6 : 0.0,
        s.child_end_ns > s.fast_end_ns ? double(s.child_end_ns - s.fast_end_ns) / 1e6 : 0.0,
        (unsigned long)s.start_packet, (unsigned long)s.end_packet, exits[i], (unsigned long)s.slow_hash.store_count);
    checked += s.checked_windows;
    expected = end;
    prefix_ok &= match;
    if (prefix_ok)
      trusted = end;
    if (!match)
      ret = 2;
  }
  if (checked != windows || expected != windows)
    ret = 2;
  printf(
      "RefForkSummary %s Segments=%u FastWindows=%lu CheckedWindows=%lu ForkMs=%.3f FinishWaitMs=%.3f "
      "TrustedWindows=%lu SnapshotCopyBytes=0\n",
      ret == 0 ? "MATCH" : "MISMATCH", segment_count, (unsigned long)windows, (unsigned long)checked,
      double(fork_ns) / 1e6, double(now_ns() - begin) / 1e6, (unsigned long)trusted);
  fflush(stdout);
  munmap(control, sizeof(Shared));
  control = nullptr;
  finished = true;
  final_result = ret;
  return ret;
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
#endif
