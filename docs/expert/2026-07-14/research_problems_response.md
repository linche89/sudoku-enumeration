# A concrete exact route: sparse three-pair contraction plus external-memory closure

I found one exact mathematical transform that appears genuinely useful against the reported (C=6) bottleneck, together with a bounded-memory implementation strategy. I have **not** computed (F_6(G_2)) or independently verified the announced value of (N(6)), so that numerical verification remains open. The proposal below does not use the announced value as input. It is designed around the supplied future-twin state, complementary row pairs, and benchmark measurements. 

The main idea is:

1. Process three complete complementary row pairs, externally closing the difficult (p=5\to6) frontier.
2. At that midpoint, replace all remaining future-twin layers by an exact sparse separator contraction.
3. For many outer graphs, store exact continuation values in level-synchronous, sorted tables instead of using a recursive random-access memo.

The first item is the only substantial external frontier that the proposed (G_2) run must close. The second item removes every lower frontier and every post-midpoint canonicalization call.

---

## 1. Exact three-pair tail transform

Assume that three complete complementary pairs have been processed. Six left rows remain, arranged as three complementary pairs, which I will call (q,r,t).

For each right column (x), let

[
K_x\subseteq[6]
]

be the three colors already used at (x), and put

[
A_x=[6]\setminus K_x,\qquad |A_x|=3.
]

The remaining neighborhood of (x) contains exactly one row from each of (q,r,t). Write the associated side bits as

[
b_q(x),b_r(x),b_t(x)\in{0,1}.
]

Choose one remaining pair, say (q), as a cut, and let

[
H_v={x:b_q(x)=v},\qquad v=0,1.
]

Each (H_v) contains exactly six columns.

### 1.1 The half kernel

A completion restricted to (H_v) assigns the three colors in (A_x) bijectively to the three remaining edges at (x). Write these assigned colors as

[
\alpha_x,\beta_x,\gamma_x
]

on the (q,r,t) edges respectively.

The (q)-row condition inside (H_v) is

[
{\alpha_x:x\in H_v}=[6].
]

For the four other remaining rows, record the partial color-use sets

[
\begin{aligned}
R_{r,0}&={\beta_x:x\in H_v,\ b_r(x)=0},\
R_{r,1}&={\beta_x:x\in H_v,\ b_r(x)=1},\
R_{t,0}&={\gamma_x:x\in H_v,\ b_t(x)=0},\
R_{t,1}&={\gamma_x:x\in H_v,\ b_t(x)=1}.
\end{aligned}
]

Any local assignment that repeats a color in one of these four partial rows is discarded. Define

[
\Gamma_{H_v}(R_{r,0},R_{r,1},R_{t,0},R_{t,1})
]

to be the number of surviving local assignments with that four-set signature.

For a signature (\sigma=(R_{r,0},R_{r,1},R_{t,0},R_{t,1})), define componentwise complement

[
\overline{\sigma}
=================

([6]\setminus R_{r,0},
[6]\setminus R_{r,1},
[6]\setminus R_{t,0},
[6]\setminus R_{t,1}).
]

Then the exact continuation value of the midpoint state is

[
\boxed{
V(s)=\sum_{\sigma}
\Gamma_{H_0}(\sigma),
\Gamma_{H_1}(\overline{\sigma}).
}
]

This is simply a sparse inner product after a complement permutation.

### 1.2 Correctness proof

Every global completion restricts uniquely to one local assignment on (H_0) and one on (H_1).

Within each half, the (q)-side row receives all six colors by the definition of (\Gamma). Consider one of the other four rows, for example the (r)-side-0 row. The columns incident with that row are partitioned between (H_0) and (H_1). It receives every color exactly once precisely when the color set contributed by (H_1) is the complement of that contributed by (H_0). The same condition holds independently for the other three rows.

Conversely, two local assignments with complementary four-set signatures glue uniquely to a proper coloring of all remaining edges. Right-column properness is already ensured because each local triple is a bijection onto (A_x).

