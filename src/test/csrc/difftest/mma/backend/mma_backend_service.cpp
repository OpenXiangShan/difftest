/***************************************************************************************
* Copyright (c) 2026 Beijing Institute of Open Source Chip (BOSC)
*
* DiffTest is licensed under Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
***************************************************************************************/

#include "mma/backend/mma_backend_service.h"

#ifdef CONFIG_DIFFTEST_AMUCTRLEVENT
#include "mma/mma_verifier.h"

MmaServiceBackend::MmaServiceBackend(MmaBackendKind kind) : kind(kind), process(host, kind == MmaBackendKind::Cpu) {}

void MmaServiceBackend::start() {
  process.prepare();
}

MatrixResult MmaServiceBackend::allocate_result() {
  return process.allocate_result();
}

void MmaServiceBackend::prepare_result(MmaVerificationBuffer *buffer) {
  process.prepare_result(buffer);
}

void MmaServiceBackend::release_result(MmaVerificationBuffer *buffer) {
  process.release_result(buffer);
}

void MmaServiceBackend::verify(const std::vector<MmaVerificationBuffer *> &buffers, std::vector<uint8_t> &passed) {
  process.verify(buffers, passed, kind == MmaBackendKind::Cpu);
}

bool MmaServiceBackend::is_batch_compatible(const MmaVerificationBuffer *first,
                                            const MmaVerificationBuffer *candidate) const {
  if (kind == MmaBackendKind::Cpu)
    return true;
  if (!first || !candidate || !is_mma_32bit_result_type(first->amu_event.typed))
    return false;
  const auto &a = first->amu_event;
  const auto &b = candidate->amu_event;
  if (a.isfp && a.types1 != Fp8E5M2 && a.types1 != Fp8E4M3 && a.types1 != Fp16 && a.types1 != Bf16 && a.types1 != Tf32)
    return false;
  return a.isfp == b.isfp && a.types1 == b.types1 && a.types2 == b.types2 && a.typed == b.typed;
}

MmaBackend *make_mma_backend() {
#ifdef CONFIG_DIFFTEST_MMA_CUDA
  Info("[INFO] MMA reference model backend: CUDA\n");
  return new MmaServiceBackend(MmaBackendKind::Cuda);
#else
  Info("[INFO] MMA reference model backend: CPU\n");
  return new MmaServiceBackend(MmaBackendKind::Cpu);
#endif
}

#endif // CONFIG_DIFFTEST_AMUCTRLEVENT
