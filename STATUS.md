# Current Project Status

Last updated: 2026-09-06

This is the single authoritative status page. Dated reports preserve evidence;
historical handoffs and raw expert responses are not current project state.

## Current C=6 decision (2026-09-06)

An exact new route now reuses the completed L4/L5 support catalogues, shares
F4 graph values without merging native downstream responses, and computes
F5 by a rooted reverse recurrence. Its complete C5 and interruption/recovery
gates pass. The ninth bounded real C6 window completed the entire F4 namespace:
903,398,621 IDs, 903,398,603 live native states, 18 insertion holes and
140,069,579 closed graph-representative values. The independent FULL-domain
byte/backup/alias audit passed: every live alias resolves to a closed value,
with zero unresolved aliases. The owner has since completed the protected
native F4 export and the first numerical F5 session: 30,200,000 saved IDs
(31.3106%), with 30,199,969 live closed F5 values. No complete F5 catalogue
or independently closed N(6) exists yet.

Window8 was interrupted by overnight workstation sleep and ended under the
hard time bound after wake, at prefix 797,325,000 (controller exit 98).
It is not recorded as a normal successful computing window. The owner's
manual continuation, window9, added the remaining 106,073,621 IDs and
17,245,383 representative values. Its engine finished at 09:38 on 2026-09-06;
the external after-backup finished at 09:47. Engine/controller exit was zero
and engine stderr was empty. All 36,137 committed files, 10,850,034,524 bytes,
are physically backed up. No F4 computing/controller process remained at
that handoff; the owner has subsequently started F5 as described below.

On the owner's subsequent request, the independent reader redecoded every
record, checked both window9 physical backups, and rechecked all 18,104
files from the SHA-pinned window7 audit. It inherited no unaudited window8
counters. Full audit elapsed was 166.588572 seconds under a 180-second bound,
with worker/parent peaks 145,981,440 / 171,200,512 bytes, each capped at 1 GiB.
This certifies complete internal alias/value closure and storage integrity
at the verified reads, not independent numerical reevaluation of every F4.
See `docs/reports/og2/c6-shared-f4-complete-audit-20260906.md`.

The protected native F4 export and independent readback subsequently passed
on the owner's first manual F5 session. Export SHA-256 is
`7BA5E5BA3255DD17851043521F67FB4EE70F76AE565FD6CA9AD962E7D5014D94`;
the input/source and separate-volume output copy are bound by its receipt.
Do not rerun the completed F4 scan or substitute the old partial T4 image.

The owner subsequently requested a local same-command resumable F5 launcher.
`run_f5.ps1` is prepared and its read-only preflight, synthetic receipt/window
guards, and fresh full/direct/shared/reverse verification pass. It refreshes
gates, performs the protected F4 export if absent, then checks a 10,000-ID
resumed canary before a bounded 330/360-minute F5 window. Re-running the same
script resumes closed chunks. It does not automatically relaunch after a hard
bound or run the final N(6) stage. No C6 export or numerical F5 was launched
during this preparation. See
`docs/reports/og2/c6-f5-manual-resume-launcher-20260906.md`.

The first owner-run F5 session ended normally at the 330-minute soft limit,
not at complete F5 closure. Its 3,020 committed chunks plus manifest contain
242,373,376 bytes per physical copy; all 3,021 local/backup SHA-256 values
were independently compared with the after-backup receipt and matched.
This is storage/contiguous-prefix evidence, not independent numerical
reevaluation of every F5 value. The regular window added 30,190,000 IDs in
19,763.068581 computation seconds, with a 45,665,304,576-byte peak.

The owner started another ordinary session at 18:28 on 2026-09-06; it is
left running and its unfinished suffix is not included in the reviewed
prefix above. For the FOLLOWING session the owner requested a single longer
window to finish the expected remaining 37%--40%. The launcher now accepts
`-WorkMinutes 450 -MaxMinutes 480` (7.5-hour soft / 8-hour hard child limits),
stopping early on full F5 closure. Defaults stay 330/360; the current running
session is unchanged. Only two entry-parameter limits changed, not counting
arithmetic, the underlying controller, checkpoint formats or safety gates.
Synthetic parameter/receipt/terminal tests pass; no extra production process
was launched during this change. See
`docs/reports/og2/c6-f5-window1-and-long-window-20260906.md`.

