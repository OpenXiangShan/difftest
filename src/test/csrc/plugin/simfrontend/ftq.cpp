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

#include "ftq.h"
#include "debug.h"
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <iostream>
#include <sys/types.h>

static bool is_link_register(uint32_t reg_idx) {
  return (reg_idx == 1 || reg_idx == 5);
}

static BrType get_br_type(uint32_t instr, bool rvc) {
  if (rvc) {
    uint32_t opcode = instr & 0x3;
    uint32_t funct3 = (instr >> 13) & 0x7;

    if (opcode == 0b01) {
      if (funct3 == 0b101)
        return BrType::Jal;
      if (funct3 == 0b110 || funct3 == 0b111)
        return BrType::Branch; // C.BEQZ, C.BNEZ
    } else if (opcode == 0b10) {
      uint32_t funct5 = (instr >> 2) & 0x1F;
      if (funct3 == 0b100 && funct5 == 0b00000)
        return BrType::Jalr;
    }
  } else {
    uint32_t opcode = instr & 0x7F;
    uint32_t funct3 = (instr >> 12) & 0x7;
    if (opcode == 0b1100011)
      return BrType::Branch; // B-type instructions
    if (opcode == 0b1101111)
      return BrType::Jal; // JAL
    if (opcode == 0b1100111 && funct3 == 0b000)
      return BrType::Jalr; // JALR
  }
  return BrType::NotCfi;
}

bool parse_line(std::string_view line, uint64_t &pc, uint32_t &instr) {
  size_t colon_pos = line.find(':');
  if (colon_pos == std::string_view::npos) {
    std::cout << "This: { " << line << " } parsec failed" << std::endl;
    assert(false && "Line format error: no colon found");
  }

  std::string_view pc_sv = line.substr(0, colon_pos);
  std::string_view instr_sv = line.substr(colon_pos + 1);

  if (pc_sv.empty() || instr_sv.empty()) {
    return false;
  }

  auto res_pc = std::from_chars(pc_sv.data(), pc_sv.data() + pc_sv.size(), pc, 16);
  if (res_pc.ec != std::errc() || res_pc.ptr != pc_sv.data() + pc_sv.size()) {
    return false;
  }

  auto res_instr = std::from_chars(instr_sv.data(), instr_sv.data() + instr_sv.size(), instr, 16);
  if (res_instr.ec != std::errc() || res_instr.ptr != instr_sv.data() + instr_sv.size()) {
    return false;
  }

  return true;
}

RiscvInstructionInfo analyze_instruction(uint32_t instr) {
  RiscvInstructionInfo info;

  info.isRVC = is_rvc(instr);
  info.brType = get_br_type(instr, info.isRVC);

  if (info.isRVC) {
    info.rd = instr >> 12 & 0x1;
    if (info.brType == BrType::Jal) {
      info.rs1 = 0;
    } else {
      info.rs1 = (instr >> 7) & 0x1F;
    }
  } else {
    info.rd = (instr >> 7) & 0x1F;
    info.rs1 = (instr >> 15) & 0x1F;
  }

  bool is_jal_not_rvc = (info.brType == BrType::Jal && !info.isRVC);
  bool is_jalr = (info.brType == BrType::Jalr);

  info.isCall = (is_jal_not_rvc || is_jalr) && is_link_register(info.rd);
  info.isRet = (info.brType == BrType::Jalr) && is_link_register(info.rs1) && !info.isCall;

  info.isJump = info.brType == BrType::Jal || info.brType == BrType::Jalr;
  return info;
}

#include <type_traits>
template <typename T> void set_bit_field(T &data, T value, int position, int width) {
  static_assert(std::is_unsigned_v<T>, "set_bit_field can only be used with unsigned types.");

  T mask = (static_cast<T>(1) << width) - 1;
  data &= ~(mask << position);
  data |= (value & mask) << position;
}

template <typename T> T get_bit_field(T data, int position, int width) {
  static_assert(std::is_unsigned_v<T>, "get_bit_field can only be used with unsigned types.");

  T mask = (static_cast<T>(1) << width) - 1;
  return (data >> position) & mask;
}