Thus restriction and gluing give a weight-preserving bijection, proving the formula.

No canonical representative below the midpoint is involved in this proof.

---

## 2. Why this contraction is small

For one half (H_v), first assign the (q)-row colors (\alpha_x). This is a perfect matching in a (6\times6) zero-one matrix whose columns each contain exactly three ones.

If its row sums are (r_1,\ldots,r_6), then (\sum r_i=18). The Bregman–Minc permanent bound gives

[
\operatorname{per} A\leq
\prod_{i=1}^6 (r_i!)^{1/r_i}.
]

The right-hand side is maximized at (r_i=3), giving

[
\operatorname{per} A\leq (3!)^{6/3}=36.
]

The permanent bound and its zero-one-matrix formulation are classical. ([ir.cwi.nl][1])

After choosing the six (\alpha_x), each column has two remaining colors. They can be assigned to the (r)- and (t)-edges in at most (2^6=64) ways. Therefore

[
\sum_\sigma \Gamma_{H_v}(\sigma)\leq36\cdot64=2304,
]

and

[
V(s)\leq2304^2=5{,}308{,}416.
]

Consequences:

* Each signature is four six-bit masks, hence only 24 bits.
* Each half kernel has support at most (2304).
* A kernel count fits in 16 bits.
* The entire midpoint continuation value fits comfortably in 32 bits.
* Computing (V(s)) needs no graph canonicalization, no hash table of successor states, and no lower frontier.

The corresponding bounds for the required validation cases are:

| (C) | Pairs processed before the transform | Maximum (q)-row matchings per half | Maximum local assignments per half |
| --: | -----------------------------------: | ---------------------------------: | ---------------------------------: |
|   4 |                                    1 |                               (10) |                   (10\cdot2^4=160) |
|   5 |                                    2 |                               (19) |                   (19\cdot2^5=608) |
|   6 |                                    3 |                               (36) |                  (36\cdot2^6=2304) |

The formula works whenever exactly three complementary pairs remain.

### Why full rank is not an obstruction

The contraction can be written as

[
V(s)=\Gamma_{H_0}^{T}P\Gamma_{H_1},
]

where (P) is the permutation matrix taking a signature to its componentwise complement. In particular, (P) is full rank.

So this method does not discard an irreducible sector or posit a low-rank channel. It applies a full-rank operator implicitly to two sparse vectors. The supplied rank-(462/462) and (C=4,5) full-rank evidence therefore does not contradict it. It is analogous to the distinction between “a matrix has full rank” and “multiplication by the matrix requires materializing every entry.”

The cut is also different from the natural color (3+3) contraction: it is a cut through one complementary **row pair**, leaving four row-use masks as the exact separator data.

---

## 3. Prototype checks

I implemented the midpoint contraction and tested it in several ways.

For direct correctness checks:

* 50 random reachable (C=4) midpoint states matched a direct row-by-row tail recursion.
* 50 random reachable (C=5) midpoint states matched direct recursion.
* 20 random reachable (C=6) midpoint states based on the supplied (G_2) masks matched direct recursion.
* For every sampled (C=6) state, all three choices of the cut pair (q) produced the same continuation value.

I also generated 1,000 random valid prefixes through the first three supplied complementary pairs for each benchmark graph. These are actual reachable midpoint states, although they are not sampled according to the frontier weights.

The table reports the mean of

[
|\operatorname{supp}\Gamma_{H_0}|
+
|\operatorname{supp}\Gamma_{H_1}|
]

for each fixed cut:

| Graph | Cut pair 4 | Cut pair 5 | Cut pair 6 | Largest support sum seen |
| ----- | ---------: | ---------: | ---------: | -----------------------: |
| (G_1) |      172.5 |      176.6 |      168.9 |                      682 |
| (G_2) |      116.0 |      174.8 |      121.0 |                      671 |

