#include "difftrace.h"
#include <sys/stat.h>
#include <sys/types.h>

#ifdef CONFIG_IOTRACE_ZSTD
static DiffTraceZstd *trace_zstd = new DiffTraceZstd;
#endif // CONFIG_IOTRACE_ZSTD

template <typename T>
DiffTrace<T>::DiffTrace(const char *_trace_name, bool is_read, uint64_t _buffer_size) : is_read(is_read) {
  buffer_size = _buffer_size;
  if (!is_read) {
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

template <typename T> void DiffTrace<T>::next_file_name(char *file_name) {
  static uint64_t trace_index = 0;
  char dirname[128];
  if (strchr(trace_name, '/')) {
    snprintf(dirname, 128, "%s", trace_name);
  } else {
    char *noop_home = getenv("NOOP_HOME");
    snprintf(dirname, 128, "%s/%s", noop_home, trace_name);
  }
  mkdir(dirname, 0755);
#ifndef CONFIG_IOTRACE_ZSTD
  const char *prefix = "bin";
#else
  const char *prefix = "zstd";
#endif // CONFIG_IOTRACE_ZSTD
  snprintf(file_name, 128, "%s/%lu.%s", dirname, trace_index, prefix);
  trace_index++;
}

template <typename T> bool DiffTrace<T>::trace_file_next() {
  char *filename = (char *)malloc(128);
#ifdef CONFIG_IOTRACE_ZSTD
  if (trace_zstd->need_load_new_file == true && is_read) {
    next_file_name(filename);
    trace_zstd->diff_zstd_next(filename, is_read);
    trace_zstd->need_load_new_file = false;
  } else if (!is_read) {
    next_file_name(filename);
    trace_zstd->diff_zstd_next(filename, is_read);
  }
#else
  next_file_name(filename);
  static FILE *file = nullptr;
  if (file) {
    fclose(file);
  }
#endif

  if (is_read) {
#ifndef CONFIG_IOTRACE_ZSTD
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
#else
    buffer = (T *)calloc(buffer_size, sizeof(T));
    trace_zstd->diff_IOtrace_load((char *)buffer, sizeof(T));
#endif // CONFIG_IOTRACE_ZSTD
  } else if (buffer_count > 0) {
    Info("Writing %lu traces to %s ...\n", buffer_count, filename);
#ifndef CONFIG_IOTRACE_ZSTD
    FILE *file = fopen(filename, "wb");
    fwrite(buffer, sizeof(T), buffer_count, file);
    fclose(file);
#else
    trace_zstd->diff_IOtrace_dump((char *)buffer, sizeof(T) * buffer_count);
#endif
  }
  buffer_count = 0;
  return 0;
}

template class DiffTrace<DiffTestState>;

#ifdef CONFIG_IOTRACE_ZSTD
void DiffTraceZstd::diff_zstd_next(const char *file_name, bool is_read) {
  if (is_read) {
    if (io_trace_file_i.is_open()) {
      io_trace_file_i.close();
    }
    io_trace_file_i.open(file_name, std::ios::binary);
    if (io_trace_file_i.is_open() == false) {
      printf("No more trace files.End simulation\n");
      exit(0);
    }
  } else {
    if (io_trace_file_o.is_open()) {
      io_trace_file_o.close();
    }
    io_trace_file_o.open(file_name, std::ios::binary);
  }
}

void DiffTraceZstd::diff_IOtrace_dump(const char *str, uint64_t len) {
  static const size_t cLevel = 1; // compression level

  std::vector<char> outputBuffer(max_compress_size);
  trace_cctx = ZSTD_createCCtx();

  size_t compressedSize = ZSTD_compressCCtx(trace_cctx, outputBuffer.data(), outputBuffer.size(), str, len, cLevel);
  if (ZSTD_isError(compressedSize)) {
    std::cerr << "Zstd Compress error: " << ZSTD_getErrorName(compressedSize) << std::endl;
    ZSTD_freeCCtx(trace_cctx);
    return;
  }

  io_trace_file_o.write(outputBuffer.data(), compressedSize);
  ZSTD_freeCCtx(trace_cctx);
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
    //printf("iotrace load erase get size %ld len %ld\n", have_size, len);
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
    io_trace_file_i.read(inputBuffer.data(), inbufferSize);
    input.size = io_trace_file_i.gcount();
    input.pos = 0;

    // Outputs the current file pointer location
    std::streampos currentPos = io_trace_file_i.tellg();
    if (currentPos == -1) {
      std::cout << "Decompress read zstd file error" << std::endl;
      return 2;
    }
  } else if (input.size == 0) {
    ZSTD_freeDCtx(trace_dctx);
    trace_dctx = NULL;
    return 1;
  } else {
    input.size = input.size;
    input.pos = input.pos;
  }

  // Decompress the data
  size_t ret = ZSTD_decompressStream(trace_dctx, &output, &input);
#ifdef IOTRACE_ZSTD_DEBUG
  std::cout << "Current input file position: " << currentPos << std::endl;
  if (output.pos == output.size) {
    std::cout << "there might be some data left within internal buffers" << std::endl;
  }
  if (input.pos == input.size) {
    std::cout << "once a block has been processed, more data is needed" << std::endl;
  }
  if (ZSTD_isError(ret)) {
    std::cerr << "Decompression error: " << ZSTD_getErrorName(ret) << std::endl;
    exit(0);
  }
#endif // IOTRACE_ZSTD_DEBUG
  io_trace_buffer.insert(io_trace_buffer.end(), outputBuffer.begin(), outputBuffer.end());

  return 0;
}
#endif // CONFIG_IOTRACE_ZSTD

#ifdef CONFIG_DIFFTEST_IOTRACE
template class DiffTrace<DiffTestIOTrace>;
#endif // CONFIG_DIFFTEST_IOTRACE