uint32_t pack_trace_data(const RiscvInstructionInfo &info, bool pred_taken, bool is_last_ftq_entry, bool wrap,
                         uint32_t queue_id, uint32_t offset) {
  uint32_t packed_value = 0;

  const uint32_t val_is_rvc = static_cast<uint32_t>(info.isRVC);
  const uint32_t val_br_type = static_cast<uint32_t>(info.brType);
  const uint32_t val_is_call = static_cast<uint32_t>(info.isCall);
  const uint32_t val_is_ret = static_cast<uint32_t>(info.isRet);
  const uint32_t val_pred_taken = static_cast<uint32_t>(pred_taken);
  const uint32_t val_queue_id = static_cast<uint32_t>(queue_id);
  const uint32_t val_queue_wrap = static_cast<uint32_t>(wrap);
  const uint32_t val_offset = static_cast<uint32_t>(offset);
  const uint32_t val_is_last_entr = static_cast<uint32_t>(is_last_ftq_entry);

  packed_value |= (1ULL << POS_CONST);
  packed_value |= (val_is_rvc << POS_IS_RVC);
  packed_value |= (val_br_type << POS_BR_TYPE);
  packed_value |= (val_is_call << POS_IS_CALL);
  packed_value |= (val_is_ret << POS_IS_RET);
  packed_value |= (val_pred_taken << POS_PRED_TAKEN);
  packed_value |= (val_queue_id << POS_QUEUE_ID);
  packed_value |= (val_queue_wrap << POS_QUEUE_ID_WRAP);
  packed_value |= (val_is_last_entr << POS_IS_LAST_ENTRY);
  packed_value |= (val_offset << POS_OFFSET);

  return packed_value;
}

UnpackedTraceData unpack_trace_data(uint32_t packed_data) {
  UnpackedTraceData unpacked;

  unpacked.isRVC = get_bit_field(packed_data, POS_IS_RVC, WIDTH_IS_RVC);
  unpacked.brType = static_cast<BrType>(get_bit_field(packed_data, POS_BR_TYPE, WIDTH_BR_TYPE));
  unpacked.isCall = get_bit_field(packed_data, POS_IS_CALL, WIDTH_IS_CALL);
  unpacked.isRet = get_bit_field(packed_data, POS_IS_RET, WIDTH_IS_RET);
  unpacked.pred_taken = get_bit_field(packed_data, POS_PRED_TAKEN, WIDTH_PRED_TAKEN);
  unpacked.queue_id = get_bit_field(packed_data, POS_QUEUE_ID, WIDTH_QUEUE_ID);
  unpacked.queue_wrap = get_bit_field(packed_data, POS_QUEUE_ID_WRAP, WIDTH_QUEUE_ID_WRAP);
  unpacked.is_last_ftq_entry = get_bit_field(packed_data, POS_IS_LAST_ENTRY, WIDTH_IS_LAST_ENTRY);
  unpacked.offset = get_bit_field(packed_data, POS_OFFSET, WIDTH_OFFSET);

  return unpacked;
}

static std::string br_type_to_string_tb(BrType type) {
  switch (type) {
    case BrType::NotCfi: return "NotCfi";
    case BrType::Branch: return "Branch";
    case BrType::Jal: return "Jal";
    case BrType::Jalr: return "Jalr";
    default: return "Unknown";
  }
}

bool out_is_discontinuous(uint64_t current_pc, bool current_instr_isrvc, uint64_t next_pc) {
  uint64_t expected_pc = current_pc + (current_instr_isrvc ? 2 : 4);
  bool discontinuous = (next_pc != expected_pc);

  return discontinuous;
}

ContinuityChecker::ContinuityChecker() : last_pc(0), last_instr_was_rvc(false), is_first_call(true) {}

bool ContinuityChecker::is_discontinuous(uint64_t current_pc, uint32_t current_instr) {
  if (is_first_call) {
    is_first_call = false;
    update_state(current_pc, current_instr);
    return true;
  }

  uint64_t expected_pc = last_pc + (last_instr_was_rvc ? 2 : 4);

  bool discontinuous = (current_pc != expected_pc);

  update_state(current_pc, current_instr);

  return discontinuous;
}

