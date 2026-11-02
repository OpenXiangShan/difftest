#include "gbus_transport.h"
#include "difftest-dpic.h"
#include "xdma.h"
#include <algorithm>
#include <cerrno>
#include <chrono>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <unistd.h>
#include <uvaps_gbus_runtime.h>
#include <vector>

// Standalone transport tests may use a generated difftest header from a
// non-FPGA target, which omits this FPGA Batch declaration.  Keep the test
// boundary ABI explicit; production FPGA builds get the same symbol from the
// generated FPGA header.
extern "C" void v_difftest_Batch(uint8_t io[CONFIG_DIFFTEST_BATCH_BYTELEN]);

namespace {
// GBD1 config window, in GeneralBD local offsets.  The RTL decodes these
// inside the same 0x1000 config window the DiffTest BAR uses.
constexpr uint64_t GBUS_REG_C2H_STATUS = 0x1200;
constexpr uint64_t GBUS_REG_C2H_CTRL = 0x1204;
constexpr uint64_t GBUS_REG_C2H_ID = 0x1208;
constexpr uint32_t GBUS_C2H_DMA_ID_MAGIC = 0x47424431u; // ASCII "GBD1"
constexpr uint64_t GBUS_REG_C2H_DMA_BASE = 0x1210;
constexpr uint64_t GBUS_REG_C2H_CAPACITY = 0x1214;
constexpr uint64_t GBUS_REG_C2H_FEATURE = 0x1238;
constexpr uint32_t GBUS_C2H_FEATURE_PRESENT = 1U << 0;
constexpr uint32_t GBUS_C2H_FEATURE_STREAM = 1U << 2;
constexpr uint64_t GBUS_REG_C2H_AVAILABLE = 0x123c; // read: frames; write: CONSUME
constexpr uint64_t GBUS_REG_C2H_RD_PTR = 0x1240;
// Config-window mirror of the oldest unread frame, used only for the first
// frame cross-check.
constexpr uint64_t GBUS_REG_C2H_DATA = 0x2000;
constexpr uint32_t GBUS_C2H_STREAM_FRAME_BYTES = 768;
constexpr uint64_t GBUS_C2H_STREAM_APERTURE = 0x0ffc0000ULL;

constexpr uint32_t GBUS_C2H_STATUS_PRESENT = 1U << 31;
constexpr uint32_t GBUS_C2H_STATUS_FRAME_ERROR = 1U << 30;
constexpr uint32_t GBUS_C2H_STATUS_PROTOCOL_ERROR = 1U << 29;
constexpr uint32_t GBUS_C2H_STATUS_ENABLED = 1U << 7;
// A complete frame is published in the streaming ring.
constexpr uint32_t GBUS_C2H_STATUS_DATA_AVAILABLE = 1U << 5;

// Writing bit 0 arms capture; the readback reports the armed state in bit 1.
constexpr uint32_t GBUS_C2H_CTRL_ENABLE = 1U << 0;
constexpr uint32_t GBUS_C2H_CTRL_ARMED = 1U << 1;

uint64_t monotonic_us() {
  return static_cast<uint64_t>(
      std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now().time_since_epoch())
          .count());
}

uint64_t env_u64(const char *name, uint64_t fallback, uint64_t maximum = std::numeric_limits<uint64_t>::max()) {
  const char *v = std::getenv(name);
  if (!v)
    return fallback;
  char *end = nullptr;
  errno = 0;
  unsigned long long parsed = std::strtoull(v, &end, 0);
  if (errno || end == v || *end || *v < '0' || *v > '9' || parsed > maximum) {
    std::fprintf(stderr, "[fpga-host] invalid %s=%s\n", name, v);
    std::exit(EXIT_FAILURE);
  }
  return static_cast<uint64_t>(parsed);
}
uint32_t load_le32(const std::vector<uint8_t> &v) {
  if (v.size() < 4)
    return 0;
  return static_cast<uint32_t>(v[0]) | (static_cast<uint32_t>(v[1]) << 8) | (static_cast<uint32_t>(v[2]) << 16) |
         (static_cast<uint32_t>(v[3]) << 24);
}
std::vector<uint8_t> store_le32(uint32_t value) {
  return {static_cast<uint8_t>(value), static_cast<uint8_t>(value >> 8), static_cast<uint8_t>(value >> 16),
          static_cast<uint8_t>(value >> 24)};
}
void append_le32(std::vector<uint8_t> &out, uint32_t value) {
  out.push_back(static_cast<uint8_t>(value));
  out.push_back(static_cast<uint8_t>(value >> 8));
  out.push_back(static_cast<uint8_t>(value >> 16));
  out.push_back(static_cast<uint8_t>(value >> 24));
}

[[noreturn]] void fatal(const char *message) {
  std::fprintf(stderr, "[fpga-host] %s\n", message);
  std::exit(EXIT_FAILURE);
}
} // namespace

