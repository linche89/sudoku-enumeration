# Exact Sudoku Enumeration

Exact, reproducible Sudoku enumeration in C++20 and Python: verified 9x9
and 2xC counts for C = 2--6, with research toward larger grids.

This project counts **labelled completed grids**, not equivalence classes of
puzzles. For `N(C)`, the grid is `2C x 2C` with `2 x C` boxes; transposing the
boxes gives the same count. Ordinary 9x9 Sudoku has 3x3 boxes and is a
separate reproduction track.

[Current status](STATUS.md) · [Reproduce and verify](docs/reproducibility.md) ·
[Methods and evidence](docs/index.md) ·
[C6 certificate downloads](https://github.com/linche89/sudoku-enumeration/releases/tag/c6-verified-2026-09-07)

## Verified results

| Boxes | Grid | Exact labelled count |
|---|---|---:|
| 2x2 | 4x4 | 288 |
| 2x3 | 6x6 | 28200960 |
| 2x4 | 8x8 | 29136487207403520 |
| 2x5 | 10x10 | 1903816047972624930994913280000 |
| 2x6 | 12x12 | 38296278920738107863746324732012492486187417600000 |
| 3x3 | 9x9 | 6670903752021072936960 |

The C6 computation was completed on September 7, 2026. All 63,199 outer
classes are present, with independently checked representatives, orbit
weights, coverage, and arbitrary-precision summation. Two final contractions
produced identical class tables. They share the same engine and closed F5
input: this is not a second independent evaluation of every F5 value.

The C6 decimal was announced by **Kjell Fredrik Pettersen in 2006**. Our
contribution is an independent implementation and reproducible verification,
not discovery of a new integer. The 9x9 work reproduces the
**Felgenhauer--Jarvis** enumeration. See the
[historical-source audit](docs/reports/og2/literature-audit-20260712.md) and
[FJ9 reproduction](docs/fj9/reproduction.md).

## Repository map

- `src/` — primary C=2--5 and FJ9 engines; see [the source map](src/README.md).
- `scripts/` — Windows build, verification, and guarded-run helpers.
- `docs/` — methods, math, reports, raw expert material, and history.
- `data/golden/` — small tracked verification data.
- `data/checkpoints/` — ignored binary checkpoints plus a tracked manifest.
- `data/logs/` — ignored transient output.
- `experiments/` — the verified hybrid C6 implementation and structural
  checkers, bounded decision prototypes, and legacy kernels. See
  [the prototype map](experiments/proto/README.md) for their different scopes.
- `reference/` — immutable external material and verification fixtures.
- `paper/` — manuscript work, not a claim of publication or peer review.
- `build/` — generated binaries; ignored.

## Quick start

The tested build environment is Windows x86-64, PowerShell, a MinGW-compatible
`g++` with C++20/OpenMP, and Python 3.13. The existing FJ9 build uses
BMI/BMI2/POPCNT/LZCNT instructions; it is not a portable baseline for all CPUs.
Standalone Python certificate checks require only the standard library.

```powershell
git clone https://github.com/linche89/sudoku-enumeration.git
Set-Location sudoku-enumeration
powershell -NoProfile -ExecutionPolicy Bypass -File scripts\build_og2.ps1
.\build\factorization_orbit.exe 5 pivot rooted4
```

The last command computes `N(5)` from scratch. Do not replace `5` with `6`:
the successful C6 method is a different, guarded multi-stage workflow.

Do not add `-march=native` on the documented Windows/MinGW setup; see the
[runbook](docs/runbooks/og2.md).

## Verification

Complete repository gate, including rebuilding the active programs:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts\verify_all.ps1
```

The gate rebuilds active binaries, reproduces the full 9x9 value, checks all
short independent OG-2 engines, runs the factorization C=2..5 exact gates,
exercises the layer-DP exact/checkpoint recovery gate, and optionally verifies
the C=6 memo read-only when it is present. That optional reload is skipped on
a fresh clone without the checkpoint. The gate does **not** recompute all C6
factorization values.

For a faster inner development loop:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts\verify_og2.ps1
```

## Research artifact policy

The [C6 certificate release](https://github.com/linche89/sudoku-enumeration/releases/tag/c6-verified-2026-09-07)
contains the complete final CSV and standalone verification programs. Its
[instructions](docs/releases/c6-certificate-README.md) distinguish checking
the final table from recomputing its factorization values. No production
checkpoint is needed to check the table or its exact weighted square sum.

Large checkpoints and bulk logs are never committed to ordinary Git. Every
important checkpoint needs a tracked manifest containing its size, SHA-256,
entry count, compatible code revision, and semantic coverage. Raw logs may be
removed only after their conclusions are captured in a dated report.

## Runtime and generalization

Given an already closed F5 catalogue, one measured final contraction took
**about 22 minutes**. The two-run finalization with checks and backups took
**55 minutes 19 seconds**. Neither figure includes earlier F4/F5 computation
or support construction. See the
[C6 finalization report](docs/reports/og2/c6-finalization-20260907.md).

The engines count ordered perfect-matching decompositions of regular
bipartite incidence graphs. The terminal total is a weighted sum of squares
of band-completion counts. The successful C6 route shares F4 values between
graph-equivalent states while retaining their distinct native responses,
then uses reverse F5 evaluation and the final layer contraction.

The identities extend beyond C6; the demonstrated runtime does not.
[Higher-C mathematics](docs/math/higher-c-structure.md) records proved
formulas, finite certificates and explicit-table size barriers. **No fast
complete N(7), N(8), or N(9) algorithm has been demonstrated here.**

## Provenance and licensing

The former repository name was `sudoku_FJ`. Historical paths and command
receipts intentionally retain it; the local checkpoint tree was not renamed.
Raw expert responses are research inputs, not established mathematical facts.

Historical attribution and third-party notices remain applicable. No blanket
license has been selected for this repository; public access alone does not
grant a new license to the retained third-party material.
