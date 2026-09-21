/***************************************************************************************
* Copyright (c) 2020-2023 Institute of Computing Technology, Chinese Academy of Sciences
* Copyright (c) 2020-2021 Peng Cheng Laboratory
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

#include "difftest.h"
#include "common.h"
#include "difftrace.h"
#include "dut.h"
#include "flash.h"
#include "goldenmem.h"
#include "ram.h"
#include "spikedasm.h"
#include "splitview.h"
#ifdef CONFIG_DIFFTEST_FORK
#include <algorithm>
#include <chrono>
#include <numeric>
#include <vector>
#endif // CONFIG_DIFFTEST_FORK
#include <csignal>
#include <cstdlib>
#ifdef CONFIG_DIFFTEST_FORK
#include <cerrno>
#include <sched.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#endif // CONFIG_DIFFTEST_FORK
#if defined(CONFIG_DIFFTEST_SQUASH) && !defined(CONFIG_DIFFTEST_FPGA)
#include "svdpi.h"
#endif // CONFIG_DIFFTEST_SQUASH && !CONFIG_DIFFTEST_FPGA
#ifdef CONFIG_DIFFTEST_PERFCNT
#include "perf.h"
#endif // CONFIG_DIFFTEST_PERFCNT
#ifdef CONFIG_DIFFTEST_QUERY
#include "query.h"
#endif // CONFIG_DIFFTEST_QUERY

namespace {
const bool fast_only = []() {
  const char *value = getenv("DIFFTEST_FAST_ONLY");
  return value != nullptr && value[0] == '1';
}();
const bool fast_only_debug = []() {
  const char *value = getenv("DIFFTEST_FAST_ONLY_DEBUG");
  return value != nullptr && value[0] == '1';
}();
uint64_t fast_only_ref_exec_instr = 0;
uint64_t fast_only_skip_count = 0;
uint64_t fast_only_event_count = 0;
uint64_t fast_only_debug_events = 0;
}

#ifdef CONFIG_DIFFTEST_FORK
namespace {
constexpr size_t fork_default_max_outstanding = 64;
constexpr size_t fork_default_group_size = 100;
constexpr size_t fork_max_group_size = 65536;
constexpr uint64_t fork_default_interval_ms = 10000;

const size_t fork_max_outstanding = []() {
  const char *value = getenv("DIFFTEST_FORK_MAX_OUTSTANDING");
  if (value == nullptr) return fork_default_max_outstanding;
  const size_t requested = strtoul(value, nullptr, 0);
  return std::max<size_t>(1, std::min(requested, fork_default_max_outstanding));
}();

const size_t fork_group_size = []() {
  const char *value = getenv("DIFFTEST_FORK_GROUP_SIZE");
  if (value == nullptr) return fork_default_group_size;
  const size_t requested = strtoul(value, nullptr, 0);
  return std::max<size_t>(1, std::min(requested, fork_max_group_size));
}();

const uint64_t fork_interval_ms = []() {
  const char *value = getenv("DIFFTEST_FORK_INTERVAL_MS");
  if (value == nullptr) return fork_default_interval_ms;
  return std::max<uint64_t>(1, strtoull(value, nullptr, 0));
}();
const bool fork_interval_debug = []() {
  const char *value = getenv("DIFFTEST_FORK_INTERVAL_DEBUG");
  return value != nullptr && value[0] == '1';
}();

const bool fork_allow_skip = []() {
  const char *value = getenv("DIFFTEST_FORK_ALLOW_SKIP");
  return value != nullptr && value[0] == '1';
}();

enum ForkChildAction : int {
  FORK_CHILD_WAIT = 0,
  FORK_CHILD_STOP = 1,
  FORK_CHILD_PROMOTE = 2,
};

struct ForkSharedResult {
  // The result is first published by the slow child.  When the endpoint does
  // not match, the same child can be promoted to the authoritative service;
  // the command slot then carries future DUT snapshots without copying NEMU
  // state back to the fast parent.
  volatile int ready;
  volatile int action;
  volatile uint64_t command_seq;
  volatile uint64_t command_ack;
  int command_ret;
  int command_trap;
  uint64_t command_dut_pc;
  uint64_t command_ref_pc_before;
  uint64_t command_ref_pc_after;
  uint8_t command_commit_valid;
  uint8_t command_skip;
  uint8_t command_fused;
  uint8_t command_fast_catchup;
  uint64_t command_group_count;
  uint64_t command_skip_window_count;
  uint64_t command_skip_commit_count;
  uint64_t command_skip_instr_count;
  uint64_t command_skip_blocked_window_count;
  uint64_t command_rollback_count;
  uint64_t command_mismatch_count;
  uint64_t command_release_count;
  uint64_t command_promotion_count;
  uint64_t command_window_count;
  uint64_t command_window_instr;
  size_t command_peak_outstanding;
  DiffTestState command_dut;
  int ret;
  int failed_index;
  int commit_stamp;
  DifftestStateHash state_hash;
};

template <typename T> T fork_load(const volatile T *value) {
  return __atomic_load_n(value, __ATOMIC_ACQUIRE);
}

template <typename T> void fork_store(volatile T *value, T data) {
  __atomic_store_n(value, data, __ATOMIC_RELEASE);
}

struct PendingForkGroup {
  uint64_t id;
  pid_t pid;
  ForkSharedResult *result;
  int parent_commit_stamp;
  DifftestStateHash parent_hash;
  std::vector<DifftestForkWindow> windows;
};

void stop_child(PendingForkGroup &group) {
  if (waitpid(group.pid, nullptr, WNOHANG) == 0) {
    kill(group.pid, SIGKILL);
    waitpid(group.pid, nullptr, 0);
  }
  munmap(group.result, sizeof(ForkSharedResult));
}

void stop_promoted_child(pid_t pid, ForkSharedResult *result) {
  if (result == nullptr) return;
  fork_store(&result->action, static_cast<int>(FORK_CHILD_STOP));
  int status = 0;
  if (waitpid(pid, &status, WNOHANG) == 0) {
    kill(pid, SIGKILL);
    waitpid(pid, &status, 0);
  }
  munmap(result, sizeof(ForkSharedResult));
}
} // namespace

std::vector<uint32_t> fork_window_sizes;
std::vector<uint64_t> fork_intervals_ms;
std::deque<PendingForkGroup> fork_pending_groups;
uint64_t fork_group_count = 0;
uint64_t fork_skip_window_count = 0;
uint64_t fork_skip_commit_count = 0;
uint64_t fork_skip_instr_count = 0;
uint64_t fork_skip_blocked_window_count = 0;
uint64_t fork_rollback_count = 0;
uint64_t fork_mismatch_count = 0;
uint64_t fork_group_release_count = 0;
size_t fork_peak_outstanding = 0;
uint64_t fork_window_count = 0;
uint64_t fork_window_instr_count = 0;
bool fork_worker_process = false;
pid_t fork_promoted_pid = -1;
ForkSharedResult *fork_promoted_result = nullptr;
uint64_t fork_promoted_sequence = 0;
uint64_t fork_promotion_count = 0;
uint64_t fork_interval_debug_count = 0;
bool fork_interval_started = false;
std::chrono::steady_clock::time_point fork_interval_start;

bool fork_interval_reached() {
  if (!fork_interval_started) return false;
  const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::steady_clock::now() - fork_interval_start);
  return static_cast<uint64_t>(elapsed.count()) >= fork_interval_ms;
}
#endif // CONFIG_DIFFTEST_FORK

Difftest **difftest = NULL;
static volatile sig_atomic_t difftest_signal_handling = 0;

static void difftest_signal_handler(int signo) {
#ifdef CONFIG_DIFFTEST_FORK
  if (fork_worker_process) {
    _Exit(128 + signo);
  }
#endif // CONFIG_DIFFTEST_FORK
  if (signo != SIGINT) {
    common_splitview_force_cleanup();
    if (difftest != NULL) {
      difftest_finish();
    }
    std::signal(signo, SIG_DFL);
    raise(signo);
    return;
  }

  if (difftest_signal_handling) {
    common_splitview_force_cleanup();
    _Exit(128 + signo);
  }
  difftest_signal_handling = 1;
  common_splitview_request_finish();
  signal_num = signo;
}

static void difftest_register_exit_handlers() {
  struct sigaction sigint_action {};
  sigint_action.sa_handler = difftest_signal_handler;
  sigemptyset(&sigint_action.sa_mask);
  sigaction(SIGINT, &sigint_action, nullptr);
  std::signal(SIGTERM, difftest_signal_handler);
  std::signal(SIGABRT, difftest_signal_handler);
  std::signal(SIGSEGV, difftest_signal_handler);
  std::signal(SIGBUS, difftest_signal_handler);
}

int difftest_init(bool enabled, size_t ramsize) {
#ifdef CONFIG_DIFFTEST_FORK
  static_assert(NUM_CORES == 1, "fork DiffTest currently supports one core");
#endif // CONFIG_DIFFTEST_FORK
#ifdef CONFIG_DIFFTEST_PERFCNT
  difftest_perfcnt_init();
#endif // CONFIG_DIFFTEST_PERFCNT
#ifdef CONFIG_DIFFTEST_IOTRACE
  difftest_iotrace_init();
#endif // CONFIG_DIFFTEST_IOTRACE
#ifdef CONFIG_DIFFTEST_QUERY
  difftest_query_init();
#endif // CONFIG_DIFFTEST_QUERY
  diffstate_buffer_init();
  difftest = new Difftest *[NUM_CORES];
  // put init_goldenmem before update_nemuproxy because the latter requires goldenmem
  if (enabled) {
    init_goldenmem();
  }
  for (int i = 0; i < NUM_CORES; i++) {
    difftest[i] = new Difftest(i);
    difftest[i]->dut = diffstate_buffer[i]->get(0, 0);
    if (enabled) {
      difftest[i]->update_nemuproxy(i, ramsize);
      difftest[i]->init_checkers();
    }
  }
  difftest_register_exit_handlers();
  return 0;
}

int difftest_state() {
  for (int i = 0; i < NUM_CORES; i++) {
    if (difftest[i]->get_trap_valid()) {
#ifdef CONFIG_DIFFTEST_FORK
      if (difftest[i]->drain_fork(true) != DiffTestChecker::STATE_OK) {
        return STATE_ABORT;
      }
#endif // CONFIG_DIFFTEST_FORK
      return difftest[i]->get_trap_code();
    }
    if (difftest[i]->proxy && difftest[i]->proxy->get_status()) {
      return difftest[i]->proxy->get_status();
    }
  }
  return STATE_RUNNING;
}

#include "common.h"

int difftest_nstep(int step, bool enable_diff) {
#ifdef CONFIG_DIFFTEST_PERFCNT
  difftest_calls[perf_difftest_nstep]++;
  difftest_bytes[perf_difftest_nstep] += 1;
#endif // CONFIG_DIFFTEST_PERFCNT

#if CONFIG_DIFFTEST_ZONESIZE > 1
  difftest_switch_zone();
#endif // CONFIG_DIFFTEST_ZONESIZE
  for (int i = 0; i < step; i++) {
    if (enable_diff) {
      if (int ret = difftest_step())
        return ret;
    } else {
      difftest_set_dut();
    }
    int status = difftest_state();
    if (status != STATE_RUNNING)
      return status;
  }
  return STATE_RUNNING;
}

void difftest_switch_zone() {
  for (int i = 0; i < NUM_CORES; i++) {
    diffstate_buffer[i]->switch_zone();
  }
}
void difftest_set_dut() {
  for (int i = 0; i < NUM_CORES; i++) {
    difftest[i]->dut = diffstate_buffer[i]->next();
  }
}

// difftest_step returns a trap code
int difftest_step() {
  difftest_set_dut();
#ifdef CONFIG_DIFFTEST_QUERY
  difftest_query_step();
#endif // CONFIG_DIFFTEST_QUERY
  for (int i = 0; i < NUM_CORES; i++) {
    if (int ret = difftest[i]->step()) {
      switch (ret) {
        case DiffTestChecker::STATE_DIFF: difftest[i]->display();
        case DiffTestChecker::STATE_ERROR: return STATE_ABORT;
        default: // STATE_TRAP
          return difftest[i]->get_trap_code();
      }
    }
  }
  return DiffTestChecker::STATE_OK;
}

void difftest_trace_read() {
  for (int i = 0; i < NUM_CORES; i++) {
    difftest[i]->trace_read();
  }
}

void difftest_trace_write(int step) {
  for (int i = 0; i < NUM_CORES; i++) {
    difftest[i]->trace_write(step);
  }
}

void difftest_finish() {
#ifdef CONFIG_DIFFTEST_FORK
  if (fast_only) {
    printf("FastOnlyRefExecInstr = %lu\n", static_cast<unsigned long>(fast_only_ref_exec_instr));
    printf("FastOnlySkipCnt = %lu\n", static_cast<unsigned long>(fast_only_skip_count));
    printf("FastOnlyEventCnt = %lu\n", static_cast<unsigned long>(fast_only_event_count));
  }
  if (!fast_only) {
    for (int i = 0; i < NUM_CORES; ++i) {
      if (int ret = difftest[i]->finish_fork()) {
        Info("fork DiffTest final group check failed on core %d (ret=%d)\n", i, ret);
      }
    }
  }
#endif // CONFIG_DIFFTEST_FORK
#ifdef CONFIG_DIFFTEST_FORK
  for (auto &group : fork_pending_groups) {
    stop_child(group);
  }
  fork_pending_groups.clear();
  if (fork_promoted_result != nullptr) {
    fork_group_count = fork_promoted_result->command_group_count;
    fork_skip_window_count = fork_promoted_result->command_skip_window_count;
    fork_skip_commit_count = fork_promoted_result->command_skip_commit_count;
    fork_skip_instr_count = fork_promoted_result->command_skip_instr_count;
    fork_skip_blocked_window_count = fork_promoted_result->command_skip_blocked_window_count;
    fork_rollback_count = fork_promoted_result->command_rollback_count;
    fork_mismatch_count = fork_promoted_result->command_mismatch_count;
    fork_group_release_count = fork_promoted_result->command_release_count;
    fork_promotion_count = fork_promoted_result->command_promotion_count;
    fork_window_count = fork_promoted_result->command_window_count;
    fork_window_instr_count = fork_promoted_result->command_window_instr;
    fork_peak_outstanding = fork_promoted_result->command_peak_outstanding;
    stop_promoted_child(fork_promoted_pid, fork_promoted_result);
    fork_promoted_result = nullptr;
    fork_promoted_pid = -1;
  }
  if (fork_window_count != 0) {
    std::sort(fork_window_sizes.begin(), fork_window_sizes.end());
    const auto percentile = [](const std::vector<uint32_t> &values, double p) {
      const size_t index = static_cast<size_t>((values.size() - 1) * p);
      return values[index];
    };
    printf("ForkWindowCnt = %lu\n", static_cast<unsigned long>(fork_window_count));
    printf("ForkGroupCnt = %lu\n", static_cast<unsigned long>(fork_group_count));
    printf("ForkSkipWindowCnt = %lu\n", static_cast<unsigned long>(fork_skip_window_count));
    printf("ForkSkipCommitCnt = %lu\n", static_cast<unsigned long>(fork_skip_commit_count));
    printf("ForkSkipInstrCnt = %lu\n", static_cast<unsigned long>(fork_skip_instr_count));
    printf("ForkSkipBlockedWindowCnt = %lu\n", static_cast<unsigned long>(fork_skip_blocked_window_count));
    printf("ForkGroupReleaseCnt = %lu\n", static_cast<unsigned long>(fork_group_release_count));
    printf("ForkGroupWindowSize = %zu\n", fork_group_size);
    printf("ForkIntervalMs = %lu\n", static_cast<unsigned long>(fork_interval_ms));
    printf("ForkMismatchCnt = %lu\n", static_cast<unsigned long>(fork_mismatch_count));
    printf("ForkRollbackCnt = %lu\n", static_cast<unsigned long>(fork_rollback_count));
    printf("ForkPromotionCnt = %lu\n", static_cast<unsigned long>(fork_promotion_count));
    printf("ForkPeakOutstanding = %zu\n", fork_peak_outstanding);
    printf("ForkWindowInstr = %lu\n", static_cast<unsigned long>(fork_window_instr_count));
    if (!fork_intervals_ms.empty()) {
      const auto minmax = std::minmax_element(fork_intervals_ms.begin(), fork_intervals_ms.end());
      uint64_t total = 0;
      for (const auto interval : fork_intervals_ms) total += interval;
      printf("ForkActualIntervalMs min=%lu avg=%lu max=%lu\n",
             static_cast<unsigned long>(*minmax.first),
             static_cast<unsigned long>(total / fork_intervals_ms.size()),
             static_cast<unsigned long>(*minmax.second));
    }
    if (fork_promotion_count == 0) {
      printf("ForkWindowN p50=%u p90=%u p99=%u max=%u\n",
             percentile(fork_window_sizes, 0.50), percentile(fork_window_sizes, 0.90),
             percentile(fork_window_sizes, 0.99), fork_window_sizes.back());
    } else {
      printf("ForkWindowN unavailable after owner promotion\n");
    }
  }
#endif // CONFIG_DIFFTEST_FORK
#ifdef CONFIG_DIFFTEST_CHECKER_PERF
  Stopwatch::print_stats(CHECKERS);
#endif
#ifdef CONFIG_DIFFTEST_PERFCNT
  uint64_t cycleCnt = difftest[0]->get_trap_event()->cycleCnt;
  uint64_t instrCnt = difftest[0]->get_trap_event()->instrCnt;
  difftest_perfcnt_finish(cycleCnt, instrCnt);
#endif // CONFIG_DIFFTEST_PERFCNT
#ifdef CONFIG_DIFFTEST_IOTRACE
  difftest_iotrace_free();
#endif // CONFIG_DIFFTEST_IOTRACE
#ifdef CONFIG_DIFFTEST_QUERY
  difftest_query_finish();
#endif // CONFIG_DIFFTEST_QUERY
  diffstate_buffer_free();
  for (int i = 0; i < NUM_CORES; i++) {
    delete difftest[i];
  }
  delete[] difftest;
  difftest = NULL;
}

#ifdef CONFIG_DIFFTEST_AMUCTRLEVENT
void difftest_mma_flush_all() {
  for (int i = 0; i < NUM_CORES; i++) {
    auto verifier = difftest[i]->get_mma_verifier();
    if (verifier) {
      verifier->flush();
    }
  }
}

void difftest_mma_stop_all() {
  for (int i = 0; i < NUM_CORES; i++) {
    auto verifier = difftest[i]->get_mma_verifier();
    if (verifier) {
      verifier->stop();
    }
  }
}

void difftest_mma_start_all() {
  for (int i = 0; i < NUM_CORES; i++) {
    auto verifier = difftest[i]->get_mma_verifier();
    if (verifier) {
      verifier->start();
    }
  }
}
#endif // CONFIG_DIFFTEST_AMUCTRLEVENT

#if defined(CONFIG_DIFFTEST_SQUASH) && !defined(CONFIG_DIFFTEST_FPGA)
svScope squashScope;
void set_squash_scope() {
  squashScope = svGetScope();
}

extern "C" void set_squash_enable(int enable);
void difftest_squash_enable(int enable) {
  if (squashScope == NULL) {
    printf("Error: Could not retrieve squash scope, set first\n");
    assert(squashScope);
  }
  svSetScope(squashScope);
  set_squash_enable(enable);
}

extern "C" void set_squash_max_fused(int squash_max_fused);
void difftest_squash_max_fused(int squash_max_fused) {
  if (squashScope == NULL) {
    printf("Error: Could not retrieve squash scope, set first\n");
    assert(squashScope);
  }
  svSetScope(squashScope);
  set_squash_max_fused(squash_max_fused);
}
#endif // CONFIG_DIFFTEST_SQUASH && !CONFIG_DIFFTEST_FPGA

#ifdef CONFIG_DIFFTEST_REPLAY
svScope replayScope;
void set_replay_scope() {
  replayScope = svGetScope();
}

extern "C" void set_replay_head(int head);
void difftest_replay_head(int head) {
  if (replayScope == NULL) {
    printf("Error: Could not retrieve replay scope, set first\n");
    assert(replayScope);
  }
  svSetScope(replayScope);
  set_replay_head(head);
}
#endif // CONFIG_DIFFTEST_REPLAY

Difftest::Difftest(int coreid) {
  state = new DiffState(coreid);
#ifdef CONFIG_DIFFTEST_REPLAY
  state_ss = (DiffState *)malloc(sizeof(DiffState));
#endif // CONFIG_DIFFTEST_REPLAY

#ifdef CONFIG_DIFFTEST_AMUCTRLEVENT
  mma_verifier = nullptr;

  // Initialize AMU finish event buffers
  for (int i = 0; i < CONFIG_DIFF_AMU_FINISH_WIDTH; ++i) {
    amu_finish_buffers[i] = new uint8_t[128 * 128 * 4];
    memset(amu_finish_buffers[i], 0, 128 * 128 * 4);
  }
#endif // CONFIG_DIFFTEST_AMUCTRLEVENT
}

Difftest::~Difftest() {
#ifdef CONFIG_DIFFTEST_AMUCTRLEVENT
  // Stop MMA verification thread and clean up verifier
  if (mma_verifier) {
    delete mma_verifier;
  }

  // Free AMU finish event buffers
  for (int i = 0; i < CONFIG_DIFF_AMU_FINISH_WIDTH; ++i) {
    delete[] amu_finish_buffers[i];
  }
#endif // CONFIG_DIFFTEST_AMUCTRLEVENT

  for (auto checker: checkers) {
    delete checker;
  }

  delete arch_event_checker;
  for (int i = 0; i < CONFIG_DIFF_COMMIT_WIDTH; i++) {
    delete instr_commit_checker[i];
  }
#ifdef CONFIG_DIFFTEST_STOREEVENT
  delete store_checker;
#endif // CONFIG_DIFFTEST_STOREEVENT
#ifdef CONFIG_DIFFTEST_LOADEVENT
  for (int i = 0; i < CONFIG_DIFF_LOAD_WIDTH; i++) {
    delete load_checker[i];
  }
#ifdef CONFIG_DIFFTEST_SQUASH
  delete load_squash_checker;
#endif // CONFIG_DIFFTEST_SQUASH
#endif // CONFIG_DIFFTEST_LOADEVENT

  delete state;
  delete difftrace;
  if (proxy) {
    delete proxy;
  }
#ifdef CONFIG_DIFFTEST_REPLAY
  free(state_ss);
  if (proxy_reg_ss) {
    free(proxy_reg_ss);
  }
#endif // CONFIG_DIFFTEST_REPLAY
}

void Difftest::init_checkers() {
  checkers.push_back(new TimeoutChecker([this]() -> DifftestTrapEvent & { return dut->trap; }, state, proxy));

  checkers.push_back(new FirstInstrCommitChecker([this]() -> DifftestInstrCommit & { return dut->commit[0]; }, state,
                                                 proxy, [this]() -> const DiffTestRegState & { return dut->regs; }));

  // Each cycle is checked for an store event, and recorded in queue.
  // It is checked every time an instruction is committed and queue has content.
#ifdef CONFIG_DIFFTEST_STOREEVENT
  for (int i = 0; i < CONFIG_DIFF_STORE_WIDTH; i++) {
    checkers.push_back(new StoreRecorder([this, i]() -> DifftestStoreEvent & { return dut->store[i]; }, state, proxy));
  }
#endif

#ifdef CONFIG_DIFFTEST_LOADEVENT
  for (int i = 0; i < CONFIG_DIFF_LOAD_WIDTH; i++) {
    auto checker = new LoadChecker([this, i]() -> DifftestLoadEvent & { return dut->load[i]; }, state, proxy, i,
                                   [this]() -> const DiffTestState & { return *dut; });
#ifdef CONFIG_DIFFTEST_SQUASH
    checkers.push_back(checker);
#else
    load_checker[i] = checker;
#endif // CONFIG_DIFFTEST_SQUASH
  }
#endif // CONFIG_DIFFTEST_LOADEVENT

#ifdef DEBUG_GOLDENMEM
  checkers.push_back(new GoldenMemoryInit([this]() -> DifftestTrapEvent & { return dut->trap; }, state, proxy));

#ifdef CONFIG_DIFFTEST_UNCACHEMMSTOREEVENT
  for (int i = 0; i < CONFIG_DIFF_UNCACHE_MM_STORE_WIDTH; i++) {
    checkers.push_back(new UncacheMmStoreChecker(
        [this, i]() -> DifftestUncacheMMStoreEvent & { return dut->uncache_mm_store[i]; }, state, proxy));
  }
#endif // CONFIG_DIFFTEST_UNCACHEMMSTOREEVENT

#ifdef CONFIG_DIFFTEST_SBUFFEREVENT
  for (int i = 0; i < CONFIG_DIFF_SBUFFER_WIDTH; i++) {
    checkers.push_back(
        new SbufferChecker([this, i]() -> DifftestSbufferEvent & { return dut->sbuffer[i]; }, state, proxy));
  }
#endif // CONFIG_DIFFTEST_SBUFFEREVENT

#ifdef CONFIG_DIFFTEST_MATRIXSTOREEVENT
  matrix_store_checker =
      new MatrixStoreChecker([this]() -> DifftestMatrixStoreEvent * { return dut->matrix_store; }, state, proxy);
  checkers.push_back(matrix_store_checker);
#endif // CONFIG_DIFFTEST_MATRIXSTOREEVENT

#ifdef CONFIG_DIFFTEST_ATOMICEVENT
  checkers.push_back(new AtomicChecker([this]() -> DifftestAtomicEvent & { return dut->atomic; }, state, proxy));
#endif // CONFIG_DIFFTEST_ATOMICEVENT
#endif

#ifdef CONFIG_DIFFTEST_CMOINVALEVENT
  checkers.push_back(
      new CmoInvalRecorder([this]() -> DifftestCMOInvalEvent & { return dut->cmo_inval; }, state, proxy));
#endif // CONFIG_DIFFTEST_CMOINVALEVENT

#ifdef DEBUG_REFILL
#ifdef CONFIG_DIFFTEST_REFILLEVENT
  for (int i = 0; i < CONFIG_DIFF_REFILL_WIDTH; i++) {
    checkers.push_back(
        new RefillChecker([this, i]() -> DifftestRefillEvent & { return dut->refill[i]; }, state, proxy, i));
  }
#endif // CONFIG_DIFFTEST_REFILLEVENT
#endif

#ifdef DEBUG_L2TLB
#ifdef CONFIG_DIFFTEST_L2TLBEVENT
  for (int i = 0; i < CONFIG_DIFF_L2TLB_WIDTH; i++) {
    checkers.push_back(new L2TLBChecker([this, i]() -> DifftestL2TLBEvent & { return dut->l2tlb[i]; }, state, proxy));
  }
#endif // CONFIG_DIFFTEST_L2TLBEVENT
#endif

#ifdef DEBUG_L1TLB
#ifdef CONFIG_DIFFTEST_L1TLBEVENT
  for (int i = 0; i < CONFIG_DIFF_L1TLB_WIDTH; i++) {
    checkers.push_back(new L1TLBChecker([this, i]() -> DifftestL1TLBEvent & { return dut->l1tlb[i]; }, state, proxy));
  }
#endif // CONFIG_DIFFTEST_L1TLBEVENT
#endif

#ifdef CONFIG_DIFFTEST_LRSCEVENT
  lrsc_checker = new LrScChecker([this]() -> DifftestLrScEvent & { return dut->lrsc; }, state, proxy);
  checkers.push_back(lrsc_checker);
#endif

#ifdef CONFIG_DIFFTEST_NONREGINTERRUPTPENDINGEVENT
  non_reg_interrupt_pending_checker = new NonRegInterruptPendingChecker(
      [this]() -> DifftestNonRegInterruptPendingEvent & { return dut->non_reg_interrupt_pending; }, state, proxy);
  checkers.push_back(non_reg_interrupt_pending_checker);
#endif

#ifdef CONFIG_DIFFTEST_MHPMEVENTOVERFLOWEVENT
  mhpmevent_overflow_checker = new MhpmeventOverflowChecker(
      [this]() -> DifftestMhpmeventOverflowEvent & { return dut->mhpmevent_overflow; }, state, proxy);
  checkers.push_back(mhpmevent_overflow_checker);
#endif
#ifdef CONFIG_DIFFTEST_CRITICALERROREVENT
  critical_error_checker =
      new CriticalErrorChecker([this]() -> DifftestCriticalErrorEvent & { return dut->critical_error; }, state, proxy);
  checkers.push_back(critical_error_checker);
#endif
#ifdef CONFIG_DIFFTEST_SYNCAIAEVENT
  aia_checker = new AiaChecker([this]() -> DifftestSyncAIAEvent & { return dut->sync_aia; }, state, proxy);
  checkers.push_back(aia_checker);
#endif
#ifdef CONFIG_DIFFTEST_SYNCCUSTOMMFLUSHPWREVENT
  custom_mflushpwr_checker = new CustomMflushpwrChecker(
      [this]() -> DifftestSyncCustomMflushpwrEvent & { return dut->sync_custom_mflushpwr; }, state, proxy);
  checkers.push_back(custom_mflushpwr_checker);
#endif

#ifdef CONFIG_DIFFTEST_AMUCTRLEVENT
  for (int i = 0; i < CONFIG_DIFF_AMU_CTRL_WIDTH; i++) {
    checkers.push_back(
        new AmuCtrlRecorder([this, i]() -> DifftestAmuCtrlEvent & { return dut->amu_ctrl[i]; }, state, proxy));
  }
  checkers.push_back(new AmuCtrlChecker(state, proxy));
  for (int i = 0; i < CONFIG_DIFF_AMU_FINISH_WIDTH; i++) {
    checkers.push_back(
        new AmuExecRecorder([this, i]() -> DifftestAmuFinishEvent & { return dut->amu_finish[i]; }, state, proxy));
  }
  checkers.push_back(new AmuExecChecker(state, proxy));
#endif // CONFIG_DIFFTEST_AMUCTRLEVENT

#ifdef CONFIG_DIFFTEST_MSYNCEVENT
  for (int i = 0; i < CONFIG_DIFF_MSYNC_WIDTH; i++) {
    checkers.push_back(new MsyncRecorder([this, i]() -> DifftestMsyncEvent & { return dut->msync[i]; }, state, proxy));
  }
#endif // CONFIG_DIFFTEST_MSYNCEVENT

  arch_event_checker = new ArchEventChecker([this]() -> DifftestArchEvent & { return dut->event; }, state, proxy,
                                            [this]() -> const DiffTestRegState & { return dut->regs; });

  std::vector<DiffTestChecker *> inst_op_checkers;
#if defined(CONFIG_DIFFTEST_LOADEVENT) && defined(CONFIG_DIFFTEST_SQUASH)
  load_squash_checker = new LoadSquashChecker(state, proxy, [this]() -> const DiffTestState & { return *dut; });
  inst_op_checkers.push_back(load_squash_checker);
#endif // CONFIG_DIFFTEST_LOADEVENT && CONFIG_DIFFTEST_SQUASH
#ifdef CONFIG_DIFFTEST_STOREEVENT
  store_checker = new StoreChecker(state, proxy);
  inst_op_checkers.push_back(store_checker);
#endif // CONFIG_DIFFTEST_STOREEVENT
#ifdef CONFIG_DIFFTEST_MSYNCEVENT
  inst_op_checkers.push_back(new MsyncChecker(state, proxy));
#endif // CONFIG_DIFFTEST_MSYNCEVENT

  for (int i = 0; i < CONFIG_DIFF_COMMIT_WIDTH; i++) {
    std::vector<DiffTestChecker *> tmp_checkers = inst_op_checkers;
#if defined(CONFIG_DIFFTEST_LOADEVENT) && !defined(CONFIG_DIFFTEST_SQUASH)
    tmp_checkers.push_back(load_checker[i]);
#endif // CONFIG_DIFFTEST_LOADEVENT && CONFIG_DIFFTEST_SQUASH
    instr_commit_checker[i] =
        new InstrCommitChecker([this, i]() -> DifftestInstrCommit & { return dut->commit[i]; }, state, proxy, i,
                               [this]() -> const DiffTestState & { return *dut; }, tmp_checkers);
  }
}

void Difftest::update_nemuproxy(int coreid, size_t ram_size = 0) {
  proxy = new REF_PROXY(coreid, ram_size);
#ifdef CONFIG_DIFFTEST_FORK
  proxy->set_exec_mode(REF_EXEC_FAST);
#endif

#ifdef CONFIG_DIFFTEST_AMUCTRLEVENT
  mma_verifier = new MmaVerifier();
  mma_verifier->start();
#endif // CONFIG_DIFFTEST_AMUCTRLEVENT

#ifdef CONFIG_DIFFTEST_REPLAY
  proxy_reg_ss = (uint8_t *)malloc(sizeof(ref_state_t));
#endif // CONFIG_DIFFTEST_REPLAY
}

#ifdef CONFIG_DIFFTEST_REPLAY
bool Difftest::can_replay() {
  auto info = dut->trace_info;
  return info.valid && !info.in_replay && info.trace_size > 1;
}

bool Difftest::in_replay_range() {
  auto info = dut->trace_info;
  if (!info.valid || !info.in_replay || info.trace_size > 1)
    return false;
  int pos = info.trace_head;
  int head = replay_status.trace_head;
  int tail = (head + replay_status.trace_size - 1) % CONFIG_DIFFTEST_REPLAY_SIZE;
  if (tail < head) { // consider ring queue
    return (pos <= tail) || (pos >= head);
  } else {
    return (pos >= head) && (pos <= tail);
  }
}

void Difftest::replay_snapshot() {
  memcpy(state_ss, state, sizeof(DiffState));
  memcpy(proxy_reg_ss, &proxy->state, sizeof(ref_state_t));
  proxy->ref_csrcpy(squash_csr_buf, REF_TO_DUT);
  proxy->ref_store_log_reset();
  proxy->set_store_log(true);
  goldenmem_store_log_reset();
  goldenmem_set_store_log(true);
#ifdef CONFIG_DIFFTEST_MATRIXSTOREEVENT
  if (matrix_store_checker != nullptr) {
    matrix_store_checker->replay_snapshot();
  }
#endif // CONFIG_DIFFTEST_MATRIXSTOREEVENT
}

void Difftest::do_replay() {
  auto info = dut->trace_info;
  replay_status.in_replay = true;
  replay_status.trace_head = info.trace_head;
  replay_status.trace_size = info.trace_size;
  memcpy(state, state_ss, sizeof(DiffState));
  memcpy(&proxy->state, proxy_reg_ss, sizeof(ref_state_t));
  proxy->ref_regcpy(&proxy->state, DUT_TO_REF, false);
  proxy->ref_csrcpy(squash_csr_buf, DUT_TO_REF);
  proxy->ref_store_log_restore();
  goldenmem_store_log_restore();
#ifdef CONFIG_DIFFTEST_MATRIXSTOREEVENT
  if (matrix_store_checker != nullptr) {
    matrix_store_checker->replay_restore();
  }
#endif // CONFIG_DIFFTEST_MATRIXSTOREEVENT
  difftest_replay_head(info.trace_head);
  // clear buffered queue
#ifdef CONFIG_DIFFTEST_STOREEVENT
  while (!state->store_event_queue.empty())
    state->store_event_queue.pop();
#endif // CONFIG_DIFFTEST_STOREEVENT
#if defined(CONFIG_DIFFTEST_LOADEVENT) && defined(CONFIG_DIFFTEST_SQUASH)
  while (!state->load_event_queue.empty())
    state->load_event_queue.pop();
#endif
#ifdef CONFIG_DIFFTEST_AMUCTRLEVENT
  for (auto &entry: state->matrix_sw_rob) {
    if (entry.res != nullptr) {
      delete[] entry.res;
      entry.res = nullptr;
    }
  }
  state->matrix_sw_rob.clear();
#endif // CONFIG_DIFFTEST_AMUCTRLEVENT
#ifdef CONFIG_DIFFTEST_MSYNCEVENT
  while (!state->msync_event_queue.empty())
    state->msync_event_queue.pop();
#endif // CONFIG_DIFFTEST_MSYNCEVENT
}
#endif // CONFIG_DIFFTEST_REPLAY

#ifdef CONFIG_DIFFTEST_FORK
static void fork_clear_event_valids(DiffTestState &window);
#endif

int Difftest::step() {
#ifdef CONFIG_DIFFTEST_FORK
  if (fast_only) {
    return fast_only_step();
  }
  bool has_skip_window = false;
  for (const auto &commit : dut->commit) {
    if (!commit.valid || !commit.skip) continue;
    has_skip_window = true;
    ++fork_skip_commit_count;
    fork_skip_instr_count += 1 + commit.nFused;
  }
  if (has_skip_window) {
    ++fork_skip_window_count;
    if (!fork_allow_skip) {
      ++fork_skip_blocked_window_count;
    }
  }
  if (fork_promoted_result != nullptr) {
    return fork_promoted_step();
  }
  if (int ret = drain_fork(false)) {
    return ret;
  }
  if (fork_promoted_result != nullptr) {
    return fork_promoted_step();
  }
#endif // CONFIG_DIFFTEST_FORK
#ifdef CONFIG_DIFFTEST_REPLAY
  static int replay_step = 0;
  if (replay_status.in_replay) {
    if (!in_replay_range()) {
      return 0;
    } else {
      replay_step++;
      if (replay_step > replay_status.trace_size) {
        Info("*** DUT run out of replay range, failed to get error location ***\n");
        return 1;
      }
    }
  }
  bool canReplay = can_replay();
  if (canReplay) {
    replay_snapshot();
  } else {
    proxy->set_store_log(false);
    goldenmem_set_store_log(false);
  }
  int ret = check_all();
  if (ret && canReplay) {
    Info("\n**** Start replay for more accurate error location ****\n");
    do_replay();
    return 0;
  } else {
    return ret;
  }
#else
#ifdef CONFIG_DIFFTEST_FORK
  if (state->has_commit && fork_window_eligible()) {
    const uint32_t window_instr = fork_window_instr();
    if (window_instr != 0 || fork_window_has_event(*dut)) {
      fork_group.push_back({*dut, window_instr});
      // The DPIC ring reuses DiffTestState entries. The child consumes the
      // snapshot, so retire the live commit probes here just as check_all()
      // would have done in the synchronous path.
      for (auto &commit : dut->commit) {
        commit.valid = 0;
      }
      fork_clear_event_valids(*dut);
      fork_window_sizes.push_back(window_instr);
      ++fork_window_count;
      fork_window_instr_count += window_instr;
      if (!fork_interval_started) {
        fork_interval_started = true;
        fork_interval_start = std::chrono::steady_clock::now();
        if (fork_interval_debug) {
          fprintf(stderr, "fork interval started at instr=%lu windows=%lu\n",
                  static_cast<unsigned long>(dut->trap.instrCnt),
                  static_cast<unsigned long>(fork_window_count));
        }
      }
      const bool interval_reached = fork_interval_reached();
      if (interval_reached && fork_interval_debug && fork_interval_debug_count++ < 8) {
        fprintf(stderr, "fork interval reached at instr=%lu windows=%lu\n",
                static_cast<unsigned long>(dut->trap.instrCnt), static_cast<unsigned long>(fork_window_count));
      }
      if (!interval_reached) {
        return DiffTestChecker::STATE_OK;
      }
      return fork_group_step();
    }
  }
  if (!fork_group.empty() && fork_interval_reached()) {
    if (int ret = fork_group_step()) {
      return ret;
    }
  }
  // A synchronous window may update REF through skip/special-event handling.
  // Make every preceding speculative group authoritative before it observes
  // or mutates the shared REF state.
  if (int ret = drain_fork(true)) {
    return ret;
  }
  if (fork_promoted_result != nullptr) {
    return fork_promoted_step();
  }
#endif // CONFIG_DIFFTEST_FORK
#ifdef CONFIG_DIFFTEST_FORK
  proxy->set_exec_mode(REF_EXEC_SLOW);
#endif
  int ret = check_all();
#ifdef CONFIG_DIFFTEST_FORK
  proxy->set_exec_mode(REF_EXEC_FAST);
#endif // CONFIG_DIFFTEST_FORK
#ifdef CONFIG_DIFFTEST_AMUCTRLEVENT
  if (mma_verifier) {
    if (ret) { // find error, wait for mma verification to complete
      mma_verifier->flush();
    }
    if (mma_verifier->has_mma_verification_error()) {
      auto buffer = mma_verifier->get_error_buffer();
      Info("MMA verification error detected at pc = 0x%lx.\n", buffer->amu_event.pc);
      Info("------ DUT Result ------\n");
      for (int i = 0; i < buffer->amu_event.mtilem; i++) {
        for (int j = 0; j < buffer->amu_event.mtilen; j++) {
          Info("%08x ", ((uint32_t *)(buffer->dut_result))[i * buffer->amu_event.mtilen + j]);
        }
        Info("\n");
      }
      Info("------ REF Result ------\n");
      for (int i = 0; i < buffer->amu_event.mtilem; i++) {
        for (int j = 0; j < buffer->amu_event.mtilen; j++) {
          Info("%08x ", ((uint32_t *)(buffer->src3))[i * buffer->amu_event.mtilen + j]);
        }
        Info("\n");
      }
      display();
      dut->trap.pc = buffer->amu_event.pc;
      ret = STATE_BADTRAP;
      mma_verifier->stop();
    }
  }
#endif // CONFIG_DIFFTEST_AMUCTRLEVENT
  return ret;
#endif // CONFIG_DIFFTEST_REPLAY
}

#ifdef CONFIG_DIFFTEST_FORK
int Difftest::fork_commit_stamp() const {
#ifdef CONFIG_DIFFTEST_SQUASH
  return state->commit_stamp;
#else
  return 0;
#endif
}

uint32_t Difftest::fork_window_instr() const {
  if (dut->event.valid) return 0;
  uint32_t count = 0;
  for (int i = 0; i < CONFIG_DIFF_COMMIT_WIDTH; ++i) {
    if (dut->commit[i].valid) {
      count += 1 + dut->commit[i].nFused;
    }
  }
  return count;
}

bool Difftest::fork_window_has_event(const DiffTestState &window) const {
  if (window.event.valid) return true;
#ifdef CONFIG_DIFFTEST_LRSCEVENT
  if (window.lrsc.valid) return true;
#endif
#ifdef CONFIG_DIFFTEST_SYNCAIAEVENT
  if (window.sync_aia.valid) return true;
#endif
#ifdef CONFIG_DIFFTEST_NONREGINTERRUPTPENDINGEVENT
  if (window.non_reg_interrupt_pending.valid) return true;
#endif
#ifdef CONFIG_DIFFTEST_MHPMEVENTOVERFLOWEVENT
  if (window.mhpmevent_overflow.valid) return true;
#endif
#ifdef CONFIG_DIFFTEST_CRITICALERROREVENT
  if (window.critical_error.valid) return true;
#endif
#ifdef CONFIG_DIFFTEST_SYNCCUSTOMMFLUSHPWREVENT
  if (window.sync_custom_mflushpwr.valid) return true;
#endif
#ifdef CONFIG_DIFFTEST_DEBUGMODE
  if (window.dmregs.debugMode != 0) return true;
#endif
  return false;
}

static void fork_clear_event_valids(DiffTestState &window) {
  window.event.valid = 0;
#ifdef CONFIG_DIFFTEST_LRSCEVENT
  window.lrsc.valid = 0;
#endif
#ifdef CONFIG_DIFFTEST_SYNCAIAEVENT
  window.sync_aia.valid = 0;
#endif
#ifdef CONFIG_DIFFTEST_NONREGINTERRUPTPENDINGEVENT
  window.non_reg_interrupt_pending.valid = 0;
#endif
#ifdef CONFIG_DIFFTEST_MHPMEVENTOVERFLOWEVENT
  window.mhpmevent_overflow.valid = 0;
#endif
#ifdef CONFIG_DIFFTEST_CRITICALERROREVENT
  window.critical_error.valid = 0;
#endif
#ifdef CONFIG_DIFFTEST_SYNCCUSTOMMFLUSHPWREVENT
  window.sync_custom_mflushpwr.valid = 0;
#endif
}

bool Difftest::fork_window_eligible() const {
#ifdef CONFIG_DIFFTEST_DEBUGMODE
  if (dut->dmregs.debugMode != 0) return false;
#endif

  bool has_commit = false;
  for (int i = 0; i < CONFIG_DIFF_COMMIT_WIDTH; ++i) {
    const auto &commit = dut->commit[i];
    if (!commit.valid) continue;
    has_commit = true;
    if (commit.special != 0) return false;
    if (commit.skip) {
#if defined(CONFIG_DIFFTEST_STOREEVENT) || defined(CONFIG_DIFFTEST_LOADEVENT)
      return false;
#else
      if (!fork_allow_skip) return false;
#endif
    }
  }
  return has_commit || fork_window_has_event(*dut);
}

int Difftest::fork_fast_apply_events(DiffTestState &window, bool &arch_event_consumes_commit,
                                     bool check_critical_error) {
  DiffTestState *saved_dut = dut;
  dut = &window;
  arch_event_consumes_commit = window.event.valid;
  int ret = DiffTestChecker::STATE_OK;

  // Keep the same ordering as check_all(): synchronization probes are applied
  // before ArchEvent, and ArchEvent takes precedence over instruction commits.
  auto apply = [&ret](DiffTestChecker *checker) {
    if (ret == DiffTestChecker::STATE_OK && checker != nullptr) {
      ret = checker->step();
    }
  };
#ifdef CONFIG_DIFFTEST_LRSCEVENT
  apply(lrsc_checker);
#endif
#ifdef CONFIG_DIFFTEST_NONREGINTERRUPTPENDINGEVENT
  apply(non_reg_interrupt_pending_checker);
#endif
#ifdef CONFIG_DIFFTEST_MHPMEVENTOVERFLOWEVENT
  apply(mhpmevent_overflow_checker);
#endif
#ifdef CONFIG_DIFFTEST_CRITICALERROREVENT
  if (check_critical_error) {
    apply(critical_error_checker);
  } else if (window.critical_error.valid) {
    // Preserve the REF-side event without making fast-only mode a checker.
    proxy->raise_critical_error();
    window.critical_error.valid = 0;
  }
#endif
#ifdef CONFIG_DIFFTEST_SYNCAIAEVENT
  apply(aia_checker);
#endif
#ifdef CONFIG_DIFFTEST_SYNCCUSTOMMFLUSHPWREVENT
  apply(custom_mflushpwr_checker);
#endif
  apply(arch_event_checker);

  dut = saved_dut;
  return ret;
}

int Difftest::fast_only_step() {
  proxy->set_exec_mode(REF_EXEC_FAST);
  state->cycle_count = dut->trap.cycleCnt;
  state->has_progress = false;

  // Fast-only mode bypasses check_all(), including FirstInstrCommitChecker.
  // Initialize the REF from the first transport snapshot before any event
  // handler or ref_exec() can make NEMU fetch from its reset-only state.  Do
  // not initialize on an event-only transport step: the regular checker path
  // enables DiffTest at the first architectural commit, and event-only steps
  // before that point must not establish a speculative REF starting point.
  if (!fast_only_initialized) {
    bool has_commit = false;
    for (int i = 0; i < CONFIG_DIFF_COMMIT_WIDTH; ++i) {
      if (dut->commit[i].valid) {
        has_commit = true;
        break;
      }
    }
    if (has_commit) {
      proxy->flash_init((const uint8_t *)flash_dev.base, flash_dev.img_size, flash_dev.img_path);
      simMemory->clone_on_demand(
          [this](uint64_t offset, void *src, size_t n) {
            uint64_t dest_addr = PMEM_BASE + offset;
            proxy->mem_init(dest_addr, src, n, DUT_TO_REF);
          },
          true);
      proxy->regcpy(&dut->regs, FIRST_INST_ADDRESS);
      state->has_commit = true;
      fast_only_initialized = true;
    }
  }

  bool arch_event_consumes_commit = false;
  if (fast_only_initialized && fork_window_has_event(*dut)) {
    ++fast_only_event_count;
    const bool print_event = fast_only_debug && fast_only_debug_events++ < 8;
    if (print_event) {
      fprintf(stderr,
              "fast-only event #%lu: dut=%p instr=%lu cycle=%lu arch=%u lrsc=%u aia=%u nonreg=%u mhpm=%u "
              "critical=%u mflush=%u debug=%lu commit0=%u\n",
              static_cast<unsigned long>(fast_only_event_count), static_cast<void *>(dut),
              static_cast<unsigned long>(dut->trap.instrCnt),
              static_cast<unsigned long>(dut->trap.cycleCnt), dut->event.valid, dut->lrsc.valid,
              dut->sync_aia.valid, dut->non_reg_interrupt_pending.valid, dut->mhpmevent_overflow.valid,
              dut->critical_error.valid,
              dut->sync_custom_mflushpwr.valid, static_cast<unsigned long>(dut->dmregs.debugMode),
              dut->commit[0].valid);
      if (dut->non_reg_interrupt_pending.valid) {
        fprintf(stderr,
                "fast-only nonreg: meip=%u mtip=%u msip=%u seip=%u stip=%u vseip=%u vstip=%u aia_meip=%u "
                "aia_seip=%u lcofi=%u\n",
                dut->non_reg_interrupt_pending.platformIRPMeip, dut->non_reg_interrupt_pending.platformIRPMtip,
                dut->non_reg_interrupt_pending.platformIRPMsip, dut->non_reg_interrupt_pending.platformIRPSeip,
                dut->non_reg_interrupt_pending.platformIRPStip, dut->non_reg_interrupt_pending.platformIRPVseip,
                dut->non_reg_interrupt_pending.platformIRPVstip, dut->non_reg_interrupt_pending.fromAIAMeip,
                dut->non_reg_interrupt_pending.fromAIASeip,
                dut->non_reg_interrupt_pending.localCounterOverflowInterruptReq);
      }
      if (dut->event.valid) {
        fprintf(stderr,
                "fast-only arch event: interrupt=%u exception=%u pc=0x%lx inst=0x%x nmi=%u hvictl=%u\n",
                dut->event.interrupt, dut->event.exception, static_cast<unsigned long>(dut->event.exceptionPC),
                dut->event.exceptionInst, dut->event.hasNMI, dut->event.virtualInterruptIsHvictlInject);
      }
      fflush(stderr);
    }
    if (int ret = fork_fast_apply_events(*dut, arch_event_consumes_commit, false)) {
      return ret;
    }
    if (print_event) {
      fprintf(stderr,
              "fast-only event #%lu applied: arch=%u lrsc=%u aia=%u nonreg=%u mhpm=%u critical=%u mflush=%u\n",
              static_cast<unsigned long>(fast_only_event_count), dut->event.valid, dut->lrsc.valid,
              dut->sync_aia.valid, dut->non_reg_interrupt_pending.valid, dut->mhpmevent_overflow.valid,
              dut->critical_error.valid, dut->sync_custom_mflushpwr.valid);
      fflush(stderr);
    }
  }

  if (!fast_only_initialized) {
    // Before the first commit, DiffTest has not established a REF starting
    // point.  Consume transport probes without applying them to NEMU so the
    // ring entry cannot replay stale valid bits when it is reused.
    fork_clear_event_valids(*dut);
  }

  if (fast_only_initialized && !arch_event_consumes_commit) {
    uint64_t pending_instr = 0;
    uint32_t committed_instr = 0;
    for (int i = 0; i < CONFIG_DIFF_COMMIT_WIDTH; ++i) {
      const auto &commit = dut->commit[i];
      if (!commit.valid) continue;
      const uint32_t instr_count = 1 + commit.nFused;
      committed_instr += instr_count;
      if (commit.skip) {
        if (pending_instr != 0) {
          proxy->ref_exec(pending_instr);
          fast_only_ref_exec_instr += pending_instr;
          pending_instr = 0;
        }
        proxy->skip_one(commit.isRVC, commit.rfwen && commit.wdest != 0, commit.fpwen, commit.vecwen,
                        commit.wdest, get_commit_data(dut, i));
        ++fast_only_skip_count;
      } else {
        pending_instr += instr_count;
      }
    }
    if (pending_instr != 0) {
      proxy->ref_exec(pending_instr);
      fast_only_ref_exec_instr += pending_instr;
    }
    if (committed_instr != 0) {
      state->has_progress = true;
      state->record_group(dut->commit[0].pc, committed_instr);
      state->last_commit_cycle = dut->trap.cycleCnt;
    }
  }

  state->cycle_count = dut->trap.cycleCnt;
  for (int i = 0; i < CONFIG_DIFF_COMMIT_WIDTH; ++i) {
    dut->commit[i].valid = 0;
  }
  return DiffTestChecker::STATE_OK;
}

int Difftest::fork_group_step() {
  assert(!fork_group.empty());
  while (fork_pending_groups.size() >= fork_max_outstanding) {
    bool released = false;
    if (int ret = fork_release_front(true, released)) {
      return ret;
    }
  }

  const uint64_t group_id = ++fork_group_count;
  uint64_t total_instr = 0;
#ifdef CONFIG_DIFFTEST_SQUASH
  uint64_t checked_instr = 0;
#endif
  for (const auto &window : fork_group) {
    total_instr += window.instr_count;
#ifdef CONFIG_DIFFTEST_SQUASH
    for (const auto &commit : window.dut.commit) {
      if (!window.dut.event.valid && commit.valid && !commit.skip) checked_instr += 1 + commit.nFused;
    }
#endif
  }
  if (fork_allow_skip && fork_skip_window_count != 0 && fork_skip_window_count <= 8) {
    for (const auto &window : fork_group) {
      for (const auto &commit : window.dut.commit) {
        if (commit.valid && commit.skip) {
          Info("fork DiffTest skip in group %lu (instr=%lu pc=0x%lx)\n",
               static_cast<unsigned long>(group_id), static_cast<unsigned long>(total_instr), proxy->state.pc);
          break;
        }
      }
    }
  }

  // The parent remains the fast owner.  The child switches the same loaded SO
  // to the precise checker mode after fork, so no cross-SO state migration is
  // needed at either boundary.
  proxy->set_exec_mode(REF_EXEC_FAST);
  proxy->sync();
  proxy->ref_store_log_reset();
  proxy->set_store_log(true);

  static const unsigned long child_delay_us = []() {
    const char *value = getenv("DIFFTEST_FORK_CHILD_DELAY_US");
    return value ? strtoul(value, nullptr, 0) : 0;
  }();

  auto *shared_result = static_cast<ForkSharedResult *>(
      mmap(nullptr, sizeof(ForkSharedResult), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0));
  if (shared_result == MAP_FAILED) {
    Info("fork DiffTest mmap failed: %s\n", strerror(errno));
    proxy->set_store_log(false);
    proxy->ref_store_log_reset();
    return DiffTestChecker::STATE_ERROR;
  }
  memset(shared_result, 0, sizeof(*shared_result));
  shared_result->failed_index = -1;

  const pid_t child = fork();
  if (child < 0) {
    Info("fork DiffTest fork failed: %s\n", strerror(errno));
    munmap(shared_result, sizeof(ForkSharedResult));
    proxy->set_store_log(false);
    proxy->ref_store_log_reset();
    return DiffTestChecker::STATE_ERROR;
  }

  if (child == 0) {
    fork_worker_process = true;
    proxy->set_exec_mode(REF_EXEC_SLOW);
    if (proxy->ref_flush_state) {
      proxy->ref_flush_state();
    }
    if (child_delay_us != 0) {
      usleep(child_delay_us);
    }
    shared_result->ret = DiffTestChecker::STATE_OK;
    for (size_t i = 0; i < fork_group.size(); ++i) {
      dut = &fork_group[i].dut;
      shared_result->ret = check_all();
      if (shared_result->ret != DiffTestChecker::STATE_OK) {
        shared_result->failed_index = static_cast<int>(i);
        proxy->display(dut);
        fflush(stdout);
        fflush(stderr);
        break;
      }
    }
    proxy->sync();
    shared_result->state_hash = proxy->state_hash();
    shared_result->commit_stamp = fork_commit_stamp();
    fork_store(&shared_result->ready, 1);

    // Keep the child alive until the parent has compared the endpoints.  On
    // a mismatch the child is promoted in place, so its private COW memory
    // and checker state become the authoritative owner without migration.
    while (true) {
      const int action = fork_load(&shared_result->action);
      if (action == FORK_CHILD_WAIT) {
        sched_yield();
        continue;
      }
      if (action != FORK_CHILD_PROMOTE) {
        _exit(0);
      }
      break;
    }

    // The owner process starts a fresh fast/slow epoch from the state it just
    // checked.  It must not retain the parent's pending fork queue, which
    // belongs to the abandoned fast process.
    fork_pending_groups.clear();
    fork_group.clear();
    fork_promoted_pid = -1;
    fork_promoted_result = nullptr;
    fork_group_count = shared_result->command_group_count;
    fork_skip_window_count = shared_result->command_skip_window_count;
    fork_skip_commit_count = shared_result->command_skip_commit_count;
    fork_skip_instr_count = shared_result->command_skip_instr_count;
    fork_skip_blocked_window_count = shared_result->command_skip_blocked_window_count;
    fork_rollback_count = shared_result->command_rollback_count;
    fork_mismatch_count = shared_result->command_mismatch_count;
    fork_group_release_count = shared_result->command_release_count;
    fork_promotion_count = shared_result->command_promotion_count;
    fork_window_count = shared_result->command_window_count;
    fork_window_instr_count = shared_result->command_window_instr;
    fork_peak_outstanding = shared_result->command_peak_outstanding;
    proxy->set_exec_mode(REF_EXEC_FAST);

    uint64_t command_seq = 0;
    while (fork_load(&shared_result->action) == FORK_CHILD_PROMOTE) {
      const uint64_t requested = fork_load(&shared_result->command_seq);
      if (requested == command_seq) {
        sched_yield();
        continue;
      }
      command_seq = requested;
      dut = &shared_result->command_dut;
      proxy->sync();
      shared_result->command_commit_valid = dut->commit[0].valid;
      shared_result->command_skip = dut->commit[0].skip;
      shared_result->command_fused = dut->commit[0].nFused;
      shared_result->command_dut_pc = dut->commit[0].valid ? dut->commit[0].pc : dut->trap.pc;
      shared_result->command_ref_pc_before = proxy->state.pc;
      if (shared_result->command_fast_catchup) {
        proxy->set_exec_mode(REF_EXEC_FAST);
        proxy->set_store_log(false);
        proxy->ref_store_log_reset();
        shared_result->command_ret = fork_promoted_fast_catchup_step();
      } else {
        shared_result->command_ret = fork_promoted_owner_step();
      }
      proxy->sync();
      shared_result->command_ref_pc_after = proxy->state.pc;
      shared_result->command_trap = get_trap_code();
      if (shared_result->command_ret == DiffTestChecker::STATE_DIFF) {
        display();
        fflush(stdout);
        fflush(stderr);
      }
      shared_result->command_group_count = fork_group_count;
      shared_result->command_skip_window_count = fork_skip_window_count;
      shared_result->command_skip_commit_count = fork_skip_commit_count;
      shared_result->command_skip_instr_count = fork_skip_instr_count;
      shared_result->command_skip_blocked_window_count = fork_skip_blocked_window_count;
      shared_result->command_rollback_count = fork_rollback_count;
      shared_result->command_mismatch_count = fork_mismatch_count;
      shared_result->command_release_count = fork_group_release_count;
      shared_result->command_promotion_count = fork_promotion_count;
      shared_result->command_window_count = fork_window_count;
      shared_result->command_window_instr = fork_window_instr_count;
      shared_result->command_peak_outstanding = fork_peak_outstanding;
      fork_store(&shared_result->command_ack, command_seq);
    }
    _exit(0);
  }

  // Execute ordinary stretches in one batch, but apply skip commits at their
  // exact instruction positions.  A blind ref_exec(total_instr) would execute
  // a skipped MMIO instruction again and make the endpoint diverge from the
  // authoritative child.
  uint64_t pending_instr = 0;
  for (const auto &window : fork_group) {
    bool arch_event_consumes_commit = false;
    if (fork_window_has_event(window.dut)) {
      if (pending_instr != 0) {
        proxy->ref_exec(pending_instr);
        pending_instr = 0;
      }
      // Keep the original snapshot intact.  The child already owns its own
      // copy, and a later promotion may need this parent-side copy to replay
      // the exact event while catching up abandoned groups.
      DiffTestState fast_window = window.dut;
      if (int ret = fork_fast_apply_events(fast_window, arch_event_consumes_commit)) {
        proxy->set_store_log(false);
        proxy->ref_store_log_reset();
        return ret;
      }
      if (arch_event_consumes_commit) continue;
    }

    bool has_skip = false;
    for (const auto &commit : window.dut.commit) {
      if (commit.valid && commit.skip) {
        has_skip = true;
        break;
      }
    }
    if (!has_skip) {
      pending_instr += window.instr_count;
      continue;
    }

    if (pending_instr != 0) {
      proxy->ref_exec(pending_instr);
      pending_instr = 0;
    }
    for (int i = 0; i < CONFIG_DIFF_COMMIT_WIDTH; ++i) {
      const auto &commit = window.dut.commit[i];
      if (!commit.valid) continue;
      if (commit.skip) {
        proxy->skip_one(commit.isRVC, commit.rfwen && commit.wdest != 0, commit.fpwen, commit.vecwen,
                        commit.wdest, get_commit_data(&window.dut, i));
      } else {
        proxy->ref_exec(1 + commit.nFused);
      }
    }
  }
  if (pending_instr != 0) {
    proxy->ref_exec(pending_instr);
  }
  proxy->sync();
#ifdef CONFIG_DIFFTEST_SQUASH
  state->commit_stamp = (state->commit_stamp + checked_instr) % CONFIG_DIFFTEST_SQUASH_STAMPSIZE;
#endif
  const int parent_commit_stamp = fork_commit_stamp();
  DifftestStateHash parent_hash = proxy->state_hash();
  proxy->set_store_log(false);
  proxy->ref_store_log_reset();
  state->has_progress = true;
  state->last_commit_cycle = fork_group.back().dut.trap.cycleCnt;
  state->cycle_count = fork_group.back().dut.trap.cycleCnt;
  state->record_group(proxy->state.pc, total_instr);

  static const long corrupt_store_group = []() {
    const char *value = getenv("DIFFTEST_FORK_CORRUPT_STORE_GROUP");
    return value ? strtol(value, nullptr, 0) : -1;
  }();
  if (corrupt_store_group == static_cast<long>(group_id)) {
    // Test the endpoint promotion path without copying or mutating a store
    // trace in the parent.  The child still owns the authoritative state.
    parent_hash.state_lo ^= 1;
  }

  fork_pending_groups.push_back(
      {group_id, child, shared_result, parent_commit_stamp, parent_hash, std::move(fork_group)});
  fork_peak_outstanding = std::max(fork_peak_outstanding, fork_pending_groups.size());
  fork_group.clear();
  const auto now = std::chrono::steady_clock::now();
  if (fork_interval_started) {
    const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - fork_interval_start);
    fork_intervals_ms.push_back(static_cast<uint64_t>(elapsed.count()));
  }
  fork_interval_start = now;
  fork_interval_started = true;
  return drain_fork(false);
}

int Difftest::fork_release_front(bool block, bool &released) {
  released = false;
  if (fork_pending_groups.empty()) return DiffTestChecker::STATE_OK;

  auto &group = fork_pending_groups.front();
  int child_status = 0;
  while (!fork_load(&group.result->ready)) {
    const pid_t waited = waitpid(group.pid, &child_status, WNOHANG);
    if (waited == group.pid) {
      Info("fork DiffTest child failed for group %lu\n", static_cast<unsigned long>(group.id));
      stop_child(group);
      fork_pending_groups.pop_front();
      return DiffTestChecker::STATE_ERROR;
    }
    if (!block) return DiffTestChecker::STATE_OK;
    usleep(1000);
  }

  auto *result = group.result;

  if (result->ret != DiffTestChecker::STATE_OK) {
    Info("fork DiffTest child check failed for group %lu (ret=%d failed_index=%d)\n",
         static_cast<unsigned long>(group.id), result->ret, result->failed_index);
    fork_store(&result->action, static_cast<int>(FORK_CHILD_STOP));
    waitpid(group.pid, &child_status, 0);
    const int ret = result->ret;
    munmap(result, sizeof(ForkSharedResult));
    fork_pending_groups.pop_front();
    return ret;
  }

  const bool state_hash_matches = memcmp(&result->state_hash, &group.parent_hash, sizeof(DifftestStateHash)) == 0;
  const bool stamp_matches = result->commit_stamp == group.parent_commit_stamp;
  if (state_hash_matches && stamp_matches) {
    fork_store(&result->action, static_cast<int>(FORK_CHILD_STOP));
    waitpid(group.pid, &child_status, 0);
    munmap(result, sizeof(ForkSharedResult));
    fork_pending_groups.pop_front();
    ++fork_group_release_count;
    released = true;
    return DiffTestChecker::STATE_OK;
  }

  Info("fork DiffTest endpoint hash mismatch for group %lu (hash=%d stamp=%d)\n",
       static_cast<unsigned long>(group.id), state_hash_matches, stamp_matches);
  ++fork_mismatch_count;
  ++fork_rollback_count;
  const uint64_t promotion_group_id = group.id;

  // The slow child already contains the complete authoritative NEMU state at
  // this endpoint.  Promote that process instead of copying registers or
  // memory back into the speculative parent.  Windows that the fast parent
  // had already submitted are replayed through a shared command slot so the
  // promoted child catches up before serving new DUT steps.
  std::vector<DifftestForkWindow> catchup_windows;
  for (size_t i = 1; i < fork_pending_groups.size(); ++i) {
    catchup_windows.insert(catchup_windows.end(), fork_pending_groups[i].windows.begin(),
                           fork_pending_groups[i].windows.end());
  }
  catchup_windows.insert(catchup_windows.end(), fork_group.begin(), fork_group.end());

  fork_store(&result->action, static_cast<int>(FORK_CHILD_PROMOTE));
  result->command_group_count = fork_group_count;
  result->command_skip_window_count = fork_skip_window_count;
  result->command_skip_commit_count = fork_skip_commit_count;
  result->command_skip_instr_count = fork_skip_instr_count;
  result->command_skip_blocked_window_count = fork_skip_blocked_window_count;
  result->command_rollback_count = fork_rollback_count;
  result->command_mismatch_count = fork_mismatch_count;
  // The current group is released immediately after promotion is scheduled;
  // include it in the counters handed to the promoted owner.
  result->command_release_count = fork_group_release_count + 1;
  result->command_promotion_count = fork_promotion_count + 1;
  result->command_window_count = fork_window_count;
  result->command_window_instr = fork_window_instr_count;
  result->command_peak_outstanding = fork_peak_outstanding;
  fork_promoted_pid = group.pid;
  fork_promoted_result = result;
  ++fork_promotion_count;

  for (size_t i = 1; i < fork_pending_groups.size(); ++i) {
    stop_child(fork_pending_groups[i]);
  }
  fork_pending_groups.clear();
  fork_group.clear();
  ++fork_group_release_count;

  for (const auto &window : catchup_windows) {
    if (int ret = fork_promoted_submit(window.dut, true)) {
      Info("fork DiffTest promoted child rejected fast catch-up window in group %lu\n",
           static_cast<unsigned long>(promotion_group_id));
      return ret;
    }
  }
  released = true;
  return DiffTestChecker::STATE_OK;
}

int Difftest::fork_promoted_submit(const DiffTestState &snapshot, bool fast_catchup) {
  if (fork_promoted_result == nullptr || fork_promoted_pid <= 0) {
    Info("fork DiffTest promoted child is unavailable\n");
    return DiffTestChecker::STATE_ERROR;
  }

  auto *result = fork_promoted_result;
  const uint64_t sequence = ++fork_promoted_sequence;
  result->command_dut = snapshot;
  result->command_fast_catchup = fast_catchup;
  fork_store(&result->command_seq, sequence);
  while (fork_load(&result->command_ack) != sequence) {
    int status = 0;
    if (waitpid(fork_promoted_pid, &status, WNOHANG) == fork_promoted_pid) {
      Info("fork DiffTest promoted child exited while processing sequence %lu\n",
           static_cast<unsigned long>(sequence));
      return DiffTestChecker::STATE_ERROR;
    }
    sched_yield();
  }

  const int ret = result->command_ret;
  fork_group_count = result->command_group_count;
  fork_skip_window_count = result->command_skip_window_count;
  fork_skip_commit_count = result->command_skip_commit_count;
  fork_skip_instr_count = result->command_skip_instr_count;
  fork_skip_blocked_window_count = result->command_skip_blocked_window_count;
  fork_rollback_count = result->command_rollback_count;
  fork_mismatch_count = result->command_mismatch_count;
  fork_group_release_count = result->command_release_count;
  fork_promotion_count = result->command_promotion_count;
  fork_window_count = result->command_window_count;
  fork_window_instr_count = result->command_window_instr;
  fork_peak_outstanding = result->command_peak_outstanding;
  if (ret == DiffTestChecker::STATE_TRAP) {
    state->has_trap = true;
    state->trap_code = result->command_trap;
  }
  if (ret == DiffTestChecker::STATE_DIFF || ret == DiffTestChecker::STATE_ERROR) {
    Info("fork DiffTest promoted child rejected sequence %lu (ret=%d valid=%u skip=%u fused=%u dut_pc=0x%lx "
         "ref_pc_before=0x%lx ref_pc_after=0x%lx)\n",
         static_cast<unsigned long>(sequence), ret, result->command_commit_valid, result->command_skip,
         result->command_fused, result->command_dut_pc, result->command_ref_pc_before, result->command_ref_pc_after);
    return DiffTestChecker::STATE_ERROR;
  }
  return ret;
}

int Difftest::fork_promoted_step() {
  // The parent still receives the DUT snapshot from the simulator.  The
  // promoted child owns the REF and all checker state, so only the plain DUT
  // record crosses the process boundary.
  return fork_promoted_submit(*dut, false);
}

int Difftest::fork_promoted_fast_catchup_step() {
  // Catch-up windows use the same fast event path as the original parent.
  // This matters after promotion: replaying only ref_exec(window_instr) would
  // silently drop LR/SC, interrupt-pending, AIA, PMU-overflow, and custom
  // synchronization probes that were present in the cached DUT window.
  const bool has_event = fork_window_has_event(*dut);
  bool arch_event_consumes_commit = false;
  if (has_event) {
    if (int ret = fork_fast_apply_events(*dut, arch_event_consumes_commit)) {
      return ret;
    }
  }

  const uint32_t window_instr = fork_window_instr();
  if (!arch_event_consumes_commit && window_instr != 0) {
    bool has_skip = false;
    for (const auto &commit : dut->commit) {
      if (commit.valid && commit.skip) {
        has_skip = true;
        break;
      }
    }

    if (!has_skip) {
      proxy->ref_exec(window_instr);
    } else {
      // Keep skipped MMIO/debug instructions out of the fast replay while
      // preserving their architectural writeback at the original position.
      for (int i = 0; i < CONFIG_DIFF_COMMIT_WIDTH; ++i) {
        const auto &commit = dut->commit[i];
        if (!commit.valid) continue;
        if (commit.skip) {
          proxy->skip_one(commit.isRVC, commit.rfwen && commit.wdest != 0, commit.fpwen, commit.vecwen,
                          commit.wdest, get_commit_data(dut, i));
        } else {
          proxy->ref_exec(1 + commit.nFused);
        }
      }
    }
  }

#ifdef CONFIG_DIFFTEST_SQUASH
  uint64_t checked_instr = 0;
  for (const auto &commit : dut->commit) {
    if (!arch_event_consumes_commit && commit.valid && !commit.skip) checked_instr += 1 + commit.nFused;
  }
  state->commit_stamp = (state->commit_stamp + checked_instr) % CONFIG_DIFFTEST_SQUASH_STAMPSIZE;
#endif

  proxy->sync();
  state->has_progress = true;
  state->last_commit_cycle = dut->trap.cycleCnt;
  state->cycle_count = dut->trap.cycleCnt;
  state->record_group(proxy->state.pc, window_instr);

  return DiffTestChecker::STATE_OK;
}

int Difftest::fork_promoted_owner_step() {
  if (fork_promoted_result != nullptr) {
    return fork_promoted_submit(*dut, true);
  }

  if (fork_window_eligible()) {
    const uint32_t window_instr = fork_window_instr();
    if (window_instr != 0 || fork_window_has_event(*dut)) {
      fork_group.push_back({*dut, window_instr});
      for (auto &commit : dut->commit) {
        commit.valid = 0;
      }
      fork_clear_event_valids(*dut);
      fork_window_sizes.push_back(window_instr);
      ++fork_window_count;
      fork_window_instr_count += window_instr;
      if (fork_group.size() < fork_group_size) {
        return DiffTestChecker::STATE_OK;
      }
      return fork_group_step();
    }
  }

  if (!fork_group.empty()) {
    if (int ret = fork_group_step()) {
      return ret;
    }
  }
  if (int ret = drain_fork(true)) {
    return ret;
  }
  if (fork_promoted_result != nullptr) {
    return fork_promoted_submit(*dut, true);
  }

  proxy->set_exec_mode(REF_EXEC_SLOW);
  const int ret = check_all();
  proxy->set_exec_mode(REF_EXEC_FAST);
  return ret;
}

int Difftest::drain_fork(bool block) {
  do {
    bool released = false;
    if (int ret = fork_release_front(block, released)) {
      return ret;
    }
    if (!released && !block) break;
  } while (!fork_pending_groups.empty());
  return DiffTestChecker::STATE_OK;
}

int Difftest::finish_fork() {
  if (!fork_group.empty()) {
    if (int ret = fork_group_step()) {
      return ret;
    }
  }
  return drain_fork(true);
}
#endif // CONFIG_DIFFTEST_FORK

inline int Difftest::check_all() {
  state->cycle_count = get_trap_event()->cycleCnt;
  state->has_progress = false;

  // normal checkers
  for (auto checker : checkers) {
    if (int ret = checker->step()) {
      return ret;
    }
  }

#ifdef DEBUG_MODE_DIFF
  // skip load & store insts in debug mode
  // for other insts copy inst content to ref's dummy debug module
  for (int i = 0; i < DIFFTEST_COMMIT_WIDTH; i++) {
    if (DEBUG_MEM_REGION(dut->commit[i].valid, dut->commit[i].pc))
      debug_mode_copy(dut->commit[i].pc, dut->commit[i].isRVC ? 2 : 4, dut->commit[i].inst);
  }
#endif

  num_commit = 0; // reset num_commit this cycle to 0
  if (dut->event.valid) {
    if (int ret = arch_event_checker->step()) {
      return ret;
    }
    dut->commit[0].valid = 0;
  } else {
#if !defined(BASIC_DIFFTEST_ONLY) && !defined(CONFIG_DIFFTEST_SQUASH)
    if (dut->commit[0].valid) {
      dut_commit_batch_pc = dut->commit[0].pc;
      ref_commit_batch_pc = proxy->state.pc;
      if (dut_commit_batch_pc != ref_commit_batch_pc) {
        pc_mismatch = true;
      }
    }
#endif
    for (int i = 0; i < CONFIG_DIFF_COMMIT_WIDTH; i++) {
      if (dut->commit[i].valid) {
        num_commit += 1 + dut->commit[i].nFused;
        if (int ret = instr_commit_checker[i]->step()) {
          return ret;
        }
      }
    }
  }

  if (int ret = update_delayed_writeback()) {
    return ret;
  }

  if (!state->has_progress) {
    return DiffTestChecker::STATE_OK;
  }

  proxy->sync();

  if (num_commit > 0) {
    state->record_group(dut->commit[0].pc, num_commit);
  }

  if (apply_delayed_writeback()) {
    return DiffTestChecker::STATE_DIFF;
  }

  if (proxy->compare(dut) || pc_mismatch) {
#ifdef FUZZING
    if (in_disambiguation_state()) {
      Info("Mismatch detected with a disambiguation state at pc = 0x%lx.\n", dut->trap.pc);
      state->raise_trap(STATE_FUZZ_COND);
      return DiffTestChecker::STATE_TRAP;
    }
#endif
#ifdef FUZZER_LIB
    stats.exit_code = SimExitCode::difftest;
#endif // FUZZER_LIB
    return DiffTestChecker::STATE_DIFF;
  }

  return DiffTestChecker::STATE_OK;
}

int Difftest::update_delayed_writeback() {
#define CHECK_DELAYED_WB(wb, delayed, n, regs_name)                                                \
  do {                                                                                             \
    for (int i = 0; i < n; i++) {                                                                  \
      auto delay = dut->wb + i;                                                                    \
      if (delay->valid) {                                                                          \
        delay->valid = false;                                                                      \
        if (!delayed[delay->address]) {                                                            \
          Info("Delayed writeback at %s has already been committed\n", regs_name[delay->address]); \
          return DiffTestChecker::STATE_DIFF;                                                      \
        }                                                                                          \
        if (delay->nack) {                                                                         \
          if (delayed[delay->address] > delay_wb_limit) {                                          \
            delayed[delay->address] -= 1;                                                          \
          }                                                                                        \
        } else {                                                                                   \
          delayed[delay->address] = 0;                                                             \
        }                                                                                          \
        state->has_progress = true;                                                                \
      }                                                                                            \
    }                                                                                              \
  } while (0);

#ifdef CONFIG_DIFFTEST_ARCHINTDELAYEDUPDATE
  CHECK_DELAYED_WB(regs_int_delayed, state->delayed_int, CONFIG_DIFF_REGS_INT_DELAYED_WIDTH, regs_name_int)
#endif // CONFIG_DIFFTEST_ARCHINTDELAYEDUPDATE
#ifdef CONFIG_DIFFTEST_ARCHFPDELAYEDUPDATE
  CHECK_DELAYED_WB(regs_fp_delayed, state->delayed_fp, CONFIG_DIFF_REGS_FP_DELAYED_WIDTH, regs_name_fp)
#endif // CONFIG_DIFFTEST_ARCHFPDELAYEDUPDATE
  return DiffTestChecker::STATE_OK;
}

int Difftest::apply_delayed_writeback() {
#define APPLY_DELAYED_WB(delayed, reg_type, regs_name)                       \
  do {                                                                       \
    static const int m = delay_wb_limit;                                     \
    for (int i = 0; i < 32; i++) {                                           \
      if (delayed[i]) {                                                      \
        if (delayed[i] > m) {                                                \
          Info("%s is delayed for more than %d cycles.\n", regs_name[i], m); \
          return DiffTestChecker::STATE_DIFF;                                \
        }                                                                    \
        delayed[i]++;                                                        \
        dut->regs.reg_type.value[i] = proxy->state.reg_type.value[i];        \
      }                                                                      \
    }                                                                        \
  } while (0);

#ifdef CONFIG_DIFFTEST_ARCHINTDELAYEDUPDATE
  APPLY_DELAYED_WB(state->delayed_int, xrf, regs_name_int)
#endif // CONFIG_DIFFTEST_ARCHINTDELAYEDUPDATE
#ifdef CONFIG_DIFFTEST_ARCHFPDELAYEDUPDATE
  APPLY_DELAYED_WB(state->delayed_fp, frf, regs_name_fp)
#endif // CONFIG_DIFFTEST_ARCHFPDELAYEDUPDATE
  return DiffTestChecker::STATE_OK;
}

void Difftest::display() {
  state->display();

  Info("\n==============  REF Regs  ==============\n");
  fflush(stdout);
  proxy->ref_reg_display();
  Info("privilegeMode: %lu\n", dut->regs.csr.privilegeMode);

  // show different register values
  proxy->display(dut);
  if (pc_mismatch) {
    REPORT_DIFFERENCE("pc", ref_commit_batch_pc, ref_commit_batch_pc, dut_commit_batch_pc);
  }
}

void Difftest::display_stats() {
  auto trap = get_trap_event();
  uint64_t instrCnt = trap->instrCnt;
  uint64_t cycleCnt = trap->cycleCnt;
  double ipc = (double)instrCnt / cycleCnt;
  Info(ANSI_COLOR_MAGENTA "Core-%d instrCnt = %'" PRIu64 ", cycleCnt = %'" PRIu64 ", IPC = %lf\n" ANSI_COLOR_RESET,
       state->coreid, instrCnt, cycleCnt, ipc);
}

// API for soft warmup, display final instr/cycle - warmup instr/cycle
void Difftest::warmup_display_stats() {
  auto trap = get_trap_event();
  uint64_t instrCnt = trap->instrCnt - warmup_info.instrCnt;
  uint64_t cycleCnt = trap->cycleCnt - warmup_info.cycleCnt;
  double ipc = (double)instrCnt / cycleCnt;
  Info(ANSI_COLOR_MAGENTA "Core-%d(Soft Warmup) instrCnt = %'" PRIu64 ", cycleCnt = %'" PRIu64
                          ", IPC = %lf\n" ANSI_COLOR_RESET,
       state->coreid, instrCnt, cycleCnt, ipc);
}