The measurements support pursuing this route instead of finishing the old
forward S1 work, but do not certify a total runtime or a global speedup
factor. The old checkpoints are unchanged and remain available as fallback.
Long computation windows must fit the owner's available daily runtime;
the scripts require positive work limits, time/RSS bounds and backups.
Detailed evidence follows under "Direct-layer investigation" below.

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

The distinct reduced band-kernel rank gate is now also measured.  Complete
explicit matrices reproduce C=2--4, including dimensions `1,3,3,1` and exact
middle determinant 2,048,000 at C=3, plus dimensions `1,5,141,5,1` and ranks
`1,5,5,1` at C=4.  The first unbottlenecked C=5 middle map is
`38801 x 38801`.  A deterministic 1,024-row, signed target CountSketch is full
rank modulo both 1,000,000,007 and 1,000,000,009, proving exactly that its
rational rank is at least 1,024.  All 2,084,272,587 canonical task-target
entries belonged to the precomputed complement basis.  This rejects a
few-hundred-dimensional factorization, but it neither proves full rank nor
excludes a factorization of rank 1,024 or several thousand.

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

The proposed B2/E3 fused column frontier is now closed negatively in its
fixed-source, target-distinguishing form by an exact support theorem rather
than a larger sampling run.  For a fixed labelled source `s`, the output
support is exactly `|M(S)| |M(T)|`, where each map count is a `12 x 12`
permanent divided by `2^6`.  A reachable C=6 grade-2 source with trivial
stabilizer has `|M(S)|=74119` and `|M(T)|=74064`, hence 5,489,549,616 terminal
targets.  Every fixed `3+3` column split has flattening rank at least
`6488 * 6503 = 42,191,464`; a 16-byte terminal record list would already be
81.801 GiB.  The independently retained C++ verifier checks reachability,
stabilizer, both permanent formulas, raw map counts, and all 20 split
supports.  This rejects fixed-source target-aware E3 and signed Ryser/Glynn
reorderings with the same full-target output semantics.  It does not reject a
target-only transform that sums different sources before representing any
`(source,target)` pair.

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
The complete C=2--4 reduced matrices and the exact C=5 rank lower-bound
certificate are recorded in
`docs/reports/og2/band-kernel-rank-20260719.md`.
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
The fixed-source support and flattening theorem, reachable C=6 witness,
independent verifier, and expert-follow-up audit are recorded in
`docs/reports/og2/source-target-frontier-lower-bound-20260720.md`.

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
| bounded negative for tiny-rank B1 | reduced band-kernel rank | Exact C=2--4 matrices; the `38801 x 38801` C=5 middle map has certified rational rank at least 1,024, rejecting a few-hundred-channel factorization but not proving full rank |
| rejected implementation | fixed-source target-distinguishing column frontier | A reachable trivial-stabilizer C=6 source forces every fixed `3+3` split to width at least 42,191,464 and has 5,489,549,616 terminal targets; signed permanent reorderings with the same output semantics do not compress it |
| rejected as primary | one-copy-orbit PSD/Kraus factorization | The identity is exact, but the factor is the already full C=4/C=5 history-to-boundary map; invariant matrices live in the orbital centralizer, not the one-point orbit basis |
| rejected implementation | full coherent-configuration/orbital materialization | The 38,226 pairs of trivial-stabilizer two-row orbits alone expose 1,761,454,080 relative-placement coordinates; Fourier blocks change basis but do not remove them |
| qualified implementation candidate | global row-incremental layer DP | Aggregates all histories at each canonical row layer; exact through every C=5 class, bounded C=6 scale probes complete, and checkpoint/restart passes repeated kill/corruption gates; full C=6 remains unrun |
| closed as primary compression target | target-only cross-source band transform | Completion-operator rank defect is exactly 2 at C=4 and C=5, so the measured target-side reduction is only O(1); global aggregation is instead supplied by the layer DP |
| open optional research, not a production prerequisite | common downstream response subspace/global incidence contraction | A coarser response span is not ruled out, but none is constructed; the retained layer DP no longer depends on finding one |

Several implementation variants are retired rather than separate mathematical
routes: fixed future-twin row orders, recursive row-adaptive memoization,
row/color-dual adaptation, eager raw color-orbit expansion, and larger
canonical caches or batches all lost bounded comparisons.  Their failures do
not invalidate the retained pair-tail and half-kernel components.