The mean tail values in those samples were about (42.8) for (G_1) and (41.8) for (G_2). Zero-tail states also occurred, as expected for proper prefixes that cannot be completed.

The observed supports are far below the worst-case bound (2304). On these samples the midpoint continuation behaves like a sparse hundred-entry inner product, not another multimillion-state DP.

[Reference implementation of the three-pair tail kernel](sandbox:/mnt/data/three_pair_tail_kernel.py)

---

## 4. Closing the difficult (G_2) midpoint externally

The remaining obstacle is construction of the canonical frontier after six processed rows. The quoted (G_2) sequence is consistent with a pair-respecting order—one row, its mate, and so on—but this should be confirmed from the run metadata before applying the numerical estimates below. 

At an even layer, choose a complementary pair. Process one row and mark its mate as the mandatory next row. The odd-layer canonical state therefore has one distinguished remaining row. After that marked row is processed, remove the marker. This permits full canonicalization of all other row, color, and column symmetries without carrying permanent original row labels.

### 4.1 Compact exact keys

With (r) remaining rows, represent a canonical state as a multiset of twelve column records. A column record is a weight-six subset of the (r) remaining-row vertices and six color vertices, because

[
|\tau_j|+|K_j|=6.
]

There are

[
q_r=\binom{r+6}{6}
]

possible records. A nondecreasing sequence of twelve record ranks is a multiset and can be combinatorially ranked in

[
b_r=
\left\lceil
\log_2 \binom{q_r+11}{12}
\right\rceil
]

bits.

Relevant sizes are:

| Layer | Remaining rows |     Exact key size |
| ----- | -------------: | -----------------: |
| (p=5) |              7 | 101 bits, 13 bytes |
| (p=6) |              6 |  90 bits, 12 bytes |

At the three-pair boundary there is a still smaller specialized encoding. Each column is a pair

[
(b,K)\in{0,1}^3\times\binom{[6]}3,
]

so there are only (8\cdot20=160) types. A multiset of twelve such types needs

[
\left\lceil\log_2\binom{171}{12}\right\rceil=60
]

bits. Thus a midpoint state key can fit in eight bytes after the pair structure has been canonically recovered.

A practical disk record is therefore either

* 8–16 bytes of key plus a 16-byte unsigned weight, or
* a fixed 32-byte aligned record for faster radix sorting.

Canonical labeling must remain exact; cheap invariants may partition or accelerate calls but may never be used as equality tests. Colored canonical labeling provides precisely the required invariant representative, and automorphism generators can be returned at the same time. ([Pallini][2])

### 4.2 Deterministic sort-reduce pipeline

For each parent chunk:

1. Decode a sorted block of canonical parent states and exact weights.
2. Generate grouped transition leaves.
3. Use the stabilizer of the selected row to combine assignment orbits when available.
4. Canonicalize one representative of each surviving orbit.
5. Append fixed-size `(child key, weight increment)` records to a bounded buffer.
6. Radix-sort the buffer, reduce equal keys locally, and write a sorted run.

After every parent chunk is closed, merge the runs by full key and sum weights in unsigned 128-bit arithmetic.

This is delayed duplicate detection specialized to an exact weighted DP. Sequential run generation and external merge are the established way to avoid random disk probes; hybrid schemes that combine duplicates in RAM before writing sorted runs have also been used successfully in large layered searches. ([AAAI][3])

A generic disk key-value store is unnecessary and would introduce precisely the random-access behavior this design avoids.

### 4.3 Assignment-orbit reduction

Let (H=\operatorname{Aut}(s,u)) be the stabilizer of the selected row in a parent state. It acts on the grouped color assignments for that row.

Assignments in one (H)-orbit have:

* isomorphic child states;
* the same grouped transition multiplier;
* orbit size computable from the generators.

