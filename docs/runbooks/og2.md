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

## Global layer-DP candidate

The row-incremental candidate is outside `src/`, but its exactness and
checkpoint recovery are part of the repository gate:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass `
  -File scripts\build_layer_dp.ps1

powershell -NoProfile -ExecutionPolicy Bypass `
  -File scripts\verify_layer_dp.ps1 -Threads 8 -KillIterations 2
```

The build script deliberately omits `-march=native`; do not copy the obsolete
raw-expert build line.  The verification script covers C=2..5 totals, all 355
C=5 class triples, canonical invariance/separation/histogram differentials,
snapshot round trips, exact external summation, narrow and forced-wide
kill/resume, corrupt-generation fallback, fail-stop I/O, the G1/G2 bridge,
bounded Windows sharing-lock replacement retry, and refusal of an unbounded
C=6 invocation.

Checkpointed experiments must use a new base:

```powershell
.\build\layer_dp_gate.exe 5 --threads 8 `
  --caps 200,20000,20000,600 `
  --load-layer 3 PATH_TO_LAYER3.snap `
  --checkpoint NEW_BASE 0 --ckpt-chunk 500
```

Resume with the same arithmetic/canonicalization mode, chunk size, capacities
large enough for the stored child, and the same base:

```powershell
.\build\layer_dp_gate.exe 5 --threads 8 `
  --caps 200,20000,20000,600 `
  --checkpoint NEW_BASE 0 --ckpt-chunk 500 `
  --resume NEW_BASE
```

Do not delete either generation or its `.Lk.snap` parent.  A completed
checkpoint may be hard-linked to the snapshot, so identical file identities
are intentional.  A new run refuses a base that already contains files;
choose a fresh stage base rather than clearing one casually.

For C=6, only the following are routine:

```powershell
.\build\layer_dp_gate.exe 6 --bridge-only
.\build\layer_dp_gate.exe 6 --stop-after 2 --threads 8 `
  --caps 2000,14000000,1,1,1
```

Any larger bounded probe needs `scripts/watch_rss.ps1` with explicit time and
memory limits.  `--ack-full-c6` is an accidental-launch interlock, not owner
authorization.  Before a large stage, follow the writable-C=6 policy below
and the completed-preflight/owner-decision boundary in
`../methods/layer-dp.md`.

Run the read-only resource plan against the intended checkpoint volume before
allocating a large layer:

```powershell
.\build\layer_penultimate_burnside.exe 6
```

This must report exact `M_5=96452755`.  The production layer-DP engine uses
the same value as a hard boundary check when constructing or loading layer 5.
The independent derivation and gate evidence are in
`../math/penultimate-layer-burnside.md` and
`../reports/og2/layer-dp-penultimate-burnside-20260801.md`.

Then run the resource plan:

```powershell
$caps = '2000,14000000,1350000000,250000000,100000'
foreach ($layer in 3,4,5) {
  .\build\layer_dp_gate.exe 6 --threads 24 --caps $caps `
    --checkpoint E:\PATH\TO\NEW_STAGE_BASE 0 `
    --resource-preflight $layer
}
```

This creates no files.  It counts fixed arrays and tables, thread caches,
wide final accumulators, cap-reserved resume, retained generations, and the
third full `.tmp` image present while A/B is atomically replaced.  The
provisional cap set above reported RAM-with-margin values
69.898 / 79.685 / 18.465 GiB and disk-with-margin values
152.726 / 132.609 / 124.242 GiB for 3->4 / 4->5 / 5->6 on 2026-07-31.
The layer-4 cap remains measurement-supported.  The 250-million layer-5 cap
is 2.59194 times the exact 96,452,755 real states; its remaining operational
risk is parallel insertion holes and production allocation behavior.

An acknowledged large run is restricted to one transition per process:
layer 3->4 must stop at 4, layer 4->5 must stop at 5, and only layer 5 may
continue to the final CSV.  The engine repeats the resource check
automatically and refuses a monolithic large-layer invocation.

The owner-authorized production S1 work uses bounded daily windows through
the dedicated controller.  The first window is:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass `
  -File scripts\run_layer_dp_c6_s1_window.ps1 `
  -AuthorizeFullC6 -WindowHours 8 -HardMaxHours 10
