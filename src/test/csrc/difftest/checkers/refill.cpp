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

#include "checker.h"

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
    return 1;
  }
  static uint64_t last_valid_addr = 0;
  char buf[512];
  char flag_buf[512];
  uint64_t realpaddr = probe.addr;
  probe.addr = probe.addr - probe.addr % 64;
  if (probe.addr != last_valid_addr) {
    last_valid_addr = probe.addr;
    if (!in_pmem(probe.addr)) {
      // speculated illegal mem access should be ignored
      return 0;
    }
    for (int i = 0; i < 8; i++) {
      read_goldenmem(probe.addr + i * 8, &buf, 8, &flag_buf);
      if ((probe.mask & (1 << i)) && probe.data[i] != *((uint64_t *)buf)) {
#ifdef CONFIG_DIFFTEST_CMOINVALEVENT
        if (cmo_inval_event_set.find(probe.addr) != cmo_inval_event_set.end()) {
          // If the data inconsistency occurs in the cache block operated by CBO.INVAL,
          // it is considered reasonable and the DUT data is used to update goldenMem.
          Info("INFO: Sync GoldenMem using refill Data from DUT (Because of CBO.INVAL):\n");
          Info("      cacheid=%d, addr: %lx\n      Gold: ", cacheid, probe.addr);
          for (int j = 0; j < 8; j++) {
            read_goldenmem(probe.addr + j * 8, &buf, 8);
            Info("%016lx", *((uint64_t *)buf));
          }
          Info("\n      Core: ");
          for (int j = 0; j < 8; j++) {
            Info("%016lx", probe.data[j]);
          }
          Info("\n");
          update_goldenmem(probe.addr, probe.data, 0xffffffffffffffffUL, 64);
          proxy->ref_memcpy(probe.addr, probe.data, 64, DUT_TO_REF);
          cmo_inval_event_set.erase(probe.addr);
          return 0;
        } else {
#endif // CONFIG_DIFFTEST_CMOINVALEVENT
#ifdef CONFIG_DIFFTEST_UNCACHEMMSTOREEVENT
          // in multi-core, uncache mm store may cause data inconsistencies.
          // so here needs to override the nemu value with the dut value by cacheline granularity.
          if (*((uint64_t *)flag_buf) != 0) {
            Info("INFO: Sync GoldenMem using refill Data from DUT (Because of uncache main-mem store):\n");
            Info("      cacheid=%d, addr: %lx\n      Gold: ", cacheid, probe.addr);
            for (int j = 0; j < 8; j++) {
              read_goldenmem(probe.addr + j * 8, &buf, 8);
              Info("%016lx", *((uint64_t *)buf));
            }
            Info("\n      Core: ");
            for (int j = 0; j < 8; j++) {
              Info("%016lx", probe.data[j]);
            }
            Info("\n");
            update_goldenmem(probe.addr, probe.data, 0xffffffffffffffffUL, 64);
            proxy->ref_memcpy(probe.addr, probe.data, 64, DUT_TO_REF);
            return 0;
          }
#endif // CONFIG_DIFFTEST_UNCACHEMMSTOREEVENT
          Info("cacheid=%d,mask=%x,realpaddr=0x%lx: Refill test failed!\n", cacheid, probe.mask, realpaddr);
          Info("addr: %lx\nGold: ", probe.addr);
          for (int j = 0; j < 8; j++) {
            read_goldenmem(probe.addr + j * 8, &buf, 8);
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
          return 0;
#ifdef CONFIG_DIFFTEST_CMOINVALEVENT
        }
#endif // CONFIG_DIFFTEST_CMOINVALEVENT
      }
    }
  }

  return 0;
}

#ifdef CONFIG_DIFFTEST_CMOINVALEVENT
bool CmoInvalRecorder::get_valid(const DifftestCmoInvalEvent &probe) {
  return probe.valid;
}

void CmoInvalRecorder::clear_valid(DifftestCmoInvalEvent &probe) {
  probe.valid = 0;
}

int CmoInvalRecorder::check(const DifftestCmoInvalEvent &probe) {
  cmo_inval_event_set.insert(probe.addr);
}
#endif // CONFIG_DIFFTEST_CMOINVALEVENT

#endif // CONFIG_DIFFTEST_REFILLEVENT
