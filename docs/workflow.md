# DiffTest Task Workflow

This document defines the standard workflow for complex difftest tasks — those involving multi-file changes, pipeline modifications, or iterative debugging. Simple one-shot edits do not require this process.

## Overview

```
Task received
  │
  ▼
Create a plan when useful (location is flexible)
  │
  ▼
Track progress when useful (location is flexible)
  │
  ▼
┌─ Per Phase ──────────────────────────┐
│  Re-read plan + progress             │
│  │                                   │
│  ▼                                   │
│  Implement changes                   │
│  │                                   │
│  ▼                                   │
│  Test (microbench → linux)           │
│  │                                   │
│  ├─ PASS → update progress, next     │
│  └─ FAIL → debug, fix, re-test      │
└──────────────────────────────────────┘
  │
  ▼
Final verification (10-min linux)
  │
  ▼
Report completion / next steps when relevant
```

---

## 1. Plan Document

For a complex or iterative task, a written plan is recommended when it improves coordination or recovery. It is not required for every non-trivial task, and no particular repository path is required. An issue, PR description, conversation note, or local untracked file may be used instead. Do not add a plan file to `.github/` unless it is useful to reviewers or maintainers.

### Suggested Sections

| Section | Contents |
|---------|----------|
| **Header** | Task title, modification scope (which directories/files may be changed), target configuration, execution requirements |
| **Design Rationale** | High-level description of *what* changes and *why*; core principles and key trade-offs |
| **Prerequisites** | Environment variables, reference files, common build commands, common test commands, debug workflow |
| **Phases** | One section per Phase: files to modify, specific changes with before/after logic, expected behavior, test instructions |
| **Final Verification** | Acceptance test (typically 10-min linux) and exit criteria |

### Guidelines

- **Explicit build and test commands.** Every Phase must include the exact commands to build and test. Do not rely on "same as before" — copy the commands so each Phase is self-contained.
- **Explicit pass/fail criteria.** For each test, state what constitutes PASS (e.g. `HIT GOOD TRAP`, no `ABORT`/`mismatch`) and FAIL.
- **Logic description before code.** Describe the intended behavior change in words before showing code snippets. State the invariants that must hold.
- **Scope boundaries.** Clearly state which files/directories are in-scope and which are off-limits (e.g. "do not modify `src/test/`").

### Versioning

If a written plan needs significant revision, update it in place or create a new version where the notes live. Keep old versions only when they provide useful review or debugging context.

---

## 2. Progress Document

When a plan is used, progress tracking is recommended for long-running or iterative work. It may be kept in the same note, issue/PR, conversation, or another suitable location; a separate progress file and a fixed `.github/` path are not required.

### Suggested Sections Per Phase

| Section | Contents |
|---------|----------|
| **Changes Made** | Bullet list of actual modifications (file, what changed) |
| **Test Results** | Each test with PASS/FAIL, key metrics (instrCnt, cycleCnt, IPC, duration) |
| **Issues & Debugging** | Problems encountered, root-cause analysis, attempted fixes (including failed ones), and final resolution |

### Status Table

Use a summary table at the top for quick overview:

```markdown
| Phase | Change Summary | Microbench | Linux 5min | Linux 10min |
|-------|---------------|-----------|-----------|-------------|
| 1     | Squasher Decoupled | ✅ 783387 | ✅ 0 errors | — |
| 2     | DeltaSplitter Decoupled | ✅ 783387 | ✅ 0 errors | — |
| Final | All changes | — | — | ✅ 600s |
```

### Guidelines

- **Record failed attempts.** When a fix fails, document the attempt, the failure mode, and why it was wrong. This prevents repeating the same mistake and preserves debugging context.
- **Update immediately.** Write progress entries as each phase completes, not at the end. If a conversation is interrupted, the available notes or issue/PR should provide recovery context.
- **Include quantitative data.** Log exact instrCnt, cycleCnt, step numbers, error messages — not just "passed" or "failed".

---

## 3. Execution Practices

### Context Recovery

At the start of each conversation or after a context reset:

1. **Re-read the plan, if one exists to restore the task definition and Phase structure.
2. **Re-read the progress notes, if they exist to determine which Phase is current and what has already been tried.
3. **Re-read relevant docs** (`difftest/docs/`) if the task involves unfamiliar modules.

Use the available task notes and repository state as the source of truth; do not invent missing progress.

### Confirming Ambiguities

When encountering unclear requirements, design choices, or unexpected test results:

- Ask the user only when a decision cannot be resolved from the repository, issue, or task context.
- Prefer asking early (before implementing a speculative fix) over asking late (after a failed debugging cycle).
- Typical situations: scope clarification, which approach to take when multiple are viable, whether a failing test is a known issue, whether to proceed to next Phase despite a partial result.

### End-of-Conversation Check

At the end of a task, report open questions and next steps in the issue/PR or conversation when relevant. No follow-up document or prompt is required.



### Sub-Agent Delegation

Delegate the following to sub-agents (e.g. Explore agent) to avoid filling the main conversation context window:

| Task | Why Delegate |
|------|-------------|
| Reading and analyzing log files (mismatch logs, build logs) | Logs are large and detailed; extracting the root cause is a focused subtask |
| Query DB inspection (sqlite3 queries, cross-DB comparisons) | Involves multiple queries and iterative interpretation |
| Waveform analysis hypotheses | Requires reading signal traces and correlating with RTL logic |
| Large-scale code reading for audit or review | Reviewing many files produces verbose context |

The sub-agent should return only the **conclusion and key evidence** (e.g. "mismatch at step 4482, field NFUSED: DUT=65, REF=64, caused by ..."), not raw query output.

---

## 4. Debugging Escalation

When a test fails, follow this escalation order:

1. **Console output** — read the last ~100 lines for checker name, cycle, and DUT/REF state.
2. **Query DB comparison** — convert to hex, compare with reference DB using `ATTACH`. Identify the first divergent STEP.
3. **Waveform** — rebuild with `EMU_TRACE=fst`, dump the suspect time range, inspect signal transitions.

At each level, form a hypothesis before escalating. If the hypothesis can be verified without the next level, do so.

Detailed command references: see [test.md §3 (Debugging Artifacts)](./test.md#3-debugging-artifacts) and [test.md §4 (Debug Workflow)](./test.md#4-debug-workflow).
