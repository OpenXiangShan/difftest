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

#include "compress.h"

double calcTime(timeval s, timeval e) {
  double sec, usec;
  sec = e.tv_sec - s.tv_sec;
  usec = e.tv_usec - s.tv_usec;
  return 1000 * sec + usec / 1000.0;
}

// Return whether the file is a gz file
bool isGzFile(const char *filename) {
#ifdef NO_GZ_COMPRESSION
  return false;
#endif
  int fd = -1;

  fd = open(filename, O_RDONLY);
  assert(fd);

  uint8_t buf[2];

  size_t sz = read(fd, buf, 2);
  if (sz != 2) {
    close(fd);
    return false;
  }

  close(fd);

  const uint8_t gz_magic[2] = {0x1f, 0x8B};
  return memcmp(buf, gz_magic, 2) == 0;
}

// Return whether the file is a zstd file
bool isZstdFile(const char *filename) {
#ifdef NO_ZSTD_COMPRESSION
  return false;
#endif
  int fd = -1;

  fd = open(filename, O_RDONLY);
  assert(fd);

  uint8_t buf[4];

  size_t sz = read(fd, buf, 4);
  if (sz != 4) {
    close(fd);
    return false;
  }

  close(fd);

  const uint8_t zstd_magic[4] = {0x28, 0xB5, 0x2F, 0xFD};
  return memcmp(buf, zstd_magic, 4) == 0;
}

long snapshot_compressToFile(uint8_t *ptr, const char *filename, long buf_size) {
#ifndef NO_GZ_COMPRESSION
  gzFile compressed_mem = gzopen(filename, "wb");

  if (compressed_mem == NULL) {
    printf("Can't open compressed binary file '%s'", filename);
    return -1;
  }

  long curr_size = 0;
  const uint32_t chunk_size = 16384;
  long *temp_page = new long[chunk_size];
  long *pmem_current = (long *)ptr;

  while (curr_size < buf_size) {
    memset(temp_page, 0, chunk_size * sizeof(long));
    for (uint32_t x = 0; x < chunk_size / sizeof(long); x++) {
      pmem_current = (long *)((uint8_t *)ptr + curr_size + x * sizeof(long));
      if (*pmem_current != 0) {
        *(temp_page + x) = *pmem_current;
      }
    }
    uint32_t bytes_write = gzwrite(compressed_mem, temp_page, chunk_size);
    if (bytes_write <= 0) {
      printf("Compress failed\n");
      break;
    }
    curr_size += bytes_write;
    // assert(bytes_write % sizeof(long) == 0);
  }
  // printf("Write %lu bytes from gz stream in total\n", curr_size);

  delete[] temp_page;

  if (gzclose(compressed_mem)) {
    printf("Error closing '%s'\n", filename);
    return -1;
  }
  return curr_size;
#else
  return 0;
#endif
}

long readFromGz(void *ptr, const char *file_name, long buf_size, uint8_t load_type) {
#ifndef NO_GZ_COMPRESSION
  assert(buf_size > 0);
  gzFile compressed_mem = gzopen(file_name, "rb");

  if (compressed_mem == NULL) {
    printf("Can't open compressed binary file '%s'", file_name);
    return -1;
  }

  uint64_t curr_size = 0;
  const uint32_t chunk_size = 16384;

  // Only load from RAM need check
  if (load_type == LOAD_RAM && (buf_size % chunk_size) != 0) {
    printf("buf_size must be divisible by chunk_size\n");
    assert(0);
  }

  long *temp_page = new long[chunk_size];

  while (curr_size < buf_size) {
    uint32_t bytes_read = gzread(compressed_mem, temp_page, chunk_size * sizeof(long));
    if (bytes_read == 0) {
      break;
    }
    for (uint32_t x = 0; x < bytes_read / sizeof(long); x++) {
      if (*(temp_page + x) != 0) {
        long *pmem_current = (long *)((uint8_t *)ptr + curr_size + x * sizeof(long));
        *pmem_current = *(temp_page + x);
      }
    }
    curr_size += bytes_read;
  }

  if (gzread(compressed_mem, temp_page, chunk_size) > 0) {
    printf("File size is larger than buf_size!\n");
    assert(0);
  }
  // printf("Read %lu bytes from gz stream in total\n", curr_size);

  delete[] temp_page;

  if (gzclose(compressed_mem)) {
    printf("Error closing '%s'\n", file_name);
    return -1;
  }
  return curr_size;
#else
  return 0;
#endif
}

