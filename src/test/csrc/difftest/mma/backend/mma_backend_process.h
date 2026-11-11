/***************************************************************************************
* Copyright (c) 2020-2026 Institute of Computing Technology, Chinese Academy of Sciences
* Copyright (c) 2026 Beijing Institute of Open Source Chip (BOSC)
*
* DiffTest is licensed under Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
***************************************************************************************/

#ifndef __MMA_BACKEND_PROCESS_H__
#define __MMA_BACKEND_PROCESS_H__

#include "mma/backend/mma_backend.h"

#ifdef CONFIG_DIFFTEST_AMUCTRLEVENT
#include <sys/types.h>

class MmaService;

class MmaProcess {
public:
  static void set_thread_limit(unsigned threads);
  MmaProcess(MmaHostBuffer (&host)[3], bool shared_service);
  ~MmaProcess();
  MmaProcess(const MmaProcess &) = delete;
  MmaProcess &operator=(const MmaProcess &) = delete;
  void prepare();
  MatrixResult allocate_result();
  void prepare_result(MmaVerificationBuffer *buffer);
  void release_result(MmaVerificationBuffer *buffer);
  void verify(const std::vector<MmaVerificationBuffer *> &buffers, std::vector<uint8_t> &passed,
              bool copy_successful_results);

private:
  void disconnect();
  void release();
  MmaHostBuffer (&host)[3];
  std::shared_ptr<MmaService> service;
  std::shared_ptr<MmaHostBuffer> dut_host;
  std::shared_ptr<void> shared_storage;
  int storage_fd = -1;
  int socket_fd = -1;
  pid_t client_pid = -1;
  void *shared = nullptr;
  uint64_t sequence = 0;
};
#endif
#endif
