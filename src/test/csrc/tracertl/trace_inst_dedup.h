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
#ifndef __TRACE_INST_DEDUP_H__
#define __TRACE_INST_DEDUP_H__

#include <vector>
#include <map>
#include <cstdint>
#include "trace_common.h"
#include "trace_format.h"

// struct TraceLoopBody {
//   uint64_t start_inst_pc_va;
//   uint64_t end_inst_pc_va;
//   uint64_t branch_target;
//   uint32_t loopBodyLength;
//   uint32_t branch_inst;
//   uint32_t repeat_num=1;
//   // bool is_squashed;
//   // bool is_branch;
//   // bool branch_taken;
// };

class TraceInstLoopDedup {

  const uint32_t RemainNum = 44; // branch hisotry length
  const uint32_t TimesThreshold = 100;
  int srcSize = -1; // inst num
  std::map<uint32_t, uint32_t> loopBodyLengthMap;
  uint32_t curLoopBodyLength = 1;
  uint64_t squashedInst = 0;

  // std::vector<TraceLoopBody> loopBodyVec;
  // bool afterFirstLoop = false;
  // bool startLoop = true;
  bool inLoop = true;
  // TraceLoopBody curLoopBody;

  int nextAvailableIdx(std::vector<Instruction> &src, int idx, uint32_t len);
  int readNData(std::vector<Instruction> &src, int idx, uint32_t len, Instruction *data);
  inline bool matchOneInst(Instruction &a, Instruction &b);
  int findOneMatch(std::vector<Instruction> &src, uint32_t idx, uint32_t len, Instruction *data);
  void setNSquashedButSkip(std::vector<Instruction> &src, int idx, uint32_t len, uint32_t num, int beginSkip, int endSkip);
  int countAndSetMatchNum(std::vector<Instruction> &src, int idx, uint32_t len, Instruction *data, uint32_t &matchNumSum);
  void dedupLen(std::vector<Instruction> &src, uint32_t len);

public:
  void trace_inst_dedup(std::vector<Instruction> &src, int dedupInstNum);
  uint64_t getSquashedInst() { return squashedInst; }

  void addInst(Instruction inst);
  void getLoopBodyLengthVec(std::vector<uint32_t> &ret);
};

#endif