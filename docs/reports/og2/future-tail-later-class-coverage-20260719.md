# Later-class half-kernel coverage decision

Date: 2026-07-19 (Asia/Singapore)

## Scope

This report closes the bounded engineering experiment requested by
`STATUS.md`: construct deterministic read-only samples from later C=6 outer
classes, test their seven-row half keys against the immutable G1/G2 table,
and measure repetition of complete kernel pairs and relative-transform
signatures.  It does not evaluate a tail contraction, produce an `F6` value,
write a checkpoint, or extrapolate a complete 63,199-class run.

The retained implementation is commit
`f7bc5ecf6b17bb9a3de87114620f046220497e17`:

```text
src/factorization_orbit.cpp
SHA-256 = A30A560FA9B164FC3C241ED0D1E2B10EF881656A91D3B1C33D1B032143142F4D

src/future_twin.hpp
SHA-256 = 05EFAAA7FD6FA6F6FEA11E354D25F17BEF213C539147A440ADC3DB8B890FD2F0

build/factorization_orbit.exe
SHA-256 = 2DA905571324757DCF690314CA2AB1E12A42D30657D70BE78C3E31A49F3DE8D5
```

The table was the existing committed G1/G2 union under
`data/logs/future-tail-kernel-table-c6-g1-g2-20260715`.  Its construction,
format, and complete G1/G2 rescans are independently recorded in
`future-tail-kernel-table-c6-20260715.md`.

## Read-only diagnostic

The new mode is enabled by five positive bounds:

```text
futuretailcoverageparents=
futuretailcoveragemidparents=
futuretailcoveragesamples=
futuretailcoveragemaxstates=
futuretailcoveragemaxrecords=
```

It requires `future`, C>=4, a pair-tail order, `futuretailrows=7`,
`futuretailcolorcanon`, `checkpointreadonly`, an existing
`futuretailkerneltable=`, and a positive `limit=`.  It rejects external
generation, signature scans, inventory creation, forced rescans, and
factorization checks.  Command-line hard maxima are 64 classes, 1,000 source
parents, 10,000 middle parents, 100,000 final samples, 2,000,000 states, and
100,000,000 generated records.

For each class, the exact in-memory recurrence closes the prefix two
transitions before the seven-row boundary.  States are sorted by their exact
record and key kind.  Evenly spaced parents are then expanded through the last
two transitions under the supplied state and record caps.  This makes the
sample deterministic and independent of unordered-map iteration order.

Every selected boundary state supplies three possible cuts and hence six
half-key occurrences.  The probe records table hits for all six, chooses the
cut with the most hits using a deterministic tie break, and retains:

- the two selected color-canonical half keys;
- the selected kernel-key pair; and
- the pair plus relative D8 row transform and relative color transform.

Sorted vectors permit exact unions across every class in one invocation.
The program prints `F=PROBE` and explicitly disables `F`/`N` accumulation.
Both the full prefix and selected expansions are included in the generated-
record bound.

## Differential and safety gates

The permanent `futuretest` now constructs a complete C=4 seven-row boundary,
loads a table made from that same boundary, and requires exact state-count
agreement, 100% occurrence and unique-key coverage, two hits for every chosen
cut, and sorted pair/signature output.

A C=5 class-300 smoke test used the pre-existing complete class-300 table.
The diagnostic constructed exactly 2,295 final states, independently matching
the committed external source manifest, then sampled 1,000 states:

```text
half occurrences/hits = 6000 / 6000
unique half keys/hits  = 661 / 661
best cuts 0/1/2 hits   = 0 / 0 / 1000
unique kernel pairs    = 648
unique full signatures = 1000
```

A deterministic repeat gave the same counters.  Separate negative tests
confirmed refusal without `checkpointreadonly`, without any one bound, with
external mode, and when either the state or generated-record bound was
deliberately too small.

