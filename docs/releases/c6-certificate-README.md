# Complete C6 Sudoku final-table certificate

This archive accompanies the sudoku-enumeration C6 verification release:
https://github.com/linche89/sudoku-enumeration/releases/tag/c6-verified-2026-09-07

The project independently reproduced Pettersen's 2006 announced value:

    N(6) = 38296278920738107863746324732012492486187417600000

This is the number of labelled completed 12x12 Sudoku grids with 2x6 boxes
(equivalently, after transposition, 6x2 boxes), not the number modulo symmetry.

## Contents

- `c6-final.csv`: all 63,199 terminal classes, not a sample.
- `s4_certificate_verify.py`: independent representative/weight/coverage check.
- `s4_exact_sum.py`: standalone Python arbitrary-precision weighted sum.
- `s4_exact_sum.ps1`: separate PowerShell/.NET BigInteger weighted sum.
- `manifest.json`: content sizes and SHA-256, source revision and expected data.

The original CSV is 4,766,612 bytes, with SHA-256
`84F2720E7BA8296D78604153B934B7291AF93C329C07F5D46538F9D7101127C7`.

## Run

Extract the ZIP to a new directory and open a terminal there. Python 3.13
was tested; no third-party Python modules or network access are needed.
Run each command separately and require exit code zero:

```text
python s4_certificate_verify.py c6-final.csv --c 6 --classes 63199 --expect-n 38296278920738107863746324732012492486187417600000 --quiet
python s4_exact_sum.py c6-final.csv --classes 63199 --expect-n 38296278920738107863746324732012492486187417600000
```

On Windows, also run the independent .NET implementation:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File s4_exact_sum.ps1 -Dump c6-final.csv -Classes 63199 -ExpectN 38296278920738107863746324732012492486187417600000
```

The Python semantic verifier reports `CERTIFICATE PASS`; the independent
sums report `PASS` for the expected total. The row count must be 63,199 and
the labelled outer mass must be 622345892187672576. A nonzero exit code is a
failure, regardless of whether some earlier lines look correct.

## Meaning and limits

Each row has representative words, a coordinate orbit size m, a labelled
multiplicity ell, and a band factorization count F. The exact contraction is

    N(6) = sum(m * ell * F^2).

The semantic verifier independently checks legality, canonicality,
stabilizers, m, repeated-word factorials, ell, uniqueness, ordering, total
coverage, and the G1/G2 value-to-representative bindings. It does not call the
C++ production engine. The two other scripts independently recompute the
weighted integer sum.

These are checks of a published final table, **not independent recomputation
of every F value**. Numerical F4/F5 evaluation, complete small-case gates,
two final contractions, and additional first-moment/Latin-family checks are
documented in the source repository. The two final contractions shared
their engine and closed F5 input. No whole-pipeline double-independence or
from-scratch minute-scale runtime is claimed.

Large F4/F5 checkpoints and production executables are intentionally not
included. Historical attribution and verification details:
https://github.com/linche89/sudoku-enumeration/blob/main/docs/reports/og2/c6-finalization-20260907.md
https://github.com/linche89/sudoku-enumeration/blob/main/docs/reports/og2/literature-audit-20260712.md
