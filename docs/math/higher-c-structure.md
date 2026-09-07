# Higher-C structure: exact interfaces and a rigorous scale boundary

Date: 2026-09-07. Independent, bounded research draft while the parent agent
finalizes N(6). No production checkpoint or active S3 file was read or changed.

The primary agent has reviewed the proofs. Retained runnable sources are
`experiments/proto/higher_c_structure_certificates.py` and
`experiments/proto/latin6_certificate.py`. They are copies of the research
sources used in the dated runs below, with no mathematical changes. The
independent first-moment implementation is retained as
`experiments/proto/higher_c_first_moment.cpp`, with the explicitly local GMP
compatibility header under `experiments/proto/linear_integer_compat/`.
These are bounded research/checking tools, not new production counting engines.

**No fast complete N(7), N(8), or N(9) algorithm is established here.**
The positive results are an all-C closed Latin-square family, an exact
small-separator response interface, and a first-moment validation identity.
The inventory result proves that extending the C6 all-native-value-table
architecture directly to C7 already requires trillions of penultimate states.
It is NOT a lower bound on every exact scalar algorithm.

Labels: **A** = self-contained proof below; **B** = external theorem (none is
needed for these derivations); **C** = finite computation, or explicitly
conditional/heuristic deployment claim. A proof label is not a novelty claim.
In particular the C4/C5 Latin-family equality was already observed in
`docs/expert/2026-07-27/nullspace-defect2-c5.md`; the result below proves the
same equality uniformly in C rather than presenting the observation as new.

## 1. Definitions

A native layer-L state is a multiset of 2C masks on 2C slots, the slots paired
into C boxes. Every symbol mask uses L different boxes, one slot in each;
every slot occurs in L masks. Giving equal masks temporary distinct symbol
labels produces an L-regular bipartite graph Q on two sets of size 2C.
F_L(Q) is the number of proper edge colorings by L specified colors, or
equivalently ordered decompositions into L perfect matchings.

The coordinate group has order g_C = 2^C C!. A native orbit has size m_x;
at the complete layer the symbol-label multiplier is
ell_x = (2C)! / product_u c_u!, and N(C) = sum_x m_x ell_x F_C(x)^2.
Changing box pairings can change native responses; no graph-value identity
below permits dropping those responses.

**A, reachability.** Every balanced native state is row-reachable. For a
d-regular bipartite graph, d|U| <= d|N(U)| for every subset U of one part.
If a maximum matching left a vertex unmatched, its alternating-reachability
set would have more left vertices than neighboring right vertices (otherwise
there would be an augmenting path), contradicting that inequality. A perfect
matching therefore exists. Delete it and repeat at degree d-1. This supplies
an ordered row decomposition, so the inventories below do not count
unreachable formal states.

## 2. All-C Latin-square family: an exact non-table primitive

Let L_C denote the number of Latin squares of order C with rows, columns,
and symbols all labeled. Let 0 <= t <= floor(C/2). Write binary complete
words, one bit per box, and consider

    x_(C,t) = { 0^(C-t), (2^C-1)^(C-t), 1^t, (2^C-2)^t }.

Here superscripts mean multiplicities, not exponentiation of the words.
The two distinct complementary word pairs differ in one coordinate.

### Theorem 2.1 — A, proved for every C >= 2

    F_C(x_(C,t)) = L_C^2 / binom(C,t).

**Proof.** Temporarily distinguish the 2C symbols. On the C-1 aligned boxes,
the symbols split into groups A and B of C symbols each. A uses only the
zero slots and B only the one slots. The induced graph of either group and
its C-1 slots is K_(C,C-1).

A proper C-edge-coloring of K_(C,C-1) gives a C by (C-1) array: each column
is a permutation of all colors and each row has distinct colors. Append
one artificial column containing each row's missing color. Each color was
used C-1 times in the old columns, so it is missing in exactly one row.
The new column is a permutation. Thus this operation is a bijection with
labeled Latin squares of order C. In particular each half has L_C colorings.

