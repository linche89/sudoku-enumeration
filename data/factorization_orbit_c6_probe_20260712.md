# C=6 global-orbit factorization probes (bounded)

Date: 2026-07-12 (Asia/Singapore)

This note records bounded experiments only.  No complete C=6 count was
attempted or claimed.

## Baseline commit

The validated C=5 implementation was committed first:

```text
c64b7d7 OG-2: add global orbit factorization counter
```

Unrelated pre-existing worktree changes were excluded from that commit.

## Outer-orbit generation

Directly extending the C=5 "enumerate every balanced histogram" method is not
viable:

```text
half-side count vectors:       2,324,784
raw balanced C=6 histograms: 2,284,361,552
```

The replacement is coordinate-by-coordinate canonical augmentation.  It gave:

```text
dimension 1: parents=1    rawChildren=1      orbits=1
dimension 2: parents=1    rawChildren=4      orbits=4
dimension 3: parents=4    rawChildren=56     orbits=17
dimension 4: parents=17   rawChildren=882    orbits=137
dimension 5: parents=137  rawChildren=17829  orbits=2072
dimension 6: parents=2072 rawChildren=485720 orbits=63199
```

Measured outer time was approximately 2.7 seconds.  Both exact gates passed:

```text
outer classes=63199
multiplicity sum=622345892187672576
expected=924^6=622345892187672576 [OK]
```

## Safe representation extensions

- histogram storage: 64 cells
- bipartite graph size: 12+12 vertices
- graph canonical keys: 192 bits (the C=6 adjacency matrix needs 144 bits)
- individual factorization counts: unsigned 128-bit
- full C=6 accumulation remains disabled
- C=6 inner work requires a positive `limit=` argument

After these changes, the complete C=2/3/4 gates still passed and the first C=5
class retained its known factorization count.  After batch canonicalization was
added, a full C=5 rerun also passed:

```text
N(5)=1903816047972624930994913280000 [OK]
countTime=63.241589s
outerTime=0.922677s
```

## C=6 top-level inspection

Five spread-out classes were inspected without recursion:

```text
class      top perfect matchings  components
1          192528                 1
15800      198688                 1
31600      199856                 1
47400      197904                 1
63199      518400                 2
```

For comparison, the first C=5 class generated only 16,805 perfect matchings in
the complete recursive run.  Thus C=6 begins with roughly an order of magnitude
more top-level branching.

## Bounded inner probes

### Per-residual strong canonicalization

With a 20,000-node canonicalization budget, class 1 did not finish in about 77
CPU seconds.  Reducing the budget to 100 without batching also did not finish
in the same bounded window.  The fixed cost of invoking canonicalization for
approximately 192,000 top residuals dominated.

### Weak keys only

With canonicalization disabled (`canonbudget=0`), the top level became cheap,
but recursive states expanded rapidly.  The run was stopped at approximately:

```text
CPU time:       62.8 s
perfect matches: 92,100,000
graph memo:       625,174
canon cache:    2,426,299 (all weak fallbacks)
working set:      309 MB
```

### Batched parallel canonicalization

Residuals were first grouped by exact weak keys, then missing strong canonical
forms were calculated in OpenMP batches.  The first top batch collapsed:

```text
192,528 weak residuals -> 1,622 strong residual classes
```

This preserved every C=2/3/4 gate and the complete C=5 value.  A five-minute
class-1-only C=6 run with `canonbudget=100` was stopped at:

```text
perfect matches: 19,700,000
graph memo:          69,514
canon cache:      7,092,538
fallbacks:          817,980
canon nodes:    201,891,747
working set:          663 MB
```

The class had not yet closed, so there is no valid per-class runtime estimate
and no basis for extrapolating to all 63,199 classes.

## Current conclusion

1. The C=6 outer problem is solved cleanly and cheaply.
2. Batched strong canonicalization is directionally correct and reduces the
   recursive graph-state count by roughly an order of magnitude versus weak
   keys alone.
3. Retaining millions of raw-to-canonical cache entries is not a viable direct
   path to a full C=6 run.
4. The next bounded engineering step should make the canonical cache bounded or
   degree-local and persist only the much smaller strong residual memo.  The
   immediate gate is completing one C=6 class before considering a wider run.

## Follow-up: bounded caches and degree diagnosis

The raw-to-canonical cache was split by residual degree and given an optional
exact capacity bound.  Eviction only causes recomputation; it cannot change a
count.  Replacing whole-cache clearing with FIFO eviction improved the
three-minute class-1 probe:

```text
policy                    PM after ~3 min   working set
whole degree-cache clear     6.35 million      103 MB
per-entry FIFO eviction      7.25 million      131 MB
```

A larger two-million-entry degree-3 cache used approximately 313 MB but did not
improve throughput enough to justify the memory.  The 300k-per-degree FIFO is
the better bounded baseline.

