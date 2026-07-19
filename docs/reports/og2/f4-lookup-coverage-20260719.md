# C=6 F4-Lookup Coverage Decision — 2026-07-19

## Question

Could the historically motivated `2+4` identity be made cheap by enumerating
two-factors `H` of each outer graph `Q`, then looking up the degree-four
residual `Q-H` in one shared graph-memo table?

The identity is exact, but its class-local form is not a new recurrence.  Two
successive applications of the primary engine's rooted matching recurrence

```text
F_d(G) = d * sum_(M contains e) F_(d-1)(G-M)
```

are the ordered version of choosing `H=M1 union M2`; the `F2(H)` factor
accounts for its ordered one-factorizations.  The open scale questions were
therefore:

1. how many uncolored spanning two-factors does a C=6 outer graph contain;
2. how much does rooted isomorphism aggregation reduce the work; and
3. do the resulting canonical F4 residual keys reuse the existing G1/G2
   checkpoint or one another across outer classes?

## Implementation and safety

Commit `b99de7624359cc8d77054f8960bd305ca618e920` adds a bounded diagnostic to
`src/factorization_orbit.cpp`:

- `f4coveragecheck` performs the complete two-level split at C=4 or C=5 and
  compares every selected class with the existing exact engine;
- `f4coverageparents=N` performs deterministic sampling of sorted canonical
  degree-five parents at C=6;
- `f4coveragemaxstates=N` and `f4coveragemaxrecords=N` are mandatory positive
  bounds; and
- C=6 additionally requires an existing `checkpoint=...`,
  `checkpointreadonly`, and positive `limit=`.  It refuses more than 64
  classes, 1,000 parents per class, 2,000,000 states, or 100,000,000 records.

The C=6 path computes the exact number of uncolored spanning two-factors by a
ternary right-degree DP, but it does not compute `F6`, insert graph-memo
values, or accumulate `N(6)`.  It aborts if canonicalization falls back to a
weak key.  Separate negative tests confirmed refusal without
`checkpointreadonly`, refusal without bounds, and both record and state limit
stops.

Artifacts used for the measurements were:

```text
source SHA-256 = CE936C3474FF86A02AC3F1EFB50B8D201E42986626DAD3FECB1CBA50650D1FEF
gate SHA-256   = 8EB0022E8DBFA540BA8EE693DEB3893BD93FECDFE7D6E868D72FF184D12E74F4
binary SHA-256 = 59EE6515707966FC22E00E44AEEF215031CCF13A2A8F1A95CC87A2AC501F6023
checkpoint     = FE8B68DE6C15592848D7CF69BF43928C000F0B2EE59FC263A094BCA2D376A865
```

The checkpoint hash matched `data/checkpoints/MANIFEST.md` before and after
all probes.

## Exact correctness gates

The permanent short gate is:

```powershell
.\build\factorization_orbit.exe 4 f4coveragecheck `
  f4coveragemaxstates=100000 f4coveragemaxrecords=1000000 `
  pivot rooted4
```

All 26 C=4 classes passed the differential, and the weighted result was

```text
N(4) = 29136487207403520 [OK]
```

At C=5, a strong-canonical run closed classes 1--299 before its external time
guard stopped the process.  A fresh `start=286 limit=69` run closed classes
287--355 in 95.458 seconds with 0.517 GiB peak RSS.  Taking classes 1--286
from the first log and 287--355 from the second gives exactly 355 classes and

```text
N(5) = 1903816047972624930994913280000 [OK]
```

There was no class differential failure and no canonical fallback in either
covering interval.  This closes the mathematical and normalization gate for
the diagnostic independently of the C=6 measurements.

## C=6 measurements

The loaded checkpoint contains 5,315,962 exact graph-memo entries, including
complete G1 data and the first 50 G2 degree-five parents.  Each table row below
uses ten evenly selected canonical degree-five parents unless the class has
fewer.  `#H` is the exact number of uncolored spanning two-factors.  The last
column reports unique F4 checkpoint hits over unique sampled F4 keys.

