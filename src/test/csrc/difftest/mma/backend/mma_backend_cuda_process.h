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

#ifndef __MMA_BACKEND_CUDA_PROCESS_H__
#define __MMA_BACKEND_CUDA_PROCESS_H__

#include "mma/backend/mma_backend.h"

#if defined(CONFIG_DIFFTEST_AMUCTRLEVENT) && defined(CONFIG_DIFFTEST_MMA_CUDA)
#include <string>
#include <sys/types.h>

// Entry point for the standalone worker executable.
int cuda_mma_worker_main();

class CudaMmaProcess {
public:
  explicit CudaMmaProcess(MmaHostBuffer (&host)[3]);
  ~CudaMmaProcess();
  CudaMmaProcess(const CudaMmaProcess &) = delete;
  CudaMmaProcess &operator=(const CudaMmaProcess &) = delete;
  void prepare();
  void verify(const std::vector<MmaVerificationBuffer *> &buffers, std::vector<uint8_t> &passed);

private:
  // Called on the producer when a snapshot child resumes with a stopped verifier.
  void detach_inherited();
  bool start();
  void disconnect();
  void release();
  MmaHostBuffer (&host)[3];
  int storage_fd = -1;
  int socket_fd = -1;
  pid_t worker_pid = -1;
  // The root simulator owns the service; replay processes own only a session.
  pid_t owner_pid = -1;
  pid_t client_pid = -1;
  std::string service_name;
  void *shared = nullptr;
  uint64_t sequence = 0;
};
#endif

#endif // __MMA_BACKEND_CUDA_PROCESS_H__