## Immediate objective

Updated 2026-08-02.  The global row-incremental layer DP
(`experiments/proto/layer_dp_gate.cpp`) is the qualified implementation
candidate.  It reproduces complete C=2..5 exactly, including all 355 C=5
triples, and now has explicit canonical invariance, full-group
orbit/stabilizer, separation, and histogram differentials.  Its bounded C=6
evidence includes exact `M_3 = 12,324,872` (with
772 / 20,338,525 / 2,605,194,602 reproduced in-run), a 58,400-parent random
3->4 sample projecting 2.13308e12 emissions, and a near-saturated hash-window
estimate `M_4 = 902.9e6`.  An independent exact Burnside calculation now gives
`M_5 = 96,452,755`.  These results qualify a production candidate; they are
not a completed C=6 layer or count.  Mathematical provenance remains in
`docs/expert/2026-07-26/layer-dp-permanent-profile.md`.

The two prior theory targets of this section are resolved by measurement:
the target-only reduced-kernel question has answer "compression exists but
is O(1)" (completion-operator rank defect exactly 2 at C=4 and C=5, created
entirely at the 3->4 boundary; certified null vectors in
`docs/expert/2026-07-26/c5_wvecs.txt`), so no target-side contraction can
change the C=6 scale; the bulk-organization question is answered by the
layer DP itself.

The `(C-1)`-row anchor and checkpoint/restart portion of the preflight is now
implemented and retained.  Checkpoint format v2 binds images to
key-affecting modes, verifies exact length and hashes, preserves two atomic
generations, promotes completed images to hard-linked snapshots, fails closed
on I/O errors, and has passed 20 random kill/resume cycles plus forced-wide,
mode-mismatch, stale-base, corrupt-generation, and snapshot-round-trip gates
at C=5.  Atomic replacement now retries only bounded Windows sharing/access
faults for at most five seconds; a deterministic locked-generation gate
exercises that path before verifying the exact resumed output.  The G1/G2
native-key bridge is wired with stabilizers 120 and 8.
Evidence and safe commands are in `docs/methods/layer-dp.md` and
`docs/reports/og2/layer-dp-checkpoint-gate-20260730.md`.

The resource preflight is now executable and fail-closed.  Resume reserves
the final child capacity before reading and moves the payload in place,
removing the previous 45.262-GiB duplicate-image peak at the provisional
1.35-billion layer-4 cap.  With provisional capacities
`2000,14000000,1350000000,250000000,100000`, the three large transitions
require 69.898 / 79.685 / 18.465 GiB RAM including margin and
152.726 / 132.609 / 124.242 GiB disk including retained generations,
A/B, the atomic-write temporary, and margin.  Large C=6 work is restricted
to one loaded/resumed transition per process.  These are capacity-worst-case
plans rather than measured production allocation.  The layer-4 cap is
supported by the completed random M4 measurement.  The 250-million layer-5
cap is 2.59194 times the exact `M_5`, leaving 153,547,245 entries for parallel
insertion holes and operational margin; see
`docs/reports/og2/layer-dp-penultimate-burnside-20260801.md` and
`docs/reports/og2/layer-dp-resource-preflight-20260731.md`.

Bounded S0 has now produced a reusable exact C=6 layer-3 snapshot:
12,324,872 closed states, one parallel-insertion hole,
orbit mass 566,455,903,200, and SHA-256
`1D882DB7B0B18490E981539F7C690FA90227ECEC243817F5B538BEC8DAC865B7`.
The guarded run took 521.3 seconds and peaked at 661.1 MiB; a read-only
round trip reproduced the state count and mass.  A canonical-key-hash parent
sampler and hash-window-uniform 4->5 fan/canonicalization calibration now pass
the C=5 gate.  The complete C=6 random probe then independently reproduced the
older strided M4 window within 41 observed keys out of 14,095,055.  Its uniform
20,000-state 4->5 sample found mean fan 2,617.482 (sampling SE 4.408), revising
the biased local-sample mean 2,731 downward by 4.16%.  Holding the Chao M4
estimate fixed gives 2.36323e12 projected 4->5 emissions.  A separate real
2,000-parent canonicalizing sample processed 5,260,480 emissions at
96.7 ns/emission wall on 24 threads with mean 7.47 search nodes.  This is a
bounded kernel calibration, not a production-duration guarantee.

