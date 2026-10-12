# MCSim Upstream PR Preparation Plan

## Header

- Task: prepare a clean PR to `OpenXiangShan/difftest` that adds MCSim as a software timing backend, modeled after the existing DRAMSim3 path.
- In-scope files:
  - `Makefile`
  - `src/test/csrc/common/ram.h`
  - `src/test/csrc/common/ram.cpp`
  - `src/test/csrc/emu/emu.cpp`
  - `.github/mcsim-upstream-pr-plan.md`
  - `.github/mcsim-upstream-pr-progress.md`
- Out of scope:
  - Baiyang / Synopsys / mc-cosim RTL paths
  - XiangShan top-level Scala/Make switches
  - Vendoring the full `mcsim-open` tree into difftest
  - Private gitlink history from `difftest-dfi`
- Target: branch based on `github/master`, pushed to the contributor's fork, then opened as a PR against `OpenXiangShan/difftest`.

## Design Rationale

Difftest already exposes DRAMSim3 through `WITH_DRAMSIM3`, reusing the DPI names `memory_request` / `memory_response` while `DifftestMem` remains the data path. MCSim should plug in the same way: `WITH_MCSIM` selects timing only, links an external `libmcsim` from `MCSIM_HOME`, and stays mutually exclusive with DRAMSim3.

The existing local integration already proved this shape, but it sits on a private history that also contains mc-cosim/Baiyang commits. For upstream, cherry-pick only the DRAMSim3-style hooks onto clean `master`.

## Prerequisites

- Clean worktree: `/nfs/home/zhushuhang/xiangshan-mc_nix/difftest-pr-mcsim` on `pr/mcsim-timing-backend`
- Proxy already configured for GitHub HTTPS:
  `git config --global http.https://github.com.proxy http://172.38.11.182:10808`
- Contributor needs a GitHub account with permission to fork `OpenXiangShan/difftest`
- Optional later validation:
  - `MCSIM_HOME` pointing at a built `verilator-lib`
  - compile-check with `WITH_MCSIM=1`

## Phase 1: Apply Minimal Upstream Diff

Files: `Makefile`, `ram.h`, `ram.cpp`, `emu.cpp`

- Add mutual exclusion between `WITH_DRAMSIM3` and `WITH_MCSIM`.
- Add `WITH_MCSIM` build/link validation against `MCSIM_HOME` / `libmcsim.a`.
- Share `memory_request` / `memory_response` declarations for DRAMSim3 or MCSim.
- Add MCSim init/step/finish hooks next to the DRAMSim3 hooks.

Test: `git diff --stat` shows only the intended files; no mc-cosim/Baiyang paths appear. PASS = clean four-file code diff plus plan/progress docs. FAIL = private RTL/history leakage.

## Phase 2: Auth / Fork / Push

- Confirm GitHub identity and create/use a fork of `OpenXiangShan/difftest`.
- Add the fork as a remote over HTTPS (SSH to GitHub is blocked here).
- Push `pr/mcsim-timing-backend` and open the PR.

Test: remote `ls-remote` / push succeeds and the PR URL opens against upstream `master`. PASS = PR created. FAIL = missing auth/fork.

## Final Verification

- PR contains only MCSim timing-backend integration.
- PR description states that `mcsim-open` remains an external library via `MCSIM_HOME`.
- Baiyang remains a separate XiangShan-side hardware integration.