bool ContinuityChecker::is_discontinuous(uint64_t current_pc, bool current_instr_isrvc, uint64_t next_pc) {
  uint64_t expected_pc = current_pc + (current_instr_isrvc ? 2 : 4);
  bool discontinuous = (next_pc != expected_pc);

  return discontinuous;
}

void ContinuityChecker::update_state(uint64_t pc, uint32_t instr) {
  last_pc = pc;
  last_instr_was_rvc = is_rvc(instr);
}

// ***************************
// FTQ
// ***************************

FetchTargetQueue::FetchTargetQueue()
    : final(false), need_new_group(false), enq_ptr(0), read_ptr(0), commit_ptr(0), id_head(0), id_tail(0),
      id_read_ptr(0), id_head_wrap(false), id_tail_wrap(false), id_read_wrap(false), accumulated_bytes(0) {}

bool FetchTargetQueue::will_start_new_group(bool is_discontinuous, uint32_t next_bytes) const {
  return is_discontinuous || accumulated_bytes + next_bytes > 32;
}

void FetchTargetQueue::set_final() {
  if (final)
    return;
  if ((id_tail + 1) % ID_QUEUE_SIZE == id_head)
    return;
  if (id_tail == ID_QUEUE_SIZE - 1)
    id_tail_wrap = !id_tail_wrap;
  id_tail = (id_tail + 1) % ID_QUEUE_SIZE;
  final = true;
  return;
};

bool FetchTargetQueue::is_full() const {
  if ((id_tail + 1) % ID_QUEUE_SIZE == id_head)
    return true;
  return false;
}

extern uint64_t debug_line_number;
bool FetchTargetQueue::enqueue(uint64_t pc, uint32_t instr, uint64_t next_pc, uint32_t next_instr,
                               bool is_discontinuous) {
  if (is_full())
    return false;
  bool offset_init = false;
  if (!id_queue[id_tail].is_valid) {
    id_queue[id_tail] = {true, enq_ptr, 0, id_tail_wrap};
    accumulated_bytes = 0;
    offset_init = true;
  }

  uint32_t next_bytes = is_rvc(instr) ? 2 : 4;
  bool starts_new_group = will_start_new_group(is_discontinuous, next_bytes) || need_new_group;
  need_new_group = false;

  if (starts_new_group && id_queue[id_tail].instr_count > 0) {
    if ((id_tail + 1) % ID_QUEUE_SIZE == id_head)
      return false;

    if (id_tail == ID_QUEUE_SIZE - 1) {
      id_tail_wrap = !id_tail_wrap;
    }

    id_tail = (id_tail + 1) % ID_QUEUE_SIZE;
    DEBUG_INFO(std::cout << std::hex << "new ftq group: " << id_tail << " wrap: " << id_tail_wrap << " pc: 0x" << pc
                         << " enq_ptr: " << enq_ptr << std::dec << std::endl;)

    id_queue[id_tail] = {true, enq_ptr, 0, id_tail_wrap};
    accumulated_bytes = 0;
    offset_init = true;
  } else if (id_queue[id_tail].instr_count >= MAX_INSTRS_PER_GROUP) {

    if ((id_tail + 1) % ID_QUEUE_SIZE == id_head)
      return false;
    if (id_tail == ID_QUEUE_SIZE - 1)
      id_tail_wrap = !id_tail_wrap;
    id_tail = (id_tail + 1) % ID_QUEUE_SIZE;
    id_queue[id_tail] = {true, enq_ptr, 0, id_tail_wrap};
    accumulated_bytes = 0;
    offset_init = true;
  }

  size_t current_group_id = id_tail;
  size_t current_group_wrap = id_tail_wrap;
  RiscvInstructionInfo riscv_info = analyze_instruction(instr);
  // uint32_t offset = offset_init ? 0 : (pc - target_queue[id_queue[current_group_id].start_index].pc) >> 1; // frist
  uint32_t offset = offset_init
                        ? (is_rvc(instr) ? 0 : 1)
                        : (pc - target_queue[id_queue[current_group_id].start_index].pc + (is_rvc(instr) ? 0 : 2)) >> 1;

  bool br_taken = out_is_discontinuous(pc, is_rvc(instr), next_pc);

  uint32_t self_and_next_bytes = (is_rvc(instr) ? 2 : 4) + (is_rvc(next_instr) ? 2 : 4);
  bool is_last_ftq_entry = will_start_new_group(br_taken, self_and_next_bytes);

  uint64_t packed_data =
      pack_trace_data(riscv_info, br_taken, is_last_ftq_entry, current_group_wrap, current_group_id, offset);

  auto unpacked_data = unpack_trace_data(packed_data);

  accumulated_bytes += next_bytes;
  target_queue[enq_ptr] = {pc, instr, packed_data, current_group_id, current_group_wrap, debug_line_number};
  enq_ptr = (enq_ptr + 1) % INSTR_QUEUE_SIZE;
  id_queue[current_group_id].instr_count++;

  need_new_group = riscv_info.isJump || br_taken;
  return true;
}

