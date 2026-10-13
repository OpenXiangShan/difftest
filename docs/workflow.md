# DiffTest Task Workflow

This document defines the standard workflow for complex difftest tasks — those involving multi-file changes, pipeline modifications, or iterative debugging. Simple one-shot edits do not require this process.

## Overview

```
Task received
  │
  ▼
Maintain a log
  │
  ▼
┌─ Per Phase ──────────────────────────┐
│  Re-read the log                    │
│  │                                   │
│  ▼                                   │
│  Implement changes                   │
│  │                                   │
│  ▼                                   │
│  Test (microbench → linux)           │
│  │                                   │
│  ├─ PASS → update log, next          │
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

## 1. Task Log

For complex tasks, maintain a log of changes, test results, and debugging progress. Update it as each phase completes, including failed attempts and relevant commands, metrics, and error messages.

---

## 2. Execution Practices

### Context Recovery

At the start of each conversation or after a context reset:

1. Re-read the log to restore the task definition, current phase, and previous attempts.
2. **Re-read relevant docs** (`difftest/docs/`) if the task involves unfamiliar modules.

Use the log and repository state as the source of truth.

### Confirming Ambiguities

When encountering unclear requirements, design choices, or unexpected test results:

- Ask the user only when a decision cannot be resolved from the repository, issue, or task context.
- Prefer asking early (before implementing a speculative fix) over asking late (after a failed debugging cycle).
- Typical situations: scope clarification, which approach to take when multiple are viable, whether a failing test is a known issue, whether to proceed to next Phase despite a partial result.

### End-of-Conversation Check

At the end of a task, report the result, open questions, and next steps when relevant.

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

## 3. Debugging Escalation

When a test fails, follow this escalation order:

1. **Console output** — read the last ~100 lines for checker name, cycle, and DUT/REF state.
2. **Query DB comparison** — convert to hex, compare with reference DB using `ATTACH`. Identify the first divergent STEP.
3. **Waveform** — rebuild with `EMU_TRACE=fst`, dump the suspect time range, inspect signal transitions.

At each level, form a hypothesis before escalating. If the hypothesis can be verified without the next level, do so.

Detailed command references: see [test.md §3 (Debugging Artifacts)](./test.md#3-debugging-artifacts) and [test.md §4 (Debug Workflow)](./test.md#4-debug-workflow).
