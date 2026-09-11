#include "gbus_transport.h"
#include "difftest-dpic.h"
#include "xdma.h"
#include <algorithm>
#include <cerrno>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <thread>
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
constexpr uint32_t GBUS_C2H_ID_MAGIC = 0x47425331u;     // ASCII "GBS1"
constexpr uint32_t GBUS_C2H_DMA_ID_MAGIC = 0x47424431u; // ASCII "GBD1"
constexpr uint64_t GBUS_REG_C2H_SEQ = 0x120c;
constexpr uint64_t GBUS_REG_C2H_DMA_BASE = 0x1210;
constexpr uint64_t GBUS_REG_C2H_CAPACITY = 0x1214;
// GBD1 bring-up diagnostics: what the endpoint actually saw on its AXI read
// port.  Every window is republished at the same absolute aperture, so an
// engine that ignored the requested offset would still hand back well-formed
// data; these registers are what separates that case from a framing bug.
constexpr uint64_t GBUS_REG_C2H_AR_ADDR = 0x1218;
constexpr uint64_t GBUS_REG_C2H_AR_ATTRS = 0x121c;
constexpr uint64_t GBUS_REG_C2H_AR_COUNT = 0x1220;
constexpr uint32_t GBUS_C2H_STATUS_PROTOCOL_ERROR = 1U << 29; // GBD1 only
constexpr uint32_t GBUS_C2H_STATUS_FROZEN = 1U << 5;
constexpr uint32_t GBUS_C2H_STATUS_ACTIVE = 1U << 4;
constexpr uint64_t GBUS_REG_C2H_DATA = 0x2000;
constexpr uint32_t GBUS_C2H_STAGE_WORDS = 256;
constexpr uint32_t GBUS_C2H_DATA_BYTES = GBUS_C2H_STAGE_WORDS * 4;

constexpr uint32_t GBUS_C2H_STATUS_PRESENT = 1U << 31;
constexpr uint32_t GBUS_C2H_STATUS_FRAME_ERROR = 1U << 30;
constexpr uint32_t GBUS_C2H_STATUS_DRAINING = 1U << 29;
constexpr uint32_t GBUS_C2H_STATUS_STAGED_SHIFT = 8;
constexpr uint32_t GBUS_C2H_STATUS_STAGED_MASK = 0x1ffU;
constexpr uint32_t GBUS_C2H_STATUS_FILLING = 1U << 7;
constexpr uint32_t GBUS_C2H_STATUS_HAS_DATA = 1U << 6;

constexpr uint32_t GBUS_C2H_CTRL_START_FILL = 1U << 0;
constexpr uint32_t GBUS_C2H_CTRL_DRAIN = 1U << 1;

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

uint64_t monotonic_ms() {
  return static_cast<uint64_t>(
      std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch())
          .count());
}
uint64_t env_u64(const char *name, uint64_t fallback) {
  const char *v = std::getenv(name);
  if (!v || !*v)
    return fallback;
  char *end = nullptr;
  errno = 0;
  unsigned long long parsed = std::strtoull(v, &end, 0);
  return errno || end == v || *end ? fallback : static_cast<uint64_t>(parsed);
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

void dump_gbus_packet(const std::vector<uint8_t> &packet, uint64_t packet_index) {
  const uint64_t limit = env_u64("GBUS_C2H_DUMP_PACKETS", 0);
  if (packet_index >= limit)
    return;

  dprintf(STDERR_FILENO, "[fpga-host] GBus C2H raw packet=%llu bytes=%zu\n",
          static_cast<unsigned long long>(packet_index), packet.size());
  for (size_t record = 0; record < DMA_PACKGE_NUM; ++record) {
    const size_t begin = record * sizeof(DmaDiffPackge);
    dprintf(STDERR_FILENO, "[fpga-host] GBus C2H raw packet=%llu record=%zu offset=0x%zx id=0x%02x data=",
            static_cast<unsigned long long>(packet_index), record, begin, begin < packet.size() ? packet[begin] : 0xff);
    const size_t end = std::min(begin + sizeof(DmaDiffPackge), packet.size());
    for (size_t i = begin; i < end; ++i)
      dprintf(STDERR_FILENO, "%02x", packet[i]);
    dprintf(STDERR_FILENO, "\n");
  }
}
} // namespace

