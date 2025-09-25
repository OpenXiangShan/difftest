/***************************************************************************************
* Copyright (c) 2025 Institute of Computing Technology, Chinese Academy of Sciences
* Copyright (c) 2025 Beijing Institute of Open Source Chip (BOSC)
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

#ifndef FTQ_H
#define FTQ_H

#include <array>
#include <charconv>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <iomanip>
#include <iostream>
#include <optional>
#include <vector>

enum RedirectType {
  // For now, there is only two type, but for the sake of scalability, let's write it this way.
  isMisPred = 1,
  isFalseBranch = 2 // The branch is not mispredicted, but it is a false branch (the target is the next sequential PC)
};

enum class BrType {
  NotCfi,
  Branch,
  Jal,
  Jalr
};

enum TraceBitPositions {
  POS_CONST = 0,
  POS_IS_RVC = 1,
  POS_BR_TYPE = 2,
  POS_IS_CALL = 4,
  POS_IS_RET = 5,
  POS_PRED_TAKEN = 6,
  POS_QUEUE_ID = 7,
  POS_QUEUE_ID_WRAP = 13,
  POS_IS_LAST_ENTRY = 14,
  POS_OFFSET = 15
};

enum TraceBitWidths {
  WIDTH_CONST = 1,
  WIDTH_IS_RVC = 1,
  WIDTH_BR_TYPE = 2,
  WIDTH_IS_CALL = 1,
  WIDTH_IS_RET = 1,
  WIDTH_PRED_TAKEN = 1,
  WIDTH_QUEUE_ID = 6,
  WIDTH_QUEUE_ID_WRAP = 1,
  WIDTH_IS_LAST_ENTRY = 1,
  WIDTH_OFFSET = 5,
};

struct UnpackedTraceData {
  bool isRVC;
  BrType brType;
  bool isCall;
  bool isRet;
  bool pred_taken;
  bool is_last_ftq_entry;
  bool queue_wrap;
  uint32_t queue_id;
  uint32_t offset;
};

struct RiscvInstructionInfo {
  bool isRVC;
  BrType brType;
  bool isCall;
  bool isRet;

  //TODO At present, we still have one problem
  //If the branch instruction points to the next sequential program counter, then issues may arise during processing.
  //However, in theory, the compiler will not generate such instructions.
  bool isJump;

  uint32_t rd;
  uint32_t rs1;
};

static inline bool is_rvc(uint32_t instr) {
  return (instr & 0x3) != 0x3;
}

bool parse_line(std::string_view line, uint64_t &pc, uint32_t &instr);
RiscvInstructionInfo analyze_instruction(uint32_t instr);
uint32_t pack_trace_data(const RiscvInstructionInfo &info, bool pred_taken, bool is_last_ftq_entry, bool wrap,
                         uint32_t queue_id, uint32_t offset);
UnpackedTraceData unpack_trace_data(uint32_t packed_data);

class ContinuityChecker {
public:
  ContinuityChecker();

  bool is_discontinuous(uint64_t current_pc, uint32_t current_instr);

  bool is_discontinuous(uint64_t current_pc, bool current_instr_isrvc, uint64_t next_pc);

private:
  void update_state(uint64_t pc, uint32_t instr);

  uint64_t last_pc;
  bool last_instr_was_rvc;
  bool is_first_call;
};

struct FTQEntry {
  uint64_t pc;
  uint32_t instr;
  uint64_t packed_data;
  size_t fetch_group_id;
  bool fetch_group_id_wrap;
  uint64_t line_number;
};

struct FetchGroupInfo {
  bool is_valid = false;
  size_t start_index = 0;
  size_t instr_count = 0;
  bool wrap_bit = false;
};

class FetchTargetQueue {
public:
  static constexpr size_t ID_QUEUE_SIZE = 64;
  static constexpr size_t INSTR_QUEUE_SIZE = 1024;
  static constexpr size_t MAX_INSTRS_PER_GROUP = 16;

  FetchTargetQueue();

  void set_final();
  bool is_full() const;

  bool enqueue(uint64_t pc, uint32_t instr, uint64_t next_pc, uint32_t next_instr, bool is_discontinuous);

  std::optional<FTQEntry> read();
  std::optional<FTQEntry> read_offset(uint32_t offset);
  std::optional<FTQEntry> peek() const;
  std::optional<FTQEntry> peek_offset(size_t offset) const;

  bool set_read_ptr(size_t index);
  bool set_read_ptr_offset(size_t index);

  size_t uncommitted_size() const;
  size_t readable_size() const;

  std::optional<FTQEntry> get_entry(size_t index) const;
  std::optional<FTQEntry> get_physical_entry(size_t index) const;

  size_t id_queue_committed_size() const;
  size_t id_queue_readable_size() const;

  std::optional<FetchGroupInfo> peek_id_group(size_t offset = 0) const;
  std::optional<FetchGroupInfo> read_id_group();

  size_t get_read_entry_ptr() {
    return read_ptr;
  }

  size_t get_read_ftq_id() {
    return id_read_ptr;
  }
  size_t get_read_wrap() {
    return id_read_wrap;
  }

  bool commit_to(size_t target_group_id, bool target_wrap_bit);
  bool commit_to_right_open(size_t target_group_id, bool target_wrap_bit);

  bool redirect(size_t target_group_id, bool target_wrap_bit, uint32_t redirect_type, uint64_t redirect_pc,
                uint64_t redirect_target);

  bool will_start_new_group(bool is_discontinuous, uint32_t next_bytes) const;

  void print_ftq_state(size_t idx);

  size_t get_enq_ptr() {
    return enq_ptr;
  }
  size_t get_read_ptr() {
    return read_ptr;
  }
  size_t get_commit_ptr() {
    return commit_ptr;
  }
  size_t get_id_head_ptr() {
    return id_head;
  }
  size_t get_id_tail_ptr() {
    return id_tail;
  }
  size_t get_id_read_ptr() {
    return id_read_ptr;
  }
  bool get_id_head_wrap() {
    return id_head_wrap;
  }
  bool get_id_tail_wrap() {
    return id_tail_wrap;
  }
  bool get_id_read_wrap() {
    return id_read_wrap;
  }

private:
  bool final;

  bool need_new_group;

  void advance_id_head();

  std::array<FetchGroupInfo, ID_QUEUE_SIZE> id_queue;
  std::array<FTQEntry, INSTR_QUEUE_SIZE> target_queue;

  size_t enq_ptr;
  size_t read_ptr;
  size_t commit_ptr;

  size_t id_head;
  size_t id_tail;
  size_t id_read_ptr;

  bool id_head_wrap;
  bool id_tail_wrap;
  bool id_read_wrap;
  int accumulated_bytes;
};
#endif // FTQ_H
