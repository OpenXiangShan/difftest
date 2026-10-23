# TraceRTL Kunminghu V3 difftest migration progress

| Phase | Change summary | Build | GemsFDTD smoke | gcc_expr smoke | SPEC06 0.3 |
|---|---|---|---|---|---|
| 1 | Port source GSIM wrapper onto `99f77ff45` | Not run | Not applicable | Not applicable | Not applicable |
| 2 | Resolve current GSIM/ChiselDB compatibility | Not run | Not applicable | Not applicable | Not applicable |
| 3 | Validate real trace startup and alignment | Not run | Not run | Not run | Not run |
| Final | Full 143-point validation | Not run | Not run | Not run | Not run |

## Phase 1

### Changes made

- Created the required plan and this progress record before modifying difftest code.
- Base is upstream `99f77ff455766d18cd05927b144641bd9744b19a`; source wrapper is `7f4f14ea49c58add5e6c3fc43dad6b8cad269689`.
- No difftest code changes or cherry-picks have been made yet.

### Test results

- No tests run. Phase 1 has not started.

### Issues and debugging

- External superproject submodule clones were unreliable, so the isolated checkout was initialized from local committed repositories. No source working-tree content was copied.
- The existing DeepSeek runset is not evidence of success: its build succeeded, but all 143 runs aborted at instruction 1 after a no-commit watchdog. Its branch and binary will not be reused.

## Phase 2

### Changes made

- No changes yet.

### Test results

- Not run; blocked until Phase 1 and the superproject merge are complete.

### Issues and debugging

- No Phase 2 failures recorded.

## Phase 3

### Changes made

- No changes yet.

### Test results

- GemsFDTD `38797`: not run.
- gcc_expr `4892`: not run.

### Issues and debugging

- No Phase 3 failures recorded.

## Final verification

### Changes made

- No campaign-specific changes yet.

### Test results

- Configured points: 0 submitted by this independent attempt.
- Successful points: 0 verified.
- Running points: 0.
- Failed points: 0.

### Issues and debugging

- The full campaign is blocked until the fresh build and both smoke tests pass.
