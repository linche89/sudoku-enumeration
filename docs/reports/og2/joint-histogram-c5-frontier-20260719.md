# Box-order joint-histogram C=5 frontier decision

Date: 2026-07-19 (Asia/Singapore)

## Scope

This report extends the exact box-order joint-histogram prototype from the
complete C=2..4 gate to bounded C=5 measurements.  It does not compute
`N(5)` by this route, does not run C=6, and does not read or write a graph
checkpoint.

The retained implementation is:

```text
experiments/proto/joint_histogram.cpp
SHA-256 = 3195D357277010BD8EFD5206D84CE1E81D20B44ECD28E8301348BEA22B3DAE55
```

## Implementation changes

The state key now supports the 100 active joint mask types at the C=5
layers 2 and 3.  C=5 disables the labelled-symbol transition oracle, whose
size is already prohibitive, while retaining that complete oracle for
C=2..4.

The color canonicalizer now chooses the lexicographically maximum histogram
and scans only transformations whose anchor preimage is occupied.  Every
maximum has a nonzero anchor: otherwise a nonzero cell could be moved to the
anchor and give a larger image.  Candidate images are built from at most
`2C` nonzero cells instead of clearing and copying all 100 active cells.

An independent full `S_C x S_C` plus copy-swap scan remains available.  The
anchored and full representatives agreed on every raw transition target in
complete C=2..4 runs.  The stored-state invariance, separation, histogram,
complement-involution, sequential-total, and midpoint gates also passed.
C=5 bounded runs checked the first 100 distinct raw targets of each measured
layer against the full 28,800-image scan.

Two bounded execution modes were added:

- `rawbatch=N` flushes a raw-target map after at most `N` distinct entries and
  immediately reduces it to canonical targets.  This controls resident
  memory without treating a batch as a closed exact layer.
- `sourceprobe=`, `sourcestart=`, and `leafprobe=` select deterministic
  prefixes.  Their output is explicitly marked partial.

Finally, `countleaves` computes the exact number of contingency leaves without
materializing targets.  Its memo key is only

```text
(source-type index, remaining A degrees, remaining B degrees).
```

This scalar DP is a cost oracle.  It deliberately drops the target histogram
and cycle operator, so it is not a counting algorithm for `N(C)`.

## Regression gates

The enlarged key, maximum convention, anchor restriction, sparse candidate,
and streaming reducer retained:

```text
C=2 orbit layers = 1, 2, 1
N(2) = 288

C=3 orbit layers = 1, 3, 3, 1
N(3) = 28200960

C=4 orbit layers = 1, 5, 141, 5, 1
N(4) = 29136487207403520
```

A C=4 `rawbatch=1000 nodirect` run reproduced the same sequential and
midpoint total.  Disabling copy swap again changed the C=4 middle layer from
141 to 232 and preserved the total.  The scalar leaf DP reproduced the old
enumerator totals of 168 leaves at the C=3 layer-2 transition and 252,355 at
the C=4 layer-2 transition.

## Complete C=5 first layer

The guarded command was:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -Command `
  "& { & '.\scripts\watch_rss.ps1' `
  -Exe '.\build\joint_histogram.exe' `
  -Arguments @('5','stop=1','maxstates=1000000',`
               'maxleaves=10000000','canoncheck=100') `
  -LimitGB 4 -MaxMinutes 5 -IntervalSeconds 1 `
  -LogPath 'data\logs\joint-histogram-c5-layer1-20260719.rss.csv' `
  -StdoutPath 'data\logs\joint-histogram-c5-layer1-20260719.out' `
  -StderrPath 'data\logs\joint-histogram-c5-layer1-20260719.err' }"