GbusTransport::GbusTransport() {
  prototyping_ = static_cast<uint8_t>(env_u64("GBUS_PROTOTYPING_INSTANCE", 0));
  board_ = static_cast<uint8_t>(env_u64("GBUS_BOARD", 0));
  // The runtime topology mapping is platform-specific.  On the checked UVHS
  // U2.2 setup, value 2 selects the design placed on B0.F2 (reported by the
  // daemon as topology fpgaId 3).  Keep it explicit and overrideable.
  fpga_ = static_cast<uint8_t>(env_u64("GBUS_FPGA", 2));
  dma_fpga_ = static_cast<uint8_t>(env_u64("GBUS_DMA_FPGA", fpga_));
  config_instance_ = static_cast<uint8_t>(env_u64("GBUS_CONFIG_INSTANCE", 0));
  ddr_instance_ = static_cast<uint8_t>(env_u64("GBUS_DDR_INSTANCE", 0));
  channel_ = static_cast<uint8_t>(env_u64("GBUS_CHANNEL", 0));
  port_ = static_cast<uint8_t>(env_u64("GBUS_PORT", 0));
  config_base_ = env_u64("GBUS_CONFIG_BASE", 0x1000ULL);
  // GBus DMA offsets are relative to the selected DDR IP.  NutShell's AXI
  // map starts at 0x80000000, but passing that CPU address to the runtime is
  // invalid (the verified GBus probe accepts 0x0/0x20 and rejects 0x80000000).
  ddr_base_ = env_u64("GBUS_DDR_BASE", 0x0ULL);
  // 0 means "use the largest burst the runtime accepts", resolved by the probe
  // below.  Set GBUS_C2H_BURST_WORDS=1 to force one 32-bit read per call.
  c2h_burst_words_ = static_cast<uint32_t>(env_u64("GBUS_C2H_BURST_WORDS", 0));
  c2h_poll_us_ = static_cast<uint32_t>(env_u64("GBUS_C2H_POLL_US", 1000));
  c2h_idle_timeout_sec_ = static_cast<uint32_t>(env_u64("GBUS_C2H_IDLE_TIMEOUT_SEC", 30));
  config_readback_ = env_u64("GBUS_CONFIG_READBACK", 0) != 0;
  const char *host = std::getenv("GBUS_HOST");
  host_ = host && *host ? host : "localhost";
  initialized_ = gbus_initialize(host_.c_str());
  if (!initialized_) {
    std::fprintf(stderr, "[fpga-host] GBus initialize failed (host=%s)\n", host_.c_str());
    std::exit(EXIT_FAILURE);
  } else {
    std::fprintf(stderr,
                 "[fpga-host] GBus initialized host=%s board=%u fpga=%u config_base=0x%llx ddr_base=0x%llx "
                 "\n",
                 host_.c_str(), board_, fpga_, static_cast<unsigned long long>(config_base_),
                 static_cast<unsigned long long>(ddr_base_));
    std::fprintf(stderr, "[fpga-host] GBus C2H interface layer=sram-window\n");
    if (config_readback_)
      std::fprintf(stderr, "[fpga-host] GBus config readback verification enabled\n");
  }
}

GbusTransport::~GbusTransport() {
  if (initialized_)
    gbus_finalize();
}

void GbusTransport::start(bool enable_diff) {
  dprintf(STDERR_FILENO, "[fpga-host] GBus start direct marker\n");
  dprintf(STDERR_FILENO, "[fpga-host] GBus start enter enable_diff=%d t=%llu\n", enable_diff ? 1 : 0,
          static_cast<unsigned long long>(monotonic_ms()));
  running_.store(true);
  dprintf(STDERR_FILENO, "[fpga-host] GBus start state initialized\n");
  if (!enable_diff) {
    while (running_.load() && signal_num == 0)
      usleep(10000);
    return;
  }
  c2h_last_progress_ns_ =
      std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
  c2h_drain_sram_fifo();
}

// ---------------------------------------------------------------- register I/O

