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
#include <vector>
#include <cstdlib>
#include "trace_inst_dedup.h"

int TraceInstLoopDedup::nextAvailableIdx(std::vector<Instruction> &src, int idx, uint32_t len) {
  for (; idx < srcSize;) {
    if (src[idx].is_squashed) {
      idx++;
    } else if (src[idx].isLoopFirstInst) {
      if (src[idx].loopBodyLength == len) {
        return idx;
      } else {
        // NOTE: the last loop may not finish, cause the loopBodyLength is not set
        idx += std::max((int)src[idx].loopBodyLength, 1);
      }
    } else {
      idx ++;
    }
  }
  return idx;
}


// copy len un-squashed data from src to data, starting from idx
// return the starting index of next un-squashed data
int TraceInstLoopDedup::readNData(std::vector<Instruction> &src, int idx, uint32_t len, Instruction *data) {
  for (uint32_t i = 0; i < len && idx < srcSize; idx++) {
    if (src[idx].is_squashed) {
      continue;
    } else if (i == 0 && !(src[idx].isLoopFirstInst && src[idx].loopBodyLength == len)) {
      // must start with loop first inst
      return -1;
    }
    data[i] = src[idx];
    i++;
  }
  // must end with branch/jump
  if ((data[len-1].branch_type == 0) ||
      (data[len-1].branch_taken == 0) ||
      (data[len-1].target != data[0].instr_pc_va)
    ) return -1;
  return idx;
}

inline bool TraceInstLoopDedup::matchOneInst(Instruction &a, Instruction &b) {
  return  (a.instr_pc_va == b.instr_pc_va) && (a.instr == b.instr);
}

// find one Match
// if found, return the starting index of next src index
// if not found, return -1
int TraceInstLoopDedup::findOneMatch(std::vector<Instruction> &src, uint32_t idx, uint32_t len, Instruction *data) {
  uint32_t matchIdx = 0;
  for (; matchIdx < len && idx < srcSize; idx++) {
    if (src[idx].is_squashed) continue;
    // if (src[idx].branch_type == 0) matchIdx++;
    if (matchOneInst(src[idx], data[matchIdx])) matchIdx++;
    else return -1;
  }

  if (matchIdx == len) return idx;
  else return -1;
}

// skip the first len-1 and last one
// for example, if RemainNum = 3, num = 4, keep the 0,1,3, set 2 as squashed
void TraceInstLoopDedup::setNSquashedButSkip(std::vector<Instruction> &src, int idx, uint32_t len, uint32_t num, int beginSkip, int endSkip) {
  // printf("setNSquashedButSkip idx: %d len: %d num: %d beginSkip: %d endSkip: %d\n", idx, len, num, beginSkip, endSkip);
  for (int matchNum = 0; matchNum < (num-endSkip); matchNum++) {
    // printf("  matchNum: %d\n", matchNum);
    int bodyStartNum = 0;
    int oldIdx = idx;
    for (uint32_t matchIdx = 0; matchIdx < len && idx < srcSize; idx++) {
      // printf("    matchIdx: %d idx: %d ", matchIdx, idx);
      if (src[idx].is_squashed) {
        // printf(" squashed\n");
        continue;
      }
      // printf(" not squashed,");
      matchIdx++;
      // beginSkip + 1, because the original one is already skipped
      if (src[idx].isLoopFirstInst) {
        bodyStartNum ++;
      }
      if (matchNum >= beginSkip) {
        // printf(" set squashed");
        src[idx].is_squashed = true;
      } else {
        // printf(" begin skipped");
      }
      // printf("\n");
    }
    if (bodyStartNum != 1) {
      printf("Error: bodyStartNum %d idx: %d\n", bodyStartNum, idx);
      for (int i = oldIdx; i < idx; i++) {
        src[i].dump();
      }
      // int pNum = 100;
      // for (int i = 0; i < pNum; i++) {
      //   src[idx-pNum+i].dump();
      // }
      exit(1);
    }
  }
}


