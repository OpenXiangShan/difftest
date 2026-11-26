#ifndef FPGA_GBUS_TRANSPORT_H
#define FPGA_GBUS_TRANSPORT_H

#include "fpga_transport.h"
#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <cstring>
#include <deque>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

// GBus DiffTest transport.  The GBD1 streaming frame FIFO is the only C2H
// path: the RTL appends each complete 768-byte DiffTest frame to a ring and
// the host drains it with AXI DMA reads, releasing frames with CONSUME.
// The register-window (GBS1) and DDR-ring transports were removed.
class GbusC2hHelper;

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
  // The streaming ring lives in the reserved top of port 0, outside guest RAM,
  // so this only documents the contract and never rejects a guest range.
  void validate_guest_ram(uint64_t base, uint64_t size) const;

private:
  uint32_t c2h_reg_read(uint64_t offset);
  void c2h_reg_write(uint64_t offset, uint32_t value);
  void c2h_read_block(uint64_t offset, uint32_t words, std::vector<uint32_t> &out);
  void c2h_probe_stream();
  void c2h_drain_stream_frames();
  void c2h_dispatch_range(const uint8_t *packet, size_t packet_size);
  void c2h_dispatch_worker();
  void c2h_submit_batch(std::vector<uint8_t> &&payload);
  void c2h_finish_dispatch();

  uint8_t prototyping_ = 0;
  uint8_t board_ = 0;
  uint8_t fpga_ = 0;
  // Config/GENERALBD is hosted on the GBus FPGA.  The two DMA directions can
  // live on different FPGAs: H2C writes the workload into the functional CPU
  // DDR, while C2H reads the streaming ring that sits beside the GBus host.
  // On the 20261106 XiangShan build the DDR controller is on b0.f0 and the
  // GBus host (with the ring) on b0.f2, so one shared index cannot serve both.
  // Each defaults to GBUS_DMA_FPGA, then to fpga_, which is the single-FPGA
  // topology where every endpoint is on the same device.
  uint8_t h2c_dma_fpga_ = 0;
  uint8_t c2h_dma_fpga_ = 0;
  uint8_t config_instance_ = 0;
  uint8_t ddr_instance_ = 0;
  uint8_t channel_ = 0;
  uint8_t port_ = 0;
  // GBus register access is a windowed operation.  Unlike XDMA BAR0, the
  // UVHS runtime rejects offsets below 0x1000; the RTL register map remains
  // unchanged and is reached through this configurable window base.
  uint64_t config_base_ = 0x1000;
  uint64_t ddr_base_ = 0;
  // Read the workload back after H2C to catch a silently dropped write.
  // Enabled by default; set GBUS_H2C_VERIFY=0 to skip the extra DMA reads.
  bool h2c_verify_ = true;
  // Cross-check the first streamed frame against the config-window mirror.
  bool c2h_verify_first_bank_ = true;
  // Confirm each CONSUME by reading the ring read pointer back.  Set
  // GBUS_C2H_CONSUME_VERIFY=0 to drop that read and save one runtime round trip
  // per batch when the drain, not the check, is the bottleneck.
  bool c2h_consume_verify_ = true;
  // Payload bytes per gbus_dma_write when loading the H2C image.
  uint32_t h2c_chunk_bytes_ = 256;
  // Opt-in timing summary for board A/B measurements; off by default so a
  // normal run keeps its existing output.
  bool timing_ = false;
  // Streaming frame FIFO geometry, read from the GBD1 registers at startup.
  uint32_t c2h_stream_frames_ = 0;
  uint32_t c2h_stream_frame_bytes_ = 0;
  uint32_t c2h_stream_min_frames_ = 0;
  uint64_t c2h_reg_calls_ = 0;
  uint64_t c2h_reg_us_ = 0;
  uint64_t c2h_block_calls_ = 0;
  uint64_t c2h_block_us_ = 0;
  uint64_t c2h_write_calls_ = 0;
  uint64_t c2h_write_us_ = 0;
  uint64_t c2h_dma_calls_ = 0;
  uint64_t c2h_dma_us_ = 0;
  // Reference execution runs on its own thread so it overlaps the next DMA
  // call.  Buffers are handed over by move, and the queue is bounded so a
  // dispatch slower than the ring applies backpressure instead of growing.
  std::thread c2h_dispatch_thread_;
  std::mutex c2h_dispatch_mutex_;
  std::condition_variable c2h_dispatch_cv_;
  std::deque<std::vector<uint8_t>> c2h_dispatch_queue_;
  size_t c2h_dispatch_queue_limit_ = 4;
  bool c2h_dispatch_stop_ = false;
  // fpga_nstep runs on the dispatch thread when a batch carries a trap, and it
  // writes the ILA and link-enable words.  Those are GBus socket transactions on
  // the same connection the drain loop is using, so every runtime call goes
  // through this lock.  Recursive because fpga_io reads back what it wrote.
  std::recursive_mutex runtime_mutex_;
  std::atomic<uint64_t> c2h_dispatch_us_{0};
  uint32_t c2h_idle_timeout_sec_ = 30;
  // The status reads themselves pace the poll loop, so only a short delay is
  // needed when a read reports that the expected state has not arrived yet.
  uint32_t c2h_poll_us_ = 100;
  // A second connection in a child process, used to split each batch and run
  // two DMA reads at once.  Null when the child could not start, in which case
  // the drain loop stays single-stream.
  GbusC2hHelper *c2h_helper_ = nullptr;
  uint32_t c2h_helper_min_frames_ = 0;
  std::atomic<bool> running_{false};
  bool initialized_ = false;
  std::string host_;
};

// Entry point for the child process that carries the second DMA stream.  The
// transport re-executes its own binary with argv[1] == "--gbus-c2h-helper", so
// the child starts with a fresh runtime connection instead of an inherited
// socket.  Declared here because main() has to intercept the flag before the
// normal argument parser sees it.
int gbus_c2h_helper_main(int argc, const char *argv[]);

#endif
