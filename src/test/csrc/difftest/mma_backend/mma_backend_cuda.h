/***************************************************************************************
* Copyright (c) 2020-2025 Institute of Computing Technology, Chinese Academy of Sciences
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

#ifndef __MMA_BACKEND_CUDA_H__
#define __MMA_BACKEND_CUDA_H__

#include "mma_backend/mma_backend.h"

#ifdef CONFIG_DIFFTEST_AMUCTRLEVENT

class CudaMmaBackend : public MmaBackend {
public:
  bool verify(MmaVerificationBuffer *buffer) override;
};

#endif // CONFIG_DIFFTEST_AMUCTRLEVENT

#endif // __MMA_BACKEND_CUDA_H__
