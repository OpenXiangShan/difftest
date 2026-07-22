/***************************************************************************************
* Copyright (c) 2026 Institute of Computing Technology, Chinese Academy of Sciences
*
* DiffTest is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
***************************************************************************************/

#ifndef __SIMULATOR_GRHSIM_H
#define __SIMULATOR_GRHSIM_H

#include "grhsim_model_select.hpp"
#include "grhsim_port_abi.h"
#include <array>
#include <type_traits>

namespace grhsim_detail {

template <typename T>
struct always_false : std::false_type {};

template <typename T>
inline uint64_t port_read(const T &value) {
  using RawT = std::remove_cv_t<std::remove_reference_t<T>>;
  if constexpr (std::is_same_v<RawT, bool>) {
    return value ? 1 : 0;
  } else if constexpr (std::is_integral_v<RawT>) {
    return static_cast<uint64_t>(value);
  } else {
    static_assert(always_false<RawT>::value, "unsupported grhsim port type");
  }
}

template <typename T, size_t N>
inline uint64_t port_read(const std::array<T, N> &value) {
  constexpr size_t kElemBits = sizeof(T) * 8;
  constexpr size_t kMaxElems = (64 + kElemBits - 1) / kElemBits;
  const size_t limit = N < kMaxElems ? N : kMaxElems;
  uint64_t out = 0;
  for (size_t i = 0; i < limit; ++i) {
    out |= static_cast<uint64_t>(value[i]) << (i * kElemBits);
  }
  return out;
}

template <typename T>
inline void port_write(T &dest, uint64_t value) {
  using RawT = std::remove_cv_t<std::remove_reference_t<T>>;
  if constexpr (std::is_same_v<RawT, bool>) {
    dest = (value & 1) != 0;
  } else if constexpr (std::is_integral_v<RawT>) {
    dest = static_cast<RawT>(value);
  } else {
    static_assert(always_false<RawT>::value, "unsupported grhsim port type");
  }
}

template <typename T, size_t N>
inline void port_write(std::array<T, N> &dest, uint64_t value) {
  constexpr size_t kElemBits = sizeof(T) * 8;
  constexpr uint64_t kMask = kElemBits >= 64 ? ~uint64_t(0) : ((uint64_t(1) << kElemBits) - 1);
  for (size_t i = 0; i < N; ++i) {
    const size_t shift = i * kElemBits;
    dest[i] = shift >= 64 ? T(0) : static_cast<T>((value >> shift) & kMask);
  }
}

template <typename T>
inline bool termination_requested(const T &model) {
  if constexpr (requires {
                  static_cast<bool>(model.finish_requested());
                  static_cast<bool>(model.stop_requested());
                  static_cast<bool>(model.fatal_requested());
                }) {
    return model.finish_requested() || model.stop_requested() || model.fatal_requested();
  }
  return false;
}

template <typename T>
inline void finalize_if_requested(T &model) {
  if (!termination_requested(model)) {
    return;
  }
  if constexpr (requires { model.finalize(); }) {
    model.finalize();
  }
}

template <typename T>
inline uint64_t termination_exit(const T &model) {
  if (!termination_requested(model)) {
    return 0;
  }
  if constexpr (requires { static_cast<int>(model.system_exit_code()); }) {
    const int exit_code = model.system_exit_code();
    if (exit_code != 0) {
      // Zero-extend so a negative int cannot collide with the all-ones good-exit marker.
      return static_cast<uint64_t>(static_cast<uint32_t>(exit_code));
    }
  }
  return ~uint64_t{0};
}

} // namespace grhsim_detail

class GrhSIMDiffTestSim final : public Simulator {
private:
  GrhSIMModel *dut;
  bool phase_timing_enabled_ = false;
  uint64_t model_step_count_ = 0;
  uint64_t model_step_time_us_ = 0;

protected:
  inline unsigned get_uart_out_valid() override {
    return static_cast<unsigned>(grhsim_detail::port_read(grhsim_port_abi::uart_out_valid(*dut)));
  }
  inline uint8_t get_uart_out_ch() override {
    return static_cast<uint8_t>(grhsim_detail::port_read(grhsim_port_abi::uart_out_ch(*dut)));
  }
  inline unsigned get_uart_in_valid() override {
    return static_cast<unsigned>(grhsim_detail::port_read(grhsim_port_abi::uart_in_valid(*dut)));
  }
  inline void set_uart_in_ch(uint8_t ch) override {
    grhsim_detail::port_write(grhsim_port_abi::uart_in_ch(*dut), ch);
  }

public:
  GrhSIMDiffTestSim();
  ~GrhSIMDiffTestSim();

  void waveform_init(uint64_t cycles) override;
  void waveform_init(uint64_t cycles, const char *filename) override;
  void waveform_tick() override;
  void step() override;
  SimulatorRuntimeStats runtime_stats() const override;
  void set_runtime_profile_enabled(bool enabled) override;
  void dump_runtime_profile() const override;

  inline void set_clock(unsigned clock) override {
    grhsim_detail::port_write(dut->clock, clock);
  }
  inline void set_reset(unsigned reset) override {
    grhsim_detail::port_write(dut->reset, reset);
  }

  inline uint64_t get_difftest_exit() override {
    const uint64_t termination_exit = grhsim_detail::termination_exit(*dut);
    if (termination_exit != 0) {
      return termination_exit;
    }
    return grhsim_detail::port_read(grhsim_port_abi::difftest_exit(*dut));
  }
  inline uint64_t get_difftest_step() override {
    return grhsim_detail::port_read(grhsim_port_abi::difftest_step(*dut));
  }

  inline void set_perf_clean(unsigned clean) override {
    grhsim_detail::port_write(grhsim_port_abi::perf_clean(*dut), clean);
  }
  inline void set_perf_dump(unsigned dump) override {
    grhsim_detail::port_write(grhsim_port_abi::perf_dump(*dut), dump);
  }

  inline void set_log_begin(uint64_t begin) override {
    grhsim_detail::port_write(grhsim_port_abi::log_begin(*dut), begin);
  }
  inline void set_log_end(uint64_t end) override {
    grhsim_detail::port_write(grhsim_port_abi::log_end(*dut), end);
  }
};

#endif // __SIMULATOR_GRHSIM_H
