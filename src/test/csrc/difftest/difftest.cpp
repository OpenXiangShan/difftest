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
#include <numeric>
#include <vector>
#endif // CONFIG_DIFFTEST_FORK
#include <csignal>
#include <cstdlib>
#ifdef CONFIG_DIFFTEST_FORK
#include <cerrno>
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

#ifdef CONFIG_DIFFTEST_FORK
namespace {
constexpr size_t fork_max_store_entries = 8192;
constexpr size_t fork_max_migration_state_size = 4096;
constexpr size_t fork_default_max_outstanding = 64;

const size_t fork_max_outstanding = []() {
  const char *value = getenv("DIFFTEST_FORK_MAX_OUTSTANDING");
  if (value == nullptr) return fork_default_max_outstanding;
  const size_t requested = strtoul(value, nullptr, 0);
  return std::max<size_t>(1, std::min(requested, fork_default_max_outstanding));
}();

struct ForkSharedResult {
  int ret;
  int failed_index;
  size_t store_count;
  size_t migration_state_size;
  ref_state_t state;
  uint64_t csrs[4096];
  uint8_t migration_state[fork_max_migration_state_size];
  RefStoreLogEntry stores[fork_max_store_entries];
};

struct PendingForkGroup {
  uint64_t id;
  pid_t pid;
  ForkSharedResult *result;
  ref_state_t start_state;
  std::vector<uint64_t> start_csrs;
  std::vector<uint8_t> start_migration_state;
  ref_state_t parent_state;
  std::vector<uint64_t> parent_csrs;
  std::vector<uint8_t> parent_migration_state;
  std::vector<RefStoreLogEntry> parent_stores;
  std::vector<DifftestForkWindow> windows;
};

std::vector<RefStoreLogEntry> read_store_log(RefProxy *proxy) {
  std::vector<RefStoreLogEntry> entries(proxy->ref_store_log_size());
  if (!entries.empty()) {
    entries.resize(proxy->ref_store_log_copy(entries.data(), entries.size()));
  }
  return entries;
}

bool same_store_log(const std::vector<RefStoreLogEntry> &lhs, const std::vector<RefStoreLogEntry> &rhs) {
  return lhs.size() == rhs.size() &&
         std::equal(lhs.begin(), lhs.end(), rhs.begin(), [](const auto &a, const auto &b) {
           return a.addr == b.addr && a.data == b.data && a.mask == b.mask;
         });
}

void apply_store_log(RefProxy *proxy, const std::vector<RefStoreLogEntry> &entries) {
  for (const auto &entry : entries) {
    uint64_t value = 0;
    proxy->ref_memcpy(entry.addr, &value, sizeof(value), REF_TO_DUT);
    for (int byte = 0; byte < 8; ++byte) {
      if (entry.mask & (1ull << byte)) {
        const uint64_t byte_mask = 0xffull << (byte * 8);
        value = (value & ~byte_mask) | (entry.data & byte_mask);
      }
    }
    proxy->ref_memcpy(entry.addr, &value, sizeof(value), DUT_TO_REF);
  }
}

void restore_store_log(RefProxy *proxy, const std::vector<RefStoreLogEntry> &entries) {
  for (auto it = entries.rbegin(); it != entries.rend(); ++it) {
    uint64_t value = it->orig_data;
    proxy->ref_memcpy(it->addr, &value, sizeof(value), DUT_TO_REF);
  }
}

void stop_child(PendingForkGroup &group) {
  if (waitpid(group.pid, nullptr, WNOHANG) == 0) {
    kill(group.pid, SIGKILL);
    waitpid(group.pid, nullptr, 0);
  }
  munmap(group.result, sizeof(ForkSharedResult));
}
} // namespace