std::optional<FTQEntry> FetchTargetQueue::read() {
  if (read_ptr == enq_ptr)
    return std::nullopt;
  FTQEntry entry = target_queue[read_ptr];
  read_ptr = (read_ptr + 1) % INSTR_QUEUE_SIZE;
  return entry;
}

std::optional<FTQEntry> FetchTargetQueue::read_offset(uint32_t offset) {
  if (offset >= readable_size()) {
    return std::nullopt;
  }
  size_t physical_index = (read_ptr + offset) % INSTR_QUEUE_SIZE;
  FTQEntry entry = target_queue[physical_index];
  read_ptr = (physical_index + 1) % INSTR_QUEUE_SIZE;

  return entry;
}

std::optional<FTQEntry> FetchTargetQueue::peek() const {
  if (read_ptr == enq_ptr)
    return std::nullopt;
  return target_queue[read_ptr];
}

std::optional<FTQEntry> FetchTargetQueue::peek_offset(size_t offset) const {
  if (offset >= readable_size()) {
    return std::nullopt;
  }

  size_t physical_index = (read_ptr + offset) % INSTR_QUEUE_SIZE;

  return target_queue[physical_index];
}

bool FetchTargetQueue::set_read_ptr_offset(size_t offset) {
  if (offset > readable_size()) {
    return false;
  }

  read_ptr = (read_ptr + offset) % INSTR_QUEUE_SIZE;

  return true;
}

bool FetchTargetQueue::set_read_ptr(size_t index) {
  if (index > uncommitted_size()) {
    return false;
  }

  read_ptr = (commit_ptr + index) % INSTR_QUEUE_SIZE;

  return true;
}

size_t FetchTargetQueue::uncommitted_size() const {
  if (enq_ptr >= commit_ptr)
    return enq_ptr - commit_ptr;
  return INSTR_QUEUE_SIZE - (commit_ptr - enq_ptr);
}
size_t FetchTargetQueue::readable_size() const {
  if (enq_ptr >= read_ptr)
    return enq_ptr - read_ptr;
  return INSTR_QUEUE_SIZE - (read_ptr - enq_ptr);
}

std::optional<FTQEntry> FetchTargetQueue::get_entry(size_t index) const {
  if (index >= uncommitted_size())
    return std::nullopt;
  size_t physical_index = (commit_ptr + index) % INSTR_QUEUE_SIZE;
  return target_queue[physical_index];
}

std::optional<FTQEntry> FetchTargetQueue::get_physical_entry(size_t index) const {
  if (index >= INSTR_QUEUE_SIZE)
    return std::nullopt;
  return target_queue[index];
}

size_t FetchTargetQueue::id_queue_committed_size() const {
  if (id_tail == id_head) {
    return (id_tail_wrap == id_head_wrap) ? 0 : ID_QUEUE_SIZE;
  } else if (id_tail > id_head) {
    return id_tail - id_head;
  } else {
    return ID_QUEUE_SIZE - (id_head - id_tail);
  }
}