Degree-specific counters then identified the dominant layer.  Before the
degree-2 shortcut, a representative snapshot was:

```text
total PM: 2,450,000
degree-3 PM: 2,207,079
degree-4 PM:    23,615
degree-5 PM:    26,778
degree-6 PM:   192,528
```

The degree-3 recurrence was still canonicalizing 2-regular residuals even
though the exact base formula \(F_2(Q)=2^{c(Q)}\) is available.  Removing that
canonicalization passed the complete C=2/3/4/5 gates; C=5 then ran in:

```text
countTime=56.317401s
outerTime=0.849547s
N(5)=1903816047972624930994913280000 [OK]
```

For C=6 this moved the bottleneck up one layer.  After about 60 seconds the
class-1 probe had processed roughly 20.55 million perfect matchings, but was
still inside the first degree-5 residual:

```text
degree-3 misses: ~66,000
degree-4 misses: ~4,100 out of about 26,700 children
degree-5 misses: 1
```

An accidentally lingering process was detected and stopped; even after roughly
20 minutes it had processed only 9,935 of those degree-4 children and had not
closed the first degree-5 residual.  No result was produced or claimed.

### Revised next gate

Cache tuning alone is not the bridge.  The next implementation should evaluate
the roughly 26,700 degree-4 children of one degree-5 graph in coarse chunks:

1. enumerate degree-3 residuals for hundreds of degree-4 parents at once;
2. canonicalize the large combined batch in parallel;
3. compute/cache each strong degree-3 value once;
4. accumulate the corresponding degree-4 values and discard raw batch data.

This keeps memory bounded while amortizing OpenMP setup, hash-table traffic, and
canonicalization across many parents.  The gate remains one complete C=6 outer
class; a full 63,199-class run stays disabled.

## Follow-up: layered F4 evaluation and restartable strong memo

The degree-4 children are now evaluated in chunks of 128 parents.  All their
degree-3 residuals are combined, canonicalized in one OpenMP batch, evaluated
once per strong key, and then accumulated back into the parent values.  The
complete exact gates still pass after this change.  The latest C=5 gate was:

```text
countTime=54.788826s
outerTime=0.758395s
N(5)=1903816047972624930994913280000 [OK]
```

A binary checkpoint stores only completed strong graph-memo entries.  It is
written via a temporary file and atomically installed after a complete degree-5
value, so terminating a bounded run cannot expose a half-written memo.  A C=5
save/reload smoke test loaded the checkpoint and changed the first-class run
from 16,805 perfect-matching enumerations to a direct top-level memo hit with
zero new perfect matchings.

For C=6 class 1, the top 192,528 weak residuals again collapsed to 1,622 strong
degree-5 classes.  Two of those classes have now been completed exactly:

```text
F5 index  value        F4 parents  elapsed     memo after  checkpoint bytes
1         37186844160       26734  209.775 s       106332           4359632
2         36718141440       25376  205.859 s       151371           6206231
```

