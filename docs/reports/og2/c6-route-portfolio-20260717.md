# C=6 route portfolio after the linear-sum and connectivity audits

Date: 2026-07-17

## Scope

This report audits the external material received on 2026-07-15 and
2026-07-17 against the actual squared objective

```text
N(C) = sum_[G] w([G]) F_C(G)^2.
```

Raw expert files remain research inputs under `docs/expert/`.  Conclusions in
this report were accepted only after local source inspection, Windows builds,
small-C exact gates, and differential checks.  No C=6 outer-class run or
checkpoint write was performed for this audit.

## The 2026-07-15 linear result is out of scope

The supplied 132-state histogram recurrence exactly evaluates

```text
N_linear(C) = sum_[G] w([G]) F_C(G),
```

not the Sudoku square sum.  The response itself states this distinction.  The
misdirected calculation resulted from a project question that omitted the
square.

The programs are internally consistent and were reproduced locally, but their
small-C values are decisive:

```text
C=2: N_linear = 96       while N(2) = 288
C=3: N_linear = 460800   while N(3) = 28200960
```

An audit variant of the supplied direct C=3 skeleton enumerator accumulated
both quantities over all 8,000 labelled balanced skeletons:

```text
sum F_3    = 460800
sum F_3^2  = 28200960
```

Therefore the 132-state collapse is a correct result for a different first
moment.  It cannot recover the required second moment and is rejected as a
route to `N(6)`.

## The 2026-07-17 connectivity quotient is for the correct square

For each labelled symbol `s`, the new recurrence selects two permutations
`sigma_s,tau_s in S_C`.  In each band it inserts an edge
`(sigma_s(r),tau_s(r))` into a bipartite multigraph of maximum degree two.
Every completed cycle contributes the exact shared-skeleton factor two.

A partial band graph is represented by:

1. the degree `0,1,2` of each left and right color vertex; and
2. a mate involution pairing the endpoints of each live path.

Internal degree-two vertices cannot receive future edges, and a closed cycle
cannot interact with later edges.  Deleting closed cycles immediately while
multiplying by two is therefore an exact continuation quotient.  The global
state is the tuple of band states modulo band permutations, independent color
permutations in the two stacks, and stack swap.

This is a genuinely different contraction order from the active row-pair
joint-mask recurrence.  It globally sums shared skeletons and does not
enumerate the 63,199 outer orbits.

## Windows reproduction

The raw sources require Boost multiprecision, which is not installed in the
repository's default MinGW toolchain.  They were compiled without algorithmic
changes through an audit-only `cpp_int` to GMP compatibility header under
`data/logs/`.  The checked programs used 32 OpenMP threads.

| program and gate | exact state sequence or result | local wall time |
|---|---|---:|
| connectivity C=2 | `1,1,3,1,1`; `N(2)=288` | 0.172 s |
| connectivity C=3 | `1,1,8,18,19,1,1`; `N(3)=28200960` | 0.037 s |
| checked connectivity C=4 | `1,1,28,700,12856,9708,155,1,1`; exact `N(4)` | 3.959 s |
| initialized C=5 through symbol 3 | `1,1,93,83776` | 3.331 s |
| count-tensor C=2 | exact `N(2)` | 0.039 s |
| count-tensor C=3 | exact `N(3)` | 0.021 s |

The checked C=4 result was

```text
N(4) = 29136487207403520.
```

Every checked multiplication and accumulation was promoted to
`__uint128_t`; no `uint64_t` overflow was observed.  The count-tensor C=4
route did not finish within a five-minute local bound and was stopped without
leaving a process.  This does not contradict the supplied report, which only
claimed a central C=4 tensor probe and rejected that state as weaker.

The response links four C=4/C=5 `.out` and `.err` records, but those files were
not present in the received directory.  The reported peak-RSS figures could
therefore not be checked from the original logs.  The state counts, totals,
and principal timing claims were reproduced directly.

## Differential checks

The following checks were added in ignored audit code and passed:

- C=3 reachable states were tested over complete group images; C=4 reachable
  states were tested for orbit membership and sampled group invariance.
- Canonical representatives were confirmed to belong to the input orbit.
- Disabling stack swap preserved `N(3)` and `N(4)` while increasing frontier
  sizes, as an exact quotient should.
- The optimized two-symbol initializer was compared key by key with direct
  second-symbol permutation-pair enumeration:

```text
C=3:  8 states  [OK]
C=4: 28 states  [OK]
C=5: 93 states  [OK]
```

Thus the C=5 `93 -> 83776` probe is supported by an independent multiplicity
audit, not only by its final state count.

## Scaling decision

The connectivity quotient is exact but is not a sufficient compression by
itself.  After only three of ten symbols, C=5 already has 83,776 states,
slightly more than the 76,249-state middle layer of the established C=5 band
recurrence.  Its next direct transition can inspect as many as

