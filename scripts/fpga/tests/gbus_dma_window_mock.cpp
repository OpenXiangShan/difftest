#include "gbus_transport.h"
#include "xdma.h"
#include <uvaps_gbus_runtime.h>
#include <algorithm>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

int signal_num = 0;
namespace {
std::string scenario;
GbusTransport *transport = nullptr;
std::vector<uint8_t> stream(1536);
size_t start = 0, size = 0, captured = 0, payloads = 0;
unsigned initialized = 0, calls = 0, acks = 0, fills = 0, failures = 0, mirrors = 0;
unsigned fill_pending = 0, active_pending = 0, sequence_checks = 0;
uint32_t sequence = 0, ar_count = 0, ar_address = 0, ar_attrs = 0;
uint32_t ctrl_count = 0, ctrl_data = 0;
unsigned echo_consumed = 0;
bool bad_ctrl_valid = false;
bool frozen = false, identified = false, rejected = false;
bool transport_failed = false;
uint32_t bar_value = 0;
uint64_t bar_address = 0;
unsigned bar_writes = 0, bar_reads = 0;
bool bar_pending = false;
constexpr uint64_t APERTURE = 0x0ffff000;
bool bar_fence() { return scenario.compare(0, 10, "bar-fence-") == 0; }
// The startup stream-reset window consumes a well-formed control without any
// state change or fault evidence and never replays it; the host must re-issue.
bool echo_scenario() {
  return scenario == "fill-echo" || scenario == "fill-echo-stuck" || scenario == "fill-echo-bad-flag";
}
bool legacy() { return scenario == "gbs1" || scenario == "gbs1-single" || scenario == "gbs1-probe-error" ||
                      scenario == "gbs1-burst-error"; }
bool config() { return scenario == "config-window"; }
bool constructor_failure() {
  return scenario == "mode-typo" || scenario == "mode-empty" || scenario == "negative-env" ||
         scenario == "overflow-env" || scenario == "bad-chunk" || scenario == "chunk1024" || scenario == "bad-ddr-base";
}
size_t chunk() { return scenario == "chunk32" ? 32 : scenario == "chunk512" ? 512 : 256; }
void word(std::vector<uint8_t> &out, uint32_t value) {
  out = {uint8_t(value), uint8_t(value >> 8), uint8_t(value >> 16), uint8_t(value >> 24)};
}
void verify() {
  if (bar_fence()) {
    assert(initialized && !calls && !fills && !acks && !payloads && !identified);
    if (scenario == "bar-fence-ok" || scenario == "bar-fence-self-clear") {
      assert(bar_writes == 13 && bar_reads == 13 && !bar_pending && !transport_failed);
    } else if (scenario == "bar-fence-write-error") {
      assert(bar_writes == 1 && !bar_reads && transport_failed);
    } else if (scenario == "bar-fence-mismatch") {
      // CFG_RESET (offset 0x0) is fence-only; the first strict word 0x4 catches the corruption.
      assert(bar_writes == 2 && bar_reads == 2 && !bar_pending && !transport_failed);
    } else {
      assert(bar_writes == 1 && bar_reads == 1 && !bar_pending);
      assert(transport_failed == (scenario != "bar-fence-mismatch"));
    }
  } else if (constructor_failure()) {
    assert(!initialized && !calls && !fills && !acks);
  } else if (scenario == "h2c-overlap" || scenario == "h2c-overflow" || scenario == "h2c-unaligned") {
    assert(initialized && !calls && !fills && !acks);
  } else if (scenario == "bad-base" || scenario == "bad-topology" || scenario == "unknown" || scenario == "gbs1-dma") {
    assert(!calls && !fills && !acks && !payloads);
  } else if (scenario == "protocol") {
    assert(!calls && !fills && !acks && !payloads && !sequence_checks && transport_failed);
  } else if (scenario == "bad-length" || scenario == "fill-timeout" || scenario == "stop-fill") {
    assert(!calls && !acks && !payloads);
  } else if (scenario == "stop-dma" || scenario == "signal-dma") {
    assert(calls == 1 && !acks && !payloads && frozen);
  } else if (scenario == "gbs1-probe-error" || scenario == "gbs1-burst-error") {
    assert(transport_failed && !calls && !acks && !payloads);
  } else if (scenario == "short" || scenario == "error" || scenario == "oversize" || scenario == "retry") {
    assert(calls == 2 && failures == 1 && !acks && !payloads && frozen && transport_failed);
  } else if (scenario == "bad-ar" || scenario == "no-ar" || scenario == "mirror-mismatch") {
    assert(calls > 0 && !acks && !payloads && frozen);
  } else if (scenario == "seq-change" || scenario == "pre-ack-seq" || scenario == "ack-rejected") {
    assert(calls > 0 && !acks && !payloads && frozen);
  } else if (scenario == "second-window-error") {
    assert(failures == 1 && acks == 1 && payloads == 10 && frozen && transport_failed);
  } else if (scenario == "fill-echo") {
    // One control consumed by the startup window; the evidence-gated re-issue
    // and the remaining normal flow complete end to end.
    assert(echo_consumed == 1 && fills == 2 && acks == 2 && payloads == 16 && start == stream.size());
    assert(calls == 1536 / chunk() && mirrors == 256);
  } else if (scenario == "fill-echo-stuck") {
    // Every control is consumed; the bounded re-issue budget must end the run
    // without any DMA, ACK or payload.
    assert(echo_consumed == 4 && !calls && !fills && !acks && !payloads);
  } else if (scenario == "fill-echo-bad-flag") {
    // A consumed control carrying reject evidence must not be re-issued.
    assert(echo_consumed == 1 && !calls && !fills && !acks && !payloads && bad_ctrl_valid);
  } else {
    assert(payloads == 16 && fills == 2 && start == stream.size());
    if (legacy()) assert(!calls && !acks && mirrors > 0);
    else {
      assert(acks == 2);
      assert(calls == (config() ? 0 : (1536 / chunk())));
      assert(mirrors == (config() ? 384 : 256));
    }
  }
  std::fprintf(stderr, "MOCK VERIFIED %s DMA=%u ACK=%u records=%zu mirror_words=%u\n",
               scenario.c_str(), calls, acks, payloads, mirrors);
}
}

