# C=6 Current Mathematical Bottleneck

2026-09-07 closure: the complete N(6) has now been computed by the shared-F4,
reverse-F5 and final global-contraction route, with two identical 63,199-class
CSVs, independent semantic checks and exact sums. See
`../reports/og2/c6-finalization-20260907.md`. The bottleneck discussion below
preserves the earlier method-development record; it is not remaining C6 work.
Current higher-C structural results and still-open fast scalar contraction
questions are in `higher-c-structure.md`.

## What is solved

- The outer family has exactly 63,199 skeleton orbits.
- Rooted color normalization reduces perfect-matching work exactly.
- The degree-4 rooted split removes the cubic canonicalization layer.
- The first complete outer graph has
  `F6(G1) = 6986348258918400`.
- The second complete outer graph has
  `F6(G2) = 7053808087203840`, independently reproduced by two exact tail
  paths.

## What remains

A low-symmetry outer graph produces about 20,000 nonisomorphic degree-5
residuals even after the degree-6 root pivot.  The optional pair-tail backend
now avoids that residual list and closes G2 exactly, but its own final frontier
contains 221,438,460 states and occupies 9.08 GB.  A color-symmetry half-kernel
rescan evaluates that layer in 2.237 GiB, so tail RAM is no longer the immediate
single-class obstruction.  A subsequent immutable half-kernel table lowers
the full G2 tail to 824.440 seconds at 0.956 GiB.  The bottleneck has moved to
the still-large per-class frontier and contraction and, more importantly, to
how 63,199 outer classes can share prefix work or be contracted globally.

There is an exact global formulation

```text
N(6) = [Omega_x Omega_y] Phi(x,y)^12,
```

and a 3,969-product-term Ryser representation of Phi. This eliminates the
63,199-loop algebraically, but no fast coefficient extraction is established.
Direct midpoint transfer and ordinary Burnside/character changes of basis have
state spaces larger than the orbit list.

## Audited global square routes

The 132-state histogram recurrence supplied on 2026-07-15 contracts the
one-copy linear quantity `sum w(G)F_C(G)`.  It does not contract the required
two-copy square.  Exact local runs give 96 at C=2 and 460800 at C=3, whereas
the Sudoku totals are 288 and 28200960.  The one-copy skeleton marginalization
is therefore not a route to `N(6)`.

A corrected symbol-synchronous recurrence supplied on 2026-07-17 does retain
the two-copy coupling.  Each labelled symbol inserts one edge in each of C
degree-two bipartite band graphs.  An exact continuation state keeps vertex
degrees and pairs the endpoints of every live path; a closed cycle is removed
and multiplies the coefficient by two.  This proves that the full joint
histogram `h_(S,T)` is not intrinsically necessary.

The implementation was rebuilt on Windows and reproduced:

```text
C=2 states = 1, 1, 3, 1, 1
N(2) = 288

C=3 states = 1, 1, 8, 18, 19, 1, 1
N(3) = 28200960

C=4 states = 1, 1, 28, 700, 12856, 9708, 155, 1, 1
N(4) = 29136487207403520

C=5 partial states = 1, 1, 93, 83776
```

Stack-swap invariance, sampled group-canonical invariance, and the C=5
two-symbol orbit initializer were checked independently.  However, the 83,776
states occur after only three of ten C=5 symbols, and the next direct step can
test 1,206,374,400 permutation pairs.  The quotient is exact but is not a
sufficient global compression by itself.

A reverse row-block contraction has now passed a stronger small-C gate.  A
partial configuration retains the complete multiset of symbol occurrence
masks, so configuration identity survives until the final weighted square.
For orbit representatives `x,y,q`, direct group summation has the exact
normalization

```text
F(q) contribution = Stab(q)/(Stab(x)Stab(y))
                    * sum_(g in C2 wr S_C) Join(x,g y -> q).
```

The implementation cancels repeated right-stabilizer images, quotients the
remaining images by `Stab(x)`, and uses the 2+2 block-exchange symmetry.  It
reproduces every C=2..4 class and, at C=5, constructs 17,120 four-row orbits,
all 355 complete classes, and the exact known `N(5)`.  All 355
`(coordinate orbit, labelled multiplicity, F)` triples agree with the primary
factorization engine.