The penultimate-layer Burnside counter independently reproduces the complete
C=2..5 sequence 1 / 5 / 54 / 17,120 and gives exact C=6
`M_5 = 96,452,755` in 2.070 seconds.  Its Burnside numerator is
4,444,542,950,400 over the 46,080-element coordinate group.  The production
engine now hard-checks this real-state count when it constructs or loads layer
5, providing an independent boundary anchor before the final transition.

The M4, uniform 4->5, and exact M5 capacity gates are therefore closed.  The
production-cap allocation/checkpoint/restart rehearsal is also complete.  It
allocated the 1.35-billion layer-4 table on real C=6 parents, measured
61.810 GiB peak RSS against the 61.898-GiB model, deliberately stopped at
generation/cursor 91, and then fully reloaded generations 91, 146, and 170.
The final readback restored 21,993,609 entries with zero holes and did not
modify A/B.  Active checkpoints and external backups remained unchanged.

The bounded interrupted end-to-end rehearsal is also complete.  A
rehearsal-only, configuration-fingerprinted 1/100 canonical-key parent chain
was deliberately killed after a durable 29.68-GiB S1 checkpoint, resumed at
cursor 23/124, and carried through fresh S2/S3 processes, a byte-identical S3
replay, and external arbitrary-precision summation.  It processed
21,387,180,240 / 23,637,232,504 / 55,653,192 emissions in the three stages,
captured 63,117 final classes, and produced CSV SHA-256
`B247C170370D7936D3406E485BF4A50E27198BB2AD86F16CDE32B19F87038A96`.
The watermarked checksum is
`38528041484076505706899541562753024000`; it is explicitly not `N(6)`.
L5 was two states short of the independent complete value and the final
vector was 82 classes short, confirming that rehearsal images cannot be
mistaken for complete layers.

Ten-minute rehearsal checkpoints took 22.69-35.00 seconds at S1 and
6.30-8.36 seconds at S2.  The current production recommendation is a
40-minute period, for below-about-1% measured steady-state S1 checkpoint
overhead and a roughly 40-minute recomputation window.  Four separately
authorized bounded S1 windows have since used that period; any further window
still requires a fresh repository-owner decision.

No full 63,199-class C=6 result is authorized as one open-ended run.  The four
bounded S1 windows advanced through generation/cursor 5, 15, 26, and now
37/38.  Window 4 resumed generation 26, ran for 8.4374 hours, advanced twelve
parent chunks through eleven durable images, and stopped normally after the
first post-target checkpoint.  The current image contains:

```text
generation / cursor  = 37 (.a) / 38 of 124 (30.6452% of chunks)
emissions            = 704741992192
cache hits           = 34280078
claimed / holes      = 903398620 / 18
real L4 states       = 903398602
bytes                 = 32522350448
SHA-256               = ECF0837315B0FDF8AE21C394FDA6676490E43B1828A0825529A344EC17E4E844
```

The local image and D: backup were independently rehashed to that same value
on 2026-08-15.  Generation 36 remains locally in `.b` as the previous valid
fallback.  No process is running and no L4 snapshot exists.

An append-only progress sidecar now records future checkpoint headers,
cumulative and delta counters, exact real-parent fan, timings, RSS, internal
hashes, and SHA-256 without changing the engine or checkpoint format.  It
starts with an explicitly recovered generation-37 baseline; the missing
per-checkpoint fields for generations 0--36 are not reconstructable and will
not be regenerated by rerunning closed work.

The independent final-certificate verifier now recomputes representative
legality and canonicality, stabilizers and coordinate orbit sizes, labelled
multiplicities, uniqueness/order, total mass, and the exact G1/G2
representative-to-F bindings.  It passes all 355 C=5 classes and rejects a
falsified weight.  A second exact square-sum implementation uses PowerShell/
.NET `BigInteger`.  The complete C=6 certificate gate remains unexercised
because no final CSV exists.

S2 and S3 production controllers and their runbook are frozen but dormant.
S2 requires a closed, local-and-external, manifest-pinned L4 SHA-256; S3
likewise requires L5, runs two deterministic final contractions, requires
byte-identical CSVs, invokes the semantic verifier, and cross-checks the two
independent big-integer sums.  Their existence is not authorization.  Future
S1, S2, and S3 work remain separate owner decisions.  See
`docs/runbooks/layer-dp-c6-production.md`.

