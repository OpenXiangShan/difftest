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

#include "tracereader.h"
#include "debug.h"
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

MmapLineReader::MmapLineReader(const std::string &filename) {
  if (init(filename)) {
    is_open = true;
  }
}

MmapLineReader::~MmapLineReader() {
  if (mapped_data) {
    munmap(mapped_data, file_size);
  }
  if (fd != -1) {
    close(fd);
  }
}

bool MmapLineReader::init(const std::string &filename) {
  fd = open(filename.c_str(), O_RDONLY);
  if (fd == -1) {
    perror("Error opening file");
    return false;
  }

  struct stat sb;
  if (fstat(fd, &sb) == -1) {
    perror("Error getting file size");
    close(fd);
    fd = -1;
    return false;
  }
  file_size = sb.st_size;

  if (file_size == 0) {
    is_final = true;
    is_open = true;
    read_ptr = nullptr;
    end_of_file = nullptr;
    return true;
  }

  mapped_data = static_cast<char *>(mmap(NULL, file_size, PROT_READ, MAP_PRIVATE, fd, 0));
  if (mapped_data == MAP_FAILED) {
    perror("Error mapping file to memory");
    close(fd);
    fd = -1;
    mapped_data = nullptr;
    return false;
  }

  read_ptr = mapped_data;
  end_of_file = mapped_data + file_size;
  is_open = true;
  is_final = false;

  return true;
}

bool MmapLineReader::get_next_line(std::string_view &line) {
  if (read_ptr == nullptr || read_ptr >= end_of_file) {
    is_final = true;
    return false;
  }

  const void *newline_ptr_void = memchr(read_ptr, '\n', end_of_file - read_ptr);

  const char *line_start = read_ptr;
  const char *line_end;

  if (newline_ptr_void != nullptr) {

    line_end = static_cast<const char *>(newline_ptr_void);
    read_ptr = line_end + 1;
  } else {
    line_end = end_of_file;
    read_ptr = end_of_file;
  }
  line = std::string_view(line_start, line_end - line_start);

  DEBUG_INFO(std::cout << std::hex << "Read Line: 0x" << lines_read << ", 0x" << line << std::dec << std::endl;)

  lines_read++;

  return true;
}

bool MmapLineReader::probe_next_line(std::string_view &line) {
  if (read_ptr == nullptr || read_ptr >= end_of_file) {
    return false;
  }

  const void *newline_ptr_void = memchr(read_ptr, '\n', end_of_file - read_ptr);

  const char *line_start = read_ptr;
  const char *line_end;

  if (newline_ptr_void != nullptr) {
    line_end = static_cast<const char *>(newline_ptr_void);
  } else {
    line_end = end_of_file;
  }

  line = std::string_view(line_start, line_end - line_start);

  DEBUG_INFO(std::cout << std::hex << "Probe Line: 0x" << lines_read + 1 << ", 0x" << line << std::dec << std::endl;)
  return true;
}

size_t MmapLineReader::get_line_read_count() const {
  return lines_read;
}