Therefore only one child needs canonicalization per assignment orbit, with emitted multiplicity multiplied by the orbit size. Different orbits may still canonicalize to the same child; the ordinary sort-reduce stage combines those.

This costs little because a row has at most (6!=720) labeled assignments. It may not help low-symmetry middle states, but it substantially reduces early and symmetric cases and is exact.

### 4.4 Transaction and restart rules

Each parent chunk receives an immutable chunk ID. A chunk is complete only after its run files, record counts, hashes, and mass audits have been written and synchronized. A layer manifest names exactly one completed generation for every parent chunk.

The following conditions should be mandatory:

* Temporary files are never inputs to a later layer.
* A layer is usable only after an atomic `COMMITTED` manifest references every shard.
* The manifest records the parent-layer manifest hash, preventing accidental mixing of runs.
* Parent index intervals must be disjoint and cover the full parent file.
* Every reduced output shard must contain strictly increasing keys.
* A graph result is returned only from a complete chain of committed manifests.

A particularly useful exact audit is

[
\sum_{\text{emitted records}}\Delta w
=====================================

\sum_{\text{parents }s}
w(s),
\bigl(\text{total legal transition multiplicity from }s\bigr).
]

Compute this both in a wider accumulator and modulo two or three independent audit primes. It catches lost chunks, duplicate chunks, and truncation before the final merge.

---

## 5. Quantitative (G_2) estimate

These estimates are conditioned on the quoted (7{,}630{,}873)-state layer being the layer after two full pairs and the first row of the third pair. That should be checked first.

For the mate row, every incident column then has four available colors. A (6\times6) list matrix with column degree four has at most

[
\left\lfloor(4!)^{6/4}\right\rfloor=117
]

perfect matchings. For independent random four-of-six lists, the elementary expectation is

[
6!\left(\frac46\right)^6\approx63.21.
]

Thus:

| Quantity                                |                                     Projection |
| --------------------------------------- | ---------------------------------------------: |
| Parent states                           |                                (7.631) million |
| Raw child records, random-list estimate |                            about (482) million |
| Absolute list-permanent bound           |                            below (893) million |
| Distinct midpoint states                |                approximately (17)–(29) million |
| Packed committed frontier               |                  approximately (0.4)–(0.9) GiB |
| Sequential temporary SSD traffic        |                    approximately (20)–(60) GiB |
| External merge passes                   | normally one; two only with very small buffers |
| Peak RAM                                |                                (2.0)–(2.8) GiB |

The distinct-state estimate is a deliberately broad extrapolation from the reported (7{,}981{,}359) distinct keys after two million parents; it is not a theorem. The implementation should log distinct growth every (100{,}000) or (250{,}000) parents and replace this estimate immediately.

The (G_1) benchmark processed (351{,}628{,}403) grouped transition leaves in (1592.867) seconds, about (221{,}000) leaves per second end-to-end. At that rate, (482) million leaves correspond to roughly 36 minutes. Allowing a factor of two to five for lower symmetry, harder canonicalization, and external sorting suggests roughly:

* (1.2)–(3) hours for the difficult midpoint transition;
* (0.5)–(3) hours for the midpoint tail scan;
* approximately (2)–(7) hours in total on a machine in the same performance class.

The uncertainty is now primarily empirical throughput, not whether RAM can hold the layer. The midpoint tail scan has no output frontier: each committed state is decoded, its exact continuation is evaluated, and

[
w(s)V(s)
]

is accumulated into a checked 128-bit graph total.

---

## 6. Exact cross-graph continuation sharing

The same midpoint oracle gives a clean base case for a global continuation database.

For each depth (p\leq6), maintain a sorted immutable table

[
D_p:\quad \text{canonical state key}\longmapsto V(s).
]

At (p=6), missing values are computed by the three-pair transform.

For a batch of requested states at depth (p<6), use the following level-synchronous closure:

