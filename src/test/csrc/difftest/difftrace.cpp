#include "difftrace.h"
#include <fstream>
#include <limits.h>
#include <stdexcept>
#include <sys/stat.h>
#include <sys/types.h>

template <typename T>
DiffTrace<T>::DiffTrace(const char *_trace_name, bool is_read, uint64_t _buffer_size) : is_read(is_read) {
  if (!_buffer_size || _buffer_size > SIZE_MAX / sizeof(T) || !_trace_name[0])
    throw std::runtime_error("Invalid trace name or buffer size");
  buffer_size = _buffer_size;
  if (!is_read) {
    buffer = (T *)calloc(buffer_size, sizeof(T));
  }
  if (strlen(_trace_name) >= sizeof(trace_name)) {
    printf("Length of trace_name %s exceeds the path limit.\n", _trace_name);
    printf("Please use a shorter name.\n");
    exit(1);
  }
  strcpy(trace_name, _trace_name);
#ifdef CONFIG_IOTRACE_ZSTD
  trace_zstd = new DiffTraceZstd(buffer_size);
#endif
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
#ifndef CONFIG_IOTRACE_ZSTD
  if (!buffer || buffer_count == buffer_size) {
#else
  if (!buffer || trace_zstd->trace_load_len == buffer_count) {
#endif // CONFIG_IOTRACE_ZSTD
    trace_file_next();
  }
  memcpy(trace, buffer + buffer_count, sizeof(T));
  buffer_count++;
  // printf("%lu...\n", buffer_count);
  return 0;
}

template <typename T> bool DiffTrace<T>::try_read_next(T *trace) {
  if (!is_read)
    throw std::runtime_error("Cannot read from a trace writer");
  if (buffer_count == loaded_count) {
    char filename[PATH_MAX];
    next_file_name(filename);
    struct stat entry {};
    if (stat(filename, &entry)) {
      if (errno == ENOENT && trace_index > 1)
        return false;
      throw std::runtime_error(std::string("Cannot read trace: ") + filename);
    }
    if (!S_ISREG(entry.st_mode) || entry.st_size <= 0)
      throw std::runtime_error(std::string("Invalid trace file: ") + filename);
    std::ifstream file(filename, std::ios::binary);
#ifdef CONFIG_IOTRACE_ZSTD
    if (uint64_t(entry.st_size) > ZSTD_compressBound(buffer_size * sizeof(T)))
      throw std::runtime_error(std::string("Oversized compressed trace: ") + filename);
    std::vector<char> compressed(entry.st_size);
    file.read(compressed.data(), compressed.size());
    const auto bytes = ZSTD_getFrameContentSize(compressed.data(), compressed.size());
    if (!file || bytes == ZSTD_CONTENTSIZE_ERROR || bytes == ZSTD_CONTENTSIZE_UNKNOWN || !bytes ||
        bytes > buffer_size * sizeof(T) || bytes % sizeof(T))
      throw std::runtime_error(std::string("Invalid compressed trace: ") + filename);
#else
    const auto bytes = uint64_t(entry.st_size);
    if (bytes > buffer_size * sizeof(T) || bytes % sizeof(T))
      throw std::runtime_error(std::string("Invalid trace record size: ") + filename);
#endif
    free(buffer);
    buffer = (T *)calloc(bytes / sizeof(T), sizeof(T));
    if (!buffer)
      throw std::runtime_error("Cannot allocate trace buffer");
#ifdef CONFIG_IOTRACE_ZSTD
    const size_t size = ZSTD_decompress(buffer, bytes, compressed.data(), compressed.size());
    if (ZSTD_isError(size) || size != bytes)
      throw std::runtime_error(std::string("Trace decompression failed: ") + filename);
#else
    file.read(reinterpret_cast<char *>(buffer), bytes);
    if (!file)
      throw std::runtime_error(std::string("Truncated trace: ") + filename);
#endif
    buffer_count = 0;
    loaded_count = bytes / sizeof(T);
  }
  memcpy(trace, buffer + buffer_count++, sizeof(T));
  return true;
}

template <typename T> void DiffTrace<T>::next_file_name(char *file_name) {
  memset(file_name, 0, PATH_MAX);
  char dirname[PATH_MAX];
  int ret = 0;
  if (strchr(trace_name, '/')) {
    ret = snprintf(dirname, sizeof(dirname), "%s", trace_name);
  } else {
    char *noop_home = getenv("NOOP_HOME");
    if (!noop_home)
      throw std::runtime_error("NOOP_HOME is required for a relative trace name");
    ret = snprintf(dirname, sizeof(dirname), "%s/%s", noop_home, trace_name);
  }
  if (ret < 0 || ret >= (int)sizeof(dirname)) {
    throw std::runtime_error("Trace directory name is too long");
  }
  if (!is_read && mkdir(dirname, 0755) && errno != EEXIST)
    throw std::runtime_error(std::string("Cannot create trace directory: ") + dirname);
#ifndef CONFIG_IOTRACE_ZSTD
  const char *suffix = "bin";
#else
  const char *suffix = "zstd";
#endif // CONFIG_IOTRACE_ZSTD
  ret = snprintf(file_name, PATH_MAX, "%s/%lu.%s", dirname, trace_index, suffix);
  if (ret < 0 || ret >= PATH_MAX) {
    throw std::runtime_error("Trace filename is too long");
  }
  trace_index++;
}

template <typename T> bool DiffTrace<T>::trace_file_next() {
  if (!is_read && !buffer_count)
    return false;
  char filename[PATH_MAX];
#ifdef CONFIG_IOTRACE_ZSTD
  if (trace_zstd->need_load_new_file == true && is_read) {
    next_file_name(filename);
    trace_zstd->diff_zstd_next(filename, is_read);
    trace_zstd->need_load_new_file = false;
    Info("Loading traces from %s ...\n", filename);
  } else if (!is_read) {
    next_file_name(filename);
    trace_zstd->diff_zstd_next(filename, is_read);
  }
#else
  next_file_name(filename);
#endif

  if (is_read) {
    if (buffer) {
      free(buffer);
    }
#ifndef CONFIG_IOTRACE_ZSTD
    FILE *file = fopen(filename, "rb");
    if (!file) {
      printf("File %s not found.\n", filename);
      exit(0);
    }
    // check the number of traces
    fseek(file, 0, SEEK_END);
    buffer_size = ftell(file) / sizeof(T);
    buffer = (T *)calloc(buffer_size, sizeof(T));
    // read the binary file
    fseek(file, 0, SEEK_SET);
    uint64_t read_bytes = fread(buffer, sizeof(T), buffer_size, file);
    assert(read_bytes == buffer_size);
    fclose(file);
    Info("Loading %lu traces from %s ...\n", buffer_size, filename);
#else
    buffer = (T *)calloc(buffer_size, sizeof(T));
    trace_zstd->diff_IOtrace_load((char *)buffer, sizeof(T));
#endif // CONFIG_IOTRACE_ZSTD
  } else if (buffer_count > 0) {
    Info("Writing %lu traces to %s ...\n", buffer_count, filename);
#ifndef CONFIG_IOTRACE_ZSTD
    FILE *file = fopen(filename, "wb");
    if (!file)
      throw std::runtime_error(std::string("Cannot write trace: ") + filename);
    const auto written = fwrite(buffer, sizeof(T), buffer_count, file);
    const auto closed = fclose(file);
    if (written != buffer_count || closed)
      throw std::runtime_error(std::string("Trace write failed: ") + filename);
#else
    trace_zstd->diff_IOtrace_dump((char *)buffer, sizeof(T) * buffer_count);
#endif
  }
  buffer_count = 0;
  return 0;
}

template class DiffTrace<DiffTestState>;
template class DiffTrace<uint64_t>;

#ifdef CONFIG_IOTRACE_ZSTD
void DiffTraceZstd::diff_zstd_next(const char *file_name, bool is_read) {
  if (io_trace_file.is_open()) {
    io_trace_file.close();
  }
  if (is_read) {
    io_trace_file.open(file_name, std::ios::binary | std::ios::in);
    if (io_trace_file.is_open() == false) {
      printf("Run %s not find,No more trace files.End simulation\n", file_name);
      exit(0);
    }
  } else {
    io_trace_file.open(file_name, std::ios::binary | std::ios::out);
    if (!io_trace_file)
      throw std::runtime_error(std::string("Cannot write trace: ") + file_name);
  }
}

void DiffTraceZstd::diff_IOtrace_dump(const char *str, uint64_t len) {
  static const size_t cLevel = 1; // compression level

  std::vector<char> outputBuffer(ZSTD_compressBound(len));
  trace_cctx = ZSTD_createCCtx();

  size_t compressedSize = ZSTD_compressCCtx(trace_cctx, outputBuffer.data(), outputBuffer.size(), str, len, cLevel);
  if (ZSTD_isError(compressedSize)) {
    ZSTD_freeCCtx(trace_cctx);
    trace_cctx = NULL;
    throw std::runtime_error(ZSTD_getErrorName(compressedSize));
  }

  io_trace_file.write(outputBuffer.data(), compressedSize);
  io_trace_file.flush();
  ZSTD_freeCCtx(trace_cctx);
  trace_cctx = NULL;
  if (!io_trace_file)
    throw std::runtime_error("Compressed trace write failed");
}

bool DiffTraceZstd::diff_IOtrace_load(char *buffer, uint64_t len) {
  int result = diff_IOtrace_ZstdDcompress();
  if (result != 0) {
    need_load_new_file = true;
    return false;
  } else {
    uint64_t have_size = io_trace_buffer.size() / len;
    uint64_t byte_size = have_size * len;
    memcpy(buffer, io_trace_buffer.data(), byte_size);
    trace_load_len = have_size;
    // clear read data
    io_trace_buffer.erase(io_trace_buffer.begin(), io_trace_buffer.begin() + byte_size);
  }
  return true;
}

int DiffTraceZstd::diff_IOtrace_ZstdDcompress() {
  // Set up buffers
  static const size_t inbufferSize = ZSTD_DStreamInSize(); // Use ZSTD's recommended output buffer size
  static const size_t outbufferSize = ZSTD_DStreamOutSize();
  static std::vector<char> inputBuffer(inbufferSize);
  std::vector<char> outputBuffer(outbufferSize);

  if (trace_dctx == NULL) {
    trace_dctx = ZSTD_createDCtx();
  }
  // Read and decompress data in a loop
  ZSTD_outBuffer output = {outputBuffer.data(), outbufferSize, 0};
  static ZSTD_inBuffer input = {inputBuffer.data(), 0, 0};

  if (input.pos == input.size) {
    inputBuffer.resize(inbufferSize);
    io_trace_file.read(inputBuffer.data(), inbufferSize);
    input.size = io_trace_file.gcount();
    input.pos = 0;
  }

  // Always call the decoder, even when no new input was read: a previous call
  // may have filled outputBuffer while leaving output pending inside the
  // decoder, which has to be drained before the file can be considered
  // exhausted.  `ret == 0` marks the end of a frame.
  size_t ret = ZSTD_decompressStream(trace_dctx, &output, &input);
  (void)ret;

  io_trace_buffer.insert(io_trace_buffer.end(), outputBuffer.begin(), outputBuffer.begin() + output.pos);

  // No input left and nothing produced: the current file is fully consumed.
  if (input.size == 0 && output.pos == 0) {
    ZSTD_freeDCtx(trace_dctx);
    trace_dctx = NULL;
    return 1;
  }

  return 0;
}
#endif // CONFIG_IOTRACE_ZSTD

#ifdef CONFIG_DIFFTEST_IOTRACE
template class DiffTrace<DiffTestIOTrace>;
#endif // CONFIG_DIFFTEST_IOTRACE
