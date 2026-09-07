# sudoku_FJ

Exact Sudoku enumeration research in C++20.

The repository contains three related tracks:

- a completed, independently verified reproduction of the Felgenhauer-Jarvis
  9x9 terminal-grid count;
- exact 2xC engines with complete C=2..5 gates and a fully computed,
  independently certificate-verified C=6 result;
- mathematical work on higher-C structure, using the complete 63,199-class
  C6 certificate as data (no fast complete C7--C9 route is yet established).

Read [STATUS.md](STATUS.md) first. It is the only authoritative statement of
current results and open problems. The full documentation map is
[docs/index.md](docs/index.md).

## Repository map

- `src/` — active engines and verified tools; see `src/README.md`.
- `scripts/` — Windows build, verification, and guarded-run helpers.
- `docs/` — methods, math, reports, raw expert material, and history.
- `data/golden/` — small tracked verification data.
- `data/checkpoints/` — ignored binary checkpoints plus a tracked manifest.
- `data/logs/` — ignored transient output.
- `experiments/` — decision prototypes, the verified hybrid C6 layer-DP
  implementation and structural checkers, plus legacy kernels.
- `reference/` — immutable external material and verification fixtures.
- `build/` — generated binaries; ignored and safe to recreate.

## Build

On Windows with MinGW `g++`:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts\build_all.ps1
```

Do not use `-march=native` on this Windows/MinGW setup. The scripts use the
known-safe scalar instruction flags for the FJ9 engine.

## Verification

Complete repository gate:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts\verify_all.ps1
```

The gate rebuilds active binaries, reproduces the full 9x9 value, checks all
short independent OG-2 engines, runs the factorization C=2..5 exact gates,
exercises the layer-DP exact/checkpoint recovery gate, and optionally verifies
the C=6 checkpoint read-only when it is present.

For a faster inner development loop:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts\verify_og2.ps1
```

## Research artifact policy

Large checkpoints and bulk logs are never committed to ordinary Git. Every
important checkpoint needs a tracked manifest containing its size, SHA-256,
entry count, compatible code revision, and semantic coverage. Raw logs may be
removed only after their conclusions are captured in a dated report.
