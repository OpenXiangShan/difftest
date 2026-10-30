#ifndef FPGA_GBUS_TRANSPORT_H
#define FPGA_GBUS_TRANSPORT_H

#include "fpga_transport.h"
#include <atomic>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

class GbusTransport final : public FpgaTransport {
public:
  GbusTransport();
  ~GbusTransport() override;

  void start(bool enable_diff) override;
  void stop() override;
  void fpga_io(uint64_t address, uint32_t value) override;
  uint32_t fpga_io_read(uint64_t address) override;
  void wait_fpga_io_done(uint64_t address, const char *tag) override;
  void h2c_load_workload(const void *payload, uint64_t size) override;
  void validate_guest_ram(uint64_t base, uint64_t size) const;

private:
  struct C2hRegs {
    uint32_t status = 0;
    uint32_t seq = 0;
    uint32_t ar_addr = 0;
    uint32_t ar_attrs = 0;
    uint32_t ar_count = 0;
  };

  uint32_t c2h_reg_read(uint64_t offset);
  void c2h_reg_write(uint64_t offset, uint32_t value);
  void c2h_read_block(uint64_t offset, uint32_t words, std::vector<uint32_t> &out);
  void c2h_read_capture_regs(C2hRegs &regs);
  void c2h_read_stable_regs(uint32_t &status, uint32_t &seq);
  void c2h_read_fill_evidence(uint32_t &ctrl_count, uint32_t &ctrl_data, uint32_t &bad_ctrl);
  bool c2h_probe_multiword_read(uint32_t words, uint64_t *elapsed_us);
  bool c2h_probe_gbd1_block_reads();
  bool c2h_probe_aperture_header();
  bool c2h_probe_stream();
  void c2h_drain_stream_frames();
  void c2h_drain_sram_fifo();
  void c2h_drain_dma_window();
  void c2h_drain_ddr_ring();
  void c2h_dispatch_range(const uint8_t *packet, size_t packet_size);

  uint8_t prototyping_ = 0;
  uint8_t board_ = 0;
  uint8_t fpga_ = 0;
  // Config/GENERALBD is hosted on the GBus FPGA.  A multi-FPGA XiangShan
  // trace topology may place the functional DDR on another FPGA, so DMA
  // requests can target a separate UVHS FPGA while register accesses remain
  // on fpga_.  Defaults to fpga_ for the single-FPGA NutShell topology.
  uint8_t dma_fpga_ = 0;
  uint8_t config_instance_ = 0;
  uint8_t ddr_instance_ = 0;
  uint8_t channel_ = 0;
  uint8_t port_ = 0;
  // GBus register access is a windowed operation.  Unlike XDMA BAR0, the
  // UVHS runtime rejects offsets below 0x1000; the RTL register map remains
  // unchanged and is reached through this configurable window base.
  uint64_t config_base_ = 0x1000;
  uint64_t ddr_base_ = 0;
  uint64_t c2h_ring_base_ = 0;
  uint64_t c2h_dma_base_ = 0;
  uint64_t c2h_ring_size_ = 0;
  uint64_t c2h_wptr_offset_ = 0;
  // The on-chip GBS1 register window is the default C2H interface.
  // GBUS_C2H_SRAM=0 selects the legacy DDR ring.
  bool c2h_sram_ = true;
  bool c2h_axi_dma_ = false;
  bool c2h_verify_first_bank_ = true;
  // Read the workload back after H2C to catch a silently dropped write.
  // Enabled by default; set GBUS_H2C_VERIFY=0 to skip the extra DMA reads.
  bool h2c_verify_ = true;
  // Bytes per gbus_dma_read when draining a frozen bank.  The 256-byte default
  // is the size verified on the board; larger requests (up to the 1024-byte
  // bank) are expected to arrive as consecutive AXI3 bursts, but were not yet
  // observed on hardware, so they stay opt-in until a board round confirms the
  // multi-burst behavior.  The last-AR evidence check still pins the request
  // end for every accepted chunk size.
  uint32_t c2h_dma_chunk_bytes_ = 256;
  // Payload bytes per gbus_dma_write when loading the H2C image.  Same rule:
  // 256 is verified, larger sizes are opt-in until board-validated.
  uint32_t h2c_chunk_bytes_ = 256;
  // Opt-in timing summary for board A/B measurements; off by default so a
  // normal run keeps its existing output.
  bool timing_ = false;
  // GBD1 register reads can be grouped when the runtime honours a multi-word
  // count.  The probe validates that against the config BAR; a runtime that
  // ignores the count falls back to the verified single-word path.
  bool c2h_block_reads_ = false;
  // Re-issue the startup FILL as soon as the control evidence proves it was
  // consumed by the stream-reset window, instead of waiting for the full
  // deadline.  Disabled by default until a board round validates it.
  bool c2h_fast_fill_ = false;
  // GBD1 snapshot header: the first 64 aperture bytes carry the frozen bank
  // metadata and the AR evidence of the read carrying them, so one DMA read
  // replaces the per-field register polls.  Requires RTL support (register
  // 0x1238) and is negotiated at startup.
  bool c2h_aperture_header_ = false;
  bool c2h_aperture_header_ready_ = false;
  // Ping-pong capture: the RTL keeps filling the other bank by itself, so the
  // host stops issuing FILL and only waits for a frozen bank.
  bool c2h_pingpong_ = false;
  bool c2h_pingpong_ready_ = false;
  // Streaming frame FIFO: the RTL appends every complete 768-byte frame to a
  // ring and exposes only a frame counter, so the host reads a whole batch per
  // DMA call and releases it with one CONSUME write.  Negotiated from feature
  // bit 2, so an older bitstream keeps using the frozen-bank protocol.
  bool c2h_stream_ = true;
  bool c2h_stream_ready_ = false;
  uint32_t c2h_stream_frames_ = 0;
  uint32_t c2h_stream_frame_bytes_ = 0;
  uint32_t c2h_stream_min_frames_ = 0;
  // True when the operator pinned the threshold instead of letting it
  // default to the ring depth.
  bool c2h_stream_min_frames_set_ = false;
  uint32_t c2h_header_bytes_ = 0;
  uint64_t c2h_reg_calls_ = 0;
  uint64_t c2h_reg_us_ = 0;
  uint64_t c2h_block_calls_ = 0;
  uint64_t c2h_block_us_ = 0;
  uint64_t c2h_write_calls_ = 0;
  uint64_t c2h_write_us_ = 0;
  uint64_t c2h_dma_calls_ = 0;
  uint64_t c2h_dma_us_ = 0;
  uint64_t c2h_dispatch_us_ = 0;
  uint32_t c2h_idle_timeout_sec_ = 30;
  // Multi-word reads require explicit selection on unverified runtime releases.
  uint32_t c2h_burst_words_ = 1;
  // The status reads themselves pace the poll loop, so only a short delay is
  // needed when a read reports that the expected state has not arrived yet.
  uint32_t c2h_poll_us_ = 100;
  std::atomic<bool> running_{false};
  bool initialized_ = false;
  std::string host_;
};

#endif
