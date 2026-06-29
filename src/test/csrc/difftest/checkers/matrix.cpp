/***************************************************************************************
* Copyright (c) 2020-2025 Institute of Computing Technology, Chinese Academy of Sciences
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

#ifdef CONFIG_DIFFTEST_AMUCTRLEVENT

#include <cassert>

#ifndef CONFIG_DIFF_AMU_ARLEN
#error "Missing CONFIG_DIFF_AMU_ARLEN: should be generated from hardware TLEN."
#endif

#ifndef CONFIG_DIFF_AMU_TRLEN
#error "Missing CONFIG_DIFF_AMU_TRLEN: should be generated from hardware TRLEN."
#endif

#ifndef CONFIG_DIFF_AMU_BANK_WIDTH
#error "Missing CONFIG_DIFF_AMU_BANK_WIDTH: should be generated from AmuFinishEvent layout."
#endif

#ifndef CONFIG_DIFF_AMU_FINISH_BANKS
#error "Missing CONFIG_DIFF_AMU_FINISH_BANKS: should be generated from AmuFinishEvent layout."
#endif

#ifndef CONFIG_DIFF_AMU_FINISH_WORDS_PER_BANK
#error "Missing CONFIG_DIFF_AMU_FINISH_WORDS_PER_BANK: should be generated from AmuFinishEvent layout."
#endif

#ifndef CONFIG_DIFF_AMU_AB_WORDS_PER_BANK
#error "Missing CONFIG_DIFF_AMU_AB_WORDS_PER_BANK: should be generated from AB matrix-register layout."
#endif

#ifndef CONFIG_DIFF_AMU_C_WORDS_PER_BANK
#error "Missing CONFIG_DIFF_AMU_C_WORDS_PER_BANK: should be generated from C matrix-register layout."
#endif

#ifndef CONFIG_DIFF_AMU_AB_REG_SIZE_BYTES
#error "Missing CONFIG_DIFF_AMU_AB_REG_SIZE_BYTES: should be generated from AB matrix-register size."
#endif

#ifndef CONFIG_DIFF_AMU_C_REG_SIZE_BYTES
#error "Missing CONFIG_DIFF_AMU_C_REG_SIZE_BYTES: should be generated from C matrix-register size."
#endif

static_assert(CONFIG_DIFF_AMU_FINISH_BANKS > 0, "CONFIG_DIFF_AMU_FINISH_BANKS should be positive");
static_assert(CONFIG_DIFF_AMU_FINISH_WORDS_PER_BANK > 0, "CONFIG_DIFF_AMU_FINISH_WORDS_PER_BANK should be positive");
static_assert(CONFIG_DIFF_AMU_AB_WORDS_PER_BANK > 0, "CONFIG_DIFF_AMU_AB_WORDS_PER_BANK should be positive");
static_assert(CONFIG_DIFF_AMU_C_WORDS_PER_BANK > 0, "CONFIG_DIFF_AMU_C_WORDS_PER_BANK should be positive");
static_assert(CONFIG_DIFF_AMU_AB_WORDS_PER_BANK <= CONFIG_DIFF_AMU_FINISH_WORDS_PER_BANK,
              "AB words-per-bank should not exceed event payload words-per-bank");
static_assert(CONFIG_DIFF_AMU_C_WORDS_PER_BANK <= CONFIG_DIFF_AMU_FINISH_WORDS_PER_BANK,
              "C words-per-bank should not exceed event payload words-per-bank");
static_assert(CONFIG_DIFF_AMU_AB_REG_SIZE_BYTES > 0, "CONFIG_DIFF_AMU_AB_REG_SIZE_BYTES should be positive");
static_assert(CONFIG_DIFF_AMU_C_REG_SIZE_BYTES > 0, "CONFIG_DIFF_AMU_C_REG_SIZE_BYTES should be positive");

static char amu_ctrl_op_str[4][16] = {"MMA", "MLS", "MRELEASE", "MARITH"};

static inline size_t get_amu_result_size(const DifftestAmuCtrlEvent &amu_event) {
  const size_t rows = amu_event.mtilem;
  const size_t cols = amu_event.mtilen;
  const size_t element_size = get_element_size(amu_event.typed);

  switch (amu_event.op) {
    case 0: // MMA
      return rows * cols * element_size;
    case 1: // Matrix load/store
      // Keep old behavior: only alloc in load-like path.
      return (amu_event.sat == 0) ? rows * cols * element_size : 0;
    case 3: // Matrix Arith
      // MARITH writes a whole matrix register selected by md:
      // md < 4  -> tile register (AB), md >= 4 -> accumulator register (C).
      return (amu_event.md < 4) ? CONFIG_DIFF_AMU_AB_REG_SIZE_BYTES : CONFIG_DIFF_AMU_C_REG_SIZE_BYTES;
    default:
      return 0;
  }
}

// 1. Capture AME commit from DUT ROB → push to software ROB (WAIT_REF_COMMIT)
bool AmuCtrlRecorder::get_valid(const DifftestAmuCtrlEvent &probe) {
  return probe.valid;
}

void AmuCtrlRecorder::clear_valid(DifftestAmuCtrlEvent &probe) {
  probe.valid = 0;
}

int AmuCtrlRecorder::check(const DifftestAmuCtrlEvent &probe) {
  DiffState::AmeInstRobEntry entry;
  entry.amu_event = probe;
  entry.state = DiffState::WAIT_REF_COMMIT;
  entry.res = NULL;
  state->matrix_sw_rob.push_back(entry);
  return STATE_OK;
}

// 2. When REF commits same inst, compare amu_ctrl → on match set WAIT_DUT_EXEC
int AmuCtrlChecker::do_step() {
  for (auto iter = state->matrix_sw_rob.begin(); iter != state->matrix_sw_rob.end(); ++iter) {
    if (iter->state == DiffState::WAIT_REF_COMMIT) {
      DifftestAmuCtrlEvent amu_event = iter->amu_event;
      // NOTE:
      // AMU ctrl is treated as a non-squashable, ordering-sensitive event.
      // Under squash mode, it still requires one-to-one synchronization with REF.
      // Therefore "no available REF event" (-1) is an error instead of a retriable case.
      auto rm = amu_event.rm;
      auto op = amu_event.op;
      auto md = amu_event.md;
      auto sat = amu_event.sat;
      auto ms1 = amu_event.ms1;
      auto ms2 = amu_event.ms2;
      auto mtilem = amu_event.mtilem;
      auto mtilen = amu_event.mtilen;
      auto mtilek = amu_event.mtilek;
      auto types1 = amu_event.types1;
      auto types2 = amu_event.types2;
      auto typed = amu_event.typed;
      auto transpose = amu_event.isfp;
      auto base = amu_event.base;
      auto stride = amu_event.stride;
      auto isfp = amu_event.isfp;
      uint64_t pc = amu_event.pc;

      int check_res = proxy->get_amu_ctrl_event(&amu_event);

      if (check_res == 1) {
        printf("\n==============  Amu Mma Ctrl Event (Core %d)  ==============\n", state->coreid);
        printf("Mismatch for amu ctrl event \n");
        printf("  REF AMU ctrl: pc 0x%016lx, op %s\n", amu_event.pc, amu_ctrl_op_str[amu_event.op]);
        switch (amu_event.op) {
          case 0: // MMA
            printf("                md %d, rm %d, sat %d, ms1 %d, ms2 %d, fp %d,\n"
                   "                mtilem %d, mtilen %d, mtilek %d, types1 %d, types2 %d, typed %d\n",
                   amu_event.md, amu_event.rm, amu_event.sat, amu_event.ms1, amu_event.ms2, amu_event.isfp,
                   amu_event.mtilem, amu_event.mtilen, amu_event.mtilek, amu_event.types1, amu_event.types2,
                   amu_event.typed);
            break;
          case 1: // MLS
            printf("                md %d, ls %d, transpose %d, isacc %d,\n"
                   "                base %016lx, stride %016lx, row %d, column %d, widths %d\n",
                   amu_event.md, amu_event.sat, amu_event.isfp, amu_event.types1, amu_event.base, amu_event.stride,
                   amu_event.mtilem, amu_event.mtilen, amu_event.typed);
            break;
          case 2: // MRelease
            printf("                msyncRd %d\n", amu_event.mtilem);
            break;
          case 3: // Arith
            printf("                md %d, opType %#lx\n", amu_event.md, amu_event.base);
            break;
          default:
            printf("                Unknown amu event op\n");
            break;
        }
        printf("  DUT AMU ctrl: pc 0x%016lx, op %s\n", pc, amu_ctrl_op_str[op]);
        switch (op) {
          case 0: // MMA
            printf("                md %d, rm %d, sat %d, ms1 %d, ms2 %d, fp %d,\n"
                   "                mtilem %d, mtilen %d, mtilek %d, types1 %d, types2 %d, typed %d\n",
                   md, rm, sat, ms1, ms2, isfp, mtilem, mtilen, mtilek, types1, types2, typed);
            break;
          case 1: // MLS
            printf("                md %d, ls %d, transpose %d, isacc %d,\n"
                   "                base %016lx, stride %016lx, row %d, column %d, widths %d\n",
                   md, sat, transpose, types1, base, stride, mtilem, mtilen, typed);
            break;
          case 2: // MRelease
            printf("                msyncRd %d\n", mtilem);
            break;
          case 3: // Arith
            printf("                md %d, opType %#lx\n", md, base);
            break;
          default:
            printf("                Unknown amu event op\n");
            break;
        }
        return STATE_ERROR;
      } else if (check_res == -1) {
        printf("\n==============  Amu Mma Ctrl Event (Core %d)  ==============\n", state->coreid);
        printf("  No available REF AMU ctrl\n");
        printf("  DUT AMU ctrl: pc 0x%016lx, op %s\n", pc, amu_ctrl_op_str[op]);
        switch (op) {
          case 0: // MMA
            printf("                md %d, rm %d, sat %d, ms1 %d, ms2 %d, fp %d,\n"
                   "                mtilem %d, mtilen %d, mtilek %d, types1 %d, types2 %d, typed %d\n",
                   md, rm, sat, ms1, ms2, isfp, mtilem, mtilen, mtilek, types1, types2, typed);
            break;
          case 1: // MLS
            printf("                md %d, ls %d, transpose %d, isacc %d,\n"
                   "                base %016lx, stride %016lx, row %d, column %d, widths %d\n",
                   md, sat, transpose, types1, base, stride, mtilem, mtilen, typed);
            break;
          case 2: // MRelease
            printf("                msyncRd %d\n", mtilem);
            break;
          case 3: // Arith
            printf("                md %d, opType %#lx\n", md, base);
            break;
          default:
            printf("                Unknown amu event op\n");
            break;
        }
        return STATE_ERROR;
      } else {
        iter->state = DiffState::WAIT_DUT_EXEC;
      }
    }
  }
  return STATE_OK;
}

// 3. Capture DUT matrix writeback, update register copy → on finish set WAIT_SWROB_COMMIT
bool AmuExecRecorder::get_valid(const DifftestAmuFinishEvent &probe) {
  return probe.valid;
}

void AmuExecRecorder::clear_valid(DifftestAmuFinishEvent &probe) {
  probe.valid = 0;
}

int AmuExecRecorder::check(const DifftestAmuFinishEvent &probe) {
  const static size_t ARLen = CONFIG_DIFF_AMU_ARLEN;
  const static size_t TRLen = CONFIG_DIFF_AMU_TRLEN;
  const static size_t bankWidth = CONFIG_DIFF_AMU_BANK_WIDTH;
  for (auto &entry : state->matrix_sw_rob) {
    if (entry.amu_event.pc == probe.pc && entry.state == DiffState::WAIT_DUT_EXEC) {
      const size_t matrix_size = get_amu_result_size(entry.amu_event);
      size_t matrix_u64_size = (matrix_size + sizeof(uint64_t) - 1) / sizeof(uint64_t);
      if (matrix_u64_size == 0) {
        matrix_u64_size = 1;
      }
      if (entry.res == NULL) {
        // first `valid` for this inst: alloc space for matrix inst
        entry.res = new uint64_t[matrix_u64_size];
        memset(entry.res, 0, matrix_u64_size * sizeof(uint64_t));
      }
      uint8_t md = entry.amu_event.md;
      size_t stride = 0;
      if (md < 4) {
        stride = TRLen / bankWidth;
      } else {
        stride = ARLen / bankWidth;
      }
      const size_t matrix_words_per_bank = (md < 4) ? CONFIG_DIFF_AMU_AB_WORDS_PER_BANK : CONFIG_DIFF_AMU_C_WORDS_PER_BANK;
      assert(stride > 0);
      assert(matrix_words_per_bank > 0);

      for (int j = 0; j < CONFIG_DIFF_AMU_FINISH_BANKS; ++j) {  // for each bank
        if (probe.bankValid[j]) {
          const size_t addr = probe.bankAddr[j];
          const size_t matrix_entry =
              addr / stride * stride * CONFIG_DIFF_AMU_FINISH_BANKS + j * stride + addr % stride;
          const size_t idx = matrix_entry * matrix_words_per_bank;
          assert(idx + matrix_words_per_bank <= matrix_u64_size);

          uint8_t *dst = reinterpret_cast<uint8_t *>(&entry.res[idx]);
          const uint8_t *src = reinterpret_cast<const uint8_t *>(
              &probe.data[j * CONFIG_DIFF_AMU_FINISH_WORDS_PER_BANK]);
          const uint64_t mask = probe.bankMask[j];
          const size_t bank_bytes = matrix_words_per_bank * sizeof(uint64_t);
          for (size_t k = 0; k < bank_bytes; ++k) {
            if ((mask >> k) & 0x1U) {
              dst[k] = src[k];
            }
          }
        }
      }
      if (probe.finish) {
        entry.state = DiffState::WAIT_SWROB_COMMIT;
      }
      break;
    }
  }
  return STATE_OK;
}

// 4. Commit ready inst from software ROB: REF re-exec and compare, then release
int AmuExecChecker::do_step() {
  Difftest *dt = difftest[state->coreid];
  MmaVerifier *mma_verifier = dt->get_mma_verifier();
  // For each amu ctrl in sw_rob, check whether the inst is able to be committed.
  for (auto iter = state->matrix_sw_rob.begin(); iter != state->matrix_sw_rob.end();) {
    if (iter->state == DiffState::WAIT_SWROB_COMMIT) {
      DifftestAmuCtrlEvent amu_event = iter->amu_event;
      uint8_t op = amu_event.op;
      MmaVerificationBuffer *buffer = nullptr;
      switch (op) {
        case 0: // MMA
          // Allocate buffer for MMA verification
          buffer = mma_verifier->allocate_buffer(&amu_event);
          // Store DUT result in the buffer
          memcpy(buffer->dut_result, iter->res,
                 amu_event.mtilem * amu_event.mtilen * get_element_size(amu_event.typed));
          // Call get_amu_lazy with buffer pointers
          // Store REF's src1/2/3 in the buffer, and copy DUT's result to REF
          // REF will directly take DUT's result instead of executing the MMA instruction
          proxy->get_amu_lazy(&amu_event, iter->res, buffer->src1, buffer->src2, buffer->src3);
          // Pass buffer to verification thread
          mma_verifier->add_to_verification_queue(buffer);
          delete[] iter->res;
          break;
        case 1: // MLS
        case 2: // MRelease
        case 3: // Arith
          if (proxy->get_amu_exec(&amu_event, iter->res) == 1) {
            printf("Mismatch for amu exec event: pc 0x%016lx, op %s\n", amu_event.pc,
                   amu_ctrl_op_str[amu_event.op]);
            if (iter->res != nullptr) {
              delete[] iter->res;
              iter->res = nullptr;
            }
            return STATE_ERROR;
          }
          if (iter->res != nullptr) {
            delete[] iter->res;
            iter->res = nullptr;
          }
          break;
        default:
          printf("Unknown amu event op: %d\n", op);
          if (iter->res != nullptr) {
            delete[] iter->res;
            iter->res = nullptr;
          }
          return STATE_ERROR;
      }
      // Remove the processed event from matrix_sw_rob
      iter = state->matrix_sw_rob.erase(iter);
    } else {
      // When there's an unfinished inst before the current inst, the current inst will not be committed.
      // So, when the current inst is unfinished, break the loop.
      break;
    }
  }
  return STATE_OK;
}

#endif // CONFIG_DIFFTEST_AMUCTRLEVENT

#ifdef CONFIG_DIFFTEST_MSYNCEVENT

static char msync_event_op_str[3][16] = {"msyncregreset", "macquire", "mfence"};

int MsyncChecker::do_step() {
  while (!state->msync_event_queue.empty()) {
    DifftestMsyncEvent msync_event = state->msync_event_queue.front();
#ifdef CONFIG_DIFFTEST_SQUASH
    // TODO: What is squash? How to squash?
#endif // CONFIG_DIFFTEST_SQUASH
    // Save the msync event info.
    auto op = msync_event.op;
    auto msyncRd = msync_event.msyncRd;
    uint64_t pc = msync_event.pc;

    int check_res = proxy->get_msync_event(&msync_event);

    if (check_res == 1) {
      // Compare the msync event info.
      // For mismatch, proxy returns 1 and sets the msync event info.
      // Use global difftest pointer to access the owning Difftest instance for this core
      printf("\n==============  Msync Event (Core %d)  ==============\n", state->coreid);
      printf("Mismatch for Msync event\n");
      printf("  REF Msync: pc 0x%016lx, op %s, MsyncRd %d\n", msync_event.pc,
             msync_event_op_str[msync_event.op], msync_event.msyncRd);
      printf("  DUT Msync: pc 0x%016lx, op %s, MsyncRd %d\n", pc, msync_event_op_str[op], msyncRd);
      state->msync_event_queue.pop();
      return STATE_ERROR;
    } else if (check_res == -1) {
      printf("\n==============  Msync Event (Core %d)  ==============\n", state->coreid);
      printf("  No available REF Msync events\n");
      printf("  DUT Msync: pc 0x%016lx, op %s, MsyncRd %d\n", pc, msync_event_op_str[op], msyncRd);
      state->msync_event_queue.pop();
      return STATE_ERROR;
    }
    state->msync_event_queue.pop();
  }
  return STATE_OK;
}

bool MsyncRecorder::get_valid(const DifftestMsyncEvent &probe) {
  return probe.valid;
}

void MsyncRecorder::clear_valid(DifftestMsyncEvent &probe) {
  probe.valid = 0;
}

int MsyncRecorder::check(const DifftestMsyncEvent &probe) {
  state->msync_event_queue.push(probe);
  return STATE_OK;
}

#endif // CONFIG_DIFFTEST_MSYNCEVENT
