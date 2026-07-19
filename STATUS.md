# Current Project Status

Last updated: 2026-07-19

This is the single authoritative status page. Dated reports preserve evidence;
historical handoffs and raw expert responses are not current project state.

## Stable results

The FJ05 9x9 reproduction is complete and independently cross-checked against
all 71 reference classes:

```text
N0 = 6670903752021072936960
```

The exact 2xC counts are verified by multiple engines:

| C | N(C) |
|---:|---:|
| 2 | 288 |
| 3 | 28200960 |
| 4 | 29136487207403520 |
| 5 | 1903816047972624930994913280000 |

The primary exact engine is `src/factorization_orbit.cpp`. Five fresh C=5
full gates averaged about 8.66 seconds with `pivot rooted4` on the reference
machine.  A same-binary controlled ablation measured factor-stage times of
53.35 seconds for the plain recurrence, 36.93 seconds with only the rooted
edge pivot, 11.48 seconds with only the degree-four split, and 8.58 seconds
with both.  See `docs/reports/og2/c5-paper-ablation-20260712.md`.

The optional future-twin backend independently agrees with the primary engine
on every outer class for complete C=2..5. It is selected explicitly with
`future`; the default engine and checkpoint format are unchanged.

An independent reverse row-block gluing prototype now also closes complete
C=2..5.  At C=5 it constructs 17,120 four-row configuration orbits by exact
`2+2` gluing, joins them to one row, reproduces all 355 complete classes and
the known `N(5)`, and agrees with `factorization_orbit` on every sorted
`(coordinate orbit, labelled multiplicity, F)` triple.  It is a verified
decision prototype under `experiments/proto/`.  Its bounded C=6 probe now
constructs the exact 772-orbit two-row layer, but it has not generated a
complete four-row layer or entered the outer contraction.

## Historical C=6 target

The C=6 decimal value is not an unpublished target.  Kjell Fredrik Pettersen
announced the following count on the New Sudoku Players' Forum on 2006-11-14:

```text
N(6) = 38296278920738107863746324732012492486187417600000
```

The same thread reports 63,199 outer band-configuration classes and describes
the weighted square sum used to obtain the result.  No buildable historical
source or independent executable verification has been located in the current
literature audit.  This project's C=6 objective is therefore an independent,
open verification of that historical claim, not discovery of a new integer.
See `docs/reports/og2/literature-audit-20260712.md`.

## C=6 frontier

There are exactly 63,199 outer skeleton orbits. The first one is closed:

```text
F6(G1) = 6986348258918400
class time = 706.652 s
peak working set = 665.6 MiB
```

The future-twin backend now gives an independent cold reproduction with no
checkpoint input:

```text
F6(G1) = 6986348258918400
class time = 1592.867 s
peak working set = 1.670 GiB
peak frontier = 10025564 states
canonical fallbacks = 0
```

The optional pair-tail backend now also closes the second, lower-symmetry
graph exactly:

```text
F6(G2) = 7053808087203840
pair-adaptive prefix states = 1, 1, 352, 77956, 9664963, 221438460
layer-5 raw records = 607148632
layer-5 exact mass = 2009438804160
layer-5 file bytes = 9078976900
```

The exact seven-row tail processed all 221,438,460 states.  It had 295 zero
states, mean support 3608.906, maximum support 11,936, and maximum tail value
14,980.  Parent-boundary generation and tail checkpoints were resumed after
bounded stops.  A separate `checkpointreadonly` reopening returned the same
G2 value in 7.665 seconds of class time without writing state.

An independent color-symmetry tail rescan then forced all 221,438,460 states
through a `D8 x S6` half-kernel quotient instead of accepting the saved tail
accumulator.  It reproduced the same G2 value in 3,634.499 seconds with 24
threads and peaked at 2,402,320,384 bytes (2.237 GiB).  It made
347,136,486/442,876,920 kernel lookups hit, reduced local assignment work to
391,493,454,865, and retained no unbounded raw color-mapping cache.  The run
was read-only, used `start=1 limit=1`, and passed the existing committed-result
audit.

