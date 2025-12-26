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

#ifdef CONFIG_DIFFTEST_LOADEVENT
bool LoadChecker::get_valid(const DifftestLoadEvent &probe) {
  return probe.valid;
}
void LoadChecker::clear_valid(DifftestLoadEvent &probe) {
  probe.valid = 0;
}

int LoadChecker::check(const DifftestLoadEvent &probe) {
  const auto &dut = get_dut_state();

#ifdef CONFIG_DIFFTEST_SQUASH
  state->load_event_queue.push(probe);
  return STATE_OK;
#else
  if (probe.isVLoad) {
#ifdef CONFIG_DIFFTEST_ARCHVECREGSTATE
#ifdef CONFIG_DIFFTEST_VECCOMMITDATA
    auto commitData = dut.vec_commit_data[index].data;
#else
    uint64_t commitData[16];
    for (int j = 0; j < 16; j++) {
      commitData[j] = dut.pregs_vrf.value[dut.commit[index].otherwpdest[j]];
    }
#endif // CONFIG_DIFFTEST_VECCOMMITDATA
    return do_vec_load_check(probe, dut.commit[index].wdest, commitData);
#else
    Info("isVLoad should never be set if vector is not enabled\n");
    return STATE_ERROR;
#endif // CONFIG_DIFFTEST_ARCHVECREGSTATE
  }

  bool regWen = ((dut.commit[index].rfwen && dut.commit[index].wdest != 0) || dut.commit[index].fpwen) &&
                !dut.commit[index].vecwen;
  auto refRegPtr = proxy->arch_reg(dut.commit[index].wdest, dut.commit[index].fpwen);
  auto commitData = get_commit_data(&dut, index);
  return do_load_check(probe, regWen, refRegPtr, commitData);
#endif // CONFIG_DIFFTEST_SQUASH
}

#ifdef CONFIG_DIFFTEST_SQUASH
bool LoadSquashChecker::get_valid() {
  return !state->load_event_queue.empty() && state->load_event_queue.front().stamp == state->commit_stamp;
}

void LoadSquashChecker::clear_valid() {
  state->load_event_queue.pop();
}

int LoadSquashChecker::check() {
  auto &probe = state->load_event_queue.front();
  bool regWen = probe.regWen;
  auto refRegPtr = proxy->arch_reg(probe.wdest, probe.fpwen);
  auto commitData = probe.commitData;
  do_load_check(probe, regWen, refRegPtr, commitData);
}
#endif // CONFIG_DIFFTEST_SQUASH

#ifdef CONFIG_DIFFTEST_ARCHVECREGSTATE
int LoadChecker::do_vec_load_check(const DifftestLoadEvent &probe, uint8_t firstLdest, const uint64_t *commitData) {
  if (!enable_vec_load_goldenmem_check) {
    return STATE_OK;
  }

  // ===============================================================
  //                      Comparison data
  // ===============================================================
  uint32_t vdNum = proxy->get_ref_vdNum();

  proxy->sync();

  bool reg_mismatch = false;

  for (int vdidx = 0; vdidx < vdNum; vdidx++) {
    auto vecNextLdest = firstLdest + vdidx;
    for (int i = 0; i < VLENE_64; i++) {
      uint64_t dutRegData = commitData[VLENE_64 * vdidx + i];
      uint64_t *refRegPtr = proxy->arch_vecreg(VLENE_64 * vecNextLdest + i);
      reg_mismatch |= dutRegData != *refRegPtr;
    }
  }

  // ===============================================================
  //                      Regs Mismatch handle
  // ===============================================================
  bool goldenmem_mismatch = false;

  if (reg_mismatch) {
    // ===============================================================
    //                      Check golden memory
    // ===============================================================
    uint64_t *vec_goldenmem_regPtr = (uint64_t *)proxy->get_vec_goldenmem_reg();

    if (vec_goldenmem_regPtr == nullptr) {
      Info("Vector Load comparison failed and no consistency check with golden mem was performed.\n");
      return STATE_ERROR;
    }

    for (int vdidx = 0; vdidx < vdNum; vdidx++) {
      for (int i = 0; i < VLENE_64; i++) {
        uint64_t dutRegData = commitData[VLENE_64 * vdidx + i];
        goldenmem_mismatch |= dutRegData != vec_goldenmem_regPtr[VLENE_64 * vdidx + i];
      }
    }

    if (!goldenmem_mismatch) {
      // ===============================================================
      //                      sync memory and regs
      // ===============================================================
      proxy->vec_update_goldenmem();

      for (int vdidx = 0; vdidx < vdNum; vdidx++) {
        auto vecNextLdest = firstLdest + vdidx;
        for (int i = 0; i < VLENE_64; i++) {
          uint64_t dutRegData = commitData[VLENE_64 * vdidx + i];
          uint64_t *refRegPtr = proxy->arch_vecreg(VLENE_64 * vecNextLdest + i);
          *refRegPtr = dutRegData;
        }
      }

      proxy->sync(true);
    } else {
      Info("Vector Load register and golden memory mismatch\n");
      return STATE_ERROR;
    }
  }

  return STATE_OK;
}
#endif // CONFIG_DIFFTEST_ARCHVECREGSTATE

