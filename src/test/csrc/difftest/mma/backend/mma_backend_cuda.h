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

#include "mma/backend/mma_backend_engine.h"

#ifdef CONFIG_DIFFTEST_AMUCTRLEVENT

class CudaMmaWorkspace;
struct CudaMmaBatchItem;

class CudaMmaEngine final : public MmaEngine {
public:
  CudaMmaEngine();
  CudaMmaEngine(const CudaMmaEngine &) = delete;
  CudaMmaEngine &operator=(const CudaMmaEngine &) = delete;
  ~CudaMmaEngine() override;
  bool register_storage(void *storage, size_t bytes, uint8_t *const operands[3]) override;
  void unregister_storage(void *storage) override;
  void compute(const std::vector<MmaVerificationBuffer *> &buffers, uint8_t *passed, uint8_t *const operands[3],
               const Completion &completed) override;
  void shutdown() override;

private:
  MmaHostBuffer host[3];
  std::unique_ptr<CudaMmaWorkspace> workspace;
  std::vector<CudaMmaBatchItem> items;
};

#endif // CONFIG_DIFFTEST_AMUCTRLEVENT

#endif // __MMA_BACKEND_CUDA_H__