uint32_t GbusTransport::c2h_reg_read(uint64_t offset) {
  std::vector<uint8_t> data;
  const uint64_t begin = monotonic_us();
  const int rc = gbus_read(prototyping_, board_, fpga_, config_instance_, config_base_ + offset, 1, data);
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
  if (rc != 1) {
    dprintf(STDERR_FILENO,
            "[fpga-host] GBus C2H register write failed rc=%d offset=0x%llx size=4 value=0x%x begin=%llu end=%llu\n",
            rc, static_cast<unsigned long long>(config_base_ + offset), value, static_cast<unsigned long long>(begin),
            static_cast<unsigned long long>(monotonic_us()));
    std::exit(EXIT_FAILURE);
  }
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
    std::fprintf(stderr, "[fpga-host] GBus multi-word probe: count=%u rc=%d bytes=%zu (expected %u) -> not usable\n",
                 words, rc, block.size(), words * 4);
    return false;
  }
  for (size_t i = 0; i < words; ++i) {
    std::vector<uint8_t> one;
    if (gbus_read(prototyping_, board_, fpga_, config_instance_, config_base_ + i * 4, 1, one) != 1 ||
        one.size() != 4) {
      std::fprintf(stderr, "[fpga-host] GBus multi-word probe: single read of word %zu failed\n", i);
      return false;
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
  dump_gbus_packet(range, c2h_reads_);
  auto *head = reinterpret_cast<const FpgaPackgeHead *>(range.data());
  for (size_t i = 0; i < DMA_PACKGE_NUM; ++i) {
    auto *payload_ptr = const_cast<uint8_t *>(head->diff_packge[i].diff_packge);
    v_difftest_Batch(payload_ptr);
  }
}

void GbusTransport::c2h_drain_sram_fifo() {
  // Identify before any probe or data access: GBS1 reads are destructive,
  // whereas GBD1 DMA reads must leave the frozen window intact until ACK.
  const uint32_t id = c2h_reg_read(GBUS_REG_C2H_ID);
  if (id == GBUS_C2H_DMA_ID_MAGIC) {
    c2h_drain_dma_window();
    return;
  }
  if (id != GBUS_C2H_ID_MAGIC) {
    std::fprintf(stderr, "[fpga-host] GBus C2H unknown ID offset=0x%llx id=0x%08x (expected GBS1/GBD1)\n",
                 static_cast<unsigned long long>(config_base_ + GBUS_REG_C2H_ID), id);
    running_.store(false);
    std::exit(EXIT_FAILURE);
  }
  const size_t packet_size = sizeof(FpgaPackgeHead);
  // The RTL stages up to GBUS_C2H_STAGE_WORDS words per fill, and a fill may end
  // when the FIFO runs dry, so a range can span several fills.  Keep the bytes
  // in a running accumulator and hand out whole 768-byte ranges.
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
    std::fprintf(stderr, "[fpga-host] GBus C2H SRAM staging window is absent status=0x%08x\n", status);
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
      const uint64_t now_ns =
          std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch())
              .count();
      if (c2h_idle_timeout_sec_ &&
          now_ns - c2h_last_progress_ns_ > static_cast<uint64_t>(c2h_idle_timeout_sec_) * 1000000000ULL) {
        std::fprintf(stderr, "[fpga-host] GBus C2H stalled reads=%llu bytes=%llu status=0x%08x\n",
                     static_cast<unsigned long long>(c2h_reads_), static_cast<unsigned long long>(c2h_bytes_), st);
        c2h_last_progress_ns_ = now_ns;
      }
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
      // The sender has data but the fill staged nothing.  One or two of these
      // are a harmless race against the control write landing; a long run of
      // them means the fill never actually starts, which used to show up only
      // as a 30-minute hang with no explanation.
      if (++c2h_empty_fills_ == 200) {
        std::fprintf(stderr,
                     "[fpga-host] GBus C2H staged no words on %llu consecutive fills while the sender has data "
                     "status=0x%08x; the control write is probably not reaching the FIFO\n",
                     static_cast<unsigned long long>(c2h_empty_fills_), st);
      }
      usleep(c2h_poll_us_);
      continue;
    }
    c2h_empty_fills_ = 0;
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
        burst_ok = gbus_read(prototyping_, board_, fpga_, config_instance_, config_base_ + offset, chunk, block) == 1 &&
                   block.size() == static_cast<size_t>(chunk) * 4;
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
      ++c2h_reads_;
      c2h_bytes_ += packet_size;
      c2h_last_progress_ns_ =
          std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch())
              .count();
      dprintf(STDERR_FILENO, "[fpga-host] GBus C2H progress reads=%llu bytes=%llu staged=%u\n",
              static_cast<unsigned long long>(c2h_reads_), static_cast<unsigned long long>(c2h_bytes_), words);
    }
  }
}

