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
#include <algorithm>

void ElfBinary::load() {
  assert(size >= sizeof(Elf64_Ehdr));
  eh64 = (const Elf64_Ehdr *)raw;
  assert(IS_ELF32(*eh64) || IS_ELF64(*eh64));

  if (IS_ELF32(*eh64))
    parse(data32);
  else
    parse(data64);
}

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
  std::sort(sections.begin(), sections.end(),
            [](const ElfSection &a, const ElfSection &b) { return a.data_dst < b.data_dst; });
}

ElfBinaryFile::ElfBinaryFile(const char *filename) : filename(filename) {
  int fd = open(filename, O_RDONLY);
  struct stat s;
  assert(fd != -1);
  assert(fstat(fd, &s) >= 0);
  size = s.st_size;

  raw = (uint8_t *)mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
  assert(raw != MAP_FAILED);
  close(fd);

  load();
}

ElfBinaryFile::~ElfBinaryFile() {
  if (raw)
    munmap((void *)raw, size);
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

  if (base_addr != PMEM_BASE) {
    printf(
        "The first address in the elf does not match the base of the physical memory.\n"
        "It is likely that execution leads to unexpected behaviour.\n");
  }

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
  // Since we are unpacking the sections, the total amount of bytes is the last
  // section offset plus its size.
  auto last_section = elf_file.sections.back();
  return last_section.data_dst - base_addr + last_section.data_len + last_section.zero_len;
}