The missing-color vector on the C labeled symbols is a permutation. The
global color group acts transitively and freely on its possible values,
so every prescribed permutation occurs in exactly L_C / C! half-colorings.

At the remaining mixing box, one slot receives C-t symbols of A and t of B;
the other receives the remaining t and C-t symbols. Fix a coloring of A.
The missing colors on its first subset are a set S of size C-t. For the
mixing slot to contain every color once, B's corresponding t symbols must
receive exactly the complement of S. Of the C! missing-color permutations
on B, precisely t!(C-t)! meet this condition. The other mixing slot then
also has all colors. The number of B colorings compatible with this A
coloring is L_C t!(C-t)! / C! = L_C / binom(C,t). Multiply by L_C.
There is no row factorial or symbol-multiplicity factorial left to insert.

### Orbit weights and a closed contribution

**A.** At t=0, m=2^(C-1): all coordinate orders fix the pair of constant
words, and a global bit complement exchanges them. For C>=3 and 0<t<C/2,
the exceptional mixing box is identifiable, giving C choices, and the
orientation word is determined up to global complement, giving
m=C 2^(C-1). If C is even and t=C/2, flipping the mixing box also fixes the
multiset, so m=C 2^(C-2). The sole small exception is C=2,t=1, where all
four words occur once and m=1. Also

    ell = (2C)! / ((C-t)!^2 t!^2).

Consequently each such class contributes

    m ell F_C^2 = m binom(2C,C) L_C^4.

Summing this entire Hamming-distance-one family for C>=3 gives the exact
partial contribution

    2^(C-1) [1 + C(C-1)/2] binom(2C,C) L_C^4.

This is a certified sector of N(C), NOT the full count. It has only
floor(C/2)+1 classes and does not address the generic outer-class majority.
Computing its values is cheap once L_C is independently known; no claim
that arbitrary higher-order Latin-square counts are cheap is implied.

### C6 independent numerical anchors

**C, exact finite certificate.** `latin6_certificate.py` enumerated every
reduced Latin square at order 6 by a row/column bitmask backtrack, with the
first row and column fixed to 0,1,...,5. Every leaf was independently tested
for row/column permutations. No Sudoku counting code or catalogue was used.
There were exactly 9,408 reduced squares. The multiplier is 6! 5! = 86,400,
not (6!)^2: an arbitrary Latin square is normalized first by its unique
symbol permutation fixing the first row, then by its unique permutation of
the remaining rows fixing the first column. Thus L_6=812,851,200.

The deterministic enumeration SHA-256 (concatenated row-major byte encodings)
was `5F03649C3835253F2C28908E435B7993783F7C51ED6C35FF2B8B0FD0E61E8E21`.

| t | m | ell | F6, proved from independent L6 |
|--:|--:|--:|--:|
| 0 | 32 | 924 | 660727073341440000 |
| 1 | 192 | 33264 | 110121178890240000 |
| 2 | 192 | 207900 | 44048471556096000 |
| 3 | 96 | 369600 | 33036353667072000 |

Binary words are {0^(6-t),63^(6-t),1^t,62^t}. True two-slot masks are
{1365^(6-t),2730^(6-t),1366^t,2729^t}. Coordinate canonicalization may move
these representatives, so match complementary multiplicities and Hamming
distance, not their literal canonical string. Matching against the final
C6 CSV belongs to the parent agent; it has not been performed by this draft.

## 3. Exact separator interfaces instead of one scalar per graph summary

### Theorem 3.1 — A, one-sided tight-cut product

Let Q be a d-regular bipartite multigraph with distinguished parallel edges.
Partition its vertices into A and B. Suppose the cut has exactly d edges,
all from left vertices of A to right vertices of B, and
|A_left|=|A_right|+1. Form Q_A by replacing B with one new right vertex,
and Q_B by replacing A with one new left vertex. Retain the d cut-edge
identities, allowing parallel edges after contraction. Then

    F_d(Q) = F_d(Q_A) F_d(Q_B) / d!.

