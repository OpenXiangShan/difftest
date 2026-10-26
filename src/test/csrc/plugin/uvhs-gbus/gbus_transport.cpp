#include "gbus_transport.h"
#include "difftest-dpic.h"
#include "xdma.h"
#include <algorithm>
#include <cerrno>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
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
// SRAM staging window register block, in GeneralBD local offsets.  The RTL
// decodes these inside the same 0x1000 config window the DiffTest BAR uses.
constexpr uint64_t GBUS_REG_C2H_STATUS = 0x1200;
constexpr uint64_t GBUS_REG_C2H_CTRL = 0x1204;
constexpr uint64_t GBUS_REG_C2H_ID = 0x1208;
constexpr uint32_t GBUS_C2H_ID_MAGIC = 0x47425331u; // ASCII "GBS1"
constexpr uint32_t GBUS_C2H_DMA_ID_MAGIC = 0x47424431u;
constexpr uint64_t GBUS_REG_C2H_SEQ = 0x120c;
constexpr uint64_t GBUS_REG_C2H_DMA_BASE = 0x1210;
constexpr uint64_t GBUS_REG_C2H_CAPACITY = 0x1214;
constexpr uint64_t GBUS_REG_C2H_AR_ADDR = 0x1218;
constexpr uint64_t GBUS_REG_C2H_AR_ATTRS = 0x121c;
constexpr uint64_t GBUS_REG_C2H_AR_COUNT = 0x1220;
// First-control evidence.  Every write to the control register increments the
// count and updates the data word; an illegal control additionally latches the
// sticky bad-control word whose bit 31 is the reject flag.
constexpr uint64_t GBUS_REG_C2H_CTRL_COUNT = 0x1224;
constexpr uint64_t GBUS_REG_C2H_CTRL_DATA = 0x1228;
constexpr uint64_t GBUS_REG_C2H_BAD_CTRL = 0x1230;
constexpr uint64_t GBUS_REG_C2H_FEATURE = 0x1238;
constexpr uint32_t GBUS_C2H_FEATURE_PRESENT = 1U << 0;
constexpr uint32_t GBUS_C2H_FEATURE_HEADER = 1U << 1;
constexpr uint32_t GBUS_C2H_FEATURE_PINGPONG = 1U << 2;
// Streaming frame FIFO: the same feature bit names the capability, and the
// counters below replace the frozen-bank metadata.
constexpr uint32_t GBUS_C2H_FEATURE_STREAM = 1U << 2;
constexpr uint64_t GBUS_REG_C2H_AVAILABLE = 0x123c;  // read: frames; write: CONSUME
constexpr uint64_t GBUS_REG_C2H_RD_PTR = 0x1240;
constexpr uint64_t GBUS_REG_C2H_WR_PTR = 0x1244;
constexpr uint32_t GBUS_C2H_STREAM_FRAME_BYTES = 768;
// The aperture snapshot header repeats the capability in its own flags word,
// using the RTL's present/header bit pair (bits 31 and 30).
constexpr uint32_t GBUS_C2H_HDR_FLAG_PRESENT = 1U << 31;
constexpr uint32_t GBUS_C2H_HDR_FLAG_HEADER = 1U << 30;
constexpr uint32_t GBUS_C2H_HEADER_BYTES = 64;
constexpr uint32_t GBUS_C2H_BAD_CTRL_VALID = 1U << 31;
constexpr uint64_t GBUS_C2H_APERTURE = 0x0ffc0000ULL;
constexpr uint64_t GBUS_REG_C2H_DATA = 0x2000;
constexpr uint32_t GBUS_C2H_STAGE_WORDS = 256;
// Former frozen bank size.  Retained for the legacy register ABI; the
// streaming ring reports its own size through 0x1214.
constexpr uint32_t GBUS_C2H_DATA_BYTES = GBUS_C2H_STAGE_WORDS * 4;

constexpr uint32_t GBUS_C2H_STATUS_PRESENT = 1U << 31;
constexpr uint32_t GBUS_C2H_STATUS_FRAME_ERROR = 1U << 30;
constexpr uint32_t GBUS_C2H_STATUS_PROTOCOL_ERROR = 1U << 29;
constexpr uint32_t GBUS_C2H_STATUS_FROZEN = 1U << 5;
constexpr uint32_t GBUS_C2H_STATUS_ACTIVE = 1U << 4;
constexpr uint32_t GBUS_C2H_STATUS_STAGED_SHIFT = 8;
constexpr uint32_t GBUS_C2H_STATUS_STAGED_MASK = 0x1ffU;
constexpr uint32_t GBUS_C2H_STATUS_FILLING = 1U << 7;
// Streaming engine: bit 7 reports capture-enable, bit 5 data-available.
constexpr uint32_t GBUS_C2H_STATUS_ENABLED = 1U << 7;
constexpr uint32_t GBUS_C2H_STATUS_HAS_DATA = 1U << 6;

constexpr uint32_t GBUS_C2H_CTRL_START_FILL = 1U << 0;
constexpr uint32_t GBUS_C2H_CTRL_ACK = 1U << 1;
// The control readback reports the capture-enable state in bit 1, not the
// command bit: writing 1 arms capture, reading 2 proves it is armed.
constexpr uint32_t GBUS_C2H_CTRL_ENABLED = 1U << 1;

