# Maximum-missing-edge pruning for the unchanged native key

Date: 2026-09-05. This note proves a compatible canonical-search pruning rule
and records a separately implemented finite gate. It does not change the
released producer, reverse kernel, controllers, checkpoint keys or formats.
No C6 F5 values, full replacement index or N(6) were computed here.

Two different proposals must not be conflated:

| Proposal | Output key | Existing native index |
|---|---|---|
| Maximum-missing-edge prefix | Exactly the current key | Compatible, subject to release gates |
| Canonical missing-geometry transporter trie | Different exact native key | Cannot be queried directly with these keys |

The second proposal never identifies distinct native states: it changes
their representative, not their equivalence relation. An eventual in-memory
index could transform every loaded L4 key while retaining its stable ID,
stabilizer and closed F4. It would replace the native-key index, not require
changing saved checkpoints. That full-size construction and lookup benchmark
has **not** been executed.

## Definitions and exact scope

Let C be the box count, n=2C, and L=C-2. A native state is a multiset of n
symbol masks. Each mask occupies exactly L distinct boxes, using one side
in each occupied box; each of the 2C slots occurs in exactly L masks.
The encoded per-box fields are 0 (missing), 2 (side zero), and 3 (side one).
Permuting boxes and independently flipping their sides gives
G=(C2)^C semidirect S_C. Symbol positions are unlabelled; equal masks remain
separate positions when bit masks encode their occurrences.

Define Z_b to be the set of symbol positions missing box b. Balance gives
|Z_b|=2C-2L=4. Every symbol misses two boxes, so

```text
H[b,c] = |Z_b intersect Z_c|
```

is the edge multiplicity of a loopless 4-regular multigraph on the C boxes.
This includes C=2,L=0: four empty masks give four parallel missing edges.

The current `Canonizer` in `experiments/proto/layer_dp_gate.cpp` minimizes
the sequence of column signatures K0,...,K(C-1), with one initially
undivided group of symbol positions. Choosing a box and side orientation
refines every existing group into its 0,2,3 subgroups, in that order. A
column signature concatenates the sorted field digits inside these ordered
groups. All signatures have fixed length n in base four and therefore have
the same lexicographic and unsigned-integer order.

This note requires the **unseeded initial partition**: `g_wlseed=false`,
with no one-missing anchor. The released C6 L4 predecessor path has exactly
these semantics. It is not permission to apply the pruning to the anchored
L=C-1 canonicalizer, an arbitrary colored partition, or a different
canonical comparison order.

## Lemma A: exact compatible prefix pruning

**(A) Self-contained proof.** Put M=max_{b<c} H[b,c]. Every globally minimal
native canonical path begins with an ordered pair (b,c) satisfying
H[b,c]=M. Therefore it is exact to restrict:

1. The first box to endpoints of at least one multiplicity-M edge.
2. The second box to multiplicity-M neighbors of the selected first box.
3. Neither subsequent box choices nor either side orientation.

Proof. Every first-column signature is the same: four zeros, L twos and
L threes, regardless of the selected box and flip. Consequently K0 cannot
distinguish any complete coordinate path.

After selecting first box b, the first refined group is Z_b. Flipping b
does not change that group or its position. For any second box c and either
flip, the first four digits of K1 are the sorted fields on Z_b. Exactly
H[b,c] of those digits are zero; every remaining digit is 2 or 3.

Compare two paths whose first two boxes have missing-edge multiplicities
u<v. Their K0 signatures agree. In K1, the first u digits are zero in
both paths, but the next digit is nonzero in the first path and zero in the
second. Thus the second path has a strictly smaller K1, independently of
the flips and every later digit or column. Any path beginning with a
nonmaximum edge therefore loses to every path beginning with a maximum
edge. Each ordered pair of distinct boxes extends to a complete box order,
so such comparison paths always exist. The discarded paths cannot attain
the global minimum. This proves the restriction.

All maximum-edge ties and both flips must be retained. The restricted
search contains **every** globally minimizing coordinate transformation,
not just one representative of each tie. It follows that both the winning
state and the number of minimizing transformations are unchanged. In
particular the current exact `minCount`/stabilizer needs no adjustment.
Repeated masks cause no exception: H counts their separate row positions,
exactly as the current `z0` bit masks and signature popcounts do. QED.