**Proof.** The new vertices have degree d and every old vertex keeps its
degree, so both pieces are d-regular and balanced. In a global coloring,
each color has one more left than right incidence inside A. Internal edges
cancel those incidences; with no oppositely directed cut edge, each color
occurs exactly once on the cut. Thus a global coloring induces a coloring
of each contracted graph, with agreeing cut-edge colors. Conversely such
an agreeing pair glues uniquely.

On either piece the d cut-edge colors form a permutation. Relabeling all
colors shows that each specified permutation occurs F_d(Q_A)/d! or
F_d(Q_B)/d! times, respectively. Sum the products over the d! permutations.
The factor is 1/d!, not 1/(d!)^2 and not 1. The construction needs parallel
edge identities if contraction creates parallel edges.

For ordinary disconnected components the corresponding formula is simply
the product of F values: the same labeled colors can be chosen independently
on different components. These statements preserve F only, not native box
responses.

### Theorem 3.2 — A, full boundary-partition response

For any vertex cut with k labeled cut edges, keep a half-edge at both ends
of every cut edge. For a partition pi of the k edge positions into b blocks,
let K_A(pi) be the number of proper d-edge-colorings of the A piece with
boundary edges assigned a fixed canonical choice of b distinct colors,
constant exactly on each block. Set it to zero if b>d or boundary constraints
are violated. Define K_B likewise. Global color relabeling makes the
particular canonical choice irrelevant. Then exactly

    F_d(Q) = sum_(pi, b(pi)<=d) (d)_(b(pi)) K_A(pi) K_B(pi),

where (d)_b = d(d-1)...(d-b+1).

**Proof.** Every full coloring supplies one boundary assignment phi. For each
fixed phi its two extensions are independent, contributing K_A(pi)K_B(pi),
where pi records equality of its colors. There are exactly (d)_b assignments
with partition pi. Sum over pi. The one-sided tight cut is the special case
where the sole feasible partition is the discrete one.

This is the needed logical distinction from the disproved scalar summaries:
the kernel retains the *entire* response to every feasible boundary color
relation. It does not assume that graphs with the same missing geometry,
permanent, or used-graph class have proportional native responses.

### Precise conditional algorithmic interface

**A, given-width bound.** Suppose a supplied binary decomposition of a graph
has at most w boundary half-edges per piece. Store at most Bell(w) responses
per piece. A naive exact join enumerates equality partitions on at most
3w positions (the two child interfaces and the parent interface), checks
shared-edge equalities, restricts to the two child partitions, and sums.
If b external blocks and j additional internal color blocks occur, their
color-allocation multiplier is (d-b)_j. Every global assignment has exactly
one such equality partition, so this join is exact.

For a decomposition with v pieces this gives the conservative upper bounds

    O(v poly(w) Bell(3w)) arithmetic operations;
    O(v Bell(w)) stored integers, or O(h Bell(w)) by depth-first evaluation
    with decomposition height h.

Integer bit lengths and their arithmetic cost must also be charged. Finding
a useful decomposition is separate. Canonical piece sharing requires exact
piece/boundary isomorphism keys, their canonicalization time, and retained
boundary transporters; it is not included free in these bounds. For a
single given labeled graph no graph canonicalization is needed.

The same proof applies directly to Sudoku as a constraint network: variables
are the 4C^2 cells, domains are [2C], and each row, column, and box has an
all-different factor. For a region, the response is indexed by the equality
partition of its exposed cell variables. This contracts N(C) as one scalar
without materializing the native F_(C-1) catalogue. Its formal exactness is
not a speedup: large all-different factors and boundary sets may make the
Bell-number profile enormous.

