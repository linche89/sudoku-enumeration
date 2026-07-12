Can we evaluate (F_6(Q_B)) directly in substantially less than one second per (12+12)-vertex graph, or aggregate the weighted sum without evaluating all 63,199 graphs separately?

## Verdict

**A fresh exact evaluation of an arbitrary (F_6(Q_B)) in substantially less than one second is not supported by any reduction I can presently justify.** The earlier rooted-block identities should remove the catastrophic cubic canonicalization layer, but they do not plausibly supply the additional four-to-five orders of magnitude needed for a subsecond worst-case evaluator.

**The weighted outer sum can be written as one exact coefficient, without mentioning the 63,199 representatives.** That is a genuine algebraic aggregation. Unfortunately, its straightforward transfer matrices are much larger than the orbit list, and ordinary Burnside or character diagonalization does not collapse them.

The remaining credible possibility is a highly structured, low-rank contraction of that single coefficient. It can be tested exactly on (C=4,5), but low rank has not yet been established for (C=6).

---

# 1. A single exact coefficient for the complete weighted sum

For (b=(b_1,\ldots,b_6)\in{0,1}^6), define

[
p_b(x)
======

\operatorname{perm}
\left(
x_{r,a,b_r}
\right)_{r,a=1}^6 ,
]

where there are (72) variables

[
x_{r,a,\epsilon},
\qquad
r,a\in[6],\quad \epsilon\in{0,1}.
]

Let the columns of (B) be (b^{(1)},\ldots,b^{(12)}). Then

[
\boxed{
F_6(Q_B)
========

[\Omega_x]\prod_{j=1}^{12}p_{b^{(j)}}(x),
}
\tag{1}
]

where

[
\Omega_x=
\prod_{r=1}^6\prod_{a=1}^6
x_{r,a,0}x_{r,a,1}.
]

### Why (1) is exact

At right vertex (j), a proper coloring assigns the six colors bijectively to the six incident edges, hence contributes one permutation in (p_{b^{(j)}}). Extracting (\Omega_x) says that at each left vertex ((r,\epsilon)), every color occurs exactly once.

Now introduce an independent variable family (y), and put

[
\Phi(x,y)
=========

\sum_{b\in{0,1}^6}
p_b(x)p_b(y).
]

Then, provided (m(B)) is the size of the labeled skeleton orbit as in the stated outer sum,

[
\boxed{
\sum_{[B]}m(B)F_6(Q_B)^2
========================

[\Omega_x\Omega_y],
\Phi(x,y)^{12}.
}
\tag{2}
]

No row-balance condition has to be added: the target monomial automatically forces six occurrences of (b_r=0) and six of (b_r=1) for every (r).

Thus (2) sums all

[
\binom{12}{6}^6
===============

622{,}345{,}892{,}187{,}672{,}576
]

labeled skeletons implicitly and produces exactly the orbit-weighted answer.

---

## 1.1 A 3,969-term representation of the column tensor

Formula (2) initially appears to involve

[
64(6!)^2=33{,}177{,}600
]

column monomials. Ryser expansion compresses it considerably.

Define

[
X_{r,S,\epsilon}
================

\sum_{a\in S}x_{r,a,\epsilon},
\qquad
Y_{r,T,\epsilon}
================

\sum_{a\in T}y_{r,a,\epsilon}.
]

Then

[
\boxed{
\Phi(x,y)=
\sum_{\substack{\varnothing\ne S\subseteq[6]\
\varnothing\ne T\subseteq[6]}}
(-1)^{|S|+|T|}
\prod_{r=1}^6
\left(
X_{r,S,0}Y_{r,T,0}
+
X_{r,S,1}Y_{r,T,1}
\right).
}
\tag{3}
]

There are only

[
(2^6-1)^2=3{,}969
]

product terms.

Equations (2)–(3) are the strongest direct aggregation I can presently give: they completely eliminate the outer graph loop at the algebraic level.

The problem is now the exact extraction of one squarefree coefficient of (\Phi^{12}).

---

# 2. Why the direct individual transfer is not subsecond

