/***************************************************************************************
* Copyright (c) 2020-2026 Institute of Computing Technology, Chinese Academy of Sciences
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

#ifndef __MMA_BACKEND_H__
#define __MMA_BACKEND_H__

#include "common.h"

#ifdef CONFIG_DIFFTEST_AMUCTRLEVENT

struct MmaVerificationBuffer;

class IMmaBackend {
public:
  virtual ~IMmaBackend() = default;
  virtual bool verify(MmaVerificationBuffer *buffer) = 0;
};

#endif // CONFIG_DIFFTEST_AMUCTRLEVENT

#endif // __MMA_BACKEND_H__