**C, blocked speedup lemma.** To turn this interface into a C7 route, one
must supply either (i) a decomposition whose *actual stored profiles* and
joins fit the budget, or (ii) a proved implicit representation and fast
application of the large profiles. Neither has been constructed. A
contraction-order plan alone, or a bare treewidth/Fourier reformulation,
does not meet this gate. Applying separator kernels separately after
enumerating trillions of native states would not solve the main scaling wall.

The Latin-square and tight-cut primitives are examples of exact large pieces
whose response can simplify algebraically. The productive theorem target is
a growing family of such *parameterized response kernels*, closed under
joins, with measured generic coverage. It is not another one-number graph
summary. The general C6 rank counterexamples remain respected.

## 4. Exact native inventory lower bounds at C7, C8, C9

### Theorem 4.1 — A, penultimate raw inventory

Let P_C count raw native states at L=C-1: symbols are unlabeled, but box and
side coordinates are labeled. Define

    f_C(t) = [x^(C-1)] (1+x)^(2(C-1-t)) (1+x^2)^t,  0<=t<=C-1.

Then

    P_C = 2^(-C) sum_(k=0)^C binom(C,k)
                       f_C(k-1)^k f_C(k)^(C-k),

omitting factors of exponent zero at k=0 and k=C.

**Proof.** Exactly two symbol masks miss each box. Temporarily label that
pair at every box; the group of independent pair swaps has size 2^C, and
its orbits are precisely the desired tuples of unordered pairs. For a
permutation action, the average number of fixed labeled objects equals the
number of orbits: each orbit contributes its size times stabilizer size,
namely the group order, to the double count of (object, fixing element).

Choose a swap element active in k of the C missing-box groups. At box q,
the other C-1 groups contribute two independent bits if unswapped, or one
bit repeated twice if swapped. If q itself is in the active set there are
k-1 swapped contributing groups; otherwise there are k. The coefficient
f_C(k-1) or f_C(k) counts choices with exactly C-1 one-side incidences at
q. Across different q the bits are independent. Thus the fixed count is
f_C(k-1)^k f_C(k)^(C-k). There are binom(C,k) such elements. Average.

Coordinate orbits have size at most g_C=2^C C!, giving the rigorous bound

    M_(C-1)(C) >= ceil(P_C / (2^C C!)).

No assumption of trivial stabilizers enters this inequality.

### Theorem 4.2 — A, complete raw inventory

Let A_C count raw complete native states. For a partition lambda of 2C, put
z_lambda=product_d d^(a_d) a_d!, and

    b_lambda = [x^C] product_(d in lambda) (1+x^d).

Then

    A_C = sum_(lambda partition of 2C) b_lambda^C / z_lambda,
    M_C(C) >= ceil(A_C / (2^C C!)).

**Proof.** Label all 2C symbols and average fixed counts over their full
permutation group. A fixed binary array is constant on each symbol cycle.
At one box, choosing cycles with total length C specifies exactly the C
symbols on side one; there are b_lambda choices. Choices in the C boxes
are independent, giving b_lambda^C. A cycle type lambda has (2C)!/z_lambda
elements, and averaging divides by (2C)!. The resulting orbits are exactly
the balanced multisets of binary words. Divide by the maximum coordinate
orbit size and round upward for the last assertion.

### Actual finite integer evaluations

**C, exact finite certificates, not N(C).** `finite_certificates.py` uses
integer numerators and checks every division. At C=9 the complete formula
has only 385 partition terms. The penultimate formula has only C+1 terms
after C short univariate coefficient extractions. It reads no large input.

| C | Complete raw A_C | Proved outer-orbit lower bound |
|--:|--:|--:|
| 6 | 2284361552 | 49574 |
| 7 | 95810590463936 | 148515921 |
| 8 | 46981500620124470512 | 4551624176522 |
| 9 | 281653362065380754593475328 | 1515939767371987397 |

