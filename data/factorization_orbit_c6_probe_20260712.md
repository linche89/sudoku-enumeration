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