This verifies the contraction, not its C=6 scale.  C=5 already has 165,744
labelled-coordinate two-row configurations in 107 orbits; 40 is only the
number of masks available to one symbol.  Its four-row coordinate-orbit mass
is 62,185,328, and the exact interval computation accumulated 122,166,792
contingency leaves plus 95,227,539 cold canonicalizations.  Thus the next
question is a bounded C=6 frontier measurement, especially for low-stabilizer
pairs, not another normalization derivation.

That bounded measurement is now complete.  A cycle/matching orbit generator,
fully differential against the old labelled generator at C=3..5, gives the
exact C=6 two-row inventory:

```text
labelled-coordinate configurations = 20338525
coordinate orbits                  = 772
unordered orbit pairs              = 298378
```

An independent raw DFS confirmed the labelled count.  First/middle/last pair
samples had 10, 1,448, and 43 double-coset placements.  One exact placement
from each produced 2,662, 15,360, and 27,793 contingency leaves.  The middle
sample reduced 15,360 leaves to 15,168 raw keys; its first 2,000 raw keys also
remained 2,000 distinct canonical four-row outputs.  Generic cold
canonicalization scans 46,080 images and sustained only hundreds of outputs
per second.

The exact stabilizer histogram contains 276 trivial-stabilizer orbits.  Their
38,226 unordered pairs alone require 1,761,454,080 double-coset placements.
An exact first-mask anchor improves delayed key-only canonicalization to about
8,195 keys/s and passed 100 full-group key checks, but a 100,000-leaf generic
prefix still had 99,423 distinct canonical keys.  The problem is therefore
not only the avoidable stabilizer scan; the pair/record inventory itself is
large.

Thus direct orbit-pair group summation plus immediate canonicalization fails
the C=6 scale gate.  Reverse gluing is not mathematically rejected, but it now
requires a historical-style bulk lookup/external-reduction design rather than
the proposed cheap pairwise implementation.

The complementary top-down `2+4` lookup interpretation has now also been
measured.  It is exact, but class-locally it is just two successive levels of
the primary rooted matching recurrence, with the two ordered matchings grouped
as a two-factor `H`.  Exact ternary-DP counts give about 1.33--1.39 billion
uncolored spanning two-factors for measured ordinary C=6 classes; the final,
highly symmetric class has 4.617 billion.  Rooted aggregation reduces the
first matching layer to as many as about 32,000 canonical degree-five parents,
but ten sampled generic parents still produce roughly 41,000--52,000 unique
degree-four residual keys.

The existing 5,315,962-entry G1/G2 graph memo covers only about 2--4% of those
keys in distributed later classes.  Twenty-parent checks at classes 10,001
and 31,600 are more than 99.8% unique within the sample.  Across classes 3--12,
516,100 per-class unique keys reduce to a union of 513,228, only a 0.56% gain.
Therefore class-local two-factor enumeration plus a shared F4 memo also fails
the scale gate.  This leaves a historical bulk route open only if a new global
incidence construction avoids both the 1.761-billion bottom-up placement
lower bound and the roughly 1.3-billion top-down two-factor list per ordinary
outer graph.

A box-order joint-histogram route now gives a second exact global square
formulation.  It closes C=2..4 with orbit layers `1,2,1`, `1,3,3,1`, and
`1,5,141,5,1`.  Its bounded C=5 first layer has seven states.  The complete
next-pass allocation inventory has 652,001,548 contingency leaves, while a
scalar residual-degree DP counts them using only 49,890 memo states.  A
five-million-leaf prefix reduced to 38,373 canonical targets, and one complete
6,516,556-leaf source reduced to 20,318 targets.  This keeps the boundary
representation open but rejects the naive leaf-by-leaf transition as a C=6
kernel.  The scalar memo omits target identity and cycle weight; its
target- and cycle-aware lift is exact through C=4 but also fails its C=5 scale
gate.  Six of seven C=5 source transitions exceed ten million lifted states;
the remaining source reaches 6,323,400 raw states from 6,516,556 leaves.

A related backup proposal applies each symbol-synchronous transition as an
operator-valued double permanent with a two-subset internal DP.  That proposal
has now been implemented and differentially checked per raw target through
C=4.  It is exact but has no internal compression.  For a fixed source, the
partial target degree increments in each processed band identify that band's
chosen left and right colors, so the partial target recovers the complete
assignment prefix.  At the C=5 third symbol there are 857,244 legal
assignments, 857,244 raw targets, and zero prefix merges.  Three deterministic
100-source samples of the fourth-symbol transition again have zero merges;
1,434,260 canonical targets remain from 1,442,763 raw targets.  Thus both
straightforward operator lifts reach a near-injective labelled frontier before
the useful full-group quotient.