std::vector<uint32_t> fork_window_sizes;
std::deque<PendingForkGroup> fork_pending_groups;
uint64_t fork_group_count = 0;
uint64_t fork_rollback_count = 0;
uint64_t fork_group_release_count = 0;
size_t fork_peak_outstanding = 0;
bool fork_worker_process = false;
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
  for (auto &group : fork_pending_groups) {
    stop_child(group);
  }
  fork_pending_groups.clear();
  if (!fork_window_sizes.empty()) {
    std::sort(fork_window_sizes.begin(), fork_window_sizes.end());
    const auto percentile = [](const std::vector<uint32_t> &values, double p) {
      const size_t index = static_cast<size_t>((values.size() - 1) * p);
      return values[index];
    };
    printf("ForkWindowCnt = %zu\n", fork_window_sizes.size());
    printf("ForkGroupCnt = %lu\n", static_cast<unsigned long>(fork_group_count));
    printf("ForkGroupReleaseCnt = %lu\n", static_cast<unsigned long>(fork_group_release_count));
    printf("ForkRollbackCnt = %lu\n", static_cast<unsigned long>(fork_rollback_count));
    printf("ForkPeakOutstanding = %zu\n", fork_peak_outstanding);
    printf("ForkWindowInstr = %lu\n",
           static_cast<unsigned long>(std::accumulate(fork_window_sizes.begin(), fork_window_sizes.end(), uint64_t{0})));
    printf("ForkWindowN p50=%u p90=%u p99=%u max=%u\n",
           percentile(fork_window_sizes, 0.50), percentile(fork_window_sizes, 0.90),
           percentile(fork_window_sizes, 0.99), fork_window_sizes.back());
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
  checkers.push_back(new LrScChecker([this]() -> DifftestLrScEvent & { return dut->lrsc; }, state, proxy));
#endif

#ifdef CONFIG_DIFFTEST_NONREGINTERRUPTPENDINGEVENT
  checkers.push_back(new NonRegInterruptPendingChecker(
      [this]() -> DifftestNonRegInterruptPendingEvent & { return dut->non_reg_interrupt_pending; }, state, proxy));
#endif

#ifdef CONFIG_DIFFTEST_MHPMEVENTOVERFLOWEVENT
  checkers.push_back(new MhpmeventOverflowChecker(
      [this]() -> DifftestMhpmeventOverflowEvent & { return dut->mhpmevent_overflow; }, state, proxy));
#endif
#ifdef CONFIG_DIFFTEST_CRITICALERROREVENT
  checkers.push_back(
      new CriticalErrorChecker([this]() -> DifftestCriticalErrorEvent & { return dut->critical_error; }, state, proxy));
#endif
#ifdef CONFIG_DIFFTEST_SYNCAIAEVENT
  checkers.push_back(new AiaChecker([this]() -> DifftestSyncAIAEvent & { return dut->sync_aia; }, state, proxy));
#endif
#ifdef CONFIG_DIFFTEST_SYNCCUSTOMMFLUSHPWREVENT
  checkers.push_back(new CustomMflushpwrChecker(
      [this]() -> DifftestSyncCustomMflushpwrEvent & { return dut->sync_custom_mflushpwr; }, state, proxy));
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

int Difftest::step() {
#ifdef CONFIG_DIFFTEST_FORK
  if (int ret = drain_fork(false)) {
    return ret;
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
    if (window_instr != 0) {
      fork_group.push_back({*dut, window_instr});
      fork_window_sizes.push_back(window_instr);
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
  // A synchronous window may update REF through skip/special-event handling.
  // Make every preceding speculative group authoritative before it observes
  // or mutates the shared REF state.
  if (int ret = drain_fork(true)) {
    return ret;
  }
#endif // CONFIG_DIFFTEST_FORK
  int ret = check_all();
#ifdef CONFIG_DIFFTEST_FORK
  if (ret == DiffTestChecker::STATE_OK) {
    capture_fork_authority(fork_group_count);
  }
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
void Difftest::set_fork_authority(uint64_t group_id, const ref_state_t &new_state, const uint64_t *csrs,
                                   size_t csr_count, const uint8_t *migration_state, size_t migration_state_size) {
  assert(csr_count == 4096);
  assert(migration_state_size <= fork_max_migration_state_size);
  fork_authority_state = new_state;
  fork_authority_csrs.assign(csrs, csrs + csr_count);
  fork_authority_migration_state.assign(migration_state, migration_state + migration_state_size);
  fork_authority_group = group_id;
  fork_authority_valid = true;
}

void Difftest::capture_fork_authority(uint64_t group_id) {
  proxy->sync();
  std::vector<uint64_t> csrs(4096);
  proxy->ref_csrcpy(csrs.data(), REF_TO_DUT);
  std::vector<uint8_t> migration_state(proxy->ref_migration_state_size());
  if (migration_state.size() > fork_max_migration_state_size) {
    Info("fork DiffTest migration state is too large: %zu bytes\n", migration_state.size());
    return;
  }
  proxy->ref_migration_state_copy(migration_state.data(), REF_TO_DUT);
  set_fork_authority(group_id, proxy->state, csrs.data(), csrs.size(), migration_state.data(), migration_state.size());
}

void Difftest::restore_fork_authority() {
  assert(fork_authority_valid);
  proxy->state = fork_authority_state;
  proxy->ref_regcpy(&proxy->state, DUT_TO_REF, false);
  proxy->ref_csrcpy(fork_authority_csrs.data(), DUT_TO_REF);
  proxy->ref_migration_state_copy(fork_authority_migration_state.data(), DUT_TO_REF);
  proxy->ref_state_migrate();
}

uint32_t Difftest::fork_window_instr() const {
  uint32_t count = 0;
  for (int i = 0; i < CONFIG_DIFF_COMMIT_WIDTH; ++i) {
    if (dut->commit[i].valid) {
      count += 1 + dut->commit[i].nFused;
    }
  }
  return count;
}

bool Difftest::fork_window_eligible() const {
  if (dut->event.valid) return false;
#ifdef CONFIG_DIFFTEST_LRSCEVENT
  if (dut->lrsc.valid) return false;
#endif
#ifdef CONFIG_DIFFTEST_SYNCAIAEVENT
  if (dut->sync_aia.valid) return false;
#endif
#ifdef CONFIG_DIFFTEST_NONREGINTERRUPTPENDINGEVENT
  if (dut->non_reg_interrupt_pending.valid) return false;
#endif
#ifdef CONFIG_DIFFTEST_CRITICALERROREVENT
  if (dut->critical_error.valid) return false;
#endif
#ifdef CONFIG_DIFFTEST_SYNCCUSTOMMFLUSHPWREVENT
  if (dut->sync_custom_mflushpwr.valid) return false;
#endif
#ifdef CONFIG_DIFFTEST_DEBUGMODE
  if (dut->dmregs.debugMode != 0) return false;
#endif

  bool has_commit = false;
  for (int i = 0; i < CONFIG_DIFF_COMMIT_WIDTH; ++i) {
    const auto &commit = dut->commit[i];
    if (!commit.valid) continue;
    has_commit = true;
    // A skip or special event can change REF state without a one-to-one
    // architectural execution. Keep those boundaries on the authoritative
    // synchronous path until the migration protocol covers them.
    if (commit.skip || commit.special != 0) return false;
  }
  return has_commit;
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
  for (const auto &window : fork_group) {
    total_instr += window.instr_count;
  }

  proxy->sync();
  const ref_state_t start_state = proxy->state;
  std::vector<uint64_t> start_csrs(4096);
  proxy->ref_csrcpy(start_csrs.data(), REF_TO_DUT);
  std::vector<uint8_t> start_migration_state(proxy->ref_migration_state_size());
  if (start_migration_state.size() > fork_max_migration_state_size) {
    Info("fork DiffTest migration state is too large: %zu bytes\n", start_migration_state.size());
    return DiffTestChecker::STATE_ERROR;
  }
  proxy->ref_migration_state_copy(start_migration_state.data(), REF_TO_DUT);
  if (!fork_authority_valid) {
    set_fork_authority(group_id - 1, start_state, start_csrs.data(), start_csrs.size(), start_migration_state.data(),
                        start_migration_state.size());
  }

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
    if (child_delay_us != 0) {
      usleep(child_delay_us);
    }
    for (size_t i = 0; i < fork_group.size(); ++i) {
      dut = &fork_group[i].dut;
      shared_result->ret = check_all();
      if (shared_result->ret != DiffTestChecker::STATE_OK) {
        shared_result->failed_index = static_cast<int>(i);
        break;
      }
    }
    proxy->sync();
    shared_result->state = proxy->state;
    proxy->ref_csrcpy(shared_result->csrs, REF_TO_DUT);
    shared_result->migration_state_size = proxy->ref_migration_state_size();
    if (shared_result->migration_state_size > fork_max_migration_state_size) {
      shared_result->ret = DiffTestChecker::STATE_ERROR;
    } else {
      proxy->ref_migration_state_copy(shared_result->migration_state, REF_TO_DUT);
    }
    const auto child_store_log = read_store_log(proxy);
    if (child_store_log.size() > fork_max_store_entries) {
      shared_result->ret = DiffTestChecker::STATE_ERROR;
    } else {
      shared_result->store_count = child_store_log.size();
      std::copy(child_store_log.begin(), child_store_log.end(), shared_result->stores);
    }
    _exit(0);
  }

  // SHARE_BATCH_EXEC keeps the request exact while amortizing the shared REF
  // boundary. It still returns early for a trap or other NEMU stop condition.
  proxy->ref_exec(total_instr);
  proxy->sync();
  auto parent_store_log = read_store_log(proxy);
  const ref_state_t parent_state = proxy->state;
  std::vector<uint64_t> parent_csrs(4096);
  proxy->ref_csrcpy(parent_csrs.data(), REF_TO_DUT);
  std::vector<uint8_t> parent_migration_state(proxy->ref_migration_state_size());
  if (parent_migration_state.size() > fork_max_migration_state_size) {
    Info("fork DiffTest migration state is too large: %zu bytes\n", parent_migration_state.size());
    kill(child, SIGKILL);
    waitpid(child, nullptr, 0);
    munmap(shared_result, sizeof(ForkSharedResult));
    proxy->set_store_log(false);
    proxy->ref_store_log_reset();
    return DiffTestChecker::STATE_ERROR;
  }
  proxy->ref_migration_state_copy(parent_migration_state.data(), REF_TO_DUT);
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
  if (corrupt_store_group == static_cast<long>(group_id) && !parent_store_log.empty()) {
    const int byte = __builtin_ctzll(parent_store_log.front().mask);
    parent_store_log.front().data ^= 1ull << (byte * 8);
    apply_store_log(proxy, {parent_store_log.front()});
  }

  fork_pending_groups.push_back(
      {group_id, child, shared_result, start_state, std::move(start_csrs), std::move(start_migration_state), parent_state,
       std::move(parent_csrs),
       std::move(parent_migration_state), std::move(parent_store_log), std::move(fork_group)});
  fork_peak_outstanding = std::max(fork_peak_outstanding, fork_pending_groups.size());
  fork_group.clear();
  return drain_fork(false);
}

int Difftest::fork_release_front(bool block, bool &released) {
  released = false;
  if (fork_pending_groups.empty()) return DiffTestChecker::STATE_OK;

  auto &group = fork_pending_groups.front();
  int child_status = 0;
  const pid_t waited = waitpid(group.pid, &child_status, block ? 0 : WNOHANG);
  if (waited == 0) return DiffTestChecker::STATE_OK;
  if (waited != group.pid || !WIFEXITED(child_status) || WEXITSTATUS(child_status) != 0) {
    Info("fork DiffTest child failed for group %lu\n", static_cast<unsigned long>(group.id));
    stop_child(group);
    fork_pending_groups.pop_front();
    return DiffTestChecker::STATE_ERROR;
  }

  auto *result = group.result;
  std::vector<RefStoreLogEntry> child_stores(result->stores, result->stores + result->store_count);

  const bool start_state_matches =
      fork_authority_valid && memcmp(&group.start_state, &fork_authority_state, sizeof(ref_state_t)) == 0;
  const bool start_csrs_match =
      fork_authority_valid && group.start_csrs.size() == fork_authority_csrs.size() &&
      memcmp(group.start_csrs.data(), fork_authority_csrs.data(), sizeof(uint64_t) * group.start_csrs.size()) == 0;
  const bool start_migration_state_matches =
      fork_authority_valid && group.start_migration_state == fork_authority_migration_state;
  const bool start_matches = start_state_matches && start_csrs_match && start_migration_state_matches;

  if (!start_matches) {
    Info("fork DiffTest start mismatch for group %lu (authority=%lu state=%d csrs=%d migration=%d)\n",
         static_cast<unsigned long>(group.id), static_cast<unsigned long>(fork_authority_group), start_state_matches,
         start_csrs_match, start_migration_state_matches);

    std::vector<std::vector<DifftestForkWindow>> replay_groups;
    for (auto &pending : fork_pending_groups) {
      replay_groups.push_back(pending.windows);
    }
    auto partial_group = std::move(fork_group);
    fork_group.clear();

    for (auto it = fork_pending_groups.rbegin(); it != fork_pending_groups.rend(); ++it) {
      restore_store_log(proxy, it->parent_stores);
    }
    for (auto &pending : fork_pending_groups) {
      stop_child(pending);
    }
    fork_pending_groups.clear();
    restore_fork_authority();
    ++fork_rollback_count;

    for (auto &windows : replay_groups) {
      fork_group = std::move(windows);
      if (int ret = fork_group_step()) return ret;
    }
    fork_group = std::move(partial_group);
    released = true;
    return DiffTestChecker::STATE_OK;
  }

  if (result->ret != DiffTestChecker::STATE_OK) {
    if (result->ret == DiffTestChecker::STATE_DIFF && result->failed_index >= 0 &&
        static_cast<size_t>(result->failed_index) < group.windows.size()) {
      proxy->state = result->state;
      proxy->display(&group.windows[result->failed_index].dut);
    }
    const int ret = result->ret;
    stop_child(group);
    fork_pending_groups.pop_front();
    return ret;
  }

  const bool state_matches = memcmp(&result->state, &group.parent_state, sizeof(ref_state_t)) == 0;
  const bool csrs_match = memcmp(result->csrs, group.parent_csrs.data(), sizeof(result->csrs)) == 0;
  const bool migration_state_matches =
      result->migration_state_size == group.parent_migration_state.size() &&
      memcmp(result->migration_state, group.parent_migration_state.data(), result->migration_state_size) == 0;
  const bool stores_match = same_store_log(child_stores, group.parent_stores);
  if (state_matches && csrs_match && migration_state_matches && stores_match) {
    set_fork_authority(group.id, result->state, result->csrs, 4096, result->migration_state,
                        result->migration_state_size);
    stop_child(group);
    fork_pending_groups.pop_front();
    ++fork_group_release_count;
    released = true;
    return DiffTestChecker::STATE_OK;
  }

  Info("fork DiffTest endpoint mismatch for group %lu (state=%d csrs=%d migration=%d stores=%d)\n",
       static_cast<unsigned long>(group.id), state_matches, csrs_match, migration_state_matches, stores_match);
  ++fork_rollback_count;

  std::vector<std::vector<DifftestForkWindow>> replay_groups;
  for (size_t i = 1; i < fork_pending_groups.size(); ++i) {
    replay_groups.push_back(fork_pending_groups[i].windows);
  }
  auto partial_group = std::move(fork_group);
  fork_group.clear();

  for (auto it = fork_pending_groups.rbegin(); it != fork_pending_groups.rend(); ++it) {
    restore_store_log(proxy, it->parent_stores);
  }
  apply_store_log(proxy, child_stores);
  proxy->state = result->state;
  proxy->ref_regcpy(&proxy->state, DUT_TO_REF, false);
  proxy->ref_csrcpy(result->csrs, DUT_TO_REF);
  proxy->ref_migration_state_copy(result->migration_state, DUT_TO_REF);
  proxy->ref_state_migrate();
  set_fork_authority(group.id, result->state, result->csrs, 4096, result->migration_state,
                      result->migration_state_size);

  for (auto &pending : fork_pending_groups) {
    stop_child(pending);
  }
  fork_pending_groups.clear();
  ++fork_group_release_count;

  for (auto &windows : replay_groups) {
    fork_group = std::move(windows);
    if (int ret = fork_group_step()) return ret;
  }
  fork_group = std::move(partial_group);
  released = true;
  return DiffTestChecker::STATE_OK;
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
#endif // CONFIG_DIFFTEST_FORK

inline int Difftest::check_all() {
  state->cycle_count = get_trap_event()->cycleCnt;
  state->has_progress = false;

  // normal checkers
  for (auto checker: checkers) {
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
