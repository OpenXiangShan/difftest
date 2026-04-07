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

static char amu_ctrl_op_str[4][16] = {"MMA", "MLS", "MRELEASE", "MARITH"};

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
            printf("                tokenRd %d\n", amu_event.mtilem);
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
            printf("                tokenRd %d\n", mtilem);
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
            printf("                tokenRd %d\n", mtilem);
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
  const static size_t ARLen = 128 * 32;
  const static size_t TRLen = 64 * 8;
  const static size_t ABankWidth = 256;
  const static size_t TBankWidth = 256;
  for (auto &entry : state->matrix_sw_rob) {
    if (entry.amu_event.pc == probe.pc && entry.state == DiffState::WAIT_DUT_EXEC) {
      if (entry.res == NULL) {
        // first `valid` for this inst: alloc space for matrix inst
        size_t matrix_size = 0;
        switch (entry.amu_event.op) {
          // TODO: do not use hardcoded matrix size here
          case 0: // MMA
            matrix_size = 128 * 128 * 4;
            break;
          case 1: // Matrix load/store
          case 3: // Matrix Arith
            if (entry.amu_event.sat == 0) {  // load
              if ((entry.amu_event.md & 4) == 0) {  // tile register
                matrix_size = 128 * 64 * 1;
              } else {  // acc register
                matrix_size = 128 * 128 * 4;
              }
            }
            break;
          default:
            break;
        }
        entry.res = new uint64_t[matrix_size / 8];
        memset(entry.res, 0, matrix_size);
      }
      uint8_t md = entry.amu_event.md;
      size_t stride = 0;
      if (md < 4) {
        stride = TRLen / TBankWidth;
      } else {
        stride = ARLen / ABankWidth;
      }
      for (int j = 0; j < 8; ++j) {  // for each bank
        if (probe.bankValid[j]) {
          size_t addr = probe.bankAddr[j];
          for (int k = 0; k < 4; ++k) {
            entry.res[(addr / stride * stride * 8 + j * stride + addr % stride) * 4 + k] =
                probe.data[j * 4 + k];
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
          proxy->get_amu_lazy(&amu_event, iter->res, buffer->src1, buffer->src2, buffer->src3);
          // Pass buffer to verification thread
          mma_verifier->add_to_verification_queue(buffer);
          break;
        case 1: // MLS
        case 2: // MRelease
        case 3: // Arith
          if (proxy->get_amu_exec(&amu_event, iter->res) == 1) {
            printf("Mismatch for amu exec event: pc 0x%016lx, op %s\n", amu_event.pc,
                   amu_ctrl_op_str[amu_event.op]);
            return STATE_ERROR;
          }
          break;
        default:
          printf("Unknown amu event op: %d\n", op);
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

#ifdef CONFIG_DIFFTEST_TOKENEVENT

static char token_event_op_str[2][16] = {"msyncregreset", "macquire"};

int TokenChecker::do_step() {
  while (!state->token_event_queue.empty()) {
    DifftestTokenEvent token_event = state->token_event_queue.front();
#ifdef CONFIG_DIFFTEST_SQUASH
    // TODO: What is squash? How to squash?
#endif // CONFIG_DIFFTEST_SQUASH
    // Save the token event info
    auto op = token_event.op;
    auto tokenRd = token_event.tokenRd;
    uint64_t pc = token_event.pc;

    int check_res = proxy->get_token_event(&token_event);

    if (check_res == 1) {
      // Compare the token event info
      // For dismatch, proxy returns 1 and sets the token event info
      // Use global difftest pointer to access the owning Difftest instance for this core
      printf("\n==============  Token Event (Core %d)  ==============\n", state->coreid);
      printf("Mismatch for token event\n");
      printf("  REF Token: pc 0x%016lx, op %s, tokenRd %d\n", token_event.pc,
             token_event_op_str[token_event.op], token_event.tokenRd);
      printf("  DUT Token: pc 0x%016lx, op %s, tokenRd %d\n", pc, token_event_op_str[op], tokenRd);
      state->token_event_queue.pop();
      return STATE_ERROR;
    } else if (check_res == -1) {
      printf("\n==============  Token Event (Core %d)  ==============\n", state->coreid);
      printf("  No available REF Token\n");
      printf("  DUT Token: pc 0x%016lx, op %s, tokenRd %d\n", pc, token_event_op_str[op], tokenRd);
      state->token_event_queue.pop();
      return STATE_ERROR;
    }
    state->token_event_queue.pop();
  }
  return STATE_OK;
}

bool TokenRecorder::get_valid(const DifftestTokenEvent &probe) {
  return probe.valid;
}

void TokenRecorder::clear_valid(DifftestTokenEvent &probe) {
  probe.valid = 0;
}

int TokenRecorder::check(const DifftestTokenEvent &probe) {
  state->token_event_queue.push(probe);
  return STATE_OK;
}

#endif // CONFIG_DIFFTEST_TOKENEVENT
