// Deterministic API-level tests of the production GbusTransport, not a copy of it.
#include "gbus_transport.h"
#include "xdma.h"
#include <algorithm>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <uvaps_gbus_runtime.h>
#include <vector>

int signal_num = 0;
namespace {
std::string scenario;
GbusTransport *transport;
std::vector<uint8_t> stream(1536);
size_t bank_start = 0, bank_size = 0, captured = 0, payloads = 0;
unsigned fills = 0, acks = 0, calls = 0, failures = 0, register_data = 0;
unsigned idle_polls = 0, fill_polls = 0, active_polls = 0, seq_checks = 0;
unsigned fill_pending = 0, active_pending = 0;
uint32_t seq = 0;
bool frozen = false, identified = false, changed = false, ack_rejected = false;
bool legacy() {
  return scenario == "gbs1" || scenario == "gbs1-single";
}
size_t chunk() {
  return scenario == "chunk32" ? 32 : scenario == "chunk1024" ? 512 : 256;
}
void word(std::vector<uint8_t> &v, uint32_t w) {
  v = {uint8_t(w), uint8_t(w >> 8), uint8_t(w >> 16), uint8_t(w >> 24)};
}
void verify() {
  if (scenario == "unknown") {
    assert(!identified && calls == 0 && fills == 0 && register_data == 0);
  } else if (scenario == "bad-chunk" || scenario == "bad-base") {
    assert(calls == 0 && acks == 0 && payloads == 0);
  } else if (scenario == "stop-fill" || scenario == "bad-length" || scenario == "protocol") {
    assert(calls == 0 && acks == 0 && payloads == 0);
  } else if (scenario == "stop-dma" || scenario == "signal-dma") {
    assert(calls == 1 && acks == 0 && payloads == 0 && frozen);
  } else if (scenario == "short" || scenario == "error" || scenario == "oversize") {
    assert(failures == 3 && calls == 4 && acks == 0 && payloads == 0 && frozen);
  } else if (scenario == "seq-change") {
    assert(calls == 2 && acks == 0 && payloads == 0 && frozen);
  } else if (scenario == "pre-ack-seq" || scenario == "pre-ack-status" || scenario == "ack-rejected") {
    assert(calls == 4 && acks == 0 && payloads == 0 && frozen);
    assert(ack_rejected == (scenario == "ack-rejected"));
  } else if (scenario == "fill-timeout") {
    assert(calls == 0 && fills == 1 && acks == 0 && payloads == 0);
  } else if (scenario == "second-window-error") {
    assert(failures == 3 && acks == 1 && payloads == 8 && frozen);
  } else {
    assert(payloads == 16 && fills == 2 && bank_start == stream.size());
    if (legacy())
      assert(calls == 0 && acks == 0 && register_data > 0);
    else {
      assert(acks == 2 && register_data == 0 && fill_polls >= 4 && active_polls >= 4);
      assert(calls == 1536 / chunk() + (scenario == "retry-short-error" ? 2 : 0));
    }
  }
  assert(register_data == 0 || legacy());
  std::fprintf(stderr, "MOCK VERIFIED %s calls=%u ACK=%u payloads=%zu\n", scenario.c_str(), calls, acks, payloads);
}
} // namespace