// The config BAR occupies 0x1000..0x1030.  Its words are individually readable
// through the already-verified single-word path and are known to differ from
// one another, which makes them usable as ground truth for the multi-word read
// probe.
constexpr size_t GBUS_CONFIG_BAR_WORDS = 13;

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
  ddr_base_ = env_u64("GBUS_DDR_BASE", 0x0ULL, GBUS_C2H_APERTURE);
  if (config_base_ < 0x1000 || config_base_ % 4 || ddr_base_ % 32) {
    std::fprintf(stderr, "[fpga-host] invalid GBus config/DDR base alignment or range\n");
    std::exit(EXIT_FAILURE);
  }
  c2h_ring_base_ = env_u64("GBUS_C2H_RING_BASE", 0x81000000ULL);
  c2h_dma_base_ =
      env_u64("GBUS_C2H_DMA_BASE", c2h_ring_base_ >= 0x80000000ULL ? c2h_ring_base_ - 0x80000000ULL : c2h_ring_base_);
  // The usable ring is one 256-byte tail shorter than 16 MiB so it is an
  // exact multiple of a 768-byte FpgaPackgeHead.  No DMA read can straddle the
  // physical ring boundary.
  c2h_ring_size_ = env_u64("GBUS_C2H_RING_SIZE", 0x00ffff00ULL);
  // Legacy DDR-ring parameters.  Only used when GBUS_C2H_SRAM=0; the default
  // SRAM staging window lives in the same 0x1000 config window as the DiffTest
  // BAR and reserves no guest address space.
  c2h_wptr_offset_ = env_u64("GBUS_C2H_WPTR_OFFSET", 0x0108ULL);
  c2h_sram_ = env_u64("GBUS_C2H_SRAM", 1, 1) != 0;
  const char *mode = std::getenv("GBUS_C2H_MODE");
  if (mode && std::strcmp(mode, "config-window") != 0 && std::strcmp(mode, "axi-dma") != 0) {
    std::fprintf(stderr, "[fpga-host] GBUS_C2H_MODE must be config-window or axi-dma\n");
    std::exit(EXIT_FAILURE);
  }
  c2h_axi_dma_ = mode && std::strcmp(mode, "axi-dma") == 0;
  if (c2h_axi_dma_ && !c2h_sram_) {
    std::fprintf(stderr, "[fpga-host] axi-dma mode requires the GBD1 SRAM endpoint\n");
    std::exit(EXIT_FAILURE);
  }
  c2h_dma_chunk_bytes_ = static_cast<uint32_t>(env_u64("GBUS_C2H_DMA_CHUNK_BYTES", 256, 1024));
  if (!c2h_dma_chunk_bytes_ || c2h_dma_chunk_bytes_ % 32) {
    std::fprintf(stderr, "[fpga-host] DMA chunk must be 32..1024 bytes, multiple of 32\n");
    std::exit(EXIT_FAILURE);
  }
  h2c_chunk_bytes_ = static_cast<uint32_t>(env_u64("GBUS_H2C_CHUNK_BYTES", 256, 1024));
  if (!h2c_chunk_bytes_ || h2c_chunk_bytes_ % 32) {
    std::fprintf(stderr, "[fpga-host] H2C chunk must be 32..1024 bytes, multiple of 32\n");
    std::exit(EXIT_FAILURE);
  }
  c2h_verify_first_bank_ = env_u64("GBUS_C2H_VERIFY_FIRST_BANK", 1, 1) != 0;
  timing_ = env_u64("GBUS_TIMING", 0, 1) != 0;
  // GBUS_C2H_BLOCK_READS=1 groups the per-bank register reads into single
  // runtime calls, but only after the probe in c2h_drain_sram_fifo() proves
  // this runtime returns consecutive words for count > 1.
  c2h_block_reads_ = env_u64("GBUS_C2H_BLOCK_READS", 0, 1) != 0;
  c2h_fast_fill_ = env_u64("GBUS_C2H_FAST_FILL", 0, 1) != 0;
  c2h_aperture_header_ = env_u64("GBUS_C2H_APERTURE_HEADER", 0, 1) != 0;
  c2h_stream_ = env_u64("GBUS_C2H_STREAM", 1, 1) != 0;
  c2h_stream_min_frames_ =
      static_cast<uint32_t>(env_u64("GBUS_C2H_STREAM_MIN_FRAMES", 64, 4096));
  c2h_pingpong_ = env_u64("GBUS_C2H_PINGPONG", 0, 1) != 0;
  c2h_idle_timeout_sec_ = static_cast<uint32_t>(env_u64("GBUS_C2H_IDLE_TIMEOUT_SEC", 30, 3600));
  // Multi-word capability probes are opt-in on unverified runtime releases.
  c2h_burst_words_ = static_cast<uint32_t>(env_u64("GBUS_C2H_BURST_WORDS", 1, GBUS_C2H_STAGE_WORDS));
  c2h_poll_us_ = static_cast<uint32_t>(env_u64("GBUS_C2H_POLL_US", 100, 1000000));
  const char *host = std::getenv("GBUS_HOST");
  host_ = host && *host ? host : "localhost";
  initialized_ = gbus_initialize(host_.c_str());
  if (!initialized_) {
    std::fprintf(stderr, "[fpga-host] GBus initialize failed (host=%s)\n", host_.c_str());
    std::exit(EXIT_FAILURE);
  } else {
    std::fprintf(stderr,
                 "[fpga-host] GBus initialized host=%s board=%u fpga=%u config_base=0x%llx ddr_base=0x%llx "
                 "c2h_dma_base=0x%llx\n",
                 host_.c_str(), board_, fpga_, static_cast<unsigned long long>(config_base_),
                 static_cast<unsigned long long>(ddr_base_), static_cast<unsigned long long>(c2h_dma_base_));
    std::fprintf(stderr, "[fpga-host] GBus C2H interface layer=%s mode=%s\n", c2h_sram_ ? "sram-window" : "ddr-ring",
                 c2h_axi_dma_ ? "axi-dma" : "config-window");
  }
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
  if (c2h_sram_) {
    c2h_drain_sram_fifo();
    return;
  }
  c2h_drain_ddr_ring();
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

// Grouped reads exist only to remove runtime round-trips.  Every caller must
// still validate the exact word count and the semantics of the fields it uses;
// the probe below proves this runtime returns consecutive words.
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

bool GbusTransport::c2h_probe_gbd1_block_reads() {
  // The block spans status/control/ID/sequence/aperture/capacity/AR evidence.
  // ID, aperture and capacity are distinct constants, so a runtime that
  // ignores the address or the count cannot reproduce all three.
  // ID, aperture and capacity must all reproduce through both a grouped and a
  // single-word read, so a runtime that ignores the count or the address cannot
  // pass.  Capacity is compared against this bitstream's own value instead of a
  // compile-time constant: the streaming ring is much larger than the legacy
  // 1 KiB bank, and a stale constant would reject every valid runtime.
  std::vector<uint32_t> words;
  c2h_read_block(GBUS_REG_C2H_STATUS, 13, words);
  if (words[2] != GBUS_C2H_DMA_ID_MAGIC || words[4] != GBUS_C2H_APERTURE || words[5] == 0)
    return false;
  return c2h_reg_read(GBUS_REG_C2H_ID) == words[2] && c2h_reg_read(GBUS_REG_C2H_DMA_BASE) == words[4] &&
         c2h_reg_read(GBUS_REG_C2H_CAPACITY) == words[5];
}

void GbusTransport::c2h_read_stable_regs(uint32_t &status, uint32_t &seq) {
  if (c2h_block_reads_) {
    std::vector<uint32_t> words;
    c2h_read_block(GBUS_REG_C2H_STATUS, 4, words);
    status = words[0];
    seq = words[3];
    return;
  }
  status = c2h_reg_read(GBUS_REG_C2H_STATUS);
  seq = c2h_reg_read(GBUS_REG_C2H_SEQ);
}

void GbusTransport::c2h_read_fill_evidence(uint32_t &ctrl_count, uint32_t &ctrl_data, uint32_t &bad_ctrl) {
  if (c2h_block_reads_) {
    std::vector<uint32_t> words;
    c2h_read_block(GBUS_REG_C2H_CTRL_COUNT, 4, words);
    ctrl_count = words[0];
    ctrl_data = words[1];
    bad_ctrl = words[3];
    return;
  }
  ctrl_count = c2h_reg_read(GBUS_REG_C2H_CTRL_COUNT);
  ctrl_data = c2h_reg_read(GBUS_REG_C2H_CTRL_DATA);
  bad_ctrl = c2h_reg_read(GBUS_REG_C2H_BAD_CTRL);
}

// The aperture header is an RTL feature: register 0x1238 reports
// present|header once the hardware latches it.  Older bitstreams return 0, so
// the host keeps the verified register-polling path instead of failing.
bool GbusTransport::c2h_probe_aperture_header() {
  // Enabling is only legal while no bank is frozen, which holds at startup.
  uint32_t want = GBUS_C2H_FEATURE_HEADER;
  if (c2h_pingpong_)
    want |= GBUS_C2H_FEATURE_PINGPONG;
  c2h_reg_write(GBUS_REG_C2H_FEATURE, want);
  const uint32_t feature = c2h_reg_read(GBUS_REG_C2H_FEATURE);
  const uint32_t need = GBUS_C2H_FEATURE_PRESENT | want;
  if ((feature & need) != need) {
    if (!(feature & GBUS_C2H_FEATURE_HEADER)) {
      // Retry header-only so a runtime without ping-pong keeps the read path.
      if (c2h_pingpong_) {
        c2h_reg_write(GBUS_REG_C2H_FEATURE, GBUS_C2H_FEATURE_HEADER);
        const uint32_t again = c2h_reg_read(GBUS_REG_C2H_FEATURE);
        if ((again & (GBUS_C2H_FEATURE_PRESENT | GBUS_C2H_FEATURE_HEADER)) !=
            (GBUS_C2H_FEATURE_PRESENT | GBUS_C2H_FEATURE_HEADER))
          return false;
        c2h_pingpong_ready_ = false;
        c2h_header_bytes_ = GBUS_C2H_HEADER_BYTES;
        return true;
      }
      return false;
    }
    c2h_pingpong_ready_ = false;
  } else {
    c2h_pingpong_ready_ = (feature & GBUS_C2H_FEATURE_PINGPONG) != 0;
  }
  c2h_header_bytes_ = GBUS_C2H_HEADER_BYTES;
  return true;
}

