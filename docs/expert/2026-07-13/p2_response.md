## Result

There is an exact (3+3) symmetry-adapted block contraction, with fully explicit vector spaces, projectors, and a very small **final** contraction. However, it does **not** currently give an independently buildable computation of (N(6)), because constructing the block entries remains large.

More importantly, exact (C=4) and (C=5) calculations show that the natural coloring intertwiners have **full channel rank**. Thus the proposed mechanism—“the coloring tensor occupies only a small part of the available invariant channels”—fails in both smaller nontrivial cases. For (C=6), representation dimensions impose no rank loss.

So the answer is:

> The (3+3) contraction can be written exactly in symmetry blocks, but the available evidence is against a useful low-rank collapse. This calculation does not independently establish the announced value of (N(6)).

---

# 1. Exact (3+3) contraction

Let (G=S_{12}), and let (\mathcal C) be the set of unordered balanced cuts

[
S\mid S^c,\qquad |S|=6.
]

Thus (|\mathcal C|=462), and let

[
W=\mathbb Q[\mathcal C].
]

As in the question,

[
W\cong
S^{(12)}\oplus S^{(10,2)}\oplus S^{(8,4)}\oplus S^{(6,6)}.
]

Split the six band rows into two triples. Define the midpoint set

[
\mathcal X=
\left{
(S_1,\ldots,S_{12}):
S_s\in\binom{[6]}3,\quad
#{s:a\in S_s}=6\ \text{for every }a\in[6]
\right}.
]

Its cardinality is the number from (7),

[
|\mathcal X|=3{,}718{,}394{,}156{,}400.
]

For a triple of cuts (a=(c_1,c_2,c_3)\in\mathcal C^3), define

[
u_a\in \mathbb Q^{\mathcal X}
]

as follows. For (\mathbf S=(S_1,\ldots,S_{12})), let (u_a(\mathbf S)) be the number of arrays

[
q_{r,s}\in[6],
\qquad r=1,2,3,\quad s=1,\ldots,12,
]

satisfying

[
{q_{1,s},q_{2,s},q_{3,s}}=S_s
]

for every symbol (s), and such that for each row (r),

[
s\longmapsto \bigl(c_r(s),q_{r,s}\bigr)
]

is a bijection from the twelve symbols to ({0,1}\times[6]).

Let

[
J:\mathbb Q^{\mathcal X}\longrightarrow\mathbb Q^{\mathcal X},
\qquad
J(S_1,\ldots,S_{12})=(S_1^c,\ldots,S_{12}^c).
]

If (a) is the left triple of cuts and (d) is the right triple, then the one-stack completion count is exactly

[
F(a,d)=\langle u_a,J u_d\rangle.
\tag{12}
]

Indeed, the three colors used in the second half must be the complement of the three colors used in the first half, symbol by symbol.

The map (a\mapsto u_a) is unchanged by permuting the three rows and by reversing either side of any cut. It therefore factors through

[
D=\operatorname{Sym}^3 W,
\qquad
\dim D=\binom{464}{3}=16{,}542{,}064.
]

Write this map as

[
U:D\longrightarrow \mathbb Q^{\mathcal X}.
]

With the inner product on (D) inherited from (W^{\otimes3}), define

[
M=U^*JU.
\tag{13}
]

Then

[
\boxed{
N(6)=2^6|M|_{\mathrm{HS}}^2.
}
\tag{14}
]

The factor (2^6) restores the two orientations of each unordered cut. Formula (14) is simply

[
N(6)
====

2^6
\sum_{a,d\in\mathcal C^3}
\langle u_a,J u_d\rangle^2.
]

This proves equality with the original coefficient (11).

A convenient exact basis for (D) is the divided-power basis indexed by multisets of three cuts. If a multiset has cut multiplicities (n_c), its diagonal Gram entry is

[
\frac{3!}{\prod_c n_c!}.
\tag{15}
]

Thus neither normalized basis vectors nor square roots are needed in modular arithmetic.

---

# 2. Complete (S_{12}) block decomposition

For a permutation with cycle type (\mu\vdash12), the character of (W) is

[
\chi_W(\mu)
===========

\frac12
\left(
[z^6]\prod_{\ell\in\mu}(1+z^\ell)
+
\mathbf 1_{\text{all }\ell\text{ even}},
2^{\ell(\mu)}
\right).
\tag{16}
]

The first term counts balanced subsets fixed setwise; the second counts cuts whose two sides are exchanged.

The symmetric-cube character is

