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
#include "diffstate.h"
#include <cstdint>
#include <sys/types.h>

#ifdef CONFIG_DIFFTEST_STOREEVENT
bool StoreRecorder::get_valid(const DifftestStoreEvent &probe) {
  return probe.valid;
}
void StoreRecorder::clear_valid(DifftestStoreEvent &probe) {
  probe.valid = 0;
}

// Expand 8-bit mask to bit-and with 64-bit wide data
static uint64_t MaskExpand(uint8_t mask) {
  uint64_t expander = 0x0101010101010101ULL;
  uint64_t selector = 0x8040201008040201ULL;
  return (((((mask * expander) & selector) * 0xFFULL) >> 7) & expander) * 0xFFULL;
}

int StoreRecorder::check(const DifftestStoreEvent &probe) {
  if (!probe.valid)
    return STATE_OK;

  int BLOCKOFFSETBITS = 6;
  int WORDBYTES = 8;
  int COMMITBYTES = 16;

  uint64_t calcAddr;
  uint64_t calcDataLow;
  uint64_t calcDataHigh;
  uint16_t calcMask;
  uint8_t calcEEW;
  bool calcIsVSLine;

  auto isNonCacheable = probe.isNonCacheable;
  auto isVStore = probe.isVStore;
  auto isUStride = probe.isUStride;
  auto isMasked = probe.isMasked;
  auto isWhole = probe.isWhole;
  auto veew = probe.veew;
  auto nf = probe.nf;
  auto rawDataLow = probe.rawDataLow;
  auto rawDataHigh = probe.rawDataHigh;
  auto rawMask = probe.rawMask;
  auto rawAddr = probe.rawAddr;
  auto wLine = probe.wLine;
  auto isHighPart = probe.isHighPart;

  if (!isNonCacheable) {
    bool isVse = isVStore && isUStride;
    bool isVsm = isVStore && isMasked;
    bool isVsr = isVStore && isWhole;

    uint8_t eew = veew;
    uint32_t EEB = 1 << eew;
    uint8_t nf2 = isVsr ? 0 : nf;

    bool isSegment = nf2 != 0 && !isVsm;
    bool isVSLine = (isVse || isVsm || isVsr) && !isSegment;
    bool isWline = wLine;
    calcIsVSLine = isVSLine;

    if (isVSLine) {
      calcAddr = rawAddr;
      calcDataLow = rawDataLow;
      calcDataHigh = rawDataHigh;
      calcMask = rawMask;
    } else if (isWline) {
      calcAddr = rawAddr;
      calcDataLow = rawDataLow;
      calcDataHigh = rawDataHigh;
      calcMask = rawMask;
      Assert(rawDataLow == 0 && rawDataHigh == 0, "wLine only supports whole zero write now");
    } else if (isHighPart) {
      calcAddr = (rawAddr & (~0xFULL)) | 0b1000;
      calcMask = (rawMask >> 8) & 0xFF;
      uint64_t tmpData = rawDataHigh;
      calcDataLow = tmpData & MaskExpand(calcMask);
      calcDataHigh = 0;
    } else {
      calcAddr = rawAddr & (~0xFULL);
      calcMask = rawMask & 0xFF;
      uint64_t tmpData = rawDataLow;
      calcDataLow = tmpData & MaskExpand(calcMask);
      calcDataHigh = 0;
    }
    calcEEW = EEB;
  } else {
    calcAddr = rawAddr & (~7ULL);
    calcMask = rawMask;
    calcDataLow = rawDataLow & MaskExpand(calcMask);
    calcDataHigh = 0;
    calcEEW = 0;
    calcIsVSLine = false;
  }

  auto addr = calcAddr;
  auto lowData = calcDataLow;
  auto highData = calcDataHigh;
  auto mask = calcMask;
  auto offset = probe.offset;
  auto eew = calcEEW;
  auto pc = probe.pc;
  auto robIdx = probe.robidx;

  uint64_t rawVecAddr = addr + offset;

  if (calcIsVSLine) {
    uint16_t flow = COMMITBYTES / eew;
    uint64_t flowMask = (eew == 1) ? 0x1ULL : (eew == 2) ? 0x3ULL : (eew == 4) ? 0xfULL : (eew == 8) ? 0xffULL : 0x0ULL;
    uint64_t flowMaskBit = (eew == 1)   ? 0xffULL
                           : (eew == 2) ? 0xffffULL
                           : (eew == 4) ? 0xffffffffULL
                           : (eew == 8) ? 0xffffffffffffffffULL
                                        : 0x0ULL;

    // cross 128bits.
    bool handleMisalign = ((mask << (16 - offset)) & 0xFFFF) != 0;
    // For requests exceeding 128 bits, we perform fragmentation.
    // Processing the high-order bits of data exceeding 128 bits.
    if (handleMisalign) {
      uint64_t selVecData = offset >= 8 ? highData : lowData;
      uint16_t rawOffset = offset % 8;
      uint64_t refStoreCommitData = selVecData << (64 - (rawOffset * 8)) >> (64 - (rawOffset * 8));
      uint8_t refStoreCommitMask = mask << rawOffset >> rawOffset;
      DiffState::StoreCommit storeCommit = {probe.valid,
                                            addr,
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
    for (int i = 0; i < flow; i++) {
      uint32_t rawOffset = eew * i + offset;
      uint32_t nextOffset = rawOffset + eew;

      // to next sbuffer write event.
      if (rawOffset >= 16)
        break;

      uint64_t selVecData = rawOffset >= 8 ? highData : lowData;
      auto dataOffset = (rawOffset * 8) % 64;
      auto refStoreCommitAddr = rawVecAddr + eew * i;
      uint8_t refStoreCommitMask = (mask >> rawOffset) & flowMask;

      if (refStoreCommitMask == 0)
        continue;

      uint64_t refStoreCommitData;
      bool needNextData = (rawOffset < 8) && (nextOffset > 8);

      if (needNextData) {
        auto presentDataOffset = dataOffset;
        auto presentData = lowData >> presentDataOffset;

        nextOffset = 8 - rawOffset;
        auto nextDataOffset = nextOffset * 8;
        auto nextData = calcDataHigh << nextDataOffset;

        refStoreCommitData = (nextData + presentData) & flowMaskBit;
      } else {
        refStoreCommitData = (selVecData >> dataOffset) & flowMaskBit;
      }

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
  } else if (probe.wLine) {
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
    DiffState::StoreCommit storeCommit = {probe.valid,
                                          calcAddr,
                                          lowData,
                                          static_cast<uint8_t>(calcMask & 0xFF),
                                          pc,
                                          robIdx
#ifdef CONFIG_DIFFTEST_SQUASH
                                          ,
                                          probe.stamp
#endif // CONFIG_DIFFTEST_SQUASH
    };
    state->store_event_queue.push(storeCommit);
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
