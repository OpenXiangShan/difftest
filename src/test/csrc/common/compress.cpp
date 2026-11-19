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
#include <algorithm>
#include <climits>

double calcTime(timeval s, timeval e) {
  double sec, usec;
  sec = e.tv_sec - s.tv_sec;
  usec = e.tv_usec - s.tv_usec;
  return 1000 * sec + usec / 1000.0;
}

CompressionKind detect_compression(InputReader &input) {
  uint8_t magic[4];
  ssize_t n = input.peek(magic, sizeof(magic));
  if (n < 0)
    throw std::runtime_error("Cannot detect image compression");
  if (n >= 2 && magic[0] == 0x1f && magic[1] == 0x8b)
    return CompressionKind::Gzip;
  const uint8_t zstd_magic[] = {0x28, 0xb5, 0x2f, 0xfd};
  // A zstd stream may start with a skippable frame.
  if (n == 4 && (!memcmp(magic, zstd_magic, 4) ||
                 ((magic[0] & 0xf0) == 0x50 && magic[1] == 0x2a && magic[2] == 0x4d && magic[3] == 0x18)))
    return CompressionKind::Zstd;
  return CompressionKind::None;
}

#if !defined(NO_GZ_COMPRESSION) || !defined(NO_ZSTD_COMPRESSION)
static bool has_compression(const char *filename, CompressionKind kind) {
  auto input = open_input(filename);
  return detect_compression(*input) == kind;
}
#endif

bool isGzFile(const char *filename) {
#ifdef NO_GZ_COMPRESSION
  return false;
#else
  return has_compression(filename, CompressionKind::Gzip);
#endif
}

