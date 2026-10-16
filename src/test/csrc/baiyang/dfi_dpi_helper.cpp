/***************************************************************************************
* Copyright (c) 2026 Beijing Institute of Open Source Chip (BOSC)
*
* DiffTest is licensed under Mulan PSL v2.
***************************************************************************************/

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <mutex>

#include <svdpi.h>

#include "dfi/dfi_handshake_fsm.h"
#include "ram.h"

#ifndef BAIYANG_ADDRESS_MAPPING
#define BAIYANG_ADDRESS_MAPPING "src/main/baiyang/address_mapping_baiyang_dual.ini"
#endif

#ifndef BAIYANG_DFI_CHANNELS
#define BAIYANG_DFI_CHANNELS 2
#endif

namespace {

constexpr size_t kDfiChannels = BAIYANG_DFI_CHANNELS;
constexpr uint32_t kDfiPhases = 2U;
constexpr uint32_t kCmdKindWidth = 4U;
constexpr uint32_t kCmdRankWidth = 2U;
constexpr uint32_t kCmdBankWidth = 4U;

enum class dfi_cmd_kind : uint32_t {
  kNop = 0U,
  kActive,
  kRead,
  kWrite,
  kPrecharge,
  kRefresh,
  kMrs,
  kZq,
  kUnknown,
};

struct set_default_mapping {
  set_default_mapping() {
    if (std::getenv("MC_ADDRESS_MAPPING") == nullptr) {
      setenv("MC_ADDRESS_MAPPING", BAIYANG_ADDRESS_MAPPING, 0);
    }
  }
};

struct difftest_ram_backend {
  SimMemory* memory = nullptr;
  uint64_t size_bytes = 0U;
};

set_default_mapping g_default_mapping;
std::array<dfi::dfi_handshake_fsm, kDfiChannels> g_dfi_fsm;
std::array<dfi::dfi_output_drive, kDfiChannels> g_dfi_out{};
std::array<bool, kDfiChannels> g_preload_done{};
difftest_ram_backend g_ram_backend;
std::mutex g_ram_mutex;

size_t channel_index(uint32_t channel) {
  if (channel >= kDfiChannels) {
    std::cerr << "[DFI] invalid channel " << channel
              << " (configured channels=" << kDfiChannels << ")" << std::endl;
    std::abort();
  }
  return static_cast<size_t>(channel);
}

uint32_t extract_field(uint32_t value, uint32_t phase, uint32_t width) {
  return (value >> (phase * width)) & ((1U << width) - 1U);
}

uint32_t extract_arg(uint64_t value, uint32_t phase) {
  return static_cast<uint32_t>(value >> (phase * 32U));
}

bool backend_range_valid(const difftest_ram_backend* backend, uint64_t offset) {
  return backend != nullptr && backend->memory != nullptr &&
         dfi::virtual_ram::kBeatBytes <= backend->size_bytes &&
         offset <= backend->size_bytes - dfi::virtual_ram::kBeatBytes;
}

bool backend_read(void* context, uint64_t offset, uint32_t* data) {
  auto* backend = static_cast<difftest_ram_backend*>(context);
  if (data == nullptr || !backend_range_valid(backend, offset)) {
    return false;
  }

  const std::lock_guard<std::mutex> lock(g_ram_mutex);
  for (size_t word = 0U; word < dfi::virtual_ram::kBeatBytes / sizeof(uint64_t); ++word) {
    const uint64_t value = difftest_ram_read(offset / sizeof(uint64_t) + word);
    std::memcpy(data + word * 2U, &value, sizeof(value));
  }
  return true;
}

bool backend_write(void* context,
                   uint64_t offset,
                   const uint32_t* data,
                   uint32_t byte_enable) {
  auto* backend = static_cast<difftest_ram_backend*>(context);
  if (data == nullptr || !backend_range_valid(backend, offset)) {
    return false;
  }

  const std::lock_guard<std::mutex> lock(g_ram_mutex);
  for (size_t word = 0U; word < dfi::virtual_ram::kBeatBytes / sizeof(uint64_t); ++word) {
    uint64_t word_data;
    std::memcpy(&word_data, data + word * 2U, sizeof(word_data));
    uint64_t word_mask = 0U;
    const uint32_t enables = byte_enable >> (word * sizeof(uint64_t));
    for (size_t byte = 0U; byte < sizeof(uint64_t); ++byte) {
      word_mask |= ((enables >> byte) & 0x1U) * (0xFFULL << (byte * 8U));
    }
    if (word_mask != 0U) {
      difftest_ram_write(offset / sizeof(uint64_t) + word, word_data, word_mask);
    }
  }
  return true;
}

void dispatch_commands(dfi::dfi_handshake_fsm& fsm,
                       uint32_t cmd_valid,
                       uint32_t cmd_kind,
                       uint32_t cmd_rank,
                       uint32_t cmd_bank,
                       uint64_t cmd_arg0,
                       uint64_t cmd_arg1) {
  for (uint32_t phase = 0U; phase < kDfiPhases; ++phase) {
    if (((cmd_valid >> phase) & 0x1U) == 0U) {
      continue;
    }

    const auto kind =
        static_cast<dfi_cmd_kind>(extract_field(cmd_kind, phase, kCmdKindWidth));
    const uint32_t rank = extract_field(cmd_rank, phase, kCmdRankWidth);
    const uint32_t bank = extract_field(cmd_bank, phase, kCmdBankWidth);
    const uint32_t arg0 = extract_arg(cmd_arg0, phase);
    const uint32_t arg1 = extract_arg(cmd_arg1, phase);
    switch (kind) {
      case dfi_cmd_kind::kActive: fsm.on_active(phase, rank, bank, arg0); break;
      case dfi_cmd_kind::kRead: fsm.on_read(phase, rank, bank, arg0); break;
      case dfi_cmd_kind::kWrite: fsm.on_write(phase, rank, bank, arg0); break;
      case dfi_cmd_kind::kPrecharge: fsm.on_precharge(phase, rank, bank, arg0); break;
      case dfi_cmd_kind::kRefresh: fsm.on_refresh(phase, rank); break;
      case dfi_cmd_kind::kMrs: fsm.on_mrs(phase, rank, arg0, arg1); break;
      case dfi_cmd_kind::kZq: fsm.on_zq(phase, rank); break;
      case dfi_cmd_kind::kNop:
      case dfi_cmd_kind::kUnknown: break;
    }
  }
}

}  // namespace