// A multi-word read is the only way the staging window can be drained at a
// usable rate: the register window is 32 bits wide, so a 768-byte DiffTest
// range is 192 separate addresses.  The runtime documents `count` in units of
// four bytes but the platform behaviour for count > 1 was never verified, and
// it is not something the RTL can compensate for.  Probe it against the config
// BAR, whose words are individually readable through the already-verified
// single-word path and are known to differ from one another.  A runtime that
// ignored the address, ignored the length, or returned one word repeated would
// all be caught here.
bool GbusTransport::c2h_probe_multiword_read(uint32_t words, uint64_t *elapsed_us) {
  if (words < 2)
    return false;
  const uint64_t begin = monotonic_us();
  std::vector<uint8_t> block;
  const int rc = gbus_read(prototyping_, board_, fpga_, config_instance_, config_base_, words, block);
  if (elapsed_us)
    *elapsed_us = monotonic_us() - begin;
  if (rc != 1 || block.size() != words * 4) {
    std::fprintf(stderr, "[fpga-host] GBus multi-word probe failed: count=%u rc=%d bytes=%zu (expected %u)\n", words,
                 rc, block.size(), words * 4);
    std::exit(EXIT_FAILURE);
  }
  for (size_t i = 0; i < words; ++i) {
    std::vector<uint8_t> one;
    if (gbus_read(prototyping_, board_, fpga_, config_instance_, config_base_ + i * 4, 1, one) != 1 ||
        one.size() != 4) {
      std::fprintf(stderr, "[fpga-host] GBus multi-word probe: single read of word %zu failed\n", i);
      std::exit(EXIT_FAILURE);
    }
    if (std::memcmp(one.data(), block.data() + i * 4, 4) != 0) {
      std::fprintf(stderr,
                   "[fpga-host] GBus multi-word probe: word %zu block=%02x%02x%02x%02x single=%02x%02x%02x%02x -> "
                   "count>1 does not return consecutive words\n",
                   i, block[i * 4 + 3], block[i * 4 + 2], block[i * 4 + 1], block[i * 4], one[3], one[2], one[1],
                   one[0]);
      return false;
    }
  }
  return true;
}

// ------------------------------------------------------------- SRAM window

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

void GbusTransport::c2h_drain_sram_fifo() {
  const uint32_t id = c2h_reg_read(GBUS_REG_C2H_ID);
  if (id == GBUS_C2H_DMA_ID_MAGIC) {
    if (c2h_block_reads_) {
      if (c2h_probe_gbd1_block_reads()) {
        std::fprintf(stderr, "[fpga-host] GBD1 block register reads verified (13 words per call)\n");
      } else {
        c2h_block_reads_ = false;
        std::fprintf(stderr, "[fpga-host] GBD1 block register reads REJECTED; using single-word reads\n");
      }
    }
    if (c2h_aperture_header_) {
      c2h_aperture_header_ready_ = c2h_probe_aperture_header();
      std::fprintf(stderr, "[fpga-host] GBD1 aperture header %s\n",
                   c2h_aperture_header_ready_ ? "verified (one DMA read per bank)"
                                              : "not implemented; using registers");
    }
    if (c2h_stream_) {
      c2h_stream_ready_ = c2h_probe_stream();
      if (c2h_stream_ready_) {
        std::fprintf(stderr, "[fpga-host] GBD1 streaming frame FIFO verified (%u frames, one DMA per batch)\n",
                     c2h_stream_frames_);
        c2h_drain_stream_frames();
        return;
      }
      std::fprintf(stderr, "[fpga-host] GBD1 streaming FIFO not present; using the frozen-bank protocol\n");
    }
    c2h_drain_dma_window();
    return;
  }
  if (c2h_aperture_header_) {
    std::fprintf(stderr, "[fpga-host] GBus C2H aperture header requires the GBD1 endpoint\n");
    std::exit(EXIT_FAILURE);
  }
  if (c2h_axi_dma_) {
    std::fprintf(stderr, "[fpga-host] axi-dma mode requires GBD1, got ID=0x%08x\n", id);
    std::exit(EXIT_FAILURE);
  }
  if (id != GBUS_C2H_ID_MAGIC) {
    std::fprintf(stderr, "[fpga-host] GBus C2H unknown ID offset=0x%llx id=0x%08x (expected GBS1)\n",
                 static_cast<unsigned long long>(config_base_ + GBUS_REG_C2H_ID), id);
    running_.store(false);
    std::exit(EXIT_FAILURE);
  }
  const size_t packet_size = sizeof(DmaDiffPackge);
  // A Delta completion can arrive before the enclosing eight-record range is
  // full. Dispatch complete records immediately and retain partial records
  // across register windows; waiting for a full range can strand DeltaInfo.
  std::vector<uint8_t> accumulator;
  accumulator.reserve(2 * packet_size);
  std::vector<uint8_t> block;

  uint32_t burst_words = c2h_burst_words_;
  if (burst_words == 0) {
    uint64_t elapsed_us = 0;
    burst_words = c2h_probe_multiword_read(GBUS_CONFIG_BAR_WORDS, &elapsed_us) ? GBUS_C2H_STAGE_WORDS : 1;
    std::fprintf(stderr,
                 "[fpga-host] GBus multi-word register read probe: count=%zu %s (%llu us); using %u words per "
                 "gbus_read call\n",
                 GBUS_CONFIG_BAR_WORDS, burst_words == 1 ? "REJECTED" : "accepted",
                 static_cast<unsigned long long>(elapsed_us), burst_words);
  }
  if (burst_words > GBUS_C2H_STAGE_WORDS)
    burst_words = GBUS_C2H_STAGE_WORDS;

  const uint32_t status = c2h_reg_read(GBUS_REG_C2H_STATUS);
  if ((status & GBUS_C2H_STATUS_PRESENT) == 0) {
    std::fprintf(stderr,
                 "[fpga-host] GBus C2H staging window is absent status=0x%08x; this bitstream has the DDR ring "
                 "interface, set GBUS_C2H_SRAM=0\n",
                 status);
    std::exit(EXIT_FAILURE);
  }
  std::fprintf(stderr,
               "[fpga-host] GBus C2H staging window ready id=0x%08x status=0x%08x stage_bytes=%u burst_words=%u "
               "poll_us=%u\n",
               id, status, GBUS_C2H_DATA_BYTES, burst_words, c2h_poll_us_);

  while (running_.load() && signal_num == 0) {
    uint32_t st = c2h_reg_read(GBUS_REG_C2H_STATUS);
    if (st & GBUS_C2H_STATUS_FRAME_ERROR) {
      std::fprintf(stderr,
                   "[fpga-host] GBus C2H framing error: the sender's tlast did not land on the 24th beat of a "
                   "768-byte range status=0x%08x\n",
                   st);
      running_.store(false);
      std::exit(EXIT_FAILURE);
    }
    if (st & GBUS_C2H_STATUS_FILLING) {
      usleep(c2h_poll_us_);
      continue;
    }
    // Only start a fill when the sender has actually buffered something.
    // Starting one on an empty FIFO would end on the dry check with zero words
    // and turn the idle wait into a register-access spin.
    if ((st & GBUS_C2H_STATUS_HAS_DATA) == 0) {
      usleep(c2h_poll_us_);
      continue;
    }

    // Payload is buffered, so this fill completes as soon as the FIFO drains
    // into the staging window, or immediately if less than a window is left.
    c2h_reg_write(GBUS_REG_C2H_CTRL, GBUS_C2H_CTRL_START_FILL);
    const auto fill_deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(200);
    do {
      if (!running_.load() || signal_num != 0)
        return;
      st = c2h_reg_read(GBUS_REG_C2H_STATUS);
      if ((st & GBUS_C2H_STATUS_FILLING) == 0)
        break;
      usleep(20);
    } while (std::chrono::steady_clock::now() < fill_deadline && running_.load() && signal_num == 0);
    if (!running_.load() || signal_num != 0)
      return;
    if (st & GBUS_C2H_STATUS_FILLING) {
      std::fprintf(stderr, "[fpga-host] GBus C2H staging fill did not complete status=0x%08x\n", st);
      running_.store(false);
      std::exit(EXIT_FAILURE);
    }

    const uint32_t words = (st >> GBUS_C2H_STATUS_STAGED_SHIFT) & GBUS_C2H_STATUS_STAGED_MASK;
    if (words == 0) {
      // The sender has data but the fill staged nothing. Retry after a short
      // wait; a control-write race can produce a few empty fills.
      usleep(c2h_poll_us_);
      continue;
    }
    if (words > GBUS_C2H_STAGE_WORDS) {
      std::fprintf(stderr, "[fpga-host] GBus C2H staged word count %u exceeds the window\n", words);
      running_.store(false);
      std::exit(EXIT_FAILURE);
    }

    uint32_t remaining = words;
    uint64_t offset = GBUS_REG_C2H_DATA;
    while (remaining) {
      const uint32_t chunk = std::min(remaining, burst_words);
      bool burst_ok = false;
      if (chunk > 1) {
        block.clear();
        const int rc = gbus_read(prototyping_, board_, fpga_, config_instance_, config_base_ + offset, chunk, block);
        if (rc != 1 || block.size() != static_cast<size_t>(chunk) * 4) {
          std::fprintf(stderr, "[fpga-host] GBus burst read failed rc=%d requested=%u actual=%zu\n", rc, chunk * 4,
                       block.size());
          std::exit(EXIT_FAILURE);
        }
        burst_ok = true;
      }
      if (burst_ok) {
        accumulator.insert(accumulator.end(), block.begin(), block.end());
      } else {
        for (uint32_t i = 0; i < chunk; ++i)
          append_le32(accumulator, c2h_reg_read(offset + i * 4));
      }
      offset += static_cast<uint64_t>(chunk) * 4;
      remaining -= chunk;
    }

    while (accumulator.size() >= packet_size) {
      c2h_dispatch_range(accumulator.data(), packet_size);
      accumulator.erase(accumulator.begin(), accumulator.begin() + static_cast<ptrdiff_t>(packet_size));
    }
  }
}