The proportional and complete project gates passed after the final source:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts\verify_og2.ps1
powershell -NoProfile -ExecutionPolicy Bypass -File scripts\verify_all.ps1
```

The complete gate took 133.1 seconds and reproduced all mandatory C=2..5,
FJ9, future-twin, outer-generation, and read-only C=6 checks.

## Distributed C=6 measurements

Six widely separated classes first used 10 source parents, 100 middle
parents, and 1,000 boundary samples:

| class | exact prefix states | half hits | unique-key hits | best cuts 0/1/2 | unique pairs | unique signatures |
|---:|---:|---:|---:|---:|---:|---:|
| 3 | 47,587 | 6,000/6,000 | 4,751/4,751 | 0/0/1,000 | 989 | 1,000 |
| 101 | 152,731 | 2,988/6,000 | 2,409/4,854 | 212/97/691 | 996 | 1,000 |
| 1,001 | 145,290 | 1,858/6,000 | 1,545/4,926 | 114/524/362 | 990 | 1,000 |
| 10,001 | 160,333 | 591/6,000 | 514/5,050 | 681/188/131 | 994 | 1,000 |
| 31,600 | 33,945 | 0/6,000 | 0/4,427 | 1,000/0/0 | 993 | 1,000 |
| 63,199 | 4 | 0/816 | 0/11 | 136/0/0 | 20 | 136 |

Class 63,199 has only 136 final states, so its complete boundary rather than
1,000 states was sampled.  Increasing classes 10,001 and 31,600 to 20 source
parents, 200 middle parents, and 2,000 samples preserved the diagnosis:

```text
class 10001: 1576/12000 half hits, 1986/2000 unique pairs,
             2000/2000 unique signatures
class 31600:    0/12000 half hits, 1981/2000 unique pairs,
             2000/2000 unique signatures
```

Additional 500-state samples at classes 5,000, 15,000, 25,000, 40,000,
50,000, and 60,000 had half-occurrence hit counts of 1,821, 293, 185, 176,
111, and 216 out of 3,000.  Coverage is therefore heterogeneous and not an
ordering artifact confined to one class.

## Cross-class union gate

The final reproducible command sampled ten consecutive ordinary classes:

```powershell
& .\scripts\watch_rss.ps1 `
  -Exe .\build\factorization_orbit.exe `
  -Arguments @(
    '6','start=10000','limit=10','future',
    'futureorder=pair-adaptive-tail','futuretailrows=7',
    'futuretailcolorcanon','checkpointreadonly',
    'futuretailkerneltable=data\logs\future-tail-kernel-table-c6-g1-g2-20260715',
    'futuretailcoverageparents=5','futuretailcoveragemidparents=50',
    'futuretailcoveragesamples=500',
    'futuretailcoveragemaxstates=500000',
    'futuretailcoveragemaxrecords=5000000') `
  -LimitGB 4 -MaxMinutes 3 -IntervalSeconds 1 `
  -LogPath data\logs\future-tail-coverage-c6-class10001-10010-p5-repro-20260719.rss.csv `
  -StdoutPath data\logs\future-tail-coverage-c6-class10001-10010-p5-repro-20260719.out `
  -StderrPath data\logs\future-tail-coverage-c6-class10001-10010-p5-repro-20260719.err
```

It exited zero and deterministically reproduced:

```text
classes/probes                 = 10
sum exact prefix states        = 1338054
source/middle parents          = 50 / 500
final/sample states            = 31265 / 5000
generated records              = 1569691
half occurrence hits           = 5234 / 30000  (17.4467%)
per-class unique-key hit sum   = 4381 / 24621  (17.7930%)
best cuts with 0/1/2 hits      = 2692 / 1579 / 729
per-class unique half-key sum  = 24621
cross-class half-key union     = 23452
per-class unique pair sum      = 4962
cross-class pair union         = 4962
per-class signature sum        = 5000
cross-class signature union    = 5000
```

Only 1,169 of 24,621 per-class unique half keys repeated across this block, a
4.7488% reduction.  No selected kernel pair repeated across class boundaries,
and no complete relative-transform signature repeated at all.  The observed
RSS maximum was 970,858,496 bytes (0.904 GiB).  The elapsed counting time was
18.52 seconds; this is a diagnostic throughput measurement, not a projected
complete-run cost.

## Decision

The fixed G1/G2 table is a verified and valuable accelerator for G1 and G2,
but it is rejected as a broad later-class coverage mechanism.  Table coverage
can fall to zero, generic selected pairs are about 99% unique even within a
class, and the ten-class union found no cross-class pair or full-signature
reuse.  Naive batching or transformed-target caching at those coordinates
therefore cannot remove the class-local contractions.

This does not reject the exact half-kernel quotient, a dynamically constructed
much larger global table, or every possible transformed contraction.  The
sample shows that such a table would have to absorb mostly new later-class
keys, and none of these variants removes the exact per-class prefix, which is
already 9.08 GB for G2.

There is still no implemented global C=6 route that passes its scale gate.
The next primary research gate should return to bulk reverse gluing: first
derive and differentially verify a construction that generates or looks up
four-row configurations globally rather than enumerating the known
1.761-billion trivial-stabilizer pair placements.  Streaming the same nearly
injective placement list is not an accepted improvement.  Only after that
asymptotic work factor is removed should a positive-limit C=6 external probe
be run.  No complete C=6 four-row layer or 63,199-class run is authorized.