A complete color-canonical half-kernel inventory has now replaced that
private-cache experiment with one versioned immutable table.  G1 contains
43,722 unique half keys, G2 contains 108,525, and their intersection is
38,841.  The union has 113,406 keys and 118,697,748 sparse kernel records.  Its
flat file is 952,303,792 bytes (908.2 MiB) and is shared read-only by all
workers.  A forced full G2 rescan reproduced the exact value in 824.440 tail
seconds, peaked at 0.956 GiB, hit all 442,876,920 kernel lookups, generated no
local assignments, and made no kernel evictions.  The same table forced G1 in
219.709 tail seconds with all 112,923,252 lookups hitting and reproduced
`F6(G1)` exactly.

This removes the G1/G2 private-kernel generation, eviction, and tail-RAM
walls.  It does not remove the remaining scaling wall: one class-local job
still needs a 9.08 GB frontier and hundreds of millions of tail states, a G2
tail still takes 13.7 minutes, and table coverage and frontier cost are unknown
for the other 63,197 classes.  No full C=6 count is currently in progress.

## Audited route decision

The single-graph aggregation question now has a verified optional backend. A
future-twin recurrence processes the 12 left vertices directly and identifies
right columns by their remaining neighborhood and used-color set
`(tau, K)`.  It never enumerates a top perfect matching, constructs a residual
`Q-M`, or calls `F5` separately. Canonical keys, labelled transition
multiplicities, complete per-class C=2..5 agreement, a forced safe-fallback
gate, and the cold G1 value have all been verified.  Paired row processing, an
exact seven-row tail, external sort/reduce, and parent-boundary transactions
now close G1 and G2 within bounded memory.  They do not provide the cross-class
prefix reuse or global contraction needed for the complete outer sum.  The
shared half-kernel table does provide exact cross-class reuse at the leaf
kernel level, but it leaves each outer graph's prefix and kernel-pair
contractions separate.

The proposed natural 3+3 low-rank route did not pass its required small-C
decision gate.  The exact symmetry-block endpoint exists, but reproduced
certificates give full rank 630/630 at C=4 and full row rank 8001/8001 at C=5.
At C=6 every domain multiplicity fits inside the midpoint module, so symmetry
forces no channel loss.  This rules out ordinary irreducible-channel
truncation as a justified C=6 implementation route; it does not rule out a
different fast implicit contraction for a full-rank operator.

The 132-state global recurrence supplied on 2026-07-15 is exact for the linear
quantity `sum_[G] w([G]) F_C(G)`, but the Sudoku objective is the weighted
square `sum_[G] w([G]) F_C(G)^2`.  Its C=2 and C=3 outputs are 96 and 460800,
not 288 and 28200960.  This route resulted from a question that omitted the
square and is out of scope for `N(6)`.

The corrected 2026-07-17 symbol-synchronous connectivity recurrence does
compute the square.  It replaces each partial degree-two band graph by vertex
degrees plus a pairing of live path endpoints; completed cycles are deleted
and contribute an exact factor two.  Local Windows builds reproduced
`N(2)`, `N(3)`, and `N(4)`, stack-swap invariance, sampled group-canonical
invariance, and the optimized two-symbol initializer key by key.  The C=5
probe has state counts `1, 1, 93, 83776` after only three of ten symbols.  A
direct next step can inspect 1,206,374,400 permutation pairs, so connectivity
compression alone does not pass the C=5 gate and is not yet a C=6 route.  The
proposed operator-valued double-permanent subset DP has not been implemented
or measured.

A box-order joint-histogram prototype now computes the same squared objective
globally without enumerating outer classes.  Its state records the number of
symbols of every pair of used-color masks `(S,T)`.  The exact transition uses
the labelled contingency multiplicity and the cycle factor for compatible
shared cuts; orbit-total midpoint contraction includes the state-dependent
color-orbit size.  Complete Windows runs reproduce N(2), N(3), and N(4) both
sequentially and through complementary midpoint contraction.  An independent
labelled-symbol kernel agrees with every raw transition coefficient through
C=4.  The orbit layers are `1,2,1`, `1,3,3,1`, and `1,5,141,5,1`; disabling
copy swap changes the C=4 middle layer to 232 while preserving the result.
The current prototype intentionally refuses C>4, so C=5 growth remains the
next bounded decision rather than an established C=6 route.

Reverse row-block gluing has now passed the decision gate that the other open
global proposals have not.  The exact orbit formula needs stabilizer orders
but no stored transporters.  After cancelling repeated right-orbit images,
quotienting by the left stabilizer, and using the 2+2 block-exchange symmetry,
complete local runs reproduce C=2..4 and all C=5 classes.  The exact C=5
inventories are:

