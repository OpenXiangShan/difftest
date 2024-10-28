/***************************************************************************************
* Copyright (c) 2024 Axelera AI
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

#include "elfloader.h"

template <typename ehdr_t, typename phdr_t, typename shdr_t, typename sym_t>
void ElfBinary::parse(ElfBinaryData<ehdr_t, phdr_t, shdr_t, sym_t> &data) {
  data.eh = (const ehdr_t *)raw;
  data.ph = (const phdr_t *)(raw + data.eh->e_phoff);
  entry = data.eh->e_entry;
  assert(size >= data.eh->e_phoff + data.eh->e_phnum * sizeof(*data.ph));
  for (unsigned i = 0; i < data.eh->e_phnum; i++) {
    if (data.ph[i].p_type == PT_LOAD && data.ph[i].p_memsz) {
      if (data.ph[i].p_filesz) {
        assert(size >= data.ph[i].p_offset + data.ph[i].p_filesz);
        sections.push_back({
          .data_src = (const uint8_t *)raw + data.ph[i].p_offset,
          .data_dst = data.ph[i].p_paddr,
          .data_len = data.ph[i].p_filesz,
          .zero_dst = data.ph[i].p_paddr + data.ph[i].p_filesz,
          .zero_len = data.ph[i].p_memsz - data.ph[i].p_filesz,
        });
      }
    }
  }
}

bool isElfFile(const char *filename) {
  int fd = -1;

#ifdef NO_IMAGE_ELF
  return false;
#endif

  fd = open(filename, O_RDONLY);
  assert(fd);

  uint8_t buf[4];

  size_t sz = read(fd, buf, 4);
  if (!IS_ELF(*((const Elf64_Ehdr *)buf))) {
    close(fd);
    return false;
  }

  close(fd);
  return true;
}

long readFromElf(void *ptr, const char *file_name, long buf_size) {
  auto elf_file = ElfBinaryFile(file_name);

  if (elf_file.sections.size() < 1) {
    printf("The requested elf '%s' contains zero sections\n", file_name);
    return -1;
  }

  uint64_t len_written = 0;
  auto base_addr = elf_file.sections[0].data_dst;

  for (auto section: elf_file.sections) {
    auto len = section.data_len + section.zero_len;
    auto offset = section.data_dst - base_addr;

    if (offset + len > buf_size) {
      printf("The size (%ld bytes) of the section at address 0x%lx is larger than buf_size!\n", len, section.data_dst);
      return -1;
    }

    printf("Loading %ld bytes at address 0x%lx at offset 0x%lx\n", len, section.data_dst, offset);
    std::memset((uint8_t *)ptr + offset, 0, len);
    std::memcpy((uint8_t *)ptr + offset, section.data_src, section.data_len);
    len_written += len;
  }
  return len_written;
}
