# OG-2 Runbook

This is the safe workflow for exact 2xC work. Current results and bottlenecks
live in `../../STATUS.md`; dated timings live under `../reports/og2/`.

## Build

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts\build_og2.ps1
```

Built programs:

- `factorization_orbit.exe` — primary exact route.
- `mp_q.exe` — independent transfer-kernel research route.
- `ms_fast.exe`, `mp_fast.exe`, `mp_c6.exe` — independent validators.
- `canon_refine2.exe` — canonicalization validator.

## Short gate

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts\verify_og2.ps1
```

This checks C=2..4 across independent engines and runs canonicalization and
histogram differential tests.

## Primary exact gate

```powershell
.\build\factorization_orbit.exe 2 pivot rooted4
.\build\factorization_orbit.exe 3 pivot rooted4
.\build\factorization_orbit.exe 4 pivot rooted4
.\build\factorization_orbit.exe 5 pivot rooted4
```

Expected values are listed in `../../STATUS.md`. Any mismatch is a hard stop.

## Reverse-gluing decision prototype

This prototype is intentionally outside the standard build:

```powershell
g++ -O3 -mpopcnt -std=c++20 -Wall -Wextra `
  experiments\proto\reverse_glue.cpp -o build\reverse_glue.exe

.\build\reverse_glue.exe 2 summary
.\build\reverse_glue.exe 3 summary
.\build\reverse_glue.exe 4 summary
```

The expected totals are the ordinary C=2..4 gates.  A C=5 `2+2` layer may be
split into closed unordered-pair intervals:

```powershell
.\build\reverse_glue.exe 5 summary pairstart=0 pairprobe=500 `
  canoncachecap=2000000 `
  partialout=data\logs\reverse-glue-c5-parts\part-0000-0499.bin
```

Every `partialout` path must be new.  The writer installs a file only after the
pair interval closes, immediately reopens it, and checks every record.  To
merge a complete set, pass one `partialin=PATH` argument per file.  The reader
sorts the embedded intervals and rejects gaps, overlaps, identity mismatches,
unsorted records, bad masks, padding, trailing bytes, or truncated data before
running the 4+1 layer.

The verified C=5 inventory is:

```text
two-row coordinate orbits = 107
unordered pair interval   = [0,5778)
four-row orbits           = 17120
complete classes          = 355
N(5) = 1903816047972624930994913280000
```

The only permitted C=6 modes are layer-only or explicitly bounded probes.  An
exact, read-only two-row inventory check is:

```powershell
.\build\reverse_glue.exe 6 layerprobe layercountcheck summary
```

It generates no four-row layer and touches no checkpoint.  A four-row sample
must provide positive pair, placement, and leaf bounds, use raw reduction, and
set an explicit canonical-cache cap no larger than 100,000:

```powershell
.\build\reverse_glue.exe 6 summary rawreduce `
  pairprobe=1 pairstart=149189 placementprobe=1 leafprobe=2000 `
  canoncachecap=5000
```

`partialout` and `partialin` are rejected at C=6.  `placementsonly` skips the
contingency kernel; `nocanonicalprobe` measures bounded raw output without a
group-canonicalization scan; `keyonlyprobe` computes an anchored canonical key
without a stabilizer, and `keyverify=N` checks up to 1,000 such keys by a full
group scan.  None produces exact four-row values.  The
hard maxima are 16 pairs, 100,000 placements, and 100,000 contingency leaves;
the program refuses an unbounded C=6 join.  See `../methods/reverse-gluing.md`,
`../reports/og2/reverse-gluing-c4-c5-20260718.md`, and
`../reports/og2/reverse-gluing-c6-frontier-probe-20260718.md`.

## Joint-histogram decision prototype

This prototype is also outside the standard build:

```powershell
g++ -O3 -mpopcnt -std=c++20 -Wall -Wextra `
  experiments\proto\joint_histogram.cpp -o build\joint_histogram.exe

.\build\joint_histogram.exe 2
.\build\joint_histogram.exe 3
.\build\joint_histogram.exe 4
.\build\joint_histogram.exe 4 noswap
```