```text
two-row labelled configurations = 165744
two-row coordinate orbits       = 107
unordered 2+2 orbit pairs       = 5778
four-row coordinate orbits      = 17120
four-row coordinate mass        = 62185328
complete classes                = 355
```

Thirteen closed pair intervals covered `[0,5778)` without gaps or overlaps.
Their exact merge followed by the 4+1 join reproduced
`N(5) = 1903816047972624930994913280000`; the 4+1 stage took 6.20 seconds.
The intentionally independent interval kernels accumulated 122,166,792
contingency leaves and 95,227,539 cold canonicalizations.  This verifies the
mathematics while identifying the real scaling wall: the number of
configuration states and low-stabilizer canonical outputs, not merely the 40
or 60 masks available to one symbol.

The positive-limit C=6 frontier probe is now complete.  An exact
cycle/matching quotient, differentially checked at C=3..5, found 20,338,525
labelled-coordinate two-row configurations in 772 coordinate orbits; an
independent raw DFS confirmed the labelled count.  These give 298,378
unordered orbit pairs.  First/middle/last sampled pairs had 10, 1,448, and 43
double-coset placements.  One exact placement from each emitted 2,662,
15,360, and 27,793 contingency leaves; the middle placement had 15,168
distinct raw keys, and its first 2,000 raw keys remained 2,000 distinct
canonical four-row representatives.  Cold C=6 canonicalization measured only
hundreds of outputs per second.

The stabilizer inventory gives a stronger exact lower bound: 276 two-row
orbits have trivial stabilizer, so their 38,226 unordered pairs alone require
1,761,454,080 double-coset placements.  A delayed first-mask anchor avoids
computing a stabilizer for every raw key and reached about 8,195 keys/s, with
100/100 sampled keys matching full canonicalization.  But a 100,000-leaf
middle-pair prefix still reduced only from 99,808 raw keys to 99,423 canonical
keys.

Therefore the expert's proposed cheap implementation -- brute group summation
per orbit pair with immediate canonicalization -- does not pass the C=6 scale
gate.  Reverse gluing remains historically plausible only as a substantially
different bulk/external lookup design.  No complete C=6 four-row gluing layer
has been generated.

The external derivations, supplied code, local reproduction commands, hashes,
and limitations are recorded in
`docs/reports/og2/c6-expert-routes-audit-20260713.md`.
The implemented recurrence and its C=6 evidence are recorded in
`docs/methods/future-twin.md` and
`docs/reports/og2/future-pair-tail-external-c6-20260714.md`.  The exact
color-symmetry signature, benchmarks, negative results, and complete G2
read-only rescan are recorded in
`docs/reports/og2/future-tail-color-symmetry-c6-20260715.md`.  The complete
G1/G2 inventories, exact overlap, shared-table format, benchmarks, and forced
G1/G2 table rescans are recorded in
`docs/reports/og2/future-tail-kernel-table-c6-20260715.md`.  The linear-sum
scope correction, connectivity reproduction, differential checks, C=5 wall,
and current route portfolio are recorded in
`docs/reports/og2/c6-route-portfolio-20260717.md`.
The reverse-gluing derivation, exact C=4/C=5 gates, interval-file evidence,
and C=6 decision boundary are recorded in `docs/methods/reverse-gluing.md` and
`docs/reports/og2/reverse-gluing-c4-c5-20260718.md`.  The exact C=6 two-row
inventory and bounded four-row samples are recorded in
`docs/reports/og2/reverse-gluing-c6-frontier-probe-20260718.md`.
The exact joint-histogram recurrence, orbit normalization, C=2..4 differential
gates, and current boundary are recorded in `docs/methods/joint-histogram.md`
and `docs/reports/og2/joint-histogram-c2-c4-20260719.md`.

## Route portfolio

`Rejected` means that the named compression mechanism failed its exact
decision test, not that every future algorithm using related mathematics is
impossible.