GbusTransport::GbusTransport() {
  prototyping_ = static_cast<uint8_t>(env_u64("GBUS_PROTOTYPING_INSTANCE", 0, 255));
  board_ = static_cast<uint8_t>(env_u64("GBUS_BOARD", 0, 255));
  // The runtime topology mapping is platform-specific.  On the checked UVHS
  // U2.2 setup, value 2 selects the design placed on B0.F2 (reported by the
  // daemon as topology fpgaId 3).  Keep it explicit and overrideable.
  fpga_ = static_cast<uint8_t>(env_u64("GBUS_FPGA", 2, 255));
  dma_fpga_ = static_cast<uint8_t>(env_u64("GBUS_DMA_FPGA", fpga_, 255));
  config_instance_ = static_cast<uint8_t>(env_u64("GBUS_CONFIG_INSTANCE", 0, 255));
  ddr_instance_ = static_cast<uint8_t>(env_u64("GBUS_DDR_INSTANCE", 0, 255));
  channel_ = static_cast<uint8_t>(env_u64("GBUS_CHANNEL", 0, 255));
  port_ = static_cast<uint8_t>(env_u64("GBUS_PORT", 0, 255));
  config_base_ = env_u64("GBUS_CONFIG_BASE", 0x1000ULL, 0xc000);
  // DMA uses the raw port-local address; CPU DRAM remapping is downstream.
  ddr_base_ = env_u64("GBUS_DDR_BASE", 0x0ULL, GBUS_C2H_STREAM_APERTURE);
  if (config_base_ < 0x1000 || config_base_ % 4 || ddr_base_ % 32)
    fatal("invalid GBus config/DDR base alignment or range");
  // The runtime accepts up to 0x7FFFF bytes per call, so a 1 MiB workload only
  // needs a handful of writes instead of one per KiB.  0x7FFFF rounded down to
  // the required 32-byte alignment is the largest legal chunk.
  constexpr uint32_t GBUS_H2C_MAX_CHUNK = 0x7FFE0;
  h2c_chunk_bytes_ = static_cast<uint32_t>(env_u64("GBUS_H2C_CHUNK_BYTES", 256, GBUS_H2C_MAX_CHUNK));
  if (!h2c_chunk_bytes_ || h2c_chunk_bytes_ % 32)
    fatal("H2C chunk must be 32..0x7FFE0 bytes and a multiple of 32");
  c2h_verify_first_bank_ = env_u64("GBUS_C2H_VERIFY_FIRST_BANK", 1, 1) != 0;
  h2c_verify_ = env_u64("GBUS_H2C_VERIFY", 1, 1) != 0;
  timing_ = env_u64("GBUS_TIMING", 0, 1) != 0;
  // Preserve the established 64-frame batching default. Set the variable to 0
  // explicitly to opt into the ring-sized threshold optimization.
  c2h_stream_min_frames_ = static_cast<uint32_t>(env_u64("GBUS_C2H_STREAM_MIN_FRAMES", 64, 4096));
  c2h_idle_timeout_sec_ = static_cast<uint32_t>(env_u64("GBUS_C2H_IDLE_TIMEOUT_SEC", 30, 3600));
  c2h_poll_us_ = static_cast<uint32_t>(env_u64("GBUS_C2H_POLL_US", 100, 1000000));
  const char *host = std::getenv("GBUS_HOST");
  host_ = host && *host ? host : "localhost";
  initialized_ = gbus_initialize(host_.c_str());
  if (!initialized_) {
    std::fprintf(stderr, "[fpga-host] GBus initialize failed (host=%s)\n", host_.c_str());
    std::exit(EXIT_FAILURE);
  }
  std::fprintf(stderr, "[fpga-host] GBus initialized host=%s board=%u fpga=%u config_base=0x%llx ddr_base=0x%llx\n",
               host_.c_str(), board_, fpga_, static_cast<unsigned long long>(config_base_),
               static_cast<unsigned long long>(ddr_base_));
}

GbusTransport::~GbusTransport() {
  if (initialized_) {
    if (!gbus_finalize()) {
      std::fprintf(stderr, "[fpga-host] GBus finalize failed; socket close unconfirmed\n");
      std::exit(EXIT_FAILURE);
    }
    std::fprintf(stderr, "[fpga-host] GBus finalized; socket close confirmed\n");
  }
}

void GbusTransport::start(bool enable_diff) {
  running_.store(true);
  if (!enable_diff) {
    while (running_.load() && signal_num == 0)
      usleep(10000);
    return;
  }
  c2h_probe_stream();
  c2h_drain_stream_frames();
}

// ---------------------------------------------------------------- register I/O