[
\chi_D(g)
=========

\frac{
\chi_W(g)^3+
3\chi_W(g)\chi_W(g^2)+
2\chi_W(g^3)
}{6}.
\tag{17}
]

Exact Murnaghan–Nakayama decomposition gives

[
D\cong
\bigoplus_{\lambda\vdash12}
S^\lambda\otimes\mathbb Q^{m_\lambda},
\tag{18}
]

with:

| Quantity                                |       Exact value |
| --------------------------------------- | ----------------: |
| Nonzero (S_{12})-types                  |              (67) |
| (\sum_\lambda d_\lambda m_\lambda)      |  (16{,}542{,}064) |
| (\sum_\lambda m_\lambda)                |         (6{,}206) |
| (\max_\lambda m_\lambda)                |             (344) |
| (\sum_\lambda m_\lambda^2)              |   (1{,}081{,}862) |
| (\sum_\lambda m_\lambda^3)              | (242{,}094{,}680) |
| (\sum_\lambda m_\lambda(m_\lambda+1)/2) |       (544{,}034) |

The largest multiplicities begin with

[
\begin{array}{c|r}
\lambda&m_\lambda\ \hline
(6,4,2)&344\
(6,3,2,1)&340\
(5,4,2,1)&300\
(7,4,1)&271\
(7,3,2)&265
\end{array}
]

Because (M) commutes with (S_{12}), Schur’s lemma gives

[
M=
\bigoplus_\lambda
I_{d_\lambda}\otimes M_\lambda,
\tag{19}
]

where (M_\lambda) is an (m_\lambda\times m_\lambda) symmetric matrix. Therefore

[
\boxed{
N(6)
====

64\sum_{\lambda\vdash12}
d_\lambda,
\operatorname{tr}
\left(M_\lambda M_\lambda^T\right).
}
\tag{20}
]

This is the exact symmetry-adapted final contraction.

Once the (M_\lambda) are available, the final scalar is cheap:

* (544{,}034) stored entries if symmetry is used;
* approximately (4.15) MiB with 64-bit residues;
* fewer than (1.1) million modular arithmetic operations for the weighted Frobenius norm.

No dense block multiplication is needed: square each diagonal entry once and each off-diagonal entry once with weight two.

---

# 3. Explicit multiplicity bases and projectors

A half-skeleton is described by a histogram

[
h_u,\qquad u\in{0,1}^3,
]

with

[
\sum_u h_u=12,
\qquad
\sum_{u:u_r=1}h_u=6
\quad(r=1,2,3).
]

Quotienting by row permutations and independent reversals of the three cuts, namely by

[
C_2^3\rtimes S_3,
]

gives exactly

[
17
]

half-skeleton orbit types.

Choose representatives (\alpha=1,\ldots,17), and let (H_\alpha\leq S_{12}) be the stabilizer of the corresponding multiset of three cuts. Then

[
D\cong
\bigoplus_{\alpha=1}^{17}
\mathbb Q[G/H_\alpha].
\tag{21}
]

Consequently, a concrete multiplicity basis for type (\lambda) is

[
\mathcal M_\lambda
==================

\bigoplus_{\alpha=1}^{17}
(S^\lambda)^{H_\alpha}.
\tag{22}
]

There are two direct exact constructions.

The Reynolds projector is

[
R_{\lambda,\alpha}
==================

\frac1{|H_\alpha|}
\sum_{h\in H_\alpha}\rho_\lambda(h).
\tag{23}
]

More practically, over a prime (p>12), take an integral Specht or Murphy basis and solve

[
(\rho_\lambda(s)-I)v=0
\tag{24}
]

for a generating set (s) of (H_\alpha). This avoids enumerating the stabilizer and avoids the square roots that occur in orthonormal seminormal representations.

The isotypic projector on (D) is

[
E_\lambda
=========

\frac{d_\lambda}{12!}
\sum_{g\in S_{12}}
\chi^\lambda(g^{-1})\rho_D(g).
\tag{25}
]

Choose a (G)-adapted isometry

[
T_\lambda:
S^\lambda\otimes\mathbb Q^{m_\lambda}
\longrightarrow D.
]

Then the channel matrix is explicitly

[
\boxed{
M_\lambda
=========

\frac1{d_\lambda}
\operatorname{Tr}*{S^\lambda}
\left(
T*\lambda^*U^*JUT_\lambda
\right).
}
\tag{26}
]

In a nonorthogonal modular multiplicity basis with Gram matrix (G_\lambda), the contribution to (20) is computed as

