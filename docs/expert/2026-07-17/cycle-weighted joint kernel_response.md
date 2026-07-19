# Result now

[
\boxed{\text{The cycle-weighted transition can be applied exactly without storing }h_{S,T}.}
]

I obtained an exact **connectivity-lumped, symbol-synchronous recurrence**. It independently reproduces

[
\begin{aligned}
N(2)&=288,\
N(3)&=28,200,960,\
N(4)&=29,136,487,207,403,520.
\end{aligned}
]

These agree with the established sequence. ([整数序列在线百科全书][1])

It has **not yet reproduced (N(5))**. The exact (C=5) computation reaches

[
83,776
]

symmetry-reduced states after only three of the ten labeled symbols. Thus, the connectivity quotient answers the mathematical question affirmatively, but it does not by itself make (C=5), and therefore (C=6), computationally feasible.

The best current conclusion is:

[
\boxed{
\begin{array}{l}
\text{Full joint histograms are not necessary;}\
\text{degrees plus live-path connectivity are sufficient;}\
\text{but a fast implicit application of the permutation sum is still necessary.}
\end{array}}
]

---

# 1. Exact reformulation by labeled symbols

For each labeled symbol

[
s\in[2C],
]

let

[
\sigma_s,\tau_s\in S_C
]

be its band-to-column permutations in the two stacks.

For each band (r), construct a bipartite multigraph

[
H_r
]

with left and right vertex sets both equal to ([C]), and with the edge associated with symbol (s) joining

[
\sigma_s(r)\quad\text{to}\quad\tau_s(r).
]

The row constraints say that every vertex of (H_r) must ultimately have degree two.

For any legal final (H_r), the common band skeleton is a perfect matching of (H_r). Because a 2-regular bipartite multigraph is a disjoint union of even cycles and each cycle has exactly two alternating perfect matchings,

[
#{\text{compatible skeletons in band }r}
========================================

2^{\kappa(H_r)}.
]

Therefore the squared count has the exact one-line formulation

[
\boxed{
N(C)
====

\sum_{(\sigma_s,\tau_s)\in(S_C^2)^{2C}}
\prod_{r=1}^{C}
\mathbf 1_{{H_r\text{ is 2-regular}}}
,2^{\kappa(H_r)}.
}
\tag{1}
]

This is already independent of (h_{S,T}).

All multiplicities are transparent:

* symbols are labeled by their processing order (s=1,\ldots,2C);
* the two stack permutations are ordered;
* bands and columns remain labeled before symmetry quotienting;
* every skeleton is counted once by its corresponding perfect matching;
* every final cycle contributes exactly a factor of two.

---

# 2. The exact connectivity quotient

The important observation is that a partial graph (H_r) has maximum degree two. Every nontrivial connected component is therefore either:

* a path, or
* a completed cycle.

A completed cycle can never interact with any later edge because all its vertices already have degree two. It may be deleted immediately, provided the coefficient is multiplied by two.

A live path matters only through its two degree-one endpoints. Its internal degree-two vertices can never receive another edge.

Therefore an exact local state for one band is:

[
q_r=(d_r,\mu_r),
]

where

[
d_r(v)\in{0,1,2}
]

is the degree of every left and right color vertex, and (\mu_r) is a fixed-point-free involution on the degree-one vertices. Two endpoints are paired by (\mu_r) precisely when they are the endpoints of the same current path.

No edge list and no subset pair ((S,T)) is retained.

## Exact edge-update rules

Insert an edge (uv), where (u) is a left color and (v) a right color.

| Current degrees        | Exact update                                                                 |
| ---------------------- | ---------------------------------------------------------------------------- |
| (0,0)                  | Create a new path; pair (u) with (v).                                        |
| (1,0)                  | Extend the path ending at (u); replace endpoint (u) by (v).                  |
| (0,1)                  | Symmetric case.                                                              |
| (1,1), different paths | Merge the paths; pair their two other endpoints.                             |
| (1,1), same path       | Close a cycle; delete the endpoint pair and multiply the coefficient by (2). |
| Either degree is (2)   | Illegal transition.                                                          |

If the new edge closes cycles in (z) different bands, the transition multiplier is

[
2^z.
]

## Why this quotient is exact

Suppose two partial graphs have the same degree vector and the same endpoint pairing. Any future edge:

1. is legal in one exactly when it is legal in the other;
2. joins two different live paths in one exactly when it does so in the other;
3. closes a live path into a cycle in one exactly when it does so in the other;
4. produces the same new degree vector and endpoint pairing;
5. produces the same factor (1) or (2).

Induction over all future edges gives