uint32_t GbusTransport::c2h_reg_read(uint64_t offset) {
  std::vector<uint8_t> data;
  const uint64_t begin = monotonic_us();
  const int rc = gbus_read(prototyping_, board_, fpga_, config_instance_, config_base_ + offset, 1, data);
  ++c2h_reg_calls_;
  c2h_reg_us_ += monotonic_us() - begin;
  if (rc != 1 || data.size() != 4) {
    dprintf(STDERR_FILENO,
            "[fpga-host] GBus C2H register read failed rc=%d offset=0x%llx size=4 actual=%zu begin=%llu end=%llu\n", rc,
            static_cast<unsigned long long>(config_base_ + offset), data.size(), static_cast<unsigned long long>(begin),
            static_cast<unsigned long long>(monotonic_us()));
    std::exit(EXIT_FAILURE);
  }
  return load_le32(data);
}

void GbusTransport::c2h_reg_write(uint64_t offset, uint32_t value) {
  auto data = store_le32(value);
  const uint64_t begin = monotonic_us();
  const int rc = gbus_write(prototyping_, board_, fpga_, config_instance_, config_base_ + offset, 1, data);
  ++c2h_write_calls_;
  c2h_write_us_ += monotonic_us() - begin;
  if (rc != 1) {
    dprintf(STDERR_FILENO,
            "[fpga-host] GBus C2H register write failed rc=%d offset=0x%llx size=4 value=0x%x begin=%llu end=%llu\n",
            rc, static_cast<unsigned long long>(config_base_ + offset), value, static_cast<unsigned long long>(begin),
            static_cast<unsigned long long>(monotonic_us()));
    std::exit(EXIT_FAILURE);
  }
}

void GbusTransport::c2h_read_block(uint64_t offset, uint32_t words, std::vector<uint32_t> &out) {
  std::vector<uint8_t> data;
  const uint64_t begin = monotonic_us();
  const int rc = gbus_read(prototyping_, board_, fpga_, config_instance_, config_base_ + offset, words, data);
  ++c2h_block_calls_;
  c2h_block_us_ += monotonic_us() - begin;
  if (rc != 1 || data.size() != static_cast<size_t>(words) * 4) {
    dprintf(STDERR_FILENO,
            "[fpga-host] GBus C2H block read failed rc=%d offset=0x%llx words=%u actual=%zu begin=%llu end=%llu\n", rc,
            static_cast<unsigned long long>(config_base_ + offset), words, data.size(),
            static_cast<unsigned long long>(begin), static_cast<unsigned long long>(monotonic_us()));
    std::exit(EXIT_FAILURE);
  }
  out.resize(words);
  for (uint32_t i = 0; i < words; ++i) {
    const uint8_t *p = data.data() + static_cast<size_t>(i) * 4;
    out[i] = static_cast<uint32_t>(p[0]) | (static_cast<uint32_t>(p[1]) << 8) | (static_cast<uint32_t>(p[2]) << 16) |
             (static_cast<uint32_t>(p[3]) << 24);
  }
}

// ------------------------------------------------------- streaming frame FIFO

// The GBD1 endpoint is the only supported C2H interface.  Verify its identity,
// that the runtime honours grouped config reads (the drain loop depends on
// them), and that the streaming frame FIFO is present.
void GbusTransport::c2h_probe_stream() {
  const uint32_t id = c2h_reg_read(GBUS_REG_C2H_ID);
  if (id != GBUS_C2H_DMA_ID_MAGIC) {
    char message[160];
    std::snprintf(message, sizeof(message),
                  "GBus C2H endpoint is not GBD1 (id=0x%08x); the register-window transport is no longer supported",
                  id);
    fatal(message);
  }

  std::vector<uint32_t> words;
  c2h_read_block(GBUS_REG_C2H_STATUS, 13, words);
  const bool grouped = words[2] == id && words[4] == GBUS_C2H_STREAM_APERTURE && words[5] != 0 &&
                       c2h_reg_read(GBUS_REG_C2H_ID) == words[2] &&
                       c2h_reg_read(GBUS_REG_C2H_DMA_BASE) == words[4] &&
                       c2h_reg_read(GBUS_REG_C2H_CAPACITY) == words[5];
  if (!grouped)
    fatal("GBus runtime does not return consecutive words for grouped config reads; GBD1 streaming requires them");
  std::fprintf(stderr, "[fpga-host] GBD1 block register reads verified (13 words per call)\n");

  const uint32_t feature = c2h_reg_read(GBUS_REG_C2H_FEATURE);
  if ((feature & (GBUS_C2H_FEATURE_PRESENT | GBUS_C2H_FEATURE_STREAM)) !=
      (GBUS_C2H_FEATURE_PRESENT | GBUS_C2H_FEATURE_STREAM))
    fatal("GBus bitstream does not expose the GBD1 streaming frame FIFO");

  const uint32_t ring = c2h_reg_read(GBUS_REG_C2H_CAPACITY);
  const uint64_t base = c2h_reg_read(GBUS_REG_C2H_DMA_BASE);
  if (base != GBUS_C2H_STREAM_APERTURE || ring == 0 || ring % GBUS_C2H_STREAM_FRAME_BYTES != 0)
    fatal("GBus GBD1 streaming aperture or ring size is invalid");
  c2h_stream_frames_ = ring / GBUS_C2H_STREAM_FRAME_BYTES;
  c2h_stream_frame_bytes_ = GBUS_C2H_STREAM_FRAME_BYTES;
  // A batch cannot exceed the ring, and draining at the ring depth is what
  // amortises the fixed per-call cost.  Leave a small margin so the producer is
  // not forced to stall exactly at the boundary.
  if (c2h_stream_min_frames_ == 0 || c2h_stream_min_frames_ > c2h_stream_frames_)
    c2h_stream_min_frames_ = c2h_stream_frames_ > 8 ? c2h_stream_frames_ - 8 : c2h_stream_frames_;
  if (c2h_stream_frames_ == 0)
    fatal("GBus GBD1 streaming ring is empty");
}

