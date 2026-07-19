# Connectivity double-permanent decision

Date: 2026-07-19 (Asia/Singapore)

## Scope

This report closes the bounded decision experiment requested by `STATUS.md`:
retain a local Windows implementation of the verified connectivity state,
add the proposed two-subset operator-valued double-permanent transition,
compare it with direct permutation-pair enumeration at C=2..4, and measure
bounded C=5 frontiers.  It does not complete C=5, run C=6, enumerate outer
classes, or access a graph checkpoint.

The retained source is:

```text
experiments/proto/connectivity_operator.cpp
SHA-256 = C22D4840A9A30F81A47984BC08E4D12CF1139027A4CBBD2ED10B962989615239
```

The expert sources under `docs/expert/2026-07-17/` use Boost
Multiprecision.  The local MinGW installation did not provide that header, so
the decision prototype removes this wrapper dependency and uses checked
`unsigned __int128`; all mandatory values through N(5) fit in that type.  It
preserves the supplied connectivity definition and adds two independently
selectable transition kernels.

## Exact implementations

The `direct` kernel enumerates all pairs of color permutations for a fixed
source.  It constructs every labelled raw target and its cycle factor.  The
`subset` kernel instead processes bands one at a time while storing the
partial connectivity state and the two used-color masks.  `differential`
runs both and requires exact equality of every raw target coefficient before
canonical reduction.

All accumulations and products are overflow checked.  C=5 beyond symbol 3 is
accepted only with `subset` or `differential`, a positive `sourceprobe`, and
explicit positive operator state and record limits.  The program refuses
C=6.  A source probe remains marked partial even when its interval happens to
cover the entire current source layer.

## Complete small-C differential gates

The prototype was built on Windows/MinGW with:

```powershell
g++ -O3 -mpopcnt -std=c++20 -Wall -Wextra -fopenmp `
  experiments\proto\connectivity_operator.cpp `
  -o build\connectivity_operator.exe
```

Then the complete direct/subset differential was run:

```powershell
.\build\connectivity_operator.exe 2 differential
.\build\connectivity_operator.exe 3 differential
.\build\connectivity_operator.exe 4 differential
.\build\connectivity_operator.exe 4 differential noswap
```

Every source-to-raw-target coefficient agreed.  The state layers and totals
were:

| C | connectivity layers | result |
|---:|---|---:|
| 2 | `1,1,3,1,1` | 288 |
| 3 | `1,1,8,18,19,1,1` | 28200960 |
| 4 | `1,1,28,700,12856,9708,155,1,1` | 29136487207403520 |

With copy exchange disabled, the C=4 layers were
`1,1,43,1290,25046,18775,234,1,1` and the total was unchanged.  No subset
prefix merged in any complete small-C run.

## C=5 initializer and third symbol

The optimized two-symbol initializer was compared with a deliberately slow
direct construction under a 4-GiB, two-minute guard:

```powershell
& .\scripts\watch_rss.ps1 `
  -Exe .\build\connectivity_operator.exe `
  -Arguments @('5','stop=2','subset','initcheck') `
  -LimitGB 4 -MaxMinutes 2 -IntervalSeconds 1
```

It reported `init2_check=1 states=93`.  The guard completed in 55.6 seconds;
the one-second RSS maximum was 7,475,200 bytes.

The full third-symbol differential command was:

```powershell
.\build\connectivity_operator.exe 5 stop=3 differential
```

Its exact output was:

```text
source states              = 93
target states              = 83776
valid moves                = 857244
raw support                = 857244
sum per-source canonical   = 273518
subset generated records   = 2056501
subset merged prefixes     = 0
maximum subset frontier    = 14400
transition time            = 2.16107 s
```

The elapsed time is a local observation, not a portable bound.

## Bounded fourth-symbol probes

Six individual source indices were spread across the sorted 83,776-state
frontier.  Direct and subset coefficients agreed for every raw target:

| source | legal/raw targets | canonical targets | subset peak | band-prefix frontiers |
|---:|---:|---:|---:|---|
| 0 | 2,809 | 2,805 | 4,096 | `20,256,1521,4096,2809` |
| 10,000 | 7,488 | 7,486 | 9,360 | `25,400,2880,9360,7488` |
| 20,000 | 2,809 | 2,805 | 4,096 | `20,256,1521,4096,2809` |
| 40,000 | 4,992 | 4,977 | 7,488 | `25,320,2340,7488,4992` |
| 60,000 | 6,144 | 6,110 | 9,360 | `25,400,2880,9360,6144` |
| 83,775 | 6,144 | 6,142 | 7,488 | `25,320,2304,7488,6144` |

For every row, the number of unique subset states at each of the five band
boundaries exactly equalled the number of legal placement prefixes.

Three deterministic 100-source intervals were then run with a 4-GiB,
two-minute external guard and per-source limits of 20,000 operator states and
100,000 generated records:

| source interval | legal/raw targets | sum canonical | generated records | max peak | merged global targets |
|---|---:|---:|---:|---:|---:|
| `[0,100)` | 314,961 | 314,360 | 969,034 | 6,360 | 295,202 |
| `[40000,40100)` | 519,912 | 515,622 | 1,526,928 | 9,216 | 500,832 |
| `[80000,80100)` | 607,890 | 604,278 | 1,738,862 | 14,400 | 592,275 |
| total | 1,442,763 | 1,434,260 | 4,234,824 | 14,400 | not additive |

All 300 direct/subset raw maps agreed.  Across their 1,500 band-prefix
boundaries, unique frontier size equalled legal placement paths every time.
The completed canonical/raw ratio was 99.410645%; individual ratios ranged
from 50.624589% to 100%.  A final rerun of the middle interval with explicit
merge instrumentation again reported zero merged prefixes and a one-second
RSS maximum of 119,365,632 bytes.

The exact bounded command for that final interval was:

```powershell
& .\scripts\watch_rss.ps1 `
  -Exe .\build\connectivity_operator.exe `
  -Arguments @('5','stop=4','differential',`
               'sourcestart=40000','sourceprobe=100',`
               'maxoperatorstates=20000',`
               'maxoperatorrecords=100000') `
  -LimitGB 4 -MaxMinutes 2 -IntervalSeconds 1
```

## Structural diagnosis

Fix a raw source state.  After processing band `i`, exactly one left and one
right degree in that labelled band have increased by one.  Comparing the
partial target degree vector with the source therefore recovers the chosen
color pair for every processed band.  The raw partial state determines the
entire assignment prefix.  Live-path pairing and deletion of completed cycles
cannot erase this degree difference.

It follows that the implemented subset recurrence is injective before the
final group quotient.  The observed zero-merge counts are required by the
state definition, not an unlucky sample.  Full target canonicalization is too
late to help and is itself weak on the sampled generic sources.

Canonicalizing a partial target alone under the full group would not be an
exact repair: that group also moves the fixed source and the processed-band
order.  A correct symmetry reduction would have to quotient the joint source
and partial target, typically under the source stabilizer, or use a different
operator transform that never materializes the labelled degree differences.

## Decision

The naive operator-valued double-permanent subset DP is exact and fails its
C=5 compression gate.  It changes traversal order but enumerates the same
legal assignments injectively.  It is therefore rejected as the next global
C=6 implementation.

This decision does not reject the connectivity quotient, every possible
source-stabilizer or representation-theoretic operator, or a future compact
double-permanent transform.  None of those alternatives currently has a
defined sufficient state or a measured scale advantage.  The next bounded
engineering result is later-class coverage of the existing G1/G2 half-kernel
table plus repetition measurements for kernel-key pairs and relative
transforms; no full 63,199-class run is authorized.
