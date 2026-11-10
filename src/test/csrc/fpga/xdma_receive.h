#ifndef DIFFTEST_XDMA_RECEIVE_H
#define DIFFTEST_XDMA_RECEIVE_H

#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
#include <sys/types.h>
#include <system_error>

namespace xdma_receive {

// Own the read batch until every complete record has been consumed. In pool
// mode the kernel returns whole records; ordinary streams may fragment them.
class RecordReader {
public:
  RecordReader(size_t record_bytes, size_t read_records, bool whole_records)
      : record_bytes_(record_bytes), whole_records_(whole_records) {
    if (!record_bytes || !read_records || read_records > std::numeric_limits<size_t>::max() / record_bytes)
      throw std::invalid_argument("Invalid XDMA receive batch size");
    capacity_ = record_bytes * read_records;
    void *memory = nullptr;
    const int error = posix_memalign(&memory, 4096, capacity_);
    if (error)
      throw std::system_error(error, std::generic_category(), "XDMA receive allocation");
    memory_.reset(static_cast<uint8_t *>(memory));
  }

  template <typename Read, typename KeepRunning> const uint8_t *next(Read read, KeepRunning keep_running) {
    if (held_) {
      begin_ += record_bytes_;
      held_ = false;
    }
    while (keep_running()) {
      if (end_ - begin_ >= record_bytes_) {
        held_ = true;
        return memory_.get() + begin_;
      }
      const size_t retained = end_ - begin_;
      if (retained && begin_)
        std::memmove(memory_.get(), memory_.get() + begin_, retained);
      begin_ = 0;
      end_ = retained;
      const size_t requested = capacity_ - end_;
      const ssize_t received = read(memory_.get() + end_, requested);
      if (!keep_running())
        return nullptr;
      if (received < 0) {
        if (errno == EINTR)
          continue;
        throw std::system_error(errno, std::generic_category(), "XDMA receive");
      }
      if (received == 0)
        throw std::runtime_error(retained ? "XDMA stream ended inside a packet" : "XDMA stream ended");
      if (static_cast<size_t>(received) > requested ||
          (whole_records_ && static_cast<size_t>(received) % record_bytes_))
        throw std::runtime_error("XDMA receive returned an invalid packet length");
      end_ += static_cast<size_t>(received);
    }
    return nullptr;
  }

private:
  std::unique_ptr<uint8_t, decltype(&std::free)> memory_{nullptr, &std::free};
  size_t record_bytes_ = 0, capacity_ = 0, begin_ = 0, end_ = 0;
  bool whole_records_ = false, held_ = false;
};

class PacketSequence {
public:
  void accept(uint8_t actual) {
    if (actual != expected_)
      throw std::runtime_error("XDMA packet sequence mismatch: got " + std::to_string(actual) + ", expected " +
                               std::to_string(expected_));
    expected_ = static_cast<uint8_t>(expected_ + 1);
  }

private:
  uint8_t expected_ = 0;
};

template <typename KeepRunning, typename Decode> void dispatch(size_t count, KeepRunning keep_running, Decode decode) {
  for (size_t i = 0; i < count && keep_running(); ++i)
    decode(i);
}

} // namespace xdma_receive
#endif