// The RTL appends every validated 768-byte frame to a ring and publishes only a
// frame counter, so the host never freezes, fills or acknowledges a bank.  One
// iteration reads the counter, copies as many frames as the ring exposes with a
// single DMA call, and releases exactly those frames with one CONSUME write.
// Because only CONSUME moves the read pointer, a long DMA call costs throughput
// but can never corrupt or drop frames.
void GbusTransport::c2h_drain_stream_frames() {
  using Clock = std::chrono::steady_clock;
  const auto alive = [&]() { return running_.load() && signal_num == 0; };
  const auto fail = [&](const char *reason, uint32_t status) {
    std::fprintf(stderr, "[fpga-host] GBD1 stream %s status=0x%08x (frames not released)\n", reason, status);
    running_.store(false);
    std::exit(EXIT_FAILURE);
  };
  const auto check_status = [&](uint32_t status) {
    if (!(status & GBUS_C2H_STATUS_PRESENT) ||
        (status & (GBUS_C2H_STATUS_FRAME_ERROR | GBUS_C2H_STATUS_PROTOCOL_ERROR)))
      fail("invalid status", status);
  };

  const uint64_t aperture = c2h_reg_read(GBUS_REG_C2H_DMA_BASE);
  const uint32_t ring_bytes = c2h_reg_read(GBUS_REG_C2H_CAPACITY);
  if (aperture != GBUS_C2H_STREAM_APERTURE || ring_bytes != c2h_stream_frames_ * c2h_stream_frame_bytes_)
    fail("invalid aperture/ring", 0);
  if (dma_fpga_ != 2 || fpga_ != 2 || port_ != 0 || channel_ != 0 || ddr_instance_ != 0)
    fail("invalid port-0 DMA topology", 0);

  // Bit 0 is ENABLE.  The engine drops ENABLE whenever the DiffTest stream is
  // still held in reset, and the host reaches this point before the CPU has
  // been released, so the first write usually lands in that window and is
  // deliberately ignored rather than latched for later replay.  Arm here, then
  // re-arm from the loop until the readback confirms it stuck.
  c2h_reg_write(GBUS_REG_C2H_CTRL, GBUS_C2H_CTRL_ENABLE);
  const uint32_t armed = c2h_reg_read(GBUS_REG_C2H_CTRL);
  check_status(c2h_reg_read(GBUS_REG_C2H_STATUS));
  const bool armed_now = (armed & GBUS_C2H_CTRL_ARMED) != 0;

  std::fprintf(stderr,
               "[fpga-host] GBD1 stream ready base=0x%llx frames=%u bytes=%u min_batch=%u poll_us=%u enabled=%u\n",
               static_cast<unsigned long long>(aperture), c2h_stream_frames_, ring_bytes, c2h_stream_min_frames_,
               c2h_poll_us_, armed_now ? 1U : 0U);

  std::vector<uint8_t> accumulator;
  accumulator.reserve(ring_bytes);
  uint64_t batches = 0, frames = 0, bytes = 0;
  const auto started = Clock::now();
  auto last_progress = Clock::now();
  auto last_change = Clock::now();
  uint32_t previous_available = 0;

  while (alive()) {
    const uint32_t status = c2h_reg_read(GBUS_REG_C2H_STATUS);
    check_status(status);

    // STATUS bit 5 already says whether any complete frame is published, so an
    // empty poll needs no counter read at all.
    if (!(status & GBUS_C2H_STATUS_DATA_AVAILABLE)) {
      // Arming is lost across the startup stream-reset window; re-assert it
      // until the readback shows capture really is on.
      if (!(status & GBUS_C2H_STATUS_ENABLED))
        c2h_reg_write(GBUS_REG_C2H_CTRL, GBUS_C2H_CTRL_ENABLE);
      if (c2h_idle_timeout_sec_ && Clock::now() - last_progress >= std::chrono::seconds(c2h_idle_timeout_sec_)) {
        const uint32_t last = c2h_reg_read(GBUS_REG_C2H_STATUS);
        fail(last & GBUS_C2H_STATUS_ENABLED ? "no capture progress" : "capture never armed", last);
      }
      usleep(c2h_poll_us_);
      continue;
    }

    // The drain loop needs the two ring counters in one call; the startup
    // probe has already established that this runtime honours a multi-word
    // count.  A grouped read costs a fixed overhead plus a per-word cost, so
    // the write pointer is left out: nothing reads it.
    std::vector<uint32_t> words;
    c2h_read_block(GBUS_REG_C2H_AVAILABLE, 2, words);
    if (words.size() != 2)
      fail("counter read failed", status);
    const uint32_t available = words[0];
    uint32_t rd_ptr = words[1];
    if (available > c2h_stream_frames_ || rd_ptr >= ring_bytes || rd_ptr % c2h_stream_frame_bytes_ != 0)
      fail("invalid counter snapshot", status);
    if (available != previous_available) {
      previous_available = available;
      last_change = Clock::now();
    }

    if (available == 0) {
      usleep(c2h_poll_us_);
      continue;
    }
    // Wait until the ring is worth draining, but never strand a tail: once the
    // producer stops publishing, ship whatever is left.  Draining at the ring
    // depth rather than at an arbitrary fraction amortises the fixed per-call
    // cost over the largest batch the hardware can hold.
    if (available < c2h_stream_min_frames_ && Clock::now() - last_change < std::chrono::milliseconds(20)) {
      usleep(c2h_poll_us_);
      continue;
    }

    // A batch never crosses the ring end; a wrapped batch takes two calls.
    const uint32_t frames_to_end = (ring_bytes - rd_ptr) / c2h_stream_frame_bytes_;
    uint32_t take = std::min(available, frames_to_end);
    const size_t count = static_cast<size_t>(take) * c2h_stream_frame_bytes_;

    std::vector<uint8_t> block;
    const uint64_t begin = monotonic_us();
    const int rc =
        gbus_dma_read(prototyping_, board_, dma_fpga_, ddr_instance_, aperture + rd_ptr, count, channel_, port_, block);
    const uint64_t elapsed_us = monotonic_us() - begin;
    ++c2h_dma_calls_;
    c2h_dma_us_ += elapsed_us;
    if (!alive())
      return;
    if (rc != 1 || block.size() != count || elapsed_us > 10000000ULL)
      fail("DMA capture failed", status);

    const uint64_t dispatch_begin = monotonic_us();
    // Dispatch straight out of the DMA buffer.  A batch starts at a frame
    // boundary, so it only continues an unfinished record when the previous
    // batch left a partial one; that tail is small enough to prepend.
    size_t consumed = 0;
    if (!accumulator.empty()) {
      accumulator.insert(accumulator.end(), block.begin(), block.end());
      while (accumulator.size() - consumed >= sizeof(DmaDiffPackge) && alive()) {
        c2h_dispatch_range(accumulator.data() + consumed, sizeof(DmaDiffPackge));
        consumed += sizeof(DmaDiffPackge);
      }
      accumulator.erase(accumulator.begin(), accumulator.begin() + static_cast<ptrdiff_t>(consumed));
    } else {
      while (block.size() - consumed >= sizeof(DmaDiffPackge) && alive()) {
        c2h_dispatch_range(block.data() + consumed, sizeof(DmaDiffPackge));
        consumed += sizeof(DmaDiffPackge);
      }
      // A frame is 768 B and a record is 96 B, so a batch normally ends exactly
      // on a record boundary; keep only a genuinely partial tail.
      accumulator.assign(block.begin() + static_cast<ptrdiff_t>(consumed), block.end());
    }
    c2h_dispatch_us_ += monotonic_us() - dispatch_begin;

    // First-batch cross-check: the config window mirrors the oldest unread
    // frame, so its words must match the DMA payload exactly.  This catches a
    // ring whose counters advance while the RAM path is wrong, which CoreMark
    // alone would only notice much later.
    if (batches == 0 && c2h_verify_first_bank_) {
      std::vector<uint32_t> words;
      c2h_read_block(GBUS_REG_C2H_DATA, c2h_stream_frame_bytes_ / 4, words);
      const bool same_size = words.size() * 4 == c2h_stream_frame_bytes_;
      bool same = same_size;
      for (size_t i = 0; same && i < words.size(); ++i) {
        const uint32_t word = words[i];
        same = block[i * 4] == static_cast<uint8_t>(word) && block[i * 4 + 1] == static_cast<uint8_t>(word >> 8) &&
               block[i * 4 + 2] == static_cast<uint8_t>(word >> 16) &&
               block[i * 4 + 3] == static_cast<uint8_t>(word >> 24);
      }
      if (!same)
        fail("first-frame DMA/config mirror mismatch", status);
      std::fprintf(stderr, "[fpga-host] GBD1 first frame DMA/config mirror verified bytes=%u\n",
                   c2h_stream_frame_bytes_);
    }

    // Release only what has already been copied out of the ring.  The read
    // pointer is checked instead of the availability counter: the producer
    // keeps writing during the DMA call, so a counter comparison would race,
    // while the read pointer must advance by exactly the released frame count.
    c2h_reg_write(GBUS_REG_C2H_AVAILABLE, take);
    const uint32_t status_after = c2h_reg_read(GBUS_REG_C2H_STATUS);
    check_status(status_after);
    const uint32_t rd_after = c2h_reg_read(GBUS_REG_C2H_RD_PTR);
    const uint64_t expect_rd = (static_cast<uint64_t>(rd_ptr) + count) % ring_bytes;
    if (rd_after != expect_rd) {
      // The engine drops capture (and clears both ring pointers) when the CPU
      // ends its DiffTest stream, so a CONSUME issued after the trap cannot be
      // acknowledged.  Everything already copied out has been dispatched; stop
      // cleanly instead of reporting a transport fault.
      if (!(status_after & GBUS_C2H_STATUS_ENABLED)) {
        std::fprintf(stderr, "[fpga-host] GBD1 stream ended by CPU after frames=%llu bytes=%llu\n",
                     static_cast<unsigned long long>(frames), static_cast<unsigned long long>(bytes));
        break;
      }
      fail("CONSUME was not applied", status_after);
    }

    frames += take;
    bytes += count;
    ++batches;
    last_progress = Clock::now();
    previous_available = 0; // re-read at the top of the loop
    last_change = Clock::now();

    const bool report = batches <= 2 || (batches & (batches - 1)) == 0;
    if (report) {
      const auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now() - started).count();
      std::fprintf(stderr,
                   "[fpga-host] GBUS_TIMING c2h frames=%llu bytes=%llu elapsed_ms=%lld avg_frame_us=%.1f "
                   "last_batch=%u dma_calls=%llu dma_us=%llu reg_calls=%llu reg_us=%llu "
                   "block_calls=%llu block_us=%llu write_calls=%llu write_us=%llu dispatch_us=%llu "
                   "poll_us=%u stream=1\n",
                   static_cast<unsigned long long>(frames), static_cast<unsigned long long>(bytes),
                   static_cast<long long>(elapsed_ms), frames ? static_cast<double>(elapsed_ms) * 1000.0 / frames : 0.0,
                   take, static_cast<unsigned long long>(c2h_dma_calls_), static_cast<unsigned long long>(c2h_dma_us_),
                   static_cast<unsigned long long>(c2h_reg_calls_), static_cast<unsigned long long>(c2h_reg_us_),
                   static_cast<unsigned long long>(c2h_block_calls_), static_cast<unsigned long long>(c2h_block_us_),
                   static_cast<unsigned long long>(c2h_write_calls_), static_cast<unsigned long long>(c2h_write_us_),
                   static_cast<unsigned long long>(c2h_dispatch_us_), c2h_poll_us_);
      std::fprintf(stderr, "[fpga-host] GBD1 progress mode=stream frames=%llu bytes=%llu\n",
                   static_cast<unsigned long long>(frames), static_cast<unsigned long long>(bytes));
    }
  }

  // Final summary: the periodic dump only fires on powers of two, so without
  // this the tail of a long run is invisible and the totals look smaller than
  // they are.
  const auto total_ms = std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now() - started).count();
  std::fprintf(stderr,
               "[fpga-host] GBUS_TIMING c2h totals frames=%llu bytes=%llu batches=%llu "
               "elapsed_ms=%lld avg_frame_us=%.1f dma_calls=%llu dma_us=%llu "
               "reg_calls=%llu reg_us=%llu block_calls=%llu block_us=%llu "
               "write_calls=%llu write_us=%llu dispatch_us=%llu stream=1\n",
               static_cast<unsigned long long>(frames), static_cast<unsigned long long>(bytes),
               static_cast<unsigned long long>(batches), static_cast<long long>(total_ms),
               frames ? static_cast<double>(total_ms) * 1000.0 / frames : 0.0,
               static_cast<unsigned long long>(c2h_dma_calls_), static_cast<unsigned long long>(c2h_dma_us_),
               static_cast<unsigned long long>(c2h_reg_calls_), static_cast<unsigned long long>(c2h_reg_us_),
               static_cast<unsigned long long>(c2h_block_calls_), static_cast<unsigned long long>(c2h_block_us_),
               static_cast<unsigned long long>(c2h_write_calls_), static_cast<unsigned long long>(c2h_write_us_),
               static_cast<unsigned long long>(c2h_dispatch_us_));
}

