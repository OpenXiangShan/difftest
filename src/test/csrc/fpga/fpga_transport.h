#ifndef FPGA_TRANSPORT_H
#define FPGA_TRANSPORT_H

#include <cstddef>
#include <cstdint>

class FpgaTransport {
public:
  virtual ~FpgaTransport() = default;
  virtual void start(bool enable_diff) = 0;
  virtual void stop() = 0;
  virtual void fpga_io(uint64_t address, uint32_t value) = 0;
  virtual uint32_t fpga_io_read(uint64_t address) = 0;
  virtual void wait_fpga_io_done(uint64_t address, const char *tag) = 0;
  virtual void h2c_load_workload(const void *payload, uint64_t size) = 0;
};

#endif
