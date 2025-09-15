/***************************************************************************************
* Copyright (c) 2025 Institute of Computing Technology, Chinese Academy of Sciences
* Copyright (c) 2025 Beijing Institute of Open Source Chip (BOSC)
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

#ifndef TRACE_DEBUG_H
#define TRACE_DEBUG_H

#ifdef ENABLE_TRACE_DEBUG

#define DEBUG_INFO_IF(condition, ...) \
  do {                                \
    if (condition) {                  \
      __VA_ARGS__;                    \
    }                                 \
  } while (0)

// Implement it anywhere when you need it.
bool get_debug_flag();

#define DEBUG_INFO(code)     \
  if (get_debug_flag()) {    \
    std::cout << "[TRACE] "; \
    code;                    \
  }

#else

#define DEBUG_INFO_IF(condition, ...)
#define DEBUG_INFO(code)

#endif // ENABLE_DEBUG

#endif // TRACE_DEBUG_H
