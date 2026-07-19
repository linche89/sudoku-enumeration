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
still needs a 9.08 GB frontier and hundreds of millions of tail states, and a
G2 tail still takes 13.7 minutes.

A deterministic read-only coverage probe has now tested the same table on
later C=6 classes.  One-thousand-state samples ranged from complete coverage
at class 3 to zero at class 31,600.  In ten consecutive classes
10,001--10,010, only 5,234/30,000 half-key occurrences hit; the 4,962 selected
kernel pairs and all 5,000 relative-transform signatures had zero cross-class
reuse.  The fixed G1/G2 table therefore does not generalize into a broad
later-class table or batching solution.  No full C=6 count is currently in
progress.

A second read-only audit tested the complementary class-local `2+4` view:
enumerate a two-factor `H` and look up `F4(Q-H)` in the existing graph memo.
The identity is exact but is only a regrouping of two rooted matching levels
already present in the primary engine.  Measured ordinary classes contain
about 1.33--1.39 billion uncolored spanning two-factors, not a small lookup
inventory.  In distributed later classes the G1/G2 checkpoint covers only
about 2--4% of sampled F4 keys; 20-parent checks are more than 99.8% unique.
Across classes 3--12, 516,100 per-class unique keys reduce to a union of
513,228, only 0.56%.  Thus this class-local F4 lookup does not remove the
dominant enumeration either.

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
shared half-kernel table provides exact reuse between G1 and G2 at the
leaf-kernel level, but it leaves each outer graph's prefix and kernel-pair
contractions separate.  Later-class probes show that this reuse is not broad:
coverage can fall to zero, and a ten-class block had no repeated selected
kernel pair or complete relative-transform signature across classes.

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
compression alone does not pass the C=5 gate and is not yet a C=6 route.

The proposed operator-valued double-permanent subset DP has now been
implemented as a separate Windows decision prototype.  Its direct and subset
transitions agree per source and per labelled raw target through complete
C=2..4.  At the C=5 third-symbol transition it has 857,244 legal assignments,
857,244 raw targets, and zero merged subset prefixes.  Three deterministic
100-source samples of the next transition again have zero prefix merging;
1,434,260 canonical targets remain from 1,442,763 raw targets, or 99.410645%.
This is structural: comparing a partial target's degrees with its fixed source
recovers the chosen color pair in every processed band.  The naive subset DP
therefore changes enumeration order without compressing the permutation
prefixes and fails its C=5 decision gate.

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

The bounded C=5 extension now closes the first layer with seven states, 6,210
contingency leaves, and exact layer total 52,254,720,000.  An occupied-anchor
sparse canonicalizer agrees with complete group scans through C=4 and on
bounded C=5 samples.  Streaming raw reduction removes the resident-map wall:
a five-million-leaf layer-2 prefix used about 0.038 GiB observed RSS and
reduced 4,227,388 batched raw entries to 38,373 canonical targets.  One
complete selected source reduced 6,516,556 leaves to 20,318 targets in
189.839 seconds.

The complete scalar allocation inventory is much larger: all seven layer-1
sources have exactly 652,001,548 layer-2 contingency leaves.  A residual-
degree cost DP represents their leaf counts with only 49,890 memo states and
finishes in about 0.028 seconds, but it deliberately drops target identity and
cycle weight.  Its exact target- and cycle-aware lift agrees per source and
per raw target with both independent kernels through C=4.  At C=5, however,
six of seven sources exceed a ten-million-state limit before closing.  The
remaining source has 6,323,400 terminal raw states from 6,516,556 leaves, or
97.0359% raw uniqueness.  The labelled operator therefore fails its
predeclared near-injective scale gate; the color-orbit quotient is useful only
after the frontier that this lift was meant to avoid.

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