| C | Penultimate raw P_C | Proved penultimate-orbit lower bound |
|--:|--:|--:|
| 6 | 4439972139072 | 96353563 |
| 7 | 4778447429705719808 | 7407067568369 |
| 8 | 77916645799614219084169216 | 7548658175960888971 |
| 9 | 19307504101453704460645263566072576 | 103918565223081367186667164 |

The actual audited C6 counts, 63,199 complete orbits and 96,452,755
penultimate orbits, exceed their lower bounds as required. The C7
penultimate lower bound alone would require 59,256,540,546,952 bytes
(over 59 decimal TB) for one eight-byte payload per state, before any keys,
indices, canonicalization, arithmetic or backups. At larger C even an
eight-byte exact value may not suffice. This is a bound on that explicit
table architecture only; recomputation/streaming may reduce simultaneous
storage, and a different scalar circuit is not lower-bounded by it.

The formulas take O(C^2 p(2C)) elementary integer coefficient work for A_C
and O(C^3) with the simple DP for P_C; only polynomial-length arrays are
stored. They count support but discard the factorization response, so
replacing the expensive counting problem by these cheap inventories would
be a category error.

## 5. Why bounded missing geometry is not itself sufficient

**A, explicit family.** Fix 1<=k<C. On boxes modulo C, take C distinct
missing sets e_i={i,i+1,...,i+k-1}. For each e_i choose an unordered
complementary pair of binary words on the C-k occupied boxes. There are
2^(C-k-1) choices per group. Every occupied box receives one zero and one
one from each such pair; each box belongs to exactly k missing sets.
Therefore every slot has degree C-k. The distinct missing sets identify
the groups, so all 2^[C(C-k-1)] choices give different raw native states.
Consequently this one fixed missing hypergraph supports at least

    ceil(2^[C(C-k-1)] / (2^C C!))

native orbits. At k=1 the missing geometry has no inter-box edges at all;
at k=2 it is a cycle with each missing edge doubled. Thus small missing
degree or small missing-graph treewidth does not bound the number of side
decorations. A kernel must compress their full responses, not merely count
or forget them. This statement complements, rather than duplicates, the
existing explicit same-summary/different-response counterexamples.

**A, obstruction to a tiny core from components/twins/tight cuts alone.**
For n=2C and 3<=d<=C, put both bipartition sets on Z_n and use edges
i~j iff j-i belongs to {0,...,d-1}. Pair slot j with j+C. The paired
neighborhoods are disjoint, so this is a valid native layer-d graph. All
left neighborhoods and all right neighborhoods are distinct cyclic
intervals. Consecutive left vertices share a right neighbor, so Q is
connected.

For a nonempty set S of cyclic positions, expanding by d-1 consecutive
steps grows it by sum_i min(g_i,d-1), where the g_i are the cyclic gaps
in its complement. If the expanded set is not the entire cycle, some gap
has length at least d, so growth is at least d-1>=2. If it is the entire
cycle and |S|<=n-2, growth is also at least two. Hence

    |N(S)| >= |S|+2 for 1<=|S|<=n-2.

A nontrivial one-sided tight cut would require a set of right vertices S
whose neighbors fit into |S|+1 left vertices, contradicting this inequality.
The remaining |S|=n-1 case merely isolates a single opposite vertex.
These graphs therefore have no twins, no disconnected split, and no
nontrivial cut of Theorem 3.1. At d=C-k they remain a growing 4C-vertex
core for every fixed k once C>=k+3. This does not rule out a different
algebraic primitive for circulant graphs; it falsifies the specific claim
that these elementary reductions alone yield a bounded universal core.

### Theorem 5.1 — A, terminal graph forgetting saves at most a factor two

At the **complete layer L=C only**, forgetting the box pairing but retaining
the two bipartition sides sends native outer orbits injectively to graph
isomorphism classes. If the bipartition sides are also forgotten, each
unpaired graph class contains at most two native outer orbits.

