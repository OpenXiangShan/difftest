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

  /*
  Info("addr: %08x, data:%016lx'%016lx, mask:%04x, off:%04x,"
  "eew:%d, pc:%016lx, robidx:%d, vecNeedSplit:%d, wLine:%d\n",
  addr, highData, lowData, mask, offset, eew, pc, robIdx, vecNeedSplit, wLine);
  */

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
                                                 robIdx
#ifdef CONFIG_DIFFTEST_SQUASH
                                                 ,
                                                 probe.stamp
#endif // CONFIG_DIFFTEST_SQUASH
        };
        state->store_event_queue.push(storeCommitLow);
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
                                                  robIdx
#ifdef CONFIG_DIFFTEST_SQUASH
                                                  ,
                                                  probe.stamp
#endif // CONFIG_DIFFTEST_SQUASH
        };
        state->store_event_queue.push(storeCommitHigh);
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
                                            robIdx
#ifdef CONFIG_DIFFTEST_SQUASH
                                            ,
                                            probe.stamp
#endif // CONFIG_DIFFTEST_SQUASH
      };
      state->store_event_queue.push(storeCommit);
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
                                               robIdx
#ifdef CONFIG_DIFFTEST_SQUASH
                                               ,
                                               probe.stamp
#endif // CONFIG_DIFFTEST_SQUASH
      };
      state->store_event_queue.push(storeCommitLow);
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
                                                robIdx
#ifdef CONFIG_DIFFTEST_SQUASH
                                                ,
                                                probe.stamp
#endif // CONFIG_DIFFTEST_SQUASH
      };
      state->store_event_queue.push(storeCommitHigh);
    }
  }

  return STATE_OK;
}

int StoreChecker::check() {
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
      Info("  REF commits addr 0x%016lx, data 0x%016lx, mask 0x%04x, pc 0x%016lx\n", addr, data, mask, pc);
      Info("  DUT commits addr 0x%016lx, data 0x%016lx, mask 0x%04x, pc 0x%016lx, robidx 0x%x\n", probe.addr,
           probe.data, probe.mask, probe.pc, probe.robidx);

      state->store_event_queue.pop();
      return STATE_ERROR;
    }

    state->store_event_queue.pop();
  }

  return STATE_OK;
}
#endif // CONFIG_DIFFTEST_STOREEVENT
