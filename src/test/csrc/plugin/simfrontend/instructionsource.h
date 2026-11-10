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

#ifndef INSTRUCTIONSOURCE_H
#define INSTRUCTIONSOURCE_H

#include "tracereader.h"
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

struct InstructionTraceEntry {
  uint64_t pc = 0;
  uint32_t instr = 0;
};

struct FlashImageView {
  const uint8_t *data = nullptr;
  size_t image_size = 0;
  size_t capacity = 0;
  bool is_builtin = false;
};

class InstructionTraceSource {
public:
  bool init(const std::string &filename, const FlashImageView &flash);
  bool get_next(InstructionTraceEntry &entry);
  bool probe_next(InstructionTraceEntry &entry);

  bool is_final() const;
  size_t get_line_read_count() const;
  const std::string &error() const;

private:
  static constexpr uint64_t FLASH_BASE = 0x10000000ULL;
  static constexpr uint64_t RAM_BASE = 0x80000000ULL;

  bool decode_builtin_flash(const FlashImageView &flash);
  bool skip_legacy_flash_prefix();
  bool parse_trace_line(std::string_view line, InstructionTraceEntry &entry);

  MmapLineReader trace_reader;
  std::vector<InstructionTraceEntry> flash_entries;
  size_t flash_index = 0;
  size_t line_read_count = 0;
  bool final = false;
  std::string error_message;
};

#endif // INSTRUCTIONSOURCE_H
