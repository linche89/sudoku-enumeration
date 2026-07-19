# C=6 seven-row color-symmetry tail evidence, 2026-07-15

## Scope

This report records the implementation and bounded experiments prompted by the
raw expert response under `docs/expert/2026-07-14/`.  The expert response was
used as a research input, not as an authority.  Every conclusion below comes
from the current C++ implementation, exact differential tests, or committed
local data.

The source for all G2 experiments was the immutable committed layer

```text
data/logs/future-external-c6-g2-adaptive-tail7-layers2-20260714/
  class-2/layer-05.bin
```

with the following audited identity:

```text
records = 221438460
mass = 2009438804160
bytes = 9078976900
F6(G2) = 7053808087203840
```

All C=6 experiments used `start=1 limit=1`, `checkpointreadonly`, an explicit
time bound, and an explicit working-set bound.  No active graph checkpoint or
external layer was modified.

## Exact implementation

The seven-row tail already splits a state into two half kernels across one of
three remaining complementary row pairs.  This work added four exact pieces:

1. A sufficient seven-row signature consisting of two D8-canonical half keys
   and their relative D8 transform.  The original state value can be
   reconstructed exactly from this signature.
2. Exact half-kernel canonicalization under `D8 x S_C`.  Color profiles first
   refine the color cells; permutations are enumerated only inside equal
   profile cells.  The result was differentially checked against the brute
   `8 * C!` enumeration.
3. A sufficient color-tail signature

   ```text
   (left color-canonical half key,
    right color-canonical half key,
    relative D8 transform,
    relative S_C permutation).
   ```

   Swapping the two halves inverts both relative transforms.  Reconstruction
   from this signature is exact.
4. A stateless production policy.  All six candidate halves are
   color-canonicalized temporarily so cut selection can see the real kernel
   cache keys.  Only the expensive half kernels are cached.  Raw-to-canonical
   color mappings are not retained, because their computation is cheap and an
   unbounded raw-key map was the dominant memory growth.

The diagnostic switches are:

```text
futuretailcolorcanon
futuretailforcerescan
futuretailscan=DIR
futuretailsamples=N
futuretailvalidate=N
futuretailbenchmarkrecords=N
futuretailcolorsamples=N
futuretailcolorsignatures=N
futuretailreference=DIR
```

External color-tail runs are intentionally restricted to
`checkpointreadonly`; the existing resumable tail manifest does not store the
new color-cache counters.

## Exact gates

The following checks passed after the final implementation:

- `factorization_orbit.exe 4 futuretest`, including color-signature
  reconstruction and color-refinement/brute-force differentials;
- a forced four-thread rescan of all 2,295 seven-row states in C=5 outer class
  300, reproducing `F=1324247040`;
- a separate C=5 class-300 scan in which all 2,295 color signatures were
  reconstructed and compared with direct values;
- `scripts/verify_all.ps1`, 130.1 seconds, including complete C=2..5 gates,
  the read-only C=6 class-1 gate, FJ9 71/71 reproduction, and the independent
  FJ9 combinatorial route.

The active graph checkpoint and its external safety copy both remained

```text
SHA-256 = FE8B68DE6C15592848D7CF69BF43928C000F0B2EE59FC263A094BCA2D376A865
```

before and after the C=6 runs.

## Signature and sharing evidence

Uniform samples were taken by scanning the complete sorted source layer and
selecting evenly spaced records.

| Sample | Whole signatures | Half occurrences | Unique half keys | Duplicate half occurrences |
|---|---:|---:|---:|---:|
| G2, 2,000,000 states | 2,000,000 | 4,000,000 | 1,925,254 | 51.87% |
| G2, 10,000,000 states | 10,000,000 | 20,000,000 | 4,455,199 | 77.72% |
| G1, 2,000,000 states | 2,000,000 | 4,000,000 | 1,524,598 | 61.89% |

For the 10-million G2 sample, half-key frequency-of-frequency statistics were

```text
f1 = 1869836
f2 = 817358
f3 = 454416
f4 = 287478
f5 = 195610
Chao1 estimated unique half keys = 6593973
maximum sampled frequency = 626
```

The G1/G2 two-million samples shared 628,495 raw half keys.  The intersection
covered 63.49% of G1 half occurrences and 48.55% of G2 half occurrences.

Color canonicalization is much stronger at the half level.  A uniform sample
of 100,000 distinct raw G2 half keys collapsed to 46,598 color-canonical keys;
the Chao1 estimate was 69,949 canonical half keys.  This is evidence for a
small reusable half-kernel universe, not a theorem about its complete size.

The complete tail does not show the same sharing.  In a uniform sample of
100,000 G2 states:

```text
raw sufficient signatures = 100000 unique
color sufficient signatures = 100000 unique
color-signature reconstructions checked = 1000/1000
```

