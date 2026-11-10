#ifndef DIFFTEST_MATRIX_WB_FASTPATH_H
#define DIFFTEST_MATRIX_WB_FASTPATH_H

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <stdexcept>

namespace matrix_wb_fastpath {

// A PACKET-only slot can retain the proven absence of all other valid events.
// The generated parser invalidates every slot before entering its generic path.
// Always recheck this step's WB/trap/WFI, even when the other-valid proof is hot.
// The full classifier remains the only way to establish a positive cache entry.
template <typename FullClassifier>
inline bool classify_cached(bool *other_events_clean, bool has_writeback, bool has_trap, bool has_wfi,
                            FullClassifier full_classifier) {
  if (!other_events_clean)
    return full_classifier();
  if (!has_writeback || has_trap || has_wfi) {
    *other_events_clean = false;
    return false;
  }
  if (*other_events_clean)
    return true;
  return *other_events_clean = full_classifier();
}

// A logical step may span multiple wire steps while Delta reconstruction is
// pending. Only complete() resets this classification, never a wire boundary.
class LogicalStep {
public:
  void observe(bool writeback) {
    writeback_seen_ |= writeback;
    only_writeback_ &= writeback;
  }
  bool empty() const {
    return only_writeback_ && !writeback_seen_;
  }
  bool only_writeback() const {
    return only_writeback_ && writeback_seen_;
  }
  bool complete() {
    const bool result = only_writeback();
    writeback_seen_ = false;
    only_writeback_ = true;
    return result;
  }

private:
  bool writeback_seen_ = false, only_writeback_ = true;
};

// A single packed event is followed by one-byte core/index locators. Copy
// each chunk directly into the destination event, ignoring only wire padding.
// No pointer into the incoming packet survives receive().
class DirectWriteback {
public:
  bool receive(void *event, size_t event_bytes, const uint8_t *chunk, size_t chunk_bytes) {
    if (!event || !event_bytes || !chunk || !chunk_bytes || chunk_bytes == SIZE_MAX ||
        event_bytes > SIZE_MAX - chunk_bytes - 1)
      throw std::runtime_error("Invalid direct matrix writeback layout");
    const size_t wire_bytes = (event_bytes + 2 + chunk_bytes - 1) / chunk_bytes * chunk_bytes;
    if (offset_ >= wire_bytes || chunk_bytes > wire_bytes - offset_)
      throw std::runtime_error("Direct matrix writeback exceeded its cluster");
    for (const size_t locator: {event_bytes, event_bytes + 1}) {
      if (locator >= offset_ && locator - offset_ < chunk_bytes && chunk[locator - offset_] != 0)
        throw std::runtime_error("Invalid direct matrix writeback core or index");
    }
    if (offset_ < event_bytes)
      std::memcpy(static_cast<uint8_t *>(event) + offset_, chunk, std::min(chunk_bytes, event_bytes - offset_));
    offset_ += chunk_bytes;
    if (offset_ != wire_bytes)
      return false;
    offset_ = 0;
    return true;
  }

private:
  size_t offset_ = 0;
};

// The common tiny-matrix wire step is exactly four 32-byte chunks: one
// metadata chunk followed by a 79-byte event and one-byte core/index locators.
// Process its two 64-byte callbacks without the generic info/data buffers.
// The caller establishes a clean parser/logical/Delta boundary before the
// first callback. No pointer into an incoming DMA/read buffer is retained.
class PacketWriteback {
public:
  enum class Result {
    Miss,
    Pending,
    Complete
  };
  bool pending() const {
    return pending_;
  }

  Result receive(void *event, size_t event_bytes, const uint8_t *beat, size_t beat_bytes, uint8_t head_id,
                 uint8_t writeback_id, uint8_t step_id) {
    if (!event || !beat)
      throw std::runtime_error("Invalid packet matrix writeback buffer");
    if (event_bytes != 79 || beat_bytes != 64) {
      if (pending_)
        throw std::runtime_error("Changed packet matrix writeback layout");
      return Result::Miss;
    }
    if (!pending_) {
      if (beat[0] != 1 || beat[1] != head_id || beat[2] != 1 || beat[3] != writeback_id || beat[4] != 4 ||
          beat[5] != step_id)
        return Result::Miss;
      std::memcpy(event, beat + 32, 32);
      pending_ = true;
      return Result::Pending;
    }
    // The remaining event starts at wire offset 32; locators at 79/80 are
    // therefore bytes 47/48 in this callback. Validate before modifying it.
    if (beat[47] || beat[48])
      throw std::runtime_error("Invalid packet matrix writeback core or index");
    std::memcpy(static_cast<uint8_t *>(event) + 32, beat, 47);
    pending_ = false;
    return Result::Complete;
  }

private:
  bool pending_ = false;
};

} // namespace matrix_wb_fastpath
#endif