## Direct-layer investigation (2026-09-05)

An independent two-missing-box Burnside counter now gives the exact native
four-row inventory `M_4(6) = 903,398,603`, with raw coordinate mass
`41,602,261,536,160`. It reproduces the complete C=2..5 layer counts and passes
independent fixed-point differentials; no production states enter that count.

Read-only header, payload and SHA-256 scans confirm that the current generation
37 contains 903,398,602 real keys with stored-stabilizer mass
41,602,261,532,320. The one missing orbit has been explicitly found, has
stabilizer 12 and orbit size 3,840, and is absent by a complete membership scan.
The retained rehearsal L5 catalogue contains 96,452,753 real keys; two explicit
absent witnesses of orbit sizes 32 and 192 close its exact 96,452,755-key
inventory and raw coordinate mass. Given the established legality, uniqueness
and stabilizer guarantees for the stored keys, the old catalogues plus these
three witnesses therefore constitute complete L4 and L5 SUPPORT SETS.

The L4 premises have since been checked over the entire repaired catalogue:
an alternative geometry-first canonicalizer audited all 903,398,603 live
records for balanced legality and exact stabilizer, and its rebuilt index
had zero duplicate orbit keys. Together with the independent Burnside count,
this proves complete repaired L4 support without assuming the old forward
enumerator's legality or orbit-uniqueness guarantees. It does not check any
old partial T weight.
See `docs/reports/og2/c6-geometry-ram-rekey-full-benchmark-20260905.md`.

A separate full-record L5 audit now verifies all 96,452,755 repaired live
records with the existing native canonicalizer: legality, canonical fixed
point, exact stabilizer, unique stable-ID lookup and erased T. Both protected
input files retained their complete SHA. Together with the independent L5
Burnside count, this proves complete repaired L5 support without assuming
the old enumerator's per-record semantics. It is not an independent second
canonicalization algorithm or a numerical F5 result. The bounded audit took
59.153134 seconds alongside F4 production, with a 4,591,161,344-byte aggregate
peak. See `docs/reports/og2/c6-l5-full-support-semantic-audit-20260905.md`.

This does not close any partial T4 weight or accept any rehearsal weight.
Original files are unchanged; no repaired production checkpoint has been
written. The witness data and source hashes are retained in
`data/golden/og2-c6-support-witnesses.json`; proofs and commands are in
`docs/math/two-missing-layer-burnside.md`.

Direct graph-value and rooted reverse-layer decision tools now pass every
native C=2..5 coefficient, including all 355 final C=5 classes. A native-key
reverse implementation substantially reduces canonicalization cost compared
with an unpaired-graph key on the same bounded C=6 samples. Parallel direct F4
values agree across 1/8/24 threads, but their kernel-only rate projects about
41.65 hours if every native state is evaluated separately.

Exact graph-value aliases are now implemented by enumerating admissible slot
pairings on both bipartitions and taking the minimum retained native key.
Complete C5 L4 is partitioned into 12,543 graph fibers without discarding any
of its 17,120 distinct native responses. Actual uniform C6 samples support
approximately 6.5-fold sharing of F4 evaluations; this is an inventory/work
estimate, not a whole-job speedup guarantee. A full read-only C6 index rebuild
confirmed zero duplicate stored keys and all 314,584 tested predecessor
lookups hit; it took 21.09 seconds with 24-thread indexing and peaked at
38.30 GiB. The source SHA-256 remained unchanged.

A combined closed-value engine now persists immutable, source-fingerprinted
alias/F4 chunks and exports a native layer only after complete alias closure.
Fresh C4/C5 runs reproduce every weight; the newly exported C5 F4 values feed
an independent reverse check of all 355 final values and the exact N(5).
Changed-thread resume, actual forced termination, corrupt headers/payloads,
invalid aliases, insertion holes and overwrite refusal are tested. On a
fiber-closed C6 sample, all 12,345 native values in 1,024 graph fibers match
independent cold F4 evaluations. This is explicitly a sample domain, not a
closed global F4 cache.

The shared-value mathematical route is therefore exact and its combined
implementation has passed complete small-case gates. Full-size C6 lookup
scaling and the complete nine-window shared-F4 scan have also passed; no
new end-to-end C6 duration or complete N(6) result is claimed. See
`docs/math/native-graph-value-sharing.md` and
`docs/reports/og2/c6-catalogue-direct-route-20260905.md`.