[
q_r(G)=q_r(G')
\quad\Longrightarrow\quad
\operatorname{Cont}(G)=\operatorname{Cont}(G').
]

This is weighted strong lumpability. It is a proved continuation quotient, not a statistical compression.

---

# 3. Global state and exact symmetry handling

The global state after (m) symbols is

[
Q_m=(q_1,\ldots,q_C).
]

It is canonicalized under

[
S_C^{\mathrm{bands}}
\times S_C^{\mathrm{left\ columns}}
\times S_C^{\mathrm{right\ columns}}
\rtimes C_2,
\tag{2}
]

where (C_2) swaps the two stacks.

The implementation stores the **sum of coefficients of all labeled states in an orbit**. It never divides by an orbit size. Consequently:

* labeled band multiplicity is retained;
* labeled columns in both stacks are retained;
* stack-swap symmetry loses no factor;
* multiple paths into one canonical state are added exactly;
* nontrivial stabilizers require no special division rule.

For every source representative (Q), the transition is

[
W_{m+1}(Q')
===========

\sum_Q W_m(Q)
\sum_{\substack{\sigma,\tau\in S_C\
\operatorname{can}(Q+E_{\sigma,\tau})=Q'}}
2^{z(Q,\sigma,\tau)}.
\tag{3}
]

Here (E_{\sigma,\tau}) inserts edge

[
(\sigma(r),\tau(r))
]

in every band (r).

---

# 4. Exact computed results

## Frontier sizes

| (C) | Exact symmetry-reduced states after each processed symbol |                   Result |
| --: | --------------------------------------------------------- | -----------------------: |
|   2 | (1,1,3,1,1)                                               |                    (288) |
|   3 | (1,1,8,18,19,1,1)                                         |             (28,200,960) |
|   4 | (1,1,28,700,12,856,9,708,155,1,1)                         | (29,136,487,207,403,520) |
|   5 | (1,1,93,83,776) through symbol 3                          |            not completed |

The (C=4) result was rerun with checked integer operations: every multiplication and accumulation was promoted to `__uint128_t` and the program aborted on any `uint64_t` overflow. No overflow occurred.

## (C=4) resource measurements

The complete checked run used:

* wall time: **15.23 seconds**;
* user CPU: **122.27 seconds**;
* peak resident memory: **76,508 kB**;
* largest frontier: **12,856 states**.

Transition statistics were:

| Step | Target states | Valid source/move combinations |
| ---: | ------------: | -----------------------------: |
|    1 |             1 |                            576 |
|    2 |            28 |                            576 |
|    3 |           700 |                          9,861 |
|    4 |        12,856 |                        136,896 |
|    5 |         9,708 |                      1,150,717 |
|    6 |           155 |                        366,215 |
|    7 |             1 |                            959 |
|    8 |             1 |                              1 |

This is a genuinely independent route from the row-pair joint-histogram recurrence.

---

# 5. What happened at (C=5)

The first transition nominally contains

[
(5!)^2=14,400
]

pairs ((\sigma,\tau)), all symmetry-equivalent from the empty state. I therefore fixed the first pair to

[
(\mathrm{id},\mathrm{id})
]

and restored its exact multiplicity with the factor

[
(5!)^2.
]

For the second symbol, the relative pair ((p,q)) is classified under

[
(p,q)\longmapsto
(gpg^{-1},gqg^{-1})
]

and stack swap. There are 96 such relative-pair orbits at (C=5); after applying the full connectivity-state canonicalization, they merge to 93 exact states.

The third-symbol transition produced:

* 857,244 valid source/move combinations;
* 83,776 exact canonical states;
* 6.56 seconds wall time;
* 63,204 kB peak memory;
* no checked 64-bit overflow.

The next direct transition would inspect as many as

[
83,776(5!)^2
============

1,206,374,400
]

permutation-pair candidates before legality tests and target merging.

More importantly, the 83,776-state frontier is already slightly larger than the supplied 76,249-state middle layer of the existing (C=5) band recurrence. Thus:

[
\boxed{\text{Connectivity compression alone is not the missing order-of-magnitude improvement.}}
]

It changes what must be stored and proves that (h_{S,T}) is unnecessary, but it does not yet reduce the global (C=5) state count.

---

# 6. Other routes tested

## 6.1 Full (C\times C\times C) count tensor

I also implemented the more direct state

[
T_{r,a,b}
=========

#{s\le m:\sigma_s(r)=a,\ \tau_s(r)=b}.
]

At the end,

[
N(C)
====

\sum_T f(T)\prod_{r=1}^{C}\operatorname{perm}(T_r).
\tag{4}
]

This is exact and avoids (h_{S,T}), but its quotient is weaker. At (C=4) it had 30,360 states at the central tested level, versus 12,856 for the connectivity quotient. I therefore rejected it as the primary route.

## 6.2 Exact union-of-components/Kraus identity

For a 2-regular (H(c,d)),

[
2^{\kappa(H)}
=============

#\left{
(P,Q):
\mathbf 1_{{c_s\in P}}
======================

\mathbf 1_{{d_s\in Q}}
\ \forall s
\right}.
\tag{5}
]

Indeed, compatible ((P,Q)) are exactly unions of connected cycle components.

Grouping by the common symbol subset

[
A={s:c_s\in P}={s:d_s\in Q}
]

gives an exact Gram/Kraus decomposition

[
\boxed{
K=\sum_{t=0}^{C}
\sum_{\substack{A\subseteq[2C]\|A|=2t}}
B_{t,A}\otimes B_{t,A}.
}
\tag{6}
]

This is algebraically useful because it replaces the cycle weight by a sum of tensor squares of single-copy operators. It has not yet yielded a small propagation space: the correlation now resides in the operator-valued coefficient matrices (B_{t,A}).

## 6.3 Ordinary low rank

The prior exact rank calculations—(630/630) at (C=4), (8001/8001) at (C=5), and (462/462) for the local balanced-cut transform—rule out deleting ordinary low-rank channels. They do not rule out a fast implicit full-rank application.

## 6.4 Meet-in-the-middle

A symbol split does not immediately help. The left and right halves must be joined while retaining:

* residual degrees;
* which live endpoints are connected;
* how paths in one half connect paths in the other;
* the number of newly formed cycles;
* the requirement that every symbol contributes a pair of permutations.

I did not find a join key smaller than an equivalent connectivity incidence object. So this route currently relocates rather than eliminates the difficult state.

---

# 7. The remaining narrow route

The exact one-symbol operator from a source connectivity state is

[
\mathcal T_Q
============

\sum_{\sigma,\tau\in S_C}
\bigotimes_{r=1}^{C}
L^{Q_r}_{r,\sigma(r),\tau(r)},
\tag{7}
]

where (L^{Q_r}_{r,a,b}) is the local edge-update operator.

This is an **operator-valued double permanent**. It can be applied without enumerating all completed permutation pairs by a subset recurrence:

[
F_{j+1}
\bigl(U\cup{a},V\cup{b},q'\bigr)
;{+}{=};
\lambda(q_j,a,b),
F_j(U,V,q),
\tag{8}
]

where

* (j) bands have been updated;
* (U,V\subseteq[C]) are the used left and right colors;
* (a\notin U), (b\notin V);
* (q') is obtained by the exact local connectivity update;
* (\lambda\in{0,1,2}).

At (j=C), (U=V=[C]), and the result is exactly the sum over ((\sigma,\tau)).

This is the one remaining route that simultaneously:

1. preserves exact cycle weights;
2. avoids (h_{S,T});
3. avoids ordinary low-rank assumptions;
4. has a chance to avoid the (14,400) complete permutation-pair loop per source;
5. can be tested decisively at the existing 83,776-state (C=5) layer.

The engineering decision test is straightforward:

* **viable:** average internal double-permanent frontier below roughly 200–300 records per source and completed target support far below 14,400;
* **probably insufficient:** several thousand records per source;
* **reject:** essentially one record per legal partial placement, indicating near-injective growth.

Only after this operator passes (C=5) and reproduces

[
N(5)
====

1,903,816,047,972,624,930,994,913,280,000
]

should it be extended to (C=6). ([整数序列在线百科全书][1])

---

# Bottom line

The best exploration result is not a completed (C=5) or (C=6) count. It is the following exact structural advance:

[
\boxed{
\begin{gathered}
h_{S,T}\text{ is not an intrinsically necessary state};\
\text{the exact cycle-weighted continuation state is}\
\text{degree data plus a pairing of live path endpoints};\
N(2),N(3),N(4)\text{ are reproduced independently};\
C=5\text{ shows that this quotient must be combined with}\
\text{an implicit operator-valued permanent application.}
\end{gathered}}
]

That is a sharper diagnosis than either “the joint histogram is unavoidable” or “the cycle factor alone solves the square.” It identifies the precise remaining bottleneck: **not the representation of (2^{\kappa}), but the exact summation over two coupled permutations for every connectivity state.**

## Code and exact records

* [Research note](sandbox:/mnt/data/cycle_weighted_joint_transition_report.md)
* [Exact arbitrary-precision connectivity recurrence](sandbox:/mnt/data/joint_connectivity_dp.cpp)
* [Checked parallel (C=4) implementation](sandbox:/mnt/data/joint_connectivity_dp_omp_u64_checked.cpp)
* [Checked (C=4) output](sandbox:/mnt/data/joint_connectivity_dp_c4_checked.out)
* [Checked (C=4) resource log](sandbox:/mnt/data/joint_connectivity_dp_c4_checked.err)
* [Checked (C=5) two-symbol initializer and probe](sandbox:/mnt/data/joint_connectivity_dp_omp_u64_init2_checked.cpp)
* [Checked (C=5) frontier output](sandbox:/mnt/data/joint_connectivity_dp_c5_m3_checked.out)
* [Checked (C=5) resource log](sandbox:/mnt/data/joint_connectivity_dp_c5_m3_checked.err)
* [Alternative exact (C^3)-tensor implementation](sandbox:/mnt/data/joint_tensor_dp.cpp)

[1]: https://oeis.org/A291187?utm_source=chatgpt.com "A291187"
