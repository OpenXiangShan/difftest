/***************************************************************************************
* Copyright (c) 2020-2023 Institute of Computing Technology, Chinese Academy of Sciences
* Copyright (c) 2020-2021 Peng Cheng Laboratory
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

#include <fstream>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <iostream>
#include "trace_icache.h"

TraceICache::TraceICache(const char *binary_name) {
  // uint64_t memory_size = 8 * 1024 * 1024 * 1024UL; // 8GB
  fd = open(binary_name, O_RDONLY);
  if (fd == -1) {
    perror("open file failed.");
    exit(EXIT_FAILURE);
    return;
  }
  struct stat sb;
  if (fstat(fd, &sb) == -1) {
    perror("fstat failed.");
    close(fd);
    exit(EXIT_FAILURE);
  }

  mmap_size = sb.st_size;
  ram_size = mmap_size;
  ram = (char *)mmap(NULL, mmap_size, PROT_READ, MAP_PRIVATE, fd, 0);

  if (ram == MAP_FAILED) {
    perror("mmap failed.");
    close(fd);
    exit(EXIT_FAILURE);
  }

  base_addr = 0x80000000; // 2GB
  printf("Init TraceICache %s baseAddr %08lx ram_size %08lx\n", binary_name, base_addr, ram_size);
}

bool TraceICache::readCacheLine(char *line, uint64_t addr) {
  if (!legalAddr(addr)) {
    // perror("illegal address");
    // exit(EXIT_FAILURE);
    return false;
  }

  uint64_t ram_addr = ramAddr(addr, sizeof(TraceCacheLine));
  memcpy(line, (char *)(ram + ram_addr), sizeof(TraceCacheLine));
  return true;
}

bool TraceICache::readHalfCacheLine(char *line, uint64_t addr) {
  if (!legalAddr(addr)) {
    // perror("illegal address");
    // exit(EXIT_FAILURE);
    return false;
  }

  uint64_t ram_addr = ramAddr(addr, sizeof(TraceHalfCacheLine));
  memcpy(line, (char *)(ram + ram_addr), sizeof(TraceHalfCacheLine));
  return true;
}

bool TraceICache::readDWord(uint64_t *dest, uint64_t addr) {
  if (!legalAddr(addr)) {
    // perror("illegal address");
    // exit(EXIT_FAILURE);
    return false;
  }

  uint64_t ram_addr = ramAddr(addr, sizeof(uint64_t));
  memcpy(dest, (char *)(ram + ram_addr), sizeof(uint64_t));
  return true;
}

TraceICache::~TraceICache() {
  close(fd);
  munmap(ram, mmap_size);
}