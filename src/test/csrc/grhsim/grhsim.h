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

} // namespace grhsim_detail

class GrhSIMDiffTestSim final : public Simulator {
private:
  GrhSIMModel *dut;

protected:
  inline unsigned get_uart_out_valid() override {
    return static_cast<unsigned>(grhsim_detail::port_read(dut->difftest_uart_out_valid));
  }
  inline uint8_t get_uart_out_ch() override {
    return static_cast<uint8_t>(grhsim_detail::port_read(dut->difftest_uart_out_ch));
  }
  inline unsigned get_uart_in_valid() override {
    return static_cast<unsigned>(grhsim_detail::port_read(dut->difftest_uart_in_valid));
  }
  inline void set_uart_in_ch(uint8_t ch) override {
    grhsim_detail::port_write(dut->difftest_uart_in_ch, ch);
  }

public:
  GrhSIMDiffTestSim();
  ~GrhSIMDiffTestSim();

  inline void set_clock(unsigned clock) override {
    grhsim_detail::port_write(dut->clock, clock);
  }
  inline void set_reset(unsigned reset) override {
    grhsim_detail::port_write(dut->reset, reset);
  }
  inline void step() override {
    dut->eval();
  }

  inline uint64_t get_difftest_exit() override {
    return grhsim_detail::port_read(dut->difftest_exit);
  }
  inline uint64_t get_difftest_step() override {
    return grhsim_detail::port_read(dut->difftest_step);
  }

  inline void set_perf_clean(unsigned clean) override {
    grhsim_detail::port_write(dut->difftest_perfCtrl_clean, clean);
  }
  inline void set_perf_dump(unsigned dump) override {
    grhsim_detail::port_write(dut->difftest_perfCtrl_dump, dump);
  }

  inline void set_log_begin(uint64_t begin) override {
    grhsim_detail::port_write(dut->difftest_logCtrl_begin, begin);
  }
  inline void set_log_end(uint64_t end) override {
    grhsim_detail::port_write(dut->difftest_logCtrl_end, end);
  }
};

#endif // __SIMULATOR_GRHSIM_H
