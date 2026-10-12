# MCSim Upstream PR Progress

| Phase | Change Summary | Status |
|-------|----------------|--------|
| 1 | Clean upstream diff on official master | PASS |
| 2 | Fork / push / open PR | BLOCKED on GitHub auth |

## Phase 1

### Changes Made

- Created clean worktree `/nfs/home/zhushuhang/xiangshan-mc_nix/difftest-pr-mcsim` from `github/master`.
- Added DRAMSim3-style `WITH_MCSIM` hooks in `Makefile`, `ram.h`, `ram.cpp`, and `emu.cpp`.
- Kept `mcsim-open` external via `MCSIM_HOME`; no Baiyang/mc-cosim paths included.

### Test Results

- PASS: HTTPS proxy reaches GitHub (`curl` via `172.38.11.182:10808` returns 200).
- PASS: `git diff --stat` is only the four intended source files plus plan/progress docs.
- PASS: `make -n WITH_DRAMSIM3=1 WITH_MCSIM=1` rejects mutual enablement.
- PASS: `make -n WITH_MCSIM=1` requires `MCSIM_HOME`.
- PASS: `MCSIM_HOME=/nfs/home/zhushuhang/xiangshan-mc_nix/mcsim-open` passes header/library checks.

### Issues & Debugging

- Current submodule HEAD is 10 commits ahead of upstream and includes private mc-cosim/Baiyang history, so it cannot be pushed directly.
- `gh` is not installed; GitHub SSH times out. HTTPS + proxy is the viable path.
- Upstream mutual exclusion is only DRAMSim3 vs MCSim; private `WITH_MC_COSIM` stays out of this PR.

## Phase 2

### Changes Made

- Not started.

### Test Results

- BLOCKED: need GitHub username / fork / credentials before push.

### Issues & Debugging

- No write access to `OpenXiangShan/difftest` is expected; contributor must push to a personal fork first.
