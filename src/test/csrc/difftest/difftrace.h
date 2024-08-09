#ifndef __DIFFTRACE_H__
#define __DIFFTRACE_H__

#include "common.h"
#ifdef CONFIG_DIFFTEST_IOTRACE
#include "difftest-iotrace.h"
#endif // CONFIG_DIFFTEST_IOTRACE

template <typename T> class DiffTrace {
public:
  char trace_name[32];
  bool is_read;

  DiffTrace(const char *trace_name, bool is_read, uint64_t buffer_size = 1024 * 1024);
  ~DiffTrace() {
    if (!is_read) {
      trace_file_next();
    }
    if (buffer) {
      free(buffer);
    }
  }
  bool append(const T *trace);
  bool read_next(T *trace);

private:
  uint64_t buffer_size;
  uint64_t buffer_count = 0;
  T *buffer = nullptr;

  bool trace_file_next();
};

#endif
