# Exact 2x6 Sudoku: a standalone research challenge

## ROLE AND ENVIRONMENT

You are an expert in exact combinatorial counting, graph factorizations,
invariant theory, and structured linear operators. Your task is mathematical
research with finite computational checks, not project management.

This message is your entire input. You have no repository, checkpoints,
attachments, network access, external class catalogue, or independent
subagents. Do not ask for those resources or pretend to have inspected them.
Use Python and its standard library for finite checks; any other dependency
must actually be available and disclosed. A complete small-case reference
implementation is included below.

Maintain logically separate proof branches yourself. Do not describe them
as independent researchers, independent executions, or independent evidence
unless that independence actually exists.

## TARGET: ONE FIXED PROBLEM

Find and justify a substantially less expensive EXACT method to compute
N(6), the number of labelled 12x12 Sudoku grids with 2x6 boxes.

The current verified method globally aggregates histories, but still needs
about 4.50 trillion native transitions across its two dominant stages. Its
remaining work projects to roughly 111--124 workstation hours before final
overheads. The owner can devote at most eight hours per day and must also
use the machine for other tasks. This means about 14--16 fully dedicated
calculation days, potentially more calendar days.

The primary deliverable is a concrete exact structural improvement with a
credible C=6 cost bound or measured projection. An exact counterexample or
a scoped impossibility result eliminating a serious proposed improvement
is also valuable. If you find neither, say so explicitly.

Research usefulness: an overall 3--5x improvement could reduce the burden
to several calculation days; 10x would change it substantially. These are
decision targets, not assumptions that such speedups exist. A smaller proved
improvement is reportable, but must not be presented as solving the resource
problem.

You may compute either:

- the exact final class vector and its weighted square; or
- the exact scalar N(6) directly, with an independent verification strategy.

You are NOT required to materialize the existing nine-hundred-million-state
layer or all 63,199 final class values. Do not build those requirements into
a purported lower bound unless your theorem explicitly assumes them.

Do NOT replace the target by an estimate, a leading-digit approximation,
a one-band count, a linear marginal, or a formula whose evaluation retains
the original computational bottleneck.

## CLAIM DISCIPLINE

Label every substantive lemma or algorithmic claim:

- **(A) PROVED HERE:** a complete self-contained proof.
- **(B) EXTERNAL THEOREM:** precise statement, hypotheses, and an accurately
  identified reference; explain the reduction. If you cannot verify the
  theorem's statement or applicability, label the use conditional instead.
- **(C) EXACT FINITE CERTIFICATE:** code you actually executed, exact output,
  and the exact finite domain covered. State whether arithmetic was integer,
  rational, or modular and what follows over the rationals.
- **(D) CONDITIONAL / HEURISTIC / NUMERICAL:** assumptions, unresolved steps,
  sampling uncertainty, floating-point evidence, or performance projections.

The supplied facts below are **INPUT-EXACT** or **INPUT-MEASURED**, not your
new results. You may use them as premises without reconstructing the full
original computations. Do not claim to have independently checked supplied
facts merely by repeating their values.

An assertion logically equivalent to the target, with comparable evaluation
cost, is an **EQUIVALENT RESTATEMENT**. A generating function, tensor network,
Gram matrix, permanent expression, or change of basis alone is not progress.
An algorithm must account for evaluating its representation.

No network is available: do internal due diligence by checking definitions,
normalizations, small cases, and the applicability of every supplied barrier.
Do not claim an exhaustive literature search or novelty in the literature.

## 1. COMPLETE MATHEMATICAL DEFINITION

For general C, let n=2C. Transpose the usual 2-by-C Sudoku boxes. The grid
then has two horizontal bands, each with C rows and C boxes of size C-by-2.
Transposition is a bijection and does not alter the count. Rows, columns,
boxes at their fixed positions, and the n symbols are labelled. We count
all grids, not equivalence classes of grids.

There are n slots in one row, paired as (b,s), where 0<=b<C is the box and
s in {0,1} is its side. A native L-row state is a multiset x of n occurrence
masks u satisfying:

1. Each u contains exactly L slots.
2. Each u contains at most one slot from each box pair.
3. Each slot occurs in exactly L masks, counting multiplicities.