**Proof for fixed sides.** Every slot has a neighborhood of C symbols out
of 2C. Two slot neighborhoods are disjoint if and only if they are exact
complements. Thus an admissible pairing pairs the columns of neighborhood
type S bijectively with columns of type S-complement. Their multiplicities
must agree, and this condition is sufficient.

Any two such pairings are carried to one another by a permutation inside
each equal-neighborhood column class. Those permutations fix the incidence
graph pointwise on the symbol side. After the pairs are identified, their
order and orientation differ only by the native group 2^C C!. Therefore all
admissible pairings of the same side-distinguished graph yield the same
native orbit. A graph isomorphism transports this statement between
presentations, proving injectivity.

**Forgetting the sides.** A connected bipartite graph has exactly two choices
of which side is called the slot side: the choice at any vertex propagates
along every edge. Each orientation has at most one native orbit by the
previous paragraph. A disconnected C-regular simple bipartite graph on two
sets of size 2C has at most two components, since each component contains
at least C vertices of each side. If it has two, each component is exactly
K_(C,C). Independently swapping those component sides gives only the same
native orbit consisting of C copies of a word and C of its complement.
Thus the bound of two also covers the disconnected case.

**A, exact transposability test.** For a complete native word multiset,
Q-transpose admits a native pairing if and only if

    c_w = c_(w xor (2^C-1))  for every word w.

Indeed, a left-side pairing after transpose must also pair complementary
neighborhoods. A symbol's neighborhood has exactly one slot per box, and
its full slot-set complement is precisely its bitwise complementary word.
Equal multiplicities are both necessary and sufficient. Even when this
condition holds, the transpose may yield the same native orbit, so it does
not guarantee a factor-two saving.

For C7 the proved complete native lower bound 148,515,921 therefore leaves
at least 74,257,961 terminal graph classes under this exact unpaired-cache
mechanism. It cannot provide the very large compression needed by simple
classwise counting. This is a bound on graph-isomorphism caching, NOT a
bound on scalar contraction, accidental equality of F values, or a different
response representation. It does **not** apply to intermediate L4/L5 graphs;
the successful C6 F4 pairing-fiber sharing remains valid.

**C, complete C5 pairing test.** For all 355 native complete C5 states, an
independent disjoint-neighborhood pairing DFS enumerated all 699 admissible
pairings in total. Every pairing came with an explicitly checked
equal-neighborhood column permutation carrying it to the original native
pairing. All per-graph counts matched the product of neighborhood-class
multiplicity factorials. Exactly 39 of the 355 complete word multisets
were antipodally balanced. This is the transposability count, not a count
of distinct transpose-merged graph classes or numerical speedup.

## 6. First moment: reuse a rejected counting route as a useful invariant

### Theorem 6.1 — A, all-C identity

    sum_q m_q ell_q F_C(q)
       = 2^(C^2) [product_(r,a) x_(r,a)^2] per(X)^(2C).

**Proof.** Undo the coordinate and symbol quotients using m_q ell_q, so the
left side counts pairs (labeled balanced skeleton B, proper C-edge-coloring).
For each labeled symbol j, its colors at the C boxes form a permutation
sigma_j. At each box/color pair (r,a), exactly two symbols use color a,
one on either slot. Erasing slot bits gives an ordered collection of 2C
permutations with exactly two incidences in every cell (r,a). Conversely
choose independently which of those two symbols uses slot one in each of
the C^2 cells. This reconstructs the skeleton/coloring pair in 2^(C^2)
ways. The coefficient of per(X)^(2C) counts the permutation collections.

The parent agent has independently reconstructed and run the retained small
histogram recurrence, obtaining the C6 first moment

    4876139207527966044188061990912000.

The C5 value 1388348372994018508800 also matched the first moment of the
complete native 355-class CSV. Parent-side execution and final C6 CSV
comparison are separate evidence; this draft only proves the invariant
and supplied the inspected source path. A first moment does not determine
the required second moment. This is an independent numerical consistency
check, not an N7 extrapolation rule or a full F6 reevaluation.

