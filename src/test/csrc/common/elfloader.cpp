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

bool isElfImage(InputReader &input) {
#ifdef NO_IMAGE_ELF
  (void)input;
  return false;
#else
  uint8_t magic[4];
  ssize_t n = input.peek(magic, sizeof(magic));
  if (n < 0)
    throw std::runtime_error("Cannot detect image format");
  return n == 4 && magic[0] == 0x7f && magic[1] == 'E' && magic[2] == 'L' && magic[3] == 'F';
#endif
}

bool isElfFile(const char *filename) {
  auto input = open_input(filename);
  return isElfImage(*input);
}

long readFromElf(void *ptr, const char *file_name, long buf_size) {
  auto elf = ElfBinaryFile(file_name);
  return loadElfSegments(elf, ptr, buf_size);
}

long readFromElf(void *ptr, InputReader &input, long buf_size) {
  // ELF parsing uses arbitrary offsets. Spool only ELF inputs to an anonymous
  // file, keeping raw checkpoints streaming and reusing the existing parser.
  auto close_file = [](FILE *file) { fclose(file); };
  std::unique_ptr<FILE, decltype(close_file)> file(tmpfile(), close_file);
  if (!file) {
    fprintf(stderr, "Cannot create temporary ELF backing file\n");
    return -1;
  }
  uint8_t buffer[1 << 16];
  for (;;) {
    ssize_t n = input.read(buffer, sizeof(buffer));
    if (n < 0)
      return -1;
    if (n == 0)
      break;
    if (fwrite(buffer, 1, n, file.get()) != static_cast<size_t>(n)) {
      fprintf(stderr, "Cannot write temporary ELF backing file\n");
      return -1;
    }
  }
  if (fflush(file.get()) != 0)
    return -1;
  struct stat s;
  if (fstat(fileno(file.get()), &s) < 0 || s.st_size < static_cast<off_t>(sizeof(Elf64_Ehdr))) {
    fprintf(stderr, "Truncated ELF image\n");
    return -1;
  }
  auto raw = mmap(nullptr, s.st_size, PROT_READ, MAP_PRIVATE, fileno(file.get()), 0);
  if (raw == MAP_FAILED)
    return -1;
  auto unmap = [&s](const uint8_t *ptr) { munmap(const_cast<uint8_t *>(ptr), s.st_size); };
  std::unique_ptr<const uint8_t, decltype(unmap)> mapping(static_cast<const uint8_t *>(raw), unmap);
  ElfBinary elf;
  elf.raw = mapping.get();
  elf.size = s.st_size;
  elf.load();
  return loadElfSegments(elf, ptr, buf_size);
}

long loadElfSegments(const ElfBinary &elf, void *ptr, long buf_size) {
  if (elf.sections.empty()) {
    fprintf(stderr, "ELF image contains no loadable segments\n");
    return -1;
  }
  uint64_t extent = 0;
  auto base_addr = elf.sections[0].data_dst;
  if (base_addr != PMEM_BASE) {
    printf(
        "The first address in the elf does not match the base of the physical memory.\n"
        "It is likely that execution leads to unexpected behaviour.\n");
  }
  for (auto section: elf.sections) {
    auto offset = section.data_dst - base_addr;
    if (buf_size < 0 || offset > static_cast<uint64_t>(buf_size) ||
        section.data_len > static_cast<uint64_t>(buf_size) - offset ||
        section.zero_len > static_cast<uint64_t>(buf_size) - offset - section.data_len) {
      fprintf(stderr, "ELF segment at address 0x%lx exceeds memory capacity\n", section.data_dst);
      return -1;
    }
    auto len = section.data_len + section.zero_len;
    std::memset(static_cast<uint8_t *>(ptr) + offset, 0, len);
    std::memcpy(static_cast<uint8_t *>(ptr) + offset, section.data_src, section.data_len);
    extent = std::max(extent, offset + len);
  }
  return extent;
}
