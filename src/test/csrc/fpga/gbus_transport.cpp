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

namespace {
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
  c2h_ring_base_ = env_u64("GBUS_C2H_RING_BASE", 0x81000000ULL);
  c2h_dma_base_ =
      env_u64("GBUS_C2H_DMA_BASE", c2h_ring_base_ >= 0x80000000ULL ? c2h_ring_base_ - 0x80000000ULL : c2h_ring_base_);
  // The usable ring is one 256-byte tail shorter than 16 MiB so it is an
  // exact multiple of a 768-byte FpgaPackgeHead.  No DMA read can straddle the
  // physical ring boundary.
  c2h_ring_size_ = env_u64("GBUS_C2H_RING_SIZE", 0x00ffff00ULL);
  // GENERALBD uses a 0x1000 config window for the DiffTest BAR and a
  // 0x1100 window for the C2H ring.  The RTL decodes the latter internally.
  c2h_wptr_offset_ = env_u64("GBUS_C2H_WPTR_OFFSET", 0x0108ULL);
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
  dprintf(STDERR_FILENO, "[fpga-host] GBus start ring parameters base=0x%llx size=%llu wptr=0x%llx packet=%zu\n",
          static_cast<unsigned long long>(c2h_ring_base_), static_cast<unsigned long long>(c2h_ring_size_),
          static_cast<unsigned long long>(c2h_wptr_offset_), sizeof(FpgaPackgeHead));
  if (!enable_diff) {
    while (running_.load() && signal_num == 0)
      usleep(10000);
    return;
  }
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
  if (hardware_base != static_cast<uint32_t>(c2h_ring_base_) ||
      hardware_size != static_cast<uint32_t>(c2h_ring_size_) || (hardware_status & 1U) == 0) {
    dprintf(STDERR_FILENO,
            "[fpga-host] GBus C2H ABI mismatch hw_base=0x%x sw_base=0x%llx hw_size=0x%x sw_size=0x%llx status=0x%x\n",
            hardware_base, static_cast<unsigned long long>(c2h_ring_base_), hardware_size,
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
  c2h_last_progress_ns_ =
      std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
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
    const uint32_t available = write_ptr - read_ptr;
    if (available > c2h_ring_size_) {
      dprintf(STDERR_FILENO, "[fpga-host] GBus C2H overflow producer=0x%x consumer=0x%x available=%u ring=%llu\n",
              write_ptr, read_ptr, available, static_cast<unsigned long long>(c2h_ring_size_));
      running_.store(false);
      std::exit(EXIT_FAILURE);
    }
    while (static_cast<uint32_t>(write_ptr - read_ptr) >= packet_size && running_.load() && signal_num == 0) {
      const uint64_t offset = c2h_dma_base_ + (read_ptr % c2h_ring_size_);
      std::vector<uint8_t> packet;
      if (gbus_dma_read(prototyping_, board_, dma_fpga_, ddr_instance_, offset, packet_size, channel_, port_, packet) !=
              1 ||
          packet.size() < packet_size) {
        dprintf(STDERR_FILENO, "[fpga-host] GBus C2H packet read failed offset=0x%llx\n",
                static_cast<unsigned long long>(offset));
        std::exit(EXIT_FAILURE);
      }
      dump_gbus_packet(packet, c2h_reads_);
      auto *head = reinterpret_cast<const FpgaPackgeHead *>(packet.data());
      for (size_t i = 0; i < DMA_PACKGE_NUM; ++i) {
        auto *payload_ptr = const_cast<uint8_t *>(head->diff_packge[i].diff_packge);
        v_difftest_Batch(payload_ptr);
      }
      read_ptr += static_cast<uint32_t>(packet_size);
      ++c2h_reads_;
      c2h_bytes_ += packet_size;
      c2h_last_progress_ns_ =
          std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch())
              .count();
      dprintf(STDERR_FILENO, "[fpga-host] GBus C2H progress reads=%llu bytes=%llu read_ptr=0x%llx\n",
              static_cast<unsigned long long>(c2h_reads_), static_cast<unsigned long long>(c2h_bytes_),
              static_cast<unsigned long long>(read_ptr));
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

void GbusTransport::validate_guest_ram(uint64_t base, uint64_t size) const {
  if (size > c2h_ring_base_ - base) {
    std::fprintf(stderr, "[fpga-host] guest RAM [0x%llx,0x%llx) overlaps reserved GBus C2H ring at 0x%llx\n",
                 static_cast<unsigned long long>(base), static_cast<unsigned long long>(base + size),
                 static_cast<unsigned long long>(c2h_ring_base_));
    std::exit(EXIT_FAILURE);
  }
}