## Audited future-twin route

There is a verified exact backend that attacks the single-graph wall directly. Let
`P` be the processed left vertices.  For every right column `j`, retain

```text
tau_P(j) = N(j) - P
K_j      = colors already used at j.
```

The state is the multiplicity table `n_P(tau,K)`, modulo the global color
group.  Columns with the same future neighborhood and unavailable colors have
bijections between all future completions, even if their processed
neighborhoods differed.  When the next left vertex is processed, each affected
group of multiplicity `m_g` receives a disjoint color subset of size `m_g`;
the exact labelled transition weight is `product_g m_g!`.

This recurrence does not enumerate perfect matchings or residual graphs. Its
normalization and grouped transitions pass brute-force differential tests;
the implementation agrees per outer class with the primary engine for complete
C=2..5 and independently reproduces the cold G1 value in 1592.867 seconds at
1.670 GiB peak RSS.

Paired processing, a seven-row exact tail, and external sort/reduce have now
closed the actual second graph:

```text
F6(G2) = 7053808087203840
prefix states = 1, 1, 352, 77956, 9664963, 221438460
```

The final external layer was reduced from 607,148,632 raw records, has exact
mass 2,009,438,804,160, and occupies 9,078,976,900 bytes.  Immutable parent and
tail checkpoints were resumed after bounded process stops, and a separate
`checkpointreadonly` reopening returned the same result.  Thus the G2 central
memory frontier is no longer mathematically open; its size is now exact
evidence against naive independent repetition over all outer classes.

## Audited half-kernel sharing result

The seven-row separator has an additional exact `D8 x S6` action on each half
kernel.  A sufficient complete state retains two color-canonical half keys and
their relative row and color transforms.  Reconstruction tests passed, and a
complete forced G2 tail rescan reproduced the committed value:

```text
tail time = 3634.499 s
peak working set = 2.237 GiB
kernel hits = 347136486 / 442876920
local assignments = 391493454865
```

The sharing is sharply localized.  In a 100,000-key G2 sample, raw half keys
collapsed to 46,598 color-canonical keys, with a Chao1 estimate of 69,949.
G1/G2 two-million samples also shared 628,495 raw half keys.  In contrast, all
100,000 complete color-tail signatures in a uniform G2 sample were distinct.
Therefore a persistent cross-class half-kernel table is supported by data;
whole-tail scalar memoization is not.

The complete inventories sharpen that sample result.  G1 has 43,722 unique
color-canonical half keys and G2 has 108,525.  They share 38,841 keys, covering
98.75% of G1 half occurrences and 62.36% of G2 half occurrences.  Their union
has 113,406 keys and 118,697,748 sparse records, so a flat exact table occupies
952,303,792 bytes including its index.

That table has now passed complete forced rescans:

```text
G2 tail time = 824.440 s
G2 peak working set = 0.956 GiB
G2 kernel hits = 442876920 / 442876920
G2 local assignments = 0
G2 kernel evictions = 0

G1 tail time = 219.709 s
G1 kernel hits = 112923252 / 112923252
G1 local assignments = 0
G1 kernel evictions = 0
```

Both committed factorization counts were reproduced.  This resolves the
private kernel-generation and eviction question for G1/G2, but not the exact
inner product for each complete signature: G2 still performs 1,328,630,760
color refinements and 442,876,920 kernel contractions.

The later-class coverage decision is now complete.  A deterministic read-only
probe passed C=4 table self-coverage and independently reproduced the complete
2,295-state C=5 class-300 boundary.  At C=6, however, 1,000-state samples
ranged from full G1/G2-table coverage at class 3 to zero at class 31,600.  In
classes 10,001--10,010, 5,000 sampled states produced:

```text
half hits                         = 5234 / 30000
per-class unique half-key sum     = 24621
cross-class half-key union        = 23452
per-class/cross-class pair count  = 4962 / 4962
per-class/cross-class signatures  = 5000 / 5000
```