extern "C" void dfi_dpi_reset(uint32_t channel) {
  const size_t index = channel_index(channel);
  g_dfi_fsm[index].set_channel(static_cast<uint8_t>(index));
  g_dfi_fsm[index].reset(g_dfi_out[index]);
  g_preload_done[index] = g_dfi_fsm[index].memory_ready();
}

extern "C" void dfi_dpi_cycle(uint32_t channel,
                              uint32_t frequency,
                              uint32_t ctrlupd_req,
                              uint32_t lp_req,
                              uint32_t cmd_valid,
                              uint32_t cmd_kind,
                              uint32_t cmd_rank,
                              uint32_t cmd_bank,
                              uint64_t cmd_arg0,
                              uint64_t cmd_arg1,
                              uint32_t rddata_en,
                              uint32_t wrdata_en,
                              uint32_t wrdata_mask,
                              const svBitVecVal* wrdata,
                              svBitVecVal* rddata,
                              uint32_t* rddata_valid,
                              uint32_t* rddata_dbi,
                              uint32_t* ctrlupd_ack,
                              uint32_t* ctrlupd_ack2,
                              uint32_t* lp_ack) {
  const size_t index = channel_index(channel);
  auto& fsm = g_dfi_fsm[index];
  auto& out = g_dfi_out[index];
  fsm.begin_core_cycle(frequency, ctrlupd_req, lp_req, out);
  dispatch_commands(fsm, cmd_valid, cmd_kind, cmd_rank, cmd_bank, cmd_arg0, cmd_arg1);

  dfi::dfi_data_sample in;
  in.rddata_en = rddata_en;
  in.wrdata_en = wrdata_en;
  in.wrdata_mask = wrdata_mask;
  if (fsm.needs_write_data()) {
    for (size_t i = 0U; i < in.wrdata.size(); ++i) {
      in.wrdata[i] = static_cast<uint32_t>(wrdata[i]);
    }
  }
  fsm.end_core_cycle(in, out);

  for (size_t i = 0U; i < out.rddata.size(); ++i) {
    rddata[i] = out.rddata[i];
  }
  *rddata_valid = out.rddata_valid;
  *rddata_dbi = out.rddata_dbi;
  *ctrlupd_ack = out.ctrlupd_ack;
  *ctrlupd_ack2 = out.ctrlupd_ack2;
  *lp_ack = out.lp_ack;
}

extern "C" bool baiyang_preload_done() {
  return std::all_of(g_preload_done.begin(), g_preload_done.end(), [](bool done) {
    return done;
  });
}

extern "C" bool baiyang_bind_difftest_ram(uint64_t base_addr) {
  if (simMemory == nullptr) {
    std::cerr << "[BAIYANG] cannot bind RAM before simMemory initialization" << std::endl;
    g_preload_done.fill(false);
    return false;
  }

  g_ram_backend = {simMemory, simMemory->get_size()};
  bool bound_all = true;
  for (size_t channel = 0U; channel < kDfiChannels; ++channel) {
    auto& fsm = g_dfi_fsm[channel];
    fsm.set_channel(static_cast<uint8_t>(channel));
    g_preload_done[channel] = fsm.bind_memory_backend(base_addr,
                                                      g_ram_backend.size_bytes,
                                                      &g_ram_backend,
                                                      backend_read,
                                                      backend_write);
    bound_all = bound_all && g_preload_done[channel];
  }
  return bound_all;
}

extern "C" void baiyang_preload_difftest_ram(uint64_t base_addr) {
  (void)baiyang_bind_difftest_ram(base_addr);
}