```text
sort and deduplicate requested keys
merge-join against D_p
expand only the misses
sort/reduce all child requests at depth p+1
close those child requests as one batch
merge-join parent-child edges with D_(p+1)
reduce by parent to obtain V(parent)
transactionally merge new values into D_p
```

Only the current batch’s edge tape is needed. Once new parent values are committed, its temporary child edges can be deleted.

This differs materially from the failed recursive global memo:

* no random lookup per recursive call;
* no partially evaluated state accepted as closed;
* every depth is deduplicated before expansion;
* all database probes are sorted merge-joins;
* each exact state is expanded once globally after it becomes a cache miss;
* interruption leaves either an old committed table or a fully new one.

Starting with all (63{,}199) roots produces a value for every root state. The root identities never need to be propagated through the DAG. After closure, each root independently retrieves its scalar (F_6(Q_h)), and only then is

[
m(h)F_6(Q_h)^2
]

formed. Therefore no invalid squaring of a summed root vector occurs.

### 6.1 Why processing complete pairs should improve sharing

After complete complementary pairs are deleted, the uncolored residual skeleton is just a balanced histogram on the remaining suffix bits.

I enumerated these small projected histogram spaces exactly:

| Remaining full pairs (r) | Labeled balanced histograms | (W_r)-orbits |
| -----------------------: | --------------------------: | -----------: |
|                        1 |                           1 |            1 |
|                        2 |                           7 |            4 |
|                        3 |                         176 |           17 |
|                        4 |                      17,308 |          137 |

So after three complete pairs have been processed, all (63{,}199) initial uncolored skeletons project into at most 17 uncolored suffix classes. This does **not** prove that the fully colored future-twin states have high overlap, because their (K)-assignments still matter. It does give a much stronger reason to expect cross-instance reuse than an arbitrary row order.

[Exact projection-orbit enumerator](sandbox:/mnt/data/balanced_projection_orbits.py)

The required measurements should report, at every depth,

[
R_p=
\frac{\sum_h |\mathcal S_{p,h}|}
{\left|\bigcup_h\mathcal S_{p,h}\right|},
]

together with cache-hit rate, child-edge count, and canonicalization calls saved.

### 6.2 Half-kernel sharing

A midpoint half (H) consists of six records

[
(d_1,d_2,A),\qquad
(d_1,d_2)\in{0,1}^2,\quad A\in\binom{[6]}3.
]

There are (4\cdot20=80) possible record types. A multiset of six such records has one of

[
\binom{85}{6}=437{,}353{,}560<2^{29}
]

keys. Thus an exact on-demand half-kernel cache can use a 32-bit key.

The half kernel is independent of:

* the original outer histogram;
* the processed history;
* the other half of the state.

This is a finer-grained cross-graph sharing opportunity than caching only complete midpoint states. It should be enabled after measuring repetition rates; precomputing all (437) million possible halves is unnecessary.

---

## 7. A genuine tail lumping quotient

There is also a precise, locally computable weighted-bisimulation quotient for the last two pairs.

With two pairs remaining, every column has two available colors. Processing one pair requires at most eight assignments per half, hence at most (64) paired assignments. Each such outcome leaves the final pair forced and is either:

* successful, or
* locally dead.

For a two-pair state (s), define

[
q_2(s)=(a_s,b_s),
]

where (a_s) is the total transition multiplicity to successful forced states and (b_s) the total multiplicity to dead states. Since

[
a_s+b_s\leq64,
]

there are at most

[
\binom{66}{2}=2145
]

possible quotient values.

States with equal (q_2) have identical transition weight into every target quotient class, so this is weighted strong lumpability, not merely equality of the unknown scalar (V(s)).

With three pairs remaining, define the quotient signature to be the sparse histogram of pair-transition multiplicities by (q_2)-class. Equality of these signatures is again strong lumpability. This is the ordinary backward weighted-bisimulation construction specialized to the pair process. Weighted-automaton minimization by transition-signature refinement is a standard exact construction. ([arXiv][4])

