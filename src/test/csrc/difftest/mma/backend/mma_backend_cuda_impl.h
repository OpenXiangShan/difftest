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

#ifndef __MMA_BACKEND_CUDA_IMPL_H__
#define __MMA_BACKEND_CUDA_IMPL_H__

#include "mma/backend/mma_backend.h"
#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

enum class CudaMmaType : uint8_t {
  U8U8 = 0,
  U8S8 = 1,
  S8U8 = 2,
  S8S8 = 3,
  Fp8E5M2ToFp32 = 5,
  Fp8E4M3ToFp32 = 8,
  Fp16ToFp32 = 11,
  Bf16ToFp32 = 12,
  Tf32ToFp32 = 13,
};

struct CudaMmaBatchItem {
  uint16_t tile_m;
  uint16_t tile_k;
  uint16_t tile_n;
  uint8_t types1;
  uint8_t types2;
  uint8_t sat;
  const uint8_t *src1;
  const uint8_t *src2;
  uint8_t *src3;
  const uint8_t *dut_result;
};

struct CudaMmaBatchDescriptor {
  uint64_t src1_offset;
  uint64_t src2_offset;
  uint64_t src3_offset;
  uint16_t tile_m;
  uint16_t tile_k;
  uint16_t tile_n;
  uint8_t sat;
};

struct CudaMmaCopySpan {
  uint8_t *host;
  size_t bytes;
};

struct CudaMmaOperand {
  std::unique_ptr<uint8_t[], MmaHostDeleter> packed;
  std::vector<CudaMmaCopySpan> spans;
  size_t direct_bytes = 0;
  uint8_t *device = nullptr;
};

// The workspace borrows backend-owned host storage and retains one maximum
// device batch. All requests must be released before destroying the workspace.
class CudaMmaWorkspace {
public:
  explicit CudaMmaWorkspace(MmaHostBuffer (&host)[3]) : host(host) {}
  CudaMmaWorkspace(const CudaMmaWorkspace &) = delete;
  CudaMmaWorkspace &operator=(const CudaMmaWorkspace &) = delete;
  ~CudaMmaWorkspace();
  std::vector<CudaMmaBatchDescriptor> host_descriptors;
  MmaHostBuffer (&host)[3];
  CudaMmaOperand operands[3];

  CudaMmaBatchDescriptor *descriptors = nullptr;

  bool allocate(size_t batch_size, size_t ab_reg_bytes, size_t c_reg_bytes);
  size_t batch_capacity() const;
  void release();
};

bool cuda_mma_backend_launch_kernel(CudaMmaType type, const CudaMmaBatchDescriptor *descriptors, size_t count,
                                    size_t max_output_elements, uint8_t *src1, uint8_t *src2, uint8_t *src3);

// Each backend owns one workspace. Launches must be serialized; operand
// allocation/free may run concurrently. Release requires all users to stop.
CudaMmaWorkspace *cuda_mma_backend_create(MmaHostBuffer (&host)[3], size_t batch_size, size_t ab_reg_bytes,
                                          size_t c_reg_bytes);
bool cuda_mma_backend_launch(CudaMmaWorkspace *workspace, CudaMmaType type, const CudaMmaBatchItem *items, size_t count,
                             uint8_t *passed);
void cuda_mma_backend_release(CudaMmaWorkspace *workspace);

#endif // __MMA_BACKEND_CUDA_IMPL_H__
