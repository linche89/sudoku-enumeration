## Main conclusion

There is an exact reduction that removes the degree-(4!\to3) canonicalization layer entirely, and a stronger version that can bypass the degree-4 layer when evaluating (F_5).

The key is to quotient the free color symmetry **before** recursing, and more generally to peel off a block of colors determined by edges at one vertex.

---

# 1. Rooted color-block decomposition

Put

[
U_d(G):=\frac{F_d(G)}{d!},
]

the number of unordered 1-factorizations. The action of (S_d) on ordered factorizations is free, since every color class is a nonempty perfect matching.

Fix a vertex (v), and let (S={e_1,\ldots,e_a}) be (a) distinct edges incident with (v). Let

[
\mathcal R_a(G;S)
]

be the set of spanning (a)-regular subgraphs (H\subseteq G) containing all edges in (S).

Then

[
\boxed{
U_d(G)=
\sum_{H\in\mathcal R_a(G;S)}
U_a(H),U_{d-a}(G-H)
}
\tag{1}
]

and hence

[
\boxed{
F_d(G)=
\binom da
\sum_{H\in\mathcal R_a(G;S)}
F_a(H),F_{d-a}(G-H).
}
\tag{2}
]

### Correctness

In any unordered factorization of (G), the (a) factors containing (e_1,\ldots,e_a) are distinct, because those edges meet at (v). Their union is a unique (a)-regular subgraph (H\in\mathcal R_a(G;S)). Conversely, an unordered factorization of (H), together with one of (G-H), gives an unordered factorization of (G).

Thus (1) is a bijective decomposition. Multiplying by (d!) gives (2).

This identity strictly contains the current recurrence.

---

## 1.1 One-color peeling: exact-cover pivoting

Taking (a=1), for any edge (e),

[
\boxed{
U_d(G)=
\sum_{\substack{M\in\operatorname{PM}(G)\ e\in M}}
U_{d-1}(G-M)
}
\tag{3}
]

or equivalently

[
\boxed{
F_d(G)=
d\sum_{\substack{M\in\operatorname{PM}(G)\ e\in M}}
F_{d-1}(G-M).
}
\tag{4}
]

This is the perfect-matching version of Algorithm X: choose an uncovered edge, and branch only on perfect matchings covering that edge.

If

[
\mu_G(e)=
#{M\in\operatorname{PM}(G):e\in M},
]

then

[
\sum_{e\ni v}\mu_G(e)=\operatorname{pm}(G)
]

at every vertex (v). Therefore some incident edge satisfies

[
\mu_G(e)\le \frac{\operatorname{pm}(G)}d.
\tag{5}
]

For the first (C=6) graph, this reduces the top list from (192{,}528) matchings to at most

[
\frac{192{,}528}{6}=32{,}088.
]

This does not necessarily reduce the number (1{,}622) of degree-5 isomorphism classes by a factor six—an edge-transitive graph can still expose essentially all classes—but it cuts the input multiplicity and, more importantly, gives the lower-layer reductions below.

---

## 1.2 Two-color peeling: the direct circuit-partition reduction

Taking (a=2), let (e,f) be distinct edges at one vertex. Every spanning 2-factor (H) has

[
F_2(H)=2^{c(H)}.
]

Therefore

[
\boxed{
F_d(G)=
\binom d2
\sum_{H\in\mathcal R_2(G;{e,f})}
2^{c(H)}F_{d-2}(G-H).
}
\tag{6}
]

This is the useful circuit-partition identity.

### Degree 3

[
\boxed{
F_3(G)=
3\sum_{\substack{M\in\operatorname{PM}(G)\e\in M}}
2^{c(G-M)}.
}
\tag{7}
]

### Degree 4

[
\boxed{
F_4(G)=
6
\sum_{H\in\mathcal R_2(G;{e,f})}
2^{c(H)+c(G-H)}.
}
\tag{8}
]

This evaluates (F_4) **without constructing any cubic residual graph at all**.

Equivalently, if the two factors forming (H) are generated separately,

[
\boxed{
F_4(G)=
12
\sum_{\substack{M_1\ni e\
M_2\ni f,\ M_1\cap M_2=\varnothing}}
2^{c(G-M_1-M_2)}.
}
\tag{9}
]