The restart between them loaded all 106,332 entries and proceeded directly to
the second degree-5 class (25,376 parents rather than the first class's 26,734),
with only one new degree-5 memo miss.  This verifies that the checkpoint is a
real continuation boundary, not merely a log snapshot.

The first two degree-5 values average about 207.8 seconds.  A deliberately
conservative early linear extrapolation, before allowing for unknown later
memo sharing, is therefore about 93.6 hours for the 1,622 degree-5 classes in
just the first outer class.  The second value was only about two percent faster
than the first, so the first observed cross-value sharing is not yet a
quantity-changing speedup.  This is not a full-class runtime estimate, but it
is enough evidence not to continue the same loop blindly.

### Revised next gate

Keep the exact checkpoint, but benchmark the layered evaluator's parent chunk
size and degree-3 canonical cache in short, repeatable windows from the same
checkpoint.  The immediate target is a material improvement in completed
degree-4 parents per second.  Do not widen beyond one outer class unless that
rate changes substantially.

Short runs from the same second-generation checkpoint quantified those tuning
limits:

```text
parent chunk  D4 parents / 45.85 s  peak working set
128                         4480            181.7 MB
256                         4608            238.7 MB
512                         4608            347.8 MB
```

Increasing only the degree-3 canonical cache from 300,000 to 2,000,000 entries
with chunk 128 reached 4,736 parents in 45.9 seconds (about 5.7% over baseline)
but raised peak memory to 375.5 MB.  Thus neither larger batches nor a much
larger raw-to-canonical cache changes the scale; chunk 128 with a 300k cache
remains the balanced baseline.

The checkpoint boundary was then refined from one complete degree-5 value to
periodic completed degree-4 chunks.  With `checkpointparents=2048`, a 45.95 s
run saved after 2,048 and 4,096 parents and grew the memo from 151,371 to
161,741 entries.  A fresh process loaded that file and reported its first
layered progress as:

```text
probe layeredF4 parents=128/20619 ...
```

The unsaved total had therefore fallen exactly from 24,715 to
`24,715 - 4,096 = 20,619`.  Partial degree-5 work is now restartable using only
closed, exact degree-4 and degree-3 values; no half-computed accumulator is ever
trusted.

## Follow-up: degree-3 structural keys and parallel parent enumeration

The generic canonicalizer was augmented with an exact degree-3 experimental
path.  It builds label-invariant vertex profiles from common-neighbor counts
and incident four-cycles, then refines both the original bipartite graph and
the weighted common-neighbor graphs on each side.  When all vertices receive
distinct structural colors, their order is already a canonical label.  The
resulting structural key is mapped once into the legacy strong-key namespace,
so existing graph checkpoints remain reusable.

Graphs whose structural colors remain tied have two exact fallbacks:

- `d3iso`: parallel generic canonicalization for the unresolved minority;
- `d3pairs`: synchronized two-graph color refinement and backtracking, with a
  generic canonicalization fallback if the isomorphism search is inconclusive.

The complete C=5 gate passed with `d3pairs`:

```text
N(5)=1903816047972624930994913280000 [OK]
countTime=37.863814s
degree-3 discrete classes=403
degree-3 pair representatives=757
degree-3 pair hits=2687724
degree-3 pair checks=2811166
unknown searches=0
known-key misses=0
```

This is about 30.9% faster than the previous 54.79-second C=5 baseline.  The
pair-isomorphism path did not scale well to M=12, however; the lighter `d3iso`
fallback is the C=6 choice.

Profiling then showed that reducing degree-3 canonicalization exposed a serial
stage: the 128 independent degree-4 parents in a layer were enumerating their
perfect matchings one at a time.  `parallelparents` gives every parent a local
residual map, merges the completed maps in deterministic parent order, and
updates the global counters only after the OpenMP region.

Repeatable probes from the same 161,741-entry checkpoint gave:

```text
configuration                       D4 parents / window   peak working set
baseline                            3072 / 30.59 s           178.3 MB
parallelparents                     3712 / 30.65 s           187.5 MB
parallelparents + d3iso             4096 / 30.56 s           199.3 MB

baseline                            6528 / 60.15 s           177.9 MB
d3iso only                          7040 / 60.13 s           198.6 MB
```

Finally, the combined `parallelparents d3iso` path recomputed the first C=6
degree-5 graph from an empty memo and reproduced its previously established
exact value:

```text
F5=37186844160
new elapsed=178.603s
old elapsed=209.775s
improvement=14.9%
```

Thus the specialized work is a real bounded improvement and not a new counting
assumption.  It still does not change the roughly multi-day scale of a full
first outer class, so the mathematical reformulation remains the main route to
a complete C=6 result.

After committing the optimized path, the existing partial third degree-5
state was resumed with `parallelparents d3iso checkpointparents=2048`.  It
closed exactly as:

```text
F5 index  value        saved parents before run  resumed elapsed
3         35733365760                      4096        131.222 s
```

Together with the earlier 45.95-second partial run, this is about 177.2 seconds
of useful work for the third value.  The installed checkpoint now contains
190,171 strong states and occupies 7,797,031 bytes.  A fresh 15-second process
loaded it and immediately entered the fourth degree-5 value:

```text
graph checkpoint loaded ... entries=190171
probe layeredF4 parents=128/24143 ...
```

The first three degree-5 values are therefore closed and restartable; the
fourth has 24,143 currently missing degree-4 parents.

## Follow-up: rooted color pivot

The rooted color-block identity

```text
F_d(G) = d * sum_{perfect matchings M containing e} F_(d-1)(G-M)
```

was added as the optional `pivot` path.  A (2^{12})-state subset DP computes
the perfect-matching frequency of every edge incident with row zero, and the
least-frequent edge is forced.  The enumerated residual multiplicity is checked
against that DP frequency on every call before the factor (d) is restored.
Memoized values remain the complete ordered (F_d(G)), so old checkpoints are
compatible.

All complete C=2/3/4/5 gates passed.  The fastest C=5 combination is now:

```text
flags: pivot d3pairs
N(5)=1903816047972624930994913280000 [OK]
countTime=13.776642s
perfect matchings enumerated=3256904
matching count without pivot at the same memo frontier=14095124
```

For C=6, `pivotinner` leaves degree 6 unchanged so that the previously known
first degree-5 graph is selected.  With `parallelparents d3iso` it reproduced:

```text
F5=37186844160
new elapsed=13.474s
previous elapsed=178.603s
speedup=13.3x
degree-4 parents: 26734 -> 5168
```

The stronger pair-isomorphism mode was slower at M=12 (29.490 seconds), so
`d3iso` remains the C=6 choice.

At the top degree-6 graph, full `pivot` reduced the raw matching list exactly
as predicted but barely changed its isomorphism frontier:

```text
raw degree-5 residuals:    192528 -> 32088
strong degree-5 classes:     1622 -> 1617
```

Thus the first outer class falls from an early estimate near 80 hours to about
six hours if later degree-5 values resemble the first.  The next decision gate
is the exact rooted 2-factor count for these degree-5 representatives; only a
small output count justifies implementing the two-color block enumerator.

## Follow-up: rooted two-factor split

The ternary right-degree DP was implemented first as a counting-only gate.  For
the first three degree-5 representatives, the exact minimum over the ten root
edge pairs was:

```text
F5 representative   minimum rooted 2-factors   all root-pair counts summed
1                                      1635237                       17439798
2                                      1626321                       17341639
3                                      1618911                       17299931
```

These stable 1.6-million-object minima rule out formula (10) as the immediate
replacement: it would create at least that many cubic complements per degree-5
graph.  The same gate on three degree-4 residuals gave minima 15,553, 15,382,
and 15,536, so formula (8) is on the right scale.

The optional `rooted4` evaluator now computes

```text
F4(G) = 6 * sum_H 2^(c(H) + c(G-H))
```

directly.  It fixes two edges at row zero, builds the reachable ternary
right-degree states layer by layer, and enumerates valid factors backward from
the all-two state.  A stamped dense table gives O(1) predecessor tests.  Two
rollback disjoint-set structures maintain `c(H)` and `c(G-H)` without creating
cubic residual graphs.  Only complete `F4` values enter the existing strong-key
memo, so the checkpoint format and old entries remain compatible.

All exact gates passed with the final implementation:

```text
C=2  N=288                                [OK]
C=3  N=28200960                           [OK]
C=4  N=29136487207403520                  [OK]  countTime=0.007569s
C=5  N=1903816047972624930994913280000    [OK]  countTime=9.225454s
      flags: pivot rooted4
```

On C=6, `pivotinner parallelparents rooted4` reproduced the first ten `F5`
values from a clean memo.  The first three were the independently established
values 37,186,844,160; 36,718,141,440; and 35,733,365,760.  Their ten timings
were:

```text
0.934  0.961  0.864  0.847  0.909  0.762  0.811  0.809  0.737  0.799 seconds
mean = 0.843 seconds
```

The first exact gate therefore fell from 13.474 seconds to 0.934 seconds
(14.4x).  A straight-line extrapolation of the 1,622-class first degree-5
frontier is now roughly 20--30 minutes rather than six hours, but this is not a
completed outer-class measurement and does not address the 63,199 outer-class
frontier.

Finally, the compatible checkpoint was resumed atomically after every closed
degree-5 value.  A fresh reload skipped the stored prefix and reproduced the
next previously observed value, 35,892,157,440.  It now contains 242,937 exact
states (9,960,437 bytes), with the first 14 degree-5 values closed.

## First complete C=6 outer class

The checkpoint was then expanded in bounded gates: first to 64 closed `F5`
values, then to 256.  The 50-value steady-state gate averaged 0.7606 seconds
per value and matched 34 overlapping values from the previous evaluator with
zero discrepancies.  The following 192-value gate averaged 0.7207 seconds;
at 1,272,904 memo states its peak working set was only 295.1 MiB.

Those measurements justified completing exactly one outer class, still with
`limit=1`, 64-value atomic checkpoints, and a 3 GiB memory cutoff.  The run
finished normally:

```text
flags: pivotinner parallelparents rooted4 parentchunk=128
F6(G_1) = 6986348258918400
class time = 706.652 s
count time = 707.767 s
peak working set = 665.6 MiB
memory cutoff = not triggered
memo states = 5114835
checkpoint bytes = 209708255
```

The 1,366 newly evaluated `F5` values averaged 0.4979 seconds (median 0.4930,
range 0.022--0.860).  Their final 64-value block averaged 0.3522 seconds, so
cross-`F5` memo reuse materially improved the rate instead of degrading it.
They added 3,841,930 states, or 2,812.5 states per value on average.

A cold process then loaded all 5,114,835 states and reproduced the same top
value by a direct memo hit:

```text
F6(G_1) = 6986348258918400
graph calls=1, memo hits=1, memo misses=0
class time=0.000 s
```

This closes the first exact C=6 outer-class gate.  It establishes an
approximately twelve-minute first-class cost, not the earlier six-hour
estimate.  It does not yet predict the remaining 63,198 classes: the next
bounded gate must measure how much of this five-million-state memo is reused by
the second outer graph before any full-run projection is credible.
