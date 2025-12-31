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

#ifdef CONFIG_DIFFTEST_STOREEVENT
bool StoreRecorder::get_valid(const DifftestStoreEvent &probe) {
  return probe.valid;
}
void StoreRecorder::clear_valid(DifftestStoreEvent &probe) {
  probe.valid = 0;
}

int StoreRecorder::check(const DifftestStoreEvent &probe) {
  state->store_event_queue.push(probe);
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
      if (in_disambiguation_state()) {
        Info("Store mismatch detected with a disambiguation state at pc = 0x%lx.\n", dut->trap.pc);
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