```text
83776 * (5!)^2 = 1206374400
```

permutation pairs.  The supplied connectivity code therefore does not pass
the C=5 gate, whereas the active factorization/orbit engine completes the full
C=5 total in about 8.66 seconds on the reference machine.

The one new unresolved mechanism is an implicit application of the
one-symbol operator as an operator-valued double permanent.  A subset-mask DP
would sum partial left/right permutations before all 14,400 completed pairs
are materialized.  No implementation or frontier measurement was supplied.
This is a bounded decision experiment, not yet a C=6 algorithm.

Before any C=6 extension, that operator must:

1. agree per source and per canonical target with direct permutation-pair
   enumeration at C=3 and C=4;
2. measure its internal frontier on the existing 83,776-state C=5 layer;
3. reproduce the full exact `N(5)` with CRT or arbitrary-precision weights.

As an engineering heuristic rather than a proved threshold, an average
internal frontier below roughly 200--300 records per source would justify
further work.  Several thousand records would probably be insufficient;
near-injective partial placements would reject the route as a primary
mechanism.

## Route portfolio

`Rejected` below means that the named compression mechanism failed its exact
decision test.  It is not an impossibility claim about every future algorithm
using related mathematics.

| status | route | audited evidence or missing test |
|---|---|---|
| rejected as primary | 132-state outer marginalization | Computes `sum wF`, not `sum wF^2` |
| rejected as primary | ordinary 3+3 low-rank/channel truncation | Exact ranks 630/630 at C=4 and 8001/8001 at C=5; no forced C=6 channel loss |
| rejected as primary | natural matching/Johnson low-rank collapse | Local balanced-cut transform has rank 462/462 |
| rejected as primary | materialized midpoint or ordinary Burnside/character basis | Exact spaces are no smaller than the orbit list and the useful midpoint is much larger |
| rejected as primary | naive independent 63,199-class sweep | G2 alone needs a 9.08-GB frontier and 824.440-s table-backed tail |
| rejected as primary | whole-tail scalar memoization within G2 | 100,000/100,000 complete G2 color-tail signatures were distinct; lower-level cross-class reuse remains open |
| rejected as primary | full C^3 count tensor | Weaker than connectivity at C=4 and did not finish the bounded local full run |
| proven component only | factorization/orbit and future-twin per-class engines | Fast C=5 and exact G1/G2 values, but no affordable global outer sum |
| proven component only | external pair-tail closure | Closes G1/G2 exactly but retains one large frontier per outer class |
| proven component only | shared color-canonical half-kernel table | Gives a 4.41x G2 tail speedup and exact leaf sharing, but no prefix sharing |
| proven component only | connectivity/path quotient | Exact C=2..4; already 83,776 states at C=5 symbol 3 |
| proven component only | external sorting, streaming, and checkpointing | Controls RAM and restart risk but does not reduce arithmetic work |
| open decision experiment | operator-valued double-permanent subset DP | Exact formula supplied; implementation and C=5 internal-frontier data absent |
| open decision experiment | later-class half-kernel coverage | G1/G2 table is complete only for two of 63,199 outer classes |
| open decision experiment | kernel-pair and relative-transform batching | Complete signatures are unique, but repetition below that level is unmeasured |
| open, lower-priority decision | exact rank of the reduced band kernel | Distinct from the 3+3 ranks; the proposed C=5 modular-rank experiment was never completed |
| open theory route | balanced-switch/coherent-configuration transform | Naive per-symbol Johnson commutation is inadequate; no compact algebra or fast transform is known |
| open high-upside route | cross-class symbolic prefix/global contraction | Could remove class-local frontiers; no exact bounded implementation yet |
| open engineering route | streamed consumption of the final reduced layer | Could remove the 9-GB monolith, but not the per-state contractions |

## Retired implementation variants

The following exact variants lost bounded comparisons and are not retained as
independent routes:

- fixed future-twin row orders reached multi-million-state frontiers before
  the joint quotient;
- recursive row-adaptive memoization produced about five million G1
  subproblems in three minutes without closing;
- row/color-dual adaptive variants moved the state explosion and were slower
  on complete C=5 gates;
- eager raw color-orbit expansion reached roughly 49 million mapping entries
  on the G2 sample, while the first all-six cached full rescan hit its
  30-minute/8-GB bound;
- larger graph-canonical caches and batches changed only constants in bounded
  tests.

These are negative results about implementation choices.  They do not negate
the retained pair-tail, stateless color quotient, or half-kernel table.

## Decision

The external work has been most valuable at proving exact local transforms and
closing or rejecting specific mechanisms.  It has not yet produced a global
C=6 computation whose cost is below the current boundary.

The next highest-value global experiment is the C=5 operator-valued
double-permanent measurement.  In parallel, the existing per-class track
should run only bounded later-class coverage and kernel-pair repetition probes.
Neither track authorizes a full 63,199-class computation.
