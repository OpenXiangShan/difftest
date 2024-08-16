#ifndef __DIFFTRACE_H__
#define __DIFFTRACE_H__

#include "common.h"
#ifdef CONFIG_DIFFTEST_IOTRACE
#include "difftest-iotrace.h"
#endif // CONFIG_DIFFTEST_IOTRACE
#ifdef CONFIG_IOTRACE_ZSTD
#include <fstream>
#include <iostream>
#include <vector>
#include <zstd.h>
#endif // CONFIG_IOTRACE_ZSTD

#ifdef CONFIG_IOTRACE_ZSTD
class DiffTraceZstd {
public:
  int trace_load_len = 0;
  bool need_load_new_file = true;

  DiffTraceZstd(uint64_t buffer_size) {
    trace_buffer_size = buffer_size;
    max_compress_size = 5000 * trace_buffer_size / 10;
    max_dcompress_size = 5000 * trace_buffer_size;
    io_trace_buffer.reserve(max_dcompress_size);
  };

  ~DiffTraceZstd() {
    ZSTD_freeCCtx(trace_cctx);
  }

  void diff_zstd_next(const char *file_name, bool is_read);

  void diff_IOtrace_dump(const char *str, uint64_t len);

  bool diff_IOtrace_load(char *buffer, uint64_t len);
  int diff_IOtrace_ZstdDcompress();

private:
  uint64_t trace_buffer_size = 0;
  uint64_t max_compress_size = 0; // The number of bytes in a single compression
  uint64_t max_dcompress_size = 0;
  std::vector<char> io_trace_buffer;

  ZSTD_CCtx *trace_cctx = NULL;
  ZSTD_DCtx *trace_dctx = NULL;
  std::fstream io_trace_file;
};
#endif // CONFIG_IOTRACE_ZSTD

template <typename T> class DiffTrace {
public:
  char trace_name[32];
  bool is_read;
#ifdef CONFIG_IOTRACE_ZSTD
  DiffTraceZstd *trace_zstd = NULL;
#endif // CONFIG_IOTRACE_ZSTD

  DiffTrace(const char *trace_name, bool is_read, uint64_t buffer_size = 1024 * 1024);
  ~DiffTrace() {
    if (!is_read) {
      trace_file_next();
    }
    if (buffer) {
      free(buffer);
    }
#ifdef CONFIG_IOTRACE_ZSTD
    delete trace_zstd;
#endif // CONFIG_IOTRACE_ZSTD
  }
  bool append(const T *trace);
  bool read_next(T *trace);
  void next_file_name(char *file_name);

private:
  uint64_t buffer_size;
  uint64_t buffer_count = 0;
  T *buffer = nullptr;

  bool trace_file_next();
};

#endif