int LoadChecker::do_load_check(const DifftestLoadEvent &probe, bool regWen, uint64_t *refRegPtr, uint64_t commitData) {
  if (probe.isLoad || probe.isAtomic) {
    proxy->sync();
    if (regWen && *refRegPtr != commitData) {
      uint64_t golden;
      uint64_t golden_flag;
      uint64_t mask = 0xFFFFFFFFFFFFFFFF;
      int len = 0;
      if (probe.isLoad) {
        switch (probe.opType) {
          case 0:  // lb
          case 4:  // lbu
          case 16: // hlvb
          case 20: // hlvbu
            len = 1;
            break;

          case 1:  // lh
          case 5:  // lhu
          case 17: // hlvh
          case 21: // hlvhu
          case 29: // hlvxhu
            len = 2;
            break;

          case 2:  // lw
          case 6:  // lwu
          case 18: // hlvw
          case 22: // hlvwu
          case 30: // hlvxwu
            len = 4;
            break;

          case 3:  // ld
          case 19: // hlvd
            len = 8;
            break;

          default: Info("Unknown fuOpType: 0x%x\n", probe.opType); return STATE_ERROR;
        }
      } else if (probe.isAtomic) {
        if (probe.opType % 2 == 0) {
          len = 4;
        } else { // probe.opType % 2 == 1
          len = 8;
        }
      }
      read_goldenmem(probe.paddr, &golden, len, &golden_flag);
      if (probe.isLoad) {
        switch (len) {
          case 1:
            golden = (int64_t)(int8_t)golden;
            golden_flag = (int64_t)(int8_t)golden_flag;
            mask = (uint64_t)(0xFF);
            break;
          case 2:
            golden = (int64_t)(int16_t)golden;
            golden_flag = (int64_t)(int16_t)golden_flag;
            mask = (uint64_t)(0xFFFF);
            break;
          case 4:
            golden = (int64_t)(int32_t)golden;
            golden_flag = (int64_t)(int32_t)golden_flag;
            mask = (uint64_t)(0xFFFFFFFF);
            break;
        }
      }
      if (golden == commitData || probe.isAtomic) { //  atomic instr carefully handled
        proxy->ref_memcpy(probe.paddr, &golden, len, DUT_TO_REF);
        if (regWen) {
          *refRegPtr = commitData;
          proxy->sync(true);
        }
      } else if (probe.isLoad && golden_flag != 0) {
        // goldenmem check failed, but the flag is set, so use DUT data to reset
        Info("load check of uncache mm store flag\n");
        Info("  DUT data: 0x%lx, regWen: %d, refRegPtr: 0x%lx\n", commitData, regWen, refRegPtr);
        proxy->ref_memcpy(probe.paddr, &commitData, len, DUT_TO_REF);
        update_goldenmem(probe.paddr, &commitData, mask, len);
        if (regWen) {
          *refRegPtr = commitData;
          proxy->sync(true);
        }
      } else {
#ifdef DEBUG_SMP
        // goldenmem check failed as well, raise error
        Info("---  SMP difftest mismatch!\n");
        Info("---  Trying to probe local data of another core\n");
        uint64_t buf;
        difftest[(NUM_CORES - 1) - state->coreid]->proxy->memcpy(probe.paddr, &buf, len, DIFFTEST_TO_DUT);
        Info("---    content: %lx\n", buf);
        return STATE_ERROR;
#else
        proxy->ref_memcpy(probe.paddr, &golden, len, DUT_TO_REF);
        if (regWen) {
          *refRegPtr = commitData;
          proxy->sync(true);
        }
#endif
      }
    }
  }

  return STATE_OK;
}

#endif // CONFIG_DIFFTEST_LOADEVENT