[
d_\lambda,
\operatorname{tr}
\left(
G_\lambda^{-1}M_\lambda^T
G_\lambda M_\lambda
\right).
\tag{27}
]

Thus every projection and contraction map is available over machine-word finite fields without irrational coefficients.

A dense representation of all fixed vectors contains

[
\sum_\lambda d_\lambda m_\lambda
================================

16{,}542{,}064
]

field elements, requiring approximately (63.1) MiB with 32-bit residues or (126.2) MiB with 64-bit residues. This is not the bottleneck.

---

# 4. Why the reduction does not yet solve (C=6)

The color-invariant midpoint module is

[
H=
\left(\mathbb Q[\mathcal X]\right)^{S_6}.
]

Its character is computable by the exact Burnside formula

[
\chi_H(g)
=========

\frac1{6!}
\sum_{\tau\in S_6}
#{\mathbf S\in\mathcal X:g\mathbf S=\tau\mathbf S}.
\tag{28}
]

The resulting dimension is

[
\boxed{
\dim H=5{,}171{,}942{,}314.
}
\tag{29}
]

For every one of the 67 types occurring in (D),

[
m_\lambda(H)\geq m_\lambda(D).
\tag{30}
]

For example,

[
\begin{array}{c|r|r}
\lambda&m_\lambda(D)&m_\lambda(H)\ \hline
(12)&17&132\
(6,4,2)&344&44{,}479\
(6,3,2,1)&340&77{,}854\
(5,4,2,1)&300&74{,}135
\end{array}
]

Therefore no rank reduction is forced by representation dimensions. The midpoint contains enough copies of every irreducible type to make (U) injective.

There are only

[
49{,}755
]

balanced occupancy vectors (m=(m_S)_{S\in\binom{[6]}3}), and only

[
132
]

orbits of such occupancies under color permutations. But these numbers do **not** give a valid boundary basis: (u_\alpha(\mathbf S)) also depends on how the subset classes are aligned with the eight symbol-pattern cells of the half-skeleton. Two boundary words with the same occupancy can therefore have different coefficients.

Finally, the self-adjoint (S_{12})-commutant on (D) has (544{,}034) symmetric block coordinates. Full symmetry among all six rows restricts the particular (M) to the embedded space

[
\left(\operatorname{Sym}^6W\right)^{S_{12}},
]

of dimension (63{,}199). Constructing that restricted tensor coordinate by coordinate merely recreates the forbidden outer-skeleton enumeration.

So the hard step is not the block norm (20). It is the exact construction of the (M_\lambda) without recovering either the (63{,}199) full invariant coordinates or a still larger (3+3) orbital algebra.

---

# 5. Exact rank tests

## (C=4)

For (C=4),

[
D=\operatorname{Sym}^2W_8,
\qquad
\dim D=630.
]

The exact boundary contraction gives

[
|U^*JU|_{\mathrm{HS}}^2
=======================

1{,}821{,}030{,}450{,}462{,}720,
]

and hence

[
2^4|U^*JU|_{\mathrm{HS}}^2
==========================

29{,}136{,}487{,}207{,}403{,}520=N(4).
]

The exact integer matrix (U^*JU) has rank

[
630
]

modulo (1{,}000{,}003). Therefore it has full rank (630) over (\mathbb Q).

Its nonzero block ranks—equal to the full multiplicities—are

[
\begin{aligned}
&(8):3,\quad (7,1):1,\quad (6,2):5,\quad (5,3):1,\
&(5,2,1):2,\quad (4,4):4,\quad (4,3,1):1,\
&(4,2,2):3,\quad (3,3,1,1):1,\quad
(2,2,2,2):1.
\end{aligned}
]

The explicitly constructed (U)-matrix had (626{,}220) nonzero entries, density approximately (52.6%); it was not sparse.

## (C=5)

For the (2+3) split,

[
D_2=\operatorname{Sym}^2W_{10},
\qquad \dim D_2=8{,}001,
]

and

[
D_3=\operatorname{Sym}^3W_{10},
\qquad \dim D_3=341{,}376.
]

An exact meet-in-the-middle calculation gives

[
N(5)
====

1{,}903{,}816{,}047{,}972{,}624{,}930{,}994{,}913{,}280{,}000.
]

The algebra

[
\operatorname{End}*{S*{10}}(D_2)
]

has dimension (154). I formed the exact Gram operator in its 154-dimensional orbital algebra. The matrix of left multiplication by that operator has rank

[
154
]

modulo (1{,}000{,}003). Since

[
\operatorname{End}*{S*{10}}(D_2)
\simeq
\bigoplus_\lambda M_{m_\lambda},
]

