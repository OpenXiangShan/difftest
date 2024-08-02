#include "difftrace.h"
#include <sys/stat.h>
#include <sys/types.h>

template <typename T>
DiffTrace<T>::DiffTrace(const char *_trace_name, bool is_read, uint64_t _buffer_size) : is_read(is_read) {
  if (!is_read) {
    buffer_size = _buffer_size;
    buffer = (T *)calloc(buffer_size, sizeof(T));
  }
  if (strlen(trace_name) > 31) {
    printf("Length of trace_name %s is more than 31 characters.\n", trace_name);
    printf("Please use a shorter name.\n");
    exit(0);
  }
  strcpy(trace_name, _trace_name);
}

template <typename T> bool DiffTrace<T>::append(const T *trace) {
  memcpy(buffer + buffer_count, trace, sizeof(T));
  buffer_count++;
  if (buffer_count == buffer_size) {
    return trace_file_next();
  }
  return 0;
}

template <typename T> bool DiffTrace<T>::read_next(T *trace) {
  if (!buffer || buffer_count == buffer_size) {
    trace_file_next();
  }
  memcpy(trace, buffer + buffer_count, sizeof(T));
  buffer_count++;
  // printf("%lu...\n", buffer_count);
  return 0;
}

template <typename T> bool DiffTrace<T>::trace_file_next() {
  static uint64_t trace_index = 0;
  static FILE *file = nullptr;
  if (file) {
    fclose(file);
  }
  char dirname[128];
  if (strchr(trace_name, '/')) {
    snprintf(dirname, 128, "%s", trace_name);
  } else {
    char *noop_home = getenv("NOOP_HOME");
    snprintf(dirname, 128, "%s/%s", noop_home, trace_name);
  }
  mkdir(dirname, 0755);
  char filename[128];
  const char *prefix = "bin";
  snprintf(filename, 128, "%s/%lu.%s", dirname, trace_index, prefix);
  if (is_read) {
    FILE *file = fopen(filename, "rb");
    if (!file) {
      printf("File %s not found.\n", filename);
      exit(0);
    }
    // check the number of traces
    fseek(file, 0, SEEK_END);
    buffer_size = ftell(file) / sizeof(T);
    if (buffer) {
      free(buffer);
    }
    buffer = (T *)calloc(buffer_size, sizeof(T));
    // read the binary file
    Info("Loading %lu traces from %s ...\n", buffer_size, filename);
    fseek(file, 0, SEEK_SET);
    uint64_t read_bytes = fread(buffer, sizeof(T), buffer_size, file);
    assert(read_bytes == buffer_size);
    fclose(file);
    buffer_count = 0;
  } else if (buffer_count > 0) {
    Info("Writing %lu traces to %s ...\n", buffer_count, filename);
    FILE *file = fopen(filename, "wb");
    fwrite(buffer, sizeof(T), buffer_count, file);
    fclose(file);
    buffer_count = 0;
  }
  trace_index++;
  return 0;
}

template class DiffTrace<DiffTestState>;
#ifdef CONFIG_DIFFTEST_IOTRACE
template class DiffTrace<DiffTestIOTrace>;
#endif // CONFIG_DIFFTEST_IOTRACE
