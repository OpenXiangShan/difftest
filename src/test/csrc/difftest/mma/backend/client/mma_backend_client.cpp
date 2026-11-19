/***************************************************************************************
* Copyright (c) 2026 Beijing Institute of Open Source Chip (BOSC)
*
* DiffTest is licensed under Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
***************************************************************************************/

#include "mma/backend/client/mma_backend_connection.h"

#ifdef CONFIG_DIFFTEST_AMUCTRLEVENT
#include "mma/backend/mma_backend_cpu.h"
#include "mma/mma_verifier.h"

namespace {
class MmaBackendClient final : public MmaBackend {
public:
  void start() override {
    connection.prepare();
  }

  MatrixResult allocate_result() override {
    return connection.allocate_result();
  }

  void prepare_result(MmaVerificationBuffer *buffer) override {
    connection.prepare_result(buffer);
  }

  void release_result(MmaVerificationBuffer *buffer) override {
    connection.release_result(buffer);
  }

  void verify(const std::vector<MmaVerificationBuffer *> &buffers, std::vector<uint8_t> &passed) override {
    connection.verify(buffers, passed);
  }

  bool is_batch_compatible(const MmaVerificationBuffer *first, const MmaVerificationBuffer *candidate) const override {
#ifdef CONFIG_DIFFTEST_MMA_CUDA
    if (!is_mma_32bit_result_type(first->amu_event.typed))
      return false;
    const auto &a = first->amu_event;
    const auto &b = candidate->amu_event;
    if (a.isfp && a.types1 != Fp8E5M2 && a.types1 != Fp8E4M3 && a.types1 != Fp16 && a.types1 != Bf16 &&
        a.types1 != Tf32)
      return false;
    return a.isfp == b.isfp && a.types1 == b.types1 && a.types2 == b.types2 && a.typed == b.typed;
#else
    return true;
#endif
  }

private:
  MmaBackendConnection connection{host};
};

class CpuInlineBackend final : public MmaBackend {
public:
  void verify(const std::vector<MmaVerificationBuffer *> &buffers, std::vector<uint8_t> &passed) override {
    passed.clear();
    passed.reserve(buffers.size());
    for (auto *buffer: buffers)
      passed.push_back(verify_cpu_mma(buffer) ? 1 : 0);
  }
};
} // namespace

MmaBackend *make_mma_backend() {
#ifdef CONFIG_DIFFTEST_MMA_CUDA
  Info("[INFO] MMA reference model backend: CUDA\n");
  return new MmaBackendClient();
#else
  Info("[INFO] MMA reference model backend: CPU\n");
#ifdef CONFIG_DIFFTEST_MMA_SYNC
  return new CpuInlineBackend();
#else
  return new MmaBackendClient();
#endif
#endif
}

#endif // CONFIG_DIFFTEST_AMUCTRLEVENT
