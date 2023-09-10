#include <sys/stat.h>
#include <sys/types.h>

#include "difftrace.h"

DiffTrace::DiffTrace(const char *_trace_name, bool is_read, uint64_t _buffer_size)
  : is_read(is_read) {
  if (!is_read) {
    buffer_size = _buffer_size;
    buffer = (DiffTestState *)calloc(buffer_size, sizeof(DiffTestState));
  }
  if (strlen(trace_name) > 31) {
    printf("Length of trace_name %s is more than 31 characters.\n", trace_name);
    printf("Please use a shorter name.\n");
    exit(0);
  }
  strcpy(trace_name, _trace_name);
}

bool DiffTrace::append(const DiffTestState *trace) {
  memcpy(buffer + buffer_count, trace, sizeof(DiffTestState));
  buffer_count++;
  if (buffer_count == buffer_size) {
    return trace_file_next();
  }
  return 0;
}

bool DiffTrace::read_next(DiffTestState *trace) {
  if (!buffer || buffer_count == buffer_size) {
    trace_file_next();
  }
  memcpy(trace, buffer + buffer_count, sizeof(DiffTestState));
  buffer_count++;
  // printf("%lu...\n", buffer_count);
  return 0;
}


bool DiffTrace::trace_file_next() {
  static uint64_t trace_index = 0;
  static FILE *file = nullptr;
  if (file) {
    fclose(file);
  }
  char filename[128];
  char *noop_home = getenv("NOOP_HOME");
  snprintf(filename, 128, "%s/%s", noop_home, trace_name);
  mkdir(filename, 0755);
  const char *prefix = "bin";
  snprintf(filename, 128, "%s/%s/%lu.%s", noop_home, trace_name, trace_index, prefix);
  if (is_read) {
    FILE *file = fopen(filename, "rb");
    if (!file) {
      printf("File %s not found.\n", filename);
      exit(0);
    }
    // check the number of traces
    fseek(file, 0, SEEK_END);
    buffer_size = ftell(file) / sizeof(DiffTestState);
    if (buffer) {
      free(buffer);
    }
    buffer = (DiffTestState *)calloc(buffer_size, sizeof(DiffTestState));
    // read the binary file
    Info("Loading %lu traces from %s ...\n", buffer_size, filename);
    fseek(file, 0, SEEK_SET);
    uint64_t read_bytes = fread(buffer, sizeof(DiffTestState), buffer_size, file);
    assert(read_bytes == buffer_size);
    fclose(file);
    buffer_count = 0;
  }
  else if (buffer_count > 0) {
    Info("Writing %lu traces to %s ...\n", buffer_count, filename);
    FILE *file = fopen(filename, "wb");
    fwrite(buffer, sizeof(DiffTestState), buffer_count, file);
    fclose(file);
    buffer_count = 0;
  }
  trace_index++;
  return 0;
}
