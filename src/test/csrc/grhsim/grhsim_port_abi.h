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

#ifndef __SIMULATOR_GRHSIM_PORT_ABI_H
#define __SIMULATOR_GRHSIM_PORT_ABI_H

#include <type_traits>

namespace grhsim_port_abi {

template <typename T>
struct always_false : std::false_type {};

template <typename Model>
decltype(auto) uart_out_valid(Model &dut) {
  if constexpr (requires { dut.difftest_uart_out_valid; }) {
    return (dut.difftest_uart_out_valid);
  } else if constexpr (requires { dut.difftest__uart__out__valid; }) {
    return (dut.difftest__uart__out__valid);
  } else {
    static_assert(always_false<Model>::value, "missing difftest uart out valid port");
  }
}

template <typename Model>
decltype(auto) uart_out_ch(Model &dut) {
  if constexpr (requires { dut.difftest_uart_out_ch; }) {
    return (dut.difftest_uart_out_ch);
  } else if constexpr (requires { dut.difftest__uart__out__ch; }) {
    return (dut.difftest__uart__out__ch);
  } else {
    static_assert(always_false<Model>::value, "missing difftest uart out character port");
  }
}

template <typename Model>
decltype(auto) uart_in_valid(Model &dut) {
  if constexpr (requires { dut.difftest_uart_in_valid; }) {
    return (dut.difftest_uart_in_valid);
  } else if constexpr (requires { dut.difftest__uart__in__valid; }) {
    return (dut.difftest__uart__in__valid);
  } else {
    static_assert(always_false<Model>::value, "missing difftest uart in valid port");
  }
}

template <typename Model>
decltype(auto) uart_in_ch(Model &dut) {
  if constexpr (requires { dut.difftest_uart_in_ch; }) {
    return (dut.difftest_uart_in_ch);
  } else if constexpr (requires { dut.difftest__uart__in__ch; }) {
    return (dut.difftest__uart__in__ch);
  } else {
    static_assert(always_false<Model>::value, "missing difftest uart in character port");
  }
}

template <typename Model>
decltype(auto) difftest_exit(Model &dut) {
  if constexpr (requires { dut.difftest_exit; }) {
    return (dut.difftest_exit);
  } else if constexpr (requires { dut.difftest__exit; }) {
    return (dut.difftest__exit);
  } else {
    static_assert(always_false<Model>::value, "missing difftest exit port");
  }
}

template <typename Model>
decltype(auto) difftest_step(Model &dut) {
  if constexpr (requires { dut.difftest_step; }) {
    return (dut.difftest_step);
  } else if constexpr (requires { dut.difftest__step; }) {
    return (dut.difftest__step);
  } else {
    static_assert(always_false<Model>::value, "missing difftest step port");
  }
}

template <typename Model>
decltype(auto) perf_clean(Model &dut) {
  if constexpr (requires { dut.difftest_perfCtrl_clean; }) {
    return (dut.difftest_perfCtrl_clean);
  } else if constexpr (requires { dut.difftest__perfCtrl__clean; }) {
    return (dut.difftest__perfCtrl__clean);
  } else {
    static_assert(always_false<Model>::value, "missing difftest perf clean port");
  }
}

template <typename Model>
decltype(auto) perf_dump(Model &dut) {
  if constexpr (requires { dut.difftest_perfCtrl_dump; }) {
    return (dut.difftest_perfCtrl_dump);
  } else if constexpr (requires { dut.difftest__perfCtrl__dump; }) {
    return (dut.difftest__perfCtrl__dump);
  } else {
    static_assert(always_false<Model>::value, "missing difftest perf dump port");
  }
}

template <typename Model>
decltype(auto) log_begin(Model &dut) {
  if constexpr (requires { dut.difftest_logCtrl_begin; }) {
    return (dut.difftest_logCtrl_begin);
  } else if constexpr (requires { dut.difftest__logCtrl__begin; }) {
    return (dut.difftest__logCtrl__begin);
  } else {
    static_assert(always_false<Model>::value, "missing difftest log begin port");
  }
}

template <typename Model>
decltype(auto) log_end(Model &dut) {
  if constexpr (requires { dut.difftest_logCtrl_end; }) {
    return (dut.difftest_logCtrl_end);
  } else if constexpr (requires { dut.difftest__logCtrl__end; }) {
    return (dut.difftest__logCtrl__end);
  } else {
    static_assert(always_false<Model>::value, "missing difftest log end port");
  }
}

} // namespace grhsim_port_abi

#endif // __SIMULATOR_GRHSIM_PORT_ABI_H