Give the multiset copies temporary distinct symbol labels. The bipartite
incidence graph Q_x joins n symbols to n slots, with edge (i,v) iff the
mask for symbol i contains slot v. It is a simple L-regular bipartite graph.

Define F_L(x) to be the number of ordered decompositions of E(Q_x) into L
perfect matchings. The matchings have distinct row labels. Equivalently,
F_L counts proper edge colorings with exactly these L labelled colors.
Temporary relabellings of equal-mask symbols do not change F_L.

Every state satisfying 1--3 is row-reachable: regular bipartite graphs
decompose into perfect matchings. The box restriction is already encoded
by condition 2. Also F_1=1 and F_2=2^(number of cycle components of Q_x).

The coordinate group G_C consists of arbitrary permutations of the C boxes
and independent side swaps in each box:

```text
G_C = C_2 wr S_C,     |G_C| = 2^C C!.
```

Symbol permutations have already been quotiented by taking a multiset.
For a coordinate orbit x, define

```text
s_x = |Stab_G(x)|,
m_x = |G_C| / s_x,
c_u(x) = multiplicity of mask u,
ell_x = (2C)! / product_u c_u(x)!.
```

At L=C a complete state q specifies the symbols in each slot of the first
band. Its second-band incidence is the complement in every slot. Because
each symbol occupies exactly one side of each box, this complement is
obtained by swapping all box sides, so both bands have the same F_C(q).

The exact objective and weight normalization are

```text
N(C) = sum_[q] m_q ell_q F_C(q)^2,
sum_[q] m_q ell_q = binomial(2C,C)^C.
```

There are no omitted symbol, row-order, or coordinate factorials. In
particular, a recurrence for sum m_q ell_q F_C(q) does not solve this target.

## 2. THE CURRENT EXACT GLOBAL RECURRENCE

Store orbit-total coefficients T_L(x)=m_x F_L(x). There is a unique one-row
orbit x1, represented by the multiset of all singleton slots, with T_1(x1)=1.

For a representative x, let a_u be the multiplicity of each distinct mask u.
An emission chooses sets S_u of slots such that:

- |S_u|=a_u;
- each chosen slot belongs to a box entirely absent from u;
- the S_u partition all n slots.

The child multiset z contains u union {v} for every v in S_u. Write d_w for
its multiplicities. The exact emission coefficient is

```text
K(z) = product_w d_w!.
```

This is the one-row contingency coefficient
`product_w d_w! / product_(u,v) n_(u,v)!`, whose denominator is one because
each new slot is used once. Do not permute identical input copies or replace
the output factorials by input factorials.

Let can(z) be any consistent representative of the full G_C orbit. Then

```text
T_(L+1)(can(z)) += T_L(x) K(z).
```

Let R_L(x,y) sum K over emissions from x landing in orbit y. As row vectors,

```text
T_(L+1) = T_L R_L,
F_C(q) = T_C(q)/m_q,
N(C) = sum_q (ell_q/m_q) T_C(q)^2.
```

T_C(q)/m_q is integral; ell_q/m_q alone need not be. Use exact arithmetic.
The present engine uses 64-bit intermediate T through C=6 layer 5, 128-bit
final T, and arbitrary precision for the final square sum.

This recurrence already combines different construction histories globally.
Its remaining bottleneck is the number and cost of emissions, not failure
to memoize entire past histories.

## 3. INPUT-EXACT: FINITE ANCHORS

Native coordinate-orbit counts M_1,...,M_C:

```text
C=2: 1, 2
C=3: 1, 5, 4
C=4: 1, 23, 54, 26
C=5: 1, 107, 16150, 17120, 355
```

Native emission counts under exactly the grouped convention in section 2:

```text
C=2: 4
C=3: 80, 29
C=4: 4752, 4630, 712
C=5: 440192, 2324325, 11957632, 462403
```

Verified totals:

```text
N(2) = 288
N(3) = 28200960
N(4) = 29136487207403520
N(5) = 1903816047972624930994913280000
```

Smallest normalization witness, C=2: encode a complete mask as a C-bit
word whose bit b is its side in box b. The two orbits are

```text
{0,0,3,3}: s=4, m=2, ell=6,  F=4, T=8, contribution=192
{0,1,2,3}: s=8, m=1, ell=24, F=2, T=2, contribution=96
```

Their contributions sum to 288. A proposal failing this test is rejected.