void GbusTransport::c2h_dispatch_range(const uint8_t *packet, size_t packet_size) {
  std::vector<uint8_t> range(packet, packet + packet_size);
  if (packet_size % sizeof(DmaDiffPackge) != 0) {
    std::fprintf(stderr, "[fpga-host] GBus C2H incomplete record: bytes=%zu\n", packet_size);
    std::exit(EXIT_FAILURE);
  }
  const uint64_t begin = monotonic_us();
  for (size_t offset = 0; offset < packet_size && running_.load() && signal_num == 0; offset += sizeof(DmaDiffPackge)) {
    v_difftest_Batch(range.data() + offset + offsetof(DmaDiffPackge, diff_packge));
  }
  c2h_dispatch_us_ += monotonic_us() - begin;
}

void GbusTransport::stop() {
  running_.store(false);
}

void GbusTransport::fpga_io(uint64_t address, uint32_t value) {
  if (!initialized_)
    std::exit(EXIT_FAILURE);
  auto data = store_le32(value);
  const int rc = gbus_write(prototyping_, board_, fpga_, config_instance_, config_base_ + address, 1, data);
  if (rc != 1) {
    dprintf(STDERR_FILENO, "[fpga-host] GBus register write failed offset=0x%llx\n",
            static_cast<unsigned long long>(address));
    std::exit(EXIT_FAILURE);
  }
  if (address <= HOST_IO_CPU_AXI_DELAY && address % 4 == 0) {
    // GeneralBD's write ACK is posted; the ordered read waits for the BAR's B response.
    const uint32_t observed = fpga_io_read(address);
    // MEM_INIT/MEM_H2C latch live completion status and CFG_RESET self-clears by
    // resetting the whole configuration BAR; these words are ordering fences,
    // not stable storage, so only the remaining words must read back exactly.
    if (address != HOST_IO_CFG_RESET && address != HOST_IO_MEM_INIT && address != HOST_IO_MEM_H2C &&
        observed != value) {
      std::fprintf(stderr, "[fpga-host] GBus BAR write readback mismatch offset=0x%llx wrote=0x%08x read=0x%08x\n",
                   static_cast<unsigned long long>(address), value, observed);
      std::exit(EXIT_FAILURE);
    }
  }
}