The first bounded real C6 shared-value window has now passed: 500,000 stable
ID records and 83,778 closed graph-representative F4 values are committed in
20 immutable chunks. Computation/commit took 15.465317 seconds; complete
engine elapsed was 47.835427 seconds and peak working set 41.714 GiB. All
21 files including the manifest have SHA-verified external physical copies.
This prefix is not a uniform timing sample and is not a closed native T4
layer. The new namespace, original-source pin and recovery status are recorded
in the checkpoint manifest and
`docs/reports/og2/c6-shared-f4-window1-20260905.md`.

A second bounded window resumed those same 20 chunks without recomputing
them and added 25,000 IDs and 3,649 closed representative values. The current
prefix is [0,525000), with 21 immutable chunks and 87,427 closed F4 values.
The engine took 46.116907 seconds including input validation and index
reconstruction; the new computation/commit took 0.716339 seconds. The
before/after external copies were independently SHA-verified. This remains
a nonuniform prefix, not a whole-population timing sample or closed T4 layer.
See `docs/reports/og2/c6-shared-f4-window2-20260905.md`.

The third bounded window processed 10,000,000 additional IDs in 400 new
immutable chunks. Its resulting prefix was [0,10525000), with 1,689,458 closed
representative F4 values. New computation/commit took 302.293218 seconds;
complete engine elapsed was 348.030754 seconds with a 41.715-GiB peak.
All 422 files (126,408,032 bytes) have independent SHA-verified external
copies. A separately bounded streaming reader checked all headers, payloads,
aliases, counters and before/after receipts, including preservation of the
previous 22 files. It found 2,156,246 processed IDs already resolving to
closed values and 8,368,754 referring to future representatives. No native
T4 export or population-wide runtime guarantee is implied. The released
counting implementation is retained in local commit `6d19785`; see
`docs/reports/og2/c6-shared-f4-window3-20260905.md`.

The fourth bounded window added 49,150,000 IDs and 7,655,609 closed F4 values
before its normal soft-time stop. Its resulting prefix was [0,59675000), with
2,387 immutable chunks and 9,345,067 closed representative values. Computing
and committing took 1,465.946353 seconds; complete engine wall time was
1,500.746988 seconds with a 41.714-GiB peak. All 2,388 committed files
(716,711,328 bytes) have independently checked external physical copies;
all 422 prior files are unchanged. The independent full-prefix audit found
15,991,860 processed IDs resolving to closed representatives and 43,683,140
pointing to future representatives. Neither this nonuniform prefix nor its
timing establishes a complete native T4 vector or a total N(6) duration.
See `docs/reports/og2/c6-shared-f4-window4-20260905.md`.

The fifth bounded window added 48,325,000 IDs and 7,682,792 closed F4 values.
It stopped normally after 1,500.045880 engine seconds, of which 1,465.255134
were new computation/commit, with a 44,791,046,144-byte peak. Its resulting
prefix [0,108000000) contains 4,320 chunks and 17,027,859 closed graph values.
All 4,321 files (1,297,106,176 bytes) have independently verified external
copies; all 2,388 prior files are unchanged. The independent production
bitset audit passed in 15.782692 seconds: 34,895,143 processed IDs address
closed representatives and 73,104,857 address future representatives. This
is not 108 million available native T4 weights or a percentage-complete
certificate for N(6). No native F4 export or F5 production chunk exists yet.
See `docs/reports/og2/c6-shared-f4-window5-20260905.md`.

The sixth bounded window added 48,375,000 IDs and 7,639,462 closed F4 values.
It stopped normally after 1,500.277530 engine seconds (1,464.604096 new
computation/commit), peaking at 44,789,915,648 bytes. Its resulting prefix
[0,156375000) has 6,255 chunks and 24,667,321 closed graph values. All 6,256
files (1,878,101,536 bytes) have independently checked physical copies; all
4,321 previous files are unchanged. The independent production bitset audit
passed in 21.938503 seconds: 59,094,214 processed IDs address closed values
and 97,280,786 address future representatives. No native F4 export, numerical
F5 production chunk or N(6) result is implied. See
`docs/reports/og2/c6-shared-f4-window6-20260905.md`.

