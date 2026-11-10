/***************************************************************************************
* Copyright (c) 2026 Institute of Computing Technology, Chinese Academy of Sciences
* Copyright (c) 2026 Beijing Institute of Open Source Chip (BOSC)
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

#include "instructionsource.h"
#include <charconv>
#include <string_view>

bool InstructionTraceSource::init(const std::string &filename, const FlashImageView &flash) {
  if (!trace_reader.init(filename)) {
    error_message = "failed to open instruction trace: " + filename;
    return false;
  }
  if (!flash.is_builtin) {
    final = trace_reader.is_final;
    return true;
  }
  if (!decode_builtin_flash(flash) || !skip_legacy_flash_prefix()) {
    return false;
  }
  return true;
}

bool InstructionTraceSource::decode_builtin_flash(const FlashImageView &flash) {
  if (flash.data == nullptr) {
    error_message = "built-in flash is not initialized";
    return false;
  }
  if (flash.image_size == 0) {
    error_message = "built-in flash image is empty";
    return false;
  }
  if (flash.image_size > flash.capacity) {
    error_message = "built-in flash image exceeds flash capacity";
    return false;
  }

  size_t offset = 0;
  while (offset < flash.image_size) {
    if (flash.image_size - offset < sizeof(uint16_t)) {
      error_message = "built-in flash ends with a truncated instruction";
      return false;
    }

    uint32_t instr = static_cast<uint32_t>(flash.data[offset]) | (static_cast<uint32_t>(flash.data[offset + 1]) << 8);
    const size_t instr_size = (instr & 0x3) == 0x3 ? sizeof(uint32_t) : sizeof(uint16_t);
    if (flash.image_size - offset < instr_size) {
      error_message = "built-in flash ends with a truncated instruction";
      return false;
    }
    if (instr_size == sizeof(uint32_t)) {
      instr |= static_cast<uint32_t>(flash.data[offset + 2]) << 16;
      instr |= static_cast<uint32_t>(flash.data[offset + 3]) << 24;
    }

    flash_entries.push_back({FLASH_BASE + offset, instr});
    offset += instr_size;
  }
  return true;
}

bool InstructionTraceSource::skip_legacy_flash_prefix() {
  std::string_view line;
  InstructionTraceEntry entry;
  while (trace_reader.probe_next_line(line)) {
    if (!parse_trace_line(line, entry)) {
      error_message = "invalid instruction trace line while skipping legacy flash prefix";
      return false;
    }
    if (entry.pc >= RAM_BASE) {
      break;
    }
    trace_reader.get_next_line(line);
  }
  return true;
}

bool InstructionTraceSource::parse_trace_line(std::string_view line, InstructionTraceEntry &entry) {
  const size_t colon_pos = line.find(':');
  if (colon_pos == std::string_view::npos) {
    return false;
  }

  const std::string_view pc_text = line.substr(0, colon_pos);
  const std::string_view instr_text = line.substr(colon_pos + 1);
  if (pc_text.empty() || instr_text.empty()) {
    return false;
  }

  auto pc_result = std::from_chars(pc_text.data(), pc_text.data() + pc_text.size(), entry.pc, 16);
  if (pc_result.ec != std::errc() || pc_result.ptr != pc_text.data() + pc_text.size()) {
    return false;
  }
  auto instr_result = std::from_chars(instr_text.data(), instr_text.data() + instr_text.size(), entry.instr, 16);
  return instr_result.ec == std::errc() && instr_result.ptr == instr_text.data() + instr_text.size();
}

bool InstructionTraceSource::get_next(InstructionTraceEntry &entry) {
  if (flash_index < flash_entries.size()) {
    entry = flash_entries[flash_index++];
    line_read_count++;
    return true;
  }

  std::string_view line;
  if (!trace_reader.get_next_line(line)) {
    final = true;
    return false;
  }
  if (!parse_trace_line(line, entry)) {
    error_message = "invalid instruction trace line";
    final = true;
    return false;
  }
  line_read_count++;
  return true;
}

bool InstructionTraceSource::probe_next(InstructionTraceEntry &entry) {
  if (flash_index < flash_entries.size()) {
    entry = flash_entries[flash_index];
    return true;
  }

  std::string_view line;
  if (!trace_reader.probe_next_line(line)) {
    final = true;
    return false;
  }
  if (!parse_trace_line(line, entry)) {
    error_message = "invalid instruction trace line";
    final = true;
    return false;
  }
  return true;
}

bool InstructionTraceSource::is_final() const {
  return final;
}

size_t InstructionTraceSource::get_line_read_count() const {
  return line_read_count;
}

const std::string &InstructionTraceSource::error() const {
  return error_message;
}
