# Current Project Status

Last updated: 2026-09-07

This is the only authoritative current-state page. Earlier reports, expert
inputs and the [archived development status](docs/history/status-through-c6-verification-20260907.md)
preserve provenance but do not override the verified results below.

## Complete C6 result

The full C6 computation is **CLOSED AND VERIFIED**. The computational
objective is 100% complete; no further F4, F5 or final-stage run is needed.

    N(6) = 38296278920738107863746324732012492486187417600000
    final outer classes = 63199
    labelled outer mass = 622345892187672576

This independently reproduces Pettersen's previously announced value, not a
new discovery of its decimal expansion. Historical sources are recorded in
[the literature audit](docs/reports/og2/literature-audit-20260712.md).

The completed route is support reuse -> shared graph F4 -> reverse native F5
-> final global contraction. Its closed inventories are:

| Layer or object | Complete inventory |
|---|---:|
| Native F4 support | 903,398,603 live states |
| Shared F4 graph values | 140,069,579 |
| Native F5 support and closed values | 96,452,755 live states |
| Stable F5 IDs, including 221 insertion holes | 96,452,976 |
| Final native classes | 63,199 |

All live F5 values were closed, independently storage/lineage audited,
exported with an independent all-record weighted readback, and physically
backed up. Both fresh final contractions produced byte-identical CSVs.
Independent Python representative/orbit/multiplicity/coverage verification,
Python/.NET arbitrary-precision sums, first-moment checks, four Latin6
anchors, positivity and 6! divisibility checks passed.

The two contractions use the same engine and closed F5 input. Their
agreement is a replay check, not a second independent numerical evaluation
of every F5. Certificate checks do not recompute every final factorization
value. The precise scope, source revision, commands and evidence are in
[the C6 finalization report](docs/reports/og2/c6-finalization-20260907.md).

## Runtime, with its prerequisites

- Each final contraction emitted 5,563,295,272 records. Transition times were
  1321.670 and 1331.336 seconds, from an already closed F5 input.
- The complete two-run controller, including the fresh full gate, independent
  certificate/sums and backups, took 3319.0009196 seconds; sampled aggregate
  peak memory was 4,806,529,024 bytes, with no surviving child.
- These are not from-scratch C6 runtimes: earlier support construction,
  shared-F4 evaluation and reverse-F5 evaluation are excluded.

## Other verified counts

| Problem | Exact labelled count |
|---|---:|
| FJ9, 9x9 with 3x3 boxes | 6670903752021072936960 |
| N(2) | 288 |
| N(3) | 28200960 |
| N(4) | 29136487207403520 |
| N(5) | 1903816047972624930994913280000 |
| N(6) | 38296278920738107863746324732012492486187417600000 |

The FJ9 reproduction agrees with all 71 reference classes. Complete C=2--5
gates compare independent engines and per-class data. The primary small-C
engine is [factorization_orbit.cpp](src/factorization_orbit.cpp); C5 timing
and controlled ablations are in
[the C5 report](docs/reports/og2/c5-paper-ablation-20260712.md).

## Higher-C research

There are proved all-C Latin-family and separator-response formulas, but no
demonstrated fast complete N(7), N(8) or N(9) route. An explicit penultimate
table at C7 has at least 7,407,067,568,369 native orbits; an eight-byte payload
per orbit alone needs 59,256,540,546,952 bytes. This is a barrier for that
explicit representation, not a lower bound on every scalar algorithm.

Terminal unpaired-graph caching saves at most a factor two and saves only
146 of the 63,199 C6 classes. These statements do not invalidate sharing F4
values while preserving native responses. Proofs, exact finite checks and
their limitations are in [the higher-C note](docs/math/higher-c-structure.md).

## Code and retained state

- [Source map](src/README.md): FJ9, primary C=2--5 engines and shared helpers.
- [Prototype map](experiments/proto/README.md): the verified hybrid C6 route,
  bounded research tools and independent final-table checks.
- [Checkpoint manifest](data/checkpoints/MANIFEST.md): every retained input,
  closed export, final receipt and physical backup.
- [Final certificate receipt](data/golden/og2-c6-final-certificate-receipt.json):
  source revision, complete final CSV and precise verification evidence.

The original forward S1 generation-37 image remains partial and preserved;
it must not be mistaken for the closed shared-F4 export. The closed reverse-F5
input is c6_reverse_f5_closed_20260907.L5.snap. Large checkpoints and bulk
logs are excluded from Git. Read-only verification remains mandatory;
publication is not permission to restart production. Existing local paths
and physical backups are unchanged.

## Public repository

The project is published as
[linche89/sudoku-enumeration](https://github.com/linche89/sudoku-enumeration),
preserving its existing Git history and historical attribution. The public entry points
are [README.md](README.md), [the reproducibility guide](docs/reproducibility.md)
and [the documentation index](docs/index.md). Earlier operator-specific
paths are provenance, not fresh-clone instructions.

No algorithm, canonicalization rule or checkpoint format was changed for
publication. The full former status page is retained under docs/history/;
uncommitted manuscripts and unrelated local probes are not part of this
publication.

The [C6 certificate release](https://github.com/linche89/sudoku-enumeration/releases/tag/c6-verified-2026-09-07)
contains the complete final table and standalone checks, without the large
F4/F5 checkpoints. Both the original workspace and a clean Windows clone
passed the full regression; publication fixed only a C5 fixture's LF checkout
rule, not its data. The public ZIP was downloaded and matched the tested
payload. See [the publication report](docs/reports/repository-publication-20260907.md).