// -------------------------------------------------------- frozen SRAM DMA

void GbusTransport::c2h_drain_dma_window() {
  using Clock = std::chrono::steady_clock;
  const auto alive = [&]() { return running_.load() && signal_num == 0; };
  const auto pause = [&](uint32_t us) {
    const auto end = Clock::now() + std::chrono::microseconds(us);
    while (alive() && Clock::now() < end)
      std::this_thread::sleep_until(std::min(end, Clock::now() + std::chrono::microseconds(1000)));
  };
  // Report what the AXI read port was actually asked for.  Capture is by
  // reference into a caller-provided buffer so it stays usable from both the
  // progress path and the exit path.
  const auto ar_trace = [&](char *buf, size_t n) {
    const uint32_t addr = c2h_reg_read(GBUS_REG_C2H_AR_ADDR);
    const uint32_t attrs = c2h_reg_read(GBUS_REG_C2H_AR_ATTRS);
    const uint32_t count = c2h_reg_read(GBUS_REG_C2H_AR_COUNT);
    std::snprintf(buf, n, "ar_count=%u ar_addr=0x%08x ar_id=0x%02x ar_burst=%u ar_size=%u ar_len=%u", count, addr,
                  (attrs >> 24) & 0xffU, (attrs >> 22) & 0x3U, (attrs >> 19) & 0x7U, (attrs >> 15) & 0xfU);
  };
  const auto fail = [&](const char *reason, uint32_t st, uint32_t seq, size_t progress) {
    char ar_buf[160];
    ar_trace(ar_buf, sizeof(ar_buf));
    std::fprintf(stderr, "[fpga-host] GBus GBD1 %s status=0x%08x seq=%u captured=%zu reads=%llu bytes=%llu t=%llu %s\n",
                 reason, st, seq, progress, static_cast<unsigned long long>(c2h_reads_),
                 static_cast<unsigned long long>(c2h_bytes_), static_cast<unsigned long long>(monotonic_us()), ar_buf);
    running_.store(false);
    std::exit(EXIT_FAILURE); // Never release an uncertain window.
  };
  const auto check_status = [&](uint32_t st) {
    if (!(st & GBUS_C2H_STATUS_PRESENT) || (st & (GBUS_C2H_STATUS_FRAME_ERROR | GBUS_C2H_STATUS_PROTOCOL_ERROR)))
      fail("invalid status", st, 0, 0);
  };
  const uint64_t aperture = c2h_reg_read(GBUS_REG_C2H_DMA_BASE);
  const uint32_t capacity = c2h_reg_read(GBUS_REG_C2H_CAPACITY);
  const uint64_t configured_chunk = env_u64("GBUS_C2H_DMA_CHUNK_BYTES", 256);
  if (aperture != 0x10000000ULL || capacity != GBUS_C2H_DATA_BYTES || configured_chunk == 0 ||
      configured_chunk > capacity || configured_chunk % 32 != 0)
    fail("invalid aperture/capacity/chunk (chunk must be 32..1024, multiple of 32)", 0, 0, 0);
  // Do not rely on the vendor runtime to split a request into legal AXI3
  // bursts: each call is at most 16 full 256-bit beats (512 bytes).
  const size_t chunk_limit = std::min<uint64_t>(configured_chunk, 512);
  constexpr unsigned attempts = 3;
  constexpr uint32_t busy = GBUS_C2H_STATUS_FILLING | GBUS_C2H_STATUS_ACTIVE;
  std::vector<uint8_t> accumulator;
  accumulator.reserve(2 * sizeof(FpgaPackgeHead));
  bool have_sequence = false;
  uint32_t last_sequence = 0;
  auto idle_report = Clock::now();
  auto busy_deadline = Clock::now() + std::chrono::seconds(2);
  std::fprintf(stderr, "[fpga-host] GBus GBD1 frozen SRAM DMA base=0x%llx capacity=%u chunk=%zu attempts=%u\n",
               static_cast<unsigned long long>(aperture), capacity, chunk_limit, attempts);
  {
    char ar_buf[160];
    ar_trace(ar_buf, sizeof(ar_buf));
    std::fprintf(stderr, "[fpga-host] GBus GBD1 initial AXI read trace %s\n", ar_buf);
  }
  while (alive()) {
    uint32_t st = c2h_reg_read(GBUS_REG_C2H_STATUS);
    check_status(st);
    if (st & busy) {
      if (Clock::now() >= busy_deadline)
        fail("busy deadline exceeded", st, 0, 0);
      pause(c2h_poll_us_);
      continue;
    }
    busy_deadline = Clock::now() + std::chrono::seconds(2);
    if (!(st & GBUS_C2H_STATUS_FROZEN)) {
      if (!(st & GBUS_C2H_STATUS_HAS_DATA)) {
        if (c2h_idle_timeout_sec_ && Clock::now() - idle_report >= std::chrono::seconds(c2h_idle_timeout_sec_)) {
          std::fprintf(stderr, "[fpga-host] GBus GBD1 backpressure/idle status=0x%08x reads=%llu bytes=%llu\n", st,
                       static_cast<unsigned long long>(c2h_reads_), static_cast<unsigned long long>(c2h_bytes_));
          idle_report = Clock::now();
        }
        pause(c2h_poll_us_);
        continue;
      }
      if (!alive())
        return;
      c2h_reg_write(GBUS_REG_C2H_CTRL, GBUS_C2H_CTRL_START_FILL);
      const auto deadline = Clock::now() + std::chrono::seconds(2);
      // The write can complete before publication/filling is visible. Wait for
      // frozen, not merely !filling, without issuing a second fill request.
      do {
        if (!alive())
          return;
        st = c2h_reg_read(GBUS_REG_C2H_STATUS);
        check_status(st);
        if ((st & GBUS_C2H_STATUS_FROZEN) && !(st & busy))
          break;
        if (Clock::now() >= deadline)
          fail("fill deadline exceeded", st, 0, 0);
        pause(c2h_poll_us_);
      } while (alive());
    }
    if (!alive())
      return;
    const uint32_t seq = c2h_reg_read(GBUS_REG_C2H_SEQ);
    const size_t valid = ((st >> GBUS_C2H_STATUS_STAGED_SHIFT) & GBUS_C2H_STATUS_STAGED_MASK) * 4;
    if (!valid || valid > capacity || valid % 32 || (have_sequence && seq != last_sequence + 1U))
      fail("invalid published length/sequence", st, seq, 0);
    const auto stable = [&](uint32_t state, uint32_t sequence) {
      check_status(state);
      return sequence == seq && (state & GBUS_C2H_STATUS_FROZEN) && !(state & busy) &&
             (((state >> GBUS_C2H_STATUS_STAGED_SHIFT) & GBUS_C2H_STATUS_STAGED_MASK) * 4 == valid);
    };
    // Transactional capture: neither dispatch nor the cross-window accumulator
    // changes until every chunk and the post-read sequence/status are verified.
    std::vector<uint8_t> window;
    window.reserve(valid);
    for (size_t offset = 0; offset < valid && alive();) {
      const size_t size = std::min(chunk_limit, valid - offset);
      bool captured = false;
      const auto deadline = Clock::now() + std::chrono::seconds(10);
      for (unsigned attempt = 0; attempt < attempts && alive(); ++attempt) {
        std::vector<uint8_t> block;
        const uint64_t begin = monotonic_us();
        const bool trace = c2h_reads_ < 2 || attempt != 0;
        if (trace)
          std::fprintf(stderr,
                       "[fpga-host] GBus GBD1 DMA begin offset=0x%llx size=%zu seq=%u attempt=%u progress=%zu t=%llu\n",
                       static_cast<unsigned long long>(aperture + offset), size, seq, attempt + 1, offset,
                       static_cast<unsigned long long>(begin));
        const int rc = gbus_dma_read(prototyping_, board_, dma_fpga_, ddr_instance_, aperture + offset, size, channel_,
                                     port_, block);
        const uint64_t end = monotonic_us();
        if (trace || rc != 1 || block.size() != size)
          std::fprintf(stderr,
                       "[fpga-host] GBus GBD1 DMA end rc=%d offset=0x%llx size=%zu actual=%zu seq=%u attempt=%u "
                       "begin=%llu end=%llu progress=%zu/%zu\n",
                       rc, static_cast<unsigned long long>(aperture + offset), size, block.size(), seq, attempt + 1,
                       static_cast<unsigned long long>(begin), static_cast<unsigned long long>(end), offset, valid);
        if (!alive())
          return;
        // Runtime completion may precede the AXI-active flag crossing domains.
        do {
          st = c2h_reg_read(GBUS_REG_C2H_STATUS);
          check_status(st);
          if (!(st & GBUS_C2H_STATUS_ACTIVE))
            break;
          if (Clock::now() >= deadline)
            fail("DMA active deadline exceeded", st, seq, offset);
          pause(c2h_poll_us_);
        } while (alive());
        if (!alive())
          return;
        if (!stable(st, c2h_reg_read(GBUS_REG_C2H_SEQ)))
          fail("window changed during DMA (no ACK)", st, seq, offset);
        if (Clock::now() >= deadline)
          fail("DMA deadline exceeded (no ACK)", st, seq, offset);
        if (rc == 1 && block.size() == size) {
          window.insert(window.end(), block.begin(), block.end());
          captured = true;
          break;
        }
        // Read is nondestructive. Retry the SAME offset and size, never append
        // a failed/short/oversized vector or advance the byte position.
        pause(c2h_poll_us_);
      }
      if (!alive())
        return;
      if (!captured)
        fail("DMA retries exhausted (no ACK)", st, seq, offset);
      offset += size;
    }
    if (!alive())
      return;
    const uint32_t final_seq = c2h_reg_read(GBUS_REG_C2H_SEQ);
    st = c2h_reg_read(GBUS_REG_C2H_STATUS);
    if (!stable(st, final_seq))
      fail("pre-ACK validation failed", st, seq, window.size());
    if (!alive())
      return;
    c2h_reg_write(GBUS_REG_C2H_CTRL, GBUS_C2H_CTRL_DRAIN); // GBD1 ACK/release
    const auto ack_deadline = Clock::now() + std::chrono::seconds(2);
    do {
      if (!alive())
        return;
      st = c2h_reg_read(GBUS_REG_C2H_STATUS);
      check_status(st);
      if (!(st & (GBUS_C2H_STATUS_FROZEN | busy)))
        break;
      if (Clock::now() >= ack_deadline)
        fail("ACK deadline exceeded", st, seq, window.size());
      pause(c2h_poll_us_);
    } while (alive());
    if (!alive())
      return;
    last_sequence = seq;
    have_sequence = true;
    accumulator.insert(accumulator.end(), window.begin(), window.end());
    while (accumulator.size() >= sizeof(FpgaPackgeHead) && alive()) {
      c2h_dispatch_range(accumulator.data(), sizeof(FpgaPackgeHead));
      accumulator.erase(accumulator.begin(), accumulator.begin() + sizeof(FpgaPackgeHead));
      ++c2h_reads_;
      c2h_bytes_ += sizeof(FpgaPackgeHead);
      // Bounded-rate progress rather than one line per production packet.
      if (c2h_reads_ <= 2 || (c2h_reads_ & (c2h_reads_ - 1)) == 0) {
        char ar_buf[160];
        ar_trace(ar_buf, sizeof(ar_buf));
        std::fprintf(stderr, "[fpga-host] GBus GBD1 progress reads=%llu bytes=%llu seq=%u %s\n",
                     static_cast<unsigned long long>(c2h_reads_), static_cast<unsigned long long>(c2h_bytes_), seq,
                     ar_buf);
      }
    }
    idle_report = Clock::now();
  }
}

