#include "gbus_transport.h"
#include "xdma.h"

#include <uvaps_gbus_runtime.h>

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <utility>
#include <vector>

int signal_num = 0;

namespace {
struct RegisterWrite {
  uint64_t offset;
  uint32_t value;
};

std::vector<RegisterWrite> register_writes;
uint64_t dma_write_offset = 0;
size_t dma_write_size = 0;
uint64_t dma_read_offset = 0;
size_t dma_read_size = 0;
unsigned producer_reads = 0;
unsigned batch_calls = 0;
bool finalized = false;

uint32_t load_le32(const std::vector<uint8_t> &data) {
  return static_cast<uint32_t>(data.at(0)) |
         (static_cast<uint32_t>(data.at(1)) << 8) |
         (static_cast<uint32_t>(data.at(2)) << 16) |
         (static_cast<uint32_t>(data.at(3)) << 24);
}

void store_le32(std::vector<uint8_t> &data, uint32_t value) {
  data = {static_cast<uint8_t>(value), static_cast<uint8_t>(value >> 8),
          static_cast<uint8_t>(value >> 16), static_cast<uint8_t>(value >> 24)};
}

bool saw_adjacent_writes(uint64_t first_offset, uint32_t first_value,
                         uint64_t second_offset, uint32_t second_value) {
  for (size_t i = 1; i < register_writes.size(); ++i) {
    if (register_writes[i - 1].offset == first_offset &&
        register_writes[i - 1].value == first_value &&
        register_writes[i].offset == second_offset &&
        register_writes[i].value == second_value) {
      return true;
    }
  }
  return false;
}

int fail(const char *message) {
  std::fprintf(stderr, "GBUS_TRANSPORT_MOCK_FAIL: %s\n", message);
  return 1;
}
} // namespace

extern "C" void v_difftest_Batch(uint8_t io[CONFIG_DIFFTEST_BATCH_BYTELEN]) {
  if (io[0] != static_cast<uint8_t>(batch_calls)) {
    std::fprintf(stderr, "unexpected batch marker %u at call %u\n", io[0], batch_calls);
    signal_num = 2;
    return;
  }
  ++batch_calls;
  if (batch_calls == DMA_PACKGE_NUM) signal_num = 1;
}

bool gbus_initialize(const char *host) { return host && std::string(host) == "mock-host"; }
bool gbus_finalize() {
  finalized = true;
  return true;
}

int gbus_write(uint8_t, uint8_t, uint8_t, uint8_t, uint64_t offset, size_t count,
               std::vector<uint8_t> &value) {
  if (count != 1 || value.size() < 4) return 0;
  register_writes.push_back({offset, load_le32(value)});
  return 1;
}

int gbus_read(uint8_t, uint8_t, uint8_t, uint8_t, uint64_t offset, size_t count,
              std::vector<uint8_t> &value) {
  if (count != 1) return 0;
  uint32_t result = 0;
  switch (offset) {
    // The control ABI reports the CPU-visible reservation; DMA uses the
    // corresponding 0x01000000 offset when fetching ring packets.
    case 0x1100: result = 0x81000000U; break;
    case 0x1104: result = 0x00ffff00U; break;
    case 0x1108: result = producer_reads++ == 0 ? 0U : sizeof(FpgaPackgeHead); break;
    case 0x110c: result = 1U; break;
    default: result = 0U; break;
  }
  store_le32(value, result);
  return 1;
}

int gbus_dma_write(uint8_t, uint8_t, uint8_t, uint8_t, uint64_t offset, size_t size,
                   uint8_t, uint8_t, std::vector<uint8_t> &value) {
  if (value.size() != size) return 0;
  dma_write_offset = offset;
  dma_write_size = size;
  return 1;
}

int gbus_dma_read(uint8_t, uint8_t, uint8_t, uint8_t, uint64_t offset, size_t size,
                  uint8_t, uint8_t, std::vector<uint8_t> &value) {
  dma_read_offset = offset;
  dma_read_size = size;
  value.assign(size, 0);
  auto *packet = reinterpret_cast<FpgaPackgeHead *>(value.data());
  for (unsigned i = 0; i < DMA_PACKGE_NUM; ++i) packet->diff_packge[i].diff_packge[0] = i;
  return 1;
}

int main() {
  setenv("GBUS_HOST", "mock-host", 1);
  setenv("GBUS_C2H_POLL_US", "1", 1);
  setenv("GBUS_C2H_IDLE_TIMEOUT_SEC", "0", 1);

  {
    GbusTransport transport;
    std::vector<uint8_t> workload(1024 * 1024, 0x5a);
    transport.fpga_io(HOST_IO_H2C_SIZE_MB, 1);
    transport.fpga_io(HOST_IO_MEM_H2C, 1);
    transport.h2c_load_workload(workload.data(), workload.size());
    transport.wait_fpga_io_done(HOST_IO_MEM_H2C, "mock H2C");
    transport.fpga_io(HOST_IO_MEM_CPU, 1);

    if (dma_write_offset != 0 || dma_write_size != workload.size())
      return fail("DDR H2C offset/size mismatch");
    if (!saw_adjacent_writes(0x1028, 0, 0x1024, 1))
      return fail("MEM_H2C was not released immediately before MEM_CPU ownership");

    transport.start(true);
    if (signal_num != 1) return fail("C2H loop did not terminate through the mock batch sink");
    if (dma_read_offset != 0x01000000ULL || dma_read_size != sizeof(FpgaPackgeHead))
      return fail("C2H DDR ring offset/size mismatch");
    if (batch_calls != DMA_PACKGE_NUM) return fail("C2H packet was not dispatched as eight batches");
  }

  if (!finalized) return fail("GBus runtime was not finalized");
  std::printf("GBUS_TRANSPORT_MOCK_PASS h2c_bytes=%zu c2h_bytes=%zu batches=%u\n",
              dma_write_size, dma_read_size, batch_calls);
  return 0;
}