These commands must reproduce the ordinary C=2..4 totals.  The C=4 middle
layer has 141 states with copy swap and 232 without it.  The default small-C
run compares the occupied-anchor canonicalizer with a complete group scan on
every distinct raw target.

C=5 is a bounded frontier mode, not a complete counting command.  A cheap
exact first-layer check is:

```powershell
.\build\joint_histogram.exe 5 stop=1 maxstates=1000000 `
  maxleaves=10000000 canoncheck=100
```

Expected results are seven states, 6,210 leaves, and layer total
52,254,720,000.  The complete layer-2 allocation cost can be counted without
materializing targets:

```powershell
.\build\joint_histogram.exe 5 stop=2 countleaves canoncheck=0
```

Expected values are 652,001,548 leaves represented by 49,890 scalar memo
states, with at most 8,296 memo states for one source.  `countleaves` drops
target and cycle information and therefore does not compute a layer value.

Any materialized C=5 layer-2 probe must use `rawbatch=`, a positive
`sourceprobe=` or `leafprobe=`, `maxstates=`, `maxleaves=`, and an external
time/RSS guard.  For example:

```powershell
.\build\joint_histogram.exe 5 stop=2 sourcestart=2 sourceprobe=1 `
  rawbatch=100000 maxstates=1000000 maxleaves=7000000 canoncheck=100
```

This closes one selected source transition, not the layer.  The expected
support is 20,318 canonical targets from 6,516,556 exact leaves.  Prefix and
source-probe output is explicitly partial.

The target-labelled residual operator can be rechecked at small C with:

```powershell
.\build\joint_histogram.exe 3 stop=2 operatorfinal
.\build\joint_histogram.exe 4 stop=2 operatorfinal
```

These commands compare every raw target coefficient with the contingency and
labelled-symbol kernels.  The C=4 layer-2 peak is 53,970 operator states.  The
bounded C=5 decision gate is already negative: six sources exceed a
ten-million-state limit, while the completed source 2 has 6,323,400 terminal
raw states from 6,516,556 leaves.  Reproduction requires `operatorfinal`, a
positive `sourceprobe`, explicit operator state/record/type limits, and the
external time/RSS guard; use the exact command in the dated report.

Do not run an unbounded complete C=5 transition or change the prototype to
accept C=6.  See
`../methods/joint-histogram.md` and
`../reports/og2/joint-histogram-operator-frontier-20260719.md`.

## Connectivity subset-operator decision prototype

This independent Windows prototype is also outside the standard build:

```powershell
g++ -O3 -mpopcnt -std=c++20 -Wall -Wextra -fopenmp `
  experiments\proto\connectivity_operator.cpp `
  -o build\connectivity_operator.exe

.\build\connectivity_operator.exe 2 differential
.\build\connectivity_operator.exe 3 differential
.\build\connectivity_operator.exe 4 differential
```

`differential` compares the direct permutation-pair transition with the
two-subset transition per source and per labelled raw target.  The commands
must reproduce the ordinary C=2..4 totals and the connectivity layers listed
in `../methods/connectivity-operator.md`.  The independent optimized C=5
two-symbol initializer check is:

```powershell
& .\scripts\watch_rss.ps1 `
  -Exe .\build\connectivity_operator.exe `
  -Arguments @('5','stop=2','subset','initcheck') `
  -LimitGB 4 -MaxMinutes 2 -IntervalSeconds 1