Process the six complementary left pairs one at a time. After three pairs have been processed, every right vertex has received three distinct colors. Consequently, the natural state records one 3-subset of ([6]) at each of the 12 right vertices.

Its potential midpoint state space is

[
\binom63^{12}
=============

# 20^{12}

4{,}096{,}000{,}000{,}000{,}000.
\tag{4}
]

Quotienting by the global color group removes at most a factor (6!=720), leaving more than

[
5.68\times10^{12}
]

potential states. Not every one need be reached for a particular (B), but the size explains why a standard row transfer, meet-in-the-middle, or coefficient DP is not a credible subsecond algorithm.

The equivalent column transfer has the same central obstruction: it must remember twelve partial color sets.

For comparison with the existing recurrence, an algorithm that still visits the measured (1{,}622) degree-5 classes has a total per-class budget of only

[
\frac{1\ {\rm second}}{1622}
\approx0.616\ {\rm ms}.
]

Therefore a subsecond evaluator has to avoid the degree-5 class list altogether. Faster canonicalization or the rooted recurrence alone cannot meet that target.

---

# 3. Even the globally symmetric aggregate transfer has a huge midpoint

One might hope that summing over all (B) first restores enough column symmetry to make a six-row transfer small. It does restore symmetry, but not nearly enough.

After three row-pairs, one coloring assigns a 3-subset (U_j\subset[6]) to each of the 12 columns, with every color occurring six times. The exact number of ordered such states is

[
A_3
===

[x_1^6\cdots x_6^6]
e_3(x_1,\ldots,x_6)^{12}
========================

3{,}718{,}394{,}156{,}400.
\tag{5}
]

Every state counted by (5) is reachable. To see this, form the incidence bipartite graph between the 12 columns and the six colors. Its degrees are (3) on the column side and (6) on the color side. Split each color vertex into two degree-3 vertices. The resulting 3-regular bipartite graph decomposes into three perfect matchings, yielding the three processed row-pairs.

For the square (F^2), there are (A_3^2) ordered pairs of such midpoint states. Even after quotienting by

* all (12!) column permutations, and
* independent (6!) color relabelings in the two colorings,

the number of midpoint orbits is at least

[
\boxed{
\left\lceil
\frac{A_3^2}{12!(6!)^2}
\right\rceil
============

55{,}681{,}239{,}039.
}
\tag{6}
]

An additional exchange of the two colorings would reduce this by at most another factor two.

Thus the obvious “sum over all skeletons while processing rows” algorithm still has **tens of billions of exact symmetry classes at its central layer**. This is a sharper obstruction than the raw (20^{24}) paired-state universe.

It does not rule out a signed Fourier or representation-theoretic contraction, because such a contraction could avoid enumerating states. It does rule out a direct orbit-state DP.

---

# 4. Why plain Burnside does not remove the 63,199 weights

Let (W) be the permutation module on unordered balanced cuts

[
S\mid S^c,\qquad |S|=6.
]

It has dimension

[
\dim W=\frac12\binom{12}{6}=462.
]

As an (S_{12})-module,

[
W
\cong
S^{(12)}
\oplus
S^{(10,2)}
\oplus
S^{(8,4)}
\oplus
S^{(6,6)}.
\tag{7}
]

A skeleton is a multiset of six such cuts. Therefore

[
\boxed{
\dim\left(\operatorname{Sym}^6W\right)^{S_{12}}
===============================================

63{,}199.
}
\tag{8}
]

This is not coincidentally equal to the number of skeleton orbits: invariant vectors in a permutation module have one basis vector per orbit.

So diagonalizing the one-row cut/matching kernel into the four summands in (7) does **not** leave only (4^6) numbers. The Kronecker and multiplicity spaces produced by coupling six rows restore a 63,199-dimensional invariant space.

This is an obstruction to a generic Burnside or character-table solution, not a formal lower bound for the one scalar in (2). A very special low-rank property of the coloring tensor could still make the scalar easier.

---

# 5. The outer square is already the known complement-product mechanism

The appearance of (F_6(Q_B)^2) is structurally the same as the Sade–McKay–Wanless decomposition for Latin squares. Their formula sums the product of the numbers of 1-factorizations of a regular bipartite graph and its bipartite complement, and their recurrence fixes an edge and removes a 1-factor containing it. ([Monash User Web Pages][1])

