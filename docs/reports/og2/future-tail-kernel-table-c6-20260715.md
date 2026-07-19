# C=6 color-canonical half-kernel inventory and shared table

Date: 2026-07-15
Platform: Windows, MinGW C++20, OpenMP, 24 tail workers

## Scope

This report records a bounded exact experiment.  It does **not** compute
`N(6)` and does not authorize a 63,199-class run.  It inventories every
color-canonical seven-row half kernel in the already committed G1 and G2
frontiers, builds one immutable shared table, and forces both complete tails
through that table under `checkpointreadonly`.

The immutable source layers were:

| graph | records | source bytes | committed factorization count |
|---|---:|---:|---:|
| G1 | 56,461,626 | 2,314,926,706 | 6,986,348,258,918,400 |
| G2 | 221,438,460 | 9,078,976,900 | 7,053,808,087,203,840 |

Before writable C=6 analysis, `scripts\verify_all.ps1` passed in 141 seconds.
The active checkpoint and its external safety copy both retained SHA-256

```text
FE8B68DE6C15592848D7CF69BF43928C000F0B2EE59FC263A094BCA2D376A865
```

## Exact inventory format

`futuretailinventory=DIR` with a positive
`futuretailinventoryrecords=` now scans all six candidate halves of every
selected seven-row state.  Each half is canonicalized under `D8 x S_C`.
The transactionally committed `FJFTKI01` file stores, in sorted key order:

```text
color-canonical key
number of occurrences
kernel support
number of local assignments needed to construct the kernel
```

The reader checks the header, exact file length, strictly increasing keys, and
a write/read round trip.  When a reference inventory is supplied, every
shared key must have identical support and local-assignment count.

The full inventories gave:

| metric | G1 | G2 |
|---|---:|---:|
| tail states | 56,461,626 | 221,438,460 |
| six-half occurrences | 338,769,756 | 1,328,630,760 |
| canonical unique keys | 43,722 | 108,525 |
| singleton keys | 97 | 745 |
| maximum key frequency | 59,127 | 222,625 |
| kernel records | 52,343,806 | 117,509,751 |
| kernel payload bytes | 418,750,448 | 940,078,008 |
| zero-support keys | 72 | 22 |
| maximum support | 5,968 | 7,906 |

G1 took 61.469 seconds for the inventory stage; G2 took 242.683 seconds.

## Exact G1/G2 overlap

The G2 inventory was audited against the complete G1 inventory:

```text
intersection keys                         = 38,841
G2 unique-key coverage                    = 35.7899%
G1 unique-key coverage                    = 88.8362%
G2 occurrence coverage                    = 62.3572%
G1 occurrence coverage                    = 98.7507%
intersection kernel records               = 51,155,809
union keys                                = 113,406
union kernel records                      = 118,697,748
```

The important point is that shared keys are hot.  Only 35.8% of G2's unique
keys cover 62.4% of its half occurrences.  Adding all of G1 to the G2 kernel
payload adds only 1,187,997 records, about 9.1 MiB.

## Immutable shared table

`futuretailkerneltable=DIR` requires `futureexternal`,
`checkpointreadonly`, `futuretailrows=7`, and `futuretailcolorcanon`.
With an inventory in the same command it builds or audits a union table;
without an inventory it loads an already committed table.

The `FJFTKT01` file has a sorted 24-byte key/offset/length index and one flat
array of 8-byte `(signature, weight)` records.  It is written to a temporary
file, compared byte-for-byte with the generated kernels in bounded chunks,
renamed, and made visible only by a final `COMMITTED.manifest`.  Loading checks
the complete file size, checksum, contiguous offsets, key order, nonzero
weights, and strict signature order inside every kernel.  A single loaded
instance is shared read-only by all OpenMP workers.

The G1/G2 union table is:

```text
keys                    = 113,406
kernel records          = 118,697,748
kernel payload bytes    = 949,581,984
total file bytes        = 952,303,792  (908.2 MiB)
internal checksum       = 3028732484997102959
SHA-256                 = 617A795CFE91D544FEFF1E795953F8C725C834C57E6502E5B3541AD98C8F182E
build/round-trip time   = 5.010 s
peak working set        = 0.936 GiB
```

The generated table remains an ignored experimental artifact under
`data/logs/`; it has not been promoted to the active checkpoint manifest.

## Independent exact gates

The table path first passed a C=4 build/load/forced-rescan self-test.  A full
C=5 class-300 inventory then built a 729-key, 347,555-record table and forced
all 2,295 tail states through it:

```text
F5(class 300)        = 1324247040
kernel hits          = 4590 / 4590
local assignments    = 0
kernel evictions     = 0
tail time            = 0.017 s
```

The complete repository gate passed after these code changes.

## G2 benchmark and complete rescan

On the same evenly spaced 100,000 G2 states, current code measured:

| path | direct time | local assignments | kernel hits | evictions |
|---|---:|---:|---:|---:|
| bounded private kernel cache | 17.624 s | 110,554,688 | 171,282/200,000 | 0 |
| immutable shared table, private cache disabled | 6.156 s | 0 | 200,000/200,000 | 0 |

The shared table is 2.86x faster than the same-tree private-cache control on
this sample.  The older stateless color path took 16.548 seconds, giving a
2.69x comparison across revisions.  The original raw path took 66.609
seconds.  Scanner wall time is not used for these ratios because it includes
reading the complete 9.08-GB source file.

The decisive run ignored the saved tail accumulator and recomputed all G2
states:

```text
tail states             = 221,438,460
F6(G2)                   = 7053808087203840
tail time                = 824.440 s
class time               = 825.990 s
peak working set         = 0.956 GiB
zero states              = 295
mean selected support    = 2817.108
maximum selected support = 11159
maximum tail value       = 14980
kernel hits              = 442876920 / 442876920
local assignments        = 0
kernel evictions         = 0
color refinements        = 1328630760
```

The previous complete stateless-color rescan took 3,634.499 seconds and
peaked at 2.237 GiB.  The shared-table path is therefore 4.41x faster and uses
less than half the peak working set.  Exact support from all six table hits
also improves cut selection: mean selected support fell from 4,035.736 to
2,817.108.

## Cross-graph G1 rescan

The same G1/G2 table then forced the complete G1 tail:

```text
tail states             = 56,461,626
F6(G1)                   = 6986348258918400
tail time                = 219.709 s
class time               = 267.432 s
peak working set         = 1.167 GiB
mean selected support    = 2846.071
kernel hits              = 112923252 / 112923252
local assignments        = 0
kernel evictions         = 0
color refinements        = 338769756
```

The larger class-minus-tail difference is G1 prefix work, not table loading.
This run verifies actual cross-outer-graph consumption rather than only a
same-key metadata comparison.

## What is unblocked

- G1 and G2 no longer regenerate color-canonical kernels in private worker
  caches.
- The 95.7-million G2 eviction problem is removed.
- One shared table gives exact support-aware cut selection to every worker.
- G2's full tail now fits near 1 GiB and takes 13.7 minutes rather than about
  one hour.
- The full G1/G2 kernel universe is small enough to load once per process.

## What still blocks N(6)

1. Only two of 63,199 outer graphs have complete frontier and inventory data.
   Later-class table coverage and class-cost distribution are unknown.
2. The table does not remove the class-local frontier.  G2 still requires a
   9,078,976,900-byte final layer before the tail is evaluated.
3. A 13.7-minute G2 tail repeated independently over 63,199 classes is still
   impossible.  Even treating every class like G2 would be roughly 603 serial
   days for tails alone.
4. The hot path has moved to 1.329 billion color refinements, shared-key
   lookups, and 442.9 million exact kernel contractions.  Whole color-tail
   signatures were unique in the retained 100,000-state sample, so scalar
   whole-tail memoization remains unsupported.
5. The union table is complete only for G1 and G2.  Missing later-class keys
   fall back exactly, but their frequency and payload have not been measured.
6. No cross-class prefix recurrence or global outer contraction has yet
   removed the need to construct a large frontier per outer graph.

## Next bounded experiments

The next useful work is not another unconstrained class sweep.

1. Add a bounded later-class prefix/sample probe and measure what fraction of
   its six-half occurrences hit the G1/G2 table before committing a full
   frontier.
2. Measure repetition of `(left key, relative D8, relative color)` and of the
   unordered kernel-key pair.  If those partial signatures repeat, cache the
   transformed left target list or batch contractions even though complete
   signatures are unique.
3. Replace node-based shared-key lookup with a flat read-only index and carry
   selected table slices from cut selection into contraction, avoiding two
   repeated lookups per state.
4. Investigate consuming reduced layer-5 runs incrementally so a tail can be
   accumulated without retaining one monolithic 9-GB committed layer, while
   preserving closed resumable boundaries.
5. Reopen a global contraction only if it shares prefix or kernel-pair work
   across many outer graphs; class-local tail acceleration alone cannot close
   `N(6)`.

## Retained artifacts

The principal ignored artifacts are:

```text
data/logs/future-tail-kernel-inventory-c6-g1-full-20260715/class-1/
data/logs/future-tail-kernel-inventory-c6-g2-full-vs-g1-20260715/class-2/
data/logs/future-tail-kernel-table-c6-g1-g2-20260715/
data/logs/future-tail-kernel-table-c6-g2-full-rescan-20260715.*.log
data/logs/future-tail-kernel-table-c6-g1-full-rescan-20260715.*.log
```

No long-running process remained, and checkpoint hashes were unchanged after
the complete rescans.