The obvious top-down version of that lookup has now failed its own bounded
scale gate.  Exact two-factor counts are 1,326,562,875 for G1,
1,329,588,799 for G2, and roughly 1.33--1.39 billion in sampled ordinary
classes.  The existing 5,315,962-entry graph memo self-covers G1, partially
covers its saved G2 neighborhood, but hits only about 2--4% of F4 residual
keys later in the class list.  A ten-class block gains only 2,872 keys by
cross-class union out of a per-class unique-key sum of 516,100.  Therefore
class-local `2+4` plus a shared F4 memo is not the missing historical bulk
algorithm.  A surviving bulk design must specify a genuinely global incidence
construction that avoids both this top-down work and the 1.761-billion
bottom-up low-stabilizer placements.

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
The deterministic later-class table-coverage probe, cross-class unions, and
negative batching decision are recorded in
`docs/reports/og2/future-tail-later-class-coverage-20260719.md`.
The reverse-gluing derivation, exact C=4/C=5 gates, interval-file evidence,
and C=6 decision boundary are recorded in `docs/methods/reverse-gluing.md` and
`docs/reports/og2/reverse-gluing-c4-c5-20260718.md`.  The exact C=6 two-row
inventory and bounded four-row samples are recorded in
`docs/reports/og2/reverse-gluing-c6-frontier-probe-20260718.md`.
The exact two-level differential, C=6 two-factor counts, and F4 reuse decision
are recorded in
`docs/reports/og2/f4-lookup-coverage-20260719.md`.
The exact joint-histogram recurrence, orbit normalization, C=2..4 differential
gates, bounded C=5 frontier, and current boundary are recorded in
`docs/methods/joint-histogram.md`,
`docs/reports/og2/joint-histogram-c2-c4-20260719.md`, and
`docs/reports/og2/joint-histogram-c5-frontier-20260719.md`.  The exact
target-labelled operator and its negative C=5 scale gate are recorded in
`docs/reports/og2/joint-histogram-operator-frontier-20260719.md`.
The exact connectivity transition, subset-prefix injectivity result, complete
C=2..4 differential gates, and bounded C=5 measurements are recorded in
`docs/methods/connectivity-operator.md` and
`docs/reports/og2/connectivity-double-permanent-20260719.md`.

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
| proven component only | external pair-tail and half-kernel table | Closes G1/G2 and speeds the G2 tail 4.41x, but leaves every prefix separate; later-class coverage can fall to zero |
| proven component only | connectivity/path quotient | Exact through C=4; already 83,776 states at C=5 symbol 3, and the naive subset operator supplies no prefix merging |
| proven component only | streaming/checkpoint infrastructure | Controls RAM and restart risk, not total arithmetic |
| exact through C=5; naive C=6 join fails scale gate | reverse 4+2 row-block gluing | Exact C=6 inventory is 772 two-row orbits / 298,378 pairs; trivial stabilizers force at least 1.761B double cosets, and a generic 100k-leaf prefix is 99.4% distinct after anchored canonicalization; only a bulk/external redesign remains open |
| rejected implementation | class-local `2+4` with shared F4 memo | Ordinary classes have about 1.33--1.39B two-factors; later checkpoint coverage is about 2--4%, 20-parent samples are over 99.8% unique, and a ten-class key union saves only 0.56% |
| exact through C=4; bounded C=5 layer-2 decision | box-order joint-histogram pair DP | C=5 layer 1 has 7 states and canonical targets compress strongly, but both the 652M-leaf kernel and the labelled residual-operator lift fail scale |
| rejected implementation | target-labelled residual operator | Exact per raw target through C=4; six C=5 sources exceed 10M states and the completed source is 97.0359% unique |
| rejected implementation | naive symbol-synchronous double-permanent subset DP | Exact per raw target through C=4; its labelled partial state uniquely determines every assignment prefix, with zero merges in bounded C=5 probes |
| rejected as broad reuse | fixed G1/G2 table coverage and naive kernel-pair batching | In classes 10,001--10,010 only 5,234/30,000 half occurrences hit, with zero cross-class reuse among 4,962 selected pairs and 5,000 full signatures |
| open, next bounded decision | exact rank of the reduced band kernel | Distinct from the rejected 3+3 ranks; start with exact C=3/C=4 matrices before deciding whether a C=5 modular-rank extension is justified |
| open theory route | balanced-switch/coherent-configuration transform | The naive Johnson version is inadequate; no compact algebra or fast exact transform is known |
| open high-upside route | cross-class symbolic prefix/global contraction | Could remove class-local frontiers; no bounded exact implementation yet |

Several implementation variants are retired rather than separate mathematical
routes: fixed future-twin row orders, recursive row-adaptive memoization,
row/color-dual adaptation, eager raw color-orbit expansion, and larger
canonical caches or batches all lost bounded comparisons.  Their failures do
not invalidate the retained pair-tail and half-kernel components.

## Immediate objective

The later-class half-kernel and class-local F4-lookup experiments are complete
and negative for their tested mechanisms.  The first finds essentially no
reusable kernel pairs or full relative transforms across a ten-class block.
The second finds roughly 1.3 billion two-factors per ordinary outer graph,
about 2--4% later-class checkpoint coverage, and more than 99.8% unique F4
keys in larger parent samples.  Extending either fixed table or naively
batching either incidence list is not the missing global contraction.

There is still no implemented global C=6 route that has passed its scale
gate.  The mathematical box interfaces are known, but the affordable
implementation of every pass is not.  A contraction-order planner can choose
among verified edges; it cannot manufacture a missing sufficient quotient or
turn a nearly injective edge into a compressive DP.

Historical bulk reverse gluing remains a theory lead, not a qualified large
run.  A new construction must avoid both the known 1.761-billion bottom-up
low-stabilizer placements and the roughly 1.3-billion top-down two-factor list
per ordinary outer class.  Pettersen's “more than 900 million” lookup statement
is a rough comparison point, not authorization or an acceptance oracle.

The next executable decision gate is the exact rank of the distinct reduced
band kernel from `docs/math/og2-band-kernel-lowrank.md`:

1. explicitly construct every reduced transition matrix at C=2--4 and
   differentially verify its entries and endpoint totals;
2. compute exact rational ranks at C=2--4, with independent modular checks;
3. continue to a bounded C=5 modular-rank experiment only if the small-C rank
   profile leaves meaningful compression possible.

This is a falsification experiment, not yet a C=6 algorithm.  Full or nearly
full small-C rank closes the proposed ordinary low-rank factorization; a small
rank would justify the C=5 extension and an implicit factor search.  The
cross-class symbolic-prefix/global-contraction and balanced-switch algebra
remain higher-upside theory alternatives without implementations.

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
