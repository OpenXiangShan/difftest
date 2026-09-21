/***************************************************************************************
* Copyright (c) 2020-2025 Institute of Computing Technology, Chinese Academy of Sciences
* Copyright (c) 2025 Beijing Institute of Open Source Chip
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

#include "checkers.h"
#include "common.h"
#include "diffstate.h"
#include <cstdint>
#include <cstdlib>
#include <limits>
#include <string>
#include <sys/types.h>

#ifdef CONFIG_DIFFTEST_STOREEVENT

// Expand an 8-bit mask to a 64-bit wide data mask, like from 0x99 to 0xFF0000FF'FF0000FF
static uint64_t MaskExpand(uint8_t mask) {
  uint64_t expander = 0x0101010101010101ULL;
  uint64_t selector = 0x8040201008040201ULL;
  return (((((mask * expander) & selector) * 0xFFULL) >> 7) & expander) * 0xFFULL;
}

// safe bitmask function to avoid overflow or underflow
#define BITMASK(bits)        (((bits) >= 64ULL) ? (~0ULL) : ((1ULL << (bits)) - 1ULL))
#define BITMASKRANGE(hi, lo) (BITMASK(hi) & (~BITMASK(lo)))
#define MAX_OF(a, b)         ((a) > (b) ? (a) : (b))
#define MIN_OF(a, b)         ((a) < (b) ? (a) : (b))

bool StoreRecorder::get_valid(const DifftestStoreEvent &probe) {
  return probe.valid;
}

static bool parse_store_hash_mutation(const char *text, uint64_t *group, uint64_t *record) {
  if (text == nullptr || *text == '\0') {
    return false;
  }
  char *end = nullptr;
  const uint64_t parsed_group = strtoull(text, &end, 0);
  if (end == text || *end != ':') {
    return false;
  }
  const char *record_text = end + 1;
  const uint64_t parsed_record = strtoull(record_text, &end, 0);
  if (end == record_text || *end != '\0') {
    return false;
  }
  *group = parsed_group;
  *record = parsed_record;
  return true;
}

StoreChecker::StoreChecker(DiffState *state, RefProxy *proxy) : SimpleChecker(state, proxy) {
  const char *enabled = getenv("DIFFTEST_STORE_HASH");
  hash_enabled = enabled != nullptr && *enabled != '\0' && *enabled != '0';
  if (!hash_enabled) {
    return;
  }

  difftest_store_hash_init(&hash_state);
  const char *mutation = getenv("DIFFTEST_STORE_HASH_MUTATE");
  if (mutation != nullptr && !parse_store_hash_mutation(mutation, &mutate_group, &mutate_record)) {
    Info("[StoreHash] ignoring malformed DIFFTEST_STORE_HASH_MUTATE='%s' (expected group:record)\n", mutation);
  }
  Info("[StoreHash] enabled: version=%d stores_per_group=%lu\n", DIFFTEST_STORE_HASH_VERSION,
       stores_per_group);
  if (mutate_group != UINT64_MAX) {
    Info("[StoreHash] injecting DUT hash mutation at group=%lu record=%lu\n", mutate_group, mutate_record);
  }
}
void StoreRecorder::clear_valid(DifftestStoreEvent &probe) {
  probe.valid = 0;
}

int StoreRecorder::check(const DifftestStoreEvent &probe) {

  if (!probe.valid)
    return STATE_OK;

  int BLOCKOFFSETBITS = 6;
  int WORDBYTES = 8;
  int COMMITBYTES = 16;

  auto addr = probe.addr;
  auto lowData = probe.data;
  auto highData = probe.highData;
  auto mask = probe.mask;
  auto offset = probe.offset;
  auto eew = probe.eew;
  auto pc = probe.pc;
  auto robIdx = probe.robidx;
  auto vecNeedSplit = probe.vecNeedSplit;
  auto wLine = probe.wLine;
  const uint64_t storeInstrSeq = state->next_store_instr_seq++;
  bool emitted = false;
  auto enqueue = [this, storeInstrSeq, &emitted](DiffState::StoreCommit storeCommit) {
    storeCommit.store_instr_seq = storeInstrSeq;
    state->store_event_queue.push(storeCommit);
    emitted = true;
  };

  if (vecNeedSplit) {
    // 1. separate a store event into multiple eew-width elements.
    // 2. for each element, check whether it crosses a 8B boundary.
    // 3. if it crosses a 8B boundary, split it into two sub commits,
    //    if not, commit it as a whole.
    uint16_t flow = COMMITBYTES / eew;
    uint16_t eew_off = offset % eew;

    for (int i = -1; i < flow; i++) {
      uint16_t flowMask =
          mask & BITMASKRANGE(MIN_OF(i * eew + eew_off + eew, COMMITBYTES), MAX_OF(i * eew + eew_off, 0LL));
      uint8_t commitLowMask = flowMask & 0XFF;
      bool commitLowValid = probe.valid && commitLowMask;
      uint64_t commitLowAddr = addr;
      uint64_t commitLowData = lowData & MaskExpand(commitLowMask);

      if (commitLowValid) {
        DiffState::StoreCommit storeCommitLow = {commitLowValid,
                                                 commitLowAddr,
                                                 commitLowData,
                                                 commitLowMask,
                                                 pc,
                                                 robIdx,
                                                 addr,
                                                 lowData,
                                                 highData,
                                                 mask
#ifdef CONFIG_DIFFTEST_SQUASH
                                                 ,
                                                 probe.stamp
#endif // CONFIG_DIFFTEST_SQUASH
        };
        enqueue(storeCommitLow);
      }

      uint8_t commitHighMask = (flowMask >> 8) & 0xFF;
      bool commitHighValid = probe.valid && commitHighMask;
      uint64_t commitHighAddr = addr + 8;
      uint64_t commitHighData = highData & MaskExpand(commitHighMask);

      if (commitHighValid) {
        DiffState::StoreCommit storeCommitHigh = {commitHighValid,
                                                  commitHighAddr,
                                                  commitHighData,
                                                  commitHighMask,
                                                  pc,
                                                  robIdx,
                                                  addr,
                                                  lowData,
                                                  highData,
                                                  mask
#ifdef CONFIG_DIFFTEST_SQUASH
                                                  ,
                                                  probe.stamp
#endif // CONFIG_DIFFTEST_SQUASH
        };
        enqueue(storeCommitHigh);
      }
    }
  } else if (wLine) {
    uint64_t blockAddr = addr >> BLOCKOFFSETBITS << BLOCKOFFSETBITS;
    for (int i = 0; i < 8; i++) {
      uint64_t refStoreCommitAddr = blockAddr + i * WORDBYTES;
      uint64_t refStoreCommitData = 0;
      uint8_t refStoreCommitMask = 0xff;

      DiffState::StoreCommit storeCommit = {probe.valid,
                                            refStoreCommitAddr,
                                            refStoreCommitData,
                                            refStoreCommitMask,
                                            pc,
                                            robIdx,
                                            addr,
                                            lowData,
                                            highData,
                                            mask
#ifdef CONFIG_DIFFTEST_SQUASH
                                            ,
                                            probe.stamp
#endif // CONFIG_DIFFTEST_SQUASH
      };
      enqueue(storeCommit);
    }
  } else {
    // 1. check whether the store event crosses a 8B boundary.
    // 2. if it crosses a 8B boundary, split it into two sub commits,
    //    if not, commit it as a whole.
    uint8_t commitLowMask = mask & 0XFF;
    bool commitLowValid = probe.valid && commitLowMask;
    uint64_t commitLowAddr = addr;
    uint64_t commitLowData = lowData & MaskExpand(commitLowMask);
    if (commitLowValid) {
      DiffState::StoreCommit storeCommitLow = {commitLowValid,
                                               commitLowAddr,
                                               commitLowData,
                                               commitLowMask,
                                               pc,
                                               robIdx,
                                               addr,
                                               lowData,
                                               highData,
                                               mask
#ifdef CONFIG_DIFFTEST_SQUASH
                                               ,
                                               probe.stamp
#endif // CONFIG_DIFFTEST_SQUASH
      };
      enqueue(storeCommitLow);
    }

    uint8_t commitHighMask = (mask >> 8) & 0XFF;
    bool commitHighValid = probe.valid && commitHighMask;
    uint64_t commitHighAddr = addr + 8;
    uint64_t commitHighData = highData & MaskExpand(commitHighMask);
    if (commitHighValid) {
      DiffState::StoreCommit storeCommitHigh = {commitHighValid,
                                                commitHighAddr,
                                                commitHighData,
                                                commitHighMask,
                                                pc,
                                                robIdx,
                                                addr,
                                                lowData,
                                                highData,
                                                mask
#ifdef CONFIG_DIFFTEST_SQUASH
                                                ,
                                                probe.stamp
#endif // CONFIG_DIFFTEST_SQUASH
      };
      enqueue(storeCommitHigh);
    }
  }

  if (emitted) {
    state->store_event_queue.back().is_last_record = true;
  }

  return STATE_OK;
}

int StoreChecker::check_hash_record(const DiffState::StoreCommit &probe) {
  if (!hash_started) {
    hash_started = true;
    hash_instr_begin = probe.store_instr_seq;
    hash_instr_end = probe.store_instr_seq;
    hash_instr_count = 1;
    hash_record_count = 0;
    difftest_store_hash_init(&hash_state);
  } else if (probe.store_instr_seq != hash_instr_end) {
    hash_instr_count += probe.store_instr_seq - hash_instr_end;
    hash_instr_end = probe.store_instr_seq;
  }

  uint64_t data = probe.data;
  if (hash_group_id == mutate_group && hash_record_count == mutate_record) {
    data ^= 1;
    Info("[StoreHash] injected DUT mutation at group=%lu record=%lu pc=0x%016lx\n", hash_group_id,
         hash_record_count, probe.pc);
  }
  difftest_store_hash_update(&hash_state, probe.addr, data, probe.mask);
  hash_record_count++;

  if (probe.is_last_record && hash_instr_count >= stores_per_group) {
    return flush_hash();
  }
  return STATE_OK;
}

int StoreChecker::flush_hash() {
  if (!hash_started) {
    return STATE_OK;
  }

  Info("[StoreHash] checking group=%lu instr=[%lu,%lu] stores=%lu records=%lu hash=(0x%016lx,0x%016lx)\n",
       hash_group_id, hash_instr_begin, hash_instr_end, hash_instr_count, hash_record_count, hash_state.h0,
       hash_state.h1);
  const int ret = proxy->store_commit_hash(hash_state.count, hash_state.h0, hash_state.h1, hash_group_id,
                                           hash_instr_begin, hash_instr_end);
  if (ret) {
    Info("[StoreHash] mismatch in group=%lu instr=[%lu,%lu], records=%lu\n", hash_group_id, hash_instr_begin,
         hash_instr_end, hash_record_count);
    return STATE_ERROR;
  }

  hash_started = false;
  hash_instr_count = 0;
  hash_record_count = 0;
  hash_group_id++;
  return STATE_OK;
}

int StoreChecker::check() {
  if (hash_enabled) {
    while (!state->store_event_queue.empty()) {
      auto &front = state->store_event_queue.front();
#ifdef CONFIG_DIFFTEST_SQUASH
      if (front.stamp != state->commit_stamp)
        return STATE_OK;
#endif // CONFIG_DIFFTEST_SQUASH
      const auto probe = front;
      state->store_event_queue.pop();
      if (int ret = check_hash_record(probe)) {
        return ret;
      }
    }
    return STATE_OK;
  }

  while (!state->store_event_queue.empty()) {
    auto &probe = state->store_event_queue.front();
#ifdef CONFIG_DIFFTEST_SQUASH
    if (probe.stamp != state->commit_stamp)
      return STATE_OK;
#endif // CONFIG_DIFFTEST_SQUASH
    auto addr = probe.addr;
    auto data = probe.data;
    auto mask = probe.mask;

    if (proxy->store_commit(&addr, &data, &mask)) {
#ifdef FUZZING
      if (proxy->in_disambiguation_state()) {
        Info("Store mismatch detected with a disambiguation state at pc = 0x%lx.\n", probe.pc);
        return STATE_OK;
      }
#endif
      uint64_t pc = probe.pc;
      Info("\n==============  Store Commit Event (Core %d)  ==============\n", state->coreid);
      proxy->get_store_event_other_info(&pc);
      Info("Mismatch for store commits \n");
      Info("  REF commits addr 0x%016lx, data 0x%016lx, mask 0x%02x, pc 0x%016lx\n", addr, data, mask, pc);
      Info("  DUT commits addr 0x%016lx, data 0x%016lx, mask 0x%02x, pc 0x%016lx, robidx 0x%x\n", probe.addr,
           probe.data, probe.mask, probe.pc, probe.robidx);
      Info("  DUT origin: addr 0x%016lx, high:0x%016lx, low 0x%016lx, mask:0x%04x\n", probe.origin_addr,
           probe.origin_highdata, probe.origin_lowdata, probe.origin_mask);

      state->store_event_queue.pop();
      return STATE_ERROR;
    }

    state->store_event_queue.pop();
  }

  return STATE_OK;
}

void StoreChecker::finish() {
  if (!hash_enabled) {
    return;
  }
  while (!state->store_event_queue.empty()) {
    const auto probe = state->store_event_queue.front();
    state->store_event_queue.pop();
    if (check_hash_record(probe)) {
      return;
    }
  }
  flush_hash();
}
#endif // CONFIG_DIFFTEST_STOREEVENT

#ifdef CONFIG_DIFFTEST_STOREHASHEVENT
bool StoreHashChecker::get_valid(const DifftestStoreHashEvent &probe) {
#ifdef CONFIG_DIFFTEST_SQUASH
  return probe.valid && probe.stamp == state->commit_stamp;
#else
  return probe.valid;
#endif // CONFIG_DIFFTEST_SQUASH
}

void StoreHashChecker::clear_valid(DifftestStoreHashEvent &probe) {
  probe.valid = 0;
}

int StoreHashChecker::check(const DifftestStoreHashEvent &probe) {
  Info("[StoreHash] checking hardware group=%lu instr=[%lu,%lu] records=%u hash=(0x%016lx,0x%016lx)\n",
       probe.group_id, probe.instr_begin, probe.instr_end, probe.record_count, probe.hash_lo, probe.hash_hi);
  return proxy->store_commit_hash(probe.record_count, probe.hash_lo, probe.hash_hi, probe.group_id,
                                  probe.instr_begin, probe.instr_end)
             ? STATE_ERROR
             : STATE_OK;
}
#endif // CONFIG_DIFFTEST_STOREHASHEVENT