Thus the fixed G1/G2 table does not give broad later-class coverage, and
neither selected kernel pairs nor full relative-transform signatures supplied
cross-class batching in this block.  This rejects that particular reuse
mechanism, not the half-kernel quotient itself.  A larger dynamic table would
mostly acquire new later-class keys and would still leave the class-local
prefix untouched.

## Audited 3+3 result

For `D = Sym^3(W)`, where `W` is the 462-dimensional permutation module on
unordered balanced 6|6 cuts, the global contraction has the exact endpoint

```text
N(6) = 64 * sum_lambda d_lambda * tr(M_lambda M_lambda^T).
```

The C=6 block data have 67 nonzero irreducible types, maximum multiplicity 344,
and 544,034 symmetric block entries.  The final norm would be cheap if the
blocks were known, but constructing them remains the hard part.

The supplied exact programs were rebuilt and reproduced locally:

- C=4: exact `N(4)` and rank 630/630 modulo 1,000,003;
- C=5: exact `N(5)` and centralizer rank 154/154, implying cross-channel
  row rank 8001/8001;
- C=6: midpoint dimension 5,171,942,314, with every domain multiplicity
  accommodated by the corresponding midpoint multiplicity.

Thus the hoped-for small natural channel support is absent at C=4 and C=5,
and no representation-dimension loss is forced at C=6.  This is a decision
against ordinary low-rank channel truncation, not an impossibility theorem for
all implicit full-rank contractions.

## Reduced band-kernel rank result

The reduced paired band kernel is a different operator from the 3+3 channel
maps.  Its complete stack-swap matrices have now been materialized through
C=4.  C=3 has dimensions `1,3,3,1`; its square middle matrix has nine nonzero
entries, determinant 2,048,000, and exact rank 3.  C=4 has dimensions
`1,5,141,5,1`; its layer ranks are the maximum possible `1,5,5,1`, with the
small values forced by the adjacent five-dimensional grades.

The first unbottlenecked map is the C=5 `38801 x 38801` middle layer.  A
deterministic selection of 1,024 complete source rows followed by a signed
target CountSketch gives a `1024 x 1024` matrix of full rank modulo both
1,000,000,007 and 1,000,000,009.  Because the sketch is an integer left/right
compression of the true matrix, either nonzero maximal minor proves

```text
rank_Q(Kbar_2 at C=5) >= 1024.
```

All 2,084,272,587 canonical task-target entries belonged to the exact
38,801-state complement basis.  This rejects the proposed few-hundred-channel
factorization, including a factorization through the 126-dimensional C=5
matching-cycle kernel.  It does not prove full rank or exclude a rank of 1,024
or several thousand, so it is a bounded negative decision rather than an
impossibility theorem for every structured factorization.

## Fixed-source target-frontier lower bound

The proposed fused column frontier can be decided without constructing a
larger C=5 layer.  Fix a fully labelled grade-`k` source
`s=((S_i,T_i))`.  Let `M(S)` be the balanced maps from the `2C` symbols to the
`C` columns that avoid the used masks `S_i`, with two symbols mapped to each
column; define `M(T)` analogously.  For this fixed source, the labelled target
recovers its two new maps by set difference.  Every map pair has a positive
cycle weight, so the terminal support is exactly

```text
T(s) = |M(S)| |M(T)|,
|M(S)| = permanent(B_S) / 2^C.
```

At any fixed processed-column cut, different extendable assignment prefixes
have disjoint nonzero completion-and-target supports.  The corresponding
prefix/suffix flattening rows are therefore linearly independent.  After
source-stabilizer reduction, the frontier width is at least the number of
stabilizer orbits of extendable prefixes and the terminal width is at least
`ceil(T(s)/|H_s|)`.

An independently verified reachable C=6 grade-2 source has trivial
stabilizer and gives

```text
|M(S)| = 74119
|M(T)| = 74064
terminal support = 5489549616
minimum support over every 3-column split = 6488 and 6503
minimum fixed 3+3 flattening rank = 6488 * 6503 = 42191464
```

At 16 bytes per terminal record, its fixed-source output alone would occupy
81.801 GiB before table overhead.  This is an exact obstruction to every
fixed-source linear frontier that distinguishes the full target, including a
signed Ryser/Glynn reordering with that output semantics.  It does **not**
lower-bound a target-only circuit that sums the actual multi-source input
vector directly into target-orbit coefficients before any `(source,target)`
pair is represented.