## 7. Falsifiable next gates and blocked lemmas

1. **Response-kernel closure (C, open).** Give a precise parameterized family
   of pieces containing the Latin/tight-cut primitives, an exact canonical
   boundary representation, a join rule and complete operation/storage
   accounting. Prove closure for the proposed higher-C construction. Without
   this, 'reuse C6 pieces at C7' is a research direction, not a reduction.

2. **Generic coverage (C, unmeasured).** After N6 finalization, measure how many
   complete C6 classes and factorization work admit the primitives and how
   large the irreducible response profiles remain. Do not infer coverage from
   the four very symmetric Latin classes. The circulant family is a mandatory
   negative test. No production catalogue scan is authorized by this draft.

3. **Global application (C, blocked).** A good per-graph F evaluator does not
   eliminate the native lift or the trillion-state C7 support. A replacement
   needs to contract the actual squared objective before representing every
   native state. The global exposed-cell response interface of Section 3
   supplies a mathematically exact contract, but its sufficient profile size
   and fast joins remain to be demonstrated. An equivalent contraction with
   the same huge frontier is not progress.

4. **Small-C acceptance (C, future gate).** For any new interface, compare
   complete coefficient dictionaries on C=2..4, not just totals; compare
   every C5 final class or independently close the scalar with exact
   normalization and cross-source terms. Report profile dimensions, visited
   joins, canonicalization, boundary transporters, I/O and peak memory. Only
   then consider a bounded C6 test. Previous full-rank and no-quotient
   counterexamples must remain valid; an alleged rank collapse needs its own
   exact witness or proof.

5. **No exact sequence extrapolation.** The C6 catalogues provide strong test
   fixtures, not a recurrence in C. A fitted formula can be modified by a
   multiple of product_(j=2)^6 (C-j) without changing any known N(2)..N(6).
   Only an all-C identity with complete boundary data could justify exact
   extrapolation. Numerical correlation, graph-summary regression and
   approximate tensor truncation do not meet an exact-count target.

## 8. Executed finite certificates and scope

Commands, each with a fresh output directory:

```powershell
python scripts/run_guarded_step.py --seconds 30 --gib 1 `
  --output data/logs/c7-structure-research-20260907/finite-run-02 -- `
  python data/logs/c7-structure-research-20260907/finite_certificates.py

python scripts/run_guarded_step.py --seconds 30 --gib 1 `
  --output data/logs/c7-structure-research-20260907/latin6-run-01 -- `
  python data/logs/c7-structure-research-20260907/latin6_certificate.py
```

The guard refuses an existing output directory; use a new suffix for any
repeat. Each child also has a 25-second cooperative bound. No subprocess
survived. These are read-only mathematical experiments, not production runs.

Initial finite-run-01: PASS, 1.4536594 seconds, 47,550,464 aggregate sampled
RSS bytes. The extended finite-run-02 (the current script, adding terminal
pairing transporters and circulant tests): PASS, 0.8330468 seconds,
47,706,112 aggregate sampled RSS bytes. The two timings are not an ablation
or a performance claim. Latin6 guard: PASS, 0.2455343 seconds, 46,354,432
aggregate sampled RSS bytes.

The first script directly enumerated raw complete/penultimate native objects
at C=2..4 and matched the formulas:

    C=2: complete=3, penultimate=1
    C=3: complete=28, penultimate=40
    C=4: complete=1450, penultimate=12944.

It additionally matched the established C5/C6 penultimate raw masses
62,185,328 and 4,439,972,139,072. Independent small graph recurrences checked
all seven Latin-family values through C4. A complete 355-row C5 CSV was
checked for the known total, then the three relevant classes were tested
against the Latin formula and independently generated coordinate orbits.
That CSV is an old research fixture, not a new full numerical reevaluation;
its SHA-256 is
`89AB2D962CBC5E63BDF7177FD2F7F0CEEF547A5561919A96BD62B8BB966E6B46`.

