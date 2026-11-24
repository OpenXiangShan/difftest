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
#include "goldenmem.h"

#ifdef CONFIG_DIFFTEST_REFILLEVENT
bool RefillChecker::get_valid(const DifftestRefillEvent &probe) {
  return probe.valid;
}

void RefillChecker::clear_valid(DifftestRefillEvent &probe) {
  probe.valid = 0;
}

int RefillChecker::check(const DifftestRefillEvent &probe) {
  static int delay = 0;
  delay = delay * 2;
  if (delay > 16) {
    return STATE_ERROR;
  }
  static uint64_t last_valid_addr = 0;
  char buf[512];
  char flag_buf[512];
  uint64_t realpaddr = probe.addr;
  uint64_t paddr = probe.addr - probe.addr % 64;

#ifdef CONFIG_DIFFTEST_CMOINVALEVENT
  auto cmo = state->cmo_inval_lines.find(paddr);
  if (cmo != state->cmo_inval_lines.end()) {
    // CBO.INVAL drops the line's dirty data, so after the operation the line
    // holds the memory content. Adopt the refill data, but keep the bytes the
    // REF has stored since then: the refill may have been fetched before those
    // stores were performed. Only refill data is written, so nothing is copied
    // from the REF back into goldenmem.
    const uint64_t ref_written = cmo->second.written;
    last_valid_addr = paddr;
    if (!in_pmem(paddr)) {
      state->cmo_inval_lines.erase(cmo);
      return STATE_OK;
    }
    uint64_t refill_bytes = 0;
    for (int i = 0; i < 8; i++) {
      if (probe.mask & (1U << i)) {
        refill_bytes |= UINT64_C(0xff) << (i * 8);
      }
    }
    const uint64_t sync = refill_bytes & ~cmo->second.written & ~cmo->second.synced;
    // Without per-store events (e.g. a store-hash build) the mask stays zero.
    // In that case keep the original behavior: only adopt the refill data when
    // it disagrees with goldenmem, instead of rewriting the whole line.
    bool needs_sync = (ref_written != 0);
    for (int i = 0; !needs_sync && i < 64; i++) {
      if (((sync >> i) & 1) == 0) {
        continue;
      }
      uint64_t gold = 0;
      read_goldenmem(paddr + i, &gold, 1);
      needs_sync = (uint8_t)gold != ((const uint8_t *)probe.data)[i];
    }
    if (sync != 0 && needs_sync) {
      cmo->second.synced |= sync;
      update_goldenmem(paddr, (void *)probe.data, sync, 64);
      for (int i = 0; i < 64;) {
        if (((sync >> i) & 1) == 0) {
          i++;
          continue;
        }
        int stop = i;
        while (stop < 64 && ((sync >> stop) & 1)) {
          stop++;
        }
        proxy->ref_memcpy(paddr + i, (uint8_t *)probe.data + i, stop - i, DUT_TO_REF);
        i = stop;
      }
    }
    Info("INFO: Sync GoldenMem using refill Data from DUT (Because of CBO.INVAL):\n");
    Info("      cacheid=%d, addr: %lx, kept=%016lx\n", index, paddr, ref_written);
    // A segmented refill keeps the record until the whole line has been seen.
    if (refill_bytes == UINT64_MAX) {
      state->cmo_inval_lines.erase(cmo);
    }
    return STATE_OK;
  }
#endif // CONFIG_DIFFTEST_CMOINVALEVENT

  if (paddr != last_valid_addr) {
    last_valid_addr = paddr;
    if (!in_pmem(paddr)) {
      // speculated illegal mem access should be ignored
      return STATE_OK;
    }
    for (int i = 0; i < 8; i++) {
      read_goldenmem(paddr + i * 8, &buf, 8, &flag_buf);
      if ((probe.mask & (1 << i)) && probe.data[i] != *((uint64_t *)buf)) {
#ifdef CONFIG_DIFFTEST_UNCACHEMMSTOREEVENT
        // in multi-core, uncache mm store may cause data inconsistencies.
        // so here needs to override the nemu value with the dut value by cacheline granularity.
        if (*((uint64_t *)flag_buf) != 0) {
          Info("INFO: Sync GoldenMem using refill Data from DUT (Because of uncache main-mem store):\n");
          Info("      cacheid=%d, addr: %lx\n      Gold: ", index, paddr);
          for (int j = 0; j < 8; j++) {
            read_goldenmem(paddr + j * 8, &buf, 8);
            Info("%016lx", *((uint64_t *)buf));
          }
          Info("\n      Core: ");
          for (int j = 0; j < 8; j++) {
            Info("%016lx", probe.data[j]);
          }
          Info("\n");
          update_goldenmem(paddr, (void *)probe.data, 0xffffffffffffffffUL, 64);
          proxy->ref_memcpy(paddr, (void *)probe.data, 64, DUT_TO_REF);
          return STATE_OK;
        }
#endif // CONFIG_DIFFTEST_UNCACHEMMSTOREEVENT
        Info("cacheid=%d,mask=%x,realpaddr=0x%lx: Refill test failed!\n", index, probe.mask, realpaddr);
        Info("addr: %lx\nGold: ", paddr);
        for (int j = 0; j < 8; j++) {
          read_goldenmem(paddr + j * 8, &buf, 8);
          Info("%016lx", *((uint64_t *)buf));
        }
        Info("\nCore: ");
        for (int j = 0; j < 8; j++) {
          Info("%016lx", probe.data[j]);
        }
        Info("\n");
        // continue run some cycle before aborted to dump wave
        if (delay == 0) {
          delay = 1;
        }
        return STATE_OK;
      }
    }
  }

  return STATE_OK;
}

#ifdef CONFIG_DIFFTEST_CMOINVALEVENT
bool CmoInvalRecorder::get_valid(const DifftestCMOInvalEvent &probe) {
  return probe.valid;
}

void CmoInvalRecorder::clear_valid(DifftestCMOInvalEvent &probe) {
  probe.valid = 0;
}

int CmoInvalRecorder::check(const DifftestCMOInvalEvent &probe) {
  // CBO.INVAL leaves the line with the memory content, which the REF only sees
  // when the line is refilled. Start tracking the bytes the REF stores to the
  // line in the meantime, so the refill does not revert them.
  state->cmo_inval_lines[probe.addr & ~UINT64_C(63)] = {};
  return STATE_OK;
}
#endif // CONFIG_DIFFTEST_CMOINVALEVENT

#endif // CONFIG_DIFFTEST_REFILLEVENT