The related full coherent-configuration proposal has a separate exact wall.
The 276 trivial-stabilizer two-row orbits give 38,226 unordered regular-block
pairs and hence at least

```text
38226 * 46080 = 1761454080
```

relative-placement coordinates.  Fourier decomposition changes basis but
does not shrink this full equivariant Hom space.  Only a specifically
constructed smaller subspace containing the downstream weighted response
remains open.

## Required next result

No implemented global C=6 route currently passes its scale gate.  The
fixed-source E3 frontier is no longer the next experiment: its C=6 witness
already proves that increasing state limits, changing assignment order, using
signed inclusion-exclusion, or externalizing the same target polynomial
cannot make it small.  Materializing the complete coherent-configuration
algebra merely restates the known 1.761-billion regular-block work factor.

The primary remaining question is target-only and multi-source.  With the
orbit-total normalization used by the verified joint-histogram engine, find an
exact circuit for

```text
y_[t] = sum_[s] x_[s] Kbar_k([t],[s])
```

on the actual reachable input vector `x`.  It must aggregate different sources
before materializing a source-target pair or a complete source row.  It may be
full rank: the desired gain is a fast transform or structured application,
not necessarily a low-rank factorization.  The construction must specify its
sufficient representation, exact normalization, closure across successive
bands, and operation/memory bounds.

The parallel reverse-gluing question is whether the specific downstream
weighted responses of many regular relative-placement blocks lie in a common
small Fourier/communication subspace.  This asks for a response subspace, not
the full orbital algebra.  A useful answer must construct that subspace and
bound its dimension, or prove a lower bound that also applies after the actual
source weights are combined.

Either proposal must reproduce complete C=3/C=4 output vectors or matrices
under the retained normalization, then pass a bounded C=5 actual-vector gate
that reports representation dimension, operations, time, and RSS.  Only an
exact construction with a credible C=6 projection justifies another bounded
C=6 implementation.  A lower bound eliminating these two remaining output
contracts is equally useful.  G1/G2 symmetry, cache tuning, a contraction-order
planner, and Pettersen's rough “more than 900 million” lookup statement are not
acceptance evidence.

There is therefore no authorized large-data step at present.  Historical bulk
reverse gluing remains a theory lead, and no full 63,199-class run is
authorized.

Raw source material is preserved under `../expert/2026-07-12/`,
`../expert/2026-07-13/`, `../expert/2026-07-14/`,
`../expert/2026-07-15/`, `../expert/2026-07-17/`, and
`../expert/2026-07-19/`.  The narrowed follow-up question is under
`../expert/2026-07-20/`.  Reproduction evidence is in
`../reports/og2/c6-expert-routes-audit-20260713.md` and
`../reports/og2/future-pair-tail-external-c6-20260714.md`.  Color-symmetry
implementation and full-rescan evidence are in
`../reports/og2/future-tail-color-symmetry-c6-20260715.md`.  Complete
inventory, overlap, shared-table, and forced G1/G2 evidence are in
`../reports/og2/future-tail-kernel-table-c6-20260715.md`.  The deterministic
later-class coverage and batching decision is in
`../reports/og2/future-tail-later-class-coverage-20260719.md`.  The linear-sum
scope correction, symbol-connectivity audit, and current route portfolio are in
`../reports/og2/c6-route-portfolio-20260717.md`.
The verified reverse-gluing formula and gates are in
`../methods/reverse-gluing.md` and
`../reports/og2/reverse-gluing-c4-c5-20260718.md`.  The exact C=6 two-row
inventory and bounded pair samples are in
`../reports/og2/reverse-gluing-c6-frontier-probe-20260718.md`.
The exact class-local two-level differential and F4 lookup-coverage decision
are in `../reports/og2/f4-lookup-coverage-20260719.md`.
The complete small-C reduced matrices and C=5 rank lower-bound certificate are
in `../reports/og2/band-kernel-rank-20260719.md`.
The box-order operator frontier is in
`../reports/og2/joint-histogram-operator-frontier-20260719.md`.
The connectivity subset-operator proof and measurements are in
`../methods/connectivity-operator.md` and
`../reports/og2/connectivity-double-permanent-20260719.md`.
The fixed-source support theorem, C=6 witness certificate, coherent-space
consequence, and expert-follow-up audit are in
`../reports/og2/source-target-frontier-lower-bound-20260720.md`.