The seventh bounded window used the released incremental CPU kernel and
added 296,200,000 IDs (nine holes) and 45,477,034 closed F4 values. Engine
wall time was 6,000.213190 seconds, with 5,949.581754 seconds of new
computation/commit and a 44,735,246,336-byte peak. The current prefix
[0,452575000) contains 18,103 chunks and 70,144,355 closed graph values.
All 18,104 files (5,435,534,624 bytes) have independently verified external
copies; all 6,256 prior files are unchanged. The 65.371485-second production
bitset audit found 300,246,735 live IDs resolving to closed values and
152,328,256 addressing future representatives. The observed engine-wall scan
rate was 49,364.913 IDs/s versus 32,244.034 in window6, a 1.531x ratio across
different ID intervals, not a controlled arithmetic-only speedup. A linear
extrapolation gives about 2.54 hours for the remaining F4 scan only; later
workload variation, audits, backups, export, F5 and the final sum are excluded.
This is not a guaranteed completion time. See
`docs/reports/og2/c6-shared-f4-window7-20260905.md`.

A separately implemented two-pass native bitset reader now reproduces every
window-4 audit counter against its immutable external copy in a bounded
same-prefix qualification: 59,675,000 records, 9,345,067 representatives,
15,991,860 resolved aliases and 43,683,140 future aliases. It used a
7,459,375-byte representative bitset, took 7.863253 seconds including its
controller, and retained only about 15.0 MiB worker / 57.0 MiB parent peak
RSS. The result is explicitly TEST_ONLY_PASS, not a replacement production
audit-chain node or independent F4 recalculation. A full-domain bitset needs
112,924,828 bytes. The subsequent 2026-09-06 full-domain audit measured
166.588572 seconds end-to-end and passed; resumable audit shards remain
unimplemented and were not needed for that guarded run.
See `docs/reports/og2/shared-f4-native-bitset-audit-20260905.md` for the
same-prefix and full resumed-protocol qualification commands and hashes.

The actual resumable reverse-F5 implementation now passes a fresh complete
C5 chain from a NEW shared-F4 export through all 355 F5 values, byte-identical
native L5 export, and independent N(5). Forced-kill recovery, changed-thread
resume, both input lineages, old-weight erasure, corruption and invalid-value
refusal also pass. This release took 40.8867173 seconds including compilation.
The C6 L5 support loader independently rejects duplicate keys, erases all old
weights, appends the two proved missing keys, and restores production
configuration. A separate-volume physical copy of that original support file
has the pinned SHA-256; both files remain unchanged.

On 512 uniformly sampled C6 L5 records, the reverse core made 2,471,101
successful predecessor lookups against the complete 903,398,603-key L4 index.
Query wall times at 1/8/24 threads were 7.761470/0.990884/0.432451 seconds,
with identical counters/checksum and no misses. The 24-thread process peaked
at 38.31 GiB. This test did NOT read numerical F4 values or compute F5; its
approximately 22.63-hour full-population query extrapolation excludes those
operations, other production overhead and sample uncertainty. No C6 F5
production chunk exists yet: a fully closed F4 export is the required input.
See `docs/runbooks/layer-reverse-c6.md` and
`docs/reports/og2/c6-reverse-f5-release-20260905.md`.

A compatible maximum-missing-edge prefix now prunes native canonical search
without changing its keys or exact stabilizers. Its isolated complete C5
two-missing coefficient gate passes. Against the same full C6 native index,
all 2,471,101 residual queries from 512 sources agree in key, stabilizer and
ID. The one-worker query wall was 9.1103971 versus 7.3035589 seconds; at 24
workers it was 0.4223821 versus 0.3306939 seconds (about 1.277x). This is a
lookup-only measurement, not a numerical F5 run or a whole-job speedup.
The optimization has now passed its actual-engine release/recovery gate,
including the complete C5 two-missing coefficient chain, all 355 F5 values
and N(5), the actual C6 dispatch on 314,584 bounded residuals, and zero-new-work
resume of an old-version closed C5 namespace. The released reverse binary
has SHA-256 `E09123878A3098EFF955AA4DA1CCAF33B875F9CCEF7D9F337DF28F3ED08AFB9B`.
The old executable is physically retained; shared-F4 and checkpoint semantics
are unchanged. The fresh complete repository regression also passed in
191.6011702 seconds. No full numerical C6 F5 run is implied.
See `docs/reports/og2/c6-compatible-prefix-lookup-20260905.md` and
`docs/reports/og2/c6-reverse-prefix-release-20260905.md`.

