# sudoku_FJ

Independent C++20 reproduction and extension work around the Felgenhauer-Jarvis Sudoku enumeration.

The repository has two active tracks:

- Reproduce the FJ05 9x9 Sudoku terminal-grid count, including the 71-class reduction and independent cross-checks.
- Explore exact counts for 2 x C Sudoku boxes, with C=2..5 validation gates and C=6 research notes.

## Repository Map

- `src/` - C++ engines, validators, profilers, and experimental kernels.
- `scripts/` - PowerShell build, verification, and RSS-guarded run helpers.
- `docs/` - research logs, frontier notes, and the OG-2 runbook.
- `data/` - curated run outputs, golden tables, progress records, and retained measurement data.
- `reference/` - original papers, source snapshots, OEIS/forum material, and verification fixtures.
- `experiments/proto/` - archived prototype code and differential tests referenced by the handoff notes.
- `build/` - generated binaries only; ignored and safe to recreate.

Root documents:

- `FJ_sudoku.md` - main 9x9 reproduction write-up.
- `HANDOFF.md` - current 2 x C relay state and lessons learned.
- `milestone.md` - completed milestone history.
- `plan.md` - original implementation plan.

## Build

On Windows with MinGW `g++`:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts\build_og2.ps1
```

For the original FJ05 reproduction tools, a POSIX-like shell can use:

```bash
./build.sh
```

Do not use `-march=native` on this Windows/MinGW setup; the scripts intentionally use narrower CPU flags.

## Verification

Short OG-2 gate:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts\verify_og2.ps1
```

This verifies the known C=2, C=3, and C=4 values and runs the differential checks documented in `docs/OG2_runbook.md`.

## Retention Policy

Keep the curated `data/`, `reference/`, `docs/`, `scripts/`, `src/`, and `experiments/proto/` content. Generated binaries, root scratch `*.err` files, transient RSS/output logs, and the old root `proto/` scratch directory are ignored.