bool gbus_initialize(const char *) { initialized++; return true; }
bool gbus_finalize() { return scenario != "finalize-error"; }
int gbus_read(uint8_t, uint8_t, uint8_t, uint8_t, uint64_t address, size_t count, std::vector<uint8_t> &out) {
  assert(!transport_failed && "register I/O after reported transport failure");
  assert(address >= 0x1000);
  uint64_t local = address - 0x1000;
  if (bar_fence()) {
    assert(bar_pending && local == bar_address && count == 1);
    ++bar_reads; bar_pending = false;
    word(out, scenario == "bar-fence-mismatch" ? bar_value ^ 0x80000000U : bar_value);
    if (scenario == "bar-fence-self-clear" && local == HOST_IO_CFG_RESET)
      word(out, 0);
    if (scenario == "bar-fence-self-clear" && (local == HOST_IO_MEM_INIT || local == HOST_IO_MEM_H2C))
      word(out, 2);
    if (scenario == "bar-fence-read-error") { transport_failed = true; return -7; }
    if (scenario == "bar-fence-read-zero") { transport_failed = true; return 0; }
    if (scenario == "bar-fence-short") { out.pop_back(); transport_failed = true; }
    if (scenario == "bar-fence-oversize") { out.push_back(0); transport_failed = true; }
    return 1;
  }
  if (local == 0x1208) {
    assert(count == 1 && !identified);
    identified = scenario != "unknown";
    word(out, scenario == "unknown" ? 0 : (legacy() || scenario == "gbs1-dma") ? 0x47425331 : 0x47424431);
    return 1;
  }
  assert(identified);
  if (local < 52) {
    assert(legacy());
    if (scenario == "gbs1-probe-error" && count > 1) { transport_failed = true; return -7; }
    out.clear();
    for (size_t n = 0; n < count; n++) {
      std::vector<uint8_t> part; word(part, uint32_t(local + n*4 + 0x10203040));
      out.insert(out.end(), part.begin(), part.end());
    }
    return 1;
  }
  if (local == 0x1210) { word(out, scenario == "bad-base" ? 0x10000000 : APERTURE); return 1; }
  if (local == 0x1214) { word(out, 1024); return 1; }
  if (local == 0x1218) { word(out, scenario == "bad-ar" ? 0x10000000 : ar_address); return 1; }
  if (local == 0x121c) { word(out, ar_attrs); return 1; }
  if (local == 0x1220) { word(out, scenario == "no-ar" ? 0 : ar_count); return 1; }
  if (local == 0x1224) { word(out, ctrl_count); return 1; }
  if (local == 0x1228) { word(out, ctrl_data); return 1; }
  if (local == 0x1230) { word(out, bad_ctrl_valid ? (1U << 31) : 0U); return 1; }
  if (local == 0x1234) { word(out, bad_ctrl_valid ? 1U : 0U); return 1; }
  if (local == 0x120c) {
    sequence_checks++;
    if (scenario == "pre-ack-seq" && captured == size && mirrors == size / 4) sequence++;
    word(out, sequence); return 1;
  }
  if (local == 0x1200) {
    uint32_t status = 1U << 31;
    if (scenario == "protocol" || rejected) status |= 1U << 29;
    if (fill_pending) {
      status |= 1U << 7;
      if (scenario == "stop-fill") transport->stop();
      if (scenario != "fill-timeout" && --fill_pending == 0) frozen = true;
    } else if (frozen) {
      status |= uint32_t(size / 4) << 8;
      if (!legacy()) status |= 1U << 5;
    }
    if (active_pending) { active_pending--; status |= 1U << 4; }
    if (start < stream.size()) status |= 1U << 6;
    if (scenario == "protocol") transport_failed = true;
    word(out, status); return 1;
  }
  assert(frozen && local >= 0x2000 && local + count*4 <= 0x2000 + size);
  if (scenario == "gbs1-burst-error" && count > 1) { transport_failed = true; return -7; }
  size_t offset = local - 0x2000;
  mirrors += count;
  out.assign(stream.begin()+start+offset, stream.begin()+start+offset+count*4);
  if (scenario == "mirror-mismatch") out[0] ^= 1;
  if (legacy()) {
    assert(offset == captured);
    captured += count*4;
    if (captured == size) { frozen = false; start += size; }
  } else if (config()) captured += count*4;
  return 1;
}
int gbus_write(uint8_t, uint8_t, uint8_t, uint8_t, uint64_t address, size_t count, std::vector<uint8_t> &value) {
  assert(!transport_failed && "register write after reported transport failure");
  if (bar_fence()) {
    assert(!bar_pending && address >= 0x1000 && address <= 0x1030 && address % 4 == 0);
    assert(count == 1 && value.size() == 4);
    ++bar_writes; bar_pending = true; bar_address = address - 0x1000;
    bar_value = uint32_t(value[0]) | uint32_t(value[1]) << 8 | uint32_t(value[2]) << 16 | uint32_t(value[3]) << 24;
    if (scenario == "bar-fence-write-error") { transport_failed = true; return -7; }
    return 1;
  }
  assert(address == 0x2204 && count == 1 && value.size() == 4 && identified);
  ++ctrl_count;
  ctrl_data = value[0];
  if (value[0] == 1) {
    if (echo_scenario() && (scenario == "fill-echo-stuck" || echo_consumed < 1)) {
      // Startup stream-reset window: control echoed, no state change, no fault
      // evidence, no fill; stuck keeps consuming until the host exhausts its
      // bounded re-issue budget, bad-flag adds reject evidence to the consume.
      echo_consumed++;
      if (scenario == "fill-echo-bad-flag") bad_ctrl_valid = true;
      return 1;
    }
    assert(!frozen && !fill_pending && !active_pending && start < stream.size());
    fills++; sequence++; size = std::min<size_t>(1024, stream.size()-start);
    if (scenario == "bad-length") size = 1020;
    captured = 0; sequence_checks = 0; fill_pending = 2;
  } else {
    assert(value[0] == 2 && !legacy() && frozen && !active_pending && captured == size && sequence_checks >= 2);
    if (scenario == "ack-rejected") { rejected = true; return 1; }
    acks++; frozen = false; start += size;
  }
  return 1;
}
int gbus_dma_read(uint8_t, uint8_t, uint8_t fpga, uint8_t, uint64_t address, size_t count,
                  uint8_t channel, uint8_t port, std::vector<uint8_t> &out) {
  assert(!transport_failed && "DMA retry after reported transport failure");
  assert(identified && frozen && !legacy() && !config() && !active_pending);
  assert(fpga == 2 && channel == 0 && port == 0 && address >= APERTURE);
  size_t offset = address-APERTURE;
  assert(offset == captured && count == std::min(chunk(),size-captured) && count <= 512 && count%32 == 0);
  calls++; ar_count++; ar_address = address;
  ar_attrs = (0x11U << 24) | (1U << 22) | (5U << 19) | (uint32_t(count/32-1) << 15);
  active_pending = 2;
  out.assign(stream.begin()+start+offset, stream.begin()+start+offset+count);
  if (scenario == "stop-dma") transport->stop();
  if (scenario == "signal-dma") signal_num = 2;
  if (scenario == "seq-change" && offset == 256) sequence++;
  if (offset == 256 && ((scenario == "retry" && failures < 2) || scenario == "short" || scenario == "error" ||
      scenario == "oversize" || (scenario == "second-window-error" && start == 1024))) {
    failures++;
    transport_failed = true;
    if (scenario == "short" || (scenario == "retry" && failures == 1)) { out.resize(count-32); return 1; }
    if (scenario == "oversize") { out.push_back(0); return 1; }
    std::fill(out.begin(),out.end(),0xee); return -7;
  }
  captured += count;
  return 1;
}
int gbus_dma_write(uint8_t, uint8_t, uint8_t, uint8_t, uint64_t, size_t, uint8_t, uint8_t, std::vector<uint8_t> &) {
  assert(false && "unexpected DMA write"); return 0;
}
extern "C" void v_difftest_Batch(uint8_t data[CONFIG_DIFFTEST_BATCH_BYTELEN]) {
  assert(payloads < 16);
  assert(std::equal(data,data+CONFIG_DIFFTEST_BATCH_BYTELEN,stream.begin()+payloads*sizeof(DmaDiffPackge)+1));
  if (++payloads == 16) transport->stop();
}
int main(int argc, char **argv) {
  assert(argc == 2);
  static_assert(sizeof(FpgaPackgeHead) == 768);
  scenario = argv[1];
  for (size_t n = 0; n < stream.size(); n++) stream[n] = uint8_t((n*73 + n/256*19) ^ (n>>3));
  setenv("GBUS_C2H_MODE", (config() || legacy()) ? "config-window" : "axi-dma", 1);
  setenv("GBUS_C2H_POLL_US", "10", 1);
  setenv("GBUS_C2H_BURST_WORDS", scenario == "gbs1" || scenario == "gbs1-probe-error" ? "0" :
         scenario == "gbs1-burst-error" ? "256" : "1", 1);
  setenv("GBUS_C2H_DMA_CHUNK_BYTES", scenario == "chunk32" ? "32" : scenario == "chunk512" ? "512" :
         scenario == "chunk1024" ? "1024" : scenario == "bad-chunk" ? "33" : "256",1);
  if (scenario == "mode-typo") setenv("GBUS_C2H_MODE","axidma",1);
  if (scenario == "mode-empty") setenv("GBUS_C2H_MODE","",1);
  if (scenario == "negative-env") setenv("GBUS_PORT","-1",1);
  if (scenario == "overflow-env") setenv("GBUS_DMA_FPGA","258",1);
  if (scenario == "bad-topology") setenv("GBUS_DMA_FPGA","1",1);
  if (scenario == "bad-ddr-base") setenv("GBUS_DDR_BASE","0xffffffffffffffff",1);
  std::atexit(verify);
  GbusTransport instance; transport = &instance;
  if (bar_fence()) {
    for (uint64_t address = 0; address <= HOST_IO_CPU_AXI_DELAY; address += 4) {
      instance.fpga_io(address, scenario == "bar-fence-self-clear" ? 1U : 0xfedcba98U ^ uint32_t(address));
    }
  } else if (scenario == "h2c-overlap" || scenario == "h2c-overflow" || scenario == "h2c-unaligned") {
    uint8_t byte = 0;
    instance.h2c_load_workload(&byte,scenario == "h2c-overflow" ? UINT64_MAX : scenario == "h2c-overlap" ? 0x10000000 : 33);
  } else instance.start(true);
}