C=6 exact facts:

```text
M1=1; M2=772; M3=12324872; M5=96452755; M6=63199.
1->2 emissions = 59245120.
2->3 emissions = 2605194602.
L2 sum of coordinate orbit sizes = 20338525.
L3 sum of coordinate orbit sizes = 566455903200.
M5 Burnside numerator = 4444542950400; |G6|=46080.
sum of complete weights m_q ell_q = 622345892187672576 = 924^6.
```

The exact full M4 is not supplied and has not been closed by production.
Observed distinct production keys imply M4 >= 903398602.

Two specific complete graphs have independently closed factorization values.
The following C-bit word multisets fully specify them without class indices:

```text
G1 = {0,5,10,19,29,30,39,43,44,48,54,57}
s_G1=120, m_G1=384, F6(G1)=6986348258918400.

G2 = {0,5,10,23,27,28,35,44,47,48,54,57}
s_G2=8, m_G2=5760, F6(G2)=7053808087203840.
```

Historical comparison target, NOT independently closed by this project:

```text
N(6) = 38296278920738107863746324732012492486187417600000.
```

Do not use this decimal to fill missing values, fit a formula, reconstruct
unknown residues, or certify an incomplete computation.

## 4. INPUT-MEASURED: THE RESOURCE WALL

The baseline machine has approximately 125.65 GiB physical RAM. Production
uses 24 threads. It is a shared workstation, not a continuously available
dedicated server.

The two dominant emission projections are

```text
S1, 3->4: 2.13308e12
S2, 4->5: 2.36323e12
```

The S2 fan estimate comes from a nearly saturated canonical hash window:
mean 2617.482, sampling SE 4.408 over 20000 L4 states. A larger sparse
rehearsal gives mean 2617.908. Sampling error does not include all population
capture uncertainty. These are not exact full-layer emission totals.

Current partial S1:

```text
processed parent chunks: 38/124, chunk size 100000
processed emissions:    704741992192
distinct child keys:    903398602
stored checkpoint:     32522350448 bytes
last window:           8.4374 hours
window emissions:      219681931472
window new keys:       7
peak RSS:              61.856 GiB
```

The T4 weights remain sums over a processed parent prefix, not complete
F4 values. Finding almost no new keys does not permit early stopping.

An older statistical M4 estimate, 902863734, is slightly below the current
observed lower bound. It was an estimate, not a theorem or upper bound.

Typically an emission sorts twelve child masks, canonicalizes, performs a
large-table lookup, and adds a weighted contribution. A per-parent raw-child
cache, reset on every parent, hits only about 0.00486% cumulatively. This
number is not the global canonical reuse rate. Checkpoint writing costs
about 0.8% of the last window.

Remaining S1 projects to about 55--60 hours. S2 measurements of
84.8--96.7 ns/emission WALL TIME ON 24 THREADS project to about 56--64 hours.
Do not divide these wall constants by 24 again. Production memory effects,
verification, and interruptions can increase the elapsed calendar time.

A sparse rehearsal retaining approximately one parent in 100 at each large
stage already reaches 903346741 L4 keys, 96452753 L5 keys, and 63117 final
classes. Its weights belong to a different sparse operator. Reaching most
keys is much cheaper than closing the exact full weights.

## 5. VERIFIED FRONTIER: DO NOT REDISCOVER OR OVERGENERALIZE

Treat the following as supplied exact results or measurements, with the
stated scope. Extend the list with your own verified eliminations.

### 5.1 Wrong scalar, inadequate reuse, and naive gluing

- A 132-state one-copy recurrence outputs 96 and 460800 at C=2,3. It
  computes a linear quantity, not N(C). A new small scalar state must
  preserve the square and pass the 288 normalization test.
- Per-class closure is not a cheap substitute for global work. G2 alone
  has 221438460 tail states in a 9.08-GB frontier. Even a fully precomputed
  shared half-kernel table leaves 824.440 seconds of G2 tail contraction.
  Its coverage on later classes can fall to zero.
- Class-local 2+4 enumeration means choosing a spanning 2-factor H of a
  complete 6-regular graph Q, then evaluating a residual F4(Q-H). Measured
  ordinary Q have about 1.33--1.39 billion such H. Later residual memo hits
  are about 2--4%; a ten-class union saves only 0.56% of sampled keys.
  This rejects that memo/enumeration design, not every global use of F4.