Formula (8) groups together the (2^{c(H)-1}) decompositions of the same rooted 2-factor (H), so it is never worse than (9) in number of terminal objects.

### Degree 5

[
\boxed{
F_5(G)=
10
\sum_{H\in\mathcal R_2(G;{e,f})}
2^{c(H)}F_3(G-H).
}
\tag{10}
]

This bypasses the degree-4 layer completely: enumerate rooted 2-factors, and evaluate one cubic complement per 2-factor.

There is also a useful alternative search order:

[
\boxed{
F_5(G)=
30
\sum_{\substack{M\ni e\
H\in\mathcal R_2(G-M;{f,g})}}
2^{c(H)+c(G-M-H)},
}
\tag{11}
]

where (f,g) are two edges at a vertex of the degree-4 residual. Formula (11) is “one matching, then two 2-factors”; formula (10) is “one 2-factor, then a cubic graph.” The cheaper order can be chosen separately for every degree-5 graph.

### Degree 6

There are two balanced exact splits worth benchmarking.

Fix three edges (e_1,e_2,e_3) at one vertex:

[
\boxed{
F_6(G)=
20
\sum_{H\in\mathcal R_3(G;{e_1,e_2,e_3})}
F_3(H)F_3(G-H).
}
\tag{12}
]

Alternatively, pair the six incident edges as
((e_1,e_2),(e_3,e_4),(e_5,e_6)). If (\mathcal P_v(G)) is the set of ordered partitions

[
E(G)=E(H_1)\sqcup E(H_2)\sqcup E(H_3)
]

into spanning 2-factors with (e_{2i-1},e_{2i}\in H_i), then

[
\boxed{
F_6(G)=
90
\sum_{(H_1,H_2,H_3)\in\mathcal P_v(G)}
2^{c(H_1)+c(H_2)+c(H_3)}.
}
\tag{13}
]

Equations (12) and (13) are exact direct formulations, although their output sets may be too large at degree 6. Their main value is that they provide alternatives to the degree-(6!\to5!\to4!\to3) chain and can be cheaply pre-counted before deciding whether to enumerate.

---

# 2. Fixed-state enumeration of rooted factors

The subgraphs in (2) can be generated without graph isomorphism.

To enumerate spanning (a)-regular subgraphs of a (k)-regular bipartite graph, process the 12 left vertices. At each left vertex choose (a) of its (k) incident edges. Track only the partial right-degree vector

[
(s_1,\ldots,s_{12})\in{0,\ldots,a}^{12}.
]

After processing (i) left vertices,

[
s_1+\cdots+s_{12}=ai.
]

The total number of capacity states across all layers is

[
S_{12,a}
========

\sum_{i=0}^{12}
[x^{ai}](1+x+\cdots+x^a)^{12}.
\tag{14}
]

For the relevant cases:

| (a) | total states (S_{12,a}) |   largest layer |  transitions per state |
| --: | ----------------------: | --------------: | ---------------------: |
|   1 |               (4{,}096) |           (924) |            at most (k) |
|   2 |             (265{,}721) |      (73{,}789) | at most (\binom{k}{2}) |
|   3 |         (5{,}592{,}406) | (1{,}703{,}636) | at most (\binom{k}{3}) |

In particular:

[
S_{12,2}=\frac{3^{12}+1}{2}=265{,}721,
]

and

[
S_{12,3}=\frac{4^{12}+2}{3}=5{,}592{,}406.
]

Thus the feasibility/counting table for rooted 2-factors has at most

[
10\cdot265{,}721=2{,}657{,}210
]

transitions in a degree-5 graph, and at most

[
6\cdot265{,}721=1{,}594{,}326
]

in a degree-4 graph. The forced choice at the root vertex reduces these numbers further.

A suffix table permits output-sensitive enumeration of all valid factors. During backtracking:

* maintain a rollback disjoint-set structure for the chosen 2-factor;
* in degree 4, maintain a second rollback disjoint-set structure for the complementary 2-factor;
* every attempted union whose endpoints are already connected closes one cycle.