size_t FetchTargetQueue::id_queue_readable_size() const {
  if (id_tail == id_read_ptr) {
    int read_valid = id_queue[id_read_ptr].is_valid ? 0 : 0;
    // std::cout << "id_read_ptr: " << id_read_ptr << " id_tail_ptr: " << id_tail << " valid: " << read_valid << std::endl;
    return (id_tail_wrap == id_read_wrap) ? read_valid : ID_QUEUE_SIZE;
  } else if (id_tail > id_read_ptr) {
    return id_tail - id_read_ptr;
  } else {
    return ID_QUEUE_SIZE - (id_read_ptr - id_tail);
  }
}

std::optional<FetchGroupInfo> FetchTargetQueue::peek_id_group(size_t offset) const {
  size_t physical_index = (id_read_ptr + offset) % ID_QUEUE_SIZE;

  if (offset >= id_queue_readable_size() && id_queue[physical_index].is_valid != true) {
    return std::nullopt;
  }

  return id_queue[physical_index];
}

std::optional<FetchGroupInfo> FetchTargetQueue::read_id_group() {
  if (id_queue_readable_size() == 0) {
    return std::nullopt;
  }

  FetchGroupInfo group_info = id_queue[id_read_ptr];

  if (id_read_ptr == ID_QUEUE_SIZE - 1) {
    id_read_wrap = !id_read_wrap;
  }
  id_read_ptr = (id_read_ptr + 1) % ID_QUEUE_SIZE;

  return group_info;
}

bool FetchTargetQueue::commit_to(size_t target_group_id, bool target_wrap_bit) {
  if (target_group_id >= ID_QUEUE_SIZE || !id_queue[target_group_id].is_valid) {
    std::cout << "commit_to failed: target_group_id " << target_group_id << " is invalid or does not exist.\n";
    return false;
  }
  if (id_queue[target_group_id].wrap_bit != target_wrap_bit) {
    std::cout << "commit_to failed: target_group_id " << target_group_id << " wrap bit mismatch.\n";
    return false;
  }

  while (true) {
    FetchGroupInfo &group_to_commit = id_queue[id_head];

    if (id_head == id_tail && id_head_wrap == id_tail_wrap) {
      return false;
    }

    if (!group_to_commit.is_valid) {
      return false;
    }

    commit_ptr = (group_to_commit.start_index + group_to_commit.instr_count) % INSTR_QUEUE_SIZE;

    bool was_target = (id_head == target_group_id && id_head_wrap == target_wrap_bit);

    advance_id_head();

    if (was_target) {
      break;
    }
  }

  if (uncommitted_size() < readable_size()) {
    read_ptr = commit_ptr;
  }

  return true;
}

bool FetchTargetQueue::commit_to_right_open(size_t target_group_id, bool target_wrap_bit) {
  if (target_group_id >= ID_QUEUE_SIZE || !id_queue[target_group_id].is_valid) {
    return false;
  }
  if (id_queue[target_group_id].wrap_bit != target_wrap_bit) {
    return false;
  }

  while (id_head != target_group_id || id_head_wrap != target_wrap_bit) {

    if (id_head == id_tail && id_head_wrap == id_tail_wrap) {
      return false;
    }

    FetchGroupInfo &group_to_commit = id_queue[id_head];
    if (!group_to_commit.is_valid) {
      return false;
    }

    commit_ptr = (group_to_commit.start_index + group_to_commit.instr_count) % INSTR_QUEUE_SIZE;

    advance_id_head();
  }

  if (uncommitted_size() < readable_size() || (read_ptr == commit_ptr && readable_size() > 0)) {
    bool commit_is_ahead = (commit_ptr > read_ptr && id_head_wrap == id_read_wrap) || (id_head_wrap != id_read_wrap);
    if (uncommitted_size() < readable_size()) {
      read_ptr = commit_ptr;
    }
  }

  return true;
}