- Reverse 2+2 gluing uses the 772 native two-row coordinate orbits. Of these,
  276 have trivial G6 stabilizer. Their 38226 unordered pairs alone expose
  38226*46080=1761454080 relative coordinate placements. For a generic
  100000-emission sample, about 99.4% remain distinct after canonicalization.
  Streaming these placements changes RAM, not their number. A bulk method
  must explain what work disappears before materializing those incidences.

### 5.2 A rank fact ABOUT THE NATIVE OPERATORS DEFINED HERE

Let U_L=R_L R_(L+1)...R_(C-1). Exact rational ranks are:

```text
C=4: U3 has shape 54 x 26, rank 24.
C=5: U4 has shape 17120 x 355, rank 355.
C=5: U3 has shape 16150 x 355, rank 353.
```

At these small cases, exactly two final response relations survive through
the three-row boundary. No C=6 corank-two theorem is supplied.

Full rank does NOT exclude fast structured application. These results do
NOT prove an arithmetic lower bound for the actual input T_L or its final
scalar. A claim that all C=6 approaches are ruled out by this table is false.

### 5.3 A DIFFERENT PAIRED-HISTOGRAM OPERATOR AND ITS LIMITED BARRIERS

To define the other tested state space without external material: after k
steps, each labelled symbol i has subsets (S_i,T_i) of {1,...,C}, each of
size k. At the next step choose maps a_i not in S_i and b_i not in T_i,
using every color exactly twice in each map. Update by set union.

The bipartite multigraph with one edge a_i--b_i per symbol is 2-regular.
The transition weight is 2^(number of its connected components). Equivalently
use the union of the two perfect matchings defined by the equal-a and equal-b
pairs of symbols. Parallel-edge cycles count as components.

Group symbols into the histogram h[S,T], then quotient by independent
color permutations in the two copies and exchange of the copies. For a
fixed labelled source representative, summing weighted assignments into
target orbits defines the reduced orbit-total transition matrix Kbar_k.
This is NOT the native R_L above.

Its complete C=4 orbit layers are 1,5,141,5,1. Its C=5 middle map from
grade 2 to grade 3 has shape 38801 x 38801, with certified rational rank
at least 1024. This excludes a few-hundred-dimensional linear factorization
of that map. It neither proves full rank nor excludes a fast full-rank
transform or a method specific to the actual aggregate input.

At C=5, seven first-layer source orbits have 652001548 contingency leaves
in total. A cost-only residual-degree DP represents their LEAF COUNTS with
49890 memo states, but drops target identity and cycle weight. Restoring
those data causes six sources to exceed ten million states; the closed
source has 6323400 raw terminal states from 6516556 leaves. Do not confuse
compressing the number of assignments with computing their weighted outputs.

For a fixed fully labelled source s=((S_i,T_i)), let M(S) be the balanced
maps a avoiding S, with two symbols of each color. If B_S is the n-by-n
availability matrix with two identical slot columns per color, then

```text
|M(S)| = permanent(B_S)/2^C.
```

The complete labelled target determines both a and b by set difference.
All coefficients are positive, so there are exactly |M(S)| |M(T)| distinct
labelled terminal targets. Distinct extendable assignment prefixes have
disjoint completion-and-target supports, giving linearly independent rows
in the corresponding fixed-source prefix/suffix flattening.

A supplied reachable C=6 grade-2 witness is the following list, where
"16/46" means S={1,6}, T={4,6}:

```text
16/46 35/16 35/23 46/45 12/24 24/56
16/13 25/16 34/35 45/23 36/25 12/14
```

It has trivial stabilizer under the two color-permutation groups and copy
exchange, and

```text
per(B_S)=4743616; per(B_T)=4740096
|M(S)|=74119; |M(T)|=74064
terminal target support=5489549616
minimum three-color projected supports=6488 and 6503
every fixed 3+3 split has flattening rank >= 42191464.
```

This eliminates fixed-source linear frontiers returning the full labelled
target polynomial, including signed Ryser/Glynn reorderings with that output
semantics. It is not a lower bound for a method summing sources BEFORE
representing their individual target distributions, nor for N(6) directly.

### 5.4 Other ideas already considered

