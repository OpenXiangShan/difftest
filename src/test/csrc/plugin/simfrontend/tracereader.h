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

#ifndef TRACEREADER_H
#define TRACEREADER_H

#include <string>
#include <string_view>
#include <sys/types.h>
#include <vector>

class MmapLineReader {
public:
  explicit MmapLineReader() = default;
  explicit MmapLineReader(const std::string &filename);

  bool is_final = false;

  bool is_open = false;

  ~MmapLineReader();

  bool init(const std::string &filename);

  bool get_next_line(std::string_view &line);

  bool probe_next_line(std::string_view &line);

  size_t get_line_read_count() const;

private:
  MmapLineReader(const MmapLineReader &) = delete;
  MmapLineReader &operator=(const MmapLineReader &) = delete;

  int fd = -1;
  char *mapped_data = nullptr;
  size_t file_size = 0;

  const char *read_ptr = nullptr;
  const char *end_of_file = nullptr;

  size_t lines_read = 0;
};

#endif // TRACEREADER_H