// ------------------------------------------------------- streaming frame FIFO

bool GbusTransport::c2h_probe_stream() {
  // Feature bit 2 marks a streaming frame FIFO.  The ring size comes from the
  // capacity register and must be a whole number of 768-byte frames.
  const uint32_t feature = c2h_reg_read(GBUS_REG_C2H_FEATURE);
  if (!(feature & GBUS_C2H_FEATURE_STREAM))
    return false;
  const uint32_t ring = c2h_reg_read(GBUS_REG_C2H_CAPACITY);
  const uint64_t base = c2h_reg_read(GBUS_REG_C2H_DMA_BASE);
  if (base != GBUS_C2H_APERTURE || ring == 0 || ring % GBUS_C2H_STREAM_FRAME_BYTES != 0)
    return false;
  c2h_stream_frames_ = ring / GBUS_C2H_STREAM_FRAME_BYTES;
  c2h_stream_frame_bytes_ = GBUS_C2H_STREAM_FRAME_BYTES;
  return true;
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
  if (aperture != GBUS_C2H_APERTURE || ring_bytes != c2h_stream_frames_ * c2h_stream_frame_bytes_)
    fail("invalid aperture/ring", 0);
  if (dma_fpga_ != 2 || fpga_ != 2 || port_ != 0 || channel_ != 0 || ddr_instance_ != 0)
    fail("invalid port-0 DMA topology", 0);

  // Bit 0 is ENABLE on the streaming engine (the same value the frozen-bank
  // protocol used as FILL).  The engine drops ENABLE whenever the DiffTest
  // stream is still held in reset, and the host reaches this point before the
  // CPU has been released, so the first write usually lands in that window and
  // is deliberately ignored rather than latched for later replay.  Arm here,
  // then re-arm from the loop until the readback confirms it stuck.
  c2h_reg_write(GBUS_REG_C2H_CTRL, GBUS_C2H_CTRL_START_FILL);
  const uint32_t armed = c2h_reg_read(GBUS_REG_C2H_CTRL);
  check_status(c2h_reg_read(GBUS_REG_C2H_STATUS));
  const bool armed_now = (armed & GBUS_C2H_CTRL_ENABLED) != 0;

  std::fprintf(stderr,
               "[fpga-host] GBD1 stream ready base=0x%llx frames=%u bytes=%u min_batch=%u poll_us=%u enabled=%u\n",
               static_cast<unsigned long long>(aperture), c2h_stream_frames_, ring_bytes,
               c2h_stream_min_frames_, c2h_poll_us_, armed_now ? 1U : 0U);

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

    // One grouped read returns available, the read pointer and the write
    // pointer: everything needed to size and address the batch.
    if (!c2h_block_reads_) {
      // The three counters are contiguous, so the block path is always valid
      // on a streaming bitstream; require it rather than paying three reads.
      fail("streaming mode requires grouped register reads", status);
    }
    std::vector<uint32_t> words;
    c2h_read_block(GBUS_REG_C2H_AVAILABLE, 3, words);
    if (words.size() != 3)
      fail("counter read failed", status);
    const uint32_t available = words[0];
    uint32_t rd_ptr = words[1];
    if (available > c2h_stream_frames_ || rd_ptr >= ring_bytes ||
        rd_ptr % c2h_stream_frame_bytes_ != 0)
      fail("invalid counter snapshot", status);
    if (available != previous_available) {
      previous_available = available;
      last_change = Clock::now();
    }

    if (available == 0) {
      // Arming is lost across the startup stream-reset window; re-assert it
      // until the readback shows capture really is on.
      if (!(status & GBUS_C2H_STATUS_ENABLED)) {
        c2h_reg_write(GBUS_REG_C2H_CTRL, GBUS_C2H_CTRL_START_FILL);
      }
      if (c2h_idle_timeout_sec_ &&
          Clock::now() - last_progress >= std::chrono::seconds(c2h_idle_timeout_sec_)) {
        const uint32_t last = c2h_reg_read(GBUS_REG_C2H_STATUS);
        fail(last & GBUS_C2H_STATUS_ENABLED ? "no capture progress" : "capture never armed", last);
      }
      usleep(c2h_poll_us_);
      continue;
    }
    // Wait for a worthwhile batch, but never strand a tail: once the producer
    // has stopped publishing, ship whatever is left.
    if (available < c2h_stream_min_frames_ &&
        Clock::now() - last_change < std::chrono::milliseconds(20)) {
      usleep(c2h_poll_us_);
      continue;
    }

    // A batch never crosses the ring end; a wrapped batch takes two calls.
    const uint32_t frames_to_end = (ring_bytes - rd_ptr) / c2h_stream_frame_bytes_;
    uint32_t take = std::min(available, frames_to_end);
    const size_t count = static_cast<size_t>(take) * c2h_stream_frame_bytes_;

    std::vector<uint8_t> block;
    const uint64_t begin = monotonic_us();
    const int rc = gbus_dma_read(prototyping_, board_, dma_fpga_, ddr_instance_, aperture + rd_ptr,
                                 count, channel_, port_, block);
    const uint64_t elapsed_us = monotonic_us() - begin;
    ++c2h_dma_calls_;
    c2h_dma_us_ += elapsed_us;
    if (!alive())
      return;
    if (rc != 1 || block.size() != count || elapsed_us > 10000000ULL)
      fail("DMA capture failed", status);

    const uint64_t dispatch_begin = monotonic_us();
    accumulator.insert(accumulator.end(), block.begin(), block.end());
    while (accumulator.size() >= sizeof(DmaDiffPackge) && alive()) {
      c2h_dispatch_range(accumulator.data(), sizeof(DmaDiffPackge));
      accumulator.erase(accumulator.begin(), accumulator.begin() + sizeof(DmaDiffPackge));
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
        same = block[i * 4] == static_cast<uint8_t>(word) &&
               block[i * 4 + 1] == static_cast<uint8_t>(word >> 8) &&
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
        std::fprintf(stderr,
                     "[fpga-host] GBD1 stream ended by CPU after frames=%llu bytes=%llu\n",
                     static_cast<unsigned long long>(frames), static_cast<unsigned long long>(bytes));
        break;
      }
      fail("CONSUME was not applied", status_after);
    }

    frames += take;
    bytes += count;
    ++batches;
    last_progress = Clock::now();
    previous_available = 0;  // re-read at the top of the loop
    last_change = Clock::now();

    const bool report = batches <= 2 || (batches & (batches - 1)) == 0;
    if (report) {
      const auto elapsed_ms =
          std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now() - started).count();
      std::fprintf(stderr,
                   "[fpga-host] GBUS_TIMING c2h banks=%llu bytes=%llu elapsed_ms=%lld "
                   "avg_bank_us=%.1f last_batch=%u dma_calls=%llu dma_us=%llu "
                   "reg_calls=%llu reg_us=%llu block_calls=%llu block_us=%llu "
                   "write_calls=%llu write_us=%llu dispatch_us=%llu chunk=%u poll_us=%u "
                   "block_reads=1 fast_fill=0 stream=1\n",
                   static_cast<unsigned long long>(frames), static_cast<unsigned long long>(bytes),
                   static_cast<long long>(elapsed_ms),
                   frames ? static_cast<double>(elapsed_ms) * 1000.0 / frames : 0.0, take,
                   static_cast<unsigned long long>(c2h_dma_calls_),
                   static_cast<unsigned long long>(c2h_dma_us_),
                   static_cast<unsigned long long>(c2h_reg_calls_),
                   static_cast<unsigned long long>(c2h_reg_us_),
                   static_cast<unsigned long long>(c2h_block_calls_),
                   static_cast<unsigned long long>(c2h_block_us_),
                   static_cast<unsigned long long>(c2h_write_calls_),
                   static_cast<unsigned long long>(c2h_write_us_),
                   static_cast<unsigned long long>(c2h_dispatch_us_), c2h_dma_chunk_bytes_,
                   c2h_poll_us_);
      std::fprintf(stderr, "[fpga-host] GBD1 progress mode=stream frames=%llu bytes=%llu\n",
                   static_cast<unsigned long long>(frames), static_cast<unsigned long long>(bytes));
    }
  }

  // Final summary: the periodic dump only fires on powers of two, so without
  // this the tail of a long run is invisible and the totals look smaller than
  // they are.
  const auto total_ms =
      std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now() - started).count();
  std::fprintf(stderr,
               "[fpga-host] GBUS_TIMING c2h totals banks=%llu bytes=%llu batches=%llu "
               "elapsed_ms=%lld avg_bank_us=%.1f dma_calls=%llu dma_us=%llu "
               "reg_calls=%llu reg_us=%llu block_calls=%llu block_us=%llu "
               "write_calls=%llu write_us=%llu dispatch_us=%llu stream=1\n",
               static_cast<unsigned long long>(frames), static_cast<unsigned long long>(bytes),
               static_cast<unsigned long long>(batches), static_cast<long long>(total_ms),
               frames ? static_cast<double>(total_ms) * 1000.0 / frames : 0.0,
               static_cast<unsigned long long>(c2h_dma_calls_),
               static_cast<unsigned long long>(c2h_dma_us_),
               static_cast<unsigned long long>(c2h_reg_calls_),
               static_cast<unsigned long long>(c2h_reg_us_),
               static_cast<unsigned long long>(c2h_block_calls_),
               static_cast<unsigned long long>(c2h_block_us_),
               static_cast<unsigned long long>(c2h_write_calls_),
               static_cast<unsigned long long>(c2h_write_us_),
               static_cast<unsigned long long>(c2h_dispatch_us_));
}

