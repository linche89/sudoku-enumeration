# Reproducing and verifying the results

This guide is for a fresh clone of
[sudoku-enumeration](https://github.com/linche89/sudoku-enumeration).
[STATUS.md](../STATUS.md) is the authoritative state; dated reports preserve
the exact commands and observations from the original workstation.

## Choose the verification level

| Task | Inputs | What it establishes |
|---|---|---|
| Recompute C=2--5 and FJ9 | Tracked source and small reference fixtures | Fresh numerical results and differential/recovery tests |
| Check the complete C6 final table | Public certificate ZIP and Python | Legal, distinct representative orbits, exact weights, coverage, anchors and the final weighted sum |
| Recompute C6 factorization values | Large support catalogues, compute time and guarded runs | Numerical evaluation upstream of the published final table |

The second task does not replace the third. A correct sum and correct orbit
weights alone would not prove that every supplied F value is correct. The
full project's numerical evidence is recorded separately in the
[finalization report](reports/og2/c6-finalization-20260907.md).

## Fresh small-case computation

The tested production platform is Windows x86-64 with PowerShell, Python
3.13 and a MinGW-compatible C++20 compiler with OpenMP. The FJ9 build uses
BMI/BMI2/POPCNT/LZCNT and needs a compatible CPU. Do not add `-march=native`;
the project retains safe flags for the documented MinGW stack-alignment issue.
No Linux/macOS full-engine gate is claimed by this publication.

From the repository root:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts\build_og2.ps1
.\build\factorization_orbit.exe 2 pivot rooted4
.\build\factorization_orbit.exe 3 pivot rooted4
.\build\factorization_orbit.exe 4 pivot rooted4
.\build\factorization_orbit.exe 5 pivot rooted4
```

Expected totals are 288; 28200960; 29136487207403520; and
1903816047972624930994913280000, respectively. The complete regression is:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts\verify_all.ps1
```

It rebuilds the active programs and checks small-C totals and class data,
canonicalization/transition differentials, interrupted checkpoint recovery,
and the FJ9 total 6670903752021072936960. The optional G1 memo reload is
skipped when its ignored local checkpoint is absent. This is expected on a
fresh clone, not a failure and not a full C6 computation.

## Public C6 certificate

Download `sudoku-c6-certificate-2026-09-07.zip` from the
[C6 verified release](https://github.com/linche89/sudoku-enumeration/releases/tag/c6-verified-2026-09-07).
The archive contains the complete 63,199-row CSV, three standalone checkers,
its instructions and a small artifact manifest. The CSV is not a sample.

Follow [the standalone instructions](releases/c6-certificate-README.md).
All Python checks use the standard library and no network. They read only
the final CSV, not an F4/F5 checkpoint. The separate PowerShell checker uses
.NET BigInteger rather than floating-point arithmetic.

The terminal formula is

    N(C) = sum over q of m(q) * ell(q) * F_C(q)^2.

Here m is the signed box-coordinate orbit size, ell is the number of symbol
labellings after dividing repeated-word factorials, and F is the number of
ordered perfect-matching decompositions of the represented incidence graph.
The Python semantic checker derives m and ell from every representative,
checks uniqueness and verifies the total mass binomial(2C,C)^C. See the
[certificate specification](methods/layer-dp-certificate.md).

The sum is

    N(6) = 38296278920738107863746324732012492486187417600000.

Pettersen announced the same integer in 2006. The contribution here is an
independent implementation and verification, with historical priority and
the inherited outer square-sum method explicitly acknowledged in the
[source audit](reports/og2/literature-audit-20260712.md).

## Full C6 recomputation is a separate workload

The successful chain reused complete support catalogues, shared F4 values
under graph isomorphism without discarding native responses, computed F5 by
the reverse recurrence, and applied the final global transition. It used
903,398,603 native F4 states, 140,069,579 shared F4 values and 96,452,755
closed native F5 values. The large intermediate binary files are not in Git
or the final-table release.

Given closed F5, the two measured final transitions took 1321.670 and
1331.336 seconds. The two-run finalization, including its gate, checks and
backups, took 3319.0009196 seconds. None includes earlier support generation
or F4/F5 computation. This publication does not offer a checkpoint-free,
22-minute N(6) computation.

The [checkpoint manifest](../data/checkpoints/MANIFEST.md),
[shared-value mathematics](math/native-graph-value-sharing.md),
[shared-F4 runbook](runbooks/layer-shared-c6.md),
[reverse-F5 runbook](runbooks/layer-reverse-c6.md), and
[final-stage runbook](runbooks/layer-dp-c6-production.md) document that chain.
Their historical E: and D: paths refer to the original machine, not to files
automatically supplied by cloning this repository. Do not run the
operator-specific root launchers as a fresh-clone quick start.

Any new large run needs explicit time/memory bounds, checkpoint backups and
the repository safety gates. Publishing the project does not start or
authorize a new production run. No fast complete C7--C9 method is claimed.

## Maintaining the public bundle

From a checkout with the verified final CSV, the maintainer can create a
new archive at an unused output path:

```powershell
python scripts\package_c6_certificate.py --csv PATH_TO_FINAL.csv --output build\sudoku-c6-certificate-2026-09-07.zip
```

The packager rejects a different CSV and refuses to overwrite an existing
archive. It preserves the CSV bytes and bundles the existing checkers; it
does not recalculate or alter any production state. Generated archives stay
outside ordinary Git history and are distributed as release assets.