Ordinary 3+3 irreducible-channel truncation, unrestricted orbital-algebra
materialization, and generic PSD/Kraus factorizations have been considered.
The tested small-C 3+3 maps were full rank; the other proposals retained
large relative-placement or history-response spaces. These observations
are a warning against merely proposing the same mechanism again. Because
those alternative operators are not fully specified here, do NOT use this
paragraph as a theorem excluding a newly defined operator.

## 6. RESEARCH BRANCHES: CONSTRUCTIONS, NOT SLOGANS

Investigate at least two materially different mechanisms within the actual
available budget. You may reject the suggestions below in favor of a better
construction. Keep proofs and failures from different output contracts
separate.

### Branch A: an exact direct scalar contraction

With T_L a row vector, define

```text
D=diag(ell_q/m_q),
U_L=R_L...R_(C-1),
B_L=U_L D U_L^T,
N(C)=T_L B_L T_L^T.
```

This is only an EQUIVALENT RESTATEMENT until you can represent and evaluate
the relevant scalar cheaply. Explicit B_L or an equally large Gram factor
does not qualify. Independent squared source contributions omit cross terms.

Can an invariant response algebra, exact moment closure, coefficient
extraction, or a different two-copy recurrence avoid the native L4/L5
weights? Prove sufficiency for the scalar actually requested. Distinguish
closure for arbitrary inputs from closure only along the actual reachable
sequence T_1,T_2,... . Do not assume that an input-specific shortcut exists
merely because an arbitrary-input rank obstruction does not apply.

### Branch B: bulk weighted incidence or exact incoming evaluation

Can sum_x T_L(x)R_L(x,y) be evaluated without separately canonicalizing or
enumerating nearly every native emission? Possibilities include a genuinely
structured transform, grouped weighted incidences, or direct F_L evaluation.

Quantify the actual incoming work of a reversed recurrence. Transposing a
sparse matrix, using one canonical construction parent, or knowing that a
target already exists does not recover the contributions from all other
parents. Any eliminated histories require an exact replacement coefficient.

### Branch C: hidden structure in the missing-box geometry

At C=6,L=4, every symbol misses two boxes and every box is missed by four
symbols. The missing pairs form a loopless 4-regular multigraph on six
vertices with twelve symbol edges. Each edge carries side-choice data on
the four boxes it does not touch.

At L=5, each box is missed by exactly two symbols. That simpler structure
already gives the exact M5 count and a useful five-row canonicalization
anchor. It has not eliminated the weighted four-row layer.

Can the four-row graph and its decorations support a small exact weighted
model? Do not silently discard the decorations. Prove which correlations
are sufficient for the downstream quantity, or construct a small-C pair
with equal proposed summary but different required response.

For every native L-row state, the USED bipartite graph is L-regular and
the FREE next-row compatibility graph is 2(C-L)-regular. A permanent counts
one next row, while F_L counts an ordered full factorization. These are
different quantities. Native grouped fan can also differ from the labelled
permanent because equal masks are indistinguishable in emissions.

### Branch D: a bounded exact representation that makes emissions cheaper

This is secondary to reducing their number, but could still be useful.
A cheap invariant index, semicanonical buckets, or a stronger four-row
canonicalizer must account for bucket multiplicity, exact collision
resolution, construction and lookup cost, memory traffic, and both large
stages. Fingerprints alone are not exact equality or orbit certificates.

The current five-row anchor is already included in S2 timings. Do not claim
its benefit again. A four-row anchor has only preliminary prototype evidence,
not a verified production speedup. Candidate-search reductions are not wall
speedups until invariant computation and table work are charged.

## 7. FINITE CHECKS AND RESOURCE RULES

Start with the included C=2..4 baseline and exact normalization witness.
Do not spend most of the research budget rederiving the established baseline.

For each proposed mechanism:

1. State the smallest counterexample or dimension/support test that could
   falsify it before substantial implementation.
2. Run that finite test with deterministic parameters. Report raw candidates,
   filtered candidates, orbit counts, representation size, and exact output.
3. For an operator identity, compare exact coefficients or full small-case
   action. For a quotient, test invariance, separation and weighted response;
   matching only one final total is insufficient.
