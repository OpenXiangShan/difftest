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

#ifndef __MMA_BACKEND_CPU_H__
#define __MMA_BACKEND_CPU_H__

#include "mma/backend/mma_backend_engine.h"

#ifdef CONFIG_DIFFTEST_AMUCTRLEVENT

class CpuMmaThreadPool;

bool verify_cpu_mma(MmaVerificationBuffer *buffer);

enum class CpuMmaGranularity {
  Instruction,
  OutputRows
};

class CpuMmaEngine final : public MmaEngine {
public:
  explicit CpuMmaEngine(unsigned threads, CpuMmaGranularity granularity = CpuMmaGranularity::Instruction);
  ~CpuMmaEngine() override;
  void compute(const std::vector<MmaVerificationBuffer *> &buffers, uint8_t *passed, uint8_t *const operands[3],
               const Completion &completed) override;
  void shutdown() override;

private:
  bool verify_output_rows(MmaVerificationBuffer *buffer);
  const unsigned threads;
  const CpuMmaGranularity granularity;
  std::unique_ptr<CpuMmaThreadPool> pool;
};

#endif // CONFIG_DIFFTEST_AMUCTRLEVENT

#endif // __MMA_BACKEND_CPU_H__