```

The controller reruns the complete repository gate, verifies both retained
L3 hashes, performs the target-volume resource preflight, enforces an
85-GiB RSS bound and an 8-GiB minimum-available-RAM bound, and starts only a
staged `3->4 --stop-after 4` process.  At eight hours it waits for the next
durable checkpoint marker and then stops the process.  Ten hours is an
absolute process bound.  It copies the newest durable generation to the D:
external backup directory and verifies SHA-256 before returning.

Continue the same S1 namespace on a later day with the identical defaults
plus:

```powershell
-ContinueExisting
```

A killed current chunk is deliberately not accepted as progress; resume
loads the newest verified generation and recomputes only work after its
stored cursor.  Do not rename, clear, or mix the production namespace with
the rehearsal directories.

The first production window was interrupted by workstation sleep and closed
on the hard wall-time guard after wake.  It retained generation 5 at cursor
5/124 with 92,717,503,648 emissions and 903,363,957 real child states.  No L4
snapshot was claimed.  The local and D: backup SHA-256 is
`357ACEF8F257FAB819D53BB11E1E0AF2FBA882406F4A5196805895C9BD51274A`.

That session exposed and fixed an A/B path-array parsing error in the
controller.  Before any resume, require commit `bfbfe4e` or later and run:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass `
  -File scripts\run_layer_dp_c6_s1_window.ps1 `
  -AuthorizeFullC6 -PrepareOnly -ContinueExisting
```

It must report that the external backup already covers the local generation,
then end with `PRODUCTION S1 PREPARATION PASSED`; it must not start an engine.
The retained evidence is in
`../reports/og2/layer-dp-c6-production-s1-window1-20260803.md`.

The corrected second window resumed generation 5 / cursor 5/124 and ran for
8.0241 hours.  It stopped normally on generation 15 / cursor 15/124, after
the first durable checkpoint following the eight-hour target.  The current
image contains 280,014,646,848 emissions and 903,398,573 real child states;
its verified local/external SHA-256 is
`99C2203AE846EC78627EA8C3774340E12C3F99C2C5DA6817C92217A7600218C8`.
There is still no closed L4 snapshot.  See
`../reports/og2/layer-dp-c6-production-s1-window2-20260803.md`.

The third window resumed generation 15 / cursor 15/124 and ran for 8.0682
hours.  It stopped normally on generation 26 / cursor 26/124, after the first
durable checkpoint following the eight-hour target.  The current image
contains 485,060,060,720 emissions and 903,398,595 real child states; its
verified local/external SHA-256 is
`D652E023F52F3E58EDFB35FA8E23195E1950B53CE7C9CBE7394FEA926E6795BF`.
There is still no closed L4 snapshot.  See
`../reports/og2/layer-dp-c6-production-s1-window3-20260809.md`.

The fourth window resumed generation 26 / cursor 26/124 and ran for 8.4374
hours.  It advanced twelve chunks through eleven durable generations and
stopped normally on generation 37 / cursor 38/124.  The current image
contains 704,741,992,192 emissions, 34,280,078 cache hits, 903,398,620
claimed entries, 18 holes, and 903,398,602 real child states.  The verified
local/external SHA-256 is
`ECF0837315B0FDF8AE21C394FDA6676490E43B1828A0825529A344EC17E4E844`.
Generation 36 remains in `.b` as the previous local fallback.  There is still
no closed L4 snapshot.  See
`../reports/og2/layer-dp-c6-production-s1-window4-20260810.md`.

Starting with the recovered generation-37 baseline, the S1 controller appends
each future durable checkpoint to an independently inspected, append-only
`progress.csv`.  It records header/counter deltas, exact real-parent fan,
elapsed time, RSS, internal hashes, and SHA-256 without changing the engine or
checkpoint format.  Earlier per-checkpoint counters are not reconstructable
and must not be regenerated by rerunning chunks 0--37.

The separately gated S2 and S3 controllers, stable L4/L5 snapshot backups,
two final replays, semantic certificate verifier, and two independent exact
sums are frozen in `layer-dp-c6-production.md`.  Their existence is not owner
authorization; S2 cannot start before a manifest-pinned closed L4 exists, and
S3 cannot start before a manifest-pinned closed L5 exists.

The production-cap 3-to-4 allocation/restart rehearsal passed on 2026-08-01:

```text
cap=1,350,000,000
peak RSS=61.810 GiB under a 75 GiB guard
forced stop at gen/cursor 91
successful reloads at gen/cursor 91, 146, and 170
latest retained rehearsal image: 21,993,609 entries, 0 holes
```

Its forensic checkpoint is under
`data/logs/layer-dp-c6-allocation-rehearsal-20260801`; it is not an active
production checkpoint.  Do not start it as a production continuation; the
owner decision applies to a fresh production checkpoint namespace.  Detailed
evidence is in
`../reports/og2/layer-dp-c6-allocation-restart-rehearsal-20260801.md`.