Consequently, (8) can be evaluated with no graph objects below the degree-4 input: only degree vectors, two rollback forests, and an integer accumulator.

---

## 2.1 Rigorous output bounds

The Bregman–Minc inequality gives, for a (k)-regular bipartite graph on (12+12) vertices,

[
\operatorname{pm}(G)\le (k!)^{12/k}.
]

For a fixed edge (e=uv), deleting (u,v) leaves (k-1) rows of degree (k-1) and (12-k) rows of degree (k), giving

[
\mu_G(e)
\le
(k-1)!(k!)^{(12-k)/k}.
\tag{15}
]

The resulting bounds are:

| degree (k) | (\operatorname{pm}(G)) | perfect matchings through a fixed edge |
| ---------: | ---------------------: | -------------------------------------: |
|          3 |              (1{,}296) |                                  (432) |
|          4 |             (13{,}824) |                              (3{,}456) |
|          5 |             (97{,}731) |                             (19{,}546) |
|          6 |            (518{,}400) |                             (86{,}400) |

These are direct applications of Bregman–Minc. ([arXiv][1])

For a degree-4 graph, the number of distinct rooted 2-factors in (8) is at most the number of rooted decompositions into two perfect matchings:

[
N_2(G;e,f)
\le
3{,}456\cdot432
===============

1{,}492{,}992.
\tag{16}
]

That is a loose universal bound. With your observed degree-4 perfect-matching count of roughly (2{,}500), the first rooted branch is at most about (625), so the corresponding terminal bound is approximately

[
625\cdot432=270{,}000
]

before grouping equal 2-factor unions.

For a degree-5 graph,

[
N_2(G;e,f)\le
19{,}546\cdot3{,}456
====================

67{,}550{,}976,
\tag{17}
]

but the 265,721-state DP computes the exact value of (N_2) cheaply before enumeration. This gives an exact adaptive rule:

* use (10) if the rooted 2-factor count is small;
* use (11), or the one-edge recurrence, if the rooted matching branch is smaller.

---

# 3. Exact-cover implementation using perfect-matching bitsets

The same reduction can be implemented with your existing perfect-matching lists.

For a fixed graph (G), enumerate its perfect matchings as edge masks

[
P_1,\ldots,P_p.
]

For every edge (e), build a bitset

[
B_e={i:e\in P_i}.
]

Optionally, for every matching (P_i), build the compatibility bitset

[
C_i
===

\overline{\bigcup_{e\in P_i}B_e},
\tag{18}
]

whose set bits are precisely the perfect matchings disjoint from (P_i).

The unordered recursion is then:

```text
U(mask, candidates, k):
    if k == 2:
        return 2^(cycle_count(mask) - 1)

    choose uncovered edge e minimizing
        popcount(candidates ∩ B[e])

    total = 0
    for i in candidates ∩ B[e]:
        total += U(mask \ P[i],
                   candidates ∩ C[i],
                   k - 1)

    return total
```

Finally,

[
F_k(G)=k!,U(G).
]

No canonical form appears in this algorithm. A raw residual edge mask is a valid exact memoization key.

For degree 4, the incidence bitsets require at most

[
48\cdot13{,}824\text{ bits}\approx81\text{ KiB},
]

and the full compatibility matrix requires about (22.8) MiB. For an observed degree-5 list of (27{,}000) perfect matchings, the compatibility matrix is about (91) MB. For the root list of (192{,}528), it would be about (4.6) GB, so I would retain your top residual grouping there and use the dense compatibility representation only inside degree-5 representatives.

This is the most direct replacement for the current canonicalization-heavy implementation.

---

# 4. The cubic layer is a flow-polynomial evaluation

For a cubic graph (H), identify the three colors with the three nonzero elements of

[
\mathbb F_2^2.
]

At a cubic vertex, three nonzero elements sum to zero exactly when they are all distinct. Therefore proper 3-edge-colorings are exactly nowhere-zero (\mathbb F_2^2)-flows:

[
\boxed{
F_3(H)=\Phi_H(4),
}
\tag{19}
]

where (\Phi_H) denotes the flow polynomial. Consequently,