full rank of left multiplication implies that every isotypic block is invertible. Hence the (2+3) coloring map has full row rank

[
\boxed{8{,}001}.
]

The measured channel ranks are

[
\begin{aligned}
&(10):3,\ (9,1):2,\ (8,2):6,\ (7,3):3,\ (7,2,1):3,\
&(6,4):6,\ (6,3,1):3,\ (6,2,2):4,\ (5,4,1):3,\
&(5,3,2):2,\ (5,3,1,1):1,\ (5,2,2,1):1,\
&(4,4,2):3,\ (4,3,2,1):1,\ (4,2,2,2):1.
\end{aligned}
]

The (C=5) validation harness uses the 355 full (C=5) skeleton orbits to verify the result. It is not proposed as the (C=6) algorithm.

These two exact tests give:

[
\begin{array}{c|r|r|l}
C&\text{source dimension}&\text{measured rank}&\text{result}\ \hline
4&630&630&\text{full}\
5&8{,}001&8{,}001&\text{full row rank}
\end{array}
]

Thus the natural coloring map does not merely occupy a small set of symmetry channels in either lower case.

---

# 6. The local cut transform is also full rank

There is a useful local diagnostic. Let (\mathcal P) be the (10{,}395) perfect matchings of twelve symbols, and define

[
B_{P,c}=
\begin{cases}
1,&P\text{ crosses the balanced cut }c,\
0,&\text{otherwise}.
\end{cases}
]

For cuts (c=S\mid S^c) and (d=T\mid T^c), put

[
i=\min{|S\cap T|,,6-|S\cap T|}.
]

Then

[
(B^TB)_{c,d}=i!(6-i)!.
\tag{31}
]

On the four irreducible summands of (W), its eigenvalues are

[
\begin{array}{c|r|r}
\text{module}&\text{dimension}&\text{eigenvalue}\ \hline
S^{(12)}&1&23{,}040\
S^{(10,2)}&54&1{,}920\
S^{(8,4)}&275&576\
S^{(6,6)}&132&360
\end{array}
]

All are nonzero, so

[
\operatorname{rank}B=462.
]

The four local spectral projectors are completely explicit:

[
P_\theta
========

\prod_{\theta'\neq\theta}
\frac{B^TB-\theta'I}{\theta-\theta'}.
\tag{32}
]

Hence there is no local cut-to-color channel elimination either.

---

# 7. Exact arithmetic and CRT

A rigorous bound independent of the announced target is

[
0\leq N(6)
<
\left(64(6!)^2\right)^{12}
==========================

33{,}177{,}600^{12}
<
2^{300}.
\tag{33}
]

Therefore five suitably chosen 61-bit primes whose product exceeds (2^{300}) suffice for reconstruction. A sixth prime provides a clean checksum.

Over each prime:

1. use divided-power bases with their diagonal Gram matrices;
2. use integral Specht or Murphy representations;
3. find stabilizer-fixed spaces by modular nullspaces;
4. form (M_\lambda);
5. evaluate (27);
6. sum with the Specht dimensions and multiply by (64).

Taking primes greater than (12) ensures that (12!), all stabilizer orders, and the divided-power Gram entries are invertible.

---

# Conclusion

The balanced (3+3) proposal has an exact and fairly elegant endpoint:

[
N(6)=64\sum_\lambda d_\lambda
\operatorname{tr}(M_\lambda M_\lambda^T).
]

Its final storage and arithmetic are tiny. But the actual coloring maps are not low-rank in (C=4) or (C=5), the local matching transform has full rank, and the (C=6) midpoint has enough multiplicity to accommodate every channel of (\operatorname{Sym}^3W).

Accordingly, the structural question has the following evidence-based answer:

> In the natural (3+3) channel decomposition, the coloring tensor does not exhibit low-rank support in the lower cases, and no symmetry-enforced low rank exists at (C=6). A successful independent computation of (N(6)) will need additional structure beyond ordinary irreducible-channel truncation.

This work does **not** independently recover the comparison value

[
38296278920738107863746324732012492486187417600000.
]

It supplies an exact reduction, exact rank obstruction, and reproducible (C=4) and (C=5) certificates.

* [Technical derivation](sandbox:/mnt/data/sudoku_symmetry_contraction/TECHNICAL_NOTE.md)
* [Download the complete code bundle](./sudoku_symmetry_contraction.zip)
* [Full (C=6) multiplicity table](./c6_block_multiplicities.csv)
* [Exact validation results](./RESULTS.txt)