The complete bounded interrupted rehearsal passed on 2026-08-02.  Run only
through the watermarked driver; do not reproduce it with ordinary production
checkpoints or `--ack-full-c6`:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass `
  -File scripts\rehearse_layer_dp_c6.ps1 `
  -Threads 24 -Denom 100 `
  -CheckpointMinutes 10 -ChunkParents 100000 `
  -RssLimitGB 85 -MaxStageMinutes 240 `
  -RunDir data/logs/layer-dp-c6-e2e-rehearsal-YYYYMMDD-1pct
```

The driver verifies the authoritative L3 snapshot and D: backup hashes,
binds every L4-or-later image to a rehearsal-only configuration fingerprint,
forces one S1 interruption after a durable checkpoint, waits for Windows to
reclaim RAM, resumes, runs S2/S3 in fresh processes, replays S3, and invokes
the external arbitrary-precision checksum.  If the driver itself is stopped,
resume its retained chain with exactly the same parameters plus:

```powershell
-ContinueExisting
```

The retained denominator-100 run reported:

```text
S1 peak / emissions / L4 = 61.861 GiB / 21387180240 / 903346741
S2 peak / emissions / L5 = 48.703 GiB / 23637232504 / 96452753
S3 emissions / classes   = 55653192 / 63117
CSV SHA-256              = B247C170370D7936D3406E485BF4A50E27198BB2AD86F16CDE32B19F87038A96
checksum                 = 38528041484076505706899541562753024000 (NOT N(6))
```

The 10-minute test period produced 22.69-35.00-second S1 writes and
6.30-8.36-second S2 writes.  The current production recommendation is 40
minutes, reducing measured steady-state S1 checkpoint overhead below about
1% while bounding recomputation to roughly 40 minutes.  This remains an
owner choice, not a default or authorization.  Detailed evidence is in
`../reports/og2/layer-dp-c6-bounded-end-to-end-rehearsal-20260802.md`.

The retained exact S0 snapshot can feed the measurement-only random M4 and
uniform 4-to-5 calibration without rebuilding layers 1-to-3:

```powershell
.\build\layer_dp_gate.exe 6 --threads 24 `
  --caps 2000,14000000,1,1,1 `
  --load-layer 3 data\checkpoints\layer_dp_c6_layer3_20260731.snap `
  --m4probe 58400 6 60000000 --m4-parent-seed 20260731 `
  --fan-sample 20000 --fan-canon-sample 2000 10000000
```

Run it only through `scripts/watch_rss.ps1`; the current bound is 12 GiB and
30 minutes.  A stopped prefix has no accepted M4 or fan estimate.  The
snapshot and its external-backup hash are recorded in
`../../data/checkpoints/MANIFEST.md`.

The completed 2026-08-01 run reported:

```text
M4 naive / Chao1       = 902080896 / 902863734
uniform 4->5 fan       = 2617.482 mean, 4.408 SE (n=20000)
4->5 projected work    = 2.36323e12 emissions
canonical sample       = 5260480 emissions, 96.7 ns/emission
peak observed RSS      = 3.068 GiB
```

Treat the timing as a bounded kernel measurement, not a production wall-time
promise.  The retained detailed evidence is in
`../reports/og2/layer-dp-m4-random-calibration-20260801.md`.

See `../methods/layer-dp.md` and
`../reports/og2/layer-dp-checkpoint-gate-20260730.md` plus
`../reports/og2/layer-dp-resource-preflight-20260731.md`.

## F4-lookup coverage diagnostic

The exact two-level split is part of the short C=4 gate.  It may also be run
directly:

```powershell
.\build\factorization_orbit.exe 4 f4coveragecheck `
  f4coveragemaxstates=100000 f4coveragemaxrecords=1000000 `
  pivot rooted4
```

The C=6 mode is read-only and diagnostic.  It requires an existing graph
checkpoint, `checkpointreadonly`, a positive class limit, and explicit parent,
state, and record limits.  For example:

```powershell
& .\scripts\watch_rss.ps1 `
  -Exe .\build\factorization_orbit.exe `
  -Arguments @(
    '6','start=10000','limit=1',
    'f4coverageparents=20',
    'f4coveragemaxstates=1000000',
    'f4coveragemaxrecords=5000000',
    'canonbudget=10000000',
    'checkpoint=data\checkpoints\factorization_orbit_c6_graphmemo.bin',
    'checkpointreadonly') `
  -LimitGB 4 -MaxMinutes 3 -IntervalSeconds 1 `
  -LogPath data\logs\f4-coverage.rss.csv `
  -StdoutPath data\logs\f4-coverage.out `
  -StderrPath data\logs\f4-coverage.err
```

It refuses more than 64 classes, 1,000 sampled parents per class, 2,000,000
states, or 100,000,000 records.  It reports `F=PROBE` and performs no `F6` or
`N(6)` accumulation.  A canonical fallback is a hard stop, not a usable weak
lookup key.  The decision evidence is in
`../reports/og2/f4-lookup-coverage-20260719.md`.