| outer class | exact `#H` | degree-5 canonical parents | unique F4 hits/keys |
|---:|---:|---:|---:|
| 1 (G1) | 1,326,562,875 | 1,617 | 50,993 / 50,996 |
| 2 (G2) | 1,329,588,799 | 19,850 | 7,841 / 51,973 |
| 3 | 1,332,620,698 | 17,675 | 2,153 / 51,550 |
| 101 | 1,340,908,028 | 31,961 | 1,516 / 52,250 |
| 1,001 | 1,343,967,256 | 32,165 | 1,571 / 49,037 |
| 10,001 | 1,353,449,623 | 31,480 | 879 / 41,411 |
| 31,600 | 1,386,211,473 | 8,092 | 1,145 / 47,321 |
| 63,199 | 4,617,202,500 | 1 | 0 / 10 |

The three apparent G1 misses are historical safe weak-namespace entries in the
checkpoint; the probe deliberately queries only strong canonical keys, so its
coverage count is conservative.  G2's 15.1% sampled hit rate is already
inflated by the checkpoint's first 50 G2 parents.  Distributed later-class
coverage is only about 2--4%.

Doubling the sample gives the same picture:

| outer class | sampled parents | F4 occurrences | unique keys | unique hits |
|---:|---:|---:|---:|---:|
| 10,001 | 20 | 90,445 | 90,376 (99.924%) | 2,122 (2.348%) |
| 31,600 | 20 | 92,146 | 91,997 (99.838%) | 2,051 (2.229%) |

A ten-class block, classes 3--12 with ten parents each, produced:

```text
exact two-factor sum       = 13,381,083,472
per-class unique-key sum   = 516,100
cross-class key union      = 513,228
cross-class duplicate gain = 2,872 keys (0.5565%)
checkpoint union hits      = 28,299 / 513,228
```

The representative guarded command was:

```powershell
& .\scripts\watch_rss.ps1 `
  -Exe .\build\factorization_orbit.exe `
  -Arguments @(
    '6','start=2','limit=10',
    'f4coverageparents=10',
    'f4coveragemaxstates=1000000',
    'f4coveragemaxrecords=5000000',
    'canonbudget=10000000',
    'checkpoint=data\checkpoints\factorization_orbit_c6_graphmemo.bin',
    'checkpointreadonly') `
  -LimitGB 4 -MaxMinutes 3 -IntervalSeconds 1 `
  -LogPath data\logs\f4-coverage-c6-class3-12-p10-h2-final-20260719.rss.csv `
  -StdoutPath data\logs\f4-coverage-c6-class3-12-p10-h2-final-20260719.out `
  -StderrPath data\logs\f4-coverage-c6-class3-12-p10-h2-final-20260719.err
```

It used 3.199 seconds of count time and peaked at 0.577 GiB observed RSS.

## Decision

The proposed cheap class-local implementation fails its scale assumptions:

- ordinary C=6 classes have roughly 1.33--1.39 billion, not
  `10^5--10^6`, spanning two-factors;
- rooted aggregation still leaves up to about 32,000 canonical degree-five
  parents in the measured classes;
- each sampled generic parent produces thousands of F4 lookups, with more
  than 99.8% unique keys in the 20-parent checks; and
- the existing G1/G2 table covers only about 2--4% of later-class keys, while
  ten consecutive early classes gain only 0.56% from a cross-class union.

Thus class-local `2+4` plus the existing global F4 memo does not remove the
dominant enumeration.  Even a free lookup value would leave the per-class
two-factor/residual incidence work.

This is deliberately not an impossibility claim for every historical bulk
reverse-gluing algorithm.  Pettersen's reported lookup may have been generated
globally with a different incidence construction.  Such a route remains open
only if it specifies how to avoid both the bottom-up 1.761-billion
low-stabilizer placement list and the top-down roughly 1.3-billion two-factor
list per ordinary outer class.  No such construction is currently implemented
or authorized for a large C=6 run.