This is a search pruning, not a quotient of response vectors or a reduction
in their dimension. No transition, orbit, multiplicity or color-factorial
coefficient is added or removed.

An independent mathematical red-team review confirmed the proof and the
unseeded/non-anchored precondition. That review performed no computation;
the executed checks below are the finite evidence.

## Lemma B: the distinct geometry-first key

**(A) Self-contained proof.** Choose one labelled representative h of each
missing-multigraph isomorphism class. For input H, enumerate all box
transporters sending H to h; allow every side flip. Minimize the same
signature sequence over exactly those transformations.

If native inputs x,y are coordinate-equivalent, the equivalence sends their
allowed transporter sets bijectively onto each other, and their possible
output states are identical. Thus their minima agree. Conversely every
output is a coordinate image of its input, so equal minima imply that the
inputs are native-equivalent. Hence this is an exact strong native key,
but there is no reason for its selected representative to equal the
unrestricted native minimum. Its minimizing transformations again form a
full stabilizer coset, so their count is the true stabilizer order.

The prototype enumerated 3,355 labelled C6 missing graphs in 24 classes.
Across all labelled graphs, the number of transporters is exactly
24*6!=17,280: for each class, every permutation of its fixed representative
contributes one transporter to one labelled image. Their box-order tries
have 91,376 nodes. Construction took about 0.003 seconds. This only removes
box-order choices; all side bits and symbol multiplicities are retained.

## Retained implementation and executed checks

The isolated executable is
`experiments/proto/layer_missing_geometry_canon_probe.cpp`. It includes the
unchanged native engine as its reference. It implements both Lemma A and
Lemma B separately from that reference; no active source was edited.

Build command:

```powershell
g++ -O3 -mpopcnt -std=c++20 -fopenmp -Wall -Wextra `
  experiments/proto/layer_missing_geometry_canon_probe.cpp `
  -o build/layer_missing_geometry_canon_probe.exe -lpsapi
```

The build emitted no warnings. The recorded source SHA-256 is
`662F1E7A82BEE2F4CE53C0BC99808160808A030E35B3C5E97F14C4454FD0DB1D`;
the executable SHA-256 is
`0A62D2DC3131E8C81EAA93146064B8FB822962F4F9F268121285144EA7AAFE26`.

Each executable uses one computing thread, an explicit positive query cap,
a maximum 120-second wall budget and a 1-GiB RSS guard. Inputs are small
read-only text fixtures, not binary catalogues. No F4-producing process,
checkpoint, reverse production source or controller was altered.

**(C) Exact finite certificates.** All applicable complete small-C native
two-missing layers passed key equality for Lemma A, key separation for
Lemma B, exact stabilizer equality and stabilizer-histogram equality:

| C | L | Complete native classes | Raw orbit mass | Tested coordinate transforms |
|---:|---:|---:|---:|---:|
| 2 | 0 | 1 | 1 | 8, all G2 elements |
| 3 | 1 | 1 | 1 | 48, all G3 elements |
| 4 | 2 | 23 | 2,019 | 8,832, all G4 elements on every state |
| 5 | 3 | 16,150 | 59,661,280 | 129,200, eight fixed-seed elements per state |

The C2 source is explicitly `0 0 0 0 expected=1`; it is an auxiliary empty
layer, not a nonempty Sudoku band. For C2..4 every state also passed an
independent full-group stabilizer and orbit-membership scan; C5 used eight
such full-group referees in addition to all-state differential checks.
Both new procedures passed idempotence and the listed invariance tests.

The exact complete stabilizer histograms, written as stabilizer:classes,
are:

```text
C2: 8:1
C3: 48:1
C4: 2:5,4:8,8:4,16:3,32:2,128:1
C5: 1:15013,2:977,3:1,4:123,5:3,6:3,8:20,10:2,12:1,20:4,24:3
```

The complete C5 numerical chain deliberately exercises the **two-missing
L3 predecessor branch**, not the one-missing L4 predecessor of an F5 gate:

```powershell
build/layer_missing_geometry_canon_probe.exe 5 4 `
  input=data/logs/layer-direct-gate-20260905-fixtures-v2/c5.L4.txt `
  preload=data/logs/layer-direct-gate-20260905-fixtures-v2/c5.L3.txt `
  limit=17120 maxqueries=50000000 maxseconds=120 invariance=0