## Reduced band-kernel rank prototype

This decision prototype is outside the standard build:

```powershell
g++ -O3 -mpopcnt -fopenmp -std=c++20 -I src `
  experiments\proto\band_kernel_rank.cpp `
  -o build\band_kernel_rank.exe

.\build\band_kernel_rank.exe 2
.\build\band_kernel_rank.exe 3
.\build\band_kernel_rank.exe 4
```

The complete exact gates must report dimensions `1,2,1`, `1,3,3,1`, and
`1,5,141,5,1`, with C=3 middle determinant 2,048,000 and all known endpoints
`[OK]`.  Every C<=4 side histogram is differentially compared with the brute
builder.

The retained C=5 certificate uses a closed 38,801-state band-1 checkpoint in
`data/logs`, never the active C=6 checkpoint.  A bounded sketch requires
`samples=16..2048`, `cachelog=20..28`, an existing `basis=`, and a new `out=`
path.  Always wrap it in `watch_rss.ps1`; the verified 1,024-row command used
7 GiB and 15 minute limits.  The installed certificate can be checked without
regenerating transitions:

```powershell
.\build\band_kernel_rank.exe verify `
  data\logs\band-kernel-rank-sketch-c5-m1024-20260719.bin
```

It must report both ranks as 1,024, `exactCertificate=yes`, the embedded data
hash `F8620CF513430C4B`, and `[OK]`.  This proves only the exact lower bound
`rank_Q >= 1024`; it does not prove full rank 38,801.  See
`../reports/og2/band-kernel-rank-20260719.md`.

## Fixed-source frontier certificate

This bounded verifier is outside the standard build and does not read or
write any checkpoint:

```powershell
g++ -O3 -std=c++20 -Wall -Wextra `
  experiments\proto\source_target_frontier_bound.cpp `
  -o build\source_target_frontier_bound.exe

.\build\source_target_frontier_bound.exe
```

The retained witness must report:

```text
reachable=1 types=12 automorphisms=1 swapAutomorphisms=0
permanents X=4743616 Y=4740096 maps X=74119 Y=74064
splitSupport X=6488..7806 Y=6503..7806 rankLower=42191464
terminalSupport=5489549616 recordGiB16=81.801
orbitalDimensionLower=1761454080 [OK]
```

This certifies a lower bound only for a fixed-source linear frontier whose
output distinguishes the complete target.  It does not rule out a target-only
transform that combines the actual multi-source vector before representing
source-target pairs.  See
`../reports/og2/source-target-frontier-lower-bound-20260720.md`.

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

Production `futuretailkerneltable=` use requires `futureexternal`,
`checkpointreadonly`, `futuretailrows=7`, and `futuretailcolorcanon`.  The
bounded coverage diagnostic in the next section is the only non-external
exception.  The loader audits `COMMITTED.manifest`, the exact file length,
internal checksum, sorted keys, contiguous offsets, and every kernel's
signature order before exposing the table to workers.  A single loaded
payload is shared read-only across all workers.

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

## Read-only later-class coverage probe

This mode measures table coverage and cross-class repetition without
evaluating a tail or accumulating `F`/`N`.  All five work bounds and a
positive `limit=` are mandatory:

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
  -LogPath data\logs\future-tail-coverage.rss.csv `
  -StdoutPath data\logs\future-tail-coverage.out `
  -StderrPath data\logs\future-tail-coverage.err
```

The mode rejects external writes, scans, inventories, forced rescans, and
factorization checks.  It sorts each exact source layer before evenly spaced
sampling, so repeated commands have deterministic counters.  Output must say
`F=PROBE` and `no F/N accumulation`.

The verified ten-class command above found no cross-class reuse among 4,962
selected kernel pairs or 5,000 relative-transform signatures.  It is retained
for audit and future-table comparisons, not as a path to a full class sweep.
See
`../reports/og2/future-tail-later-class-coverage-20260719.md`.

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
class-local closure does not yet scale to the full outer family.  The
later-class coverage decision is complete and negative for the fixed G1/G2
table and naive kernel-pair batching.  The class-local `2+4` audit is also
negative: ordinary classes contain about 1.3 billion two-factors, sampled F4
keys are nearly unique, and later-class coverage by the existing graph memo is
only about 2--4%.  Reopening reverse gluing requires an exact bulk lookup
design that avoids, rather than merely streams, both that top-down incidence
work and the known 1.761-billion bottom-up trivial-stabilizer placements.  No
C=6 four-row layer may be launched before that design passes complete C=4/C=5
differential gates and a separately bounded C=6 prefix test.