// -------------------------------------------------------- frozen SRAM bank

void GbusTransport::c2h_drain_dma_window() {
  using Clock = std::chrono::steady_clock;
  const auto alive = [&]() { return running_.load() && signal_num == 0; };
  const auto fail = [&](const char *reason, uint32_t status, uint32_t sequence) {
    std::fprintf(stderr, "[fpga-host] GBD1 %s mode=%s status=0x%08x seq=%u (bank not released)\n", reason,
                 c2h_axi_dma_ ? "axi-dma" : "config-window", status, sequence);
    running_.store(false);
    std::exit(EXIT_FAILURE);
  };
  const auto check_status = [&](uint32_t status) {
    if (!(status & GBUS_C2H_STATUS_PRESENT) ||
        (status & (GBUS_C2H_STATUS_FRAME_ERROR | GBUS_C2H_STATUS_PROTOCOL_ERROR)))
      fail("invalid status", status, 0);
  };
  const uint64_t aperture = c2h_reg_read(GBUS_REG_C2H_DMA_BASE);
  const uint32_t capacity = c2h_reg_read(GBUS_REG_C2H_CAPACITY);
  if (aperture != GBUS_C2H_APERTURE || capacity != GBUS_C2H_DATA_BYTES)
    fail("invalid aperture/capacity", 0, 0);
  if (c2h_axi_dma_ && (dma_fpga_ != 2 || fpga_ != 2 || port_ != 0 || channel_ != 0 || ddr_instance_ != 0))
    fail("invalid port-0 DMA topology", 0, 0);
  constexpr uint32_t busy = GBUS_C2H_STATUS_FILLING | GBUS_C2H_STATUS_ACTIVE;
  std::vector<uint8_t> accumulator;
  accumulator.reserve(capacity + sizeof(DmaDiffPackge));
  bool have_sequence = false;
  uint32_t last_sequence = 0;
  uint64_t banks = 0, bytes = 0;
  const auto started = Clock::now();
  auto last_progress = Clock::now();
  auto busy_deadline = Clock::now() + std::chrono::seconds(2);
  std::fprintf(stderr, "[fpga-host] GBD1 ready mode=%s raw_base=0x%llx capacity=%u chunk=%u verify_first=%u\n",
               c2h_axi_dma_ ? "axi-dma" : "config-window", static_cast<unsigned long long>(aperture), capacity,
               c2h_dma_chunk_bytes_, c2h_verify_first_bank_);
  while (alive()) {
    const auto bank_begin = Clock::now();
    uint64_t fill_us = 0;
    uint32_t status = c2h_reg_read(GBUS_REG_C2H_STATUS);
    check_status(status);
    if (c2h_idle_timeout_sec_ && Clock::now() - last_progress >= std::chrono::seconds(c2h_idle_timeout_sec_))
      fail("no publication progress", status, last_sequence);
    if (status & busy) {
      if (Clock::now() >= busy_deadline)
        fail("busy deadline exceeded", status, last_sequence);
      usleep(c2h_poll_us_);
      continue;
    }
    busy_deadline = Clock::now() + std::chrono::seconds(2);
    if (!(status & GBUS_C2H_STATUS_FROZEN)) {
      if (!(status & GBUS_C2H_STATUS_HAS_DATA)) {
        usleep(c2h_poll_us_);
        continue;
      }
      if (c2h_pingpong_ready_) {
        // The FIFO-backed engine captures autonomously; a FILL would be
        // rejected.  Wait for the next published buffer.
        usleep(c2h_poll_us_);
        continue;
      }
      if (!alive())
        return;
      const auto fill_begin = Clock::now();
      // A FILL that arrives before the startup stream-reset window releases is
      // consumed without any state change or fault evidence and is never
      // replayed by the RTL.  Re-issue the same logical control, but only when
      // the first-control evidence proves exactly that benign consumption:
      // the count advanced, the data word matches this request, and no reject
      // flag was latched.  Anything else stays fatal, and the budget is bound-
      // ed so a stuck window can only stop the run, never spin it.
      constexpr unsigned max_fill_reissues = 3;
      unsigned reissues = 0;
      for (;;) {
        const uint32_t ctrl_count = c2h_reg_read(GBUS_REG_C2H_CTRL_COUNT);
        c2h_reg_write(GBUS_REG_C2H_CTRL, GBUS_C2H_CTRL_START_FILL);
        const auto deadline = Clock::now() + std::chrono::seconds(2);
        // On the benign startup-window path the control is consumed
        // immediately and no fill state ever appears; proving that costs one
        // evidence read instead of waiting out the full deadline.
        auto next_evidence = Clock::now() + std::chrono::milliseconds(20);
        do {
          if (!alive())
            return;
          status = c2h_reg_read(GBUS_REG_C2H_STATUS);
          check_status(status);
          if ((status & GBUS_C2H_STATUS_FROZEN) && !(status & busy))
            break;
          if (c2h_fast_fill_ && !(status & busy) && Clock::now() >= next_evidence) {
            uint32_t evidence_count = 0, evidence_data = 0, bad_ctrl = 0;
            c2h_read_fill_evidence(evidence_count, evidence_data, bad_ctrl);
            if (evidence_count == ctrl_count + 1U && evidence_data == GBUS_C2H_CTRL_START_FILL &&
                !(bad_ctrl & GBUS_C2H_BAD_CTRL_VALID)) {
              if (++reissues > max_fill_reissues)
                fail("fill window never released", status, 0);
              std::fprintf(stderr,
                           "[fpga-host] GBD1 startup window consumed FILL %u/%u; re-issuing after stream release\n",
                           reissues, max_fill_reissues);
              break;
            }
            next_evidence = Clock::now() + std::chrono::milliseconds(20);
          }
          if (Clock::now() >= deadline) {
            // A fill actually in progress (FILLING/ACTIVE latched) is a real
            // timeout; only a control that left no fill state at all can be
            // the benign startup-window consumption.
            uint32_t evidence_count = 0, evidence_data = 0, bad_ctrl = 0;
            c2h_read_fill_evidence(evidence_count, evidence_data, bad_ctrl);
            if ((status & busy) || evidence_count != ctrl_count + 1U || evidence_data != GBUS_C2H_CTRL_START_FILL ||
                (bad_ctrl & GBUS_C2H_BAD_CTRL_VALID))
              fail("fill deadline exceeded", status, 0);
            if (++reissues > max_fill_reissues)
              fail("fill window never released", status, 0);
            std::fprintf(stderr,
                         "[fpga-host] GBD1 startup window consumed FILL %u/%u; re-issuing after stream release\n",
                         reissues, max_fill_reissues);
            break;
          }
          usleep(c2h_poll_us_);
        } while (alive());
        if (!(status & GBUS_C2H_STATUS_FROZEN)) {
          if (!alive())
            return;
          continue;
        }
        if (status & busy) {
          // The control was accepted but this read sampled mid-fill; fall back
          // to the ordinary completion wait below.
          const auto finish_deadline = Clock::now() + std::chrono::seconds(2);
          do {
            if (!alive())
              return;
            status = c2h_reg_read(GBUS_REG_C2H_STATUS);
            check_status(status);
            if ((status & GBUS_C2H_STATUS_FROZEN) && !(status & busy))
              break;
            if (Clock::now() >= finish_deadline)
              fail("fill deadline exceeded", status, 0);
            usleep(c2h_poll_us_);
          } while (alive());
        }
        break;
      }
      fill_us = static_cast<uint64_t>(
          std::chrono::duration_cast<std::chrono::microseconds>(Clock::now() - fill_begin).count());
    }
    if (!alive())
      return;
    const auto stable = [&](uint32_t expect_seq, uint32_t expect_status) {
      uint32_t stable_status = 0, stable_seq = 0;
      c2h_read_stable_regs(stable_status, stable_seq);
      status = stable_status;
      check_status(status);
      if (stable_seq != expect_seq || !(status & GBUS_C2H_STATUS_FROZEN) || (status & busy) ||
          ((status >> GBUS_C2H_STATUS_STAGED_SHIFT) & GBUS_C2H_STATUS_STAGED_MASK) * 4 !=
              (((expect_status >> GBUS_C2H_STATUS_STAGED_SHIFT) & GBUS_C2H_STATUS_STAGED_MASK) * 4))
        fail("bank changed during capture", status, expect_seq);
    };
    uint32_t sequence = 0, ar_count_before = 0;
    if (c2h_aperture_header_ready_) {
      // The snapshot header carries these fields for this read, so no register
      // access is needed before the DMA.
    } else if (c2h_block_reads_) {
      // One call returns the frozen status, the sequence and the AR counter
      // that this bank's DMA must advance.
      std::vector<uint32_t> words;
      c2h_read_block(GBUS_REG_C2H_STATUS, 13, words);
      status = words[0];
      check_status(status);
      sequence = words[3];
      ar_count_before = words[8];
    } else {
      sequence = c2h_reg_read(GBUS_REG_C2H_SEQ);
      ar_count_before = c2h_reg_read(GBUS_REG_C2H_AR_COUNT);
    }
    const size_t valid = ((status >> GBUS_C2H_STATUS_STAGED_SHIFT) & GBUS_C2H_STATUS_STAGED_MASK) * 4;
    if (!valid || valid > capacity || valid % 32 ||
        (!c2h_aperture_header_ready_ && have_sequence && sequence != last_sequence + 1U))
      fail("invalid published length/sequence", status, sequence);
    std::vector<uint8_t> window;
    window.reserve(valid);
    if (c2h_axi_dma_ && c2h_aperture_header_ready_) {
      // One DMA read returns the 64-byte snapshot header followed by the whole
      // frozen bank.  The header carries the status, sequence, length and the
      // AR evidence of this very read, so no register poll is needed before
      // ACK.  A status read is still required to observe the ACK release.
      const size_t count = c2h_header_bytes_ + valid;
      std::vector<uint8_t> block;
      const uint64_t begin = monotonic_us();
      const int rc =
          gbus_dma_read(prototyping_, board_, dma_fpga_, ddr_instance_, aperture, count, channel_, port_, block);
      const uint64_t elapsed_us = monotonic_us() - begin;
      ++c2h_dma_calls_;
      c2h_dma_us_ += elapsed_us;
      if (!alive())
        return;
      if (rc != 1 || block.size() != count || elapsed_us > 10000000ULL) {
        std::fprintf(stderr, "[fpga-host] GBD1 header+payload DMA failed rc=%d addr=0x%llx requested=%zu actual=%zu\n",
                     rc, static_cast<unsigned long long>(aperture), count, block.size());
        fail("DMA capture failed; no further transport I/O", status, sequence);
      }
      auto hw = [&](size_t index) {
        return static_cast<uint32_t>(block[index]) | (static_cast<uint32_t>(block[index + 1]) << 8) |
               (static_cast<uint32_t>(block[index + 2]) << 16) | (static_cast<uint32_t>(block[index + 3]) << 24);
      };
      const uint32_t hdr_status = hw(0);
      const uint32_t hdr_flags = hw(4);
      const uint32_t hdr_seq = hw(8);
      const uint32_t hdr_words = hw(12) & 0x1ffU;
      const uint32_t hdr_base = hw(16);
      const uint32_t hdr_capacity = hw(20);
      const uint32_t hdr_ar_addr = hw(24);
      const uint32_t hdr_attrs = hw(28);
      const uint32_t hdr_ar_count = hw(32);
      check_status(hdr_status);
      sequence = hdr_seq;
      if ((hdr_flags & (GBUS_C2H_HDR_FLAG_PRESENT | GBUS_C2H_HDR_FLAG_HEADER)) !=
              (GBUS_C2H_HDR_FLAG_PRESENT | GBUS_C2H_HDR_FLAG_HEADER) ||
          hdr_base != GBUS_C2H_APERTURE + c2h_header_bytes_ || hdr_capacity != GBUS_C2H_DATA_BYTES ||
          hdr_words * 4 != valid || (have_sequence && hdr_seq != last_sequence + 1U) ||
          (hdr_status & GBUS_C2H_STATUS_FILLING) || !(hdr_status & GBUS_C2H_STATUS_FROZEN))
        fail("snapshot header does not describe the frozen bank", hdr_status, hdr_seq);
      const uint32_t burst = (hdr_attrs >> 22) & 3U;
      const uint32_t size = (hdr_attrs >> 19) & 7U;
      const uint32_t len = (hdr_attrs >> 15) & 15U;
      const uint64_t expect_end = aperture + count;
      if (burst != 1 || size != 5 || hdr_ar_addr < aperture ||
          static_cast<uint64_t>(hdr_ar_addr) + (len + 1U) * 32U > expect_end)
        fail("snapshot header lacks matching AXI address evidence", hdr_status, hdr_seq);
      if (!banks)
        std::fprintf(stderr, "[fpga-host] GBD1 AXI verified addr=0x%08x len=%u count=%u bytes=%zu (snapshot)\n",
                     hdr_ar_addr, len, hdr_ar_count, count);
      (void)ar_count_before;
      window.insert(window.end(), block.begin() + c2h_header_bytes_, block.end());
    } else if (c2h_axi_dma_) {
      for (size_t offset = 0; offset < valid && alive();) {
        const size_t count = std::min<size_t>(c2h_dma_chunk_bytes_, valid - offset);
        const uint64_t address = aperture + offset;
        if (address < aperture || count > capacity - offset || address % 32)
          fail("invalid DMA range", status, sequence);
        {
          const uint32_t before = ar_count_before;
          std::vector<uint8_t> block;
          const uint64_t begin = monotonic_us();
          const int rc =
              gbus_dma_read(prototyping_, board_, dma_fpga_, ddr_instance_, address, count, channel_, port_, block);
          const uint64_t elapsed_us = monotonic_us() - begin;
          ++c2h_dma_calls_;
          c2h_dma_us_ += elapsed_us;
          if (!alive())
            return;
          if (rc != 1 || block.size() != count || elapsed_us > 10000000ULL) {
            std::fprintf(stderr, "[fpga-host] GBD1 DMA failed rc=%d addr=0x%llx requested=%zu actual=%zu us=%llu\n", rc,
                         static_cast<unsigned long long>(address), count, block.size(),
                         static_cast<unsigned long long>(elapsed_us));
            fail("DMA capture failed; no further transport I/O", status, sequence);
          }
          // The DMA call is synchronous; ACTIVE should already be clear.  Poll
          // only if this runtime still reports the read in progress, and take
          // the stability/AR evidence sample once it is.
          uint32_t ar_count_after = 0, ar_address = 0, attrs = 0;
          const auto active_deadline = Clock::now() + std::chrono::seconds(2);
          for (;;) {
            if (c2h_block_reads_) {
              std::vector<uint32_t> words;
              c2h_read_block(GBUS_REG_C2H_STATUS, 13, words);
              status = words[0];
              check_status(status);
              if (status & GBUS_C2H_STATUS_ACTIVE) {
                if (Clock::now() >= active_deadline)
                  fail("DMA active deadline exceeded", status, sequence);
                usleep(c2h_poll_us_);
                continue;
              }
              if (words[3] != sequence ||
                  ((status >> GBUS_C2H_STATUS_STAGED_SHIFT) & GBUS_C2H_STATUS_STAGED_MASK) * 4 != valid ||
                  !(status & GBUS_C2H_STATUS_FROZEN) || (status & busy))
                fail("bank changed during capture", status, sequence);
              ar_address = words[6];
              attrs = words[7];
              ar_count_after = words[8];
            } else {
              const uint32_t polled = c2h_reg_read(GBUS_REG_C2H_STATUS);
              check_status(polled);
              status = polled;
              if (status & GBUS_C2H_STATUS_ACTIVE) {
                if (Clock::now() >= active_deadline)
                  fail("DMA active deadline exceeded", status, sequence);
                usleep(c2h_poll_us_);
                continue;
              }
              stable(sequence, status);
              ar_count_after = c2h_reg_read(GBUS_REG_C2H_AR_COUNT);
              ar_address = c2h_reg_read(GBUS_REG_C2H_AR_ADDR);
              attrs = c2h_reg_read(GBUS_REG_C2H_AR_ATTRS);
            }
            break;
          }
          const uint32_t burst = (attrs >> 22) & 3U;
          const uint32_t size = (attrs >> 19) & 7U;
          const uint32_t len = (attrs >> 15) & 15U;
          if (ar_count_after == before || ar_count_after - before > count / 32 || burst != 1 || size != 5 ||
              ar_address < address || static_cast<uint64_t>(ar_address) + (len + 1U) * 32U != address + count)
            fail("DMA completion lacks matching AXI address evidence", status, sequence);
          if (!banks)
            std::fprintf(stderr, "[fpga-host] GBD1 AXI verified addr=0x%08x len=%u count=%u bytes=%zu\n", ar_address,
                         len, ar_count_after, count);
          ar_count_before = ar_count_after;
          window.insert(window.end(), block.begin(), block.end());
        }
        if (!alive())
          return;
        offset += count;
      }
    }
    if (!c2h_axi_dma_ || (!banks && c2h_verify_first_bank_)) {
      std::vector<uint8_t> mirror;
      mirror.reserve(valid);
      for (size_t offset = 0; offset < valid && alive(); offset += 4)
        append_le32(mirror, c2h_reg_read(GBUS_REG_C2H_DATA + offset));
      if (!alive())
        return;
      // In ping-pong mode the sequence lives in the per-bank snapshot and the
      // legacy 0x120c register no longer tracks it.  The bank cannot change
      // until ACK, so the config mirror below is already a stable sample.
      if (!c2h_pingpong_ready_)
        stable(sequence, status);
      if (c2h_axi_dma_) {
        if (mirror != window)
          fail("DMA/config mirror mismatch", status, sequence);
        std::fprintf(stderr, "[fpga-host] GBD1 first frozen bank DMA/config mirror verified seq=%u bytes=%zu\n",
                     sequence, valid);
      } else {
        window.swap(mirror);
      }
    }
    if (!alive())
      return;
    // The bank is frozen until ACK, and the last status read above is the
    // capture; the ACK poll below re-reads status and catches any fault raised
    // by the release itself.
    c2h_reg_write(GBUS_REG_C2H_CTRL, GBUS_C2H_CTRL_ACK);
    const auto ack_deadline = Clock::now() + std::chrono::seconds(2);
    do {
      if (!alive())
        return;
      status = c2h_reg_read(GBUS_REG_C2H_STATUS);
      check_status(status);
      if (c2h_pingpong_ready_) {
        // Releasing one buffer immediately hands over the other if it is
        // already full, so FROZEN need not drop; a fault-free read is enough.
        break;
      }
      if (!(status & (GBUS_C2H_STATUS_FROZEN | busy)))
        break;
      if (Clock::now() >= ack_deadline)
        fail("ACK deadline exceeded", status, sequence);
      usleep(c2h_poll_us_);
    } while (alive());
    if (!alive())
      return;
    last_sequence = sequence;
    have_sequence = true;
    accumulator.insert(accumulator.end(), window.begin(), window.end());
    while (accumulator.size() >= sizeof(DmaDiffPackge) && alive()) {
      c2h_dispatch_range(accumulator.data(), sizeof(DmaDiffPackge));
      accumulator.erase(accumulator.begin(), accumulator.begin() + sizeof(DmaDiffPackge));
    }
    if (timing_ && banks < 8)
      std::fprintf(stderr,
                   "[fpga-host] GBUS_TRACE bank=%llu seq=%u valid=%zu head=%02x%02x%02x%02x%02x%02x%02x%02x pp=%u\n",
                   static_cast<unsigned long long>(banks), sequence, valid, window[0], window[1], window[2], window[3],
                   window[4], window[5], window[6], window[7], c2h_pingpong_ready_ ? 1U : 0U);
    bytes += valid;
    ++banks;
    if (timing_ && (banks <= 2 || (banks & (banks - 1)) == 0)) {
      const auto elapsed_us = std::chrono::duration_cast<std::chrono::microseconds>(Clock::now() - started).count();
      const auto bank_us = std::chrono::duration_cast<std::chrono::microseconds>(Clock::now() - bank_begin).count();
      std::fprintf(
          stderr,
          "[fpga-host] GBUS_TIMING c2h banks=%llu bytes=%llu elapsed_ms=%lld avg_bank_us=%.1f last_bank_us=%lld "
          "fill_us=%llu dma_calls=%llu dma_us=%llu reg_calls=%llu reg_us=%llu block_calls=%llu block_us=%llu "
          "write_calls=%llu write_us=%llu dispatch_us=%llu chunk=%u poll_us=%u block_reads=%u fast_fill=%u\n",
          static_cast<unsigned long long>(banks), static_cast<unsigned long long>(bytes),
          static_cast<long long>(elapsed_us / 1000), banks ? static_cast<double>(elapsed_us) / banks : 0.0,
          static_cast<long long>(bank_us), static_cast<unsigned long long>(fill_us),
          static_cast<unsigned long long>(c2h_dma_calls_), static_cast<unsigned long long>(c2h_dma_us_),
          static_cast<unsigned long long>(c2h_reg_calls_), static_cast<unsigned long long>(c2h_reg_us_),
          static_cast<unsigned long long>(c2h_block_calls_), static_cast<unsigned long long>(c2h_block_us_),
          static_cast<unsigned long long>(c2h_write_calls_), static_cast<unsigned long long>(c2h_write_us_),
          static_cast<unsigned long long>(c2h_dispatch_us_), c2h_dma_chunk_bytes_, c2h_poll_us_,
          c2h_block_reads_ ? 1U : 0U, c2h_fast_fill_ ? 1U : 0U);
    }
    if (banks <= 2 || (banks & (banks - 1)) == 0)
      std::fprintf(stderr, "[fpga-host] GBD1 progress mode=%s banks=%llu bytes=%llu seq=%u\n",
                   c2h_axi_dma_ ? "axi-dma" : "config-window", static_cast<unsigned long long>(banks),
                   static_cast<unsigned long long>(bytes), sequence);
    last_progress = Clock::now();
  }
}