// return next idx
int TraceInstLoopDedup::countAndSetMatchNum(std::vector<Instruction> &src, int idx, uint32_t len, Instruction *data, uint32_t &squashMatchNumSum) {
  uint32_t matchNum = 1; // the first one is the original one
  int origin_idx = idx;
  while (idx < srcSize) {
    int nextIdx = findOneMatch(src, idx, len, data);
    // match false, break
    if (nextIdx == -1) break;
    // match true, set matchNum
    matchNum ++;
    idx = nextIdx;
  }
  // printf("countAndSetMatchNum origin_idx: %d len: %d matchNum: %d newIdx %d\n", origin_idx, len, matchNum, idx);
  if (matchNum > RemainNum) {
    squashMatchNumSum += matchNum - RemainNum;
    setNSquashedButSkip(src, origin_idx, len, matchNum-1, RemainNum-2, 1);
  }

  if (matchNum == 1) return -1;
  else return idx;
}

void TraceInstLoopDedup::dedupLen(std::vector<Instruction> &src, uint32_t len) {
  int idx = 0;
  uint32_t matchedIdx = 0;
  Instruction matchedData[len];
  matchedIdx = 0;
  bool isMatchState = true;
  // int last_idx_debug = idx;
  uint32_t squashedMatchNumSum = 0;
  for (; idx < srcSize;) {
    // first step: prepare matchedData, got the starting idx
    idx = nextAvailableIdx(src, idx, len);
    if (idx >= srcSize) break;

    int nextIdx = readNData(src, idx, len, matchedData);
    // printf("readNData idx: %d nextIdx: %d\n", idx, nextIdx);
    if (nextIdx == -1) { idx ++; continue; }

    nextIdx = countAndSetMatchNum(src, nextIdx, len, matchedData, squashedMatchNumSum);
    if (nextIdx == -1) { idx ++; continue; }
    else { idx = nextIdx; }
  }
  if (squashedMatchNumSum > 0) {
    printf("Dedup LoopBodyLength %d loopNum %u\n", len, squashedMatchNumSum);
  }
}


void TraceInstLoopDedup::addInst(Instruction inst) {
  if (inst.branch_type == 0 || inst.branch_taken == 0) {
    curLoopBodyLength ++;
  } else {

    curLoopBodyLength ++;
    if (loopBodyLengthMap.find(curLoopBodyLength) == loopBodyLengthMap.end()) {
      loopBodyLengthMap[curLoopBodyLength] = 1;
    } else {
      loopBodyLengthMap[curLoopBodyLength] ++;
    }
    curLoopBodyLength = 1;
  }
}

void TraceInstLoopDedup::getLoopBodyLengthVec(std::vector<uint32_t> &ret) {
  for (auto it = loopBodyLengthMap.begin(); it != loopBodyLengthMap.end(); it++) {
    if (it->second > TimesThreshold) {
      // printf("LoopBodyLength %d: %d\n", it->first, it->second);
      ret.push_back(it->first);
    }
  }
  return ;
}

// TODO: add nested loop dedup
// currently, only deepest loop dedup for speed
void TraceInstLoopDedup::trace_inst_dedup(std::vector<Instruction> &src, int dedupInstNum) {
  if (dedupInstNum <= 0 || src.size() == 0) { return; };

  srcSize = std::min(dedupInstNum, (int)src.size());
  std::vector<uint32_t> lengthVec;
  getLoopBodyLengthVec(lengthVec);
  int lengthIndx = 1;
  for (auto length : lengthVec) {
    dedupLen(src, length);
  }


  for (int i = 0; i < srcSize; i++) {
    if (src[i].is_squashed) squashedInst++;
    // src[i].dump();
  }
  printf("Squashed Inst: %lu . warmup Inst From %d to %lu\n", squashedInst, srcSize, srcSize - squashedInst);
  return ;
}