4. If it survives C=2..4, attempt a bounded C=5 decision gate. Before claiming
   a validated replacement, reproduce complete N(5), and complete class
   values if your method computes them. A scalar method is not required to
   construct a class vector solely for comparison.
5. Only then use bounded C=6 structural samples if they are informative.
   The supplied explicit witnesses can be used without any large layer.

Never start an unbounded C=6 layer enumeration. Initial exploratory runs
must declare caps; use at most 1,000,000 stored states, 5,000,000 emitted
records, ten minutes per probe and 4 GiB unless a smaller bound is needed
for your actual environment. These are maxima, not instructions to consume
them. If a guard cannot be enforced, keep the experiment small by explicit
finite bounds. Do not increase caps repeatedly to disguise a scale failure.

Do not run the deliberately slow reference below at C=5 or C=6. A larger
candidate computation needs a credible count/cost estimate before execution.
If computation is unavailable, say so and separate unexecuted code from
actual finite evidence. Do not invent output.

Finite observed equality is not a general theorem. Floating-point rank,
unseen-state estimators, and timing extrapolations are not exact certificates.
A nonzero minor modulo a prime lower-bounds rational rank, while low modular
rank alone does not upper-bound rational rank. Exact reconstruction requires
an established bound and enough modulus, not the historical N(6) decimal.

## 8. SELF-CONTAINED REFERENCE CODE

This Python 3 standard-library program constructs its own coordinate group,
uses full group scans for canonicalization, enumerates the recurrence above,
and prints complete C=2..4 summaries. It needs no files or external data.
All masks in the program use one bit per slot, bit 2*b+s; complete word
witnesses elsewhere use one bit per box and must be converted accordingly.

```python
from collections import Counter, defaultdict
from functools import lru_cache
from itertools import combinations, permutations
from math import comb, factorial, prod


def solve(C):
    if C not in (2, 3, 4):
        raise ValueError("This finite reference is restricted to C=2,3,4")
    n = 2 * C
    full = (1 << n) - 1
    actions = []
    for p in permutations(range(C)):
        for flips in range(1 << C):
            dest = [2 * p[b] + (s ^ ((flips >> b) & 1))
                    for b in range(C) for s in range(2)]
            table = [0] * (1 << n)
            for mask in range(1, 1 << n):
                bit = mask & -mask
                table[mask] = (table[mask ^ bit]
                               | (1 << dest[bit.bit_length() - 1]))
            actions.append(table)

    @lru_cache(maxsize=None)
    def canonical(x):
        return min(tuple(sorted(table[u] for u in x)) for table in actions)

    def children(x):
        types = sorted(Counter(x).items())

        def visit(k, remaining, acc):
            if k == len(types):
                assert remaining == 0
                yield tuple(sorted(acc))
                return
            u, count = types[k]
            available = [v for v in range(n)
                         if remaining & (1 << v)
                         and not (u & (3 << (2 * (v // 2))))]
            for slots in combinations(available, count):
                used = sum(1 << v for v in slots)
                added = tuple(u | (1 << v) for v in slots)
                yield from visit(k + 1, remaining ^ used, acc + added)

        yield from visit(0, full, ())

    T = {canonical(tuple(1 << v for v in range(n))): 1}
    sizes, emissions = [1], []
    for L in range(1, C):
        nxt = defaultdict(int)
        emitted = 0
        for x, tx in T.items():
            for z in children(x):
                emitted += 1
                coeff = prod(factorial(v) for v in Counter(z).values())
                nxt[canonical(z)] += tx * coeff
        T = dict(nxt)
        sizes.append(len(T))
        emissions.append(emitted)

    answer = mass = 0
    class_data = {}
    for q, tq in T.items():
        stab = sum(tuple(sorted(table[u] for u in q)) == q
                   for table in actions)
        assert len(actions) % stab == 0
        m = len(actions) // stab
        ell = factorial(n) // prod(factorial(v) for v in Counter(q).values())
        assert tq % m == 0
        F = tq // m
        class_data[q] = (m, ell, F)
        answer += m * ell * F * F
        mass += m * ell
    assert mass == comb(n, C) ** C
    if C == 2:
        assert set(class_data.values()) == {(2, 6, 4), (1, 24, 2)}
    expected = {2: 288, 3: 28200960, 4: 29136487207403520}
    assert answer == expected[C]
    canonical.cache_clear()
    return sizes, emissions, answer, mass, class_data


if __name__ == "__main__":
    for C in (2, 3, 4):
        sizes, emissions, answer, mass, classes = solve(C)
        print(f"C={C} layers={sizes} emissions={emissions} "
              f"classes={len(classes)} N={answer} weight_sum={mass}")
```

