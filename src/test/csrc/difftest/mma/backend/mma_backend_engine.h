/***************************************************************************************
* Copyright (c) 2026 Beijing Institute of Open Source Chip (BOSC)
*
* DiffTest is licensed under Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
***************************************************************************************/

#ifndef __MMA_BACKEND_ENGINE_H__
#define __MMA_BACKEND_ENGINE_H__

#include "mma/backend/mma_backend.h"
#include <functional>

#ifdef CONFIG_DIFFTEST_AMUCTRLEVENT

class MmaEngine {
public:
  using Completion = std::function<bool(size_t)>;
  virtual ~MmaEngine() = default;
  virtual bool register_storage(void *, size_t, uint8_t *const[3]) {
    return true;
  }
  virtual void unregister_storage(void *) {}
  // Retain completed until the last use of buffers and passed.
  virtual void compute(const std::vector<MmaVerificationBuffer *> &buffers, uint8_t *passed, uint8_t *const operands[3],
                       const Completion &completed) = 0;
  virtual void shutdown() {}
};

#endif // CONFIG_DIFFTEST_AMUCTRLEVENT
#endif // __MMA_BACKEND_ENGINE_H__