```

Actual output:

```text
VALUE_CHAIN COMPLETE_C5_TWOMISSING predecessors=16150 targets=17120
labelled_matchings=3375557 weak_queries=2935081
baseline_geometry_prefix_equal=YES per_query_stab_equal=YES
closed_value_stab_histogram_equal=YES
sumF4=3972941184 sumOrbitF4=14365876248576
seconds=19.0732 peak_bytes=9965568
concurrent_F4_window=YES N6=NOT_COMPUTED
```

For every target, all three computations use the exact rooted identity
F4=4*sum(matching multiplicity*F3(residual)) and agree with its independent
closed fixture value. Every one of the 2,935,081 weak residuals has equal
native/prefix keys and equal stabilizers for all three procedures. Each
predecessor is found in the complete 16,150-state closed table. Per-target
value and stabilizer equality implies the full value/stabilizer histogram
equality; the test also rejects missing or duplicate classes.

**(C) C6 bounded differential.** On all 1,024 uniform native L4 samples,
Lemma A changed zero native keys and zero stabilizers. Lemma B also
preserved separation/stabilizers, but changed every representative. Both
passed 8,192 transformed-state checks and eight full-group referees.

On the first 64 states of the independently generated uniform L5 sample,
337,680 rooted labelled matchings yielded 314,584 weak residual queries in
314,117 distinct native classes. All compatible-prefix keys and all three
stabilizers agreed. The query stabilizer histogram was
`1:314223,2:360,4:1`; it counts query occurrences, not distinct classes.
There were 1,024 additional transform checks and eight full-group referees.
The geometry-first key differed on 314,550 queries, emphasizing why the
current index cannot accept it without rekeying.

## Timings, operation counts and nonclaims

**(D) Bounded performance assessment.** Three passes over the same 314,584
C6 residuals rotated kernel order as native/geometry/prefix,
geometry/prefix/native, prefix/native/geometry. Exact work totals were:

| Kernel | Three-pass time | Search nodes | Speed relative to native |
|---|---:|---:|---:|
| Current native | 2.9959821 s | 20,303,505 | 1 |
| Compatible maximum-edge prefix | 2.1086950 s | 16,696,035 | 1.4208 |
| Different geometry-first key | 1.5439177 s | 12,640,449 | 1.9405 |

Peak process RSS was 102,658,048 bytes. The compatible implementation
evaluated 82,309,758 signatures across the three passes: approximately
87.2 signatures and 17.7 search nodes per query, compared with about
21.5 native search nodes. Its additional invariant costs 15 pairwise
12-bit intersections/popcounts and simple maximum-edge bookkeeping per C6
query. It needs no missing-graph table or alternate lookup index.

These tests ran **concurrently with the owner's real shared-F4 window**;
they are exploratory same-input comparisons, not isolated workstation
benchmarks. Kernel rotation reduces ordering bias but does not eliminate
concurrent-load effects. No ratios from the tiny C2/C3 tests are meaningful.
No full-catalogue random lookup, closed F4 memory read, F5 production
arithmetic, chunk IO, total S2 time or daily completion date is established
by these measurements. The geometry-first alternative remains unqualified
for full-size adoption.

Retained logs are under `data/logs/c6-direct-route-20260905/`:

```text
canon-prefix-c2.log
canon-prefix-c3.log
canon-prefix-c4.log
canon-prefix-c5-l3.log
canon-prefix-c5-value-chain.log
canon-prefix-c6-l4-sample.log
canon-prefix-c6-residual64.log
```

Each log records the exact inventory, domain, sample counts, histograms,
bounds-related peak, canonical operation counts and timing. For the timing
case the executed command was:

```powershell
build/layer_missing_geometry_canon_probe.exe 6 5 `
  input=data/logs/c6-direct-route-20260905/l5-sample-512.txt `
  limit=64 maxqueries=500000 maxseconds=120 invariance=128 rounds=3
```

The possible next step is a separately released reverse-only implementation
with this narrow dispatch precondition, unchanged native keys and a full
reverse release gate. No change to the active shared-F4 producer follows
from this note.
