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

#ifndef __ELFLOADER_H
#define __ELFLOADER_H

#include "common.h"
#include <assert.h>
#include <fcntl.h>
#include <iostream>
#include <map>
#include <string>
#include <sys/stat.h>
#include <vector>

#define IS_ELF(hdr) \
  ((hdr).e_ident[0] == 0x7f && (hdr).e_ident[1] == 'E' && (hdr).e_ident[2] == 'L' && (hdr).e_ident[3] == 'F')

#define IS_ELF32(hdr) (IS_ELF(hdr) && (hdr).e_ident[4] == 1)
#define IS_ELF64(hdr) (IS_ELF(hdr) && (hdr).e_ident[4] == 2)

#define PT_LOAD      1
#define SHT_NOBITS   8
#define SHT_PROGBITS 0x1
#define SHT_GROUP    0x11

typedef struct {
  uint8_t e_ident[16];
  uint16_t e_type;
  uint16_t e_machine;
  uint32_t e_version;
  uint32_t e_entry;
  uint32_t e_phoff;
  uint32_t e_shoff;
  uint32_t e_flags;
  uint16_t e_ehsize;
  uint16_t e_phentsize;
  uint16_t e_phnum;
  uint16_t e_shentsize;
  uint16_t e_shnum;
  uint16_t e_shstrndx;
} Elf32_Ehdr;

typedef struct {
  uint32_t sh_name;
  uint32_t sh_type;
  uint32_t sh_flags;
  uint32_t sh_addr;
  uint32_t sh_offset;
  uint32_t sh_size;
  uint32_t sh_link;
  uint32_t sh_info;
  uint32_t sh_addralign;
  uint32_t sh_entsize;
} Elf32_Shdr;

typedef struct {
  uint32_t p_type;
  uint32_t p_offset;
  uint32_t p_vaddr;
  uint32_t p_paddr;
  uint32_t p_filesz;
  uint32_t p_memsz;
  uint32_t p_flags;
  uint32_t p_align;
} Elf32_Phdr;

typedef struct {
  uint32_t st_name;
  uint32_t st_value;
  uint32_t st_size;
  uint8_t st_info;
  uint8_t st_other;
  uint16_t st_shndx;
} Elf32_Sym;

typedef struct {
  uint8_t e_ident[16];
  uint16_t e_type;
  uint16_t e_machine;
  uint32_t e_version;
  uint64_t e_entry;
  uint64_t e_phoff;
  uint64_t e_shoff;
  uint32_t e_flags;
  uint16_t e_ehsize;
  uint16_t e_phentsize;
  uint16_t e_phnum;
  uint16_t e_shentsize;
  uint16_t e_shnum;
  uint16_t e_shstrndx;
} Elf64_Ehdr;

typedef struct {
  uint32_t sh_name;
  uint32_t sh_type;
  uint64_t sh_flags;
  uint64_t sh_addr;
  uint64_t sh_offset;
  uint64_t sh_size;
  uint32_t sh_link;
  uint32_t sh_info;
  uint64_t sh_addralign;
  uint64_t sh_entsize;
} Elf64_Shdr;

typedef struct {
  uint32_t p_type;
  uint32_t p_flags;
  uint64_t p_offset;
  uint64_t p_vaddr;
  uint64_t p_paddr;
  uint64_t p_filesz;
  uint64_t p_memsz;
  uint64_t p_align;
} Elf64_Phdr;

typedef struct {
  uint32_t st_name;
  uint8_t st_info;
  uint8_t st_other;
  uint16_t st_shndx;
  uint64_t st_value;
  uint64_t st_size;
} Elf64_Sym;

template <typename ehdr_t, typename phdr_t, typename shdr_t, typename sym_t> struct ElfBinaryData {
  const ehdr_t *eh = nullptr;
  const phdr_t *ph = nullptr;
};

struct ElfSection {
  const uint8_t *data_src;
  uint64_t data_dst;
  size_t data_len;
  uint64_t zero_dst;
  size_t zero_len;
};

struct ElfBinary {
  const uint8_t *raw = nullptr;
  size_t size = 0;

  const Elf64_Ehdr *eh64 = nullptr;
  ElfBinaryData<Elf32_Ehdr, Elf32_Phdr, Elf32_Shdr, Elf32_Sym> data32;
  ElfBinaryData<Elf64_Ehdr, Elf64_Phdr, Elf64_Shdr, Elf64_Sym> data64;

  uint64_t entry;
  std::vector<ElfSection> sections;

  void load();

  template <typename ehdr_t, typename phdr_t, typename shdr_t, typename sym_t>
  void parse(ElfBinaryData<ehdr_t, phdr_t, shdr_t, sym_t> &data);
};

struct ElfBinaryFile : public ElfBinary {
  const std::string filename;

  ElfBinaryFile(const char *filename);

  ~ElfBinaryFile();
};

// Is the file at the given path an Elf file
bool isElfFile(const char *filename);
// load binary content at `file_name` into ptr. Returns the number of bytes
// written.
long readFromElf(void *ptr, const char *file_name, long buf_size);

#endif // __ELFLOADER_H
