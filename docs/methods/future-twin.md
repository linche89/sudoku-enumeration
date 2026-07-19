# Future-twin factorization recurrence

This is the audited optional backend implemented by `src/future_twin.hpp` and
selected by `factorization_orbit.exe ... future`.  It does not replace the
default factorization engine and does not read or write the graph-memo
checkpoint.

## Exact state and transition

After some left vertices have been processed, every right column is represented
by

```text
(tau, K)
```

where `tau` is its remaining left neighbourhood and `K` is the set of colors
already used at that column.  Reachable states satisfy

```text
|K| = C - |tau|,
each color occurs in exactly the number of processed rows.
```

Columns with the same record are future twins.  Suppose the next row meets a
record group `(tau, K)` with multiplicity `m`.  The transition chooses an
`m`-subset of the colors not in `K`; choices for all affected groups are
disjoint and together use every color once.  The group contributes a factor
`m!`, because its columns are state-identical but remain labelled in the
underlying graph.  Thus the exact transition multiplier is

```text
product over affected groups of m!
```

and no perfect matching or residual `Q-M` graph is materialized.

## Canonical keys

The fixed-row mode quotients column records and color names.  Its refinement
canonicalizer is checked against brute force over all `S_C` color
permutations.

The joint modes additionally quotient the names of all remaining rows.  A
small exact individualization/refinement search canonicalizes the colored
bipartite incidence structure consisting of remaining rows, colors, and right
columns.  The retained C=6 order is `pair-adaptive-tail`: it chooses the first
row of a complementary pair by exact grouped transition leaf count and then
processes its mandatory mate.  When seven rows remain, an exact pair-symmetry
tail kernel closes the state without materializing six more frontiers.

Joint canonicalization has a per-call node budget.  If the budget is exhausted,
the algorithm stores the sorted labelled state as a safe weak key.  Weak keys
carry a separate key-kind bit, so they cannot collide with strong canonical
keys.  This fallback can only miss an isomorphism merge; it cannot merge two
different states or alter the count.

## Verification contract

The permanent tests are:

```powershell
.\build\factorization_orbit.exe 4 futuretest
.\build\factorization_orbit.exe 4 futurecheck pivot rooted4
.\build\factorization_orbit.exe 4 futurecheck futureorder=canonical-last pivot rooted4
.\build\factorization_orbit.exe 4 futurecheck futureorder=canonical-last `
    futurecanonbudget=1 pivot rooted4
.\build\factorization_orbit.exe 5 futurecheck pivot rooted4
.\build\factorization_orbit.exe 5 futurecheck `
    futureorder=canonical-last pivot rooted4
```

`futuretest` checks canonical invariance/separation for `C=2..6` and compares
grouped transitions with labelled enumeration for random `C=2..4` graphs.  It
also compares the six- and seven-row tails with the full recurrence and tests
the sufficient raw and color-canonical tail signatures by exact
reconstruction.  The color refinement is checked against brute `D8 x S_C`
enumeration.  The test also covers external sort/reduce, intentional
parent-boundary interruption, generation restart, partial tail restart, and
completed read-only restart.  `futurecheck` compares every outer class with
the current exact engine, rather than comparing only the final weighted sum.
The forced budget-one test makes the weak-key path active and verifies that it
remains exact.

## External exact layers and checkpoints

With `futureexternal=DIR`, the last one or more prefix layers use fixed 41-byte
records.  Each in-memory buffer is sorted and locally reduced; bounded-fan-in
merges produce one globally sorted layer.  File headers and manifests store
the record count, exact 128-bit mass, processed-row count, and expected byte
length.

Generation manifests are committed only after a complete parent state.  A
restart validates every referenced run and discards any later run or temporary
merge suffix.  Original run files remain available until the final layer and
manifest have committed, so interruption during a merge cannot destroy the
last restart point.  Tail manifests similarly store only a closed exact
accumulator.  `checkpointreadonly` requires `COMMITTED.manifest` and suppresses
all external writes.

OpenMP workers evaluate a bounded record batch, after which results are
committed serially in file order with checked `unsigned __int128` arithmetic.
`futuretailcacherecords=` is a global cap divided across workers.
`futuretailchunkrecords=` controls the locality/load-balance tradeoff; 240,000
records is the verified Windows default.

## Seven-row color symmetry

Each seven-row cut produces two half kernels.  Besides D8 row symmetry, a half
kernel is invariant under a global color renaming.  The optional
`futuretailcolorcanon` path computes an exact `D8 x S_C` representative by
refining colors with their eight incidence profiles and enumerating only
inside equal-profile cells.  The two canonical half keys are joined using the
relative D8 transform and the relative color permutation, so the quotient
does not discard information.

All six candidate halves are canonicalized before choosing the cut.  This is
important: choosing a cut before unknown halves are canonicalized loses kernel
cache hits.  Raw-to-canonical mappings are deliberately not cached in the
production path.  On G2, recomputing six small refinements per state had no
measurable 100,000-state cost, while retaining raw mappings caused unbounded
parallel memory growth.  Only half kernels are cached, under the existing
`futuretailcacherecords=` bound.