Two K_(d,d) graphs spliced across a d-edge tight cut gave direct exact
F values 2, 24, 13,824 for d=2,3,4, matching L_d^2/d! in every case.
The independent graph recurrence used 150 memo entries. Integer arithmetic
was used throughout. The extended test checked all 3,584 proper nontrivial
right-side subsets of the six circulant cases C=3..5, 3<=d<=C: every one
had neighborhood expansion at least two, with legality, connectedness and
absence of repeated neighborhoods also checked. Terminal pairing results
are reported under Theorem 5.1.

Raw outputs and process-tree receipts are retained in all three run
directories. The draft adds no C6 production work, modifies no tracked file
and claims no independently closed N6. `git diff --check` passed at handoff;
the existing dirty/untracked user and parent-agent files were left alone.

## 9. Primary-agent reproduction and C6 data check

The retained Python certificates were independently rerun by the primary
agent under 30 seconds / 1 GiB each: PASS in 1.4533202 and 0.2627953 seconds,
with aggregate sampled peaks 48,128,000 and 46,333,952 bytes. Evidence is
`data/logs/c6-finalization-20260907/higher-c-retained/` and `latin6-retained/`.

The first-moment source builds without the ignored audit-directory header:

```powershell
g++ -O2 -std=c++20 -I experiments/proto/linear_integer_compat `
  experiments/proto/higher_c_first_moment.cpp `
  -o build/higher_c_first_moment.exe -lgmp

python scripts/run_guarded_step.py --seconds 30 --gib 1 `
  --output data/logs/my-linear-c6-check -- build/higher_c_first_moment.exe 6
```

GMP is the installed exact-integer library, not a counting catalogue. The
local compatibility header implements only the small integer API used by
this source; it is not a replacement for the general Boost library.
The new build passed and all C=2..6 runs completed within their 30-second /
1-GiB guards. The C2/C3/C5/C6 output was byte-identical to the earlier builds.
First moments at C=2..6 are respectively

    96
    460800
    828396011520
    1388348372994018508800
    4876139207527966044188061990912000.

The first sealed C6 final CSV has SHA-256
`84F2720E7BA8296D78604153B934B7291AF93C329C07F5D46538F9D7101127C7`.
The primary agent's separate `layer_final_moment_audit.py` checked all
63,199 rows against the first moment and the four Latin6 anchors, plus
positivity and divisibility by 6!. This passed in 0.8462208 seconds with
49,299,456 aggregate sampled bytes. The checker's C2 self-test accepted
one valid fixture and rejected 12 corruptions; its complete C5 check also
passed on the current native 355-class CSV, independently of the old fixture.
The full C6 two-run certification remains recorded separately in
`docs/reports/og2/c6-finalization-20260907.md`.

`layer_terminal_transpose_audit.py` constructs every admissible transpose,
using the independent Python semantic canonicalizer, and checks that the
resulting map on class IDs is an involution with equal F values. It does not
recompute the F values. At C5 it found 39 antipodal classes: 21 self-transpose
and nine exchanged pairs, leaving 346 unpaired graph classes. At C6 it found
388 antipodal classes: 96 self-transpose and 146 exchanged pairs, leaving
**63,053** unpaired graph classes out of 63,199 native classes. All F equality
checks passed. Their raw coordinate masses are independently
binom(20,5)=15,504 and binom(37,6)=2,324,784: an antipodal word multiset is
exactly a multiset of C complementary pairs chosen from 2^(C-1) types.
The C6 pass took 1.0416126 seconds and 95,547,392 aggregate sampled bytes
under 30 seconds / 1 GiB.

Thus the actual complete C6 data permits only 146 class eliminations by this
terminal graph-isomorphism cache. This is not a timing saving: different
classes can have very different costs. Logs and complete exchanged-pair IDs
are in `data/logs/c6-finalization-20260907/transpose-c5/` and
`transpose-c6-primary/`. No claim about arbitrary scalar compression follows.