[
\boxed{
F_3(H)
======

(-1)^{|E|-|V|+\kappa(H)}
T_H(0,-3).
}
\tag{20}
]

The standard flow-polynomial/Tutte relation is

[
\Phi_H(q)=
(-1)^{|E|-|V|+\kappa(H)}T_H(0,1-q).
]

([arXiv][2])

This answers the invariant part of the cubic question, but it is not the fastest computational form at (n=12). Equation (7) requires at most (432) terms after choosing a least-frequent edge:

[
F_3(H)
======

3\sum_{M\ni e}2^{c(H-M)}.
]

Each term is merely a cycle count in a 2-factor.

An independent exact check uses the binary cycle space. For connected (H) on (24) vertices and (36) edges,

[
\dim Z_1(H;\mathbb F_2)=36-24+1=13.
]

Enumerate the (2^{13}=8192) binary cycle-space vectors, retain those having degree two at every vertex, and sum (2^{c}). This is slower than the (432)-term pivot kernel but is an excellent independent validator.

For planar cubic graphs, the count can also be expressed through the Penrose polynomial, but that identity is tied to planarity and does not imply a cheap general evaluation. ([arXiv][3]) Counting 3-edge-colorings is (#\mathrm P)-hard even for simple planar cubic graphs, so no universal polynomial-time determinant or Pfaffian formula can exist for arbitrary cubic graphs unless (\mathrm{FP}=#\mathrm P). ([Springer Link][4]) This does not establish hardness for the bipartite subclass: the literature I found explicitly leaves exact counting for bipartite edge-colorings open, and I found no later resolution in a current search. ([arXiv][5])

---

## 4.1 Spectrum, determinant and permanent do not determine (F_3)

Here is an exact small counterexample. Let (H_1,H_2) be connected cubic bipartite graphs on (8+8) vertices with left-neighbor sets

[
\begin{aligned}
H_1:\quad&
{3,5,8},{1,2,6},{1,6,8},{4,5,8},\
&
{2,4,5},{3,6,7},{3,4,7},{1,2,7},
\end{aligned}
]

and

[
\begin{aligned}
H_2:\quad&
{1,4,6},{5,7,8},{1,3,4},{1,6,7},\
&
{2,3,8},{2,5,7},{2,6,8},{3,4,5}.
\end{aligned}
]

For their biadjacency matrices (B_1,B_2),

[
\operatorname{perm}(B_1)=\operatorname{perm}(B_2)=38,
\qquad
\det B_1=\det B_2=0,
]

and both have

[
\begin{aligned}
\det(tI-B_iB_i^\top)
={}&t^8-24t^7+218t^6-956t^5\
&+2121t^4-2276t^3+1060t^2-144t.
\end{aligned}
]

Thus they have the same bipartite adjacency spectrum, the same determinant and the same number of perfect matchings. Nevertheless,

[
F_3(H_1)=120,
\qquad
F_3(H_2)=144.
]

The underlying cycle distributions are

[
H_1:\quad
22,z+13,z^2+3,z^3,
]

and

[
H_2:\quad
16,z+18,z^2+3,z^3+z^4,
]

where the coefficient of (z^j) counts perfect matchings whose complement has (j) cycles. Evaluation at (z=2) gives the two (F_3) values.

So even the combination “spectrum + determinant + permanent” is insufficient.

---

# 5. Coefficient-extraction identities: exact but not competitive

There are two useful exact algebraic formulas, chiefly as negative results.

Let (m=|E(G)|=nd), and let (\operatorname{pm}(A)) be the number of perfect matchings in the spanning subgraph with edge set (A). Inclusion–exclusion gives

[
\boxed{
F_d(G)
======

\sum_{A\subseteq E(G)}
(-1)^{m-|A|}\operatorname{pm}(A)^d.
}
\tag{21}
]

Indeed, inclusion–exclusion forces the union of the (d) chosen perfect matchings to contain every edge. Since their total number of edge occurrences is exactly (nd=m), every edge must then occur exactly once.

For (C=6), this has (2^{72}\approx4.72\times10^{21}) terms.

A sharper Fourier form is obtained from signings (\epsilon:E\to{\pm1}). Let (A_\epsilon) be the signed biadjacency matrix. Then

[
\boxed{
F_d(G)
======

2^{-m}
\sum_{\epsilon\in{\pm1}^{E}}
\left(\prod_{e\in E}\epsilon_e\right)
\operatorname{perm}(A_\epsilon)^d.
}
\tag{22}
]

The Walsh sum retains precisely monomials in which every edge has odd exponent. Since the total degree equals the number of edge variables, every exponent must be one.

Vertex sign-switching leaves the complete summand invariant. Quotienting by switches reduces the number of signings to

[
2^\beta,
\qquad
\beta=|E|-|V|+\kappa(G).
]

For a connected (Q_B),

[
\beta=72-24+1=49,
]

so (22) still requires

[
2^{49}\approx5.63\times10^{14}
]

signed permanents. It is a nontrivial algebraic reduction from (2^{72}), but not a practical one.

Replacing the permanent by the determinant does not recover the unsigned count:

[
\left[\prod_{e\in E}x_e\right]\det(X_G)^d
=========================================

\sum_{\mathcal F}
\prod_{M\in\mathcal F}\operatorname{sgn}(M).
\tag{23}
]

This is a signed parity difference. For (K_{n,n}), it is, up to a fixed convention, the difference between even and odd Latin squares rather than their total number. ([arXiv][6])

---

# 6. Exploiting the complementary-pair structure of (Q_B)

After removing any number of perfect matchings, the complementary-pair structure persists.

At residual degree (k), write the surviving neighborhoods of the two vertices in pair (r) as

[
P_r,N_r\subseteq[12],
\qquad
P_r\cap N_r=\varnothing,
\qquad
|P_r|=|N_r|=k.
]

Every right vertex has residual degree (k).

The number of perfect matchings is

[
\boxed{
\operatorname{pm}(H)
====================

[z_1\cdots z_{12}]
\prod_{r=1}^6
\left(
\sum_{a\in P_r}
\sum_{b\in N_r}
z_az_b
\right).
}
\tag{24}
]

A subset DP over the used right vertices processes one complementary pair at a time. At layer (r), every state has size (2r). Hence the total number of possible masks is only

[
\sum_{r=0}^6\binom{12}{2r}=2^{11}=2048,
]

with maximum layer size (\binom{12}{6}=924). The transition count is at most

[
2048,k^2,
]

which is (73{,}728) at (k=6).

Prefix/suffix tables give all local pair-choice frequencies, and hence all edge frequencies, in roughly

[
O(6k^2,2^{10})
]

integer operations. Backtracking can then enumerate only the perfect matchings through the selected least-frequent edge.

A convenient exact residual representation is a (6\times12) signed ternary matrix:

[

* ;=\text{surviving edge to the first member of a pair},
  \quad

- ;=\text{edge to its partner},
  \quad
  0 ;=\text{removed}.
  ]

This is useful for the top and degree-5 pivot engines. It does not, by itself, collapse the whole (F_6) coloring problem: processing the six left pairs directly and tracking colors used at each column has a midpoint with as many as

[
\binom63^{12}=20^{12}=4.096\times10^{15}
]

unquotiented column states.

So the complementary-pair structure gives a good perfect-matching engine and compact residual representation, but not an immediate full transfer matrix for (F_6).

---

# 7. Exact outer-sum identity and a representation-theoretic obstruction

Assume (m(B)) is the number of labeled skeletons in the orbit of (B), so that

[
\sum_{[B]}m(B)F_6(Q_B)^2
========================

\sum_{B\ \mathrm{labeled}}F_6(Q_B)^2.
]

Let (\mathscr A) be the set of (6\times12) arrays (A) with entries in ([6]) such that:

1. every column is a permutation of ([6]);
2. in each row, every symbol occurs exactly twice.

For (A\in\mathscr A), let (P_r(A)) be the perfect matching on the 12 columns that pairs the two occurrences of each symbol in row (r).

For two perfect matchings (P,Q) on 12 points, the common transversals are obtained by alternating around every cycle of the multigraph (P\cup Q). Therefore their number is

[
2^{c(P\cup Q)}.
]

Interchanging the sums over skeletons and colorings gives

[
\boxed{
\sum_{B\ \mathrm{labeled}}F_6(Q_B)^2
====================================

\sum_{A,A'\in\mathscr A}
\prod_{r=1}^6
2^{c(P_r(A)\cup P_r(A'))}.
}
\tag{25}
]

This is an exact Burnside-free second-moment formulation.

There is also a first-moment checksum:

[
\boxed{
\sum_{B\ \mathrm{labeled}}F_6(Q_B)
==================================

2^{36}|\mathscr A|.
}
\tag{26}
]

Each row matching has (2^6) transversals, independently over six rows.

---

## 7.1 Rank of the row kernel

Let (T) be the (924\times10{,}395) matrix whose rows are the 6-subsets (S\subset[12]), whose columns are perfect matchings (P), and

[
T_{S,P}=1
]

when (S) is a transversal of (P). Then

[
(T^\top T)_{P,Q}=2^{c(P\cup Q)}.
\tag{27}
]

Rows (S) and (S^c) coincide, so

[
\operatorname{rank}T=462.
]

For (a=|S\cap T|),

[
(TT^\top)_{S,T}=a!(6-a)!.
]

The nonzero eigenvalues are:

| (S_{12})-module | dimension | eigenvalue |
| --------------- | --------: | ---------: |
| ([12])          |         1 | (46{,}080) |
| ([10,2])        |        54 |  (3{,}840) |
| ([8,4])         |       275 |  (1{,}152) |
| ([6,6])         |       132 |      (720) |

Thus one row kernel compresses from (10{,}395) perfect-matching states to (462) cut states.

However, this does **not** collapse the six-row outer problem to four scalars. Let (W) be the 462-dimensional permutation module on unordered (6!:!6) cuts. Skeletons are multisets of six such cuts, modulo the (S_{12}) action on columns. Consequently,

[
\dim\left(\operatorname{Sym}^6W\right)^{S_{12}}
===============================================

63{,}199.
\tag{28}
]

Equivalently, a direct Burnside evaluation over the 77 conjugacy classes of (S_{12}) gives

[
\frac1{12!}
\sum_{\pi\in S_{12}}
[t^6]
\prod_{\ell\ge1}(1-t^\ell)^{-a_\ell(\pi)}
=========================================

63{,}199,
\tag{29}
]

where (a_\ell(\pi)) is the number of (\ell)-cycles induced by (\pi) on the 462 unordered balanced cuts.

So the natural representation-theoretic invariant space has **exactly the same dimension as your solved outer orbit problem**. Plain Burnside or Johnson-scheme diagonalization therefore cannot, by itself, reduce the 63,199 states. Any successful outer aggregation must exploit the special coloring tensor (\mathscr A), not merely the group action.

For (C=4) and (C=5), the analogous one-row ranks are

[
\frac12\binom84=35,
\qquad
\frac12\binom{10}{5}=126,
]

which are useful independent validation cases.

---

# 8. Recommended exact pipeline

For the current computation, I would use the following hybrid.

### Top degree 6

Use (4):

[
F_6(Q_B)=
6\sum_{M\ni e}F_5(Q_B-M),
]

where (e) has minimum perfect-matching frequency. For the measured first graph, enumerate at most (32{,}088) matchings rather than (192{,}528). Canonicalize only these degree-5 residuals and aggregate their multiplicities.

### Each degree-5 representative

First use the 265,721-state DP to count rooted 2-factors for all candidate pairs (e,f) at a vertex.

If the minimum count is small, use

[
F_5(G)=
10\sum_{H\in\mathcal R_2(G;{e,f})}
2^{c(H)}F_3(G-H).
]

Every cubic complement is evaluated in at most (432) cycle terms, with no canonical labeling.

If the rooted 2-factor count is too large, use the matching-pivot/degree-4 route

[
F_5(G)=5\sum_{M\ni e}F_4(G-M)
]

and evaluate each (F_4) by (8) or (9). Again, no cubic canonicalization is required.

### What disappears

The existing layer

[
\text{hundreds of thousands of labeled cubic residuals}
\longrightarrow
\text{tens of thousands of canonical cubic graphs}
]

is removed completely.

For your batch of 128 degree-4 parents producing about (320{,}000) cubic residuals, one-edge normalization reduces the first branch total to at most roughly

[
320{,}000/4=80{,}000.
]

Each such cubic child then has at most (432) rooted matching terms, giving a universal diagnostic ceiling around

[
80{,}000\cdot432=34.56\text{ million}
]

very small cycle traversals, rather than (320{,}000) generic canonicalizations. Grouping by rooted 2-factor as in (8) can reduce this further.

The color normalization removes the following color-order multiplicities exactly:

[
5\cdot4=20
]

before reaching the cubic frontier of an (F_5) computation, and

[
5\cdot4\cdot3=60
]

when expanded to the (F_2) base. For (F_6), the corresponding factor to (F_2) is

[
6\cdot5\cdot4\cdot3=360.
]

These are exact reductions in factorization multiplicity; they are not guaranteed wall-clock factors because raw residuals may merge differently. The decisive additional saving is that the remaining work is bitset intersection, degree-vector DP and cycle counting rather than canonical labeling.

---

# 9. Validation route

The following checks are independent enough to catch most implementation errors.

1. **Pivot independence.** For every edge (e),

   [
   \sum_{M\ni e}U_{d-1}(G-M)
   ]

   must be independent of (e).

2. **Block independence.** For every pair (e,f) at a vertex, the right side of (8) or (10) must be independent of the chosen pair.

3. **Three independent (F_4) calculations.** Compare:
   [
   \sum_MF_3(G-M),
   ]
   the rooted matching-pair formula (9), and the rooted 2-factor formula (8).

4. **Three independent cubic calculations.** Compare:

   * the (432)-term rooted matching sum;
   * the (8192)-state binary cycle-space sum;
   * a Tutte/flow-polynomial implementation.

5. **Known (C=4,5) totals.** Run the complete outer calculation with the new inner kernel and compare the exact known values. Also verify the first- and second-moment identities (25)–(26) on these smaller cases.

Additional cheap checks are

[
d!\mid F_d(G)
]

and, for (n=12,d=6),

[
F_6(G)\le(6!)^{12}<2^{114},
]

so every individual (F_6) fits in an unsigned 128-bit integer. The full outer sum is below roughly (2^{287}) if the orbit multiplicities sum to the labeled skeleton count, so five 61-bit CRT primes suffice.

---

## Bottom line

The strongest actionable identity is

[
\boxed{
F_d(G)=
\binom da
\sum_{\substack{H\subseteq G\text{ spanning }a\text{-regular}\
e_1,\ldots,e_a\in H}}
F_a(H)F_{d-a}(G-H).
}
]

For the present bottleneck, its crucial consequences are

[
\boxed{
F_4(G)=
6\sum_{H\in\mathcal R_2(G;{e,f})}
2^{c(H)+c(G-H)}
}
]

and

[
\boxed{
F_5(G)=
10\sum_{H\in\mathcal R_2(G;{e,f})}
2^{c(H)}F_3(G-H).
}
]

They remove the measured cubic canonicalization bottleneck exactly. The remaining uncertainty is empirical rather than mathematical: the rooted 2-factor count (N_2) for the degree-5 representatives. It can be obtained with only (265{,}721) capacity states before committing to enumeration, so the first representative should give a decisive benchmark very quickly. The outer (63{,}199)-state aggregation is not automatically collapsed by Burnside or the natural matching-kernel representation—the corresponding invariant space itself has dimension (63{,}199).

[1]: https://arxiv.org/pdf/0803.0864 "https://arxiv.org/pdf/0803.0864"
[2]: https://arxiv.org/pdf/2007.05195 "https://arxiv.org/pdf/2007.05195"
[3]: https://arxiv.org/pdf/0806.4699 "https://arxiv.org/pdf/0806.4699"
[4]: https://link.springer.com/article/10.1186/s40687-016-0067-8 "https://link.springer.com/article/10.1186/s40687-016-0067-8"
[5]: https://arxiv.org/pdf/2103.11990 "https://arxiv.org/pdf/2103.11990"
[6]: https://arxiv.org/pdf/1412.7574 "https://arxiv.org/pdf/1412.7574"