```

It closed exactly:

```text
orbit states                 = 1, 7
contingency leaves           = 6210
raw targets                  = 6210
orbit transitions            = 7
layer total                  = 52254720000
time                         = 0.694396 s
observed RSS                 = 0.023 GiB
```

The RSS value is a one-second sample, not a certified sub-second peak.

## Why the original layer-2 pass failed

The first implementation retained every raw target for one source and
canonicalized only after the source transition closed.  Its deterministic
first source exceeded a 20,000,000-leaf hard limit after about 22 seconds;
the one-second watcher observed 1.715 GiB immediately before the exact limit
failure.  No partial accumulator was accepted.

Streaming removed this resident-map wall.  The same first-source ordering
gave the following exact deterministic leaf-prefix measurements:

| leaves | batched raw entries | canonical targets | canonical candidates | time | observed RSS |
|---:|---:|---:|---:|---:|---:|
| 100,000 | 92,026 | 9,318 | 233,020,800 | 2.38766 s | 0.025 GiB |
| 1,000,000 | 868,433 | 33,455 | 2,330,456,544 | 24.1853 s | 0.032 GiB |
| 5,000,000 | 4,227,388 | 38,373 | 11,719,292,832 | 154.653 s | 0.038 GiB |

The target support grows far more slowly than the allocation paths, while
the raw keys remain close to injective.  The representation therefore has
not failed this probe; the leaf-by-leaf transition kernel has.

## Exact layer-2 leaf inventory

The scalar cost DP agreed with the complete enumerator at C=3 and C=4 before
being used at C=5.  It then counted every C=5 layer-2 contingency leaf for all
seven ordered source orbits in 0.0275293 seconds:

| source index | source orbit-total coefficient | exact leaves | scalar memo states |
|---:|---:|---:|---:|
| 0 | 6,531,840,000 | 96,418,768 | 8,250 |
| 1 | 4,354,560,000 | 25,051,600 | 4,973 |
| 2 | 435,456,000 | 6,516,556 | 4,251 |
| 3 | 10,450,944,000 | 189,227,536 | 8,296 |
| 4 | 13,063,680,000 | 96,418,768 | 8,250 |
| 5 | 8,709,120,000 | 189,227,536 | 8,296 |
| 6 | 8,709,120,000 | 49,140,784 | 7,574 |
| **total** | **52,254,720,000** | **652,001,548** | **49,890** |

Thus 49,890 independent scalar memo records summarize 652,001,548 allocation
leaves.  This is direct evidence that an internal DP can collapse the degree
bookkeeping.  It is not evidence that the operator-valued frontier carrying
target identity and cycle weight is equally small.

The cheapest complete selected transition, ordered source 2, was also closed
with `rawbatch=100000` under the same 4-GiB/five-minute guard:

```text
exact leaves                  = 6516556
batched raw entries           = 6457202
canonical target support      = 20318
canonical candidates          = 18003322656
time                          = 189.839 s
observed RSS                  = 0.037 GiB
```

The selected transition is exact; the union over all seven sources is not.
At the measured 32,000--34,000 leaf/s range, a purely linear projection of
the 652-million-leaf layer is about 5.3--5.6 hours on one process, before the
unknown layer-3 work.  This is a kernel projection, not a full C=5 runtime
claim.

## Pass and planner interpretation

For the box-order route, the candidate mathematical contraction is now
explicit:

```text
empty -> layer 1 -> layer 2 -> layer 3 -> complementary midpoint
```

For C=6 the balanced form is the same three forward box transitions followed
by the exact 3+3 complementary contraction.  This specifies the mathematical
interfaces, but not yet an affordable implementation for each edge.

A useful execution planner should treat a node as

```text
(processed factor subset, exact sufficient boundary representation)
```

and an edge as a verified exact kernel or representation conversion with a
measured vector cost `(time, memory, records)`.  Subset DP, Dijkstra, or a
Pareto dynamic program can choose a contraction/elimination order among a
finite catalogue of such edges.  It cannot retain only one combinatorial
configuration path: every path contribution must still be summed inside the
chosen exact edge.  Nor can the planner invent a missing sufficient quotient;
finding such representations remains the mathematical part of the problem.

For the currently measured layer-1 to layer-2 edge, the catalogue contains:

- monolithic raw reduction: rejected by resident memory;
- streamed raw reduction: exact and memory-safe, but about 652 million leaves;
- scalar residual-degree DP: extremely small, but drops the required operator;
- operator-valued residual/subset DP: the next experiment, not yet measured;
- target-centric coefficient DP: a possible alternative once a complete exact
  target-orbit inventory exists.

## Decision

The C=5 first layer passes.  The target quotient remains promising in the
bounded layer-2 data, but the naive contingency-leaf implementation does not
qualify as a C=6 route.  The next bounded result must lift the 49,890-state
scalar residual DP to a target- and cycle-aware operator frontier, first
differentially at C=3 and C=4 and then on the seven C=5 layer-1 sources.

A complete C=5 joint-histogram run is not yet justified, and no C=6 run is
authorized by these measurements.