```

The bounded third-symbol gate is cheap:

```powershell
.\build\connectivity_operator.exe 5 stop=3 differential
```

Expected states are `1,1,93,83776`; the transition has 857,244 valid/raw
targets and zero subset-prefix merges.  Any fourth-symbol C=5 measurement
requires a positive `sourceprobe`, explicit `maxoperatorstates` and
`maxoperatorrecords`, and an external time/RSS guard.  For example:

```powershell
& .\scripts\watch_rss.ps1 `
  -Exe .\build\connectivity_operator.exe `
  -Arguments @('5','stop=4','differential',`
               'sourcestart=40000','sourceprobe=100',`
               'maxoperatorstates=20000',`
               'maxoperatorrecords=100000') `
  -LimitGB 4 -MaxMinutes 2 -IntervalSeconds 1
```

This is an exact selected-source probe, not a closed layer.  The subset state
provably identifies every processed-band choice, so increasing the bounds
does not create DP merging.  Do not extend the prototype to C=6 or start a
complete C=5 fourth-symbol transition.  See
`../methods/connectivity-operator.md` and
`../reports/og2/connectivity-double-permanent-20260719.md`.

## Read-only C=6 gate

```powershell
.\build\factorization_orbit.exe 6 limit=1 canonbudget=100 `
  canoncachecap=300000 pivotinner parallelparents rooted4 parentchunk=128 `
  checkpoint=data\checkpoints\factorization_orbit_c6_graphmemo.bin `
  checkpointreadonly
```

Expected first value:

```text
F=6986348258918400
```

`checkpointreadonly` is mandatory for verification. It prevents a different
canonicalization budget or cache policy from rewriting an otherwise valid
checkpoint with additional equivalent key forms.

## Cold future-twin gate

The optional backend never accepts a graph checkpoint:

```powershell
.\build\factorization_orbit.exe 6 limit=1 future `
  futureorder=canonical-last futureprogress
```

Expected first value:

```text
F=6986348258918400
```

The original joint-canonical run took 1592.867 seconds and peaked at 1.670 GiB.
Wrap any repeat with explicit time and RSS limits.  For the second outer class,
use `start=1 limit=1`; do not remove the positive limit.

## Resumable external pair-tail job

For a low-symmetry class, the retained exact configuration is:

```powershell
.\build\factorization_orbit.exe 6 future futureprogress `
  futureorder=pair-adaptive-tail futuretailrows=7 `
  futureexternallayers=2 futureexternalrecords=500000 `
  futureexternalfanin=32 futurelayercheckpointparents=100000 `
  futureprogressparents=100000 futuretailcacherecords=100000000 `
  futuretailcheckpointrecords=1000000 futuretailthreads=32 `
  futuretailchunkrecords=240000 `
  futureexternal=data\logs\future-c6-pair-tail start=1 limit=1
```

Run this only through `scripts\watch_rss.ps1` with explicit `-LimitGB` and
`-MaxMinutes`.  Repeating the identical command resumes committed external
layers, a generation parent boundary, or a closed tail accumulator.  A work
directory without a valid generation manifest is not reused; a retry directory
is created instead.

For verification, repeat the identical job identity and add
`checkpointreadonly`.  Read-only mode requires `COMMITTED.manifest` and writes
nothing.  The verified values are:

```text
F6(G1) = 6986348258918400
F6(G2) = 7053808087203840
```

The G2 committed layer has 221,438,460 records and occupies 9,078,976,900
bytes.  Confirm free disk space as well as RSS before starting another class.

## Independent G2 color-tail verification

To ignore the committed tail accumulator and recompute every G2 tail state
through the exact `D8 x S6` half-kernel quotient, use:

```powershell
.\build\factorization_orbit.exe 6 start=1 limit=1 future futureprogress `
  futureorder=pair-adaptive-tail futuretailrows=7 `
  futuretailcolorcanon futuretailforcerescan `
  futuretailthreads=24 futuretailchunkrecords=240000 `
  futuretailcacherecords=200000000 `
  futureexternal=data\logs\future-external-c6-g2-adaptive-tail7-layers2-20260714 `
  futureexternallayers=2 checkpointreadonly
```

`futuretailforcerescan` and external `futuretailcolorcanon` require
`checkpointreadonly`.  The source layer must already have a valid
`COMMITTED.manifest`; no partial accumulator is loaded or written.  Run only
under explicit process guards.  On the reference Windows machine the verified
bounds were:

```text
time guard = 75 minutes
working-set guard = 4 GB
actual tail time = 3634.499 seconds
actual peak = 2402320384 bytes (2.237 GiB)
F = 7053808087203840
```

The exact result is accepted only if the fresh total matches the committed
manifest.  Logs or a killed prefix never constitute a partial exact result.
See `../reports/og2/future-tail-color-symmetry-c6-20260715.md` for the complete
command, gates, signature data, and negative ablations.

For bounded signature research, use `futuretailscan=NEW_DIR` together with a
positive `futuretailsamples=`.  `futuretailvalidate=`,
`futuretailbenchmarkrecords=`, `futuretailcolorsamples=`, and
`futuretailcolorsignatures=` are optional bounded diagnostics.  Always use a
new or empty output directory; a committed analysis is reused only when its
full identity matches.

## Shared color-kernel table

The committed experimental G1/G2 table can replace all private half-kernel
generation during a read-only tail rescan:

```powershell
.\build\factorization_orbit.exe 6 start=1 limit=1 future futureprogress `
  futureorder=pair-adaptive-tail futuretailrows=7 `
  futuretailcolorcanon futuretailforcerescan `
  futuretailthreads=24 futuretailchunkrecords=240000 `
  futuretailcacherecords=0 `
  futuretailkerneltable=data\logs\future-tail-kernel-table-c6-g1-g2-20260715 `
  futureexternal=data\logs\future-external-c6-g2-adaptive-tail7-layers2-20260714 `
  futureexternallayers=2 checkpointreadonly
```

`futuretailkerneltable=` requires `futureexternal`, `checkpointreadonly`,
`futuretailrows=7`, and `futuretailcolorcanon`.  The loader audits
`COMMITTED.manifest`, the exact file length, internal checksum, sorted keys,
contiguous offsets, and every kernel's signature order before exposing the
table to workers.  A single loaded payload is shared read-only across all
workers.

Verified G2 bounds are:

```text
time guard = 45 minutes
working-set guard = 3 GB
actual tail time = 824.440 seconds
actual peak = 0.956 GiB
kernel hits = 442876920 / 442876920
local assignments = 0
kernel evictions = 0
F = 7053808087203840
```

The same table reproduced G1 with 112,923,252/112,923,252 hits and 219.709
tail seconds.  See
`../reports/og2/future-tail-kernel-table-c6-20260715.md` for complete
inventory, overlap, SHA-256, C=4/C=5 gates, and both full rescans.

To build a new table, add `futuretailinventory=NEW_BASE`, a positive
`futuretailinventoryrecords=`, and optionally
`futuretailinventoryreference=COMMITTED_CLASS_DIR` to the same read-only,
positive-limit command.  The table directory must be new or contain an exactly
matching committed table.  Do not overwrite an uncommitted directory; inspect
and distill it first.

## Writable C=6 experiments

Before a writable run:

1. pass `scripts/verify_all.ps1` in the same working tree;
2. verify the checkpoint SHA-256 against `data/checkpoints/MANIFEST.md`;
3. create a second physical copy;
4. use a positive `limit=` and an explicit memory/time guard;
5. save only at closed degree-5 or degree-4-parent boundaries;
6. distill results into a dated report before deleting logs.

Do not start a full 63,199-class run.  G2 is now closed, but its 221-million
state frontier and 13.7-minute table-backed tail demonstrate that independent
class-local closure does not yet scale to the full outer family.  The next
bounded target is a later-class prefix/sample coverage measurement against the
existing table plus kernel-pair batching diagnostics, not another
unconstrained class sweep.