I would treat this quotient as an instrumented optimization, not as a prerequisite for the (G_2) run:

* compute its merge ratio on every (C=4,5) midpoint state;
* measure it on the (G_1) midpoint;
* retain it only if it produces a substantial reduction.

The sparse separator transform already avoids the lower frontiers even if this quotient merges nothing.

---

## 8. Arithmetic and final global aggregation

For one graph, all prefix weights and all products (w(s)V(s)) fit in unsigned 128-bit arithmetic because

[
F_6(G)\leq720^{12}<2^{114}.
]

Every multiplication and addition should nevertheless be checked against the (2^{114}) bound.

For the global total, an independent bound is

[
N(6)
\leq
\binom{12}{6}^{6},720^{24}
==========================

924^6,720^{24}
<
2^{287}.
]

Therefore either:

* use an arbitrary-precision integer for the final sum, or
* use five suitably chosen near-64-bit CRT primes whose product exceeds (2^{287}), followed by a sixth independent checksum prime that is not used in reconstruction.

The outer contribution must be formed as

[
m(h),V(s_h)^2
]

for each root separately.

---

## 9. Required validation order

The implementation should be accepted only through the following sequence.

1. **Tail differential tests.** Compare the separator contraction with labeled direct recursion for random and exhaustive small states.

2. **Complete (C=4) and (C=5) runs.** Reproduce every outer-class value, not only (N(4)) and (N(5)). Record midpoint kernel support distributions and any bisimulation merges.

3. **(G_1).** Using the pair-respecting prefix and midpoint transform, reproduce

   [
   F_6(G_1)=6986348258918400.
   ]

   This checks the transition multipliers, state canon, external aggregation, and tail contraction together. 

4. **(G_2) midpoint pilot.** Close at least the first (250{,}000), then (2{,}000{,}000), parents transactionally. Measure actual assignment leaves per parent, local reduction ratio, canonicalizations per second, SSD traffic, and distinct-key growth.

5. **Full (G_2).** Close the committed midpoint, scan it through the tail oracle, and perform an independent modular rerun or a second implementation of the midpoint tail.

6. **Cross-graph experiment.** First complete all (C=5) roots through the shared continuation database. Then measure exact overlap for (G_1), (G_2), and a stratified (C=6) sample before deciding on all (63{,}199) roots.

---

## Assessment

The strongest part of this proposal is the exact identity

[
V(s)=\sum_\sigma
\Gamma_{H_0}(\sigma)\Gamma_{H_1}(\bar\sigma).
]

It turns the entire six-row suffix into two sparse kernels of rigorously bounded size. It explicitly survives the full-rank objections because its joining operator is itself a full-rank permutation.

Combined with a packed external sort-reduce pass, it gives a credible bounded-memory route to (G_2):

* close only the hard midpoint;
* retain roughly (0.4)–(0.9) GiB of committed midpoint records;
* perform mostly sequential SSD I/O;
* eliminate every lower canonical frontier;
* evaluate the suffix independently and exactly;
* reuse exact continuation values across outer graphs through sorted level-synchronous tables.

The main unresolved empirical question is how large the pair-respecting (p=6) frontier actually becomes and how quickly the existing canonicalizer handles its low-symmetry children. The mathematical suffix problem itself no longer requires a large frontier.

[1]: https://ir.cwi.nl/pub/9892/9892D.pdf "https://ir.cwi.nl/pub/9892/9892D.pdf"
[2]: https://pallini.di.uniroma1.it/nug29.pdf "https://pallini.di.uniroma1.it/nug29.pdf"
[3]: https://cdn.aaai.org/AAAI/2004/AAAI04-103.pdf "https://cdn.aaai.org/AAAI/2004/AAAI04-103.pdf"
[4]: https://arxiv.org/abs/2112.09387?utm_source=chatgpt.com "Morphisms and minimisation of weighted automata"