A different exact native representative has now passed the full-size RAM-only
rekey diagnostic, not just a small index test. All 903,398,603 live IDs and
their stabilizers were preserved; every one of 2,471,101 sampled queries
matched its original generic-native ID. Rekey plus replacement index took
74.982273 seconds; the complete guarded diagnostic took 130.204402 seconds
and peaked at 41,237,979,136 aggregate bytes. The mean 24-thread query phase
was 0.253284 seconds for the released compatible prefix and 0.192970 seconds
for geometry keys (1.31256x on this workload). Phases were not interleaved,
and no numerical F4/F5 values were read. The alternate key is not yet used
by the released reverse engine; native disk keys/chunks remain unchanged.

A same-binary CPU arithmetic ablation also passed every C4/C5 four-row value
and a 1,024-record C6 sample. The new isolated incremental packed-DSU kernel
agreed per graph in F4 and valid leaves. Median 24-thread C6 kernel time was
0.1219200 seconds versus 0.1757517 seconds for the released ternary-DP kernel
(1.442x). Three cyclic timing orders were balanced, with no competing production
process. This is a native-record sample, not the production graph-representative
population, not a global speedup measurement.
See `docs/reports/og2/c6-cpu-three-kernel-ablation-20260905.md`.

The incremental kernel is now released after complete actual-engine gates,
all 12,345 C6 sample values, old-version C5 chunk/export compatibility, a
fresh full repository gate and installed-path shared checks. The installed
binary SHA is `2B8A14606F8F78748C9338130D35551CA199E53238BEB33D95BCB05A2555AFE0`.
The old binary has local and external physical rollback copies. Native keys,
weights, alias rules and checkpoint formats are unchanged. Full and installed
gates took 192.056392 and 27.417311 seconds; no counting process remained
from either gate. See
`docs/reports/og2/shared-f4-incremental-publication-20260905.md`.

The S3 controller now binds the actual full-SHA resume parent and each final
CSV/snapshot/command tuple to an immutable source/stage receipt. Disposable
C5 gates passed two separate native contractions, all 355 classes, semantic
verification, both exact sums and interrupted-publication recovery; changed
weights, stale artifacts and unbound legacy namespaces are refused. Only
controller sidecars and safety checks changed, not counting arithmetic or
native formats. Session defaults now reserve backup time within the owner's
daily availability; an external watchdog is still required for storage stalls.
This does not supply the missing closed C6 F4/F5 inputs or launch S3.
The final complete repository gate passed in 190.6233420 seconds under a
360-second / 8-GiB aggregate guard, with no surviving child processes.
See `docs/reports/og2/c6-s3-lineage-handoff-20260905.md`.

## Current checkpoints

All retained checkpoints are documented in `data/checkpoints/MANIFEST.md`.
The original layer-DP production image remains the partial generation 37
described above. The new shared-F4 namespace contains the independently
closed representative values/aliases just described and now has a verified
native F4 export. The new F5 namespace has a reviewed 30,200,000-ID prefix;
the owner's later session is in progress. The separate factorization/orbit
graph memo contains 5,315,962 closed
exact values.  Neither binary is stored in Git.  Factorization verification
must use `checkpointreadonly`; layer-DP production images are inspected only
through their read-only header/hash helper or a protected staged resume.

## Active implementation tracks

- `layer_dp_gate` (experiments/proto): row-incremental layer DP, the
  qualified C=6 candidate; audited runbook in `docs/methods/layer-dp.md`.
- `layer_shared_f4` / `layer_reverse_f5` (experiments/proto): new exact
  support-reuse route, complete C5 and recovery gates passed; bounded real
  C6 F4 namespace now completely closed, independently audited and exported.
  Numerical F5 has a reviewed 30,200,000-ID prefix and an owner-run continuation;
  no complete F5 catalogue or final sum yet.
- `factorization_orbit`: primary C=2..5 exact factorization/orbit engine and
  the C=5 oracle for the layer DP.
- `multiset_q`: independent transfer-kernel research and cross-check route.
- `multiset_fast` / `multiset_c6`: older independent exact validators.
- `main.cpp`: completed FJ05 9x9 reproduction.

See `docs/index.md` for the evidence and provenance map.
