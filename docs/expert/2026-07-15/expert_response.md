# I. Executive conclusion

## Result

For the **linear objective exactly as written**

[
N_{\mathrm{lin}}(6)=\sum_{[G]}w([G])F_6(G),
]

the best-supported conclusion is:

[
\boxed{\textbf{5. An explicit global contraction has been derived.}}
]

The 63,199-class loop can be removed completely. No cross-class collision rate is needed.

The decisive mechanism is to **sum over the binary outer skeleton before constructing any colored frontier**. Once this is done, a fixed-skeleton colored state loses its future-neighborhood labels, and the previously insufficient color-subset occupancy histogram becomes an exact continuation state.

Under the natural outer-orbit weight convention

[
w([h])
======

|\operatorname{Orb}*{W_6}(h)|
\frac{12!}{\prod*{p\in{0,1}^6}h_p!},
\qquad
W_6=(\mathbb Z/2\mathbb Z)^6\rtimes S_6,
]

the exact answer is

[
\boxed{
N_{\mathrm{lin}}(6)
===================

4,876,139,207,527,966,044,188,061,990,912,000.
}
]

The global dynamic program has only

[
1,\ 1,\ 24,\ 132,\ 24,\ 1,\ 1
]

symmetry-reduced states at its seven levels. Its complete transition DAG has 184 nodes and 4,754 nonzero arcs. A single-threaded exact C++ implementation completed in 0.75 seconds with 14,340 kB maximum resident memory.

## Critical scope distinction

The supplied project material also contains the different, historically relevant objective

[
\sum_{[G]}w([G])F_6(G)^2.
]

The construction below **does not solve the squared objective**. In the squared problem two colorings must share the same outer skeleton, and the local skeleton sum remains coupled. Section IV gives the strongest exact paired contraction I can derive and identifies its remaining bottleneck.

The only external audit needed for the linear result is:

[
\boxed{
w([G])
======

#{\text{labeled balanced skeletons represented by }[G]}.
}
]

For the histogram convention producing the 63,199 classes, this is exactly the displayed (W_6)-orbit formula.

---

# II. Strongest concrete construction

## II.1 Unrolling the outer classes

For general (C), let

[
\mathcal B_C=
\left{
B=(b_{rj})\in{0,1}^{C\times 2C}:
\sum_{j=1}^{2C}b_{rj}=C\quad\forall r
\right}.
]

The graph (Q_B) has left vertices

[
(r,\varepsilon),\qquad r\in[C],\ \varepsilon\in{0,1},
]

right vertices (j\in[2C]), and edges

[
j\sim (r,b_{rj}).
]

If (\pi(B)) denotes the outer class containing (B), then the natural class weight satisfies

[
w([G])=\left|\pi^{-1}([G])\right|.
]

Consequently,

[
\sum_{[G]}w([G])F_C(G)
======================

\sum_{B\in\mathcal B_C}F_C(Q_B).
\tag{1}
]

For the histogram representative (h=(h_p)_{p\in{0,1}^C}),

[
w([h])
======

\frac{2^C C!}{|\operatorname{Stab}_{W_C}(h)|}
\frac{(2C)!}{\prod_p h_p!}.
\tag{2}
]

The mandatory weight-table checks at (C=6) are therefore

[
w_{\mathrm{stored}}([h])=w_{\mathrm{formula}}([h])
\quad\text{for every one of the 63,199 rows},
]

and

[
\sum_{[h]}w([h])
================

# \binom{12}{6}^{6}

622,345,892,187,672,576.
\tag{3}
]

This step handles all outer stabilizers and repeated canonical representatives exactly.

---

## II.2 Skeleton-erasure bijection

Fix a labeled balanced skeleton (B) and a proper edge-coloring of (Q_B) with labeled colors ([C]).

For every right vertex (j), define

[
\sigma_j(r)
===========

\text{color of the edge }j!-!(r,b_{rj}).
]

Because all (C) colors occur once at the degree-(C) right vertex (j),

[
\sigma_j\in S_C.
]

At a left vertex ((r,\varepsilon)), all colors also occur once. Therefore, for every cell ((r,a)),

[
#{j:\sigma_j(r)=a,\ b_{rj}=0}=1,
]

and

[
#{j:\sigma_j(r)=a,\ b_{rj}=1}=1.
]

After forgetting the bits (b_{rj}), this says

[
#{j:\sigma_j(r)=a}=2
\qquad
\forall r,a\in[C].
\tag{4}
]

Conversely, suppose that

[
(\sigma_1,\ldots,\sigma_{2C})\in S_C^{2C}
]

satisfies (4). For each of the (C^2) pairs ((r,a)), there are exactly two symbols (j) with (\sigma_j(r)=a). Choose one to receive (b_{rj}=1), and give the other (b_{rj}=0).

These (C^2) choices are independent, giving exactly

[
2^{C^2}
]

balanced skeleton/coloring pairs for each admissible ordered permutation array.

Hence, with (P_\sigma) denoting the permutation matrix of (\sigma),

