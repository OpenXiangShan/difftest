#ifndef FPGA_GBUS_TRANSPORT_H
#define FPGA_GBUS_TRANSPORT_H

#include "fpga_transport.h"
#include <atomic>
#include <cstdint>
#include <cstring>
#include <string>

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
  uint32_t c2h_reg_read(uint64_t offset);
  void c2h_reg_write(uint64_t offset, uint32_t value);
  bool c2h_probe_multiword_read(uint32_t words, uint64_t *elapsed_us);
  void c2h_drain_sram_fifo();
  void c2h_drain_dma_window();
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
  // GBS1 uses register reads; GBD1 uses a frozen SRAM DMA aperture with ACK.
  // DMA chunks are 32-byte aligned, default to 256 bytes, and are capped at
  // 512 bytes per call. Neither interface reserves guest RAM.
  // Words requested per gbus_read call when draining the staging window.  This
  // is resolved at run time from a read-only probe of the config BAR, because
  // the runtime's contract for count > 1 was never verified on this platform.
  uint32_t c2h_burst_words_ = 1;
  // Consecutive fills that staged nothing while the sender had data.  A short
  // run of these is a race with the control write; a long one means the write
  // never lands, and it should be reported rather than spun on.
  uint64_t c2h_empty_fills_ = 0;
  uint32_t c2h_poll_us_ = 1000;
  uint32_t c2h_idle_timeout_sec_ = 30;
  bool config_readback_ = false;
  std::atomic<bool> running_{false};
  uint64_t c2h_reads_ = 0;
  uint64_t c2h_bytes_ = 0;
  uint64_t c2h_last_progress_ns_ = 0;
  bool initialized_ = false;
  std::string host_;
};

#endif