// --------------------------------------------------------------- DDR ring

void GbusTransport::c2h_drain_ddr_ring() {
  dprintf(STDERR_FILENO, "[fpga-host] GBus start ring parameters base=0x%llx size=%llu wptr=0x%llx packet=%zu\n",
          static_cast<unsigned long long>(c2h_ring_base_), static_cast<unsigned long long>(c2h_ring_size_),
          static_cast<unsigned long long>(c2h_wptr_offset_), sizeof(FpgaPackgeHead));
  if (c2h_ring_base_ == 0 || c2h_ring_size_ < sizeof(FpgaPackgeHead) || c2h_wptr_offset_ == 0) {
    dprintf(STDERR_FILENO, "[fpga-host] GBus C2H ring is not configured; set GBUS_C2H_RING_BASE/SIZE/WPTR_OFFSET\n");
    while (running_.load() && signal_num == 0)
      usleep(10000);
    return;
  }
  if (c2h_ring_size_ % sizeof(FpgaPackgeHead) != 0) {
    dprintf(STDERR_FILENO, "[fpga-host] GBus C2H ring size %llu is not packet aligned (%zu)\n",
            static_cast<unsigned long long>(c2h_ring_size_), sizeof(FpgaPackgeHead));
    std::exit(EXIT_FAILURE);
  }
  const uint32_t hardware_base = fpga_io_read(0x0100);
  const uint32_t hardware_size = fpga_io_read(0x0104);
  const uint32_t hardware_status = fpga_io_read(0x010c);
  // The control window reports the CPU-visible ring reservation (normally
  // 0x81000000).  GBus DMA reads use the corresponding DDR offset
  // (normally 0x01000000), which is intentionally kept separate below.
  if (hardware_base != static_cast<uint32_t>(c2h_ring_base_) ||
      hardware_size != static_cast<uint32_t>(c2h_ring_size_) || (hardware_status & 1U) == 0) {
    dprintf(STDERR_FILENO,
            "[fpga-host] GBus C2H ABI mismatch hw_guest_base=0x%x sw_guest_base=0x%llx dma_base=0x%llx hw_size=0x%x "
            "sw_size=0x%llx status=0x%x\n",
            hardware_base, static_cast<unsigned long long>(c2h_ring_base_),
            static_cast<unsigned long long>(c2h_dma_base_), hardware_size,
            static_cast<unsigned long long>(c2h_ring_size_), hardware_status);
    std::exit(EXIT_FAILURE);
  }
  if (hardware_status & 0x80000000U) {
    dprintf(STDERR_FILENO, "[fpga-host] GBus C2H ring reports a prior DDR write error status=0x%x\n", hardware_status);
    std::exit(EXIT_FAILURE);
  }
  // HOST_IO_RESET already resets the ring writer before CPU release.  The
  // producer register is read-only in the always-running GENERALBD domain;
  // do not pretend that a cross-domain producer reset write succeeded.
  dprintf(STDERR_FILENO, "[fpga-host] GBus C2H polling ring base=0x%llx size=%llu wptr=0x%llx\n",
          static_cast<unsigned long long>(c2h_ring_base_), static_cast<unsigned long long>(c2h_ring_size_),
          static_cast<unsigned long long>(c2h_wptr_offset_));
  uint32_t read_ptr = 0;
  if (fpga_io_read(c2h_wptr_offset_) != 0) {
    dprintf(STDERR_FILENO, "[fpga-host] GBus C2H producer is non-zero after HOST_IO_RESET\n");
    std::exit(EXIT_FAILURE);
  }
  const uint64_t packet_size = sizeof(FpgaPackgeHead);
  while (running_.load() && signal_num == 0) {
    std::vector<uint8_t> ptr_data;
    if (gbus_read(prototyping_, board_, fpga_, config_instance_, config_base_ + c2h_wptr_offset_, 1, ptr_data) != 1) {
      dprintf(STDERR_FILENO, "[fpga-host] GBus C2H write-pointer read failed\n");
      std::exit(EXIT_FAILURE);
    }
    const uint32_t write_ptr = load_le32(ptr_data);
    const uint32_t ring_status = fpga_io_read(0x010c);
    if (ring_status & 0x80000000U) {
      dprintf(STDERR_FILENO, "[fpga-host] GBus C2H DDR write failed status=0x%x producer=0x%x\n", ring_status,
              write_ptr);
      std::exit(EXIT_FAILURE);
    }
    // Both pointers are byte offsets modulo the ring size.  Keeping the
    // consumer pointer bounded is required once the producer wraps from the
    // final packet back to offset zero; a raw unsigned subtraction would look
    // like a multi-gigabyte overflow at that point.
    const uint32_t available =
        write_ptr >= read_ptr ? write_ptr - read_ptr : static_cast<uint32_t>(c2h_ring_size_) - read_ptr + write_ptr;
    if (available >= c2h_ring_size_ || available % packet_size != 0) {
      dprintf(STDERR_FILENO, "[fpga-host] GBus C2H overflow producer=0x%x consumer=0x%x available=%u ring=%llu\n",
              write_ptr, read_ptr, available, static_cast<unsigned long long>(c2h_ring_size_));
      running_.store(false);
      std::exit(EXIT_FAILURE);
    }
    uint32_t pending = available;
    while (pending >= packet_size && running_.load() && signal_num == 0) {
      const uint64_t offset = c2h_dma_base_ + (read_ptr % c2h_ring_size_);
      std::vector<uint8_t> packet;
      if (gbus_dma_read(prototyping_, board_, dma_fpga_, ddr_instance_, offset, packet_size, channel_, port_, packet) !=
              1 ||
          packet.size() < packet_size) {
        dprintf(STDERR_FILENO, "[fpga-host] GBus C2H packet read failed offset=0x%llx\n",
                static_cast<unsigned long long>(offset));
        std::exit(EXIT_FAILURE);
      }
      c2h_dispatch_range(packet.data(), packet_size);
      read_ptr += static_cast<uint32_t>(packet_size);
      if (read_ptr >= c2h_ring_size_)
        read_ptr -= static_cast<uint32_t>(c2h_ring_size_);
      pending -= static_cast<uint32_t>(packet_size);
    }
    usleep(c2h_poll_us_);
  }
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
  // A GBus DMA completion is reported by the runtime call itself.  The
  // legacy XDMA status registers are not guaranteed to be mirrored by the
  // GENERALBD window (a read may legitimately remain zero), so do not turn
  // a completed GBus transfer into an infinite host-side poll.  Set
  // GBUS_POLL_STATUS=1 to require the legacy register handshake while
  // debugging a board integration that implements it.
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
  if (size % 32 || size > GBUS_C2H_APERTURE - ddr_base_ || size > std::numeric_limits<size_t>::max() || port_ != 0) {
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
  // The vendor write path reports success for chunks that did not land, so the
  // DDR content is verified instead of trusted.  A stale image is otherwise
  // indistinguishable from a CPU bug: the reference runs the intended file while
  // the DUT executes whatever the DDR happens to hold.
  const uint64_t verify_chunk = 1ULL << 18;  // 256 KiB, well under the 0x7FFFF API cap
  uint64_t mismatched = 0, patched = 0;
  for (uint64_t offset = 0; offset < size; offset += verify_chunk) {
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
      if (gbus_dma_write(prototyping_, board_, dma_fpga_, ddr_instance_, ddr_base_ + offset, count, channel_,
                         port_, data) != 1) {
        std::fprintf(stderr, "[fpga-host] GBus H2C rewrite failed offset=0x%llx\n",
                     static_cast<unsigned long long>(offset));
        std::exit(EXIT_FAILURE);
      }
      ++patched;
    }
  }
  std::fprintf(stderr, "[fpga-host] GBus H2C readback verified bytes=%llu chunks_failed=%llu patched=%llu\n",
               static_cast<unsigned long long>(size), static_cast<unsigned long long>(mismatched),
               static_cast<unsigned long long>(patched));

  std::fprintf(stderr, "[fpga-host] GBus DMA workload queued %llu bytes\n", static_cast<unsigned long long>(size));
}

void GbusTransport::validate_guest_ram(uint64_t base, uint64_t size) const {
  // The SRAM staging window is on-chip, so it reserves no guest address space
  // and the CPU may use the whole DDR.  Only the legacy DDR ring needs a
  // reservation carved out of guest RAM.
  if (c2h_sram_)
    return;
  if (base > c2h_ring_base_ || size > c2h_ring_base_ - base) {
    std::fprintf(stderr, "[fpga-host] guest RAM [0x%llx,0x%llx) overlaps reserved GBus C2H ring at 0x%llx\n",
                 static_cast<unsigned long long>(base), static_cast<unsigned long long>(base + size),
                 static_cast<unsigned long long>(c2h_ring_base_));
    std::exit(EXIT_FAILURE);
  }
}