#define PC_MASK 0x3FFFFFFFFFFFF
bool FetchTargetQueue::redirect(size_t target_group_id, bool target_wrap_bit, uint32_t redirect_type,
                                uint64_t redirect_pc, uint64_t redirect_target) {
  if (target_group_id >= ID_QUEUE_SIZE || !id_queue[target_group_id].is_valid ||
      id_queue[target_group_id].wrap_bit != target_wrap_bit) {
    return false;
  }
  const FetchGroupInfo &target_group = id_queue[target_group_id];

  DEBUG_INFO(std::cout << std::hex << "Redirecting to ftq group: " << target_group_id << " wrap: " << target_wrap_bit
                       << " start pc: 0x" << target_queue[target_group.start_index].pc << " this pc: 0x" << redirect_pc
                       << " target pc: 0x" << redirect_target << " redirect type: " << redirect_type << std::dec
                       << std::endl;)

  if (redirect_type != isMisPred) {
    for (size_t i = 0; i < target_group.instr_count; ++i) {
      size_t physical_index = (target_group.start_index + i) % INSTR_QUEUE_SIZE;
      uint64_t entry_pc = target_queue[physical_index].pc & PC_MASK;
      if (entry_pc == redirect_target) {

        id_read_ptr = target_group_id;
        id_read_wrap = target_wrap_bit;
        read_ptr = physical_index;
        return true;
      }
    }
  }

  //exception & prediction failure
  size_t next_group_id = (target_group_id + 1) % ID_QUEUE_SIZE;
  bool next_wrap_bit = (target_group_id == ID_QUEUE_SIZE - 1) ? !target_wrap_bit : target_wrap_bit;

  if (next_group_id == id_tail && next_wrap_bit == id_tail_wrap && !id_queue[next_group_id].is_valid) {

    DEBUG_INFO(std::cout << std::hex << "Redirect Failed: ftq empty " << "next ftq: 0x" << next_group_id << ", "
                         << next_wrap_bit << " " << "tail idx: 0x" << id_tail << " tail wrap: " << id_tail_wrap
                         << std::dec << std::endl;)

    return false;
  }
  if (!id_queue[next_group_id].is_valid || id_queue[next_group_id].wrap_bit != next_wrap_bit) {

    DEBUG_INFO(std::cout << std::hex << "Redirect Failed: next not valid or wrap mismatch " << "next valid "
                         << id_queue[next_group_id].is_valid << "next ftq: 0x" << next_group_id << ", " << next_wrap_bit
                         << " " << "need wrap: " << id_queue[next_group_id].wrap_bit << std::dec << std::endl;)

    return false;
  }

  const FetchGroupInfo &next_group = id_queue[next_group_id];
  uint64_t next_start_pc = target_queue[next_group.start_index].pc & PC_MASK;
  if (next_start_pc == redirect_target) {
    id_read_ptr = next_group_id;
    id_read_wrap = next_wrap_bit;
    read_ptr = next_group.start_index;
    return true;
  }

  DEBUG_INFO(std::cout << std::hex << "Redirect Failed: " << " next ftq: 0x" << next_group_id << " next start pc: 0x"
                       << target_queue[next_group.start_index].pc << " next line: 0x"
                       << target_queue[next_group.start_index].line_number << std::dec << std::endl;)

  return false;
}

void FetchTargetQueue::advance_id_head() {
  if (id_queue_committed_size() == 0)
    return;

  id_queue[id_head].is_valid = false;
  if (id_head == ID_QUEUE_SIZE - 1) {
    id_head_wrap = !id_head_wrap;
  }
  id_head = (id_head + 1) % ID_QUEUE_SIZE;
}

void FetchTargetQueue::print_ftq_state(size_t idx) {
  if (idx >= INSTR_QUEUE_SIZE) {
    std::cout << "Index out of bounds\n";
    return;
  }
  auto ftq = id_queue[idx];
  std::cout << "FTQ Entry at index " << idx << std::hex << " Valid: " << ftq.is_valid
            << " Start Pc: " << target_queue[ftq.start_index].pc << " Instruction Count: " << ftq.instr_count
            << " Wrap Bit: " << ftq.wrap_bit << std::dec << std::endl;
}