[
A_C
===

#\left{
(\sigma_1,\ldots,\sigma_{2C})\in S_C^{2C}:
\sum_{j=1}^{2C}P_{\sigma_j}=2J_C
\right},
\tag{5}
]

and

[
\boxed{
N_{\mathrm{lin}}(C)=2^{C^2}A_C.
}
\tag{6}
]

Equivalently, (N_{\mathrm{lin}}(C)) is the number of ordered decompositions of the edge-labeled multigraph (2K_{C,C}) into (2C) labeled perfect matchings. The two parallel edges in each of its (C^2) cells are distinguished by the (0/1) orientation.

This is the identity that removes the outer classes.

---

## II.3 Equivalent global row operator

The same mechanism can be stated directly at the transfer-operator level.

After (r) row pairs, a labeled partial state is

[
x=(S_1,\ldots,S_{2C}),
\qquad
S_j\in\binom{[C]}r,
]

where (S_j) is the set of colors already used by symbol (j). Necessarily,

[
#{j:a\in S_j}=2r
\qquad\forall a\in[C].
\tag{7}
]

For a fixed row skeleton (A\subset[2C]), (|A|=C), let (M_{A,r}) be the usual row-pair transition: the symbols in (A) receive a bijection to the (C) colors, as do the symbols in (A^c), subject to not reusing a color already in (S_j).

Define the skeleton-summed operator

[
L_r=\sum_{\substack{A\subset[2C]\|A|=C}}M_{A,r}.
]

For a fixed assignment (a_j\notin S_j), the assignment can arise from a skeleton (A) exactly when each color occurs twice among the (a_j). For every color, one of its two occurrences must lie in (A), giving two choices independently. Thus

[
\boxed{
L_r=2^C R_r,
}
\tag{8}
]

where

[
R_r e_x
=======

\sum_{\substack{
a_j\notin S_j\
|{j:a_j=a}|=2\ \forall a
}}
e_{(S_1\cup{a_1},\ldots,S_{2C}\cup{a_{2C}})}.
\tag{9}
]

Therefore

[
N_{\mathrm{lin}}(C)
===================

2^{C^2}
\left\langle f\middle|
R_{C-1}\cdots R_0
\middle|e\right\rangle.
\tag{10}
]

This proves explicitly why the fixed-graph future-neighborhood information disappears: the row skeleton has already been summed in (L_r).

---

## II.4 Exact quotient state and canonical map

For (x=(S_1,\ldots,S_{2C})), define its subset histogram

[
h_x(S)=#{j:S_j=S},
\qquad S\in\binom{[C]}r.
]

The admissible histogram space is

[
\mathcal H_r(C)=
\left{
h:
\begin{array}{l}
h_S\in\mathbb Z_{\ge0},[2mm]
\displaystyle\sum_Sh_S=2C,[2mm]
\displaystyle\sum_{S\ni a}h_S=2r\quad\forall a\in[C]
\end{array}
\right}.
\tag{11}
]

The group

[
H=S_{2C}^{\mathrm{symbols}}\times S_C^{\mathrm{colors}}
]

acts on labeled states by

[
(\tau,\pi)\cdot(S_1,\ldots,S_{2C})
==================================

(\pi S_{\tau^{-1}(1)},\ldots,\pi S_{\tau^{-1}(2C)}).
\tag{12}
]

Symbol permutations are completely represented by (h). The remaining canonicalization is

[
\operatorname{can}_r(h)
=======================

\min_{\pi\in S_C}
\left(h_{\pi^{-1}S}\right)_{S\in\binom{[C]}r},
\tag{13}
]

using lexicographic order on bit-mask-indexed subsets.

The orbit size of a histogram state is

[
|\mathcal O(h)|
===============

\frac{C!}{|\operatorname{Stab}_{S_C}(h)|}
\frac{(2C)!}{\prod_Sh_S!}.
\tag{14}
]

The implementation does not divide by this number. It stores orbit totals, which keeps every transition integral.

### Exact continuation invariance

Let

[
\operatorname{Cont}_r(x)
========================

\left\langle f\middle|
R_{C-1}\cdots R_r
\middle|x\right\rangle.
]

The operators (R_r) are (H)-equivariant. Therefore

[
y=gx
\quad\Longrightarrow\quad
\operatorname{Cont}_r(y)=\operatorname{Cont}_r(x).
\tag{15}
]

Thus

[
\operatorname{can}_r(h_x)=\operatorname{can}_r(h_y)
\quad\Longrightarrow\quad
\operatorname{Cont}_r(x)=\operatorname{Cont}_r(y).
]

This is the required exact quotient-validity theorem. It is not inferred from the 17 uncolored suffix classes.

The group projector onto the relevant invariant space is

[
P_r
===

\frac{1}{(2C)!,C!}
\sum_{\tau\in S_{2C}}
\sum_{\pi\in S_C}
\rho_r(\tau,\pi).
\tag{16}
]

Only the trivial (H)-isotypic component is reached from the initial state. Its multiplicity dimension is exactly the number of histogram orbits.

---

