# DiffTest Documentation

## Document Index

| Document | Contents |
|----------|----------|
| [layout.md](./layout.md) | Project directory structure, key files, modification guide |
| [hw-flow.md](./hw-flow.md) | Hardware transport pipeline: Preprocess → Squash → Delta -> Batch → Sink |
| [sw-check.md](./sw-check.md) | Software checking flow: difftest_step, checkers, reference model, DPI-C |
| [test.md](./test.md) | Build / run / debug commands: EMU, simv, FPGA Sim, reference DB comparison, phased verification |
| [workflow.md](./workflow.md) | Task workflow: plan/progress specs, execution practices, sub-agent delegation, debugging escalation |

## Recommended Reading Order

1. [layout.md](./layout.md) — Understand the overall project structure
2. [hw-flow.md](./hw-flow.md) — Understand the hardware-side data flow
3. [sw-check.md](./sw-check.md) — Understand the software-side checking logic
4. [test.md](./test.md) — Reference when building, running, or debugging
5. [workflow.md](./workflow.md) — Follow when executing complex multi-phase tasks
