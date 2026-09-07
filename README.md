# Exact Sudoku Enumeration

Exact, reproducible Sudoku enumeration in C++20 and Python: verified 9x9
and 2xC counts for C = 2--6, with research toward larger grids.

This project counts **labelled completed grids**, not counts modulo Sudoku
symmetries. For `N(C)`, the grid is `2C x 2C` with `2 x C` boxes; transposing the
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

## Verify C6 without the large computation

Download `sudoku-c6-certificate-2026-09-07.zip` from the
[C6 certificate release](https://github.com/linche89/sudoku-enumeration/releases/tag/c6-verified-2026-09-07),
extract it to a new directory, and open a terminal there. The roughly 1 MB
archive contains the **complete** 63,199-class table and standalone checks.
Only Python is needed; no compiler, network or production checkpoint is used
by these checks. Python 3.13 was tested.

```text
python s4_certificate_verify.py c6-final.csv --c 6 --classes 63199 --expect-n 38296278920738107863746324732012492486187417600000 --quiet
python s4_exact_sum.py c6-final.csv --classes 63199 --expect-n 38296278920738107863746324732012492486187417600000
```

Require exit code zero and `CERTIFICATE PASS` / `PASS`. These commands check
representatives, weights, coverage, anchors and the exact final sum; they
**do not recompute every band-completion value** in the table. The
[bundle instructions](docs/releases/c6-certificate-README.md) include a third,
independent PowerShell/.NET sum and explain the verification boundary.

## Recompute C=2--5 from source

The tested build environment is Windows x86-64, PowerShell, a MinGW-compatible
`g++` with C++20/OpenMP, and Python 3.13. The existing FJ9 build uses
BMI/BMI2/POPCNT/LZCNT instructions; it is not a portable baseline for all CPUs.
The verification commands in the previous section do not need this toolchain.

```powershell
git clone https://github.com/linche89/sudoku-enumeration.git
Set-Location sudoku-enumeration
powershell -NoProfile -ExecutionPolicy Bypass -File scripts\build_og2.ps1
.\build\factorization_orbit.exe 5 pivot rooted4
```

The last command computes `N(5)` from scratch. Do not replace `5` with `6`:
the successful C6 method is a different, guarded multi-stage workflow.
The expected C5 total is `1903816047972624930994913280000`. Use arguments
`2`, `3` or `4` for the smaller cases in the table above.

Do not add `-march=native` on the documented Windows/MinGW setup; see the
[runbook](docs/runbooks/og2.md).

## Run the regression tests

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

## How C6 was computed, and what the timing means

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

For each terminal class q, m is its coordinate orbit size, ell its labelled
multiplicity, and F its band-completion count. The exact total is

    N(C) = sum over q of m(q) * ell(q) * F_C(q)^2.

For the derivation and the large-input workflow, start with the
[reproducibility guide](docs/reproducibility.md),
[graph-value sharing method](docs/math/native-graph-value-sharing.md) and
[final-certificate specification](docs/methods/layer-dp-certificate.md).

### Beyond C6

The identities extend beyond C6; the demonstrated runtime does not.
[Higher-C mathematics](docs/math/higher-c-structure.md) records proved
formulas, finite certificates and explicit-table size barriers. **No fast
complete N(7), N(8), or N(9) algorithm has been demonstrated here.**

## Repository layout and data

- [`src/`](src/README.md) — primary C=2--5 and FJ9 engines.
- [`experiments/proto/`](experiments/proto/README.md) — the verified hybrid
  C6 engines, independent checkers and separately scoped research prototypes.
- `scripts/` — build, regression, packaging and guarded-run helpers.
- [`docs/`](docs/index.md) — methods, mathematics and dated evidence;
  expert inputs and historical notes are not authoritative current results.
- [`data/`](data/README.md) — small tracked fixtures and checkpoint manifests;
  large checkpoints and execution logs are excluded from Git.
- [`reference/`](reference/README.md) — external material and FJ9 fixtures.
- `paper/` — manuscript drafts, not a claim of publication or peer review.
- `build/` — ignored generated binaries and local verification artifacts.

The final C6 table is distributed as a release asset, not as a large Git
blob. The intermediate F4/F5 data are **not included in a clone or the ZIP**.
Historical E: and D: paths in runbooks refer to the original workstation;
do not use its root-level production launchers as a fresh-clone quick start.

## Provenance and licensing

Raw expert responses are research inputs, not established mathematical facts.

Historical attribution and third-party notices remain applicable. No blanket
license has been selected for this repository; public access alone does not
grant a new license to the retained third-party material.