bool gbus_initialize(const char *) {
  return true;
}
bool gbus_finalize() {
  return true;
}
int gbus_read(uint8_t, uint8_t, uint8_t, uint8_t, uint64_t address, size_t count, std::vector<uint8_t> &out) {
  assert(address >= 0x1000);
  const uint64_t local = address - 0x1000;
  if (local == 0x1208) {
    assert(!identified && count == 1);
    identified = scenario != "unknown";
    word(out, scenario == "unknown" ? 0 : legacy() ? 0x47425331 : 0x47424431);
    return 1;
  }
  assert(identified); // ID is required before even a config BAR burst probe.
  if (local < 13 * 4) {
    assert(legacy());
    out.clear();
    if (scenario == "gbs1-single" && count > 1)
      return 0;
    for (size_t i = 0; i < count; ++i) {
      std::vector<uint8_t> one;
      word(one, uint32_t(0x10203040 + local + i * 4));
      out.insert(out.end(), one.begin(), one.end());
    }
    return 1;
  }
  assert(count == 1 || (legacy() && local >= 0x2000));
  if (local == 0x1210) {
    word(out, scenario == "bad-base" ? 0x01000000 : 0x10000000);
    return 1;
  }
  if (local == 0x1214) {
    word(out, 1024);
    return 1;
  }
  // GBD1 AXI read-port diagnostics: last AR address, last AR attributes, and
  // the accepted-AR count.  They must be readable at any time on the DMA path.
  if (local == 0x1218) {
    assert(!legacy());
    word(out, uint32_t(0x10000000 + captured));
    return 1;
  }
  if (local == 0x121c) {
    assert(!legacy());
    word(out,
         (uint32_t(0x11) << 24) | (1U << 22) | (5U << 19) | (uint32_t(std::min<size_t>(15, chunk() / 32 - 1)) << 15));
    return 1;
  }
  if (local == 0x1220) {
    assert(!legacy());
    word(out, uint32_t(calls + 1));
    return 1;
  }
  if (local == 0x120c) {
    ++seq_checks;
    if (scenario == "pre-ack-seq" && seq_checks == 6)
      ++seq;
    word(out, seq);
    return 1;
  }
  if (local == 0x1200) {
    uint32_t st = 1U << 31;
    if (scenario == "protocol" || ack_rejected)
      st |= 1U << 29;
    if (scenario == "pre-ack-status" && seq_checks == 6)
      st |= 1U << 30;
    if (scenario == "fill-timeout" && fill_pending) {
      word(out, st | (1U << 7));
      return 1;
    }
    if (fill_pending) {
      ++fill_polls;
      if (scenario == "stop-fill")
        transport->stop();
      --fill_pending;
      if (!fill_pending)
        frozen = true;
      word(out, st | (1U << 7));
      return 1;
    }
    if (active_pending) {
      --active_pending;
      ++active_polls;
      st |= 1U << 4;
    }
    if (frozen) {
      st |= uint32_t(bank_size / 4) << 8;
      if (!legacy())
        st |= 1U << 5;
    }
    if (idle_polls++ >= 3 && bank_start < stream.size())
      st |= 1U << 6;
    word(out, st);
    return 1;
  }
  assert(legacy() && frozen && local >= 0x2000 && local + count * 4 <= 0x2400);
  assert(local - 0x2000 == captured);
  assert(captured + count * 4 <= bank_size);
  ++register_data;
  out.assign(stream.begin() + bank_start + captured, stream.begin() + bank_start + captured + count * 4);
  captured += count * 4;
  if (captured == bank_size) {
    frozen = false;
    bank_start += bank_size;
  }
  return 1;
}
int gbus_write(uint8_t, uint8_t, uint8_t, uint8_t, uint64_t address, size_t count, std::vector<uint8_t> &value) {
  assert(identified && address == 0x2204 && count == 1 && value.size() == 4);
  if (value[0] == 1) {
    assert(!frozen && !fill_pending && !active_pending && bank_start < stream.size() && idle_polls > 3);
    ++fills;
    ++seq;
    bank_size = std::min<size_t>(1024, stream.size() - bank_start);
    if (scenario == "bad-length")
      bank_size = 1020;
    captured = 0;
    seq_checks = 0;
    fill_pending = 2;
  } else {
    assert(value[0] == 2 && !legacy());
    assert(frozen && !fill_pending && !active_pending && captured == bank_size);
    assert(seq_checks >= 3); // publication, DMA completion, and pre-ACK validation
    if (scenario == "ack-rejected") {
      ack_rejected = true;
      return 1;
    }
    ++acks;
    frozen = false;
    bank_start += bank_size;
  }
  return 1;
}
int gbus_dma_read(uint8_t, uint8_t, uint8_t, uint8_t, uint64_t address, size_t size, uint8_t, uint8_t,
                  std::vector<uint8_t> &out) {
  assert(!legacy() && identified && frozen && !fill_pending && !active_pending);
  // Any C2H access to DDR or the register debug window fails this test.
  assert(address >= 0x10000000 && address + size <= 0x10000400);
  const size_t offset = address - 0x10000000;
  assert(offset == captured && size == std::min(chunk(), bank_size - captured));
  assert(size % 32 == 0 && size <= 512 && offset + size <= bank_size);
  ++calls;
  active_pending = 2;
  out.assign(stream.begin() + bank_start + offset, stream.begin() + bank_start + offset + size);
  if (scenario == "stop-dma")
    transport->stop();
  if (scenario == "signal-dma")
    signal_num = 2;
  if (scenario == "seq-change" && offset == 256 && !changed) {
    ++seq;
    changed = true;
  }
  const bool fail_here = offset == 256 && ((scenario == "retry-short-error" && failures < 2) || scenario == "short" ||
                                           scenario == "error" || scenario == "oversize" ||
                                           (scenario == "second-window-error" && bank_start == 1024));
  if (fail_here) {
    ++failures;
    if (scenario == "short" || (scenario == "retry-short-error" && failures == 1)) {
      out.resize(size - 32);
      return 1;
    }
    if (scenario == "oversize") {
      out.push_back(0);
      return 1;
    }
    std::fill(out.begin(), out.end(), 0xee); // Ensure failed bytes never leak into dispatch.
    return -7;
  }
  captured += size;
  return 1;
}
int gbus_dma_write(uint8_t, uint8_t, uint8_t, uint8_t, uint64_t, size_t, uint8_t, uint8_t, std::vector<uint8_t> &) {
  assert(false && "C2H must never write DDR");
  return 0;
}
extern "C" void v_difftest_Batch(uint8_t data[CONFIG_DIFFTEST_BATCH_BYTELEN]) {
  assert(payloads < 16);
  const size_t record = payloads * sizeof(DmaDiffPackge);
  assert(std::equal(data, data + CONFIG_DIFFTEST_BATCH_BYTELEN, stream.begin() + record + 1));
  ++payloads;
  if (payloads == 16)
    transport->stop();
}
int main(int argc, char **argv) {
  assert(argc == 2);
  static_assert(sizeof(FpgaPackgeHead) == 768, "test must exercise the real 768-byte reassembly");
  scenario = argv[1];
  for (size_t i = 0; i < stream.size(); ++i)
    stream[i] = uint8_t((i * 73 + i / 256 * 19) ^ (i >> 3));
  for (size_t i = 0; i < 16; ++i)
    stream[i * sizeof(DmaDiffPackge)] = uint8_t(i);
  setenv("GBUS_C2H_POLL_US", "10", 1);
  setenv("GBUS_C2H_BURST_WORDS", "0", 1);
  setenv("GBUS_CONFIG_BASE", "0x1000", 1);
  setenv("GBUS_C2H_DMA_CHUNK_BYTES",
         scenario == "chunk32"     ? "32"
         : scenario == "chunk1024" ? "1024"
         : scenario == "bad-chunk" ? "33"
                                   : "256",
         1);
  std::atexit(verify);
  GbusTransport instance;
  transport = &instance;
  instance.start(true);
}