void GbusTransport::stop() {
  running_.store(false);
}

void GbusTransport::fpga_io(uint64_t address, uint32_t value) {
  if (!initialized_)
    std::exit(EXIT_FAILURE);
  dprintf(STDERR_FILENO, "[fpga-host] GBus config write begin addr=0x%llx value=0x%x t=%llu\n",
          static_cast<unsigned long long>(config_base_ + address), value,
          static_cast<unsigned long long>(monotonic_ms()));
  auto data = store_le32(value);
  const int rc = gbus_write(prototyping_, board_, fpga_, config_instance_, config_base_ + address, 1, data);
  dprintf(STDERR_FILENO, "[fpga-host] GBus config write end addr=0x%llx rc=%d t=%llu\n",
          static_cast<unsigned long long>(config_base_ + address), rc, static_cast<unsigned long long>(monotonic_ms()));
  if (rc != 1) {
    dprintf(STDERR_FILENO, "[fpga-host] GBus register write failed offset=0x%llx\n",
            static_cast<unsigned long long>(address));
    std::exit(EXIT_FAILURE);
  }
  if (config_readback_) {
    // CFG_RESET is implemented by XDMAConfigBar as a short self-clearing
    // reset request, so its readback is intentionally informational.  A
    // successful read through the same GBus window is nevertheless essential:
    // it proves the GeneralBD -> AXI-Lite bridge and CDC returned data from
    // the generated register block rather than merely acknowledging a bus
    // transaction at the transport boundary.
    usleep(address == HOST_IO_CFG_RESET && value ? 10000 : 1000);
    const uint32_t readback = fpga_io_read(address);
    std::fprintf(stderr, "[fpga-host] GBus config readback addr=0x%llx write=0x%08x read=0x%08x%s\n",
                 static_cast<unsigned long long>(config_base_ + address), value, readback,
                 (address == HOST_IO_CFG_RESET && value)
                     ? " (self-clearing reset request)"
                     : (readback == value ? " (match)" : " (mismatch; register may be status/edge-triggered)"));
  }
}

