/***************************************************************************************
* Copyright (c) 2020-2026 Institute of Computing Technology, Chinese Academy of Sciences
* Copyright (c) 2026 Beijing Institute of Open Source Chip (BOSC)
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

#include "mma/backend/mma_backend.h"

#ifdef CONFIG_DIFFTEST_AMUCTRLEVENT

class CudaMmaWorkspace;

class CudaMmaBackend : public MmaBackend {
public:
  CudaMmaBackend();
  CudaMmaBackend(const CudaMmaBackend &) = delete;
  CudaMmaBackend &operator=(const CudaMmaBackend &) = delete;
  ~CudaMmaBackend() override;
  void verify(const std::vector<MmaVerificationBuffer *> &buffers, std::vector<uint8_t> &passed) override;
  bool is_batch_compatible(const MmaVerificationBuffer *first, const MmaVerificationBuffer *candidate) const override;

private:
  CudaMmaWorkspace *workspace = nullptr;
};

#endif // CONFIG_DIFFTEST_AMUCTRLEVENT

#endif // __MMA_BACKEND_CUDA_H__