The embedded reference was executed for C=2,3,4 when preparing this prompt.
Its exact output was:

```text
C=2 layers=[1, 2] emissions=[4] classes=2 N=288 weight_sum=36
C=3 layers=[1, 5, 4] emissions=[80, 29] classes=4 N=28200960 weight_sum=8000
C=4 layers=[1, 23, 54, 26] emissions=[4752, 4630, 712] classes=26 N=29136487207403520 weight_sum=24010000
```

The returned class_data supplies every complete small-C orbit with (m,ell,F).
If your canonical convention differs, compare via the explicit coordinate
group, not by incompatible representative bytes. No C=5 certificate is
assumed available: construct your own baseline from section 2 if needed,
using an efficient implementation, or state that the C=5 gate is incomplete.

## 9. RED TEAM: TRY TO BREAK YOUR OWN IDEA

Before promoting any candidate, address these tests explicitly:

- Does it reproduce the two different C=2 weights and their sum 288?
- Does it preserve the square, including cross-source terms?
- Does it confuse orbit count, orbit size, symbol multiplicity, labelled
  matching count, native emissions, or ordered factorization count?
- Can two small-case states have the same proposed invariant but different
  completion responses? Exhibit a counterexample if they can.
- Does canonicalization depend on the construction parent? If so, prove
  that reaching the same child from another parent gives consistent keys.
- Does the representation shrink before the enormous frontier, or only
  after first generating it?
- Is the proposed rank barrier about the exact operator and output contract
  under discussion, or borrowed from a different one?
- Do claimed gains survive low stabilizers, the asymmetric G2 witness,
  raw-key differences, and production-size memory traffic?
- Have S1 and S2 both been costed? Has a 24-thread wall time been divided
  by the thread count a second time? Has preprocessing been hidden?
- Is the cost estimate for a cold start or for finishing saved work?
  Assume no access to the saved state in this environment; do not assume
  its partial weights can be read as closed values.
- Is a claimed lower bound unconditional, or only a bound for a particular
  representation, source-wise evaluation, or required explicit output?

## 10. DELIVERABLE, IN ORDER

1. **One honest status sentence first.** If no exact C=6-scale improvement
   has been established, say that in sentence one. Distinguish a proved
   identity, an executable small-case candidate, a performance-qualified
   algorithm, and a fully verified C=6 result.
2. **Baseline sanity checks.** Briefly state what finite checks you actually
   ran and their results. Reproducing supplied small cases is validation,
   not the main research result.
3. **Strongest result.** Give one or two precise constructions or negative
   results with labels (A)--(D). Explain exactly which operations disappear
   or become cheaper and which supplied barrier is avoided.
4. **BLOCKED LEMMAS.** State the unproved statements required by your best
   route, the exact obstruction, and the smallest decisive next test.
   Mark equivalent restatements as such. Do not list every abandoned idea.
5. **Finite evidence.** Include runnable code and actual exact output for
   every newly claimed finite result, with caps, seed/sample rule, counts,
   elapsed time and memory if measured. Separate unexecuted code clearly.
6. **Red-team outcome.** Report concrete failed tests or counterexamples;
   do not merely assert that the method passed an internal review.
7. **C=6 decision.** Give an auditable operation/storage budget and explain
   whether another 14--16 eight-hour calculation days can plausibly be
   reduced. If the evidence does not support that claim, say so.

## PERSISTENCE AND STOPPING

Do not stop at the first attractive identity or first failed branch.
Use the available reasoning and computation budget for at least two distinct
mechanisms, including attempts to falsify your preferred one. Small-C work
must serve the C=6 decision; it is not a substitute target.

Do not claim a number of hours of reasoning that you did not perform. Do
not sleep or run needless computations to simulate research duration. If
the environment limits time or tools, report that limitation and deliver
the strongest partial or negative result supported by actual work.

An honest unresolved lemma, a concrete counterexample, or a narrowly scoped
lower bound is preferable to an unsupported claim of a mathematical shortcut.