The exact diagnostic scanner accepts `futuretailscan=DIR`, an evenly spaced
`futuretailsamples=N`, and optional `futuretailvalidate=N`,
`futuretailbenchmarkrecords=N`, `futuretailcolorsamples=N`,
`futuretailcolorsignatures=N`, and `futuretailreference=DIR`.  A color-tail
signature stores two color-canonical half keys plus their relative D8 and
relative color transforms; reconstruction from it is exact.  Scanner output
is transactionally committed only after the complete source layer has been
read.

External use of `futuretailcolorcanon` and `futuretailforcerescan` requires
`checkpointreadonly`.  This keeps experimental color counters out of the
existing resumable tail-checkpoint identity and prevents a verification run
from writing partial state.

## Color-canonical kernel inventory and table

`futuretailinventory=DIR` and a positive
`futuretailinventoryrecords=N` scan all six candidate halves of each evenly
selected seven-row state.  The committed `FJFTKI01` inventory is sorted by the
exact `D8 x S_C` key and stores occurrence count, support, and local-assignment
count.  `futuretailinventoryreference=CLASS_DIR` performs a merge audit;
shared keys must have identical support and local-assignment count.

`futuretailkerneltable=DIR` loads one immutable `FJFTKT01` table.  If an
inventory is supplied in the same command, the table is first built from the
union of the current and optional reference inventories.  The file contains a
flat 24-byte key/offset/length index and contiguous 8-byte kernel records.  It
is committed only after an exact bounded-memory write/read comparison.
Loading checks the file checksum, offsets, key order, and every kernel's
signature order.  All OpenMP workers share one read-only instance.

The table participates in cut choice: because all six candidate keys can be
looked up before selection, the work proxy uses exact kernel support rather
than a local-assignment estimate.  The final contraction iterates the smaller
of the two kernels and applies the inverse row and color transforms when the
sides are swapped.  A missing table key falls back to the exact existing
generator and bounded private cache.

External table use requires `futureexternal`, `checkpointreadonly`,
`futuretailrows=7`, and `futuretailcolorcanon`.  The complete G1/G2 table has
113,406 keys, 118,697,748 records, and a 952,303,792-byte file.  Complete
forced G1 and G2 rescans hit every selected kernel and reproduce both exact
factorization counts.  Format, overlap, checksums, and timings are recorded in
`../reports/og2/future-tail-kernel-table-c6-20260715.md`.

## C=6 use

Future mode is intentionally cold and rejects `checkpoint=...`.  A bounded
pair-tail job is configured as follows; it must still be wrapped in explicit
time and RSS guards:

```powershell
.\build\factorization_orbit.exe 6 future futureprogress `
    futureorder=pair-adaptive-tail futuretailrows=7 `
    futureexternallayers=2 futureexternalrecords=500000 `
    futureexternalfanin=32 futurelayercheckpointparents=100000 `
    futuretailcheckpointrecords=1000000 futuretailthreads=32 `
    futuretailcacherecords=100000000 futuretailchunkrecords=240000 `
    futureexternal=data\logs\future-c6-pair-tail start=0 limit=1
```

Use `start=1 limit=1` for G2.  The two independently closed values are

```text
F6(G1) = 6986348258918400
F6(G2) = 7053808087203840
```

For read-only reopening, repeat the identical class identity and add
`checkpointreadonly`.  The complete G2 layer has 221,438,460 records and is
9,078,976,900 bytes, so class-local external closure does not by itself make a
63,199-class sum feasible.  Implementation and production evidence are in
`../reports/og2/future-pair-tail-external-c6-20260714.md`.

To force an independent color-symmetry rescan of the committed G2 layer while
ignoring its saved tail accumulator:

```powershell
.\build\factorization_orbit.exe 6 start=1 limit=1 future futureprogress `
    futureorder=pair-adaptive-tail futuretailrows=7 `
    futuretailcolorcanon futuretailforcerescan `
    futuretailthreads=24 futuretailchunkrecords=240000 `
    futuretailcacherecords=200000000 `
    futureexternal=data\logs\future-external-c6-g2-adaptive-tail7-layers2-20260714 `
    futureexternallayers=2 checkpointreadonly
```

Wrap this command in explicit RSS and time guards.  The verified Windows run
took 3,634.499 seconds, peaked at 2.237 GiB, and reproduced
`F6(G2)=7053808087203840`.  Signature data, controlled ablations, and the full
rescan are in
`../reports/og2/future-tail-color-symmetry-c6-20260715.md`.

To force G2 through the committed G1/G2 table with no private kernel cache:

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

The verified Windows run took 824.440 tail seconds, peaked at 0.956 GiB, and
hit all 442,876,920 kernel lookups.  This is still a bounded single-class
verification, not evidence that 63,199 independent jobs are feasible.