bool isZstdFile(const char *filename) {
#ifdef NO_ZSTD_COMPRESSION
  return false;
#else
  return has_compression(filename, CompressionKind::Zstd);
#endif
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

namespace {
#ifndef NO_GZ_COMPRESSION
class GzipReader : public InputReader {
public:
  explicit GzipReader(std::unique_ptr<InputReader> input) : input(std::move(input)) {
    if (inflateInit2(&stream, 15 + 16) != Z_OK)
      throw std::runtime_error("Cannot initialize gzip decoder");
  }
  ~GzipReader() override {
    inflateEnd(&stream);
  }

protected:
  ssize_t read_some(void *dest, size_t capacity) override {
    stream.next_out = static_cast<Bytef *>(dest);
    stream.avail_out = std::min<size_t>(capacity, UINT_MAX);
    uInt initial_out = stream.avail_out;
    for (;;) {
      if (frame_end) {
        // gzip permits zero padding after and between members.
        for (;;) {
          if (!stream.avail_in) {
            ssize_t n = refill();
            if (n <= 0)
              return n;
          }
          while (stream.avail_in && *stream.next_in == 0) {
            stream.next_in++;
            stream.avail_in--;
          }
          if (stream.avail_in)
            break;
        }
        // Reset only the codec state, preserving unread bytes of the next member.
        Bytef *next_in = stream.next_in;
        uInt avail_in = stream.avail_in;
        Bytef *next_out = stream.next_out;
        uInt avail_out = stream.avail_out;
        int ret = inflateReset2(&stream, 15 + 16);
        stream.next_in = next_in;
        stream.avail_in = avail_in;
        stream.next_out = next_out;
        stream.avail_out = avail_out;
        if (ret != Z_OK)
          return -1;
        frame_end = false;
      }
      uInt before_in = stream.avail_in;
      int ret = inflate(&stream, Z_NO_FLUSH);
      if (ret != Z_OK && ret != Z_STREAM_END && ret != Z_BUF_ERROR) {
        fprintf(stderr, "Gzip decompression failed: %s\n", stream.msg ? stream.msg : "invalid stream");
        return -1;
      }
      frame_end = ret == Z_STREAM_END;
      if (stream.avail_out != initial_out)
        return initial_out - stream.avail_out;
      if (frame_end)
        continue;
      if (!stream.avail_in) {
        ssize_t n = refill();
        if (n < 0)
          return -1;
        if (n == 0) {
          fprintf(stderr, "Truncated gzip stream\n");
          return -1;
        }
      } else if (stream.avail_in == before_in) {
        fprintf(stderr, "Gzip decoder made no progress\n");
        return -1;
      }
    }
  }

private:
  ssize_t refill() {
    ssize_t n = input->read(buffer, sizeof(buffer));
    if (n > 0) {
      stream.next_in = buffer;
      stream.avail_in = n;
    }
    return n;
  }

  std::unique_ptr<InputReader> input;
  z_stream stream = {};
  uint8_t buffer[1 << 16];
  bool frame_end = false;
};
#endif

#ifndef NO_ZSTD_COMPRESSION
class ZstdReader : public InputReader {
public:
  explicit ZstdReader(std::unique_ptr<InputReader> input) : input(std::move(input)) {
    stream = ZSTD_createDStream();
    if (!stream)
      throw std::runtime_error("Cannot create zstd decoder");
    size_t ret = ZSTD_initDStream(stream);
    if (ZSTD_isError(ret)) {
      ZSTD_freeDStream(stream);
      throw std::runtime_error("Cannot initialize zstd decoder");
    }
  }
  ~ZstdReader() override {
    ZSTD_freeDStream(stream);
  }

protected:
  ssize_t read_some(void *dest, size_t capacity) override {
    for (;;) {
      if (frame_end && compressed.pos == compressed.size) {
        ssize_t n = refill();
        if (n <= 0)
          return n;
      }
      size_t before = compressed.pos;
      ZSTD_outBuffer output = {dest, capacity, 0};
      size_t ret = ZSTD_decompressStream(stream, &output, &compressed);
      if (ZSTD_isError(ret)) {
        fprintf(stderr, "Zstd decompression failed: %s\n", ZSTD_getErrorName(ret));
        return -1;
      }
      frame_end = ret == 0;
      if (output.pos)
        return output.pos;
      if (frame_end)
        continue;
      if (compressed.pos == compressed.size) {
        ssize_t n = refill();
        if (n < 0)
          return -1;
        if (n == 0) {
          fprintf(stderr, "Truncated zstd stream\n");
          return -1;
        }
      } else if (compressed.pos == before) {
        fprintf(stderr, "Zstd decoder made no progress\n");
        return -1;
      }
    }
  }

private:
  ssize_t refill() {
    ssize_t n = input->read(buffer, sizeof(buffer));
    if (n > 0)
      compressed = {buffer, static_cast<size_t>(n), 0};
    return n;
  }

  std::unique_ptr<InputReader> input;
  ZSTD_DStream *stream;
  uint8_t buffer[1 << 16];
  ZSTD_inBuffer compressed = {nullptr, 0, 0};
  bool frame_end = false;
};
#endif
} // namespace

std::unique_ptr<InputReader> wrap_decompressor(std::unique_ptr<InputReader> input, CompressionKind kind) {
  switch (kind) {
    case CompressionKind::None: return input;
    case CompressionKind::Gzip:
#ifndef NO_GZ_COMPRESSION
      return std::unique_ptr<InputReader>(new GzipReader(std::move(input)));
#else
      throw std::runtime_error("Gzip image support is disabled");
#endif
    case CompressionKind::Zstd:
#ifndef NO_ZSTD_COMPRESSION
      return std::unique_ptr<InputReader>(new ZstdReader(std::move(input)));
#else
      throw std::runtime_error("Zstd image support is disabled");
#endif
  }
  throw std::runtime_error("Unknown compression format");
}

static long read_compressed_memory(void *ptr, const char *filename, long size, CompressionKind kind) {
  try {
    auto input = wrap_decompressor(open_input(filename), kind);
    return read_to_memory(*input, ptr, size, MemoryWriteMode::SkipZeroWords);
  } catch (const std::exception &e) {
    fprintf(stderr, "Cannot load compressed input '%s': %s\n", filename, e.what());
    return -1;
  }
}

long readFromGz(void *ptr, const char *file_name, long buf_size, uint8_t /* load_type */) {
  return read_compressed_memory(ptr, file_name, buf_size, CompressionKind::Gzip);
}

long readFromZstd(void *ptr, const char *file_name, long buf_size, uint8_t /* load_type */) {
  return read_compressed_memory(ptr, file_name, buf_size, CompressionKind::Zstd);
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