uint32_t GbusTransport::fpga_io_read(uint64_t address) {
  if (!initialized_)
    std::exit(EXIT_FAILURE);
  std::vector<uint8_t> data;
  if (gbus_read(prototyping_, board_, fpga_, config_instance_, config_base_ + address, 1, data) != 1 ||
      data.size() != 4) {
    std::fprintf(stderr, "[fpga-host] GBus register read failed offset=0x%llx\n",
                 static_cast<unsigned long long>(address));
    std::exit(EXIT_FAILURE);
  }
  return load_le32(data);
}

void GbusTransport::wait_fpga_io_done(uint64_t address, const char *tag) {
  // A GBus DMA completion is reported by the runtime call itself.  The XDMA
  // status registers are not guaranteed to be mirrored by the GENERALBD
  // window, so do not turn a completed GBus transfer into an infinite host-
  // side poll.  Set GBUS_POLL_STATUS=1 to require the register handshake
  // while debugging a board integration that implements it.
  const char *poll = std::getenv("GBUS_POLL_STATUS");
  if (!poll || std::strtoull(poll, nullptr, 0) == 0) {
    // GBus occupies DifftestMemCtrl's AXI-stream H2C engine.  The engine
    // reports completion by changing HOST_IO_MEM_H2C from 1 to 2 in the XDMA
    // flow, but GBus register reads of that status are not guaranteed.  After
    // the synchronous gbus_dma_write() has finished, clear the request so
    // HOST_IO_MEM_CPU can return DDR ownership to the CPU.  Leaving bit 0 set
    // permanently keeps the H2C master selected.
    if (address == HOST_IO_MEM_H2C) {
      fpga_io(HOST_IO_MEM_H2C, 0);
      std::fprintf(stderr, "[fpga-host] GBus direct DMA complete; released H2C DDR ownership to CPU\n");
    }
    std::fprintf(stderr, "[fpga-host] GBus %s completion accepted without legacy status poll\n", tag);
    return;
  }
  constexpr unsigned max_retry = 600000;
  for (unsigned i = 0; i < max_retry; ++i) {
    uint32_t status = fpga_io_read(address) & 3U;
    if (status == 2U)
      return;
    if (status == 3U) {
      std::fprintf(stderr, "[fpga-host] GBus %s failed: address range exceeds AXI width\n", tag);
      return;
    }
    usleep(1000);
  }
  std::fprintf(stderr, "[fpga-host] GBus timeout waiting for %s\n", tag);
}