## II.5 Exact transition coefficients

Fix (h\in\mathcal H_r(C)). A transition can be represented by a bounded contingency table

[
n=(n_{S,a}),
\qquad
S\in\binom{[C]}r,\ a\in[C],
]

satisfying

[
n_{S,a}=0\quad\text{if }a\in S,
\tag{17}
]

[
\sum_{a\notin S}n_{S,a}=h_S,
\tag{18}
]

and

[
\sum_Sn_{S,a}=2
\qquad\forall a\in[C].
\tag{19}
]

Its target histogram is

[
\Phi(n)_T
=========

\sum_{\substack{S,a\S\cup{a}=T}}n_{S,a}.
\tag{20}
]

For a fixed labeled source state with histogram (h), the number of assignments realizing (n) is

[
\mu(n)
======

\prod_S
\binom{h_S}{(n_{S,a})_{a\notin S}}
==================================

\prod_S
\frac{h_S!}{\prod_{a\notin S}n_{S,a}!}.
\tag{21}
]

Thus the exact labeled transition is

[
T_r(h,k)
========

\sum_{\substack{n\text{ satisfying }(17)-(19)\
\Phi(n)=k}}
\mu(n).
\tag{22}
]

Equivalently, it is the coefficient

[
T_r(h,k)
========

\left[
x_1^2\cdots x_C^2
\prod_Ty_T^{k_T}
\right]
\prod_{S\in\binom{[C]}r}
\left(
\sum_{a\notin S}x_a y_{S\cup{a}}
\right)^{h_S}.
\tag{23}
]