uint32_t GbusTransport::fpga_io_read(uint64_t address) {
  if (!initialized_)
    std::exit(EXIT_FAILURE);
  std::vector<uint8_t> data;
  if (gbus_read(prototyping_, board_, fpga_, config_instance_, config_base_ + address, 1, data) != 1) {
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
    // GBus writes the workload directly through the UVHS DDR AXI master; it
    // does not feed DifftestMemCtrl's AXI-stream H2C engine.  Consequently no
    // hardware completion status changes HOST_IO_MEM_H2C from 1 to 2 as it
    // does in the XDMA flow.  Clear the request here after the synchronous
    // gbus_dma_write() has completed so the following HOST_IO_MEM_CPU write
    // can return DDR ownership to the CPU.  Leaving bit 0 set permanently
    // selects an idle H2C master over the CPU and prevents instruction fetch.
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
  const auto *bytes = static_cast<const uint8_t *>(payload);
  constexpr uint64_t chunk = 64ULL * 1024ULL * 1024ULL;
  for (uint64_t offset = 0; offset < size; offset += chunk) {
    size_t count = static_cast<size_t>((size - offset) < chunk ? (size - offset) : chunk);
    std::vector<uint8_t> data(bytes + offset, bytes + offset + count);
    if (gbus_dma_write(prototyping_, board_, dma_fpga_, ddr_instance_, ddr_base_ + offset, count, channel_, port_,
                       data) != 1) {
      std::fprintf(stderr, "[fpga-host] GBus DMA workload write failed offset=0x%llx size=%zu\n",
                   static_cast<unsigned long long>(offset), count);
      std::exit(EXIT_FAILURE);
    }
  }
  std::fprintf(stderr, "[fpga-host] GBus DMA workload queued %llu bytes\n", static_cast<unsigned long long>(size));
}

void GbusTransport::validate_guest_ram(uint64_t, uint64_t) const {}