Here,

[
\overline{Q_B}\cong Q_B
]

because bipartite complementation merely swaps the two vertices in every complementary left pair. Consequently, up to the fixed conversion between ordered and unordered factorizations, your square is precisely the self-complement specialization of that mechanism.

McKay and Wanless found that the same graph-class method becomes untenable for unrestricted order (12), because there are more than (10^{11}) degree-6 regular bipartite graph classes on (12+12) vertices. Your 63,199-element special family is vastly smaller, but the comparison explains why no general factorization formula is known to collapse the computation. ([Monash User Web Pages][1])

Recent multisymmetric-function work still identifies the same underlying problem with algebraic formulas: the summation domains can become larger than the combinatorial set being counted, with very large positive and negative contributions cancelling only at the end. ([arXiv][2])

---

# 6. A cheap spectral or determinant invariant is insufficient

The complementary-pair family itself already contains adjacency-cospectral graphs with different factorization counts.

For (C=4), take the row supports

[
\begin{aligned}
B_1:\quad&
1678,;1238,;1457,;3467,\
B_2:\quad&
2468,;2368,;1258,;1356,
\end{aligned}
]

with columns numbered (1,\ldots,8). If (A_i) is the (8\times8) biadjacency matrix of (Q_{B_i}), then both have

[
\det(\lambda I-A_iA_i^\top)
===========================

\lambda^3(\lambda-16)(\lambda-4)^2
(\lambda^2-8\lambda+8),
]

but

[
F_4(Q_{B_1})=32{,}064,
\qquad
F_4(Q_{B_2})=29{,}952.
]

Thus even inside the special paired family, the numeric adjacency spectrum, singular spectrum, and ordinary determinant data do not determine the answer. Any successful direct formula must retain substantially more than low-rank or spectral information.

---

# 7. The one remaining plausible aggregation route

The object to attack is now unambiguously

[
[\Omega_x\Omega_y]\Phi(x,y)^{12},
]

with (\Phi) given by the 3,969-term decomposition (3).

The promising experiment is not to expand it. It is to contract it in a symmetry-adapted basis:

1. Work modulo two or more machine-word primes.
2. Decompose each balanced-cut module into the four sectors in (7).
3. Couple the six row factors in a balanced (3+3) contraction tree.
4. Record the exact ranks and sparsities of the intermediate intertwiner matrices.
5. Validate the entire contraction against the known (C=4) and (C=5) totals.

There are two possible outcomes.

* If the specific coloring tensor occupies only a few thousand sparse channels, the aggregate could be computed without producing the 63,199 individual (F_6) values. This is the only route here that could conceivably reach a subsecond-per-orbit amortized cost.
* If the contraction occupies most of the 63,199 invariant channels, or the channel maps are dense, then representation theory has only changed basis. In that case the rooted (2+4) and (3+3) factorization recurrences remain the practical exact method.

## Bottom line

[
\boxed{
\text{There is an exact one-shot aggregate, but no established fast evaluation of it.}
}
]

Specifically:

* **Individual (F_6(Q_B)<1) second:** no credible exact algorithm yet; standard direct transfers have a (20^{12}) midpoint, and any recurrence retaining the 1,622 degree-5 classes misses the time budget by orders of magnitude.
* **Aggregate without 63,199 separate evaluations:** yes algebraically, via (2)–(3).
* **Does that identity currently make the computation smaller?** Not by ordinary DP, orbit aggregation, Burnside, or spectral reduction. The globally symmetric midpoint already has at least (55.7) billion state orbits.
* **Remaining opening:** an exact low-rank contraction of the 3,969-term tensor (\Phi). Its channel ranks can be measured decisively on (C=4,5) before committing to the (C=6) calculation.

[1]: https://users.monash.edu.au/~iwanless/papers/LS11AC.pdf "Mck-p.dvi"
[2]: https://arxiv.org/html/2607.05214v2 "Counting partial Latin rectangles and tridimensional rook placements with multisymmetric functions"
