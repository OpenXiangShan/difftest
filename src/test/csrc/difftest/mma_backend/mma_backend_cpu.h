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

#ifndef __MMA_BACKEND_CPU_H__
#define __MMA_BACKEND_CPU_H__

#include "mma_backend/mma_backend.h"

#ifdef CONFIG_DIFFTEST_AMUCTRLEVENT

class CpuMmaBackend : public IMmaBackend {
public:
  bool verify(MmaVerificationBuffer *buffer) override;

private:
  template <class src1_t, class src2_t>
  bool mmacc_template(MmaVerificationBuffer *buffer);

  template <int src_exp_bits, int src_mantissa_bits, int result_exp_bits, int result_mantissa_bits>
  bool mfmacc_template(MmaVerificationBuffer *buffer);
};

#endif // CONFIG_DIFFTEST_AMUCTRLEVENT

#endif // __MMA_BACKEND_CPU_H__