For source and target color orbits (O,O'), choose any representative (h\in O) and define

[
t_r(O,O')
=========

\sum_{k\in O'}T_r(h,k).
\tag{24}
]

Equivariance proves that this does not depend on the chosen representative.

Let (G_r(O)) be the total coefficient of all labeled partial states in orbit (O). Then

[
G_0(O_0)=1,
]

and

[
\boxed{
G_{r+1}(O')
===========

\sum_O G_r(O)t_r(O,O').
}
\tag{25}
]

At level (C) there is one terminal orbit, and

[
A_C=G_C(O_C).
\tag{26}
]

Equations (17)–(25) are the complete sparse matrix-generation rule. No outer class or hidden fixed-graph midpoint state occurs.

---

## II.6 Pseudocode

```text
G := { canonical histogram h_empty : 1 }

for r = 0,1,...,C-1:
    Next := empty integer map

    for each source orbit representative h with coefficient g = G[h]:
        OrbitTransition := empty integer map

        enumerate all bounded tables n satisfying:
            n[S,a] = 0 when a in S
            sum_a n[S,a] = h[S]
            sum_S n[S,a] = 2

        for each such n:
            k := Phi(n)
            multiplicity := product_S h[S]! / product_(S,a) n[S,a]!
            OrbitTransition[can(k)] += multiplicity

        for each target orbit k with multiplier m:
            Next[k] += g * m

    G := Next

A_C := coefficient of the unique terminal histogram
return 2^(C*C) * A_C
```

The implementation enumerates distributions among identical source types (S), not the

[
\frac{12!}{(2!)^6}=7,484,400
]

individual labeled row assignments.

---

## II.7 Exact (C=6) dimensions and resources

### Histogram spaces

|      Processed row pairs (r) |            0 |         1 |                 2 |                       3 |                             4 |                              5 |                              6 |   |   |
| ---------------------------: | -----------: | --------: | ----------------: | ----------------------: | ----------------------------: | -----------------------------: | -----------------------------: | - | - |
|    Raw balanced histograms ( | \mathcal H_r |         ) |                 1 |                       1 |                         3,355 |                         49,755 |                          3,355 | 1 | 1 |
|                 (S_6)-orbits |            1 |         1 |                24 |                     132 |                            24 |                              1 |                              1 |   |   |
| Total labeled partial arrays |            1 | 7,484,400 | 6,928,346,502,000 | 513,470,940,732,288,000 | 1,583,587,180,574,163,072,000 | 70,957,164,389,662,881,792,000 | 70,957,164,389,662,881,792,000 |   |   |

The previously known 49,755 midpoint occupancies and 132 color orbits are therefore exactly the right global midpoint space. They failed for fixed (G) only because they did not record alignment with a fixed future skeleton.

### Transition generation

Matrix dimensions are target-by-source.

| Map       |    Dimensions | Contingency leaves | Raw-target occurrences | Cached distinct raw targets | Nonzero quotient entries |
| --------- | ------------: | -----------------: | ---------------------: | --------------------------: | -----------------------: |
| (0\to1)   |    (1\times1) |                  1 |                      1 |                           1 |                        1 |
| (1\to2)   |   (24\times1) |             27,990 |                  3,355 |                       3,355 |                       24 |
| (2\to3)   | (132\times24) |            281,066 |                165,678 |                      36,374 |                    2,352 |
| (3\to4)   | (24\times132) |             90,557 |                 57,853 |                       2,758 |                    2,352 |
| (4\to5)   |   (1\times24) |                218 |                     24 |                           1 |                       24 |
| (5\to6)   |    (1\times1) |                  1 |                      1 |                           1 |                        1 |
| **Total** |             — |        **399,833** |            **226,912** |                  **42,490** |                **4,754** |

The complete global DAG has

[
1+1+24+132+24+1+1=184
]

nodes.

### Exact output

[
A_6
===

70,957,164,389,662,881,792,000,
]

and

[
N_{\mathrm{lin}}(6)=2^{36}A_6
=============================

4,876,139,207,527,966,044,188,061,990,912,000.
]

Measured on the current execution environment:

* wall time: 0.75 seconds;
* user CPU time: 0.71 seconds;
* maximum RSS: 14,340 kB;
* exact arithmetic: `boost::multiprecision::cpp_int`.

This includes transition generation, canonicalization, and the final contraction.

### Arithmetic bounds

A priori,

[
A_6\le (6!)^{12}=720^{12}<2^{114},
]

and therefore

[
N_{\mathrm{lin}}(6)
\le 2^{36}720^{12}<2^{150}.
]

Arbitrary-precision integer arithmetic is simplest. A CRT implementation may use any three verified 61-bit primes. For example,

[
\begin{aligned}
p_1&=2305843009213693951,\
p_2&=2305843009213693921,\
p_3&=2305843009213693907.
\end{aligned}
]

Their product has 183 bits, exceeding the rigorous (2^{150}) bound. Nonnegative CRT reconstruction is therefore unique.

---

## II.8 Multiplicity audit

| Potential factor                     | Treatment                                                                                                        |     |   |                              |    |
| ------------------------------------ | ---------------------------------------------------------------------------------------------------------------- | --- | - | ---------------------------- | -- |
| Ordered edge colors                  | Colors remain labeled in every (\sigma_j). The (S_6) quotient stores orbit totals; there is no division by (6!). |     |   |                              |    |
| Labeled symbols/right vertices       | The factors (\sigma_1,\ldots,\sigma_{12}) are ordered. The multinomial (\mu(n)) restores every symbol labeling.  |     |   |                              |    |
| Row-pair order                       | The six transfer levels are the six fixed labeled row pairs. There is no division by (6!).                       |     |   |                              |    |
| Two vertices within each row pair    | For every ((r,a)), one of two occurrences is assigned bit 1. This gives exactly (2^{36}).                        |     |   |                              |    |
| Outer row complements                | Already included among those (2^{36}) orientation choices; no extra (2^6).                                       |     |   |                              |    |
| Outer row permutations               | Restored by the outer weight (w) when classes are unrolled.                                                      |     |   |                              |    |
| Outer stabilizers                    | Included through (                                                                                               | W_6 | / | \operatorname{Stab}_{W_6}(h) | ). |
| Multiple paths to one quotient state | Added by the integer recurrence (25).                                                                            |     |   |                              |    |
| (D_8) half-kernel action             | It was an internal fixed-graph suffix quotient. No (D_8) object is constructed here, so no factor of 8 appears.  |     |   |                              |    |
| Repeated canonical representatives   | Orbit totals and transition-to-orbit multiplicities count them once with their exact labeled multiplicity.       |     |   |                              |    |

---

## II.9 Regression evidence

The following exact checks were run.

### Direct outer-loop check at (C=3)

All

[
\binom63^3=8,000
]

labeled balanced skeletons were enumerated independently. Their exact per-skeleton (F_3) values summed to

[
460,800,
]

matching the global recurrence.

### Small-(C) table

| (C) | Orbit layers        |                          (A_C) |              (N_{\mathrm{lin}}(C)=2^{C^2}A_C) |
| --: | ------------------- | -----------------------------: | --------------------------------------------: |
|   2 | (1,1,1)             |                              6 |                                            96 |
|   3 | (1,1,1,1)           |                            900 |                                       460,800 |
|   4 | (1,1,4,1,1)         |                     12,640,320 |                               828,396,011,520 |
|   5 | (1,1,7,7,1,1)       |             41,376,005,798,400 |                 1,388,348,372,994,018,508,800 |
|   6 | (1,1,24,132,24,1,1) | 70,957,164,389,662,881,792,000 | 4,876,139,207,527,966,044,188,061,990,912,000 |

### Independent (C=6) state space

A second implementation processes the twelve permutations one at a time. Its state is

[
A\in{0,1,2}^{6\times6},
\qquad
A\mathbf1=k\mathbf1,\quad
A^\top\mathbf1=k\mathbf1,
]

quotiented by independent row and column permutations. Its orbit layers are

[
1,\ 1,\ 11,\ 72,\ 997,\ 4608,\ 8513.
]

A meet-in-the-middle contraction at (k=6) produced the identical value of (A_6). This is a materially different recurrence and canonical state representation.

The global method does not expose individual class values, so it does not independently recompute

[
F_6(G_1)=6986348258918400,\qquad
F_6(G_2)=7053808087203840.
]

It is nevertheless structurally compatible with them: each labeled coloring of each labeled representative occurs exactly once in the bijection. The remaining class-level audit is to verify the stored (w(G_1)) and (w(G_2)) against (2), while retaining the existing per-class engine as the numerical unit test for those values.

### Reproducible files

* [Main 132-state exact implementation](sandbox:/mnt/data/global_linear_histogram_dp.cpp)
* [Build and usage notes](sandbox:/mnt/data/README_global_linear_aggregation.md)
* [Consolidated exact outputs, ranks, resource log, and hashes](sandbox:/mnt/data/global_linear_aggregation_exact_record.txt)
* [Exact (C=6) output](sandbox:/mnt/data/global_linear_histogram_dp_output.txt)
* [Resource-usage log](sandbox:/mnt/data/global_linear_histogram_dp_time.txt)
* [Modular-rank certificate program](sandbox:/mnt/data/global_linear_histogram_rank.cpp)
* [Independent 8,513-state count-matrix implementation](sandbox:/mnt/data/global_linear_aggregation_c6.cpp)
* [Direct (C=3) outer-skeleton regression](sandbox:/mnt/data/direct_skeleton_regression_c3.cpp)

---

# III. Immediate empirical experiment

For the stated linear objective, continuation caching is no longer needed. The following experiment remains decisive if the actual intended target is the squared sum, or if the stored outer weights fail the natural-unrolling audit.

## III.1 Class selection

Use approximately 200 classes:

1. Eight classes from each of the 17 uncolored suffix classes: 136 classes.
2. Add extremes from:

   * automorphism-group order;
   * midpoint-frontier size;
   * outer weight;
   * canonical class index.
3. Add uniform controls from early, middle, and late class-order ranges.
4. Deduplicate the selected class IDs.

Within each class, sample

[
\min(2^{20},\text{frontier size})
]

midpoint states using deterministic stratified reservoir sampling. Stratify by:

* prefix-multiplicity logarithmic decile;
* separator occupancy bucket;
* half-kernel ID;
* relative-transformation bucket.

Process classes in at least three orders:

* canonical order;
* grouped by suffix class;
* five independently seeded random orders.

This separates genuine saturation from favorable ordering.

## III.2 Nested exact keys

| Key   | Appended exact information                                                                                                                                                    |
| ----- | ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| (K_0) | Existing (D_8\times S_6)-canonical half-kernel identifier.                                                                                                                    |
| (K_1) | (K_0), suffix-class ID, separator degree profile, and used-color histogram.                                                                                                   |
| (K_2) | Canonical multiset ({(\tau(j),K_j)}_{j=1}^{12}), under suffix automorphisms and (S_6).                                                                                        |
| (K_3) | (K_2) plus exact relative color permutation, (D_8) orientation, and row-pair frame.                                                                                           |
| (K_4) | (K_3) plus the complete colored boundary incidence matrix and separator attachment map.                                                                                       |
| (K_5) | Full canonical serialization of the remaining colored incidence problem and every coupling transformation required by the tail evaluator. This is the exact continuation key. |

Prefix multiplicity is a coefficient attached to a key, not part of the key.

Every key should be stored as:

* a canonical byte string;
* a 256-bit indexing hash;
* bytewise collision resolution.

Continuation values can be stored as unsigned 128-bit integers, or as residues modulo the three CRT primes above.

## III.3 Record format

A sufficient fixed header per sampled state is:

```text
class_id                 uint32
suffix_class             uint8
outer_automorphism_order uint32
outer_weight             uint128 / length-prefixed integer
frontier_size            uint64
sample_sequence          uint64
prefix_multiplicity      uint128
K_i_byte_length          uint16
K_i_bytes                variable
continuation_value       uint128 or 3 x uint64 residues
```

Store each (K_i) in a separate stream so the storage cost and canonicalization cost of each refinement can be measured independently.

## III.4 Required measurements

For every key level and class prefix (1,\ldots,k), report:

[
D_i(k)=#{\text{globally distinct }K_i\text{ keys}},
]

as well as:

* within-class distinct keys;
* cross-class collision multiplicities;
* marginal new-key rate;
* lookup reuse from earlier classes;
* reuse conditioned on suffix class;
* reuse conditioned on automorphism-group order;
* key byte size;
* canonicalization nanoseconds per state;
* Shannon entropy and (\exp(H));
* exact false-sharing rate.

For (K_i), false sharing means a bucket contains more than one exact (K_5) continuation value. Report both bucket-weighted and state-weighted false-sharing rates.

Fit all requested growth models:

[
ak+b,
]

[
ak^\alpha+b,
]

[
a(1-e^{-k/\tau})+b,
]

and the suffix-conditioned model

[
D(k_1,\ldots,k_{17})
====================

\sum_{s=1}^{17}D_s(k_s)-D_{\mathrm{shared}}.
]

Report fitted parameters, residuals, extrapolation to 63,199 classes, and order sensitivity.

## III.5 Engineering decision thresholds

The measured (G_2) tail cost is approximately

[
\frac{824\text{ s}}{221,438,460}
\approx 3.72\ \mu\text{s per midpoint state}.
]

Treat exact caching as a primary strategy only if all of the following hold for (K_5):

1. Cross-class hit rate is at least 95%.
2. Canonical-key construction is at most (0.15,\mu\mathrm{s}) per state.
3. The fitted upper confidence bound satisfies (\alpha<0.75), or the saturation model projects fewer than (2\times10^9) global keys.
4. At 48 bytes per stored entry, projected storage is below about 96 GB.
5. The idealized CPU ratio satisfies

   [
   \frac{D}{T}+\frac{c_{\mathrm{can}}}{3.72\ \mu\mathrm{s}}
   <0.1.
   ]

Reject continuation caching as the primary solution if:

* the lower confidence bound has (\alpha>0.9);
* the marginal new-key rate remains above 50% after every suffix class has at least ten sampled classes;
* or the exact (K_5) hit rate remains below 80%.

The current 100,000-state (G_2) sample cannot decide any of these conditions because it contains no cross-class comparison.

---

# IV. Global algebraic route

## IV.1 One-copy coefficient collapse

For a bit vector (b\in{0,1}^C), define

[
p_b(x)
======

\operatorname{perm}
\left(x_{r,a,b_r}\right)_{r,a=1}^C.
]

For the linear sum, the one-symbol tensor is

[
\Psi_C(x)=\sum_{b\in{0,1}^C}p_b(x).
]

Let

[
z_{r,a}=x_{r,a,0}+x_{r,a,1}.
]

By expanding the permanent over permutations,

[
\begin{aligned}
\Psi_C(x)
&=
\sum_{\sigma\in S_C}
\prod_{r=1}^C
\left(x_{r,\sigma(r),0}+x_{r,\sigma(r),1}\right)\
&=
\operatorname{perm}(Z).
\end{aligned}
\tag{27}
]

Therefore

[
\boxed{
N_{\mathrm{lin}}(C)
===================

\left[
\prod_{r,a}x_{r,a,0}x_{r,a,1}
\right]
\operatorname{perm}(Z)^{2C}.
}
\tag{28}
]

Since

[
[x_0x_1](x_0+x_1)^2=2,
]

one obtains

[
\boxed{
N_{\mathrm{lin}}(C)
===================

2^{C^2}
\left[
\prod_{r,a}z_{r,a}^2
\right]
\operatorname{perm}(Z)^{2C}.
}
\tag{29}
]

Equation (29) is not left as a formal coefficient identity: the 132-state recurrence is an explicit contraction order for it.

## IV.2 Independent count-matrix contraction

After choosing (k) of the (2C) permutation factors, let

[
A=\sum_{j=1}^kP_{\sigma_j}.
]

Then

[
A\in{0,1,2}^{C\times C},
\qquad
A\mathbf1=k\mathbf1,
\qquad
A^\top\mathbf1=k\mathbf1.
]

Define (f_k(A)) to be the number of ordered decompositions of (A) into (k) permutation matrices. The exact recurrence is

[
f_{k+1}(B)
==========

\sum_{\substack{\sigma\in S_C\B-P_\sigma\in{0,1,2}^{C\times C}}}
f_k(B-P_\sigma).
\tag{30}
]

Quotienting by independent row and column permutations gives the (C=6) orbit layers

[
1,\ 1,\ 11,\ 72,\ 997,\ 4608,\ 8513.
]

At the midpoint,

[
A_C
===

\sum_{A}
f_C(A)f_C(2J-A).
\tag{31}
]

In orbit-total form,

[
A_C
===

\sum_{[A]}
\frac{g_C([A])g_C([2J-A])}{|[A]|}.
\tag{32}
]

This independent contraction produced exactly the same (A_6). It is slower than the histogram recurrence but serves as a strong validation of the permanent identity.

## IV.3 Exact paired kernel for the squared objective

For the squared objective, let (c_j) and (d_j) be the colors assigned to symbol (j) in the two copies during one row pair. Each color occurs twice among the (c_j) and twice among the (d_j).

Construct a bipartite multigraph (H(c,d)) with:

* left vertices equal to first-copy colors;
* right vertices equal to second-copy colors;
* one edge (j) from (c_j) to (d_j).

Every vertex has degree two, so (H(c,d)) is a disjoint union of even cycles, including two-edge cycles formed by parallel edges.

A shared skeleton (A) must choose exactly one of the two (c)-occurrences of every first-copy color and exactly one of the two (d)-occurrences of every second-copy color. Thus (A) is a perfect matching of (H(c,d)). Every cycle has exactly two perfect matchings. Consequently,

[
\boxed{
#{\text{shared row skeletons compatible with }c,d}
==================================================

2^{\kappa(H(c,d))},
}
\tag{33}
]

where (\kappa(H)) is the number of cycles.

This gives an explicit globally summed two-copy row kernel:

[
K_r(x,y)
========

\sum_{\substack{c,d\text{ legal}\
\text{each color twice}}}
2^{\kappa(H(c,d))}
,e_{(x+c,\ y+d)}.
\tag{34}
]

The natural exact state is now the joint histogram

[
h_{S,T}
=======

#{j:S_j=S,\ T_j=T},
\tag{35}
]

under

[
S_{12}\times S_6\times S_6
]

and the copy-swap involution.

The remaining bottleneck is precise: transition (34) depends on the full correlation encoded by (h_{S,T}), not merely its two marginals or the overlap matrix of a single row. No exact quotient strictly smaller than the full joint orbit state is currently proved. This is where the squared problem remains blocked.

---

# V. Rank and sparsity information

## V.1 Exact ranks of the linear quotient operators

Ranks were computed modulo the prime

[
p=1,000,003.
]

Each reported rank is the maximum possible. A maximal minor nonzero modulo (p) is a nonzero integer minor, so these are exact full-rank certificates over (\mathbb Q).

### (C=4)

| Map     | Dimensions | Nonzeros | Exact rank |
| ------- | ---------: | -------: | ---------: |
| (0\to1) | (1\times1) |        1 |          1 |
| (1\to2) | (4\times1) |        4 |          1 |
| (2\to3) | (1\times4) |        4 |          1 |
| (3\to4) | (1\times1) |        1 |          1 |

### (C=5)

| Map     | Dimensions | Nonzeros | Exact rank |
| ------- | ---------: | -------: | ---------: |
| (0\to1) | (1\times1) |        1 |          1 |
| (1\to2) | (7\times1) |        7 |          1 |
| (2\to3) | (7\times7) |       42 |          7 |
| (3\to4) | (1\times7) |        7 |          1 |
| (4\to5) | (1\times1) |        1 |          1 |

### (C=6)

| Map     |    Dimensions | Nonzeros | Exact rank |
| ------- | ------------: | -------: | ---------: |
| (0\to1) |    (1\times1) |        1 |          1 |
| (1\to2) |   (24\times1) |       24 |          1 |
| (2\to3) | (132\times24) |    2,352 |         24 |
| (3\to4) | (24\times132) |    2,352 |         24 |
| (4\to5) |   (1\times24) |       24 |          1 |
| (5\to6) |    (1\times1) |        1 |          1 |

Thus the reduction is **not** a low-rank approximation. The surviving maps are full rank. The gain comes from exact skeleton marginalization and the resulting 132-dimensional invariant multiplicity space.

## V.2 Exact (3+3) flattening

For the linear problem, the raw midpoint index set is

[
\mathcal H_3(6),
\qquad
|\mathcal H_3(6)|=49,755.
]

After the (S_6) color quotient it has 132 indices. A balanced (3+3) contraction therefore has:

* left boundary: 132 orbit states;
* right boundary: 132 orbit states;
* complement map (S\mapsto[6]\setminus S);
* exact scalar contraction of a left prefix vector and right continuation vector.

No outer class is present at this stage.

For the prior paired/squared tensor, the supplied computations found full natural ranks (630/630) at (C=4), (8001/8001) at (C=5), and a full local balanced-cut rank (462/462). Those results block ordinary channel truncation, but they do not contradict the one-copy collapse: the one-copy outer bit sum projects onto a much smaller invariant module before the midpoint is formed.

---

# VI. Approach registry

| Family                                         | State/object                                                     | New mechanism                                                                                    | Proven result                                                | Experiment                                           | Remaining gap                                                                                           | Gap strength                                                   | (C=6) dimensions                                 | Complexity                                | Status                                    |
| ---------------------------------------------- | ---------------------------------------------------------------- | ------------------------------------------------------------------------------------------------ | ------------------------------------------------------------ | ---------------------------------------------------- | ------------------------------------------------------------------------------------------------------- | -------------------------------------------------------------- | ------------------------------------------------ | ----------------------------------------- | ----------------------------------------- |
| Outer-sum marginalization and graded orbit DP  | Balanced subset histogram (h_S) and orbit totals (G_r)           | Sum each row skeleton before colored-state expansion; fixed future-neighborhood labels disappear | Equations (8), (21), and (25); exact value obtained          | Two exact implementations and direct (C=3) outer sum | Only audit that stored (w) equals natural orbit-unrolling weight                                        | Strictly weaker than original; finite table check              | 184 nodes, 4,754 arcs, peak 132 states           | 399,833 contingency leaves; 0.75 s, 14 MB | **Complete for linear objective**         |
| Myhill–Nerode/bisimulation quotient            | Continuation function on labeled partial states                  | (S_{12}\times S_6) orbit relation is a strong lumpable refinement                                | Continuation constant on histogram-color orbits              | Exhaustive transition generation                     | Coarsest equivalence not identified, but unnecessary                                                    | Weaker                                                         | Peak 132 quotient classes                        | Included above                            | **Merged with complete DP**               |
| Level-synchronous global DP                    | All skeletons advanced simultaneously by (R_r)                   | Prefix coefficients accumulated before any class identity is formed                              | Exact recurrence (25)                                        | Full (C=2,\ldots,6) runs                             | None after weight audit                                                                                 | Weaker                                                         | (1,1,24,132,24,1,1)                              | Below one second                          | **Complete**                              |
| Graded orbit algebra                           | Orbit-sum modules and structure constants (t_r(O,O'))            | Exact centralizer/intertwiner basis under (S_{12}\times S_6)                                     | Closed under every transfer; projector (16) and entries (23) | Exact modular ranks                                  | None for linear problem                                                                                 | Weaker                                                         | Multiplicity dimensions (1,1,24,132,24,1,1)      | 4,754 nonzeros                            | **Complete**                              |
| Permanent/coefficient extraction               | ([\prod z_{ra}^2]\operatorname{perm}(Z)^{12})                    | Bit sum gives (\sum_b p_b=\operatorname{perm}(x_0+x_1))                                          | Exact identity (29) and executable contraction               | Independent 8,513-state count-matrix DP              | None for linear problem                                                                                 | Weaker                                                         | Count-matrix midpoint 8,513 orbits               | 17.99 s, 29 MB in validation code         | **Complete**                              |
| Representation-theoretic block diagonalization | Trivial-isotypic multiplicity spaces of (S_{12}\times S_6)       | Exact selection rule removes all nontrivial sectors                                              | Surviving linear maps have maximal rank                      | Modular rank certificates                            | No further rank reduction exists in this basis; paired full-rank blocks still need implicit application | For paired problem, comparable to original                     | Linear peak 132; paired prior ranks 630 and 8001 | Linear trivial; paired unresolved         | **Merged for linear; blocked for square** |
| Balanced (3+3) tensor                          | Midpoint histogram orbit, or joint pair histogram for two copies | One-copy midpoint is only 132-dimensional after global marginalization                           | Exact one-copy contraction                                   | Exact dimensions and ranks                           | For the paired tensor, compact contraction of (h_{S,T}) is unproved                                     | Comparable to original                                         | Linear 132; paired much larger                   | Linear tiny; paired unknown               | **Complete linear / BLOCKED square**      |
| Holant/Hadamard transformation                 | Local bit channel (x_0,x_1)                                      | Invertible symmetric/antisymmetric basis; outer sum has support only in symmetric channel        | Exact one-copy channel elimination and factor (2^{C^2})      | Algebraic expansion and exact DP                     | Shared-skeleton two-copy tensor retains correlated channels                                             | Smaller local question, but no favorable paired identity found | One local channel survives linearly              | Constant local work                       | **Complete linear / blocked square**      |
| Fixed-class continuation cache                 | Nested keys (K_0,\ldots,K_5)                                     | Reuse exact continuations without algebraically removing class identity                          | No global reuse theorem                                      | Experiment specified in Section III                  | Whether (K_5) distinct keys grow sublinearly                                                            | Strictly weaker and falsifiable                                | Unknown                                          | Must meet 95% hit and storage thresholds  | **Active fallback only**                  |
| Shared prefix decision DAG                     | Canonical row-exposure nodes                                     | Symbolic transitions aggregate all class prefixes with exact weights                             | The completed linear DAG has 184 nodes                       | Exact enumeration                                    | For squared objective, joint-prefix DAG may still explode                                               | Linear gap absent; square gap comparable                       | 184 nodes                                        | 4,754 arcs                                | **Complete linear / unresolved square**   |
| Modular structural analysis                    | Sparse transfer matrices over finite fields                      | Exact rank certificates, not floating-point SVD                                                  | Full maximal ranks certified                                 | (C=4,5,6) ranks modulo 1,000,003                     | None for reported ranks                                                                                 | Smaller                                                        | Tables in Section V                              | Negligible                                | **Supporting evidence**                   |

---

# VII. Exact remaining gap

## For the linear objective stated in the question

There is no remaining mathematical compression lemma.

The only exact data audit is:

[
\widehat w([h])
===============

\frac{2^6 6!}{|\operatorname{Stab}_{W_6}(h)|}
\frac{12!}{\prod_p h_p!},
]

followed by

```text
assert stored_weight[h] == computed_weight[h] for all 63,199 classes
assert sum(stored_weight) == 924^6
```

If both assertions pass, the global contraction proves

[
\boxed{
N(6)
====

4,876,139,207,527,966,044,188,061,990,912,000
}
]

for the linear sum.

## For the squared objective

The smallest precise missing statement is:

> Construct an exact quotient or implicit application algorithm for the cycle-weighted joint transition (34), acting on the joint histogram (h_{S,T}), whose (C=6) intermediate dimension or operation count is demonstrably below the aggregate fixed-class frontiers.

This is currently a theorem-strength gap. Merely asserting that the joint color correlations should compress would be equivalent to the original aggregation problem. The genuinely new local mechanism available is the exact cycle factor

[
2^{\kappa(H(c,d))}.
]

The next falsifiable computation for that problem is the exact modular rank and sparsity of the cycle-weighted joint transfer at (C=4) and (C=5), followed by the (K_0,\ldots,K_5) global-key experiment if those ranks remain full.

The immediate implementation step for the problem as written is to run the weight audit and replace the 63,199-class driver with the 132-state program linked above.