| status | route | current evidence |
|---|---|---|
| rejected as primary | 132-state outer marginalization | Solves `sum wF`, not the required square |
| rejected as primary | ordinary 3+3 low-rank truncation | Full C=4/C=5 ranks and no forced C=6 channel loss |
| rejected as primary | matching/Johnson low-rank collapse | Exact local rank is 462/462 |
| rejected as primary | materialized midpoint or ordinary Burnside/character basis | Exact coordinate spaces are no smaller than the orbit list and the useful midpoint is much larger |
| rejected as primary | full `C^3` count tensor | Weaker than connectivity already at C=4; the bounded local full run did not finish |
| rejected as primary | naive independent 63,199-class sweep | G2 needs a 9.08-GB frontier and 824.440-s tail |
| rejected as primary | whole-tail scalar memoization within G2 | 100,000/100,000 sampled complete G2 signatures are distinct; cross-class lower-level reuse is still open |
| proven component only | factorization/orbit and future-twin per-class engines | Fast C=5 and exact G1/G2 values, but no affordable global outer sum |
| proven component only | external pair-tail and half-kernel table | Closes G1/G2 and speeds the G2 tail 4.41x, but leaves every prefix separate |
| proven component only | connectivity/path quotient | Exact through C=4; already 83,776 states at C=5 symbol 3 |
| proven component only | streaming/checkpoint infrastructure | Controls RAM and restart risk, not total arithmetic |
| exact through C=5; naive C=6 join fails scale gate | reverse 4+2 row-block gluing | Exact C=6 inventory is 772 two-row orbits / 298,378 pairs; trivial stabilizers force at least 1.761B double cosets, and a generic 100k-leaf prefix is 99.4% distinct after anchored canonicalization; only a bulk/external redesign remains open |
| exact through C=4; next bounded decision | box-order joint-histogram pair DP | Exact raw-transition differential, midpoint, and known-total gates; C=4 middle layer is 141 states, but C=5 is unimplemented |
| queued global decision experiment | operator-valued double-permanent subset DP | Exact proposal; no implementation or C=5 frontier measurement |
| open decision experiment | later-class table coverage and kernel-pair batching | Complete data exist only for G1/G2 |
| open, lower-priority decision | exact rank of the reduced band kernel | Distinct from the rejected 3+3 ranks; the proposed C=5 modular-rank experiment was never completed |
| open theory route | balanced-switch/coherent-configuration transform | The naive Johnson version is inadequate; no compact algebra or fast exact transform is known |
| open high-upside route | cross-class symbolic prefix/global contraction | Could remove class-local frontiers; no bounded exact implementation yet |

Several implementation variants are retired rather than separate mathematical
routes: fixed future-twin row orders, recursive row-adaptive memoization,
row/color-dual adaptation, eager raw color-orbit expansion, and larger
canonical caches or batches all lost bounded comparisons.  Their failures do
not invalidate the retained pair-tail and half-kernel components.

## Immediate objective

The box-order joint-histogram route has passed its complete C=2..4 gate with a
141-state C=4 midpoint.  The next global decision experiment is a bounded C=5
extension: enlarge the joint key and color canonicalizer, retain all C=2..4
differential gates, then report C=5 state counts, raw-target counts, transition
support, time, and memory one layer at a time.  A full N(5) run is justified
only if those bounded layers remain controlled.  If this route fails its C=5
scale gate, the operator-valued double-permanent subset DP remains the next
fallback.  Neither route authorizes a C=6 run before a complete C=5 exact gate.

Reverse gluing stays open as a separate historical-engineering question: find
a bulk four-row generator/lookup with delayed external reduction and a
canonical key cheaper than a fresh 46,080-image scan.  Pettersen's “more than
900 million” lookup statement is qualitatively consistent with the new
frontier data but still lacks a locally reproducible convention or dataset.

In parallel, the per-class track should remain bounded: probe later-class
coverage against the G1/G2 table and measure repetition of kernel-key pairs and
`(half key, relative row, relative color)` transforms.  The final layer must
also be streamed or shared across outer graphs; a 908-MiB leaf table does not
remove a 9.08-GB per-class frontier.  The present evidence does not justify a
full-run projection.

No full 63,199-class C=6 run is authorized.

## Current checkpoint

The active checkpoint is documented in
`data/checkpoints/MANIFEST.md`. It contains 5,315,962 exact memo states and is
not stored in Git. Use `checkpointreadonly` for verification so a read-only
gate cannot rewrite it.

## Active implementation tracks

- `factorization_orbit`: primary C=2..6 exact factorization/orbit route.
- `multiset_q`: independent transfer-kernel research and cross-check route.
- `multiset_fast` / `multiset_c6`: older independent exact validators.
- `main.cpp`: completed FJ05 9x9 reproduction.

See `docs/index.md` for the evidence and provenance map.