void GbusTransport::h2c_load_workload(const void *payload, uint64_t size) {
  if (!initialized_ || !payload || !size) {
    std::fprintf(stderr, "[fpga-host] GBus DMA workload requires initialized transport and non-empty payload\n");
    std::exit(EXIT_FAILURE);
  }
  if (size % 32 || size > GBUS_C2H_STREAM_APERTURE - ddr_base_ || size > std::numeric_limits<size_t>::max() ||
      port_ != 0) {
    std::fprintf(stderr, "[fpga-host] GBus padded H2C range overlaps reserved page, is unaligned or exceeds port 0\n");
    std::exit(EXIT_FAILURE);
  }
  const auto *bytes = static_cast<const uint8_t *>(payload);
  // One write per H2C chunk; the runtime splits transfers above the 512-byte
  // AXI3 burst limit into consecutive bursts.
  const uint64_t chunk = h2c_chunk_bytes_;
  const auto h2c_begin = std::chrono::steady_clock::now();
  uint64_t h2c_requests = 0;
  for (uint64_t offset = 0; offset < size; offset += chunk) {
    size_t count = static_cast<size_t>((size - offset) < chunk ? (size - offset) : chunk);
    std::vector<uint8_t> data(bytes + offset, bytes + offset + count);
    if (gbus_dma_write(prototyping_, board_, dma_fpga_, ddr_instance_, ddr_base_ + offset, count, channel_, port_,
                       data) != 1) {
      std::fprintf(stderr, "[fpga-host] GBus DMA workload write failed offset=0x%llx size=%zu\n",
                   static_cast<unsigned long long>(offset), count);
      std::exit(EXIT_FAILURE);
    }
    ++h2c_requests;
  }
  if (timing_) {
    const auto h2c_us =
        std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - h2c_begin).count();
    std::fprintf(stderr,
                 "[fpga-host] GBUS_TIMING h2c bytes=%llu chunk=%llu requests=%llu elapsed_us=%lld avg_us=%.1f\n",
                 static_cast<unsigned long long>(size), static_cast<unsigned long long>(chunk),
                 static_cast<unsigned long long>(h2c_requests), static_cast<long long>(h2c_us),
                 h2c_requests ? static_cast<double>(h2c_us) / h2c_requests : 0.0);
  }
  // Optional DDR readback.  The vendor write path reports success for chunks
  // that did not land, so a stale image looks exactly like a CPU bug: the
  // reference runs the intended file while the DUT executes whatever the DDR
  // holds.  Verifying costs one extra DMA read per chunk and changes the call
  // sequence, so it is opt-in rather than always on.
  const uint64_t verify_chunk = 1ULL << 18; // 256 KiB, well under the 0x7FFFF API cap
  uint64_t mismatched = 0, patched = 0;
  for (uint64_t offset = 0; h2c_verify_ && offset < size; offset += verify_chunk) {
    const size_t count = static_cast<size_t>((size - offset) < verify_chunk ? (size - offset) : verify_chunk);
    for (unsigned attempt = 0; attempt < 3; ++attempt) {
      std::vector<uint8_t> back;
      if (gbus_dma_read(prototyping_, board_, dma_fpga_, ddr_instance_, ddr_base_ + offset, count, channel_, port_,
                        back) == 1 &&
          back.size() == count && std::equal(back.begin(), back.end(), bytes + offset))
        break;
      ++mismatched;
      if (attempt == 2) {
        std::fprintf(stderr, "[fpga-host] GBus H2C readback mismatch offset=0x%llx size=%zu\n",
                     static_cast<unsigned long long>(offset), count);
        running_.store(false);
        std::exit(EXIT_FAILURE);
      }
      std::vector<uint8_t> data(bytes + offset, bytes + offset + count);
      if (gbus_dma_write(prototyping_, board_, dma_fpga_, ddr_instance_, ddr_base_ + offset, count, channel_, port_,
                         data) != 1) {
        std::fprintf(stderr, "[fpga-host] GBus H2C rewrite failed offset=0x%llx\n",
                     static_cast<unsigned long long>(offset));
        std::exit(EXIT_FAILURE);
      }
      ++patched;
    }
  }
  if (h2c_verify_)
    std::fprintf(stderr, "[fpga-host] GBus H2C readback verified bytes=%llu chunks_failed=%llu patched=%llu\n",
                 static_cast<unsigned long long>(size), static_cast<unsigned long long>(mismatched),
                 static_cast<unsigned long long>(patched));

  std::fprintf(stderr, "[fpga-host] GBus DMA workload queued %llu bytes\n", static_cast<unsigned long long>(size));
}

void GbusTransport::validate_guest_ram(uint64_t base, uint64_t size) const {
  // The streaming ring lives in the reserved top of port 0, outside guest RAM,
  // so the CPU may use the whole DDR.
  (void)base;
  (void)size;
}