long readFromZstd(void *ptr, const char *file_name, long buf_size, uint8_t load_type) {
#ifndef NO_ZSTD_COMPRESSION
  assert(buf_size > 0);

  int fd = -1;
  ssize_t file_size = 0;
  uint8_t *compress_file_buffer = NULL;
  size_t compress_file_buffer_size = 0;

  uint64_t curr_size = 0;
  const uint32_t chunk_size = 16384;

  // Only load from RAM need check
  if (load_type == LOAD_RAM && (buf_size % chunk_size) != 0) {
    printf("buf_size must be divisible by chunk_size\n");
    return -1;
  }

  fd = open(file_name, O_RDONLY);
  if (fd < 0) {
    printf("Can't open compress binary file '%s'", file_name);
    return -1;
  }

  file_size = lseek(fd, 0, SEEK_END);
  if (file_size == 0) {
    printf("File size must not be zero");
    return -1;
  }

  lseek(fd, 0, SEEK_SET);

  compress_file_buffer = new uint8_t[file_size];
  assert(compress_file_buffer);

  ssize_t read_cnt = 0;
  while (read_cnt < file_size) {
    ssize_t read_now = read(fd, compress_file_buffer + read_cnt, file_size - read_cnt);
    if (read_now <= 0) {
      free(compress_file_buffer);
      close(fd);
      printf("Zstd compress file read failed, read() return %zd\n", read_now);
      return -1;
    }
    read_cnt += read_now;
  }
  compress_file_buffer_size = read_cnt;
  if (compress_file_buffer_size != file_size) {
    close(fd);
    free(compress_file_buffer);
    printf("Zstd compressed file read failed, file size: %zd, read size: %zd\n", file_size, compress_file_buffer_size);
    return -1;
  }

  close(fd);

  ZSTD_inBuffer input_buffer = {compress_file_buffer, compress_file_buffer_size, 0};

  long *temp_page = new long[chunk_size];

  ZSTD_DStream *dstream = ZSTD_createDStream();
  if (!dstream) {
    printf("Can't create zstd dstream object\n");
    delete[] compress_file_buffer;
    delete[] temp_page;
    return -1;
  }

  size_t init_result = ZSTD_initDStream(dstream);
  if (ZSTD_isError(init_result)) {
    printf("Can't init zstd dstream object: %s\n", ZSTD_getErrorName(init_result));
    ZSTD_freeDStream(dstream);
    delete[] compress_file_buffer;
    delete[] temp_page;
    return -1;
  }

  while (curr_size < buf_size) {

    ZSTD_outBuffer output_buffer = {temp_page, chunk_size * sizeof(long), 0};

    size_t decompress_result = ZSTD_decompressStream(dstream, &output_buffer, &input_buffer);

    if (ZSTD_isError(decompress_result)) {
      printf("Decompress failed: %s\n", ZSTD_getErrorName(decompress_result));
      ZSTD_freeDStream(dstream);
      delete[] compress_file_buffer;
      delete[] temp_page;
      return -1;
    }

    if (output_buffer.pos == 0) {
      break;
    }

    for (uint32_t x = 0; x < output_buffer.pos / sizeof(long); x++) {
      if (*(temp_page + x) != 0) {
        long *pmem_current = (long *)((uint8_t *)ptr + curr_size + x * sizeof(long));
        *pmem_current = *(temp_page + x);
      }
    }
    curr_size += output_buffer.pos;
  }

  ZSTD_outBuffer output_buffer = {temp_page, chunk_size * sizeof(long), 0};
  size_t decompress_result = ZSTD_decompressStream(dstream, &output_buffer, &input_buffer);
  if (ZSTD_isError(decompress_result) || output_buffer.pos != 0) {
    printf("Decompress failed: %s\n", ZSTD_getErrorName(decompress_result));
    printf("Binary size larger than memory\n");
    ZSTD_freeDStream(dstream);
    delete[] compress_file_buffer;
    delete[] temp_page;
    return -1;
  }

  ZSTD_freeDStream(dstream);
  delete[] compress_file_buffer;
  delete[] temp_page;

  return curr_size;
#else
  return 0;
#endif
}

void nonzero_large_memcpy(const void *__restrict dest, const void *__restrict src, size_t n) {
  uint64_t *_dest = (uint64_t *)dest;
  uint64_t *_src = (uint64_t *)src;
  while (n >= sizeof(uint64_t)) {
    if (*_src != 0) {
      *_dest = *_src;
    }
    _dest++;
    _src++;
    n -= sizeof(uint64_t);
  }
  if (n > 0) {
    uint8_t *dest8 = (uint8_t *)_dest;
    uint8_t *src8 = (uint8_t *)_src;
    while (n > 0) {
      *dest8 = *src8;
      dest8++;
      src8++;
      n--;
    }
  }
}
