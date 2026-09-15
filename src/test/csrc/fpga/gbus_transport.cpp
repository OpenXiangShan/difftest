#include "gbus_transport.h"
#include "difftest-dpic.h"
#include "xdma.h"
#include <algorithm>
#include <cerrno>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
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
constexpr uint64_t GBUS_REG_C2H_DATA = 0x2000;
constexpr uint32_t GBUS_C2H_STAGE_WORDS = 256;
constexpr uint32_t GBUS_C2H_DATA_BYTES = GBUS_C2H_STAGE_WORDS * 4;

constexpr uint32_t GBUS_C2H_STATUS_PRESENT = 1U << 31;
constexpr uint32_t GBUS_C2H_STATUS_FRAME_ERROR = 1U << 30;
constexpr uint32_t GBUS_C2H_STATUS_STAGED_SHIFT = 8;
constexpr uint32_t GBUS_C2H_STATUS_STAGED_MASK = 0x1ffU;
constexpr uint32_t GBUS_C2H_STATUS_FILLING = 1U << 7;
constexpr uint32_t GBUS_C2H_STATUS_HAS_DATA = 1U << 6;

constexpr uint32_t GBUS_C2H_CTRL_START_FILL = 1U << 0;

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
  c2h_sram_ = env_u64("GBUS_C2H_SRAM", 1) != 0;
  // 0 means "use the largest burst the runtime accepts", resolved by the probe
  // below.  Set GBUS_C2H_BURST_WORDS=1 to force one 32-bit read per call.
  c2h_burst_words_ = static_cast<uint32_t>(env_u64("GBUS_C2H_BURST_WORDS", 0));
  c2h_poll_us_ = static_cast<uint32_t>(env_u64("GBUS_C2H_POLL_US", 1000));
  c2h_idle_timeout_sec_ = static_cast<uint32_t>(env_u64("GBUS_C2H_IDLE_TIMEOUT_SEC", 30));
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
    std::fprintf(stderr, "[fpga-host] GBus C2H interface layer=%s\n", c2h_sram_ ? "sram-window" : "ddr-ring");
  }
}

GbusTransport::~GbusTransport() {
  if (initialized_)
    gbus_finalize();
}

void GbusTransport::start(bool enable_diff) {
  running_.store(true);
  if (!enable_diff) {
    while (running_.load() && signal_num == 0)
      usleep(10000);
    return;
  }
  c2h_last_progress_ns_ =
      std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
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
  if (packet_size % sizeof(DmaDiffPackge) != 0) {
    std::fprintf(stderr, "[fpga-host] GBus C2H incomplete record: bytes=%zu\n", packet_size);
    std::exit(EXIT_FAILURE);
  }
  for (size_t offset = 0; offset < packet_size && running_.load() && signal_num == 0; offset += sizeof(DmaDiffPackge)) {
    v_difftest_Batch(range.data() + offset + offsetof(DmaDiffPackge, diff_packge));
  }
}

void GbusTransport::c2h_drain_sram_fifo() {
  const uint32_t id = c2h_reg_read(GBUS_REG_C2H_ID);
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
      if (c2h_reads_ <= 2 || (c2h_reads_ & (c2h_reads_ - 1)) == 0) {
        std::fprintf(stderr, "[fpga-host] GBus C2H progress reads=%llu bytes=%llu staged=%u\n",
                     static_cast<unsigned long long>(c2h_reads_), static_cast<unsigned long long>(c2h_bytes_), words);
      }
    }
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
      ++c2h_reads_;
      c2h_bytes_ += packet_size;
      c2h_last_progress_ns_ =
          std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch())
              .count();
    }
    usleep(c2h_poll_us_);
    const uint64_t now_ns =
        std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch())
            .count();
    if (c2h_idle_timeout_sec_ &&
        now_ns - c2h_last_progress_ns_ > static_cast<uint64_t>(c2h_idle_timeout_sec_) * 1000000000ULL) {
      dprintf(STDERR_FILENO, "[fpga-host] GBus C2H stalled reads=%llu bytes=%llu write_ptr=0x%llx\n",
              static_cast<unsigned long long>(c2h_reads_), static_cast<unsigned long long>(c2h_bytes_),
              static_cast<unsigned long long>(write_ptr));
      c2h_last_progress_ns_ = now_ns;
    }
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
  // Keep each request within the transfer size verified by the UVHS GBus runtime.
  constexpr uint64_t chunk = 256ULL;
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

void GbusTransport::validate_guest_ram(uint64_t base, uint64_t size) const {
  // The SRAM staging window is on-chip, so it reserves no guest address space
  // and the CPU may use the whole DDR.  Only the legacy DDR ring needs a
  // reservation carved out of guest RAM.
  if (c2h_sram_)
    return;
  if (size > c2h_ring_base_ - base) {
    std::fprintf(stderr, "[fpga-host] guest RAM [0x%llx,0x%llx) overlaps reserved GBus C2H ring at 0x%llx\n",
                 static_cast<unsigned long long>(base), static_cast<unsigned long long>(base + size),
                 static_cast<unsigned long long>(c2h_ring_base_));
    std::exit(EXIT_FAILURE);
  }
}