All 2,295 color signatures in the C=5 class-300 validation were also unique.
Thus whole-tail value memoization has no measured within-class reuse; the
useful sharing is in half kernels.

Committed analysis manifests include:

```text
data/logs/future-tail-signature-c6-g2-sample10m-20260715/class-2/
data/logs/future-tail-signature-c6-g1-sample2m-vs-g2-20260715/class-1/
data/logs/future-tail-colorcanon-c6-g2-sample2m-100k-20260715/class-2/
data/logs/future-tail-color-signature-c6-g2-100k-20260715/class-2/
```

## Controlled 100,000-state benchmark

All rows below use the same evenly spaced G2 sample and a 200-million kernel
record cap.  Nanoseconds cover only direct tail evaluation, not the sequential
9.08-GB source scan.

| Policy | Direct time | Local assignments | Kernel hits / 200,000 | Raw color mappings retained | Peak working set |
|---|---:|---:|---:|---:|---:|
| no color quotient | 66.609 s | 678,546,283 | 23,565 | 0 | 1.738 GB |
| cache all six refined mappings | 18.661 s | 110,554,688 | 171,282 | 502,642 | 0.462 GB |
| choose before refining unknown halves | 18.932 s | 166,268,424 | 150,252 | 173,934 | 0.644 GB |
| refine six, retain selected two | 16.537 s | 110,554,688 | 171,282 | 177,662 | 0.464 GB |
| refine six, retain no raw mappings | 16.548 s | 110,554,688 | 171,282 | 0 | 0.453 GB |

The retained stateless policy is 4.03 times faster than the non-color tail and
11.3% faster than the first exact color-refined implementation.  It preserves
the high kernel hit rate while removing the unbounded raw mapping table.

Two negative results were important:

- eagerly filling complete raw color orbits took about 60.7 seconds for the
  same sample, created roughly 49 million mapping entries, and used 3.03 GB;
- computing only the two initially selected mappings saved mappings but lost
  kernel-cache information, increasing local assignments by about 50%.

## Parallel pressure tests

The first all-six cached implementation was stopped at its 30-minute/8-GB
bound.  It reached 95.7 million states and 7.755 GB, so it was rejected despite
being exact on completed records.

With transient six-half refinement and selected-two retention, a five-minute
24-thread run reached 22.3 million states at 2.862 GB.  With no raw mapping
retention, the same prefix and throughput used 2.103 GB and the sampled working
set stayed near 2.07 GB.  Both runs were intentionally terminated at the time
bound; neither partial accumulator was used.

## Complete G2 read-only rescan

After the final full repository gate, the stateless implementation was run as

```powershell
.\build\factorization_orbit.exe 6 start=1 limit=1 future futureprogress `
  futureorder=pair-adaptive-tail futuretailrows=7 `
  futuretailcolorcanon futuretailforcerescan `
  futuretailthreads=24 futuretailchunkrecords=240000 `
  futuretailcacherecords=200000000 `
  futureexternal=data/logs/future-external-c6-g2-adaptive-tail7-layers2-20260714 `
  futureexternallayers=2 checkpointreadonly
```

An outer monitor enforced 4 GB and 75 minutes.  The run completed normally
and the recomputed value matched the committed manifest:

```text
F6(G2) = 7053808087203840
tail states = 221438460
tail time = 3634.499 s
average throughput = 60927 states/s
peak working set = 2402320384 bytes = 2.237 GiB
zero states = 295
mean selected support = 4035.736
maximum support = 11936
maximum tail value = 14980
local assignments = 391493454865
kernel hits = 347136486 / 442876920 = 78.38%
kernel evictions = 95650887
color refinements = 1328630760
raw color mappings retained = 0
```

Relative to the prior committed non-color tail statistics, local assignment
work fell by 55.1%.  The full run was slower than the layer-head five-minute
projection because later source regions have different support and cache
behavior; full-layer timing, not prefix timing, must be used for planning.

Logs are under

```text
data/logs/future-tail-color-stateless-c6-g2-full-20260715.*.log
```

## What is and is not unblocked

The G2 single-class tail is now independently reproducible in bounded memory.
This removes the previous raw-mapping RAM failure and confirms that the expert
separator idea led to a useful exact kernel symmetry.

It does not make the complete C=6 sum feasible:

- one hard class still has a 9.08-GB committed frontier and a one-hour tail;
- sampled complete tail signatures are unique, so a scalar continuation memo
  does not remove the per-state inner product;
- the 24 workers still maintain private kernel caches, causing 95.7 million
  kernel evictions in G2;
- the cost distribution over the remaining 63,197 outer classes is unknown;
- no exact cross-class persistent kernel database or batched kernel-pair
  contraction has yet been implemented.

The next bounded engineering target is therefore a versioned immutable table
of color-canonical half kernels, followed by exact cross-class overlap and
batched inner-product measurements on positive-limit later classes.  A full
63,199-class run remains unauthorized and unjustified.
