I use the notation and reductions in the uploaded statement.

## Main conclusion

I do **not** see convincing evidence that the full middle operator \(\bar K_2\) for \(C=5\) has polynomial or otherwise very low rank. There is, however, substantially more exact structure than the skeleton-by-skeleton implementation exposes:

1. Every individual transition weight is \(2^{\text{number of cycles}}\) for a pair of perfect matchings. This removes the explicit skeleton loop.
2. That cycle kernel has an exact factorization through the folded Johnson scheme, with a completely explicit spectrum.
3. \(K\) has an exact \(O(C^2)\)-sector spin/tensor representation.
4. Most promising computationally, one can avoid the \(D_k\)-dimensional joint propagation altogether: quotient the **entire skeleton sequence** by symmetry and count ordered 1-factorizations of a regular bipartite graph. There are exactly only
\[
   \boxed{63,199}
\]
   such outer symmetry classes for \(C=6\).

The fourth route is sufficiently different from the current transfer computation to serve as an independent verification.

There is also an important status correction. A secondary compilation records the candidate

\[
\boxed{38,296,278,920,738,107,863,746,324,732,012,492,486,187,417,600,000}
\]

for \(N(6)\), attributed to Pettersen, but explicitly marks it as unverified. The current OEIS entry still contains only the four terms \(C=2,3,4,5\). Thus “unknown” is best read as **no independently verified accepted value**, not “no numerical candidate exists.” ([Encyclopedia][1])

---

# 1. Exact cycle formula for every entry of \(K\)

Let a source joint state be

\[
u=(x_s,y_s)_{s\in[2C]}
\]

at grade \(k\), and let

\[
v=(x'_s,y'_s)_{s\in[2C]}
\]

be a proposed target at grade \(k+1\).

For a nonzero transition, there must be unique added columns

\[
x'_s\setminus x_s=\{a_s\},\qquad
y'_s\setminus y_s=\{b_s\}.
\]

Because both source and target are column-regular, each value \(c\in[C]\) occurs exactly twice among the \(a_s\)'s and exactly twice among the \(b_s\)'s. Therefore the fibers define two perfect matchings of the symbols:

\[
P_a=\bigl\{\{s:a_s=c\}:c\in[C]\bigr\},
\qquad
P_b=\bigl\{\{s:b_s=c\}:c\in[C]\bigr\}.
\]

Let \(\kappa(P_a,P_b)\) denote the number of connected components in the union \(P_a\cup P_b\), counting a common matching edge as a 2-cycle.

Then the exact matrix entry is

\[
\boxed{
K(v,u)=2^{\kappa(P_a,P_b)}
}
\]

provided the singleton-difference and regularity conditions hold; otherwise it is zero.

### Proof

A skeleton \(A\subseteq[2C]\) is valid for the \(X\)-placement precisely when each edge of \(P_a\) has one endpoint in \(A\) and one in \(A^c\). It is simultaneously valid for \(Y\) precisely when the same is true for every edge of \(P_b\).

The graph \(P_a\cup P_b\) is a disjoint union of alternating even cycles. On each connected component, the requirement “one endpoint from every edge” has exactly two solutions: choose either side of its bipartition. The choices are independent between components, giving \(2^\kappa\).

This also proves immediately that

\[
\boxed{M_A=M_{A^c}}.
\]

So skeletons can always be folded into complementary pairs, giving a free exact factor of two.

## Computational consequence

Instead of

1. iterating over \(A\),
2. generating all \(X\)- and \(Y\)-placements compatible with \(A\),
3. repeatedly reaching the same target for multiple common skeletons,

one may:

1. generate every degree-two added-column map \(a:[2C]\to[C]\) satisfying \(a_s\notin x_s\), once;
2. do the same for \(b\);
3. emit the joint target once, with weight \(2^{\kappa(P_a,P_b)}\).

For fixed source and target, \(a\) and \(b\) are unique, so this emits each labeled nonzero entry only once.

For unrestricted uniformly distributed perfect matchings, the average multiplicity removed is

\[
\frac{1}{(2C-1)!!}\sum_Q 2^{\kappa(P,Q)}
=
\frac{2^C C!}{(2C-1)!!}.
\]

Thus the unrestricted averages are

\[
C=5:\quad \frac{256}{63}\approx4.0635,
\qquad
C=6:\quad \frac{1024}{231}\approx4.4329.
\]

The allowed-column constraints will bias these averages, but a factor of two is guaranteed and a factor around four is a reasonable benchmark. This is not an exponential improvement, but it attacks duplication **between skeleton tasks**, which is not measured by the reported \(1.35\times\) within-task merge ratio.

---

# 2. The exact association scheme is the perfect-matching scheme

Let

\[
\mathcal M_C=\{\text{perfect matchings of }[2C]\}
\]

and let

\[
\mathcal B_C=
\bigl\{\{A,A^c\}: |A|=C\bigr\}
\]

be the set of unordered balanced bipartitions.

Define the incidence matrix

\[
R_{P,B}=
\begin{cases}
1,&\text{every edge of }P\text{ crosses }B,\\
0,&\text{otherwise}.
\end{cases}
\]

Then the cycle kernel

\[
H_{P,Q}=2^{\kappa(P,Q)}
\]

has the exact factorization

\[
\boxed{H=2RR^{\mathsf T}}.
\]

Indeed, \(P\) and \(Q\) have \(2^\kappa\) common **oriented** transversals, or \(2^{\kappa-1}\) unordered complementary pairs.

This kernel is an element of the Bose–Mesner algebra of the perfect-matching association scheme: its value depends only on the alternating-cycle type of \(P\cup Q\). That scheme and its orbital eigenspaces are studied explicitly in the literature. ([Algebraic Combinatorics][2])

## Folded Johnson form

For \(B=\{A,A^c\}\) and \(D=\{D,D^c\}\), put \(t=|A\cap D|\); replacing either representative by its complement replaces \(t\) by \(C-t\). Then

\[
\boxed{
(R^{\mathsf T}R)_{B,D}=t!(C-t)!.
}
\]

To see this, a matching crossing both bipartitions must pair

\[
A\cap D\quad\text{with}\quad A^c\cap D^c
\]

and

\[
A\cap D^c\quad\text{with}\quad A^c\cap D.
\]

There are \(t!\) and \((C-t)!\) choices respectively.

Thus \(R^{\mathsf T}R\) lies in the folded Johnson scheme on complementary \(C\)-subsets.

## Complete spectrum

The complementary-subset module decomposes multiplicity-freely as

\[
\bigoplus_{r=0}^{\lfloor C/2\rfloor}
S^{(2C-2r,2r)}.
\]

The dimension of the \(r\)-th component is

\[
d_r=
\binom{2C}{2r}-\binom{2C}{2r-1},
\]

and the eigenvalue of \(R^{\mathsf T}R\) there is

\[
\boxed{
\lambda_r=
2^{C-2r-1}
\frac{(C-r)!(2r)!}{r!}.
}
\]

The expression remains an integer in the endpoint case where the displayed power of two is \(2^{-1}\). Every \(\lambda_r\) is nonzero. Consequently,

\[
\boxed{
\operatorname{rank}R
=\operatorname{rank}H
=
\frac12\binom{2C}{C}.
}
\]

Numerically,

\[
\operatorname{rank}H=
\begin{cases}
126,&C=5,\\
462,&C=6.
\end{cases}
\]

For \(C=6\), the four nonzero components have

\[
\begin{array}{c|rrrr}
r&0&1&2&3\\ \hline
d_r&1&54&275&132\\
\lambda_r&23040&1920&576&360
\end{array}
\]

and \(H=2RR^{\mathsf T}\) therefore has nonzero eigenvalues

\[
46080,\quad3840,\quad1152,\quad720.
\]

So \(H\) satisfies a degree-five polynomial including its zero eigenvalue.

## What this proves—and what it does not

This gives a strong answer to B3:

* The **matching-fiber kernel** is exactly an association-scheme element.
* It has only \(\lfloor C/2\rfloor+1\) nonzero eigenspaces.
* Its full spectral factorization is explicit.
* All these channels are nonzero, so an exact “drop the high harmonics” truncation is impossible.

But its rank is exactly the number of complementary skeleton pairs. In that sense, the factorization \(H=2RR^{\mathsf T}\) spectrally recovers the original skeleton channel; it does not reveal an additional hidden low-dimensional bond below \(462\) at \(C=6\).

Most importantly, the matchings \(P_a\) and \(P_b\) are attributes of a **source-target pair**. There is no fixed map from a state to one perfect matching. Therefore \(\bar K\) is a state-dependent pullback of \(H\), not simply \(H\) acting on a matching coordinate. Hence

\[
\operatorname{rank}H=462
\]

does **not** imply

\[
\operatorname{rank}\bar K_k\le462.
\]

### The proposed Johnson commutation test needs modification

The ordinary Johnson adjacency changes one symbol's \(k\)-subset by deleting one column and inserting another. That changes two column degrees, so it does not preserve the column-regular state space. If \(P_{\rm reg}\) is the regularity projector and \(A_s\) is a Johnson adjacency acting on one symbol, then literally

\[
P_{\rm reg}A_sP_{\rm reg}=0.
\]

Thus “does \(\bar K\) commute with per-symbol Johnson adjacency?” is not a useful test as stated. One would need simultaneous two-symbol switches that preserve all column degrees. Those generate a larger balanced-switch or coherent-configuration algebra, which need not be commutative.

---

# 3. Evidence concerning B1: low rank

The first genuinely square middle case is already informative.

For \(C=3\), after identifying grade \(2\) with grade \(1\) by pointwise complementation, the three invariant states are indexed by the alternating-cycle types

\[
1+1+1,\qquad 2+1,\qquad 3.
\]

In one representative-to-orbit-sum normalization, the middle quotient matrix is

\[
\begin{pmatrix}
96&96&128\\
16&176&128\\
16&96&208
\end{pmatrix}.
\]

Its determinant and eigenvalues are

\[
\det=2,048,000,\qquad
\operatorname{spec}=\{80,80,320\}.
\]

Hence it has full rank \(3\).

This does not prove that the \(C=5\) middle matrix has full rank, but it shows that there is no persistent automatic collapse merely from the matching-cycle structure. The \(C=4\) rank-five phenomenon is indeed explained by the neighboring grade-five bottlenecks, not by an apparent general identity.

## A one-pass exact lower-bound experiment for \(C=5\)

A full modular rank calculation using black-box matvecs may require many expensive applications of \(\bar K_2\). A cheaper first experiment is a two-sided CountSketch.

Choose random sparse matrices over a large odd prime field,

\[
L\in\mathbb F_p^{m\times 76249},
\qquad
R\in\mathbb F_p^{76249\times m},
\]

with one random signed nonzero per column/row. During one normal transition-generation pass, accumulate

\[
S=L\bar K_2R.
\]

For every emitted entry \((i,j,w)\), this requires only

\[
S_{h_L(i),h_R(j)}
\mathrel{+}=
\sigma_i\tau_j w \pmod p.
\]

Then

\[
\operatorname{rank}_{\mathbb F_p}S
\le
\operatorname{rank}_{\mathbb F_p}\bar K_2
\le
\operatorname{rank}_{\mathbb Q}\bar K_2.
\]

Therefore, if an \(m\times m\) sketch has full rank, it is an exact certificate that

\[
\operatorname{rank}_{\mathbb Q}\bar K_2\ge m.
\]

Random collisions can only make the reported rank too small, never too large. Starting at \(m=256,512,1024\) would quickly decide whether “rank polynomial in \(C\) with a small constant” is plausible, at the cost of one transition sweep rather than hundreds or thousands of matvecs.

My current assessment is:

\[
\boxed{\text{B1 is unlikely, but E2 remains worth performing.}}
\]

---

# 4. An exact \(O(C^2)\)-sector tensor factorization

The cycle formula also has a binary-spin representation.

For a 2-regular bipartite multigraph with left vertices the \(X\)-columns, right vertices the \(Y\)-columns, and one edge \(a_s-b_s\) per symbol,

\[
2^{\kappa}
=
\sum_{\alpha,\beta\in\{\pm1\}^{C}}
\prod_s
\mathbf{1}\{\alpha_{a_s}=\beta_{b_s}\}.
\]

This simply counts the two constant spin choices on each connected component.

Let \(E_a^X\) add column \(a\) to the \(X\)-set of one symbol, when allowed, and define \(E_b^Y\) similarly. Put

\[
E_{\alpha,\pm}^X
=
\sum_{\substack{a\in[C]\\alpha_a=\pm1}}E_a^X,
\qquad
E_{\beta,\pm}^Y
=
\sum_{\substack{b\in[C]\\beta_b=\pm1}}E_b^Y,
\]

and

\[
T_{\alpha,\beta}
=
E_{\alpha,+}^X\otimes E_{\beta,+}^Y
+
E_{\alpha,-}^X\otimes E_{\beta,-}^Y.
\]

If \(P_k\) projects the \(2C\)-symbol tensor product onto grade-\(k\) column-regular states, then

\[
\boxed{
K_k
=
P_{k+1}
\sum_{\alpha,\beta\in\{\pm1\}^C}
T_{\alpha,\beta}^{\otimes 2C}
P_k.
}
\]

The global flip

\[
(\alpha,\beta)\mapsto(-\alpha,-\beta)
\]

leaves \(T_{\alpha,\beta}\) unchanged. After sandwiching by the two column-symmetry projections, all pairs having the same

\[
p=|\alpha^{-1}(+1)|,\qquad
q=|\beta^{-1}(+1)|
\]

are conjugate. Thus on the invariant space,

\[
\boxed{
\bar K_k
=
\sum_{p,q=0}^{C}
\binom Cp\binom Cq\,
\bar P_{k+1}
T_{p,q}^{\otimes2C}
\bar P_k.
}
\]

with at most \(O(C^2)\) distinct sectors, and approximately half that after the global flip.

This is a genuine compact algebraic representation and a concrete answer to the factorization portion of B2. Unfortunately, it does not by itself prove a cheap application algorithm. A symbol-by-symbol contraction still has to enforce two occupancy counts for each of \(2C\) columns; the naive frontier has exponential size. Moreover, the external target labels remain nearly injective, so the output-size obstruction remains.

The spin form is therefore best viewed as:

* a promising tensor-network representation;
* a way to search for further exact low-bond identities;
* not yet a proof of polynomial frontier dimension.

---

# 5. Exact answer to E1

A single-side reduced state, under the definition “multiset of column \(k\)-sets modulo \(S_C\),” is equivalently an unlabeled \(k\)-uniform multihypergraph on \(C\) vertices having

* \(2C\) hyperedges;
* degree \(2k\) at every vertex.

A Burnside enumeration gives:

\[
\begin{array}{c|l}
C&(s_0,s_1,\ldots,s_C)\\ \hline
2&(1,1,1)\\
3&(1,1,1,1)\\
4&(1,1,4,1,1)\\
5&(1,1,7,7,1,1)\\
6&(1,1,24,132,24,1,1)
\end{array}
\]

For \(k=2\), these are unlabeled loopless regular multigraphs: \(s_2=7\) for \(C=5\) and \(s_2=24\) for \(C=6\).

This also exposes an error in the suggested heuristic

\[
D_k\lesssim s_k^2.
\]

With the stated definition of \(s_k\),

\[
D_2(C=5)=76249
\quad\text{but}\quad
s_2(C=5)^2=49.
\]

The missing information is the symbolwise alignment between the two marginal multisets. That alignment is precisely where almost all joint-state complexity resides.

---

# 6. A different exact algorithm: global skeleton orbits and 1-factorizations

This is, in my view, the strongest practical result.

Let \(B\) be the \(C\times2C\) binary matrix whose \(I\)-th row is the indicator of \(A_I\).

Construct a bipartite graph \(Q_B\) as follows:

* left vertices are \((I,+)\) and \((I,-)\), for \(I\in[C]\);
* right vertices are the \(2C\) symbols;
* \((I,+)\) is adjacent to \(A_I\);
* \((I,-)\) is adjacent to \(A_I^c\).

Every left vertex has degree \(C\). Every symbol is adjacent to exactly one vertex from each band pair, so every right vertex also has degree \(C\). Hence \(Q_B\) is a \(C\)-regular bipartite graph with \(2C\) vertices on each side.

Let \(F(Q)\) be the number of proper edge-colorings of \(Q\) with the **labeled** colors \([C]\), equivalently the number of ordered 1-factorizations of \(Q\).

Then

\[
\boxed{\operatorname{stack}(A_0,\ldots,A_{C-1})=F(Q_B).}
\]

Indeed:

* at a left vertex, the \(C\) incident edges receiving all colors once is exactly the row-bijection constraint;
* at a symbol vertex, receiving all colors once is exactly the “symbol uses each stack-column once” constraint.

Therefore

\[
\boxed{
N(C)=\sum_B F(Q_B)^2.
}
\]

## Quotient the complete skeleton sequence

The value \(F(Q_B)\) is invariant under

* symbol permutations \(S_{2C}\);
* band permutations \(S_C\);
* independently complementing any row \(A_I\leftrightarrow A_I^c\).

Thus the relevant group is

\[
H_C=(C_2\wr S_C)\times S_{2C},
\]

and

\[
\boxed{
N(C)=\sum_{[B]\in H_C\backslash\mathcal B}
|\operatorname{Orb}(B)|\,F(Q_B)^2.
}
\]

This bypasses the grade-by-grade state space completely. It does not contradict the \(D_k\) floor, because that floor applies to propagation of an arbitrary invariant band-state vector; this calculation changes the summation order and is not such a propagation.

## Exact number of outer classes

A Burnside calculation over signed row-cycle types gives the number \(h_C\) of these global skeleton orbits.

Let \(z_\lambda=\prod_i i^{m_i}m_i!\). A conjugacy class in \(C_2\wr S_C\) is indexed by two partitions \((\lambda^+,\lambda^-)\), describing positive and negative signed cycles, with

\[
|\lambda^+|+|\lambda^-|=C.
\]

Let \(\mu\vdash2C\) be the symbol-permutation cycle type. For a positive signed row cycle of length \(a\), define

\[
P_a(\mu)
=
[x^C]
\prod_{b\in\mu}
\left(1+x^{b/\gcd(a,b)}\right)^{\gcd(a,b)}.
\]

For a negative signed row cycle, define

\[
Q_a(\mu)=
\begin{cases}
2^{\sum_{b\in\mu}\gcd(a,b)},
&
b/\gcd(a,b)\text{ is even for every part }b\in\mu,
\\[1mm]
0,&\text{otherwise}.
\end{cases}
\]

Then

\[
\boxed{
h_C=
\sum_{\substack{\lambda^+,\lambda^-\\
|\lambda^+|+|\lambda^-|=C}}
\ \sum_{\mu\vdash2C}
\frac{
\displaystyle
\prod_{a\in\lambda^+}P_a(\mu)
\prod_{a\in\lambda^-}Q_a(\mu)
}{
\displaystyle
2^{\ell(\lambda^+)+\ell(\lambda^-)}
z_{\lambda^+}z_{\lambda^-}z_\mu
}.
}
\]

Exact evaluation gives

\[
\begin{array}{c|rrrrr}
C&2&3&4&5&6\\ \hline
h_C&2&4&26&355&63,199.
\end{array}
\]

So the \(C=6\) outer problem has

\[
\boxed{63,199}
\]

objects—not millions of middle joint states and not \(924^6\) labeled skeleton sequences.

## Counting \(F(Q)\)

For a \(d\)-regular bipartite graph \(Q\), use

\[
F_0(\varnothing)=1,
\]

and

\[
\boxed{
F_d(Q)=
\sum_{M\in\operatorname{PM}(Q)}
F_{d-1}(Q-M).
}
\]

This chooses the perfect matching carrying the first remaining color. Canonicalize \(Q-M\) as an unlabelled bipartite graph and combine identical residuals:

\[
F_d(Q)
=
\sum_R
m_Q(R)F_{d-1}(R),
\]

where

\[
m_Q(R)=
\#\{M\in\operatorname{PM}(Q):
\operatorname{can}(Q-M)=R\}.
\]

The low-degree bases are especially cheap:

\[
F_1(Q)=1,
\]

and if \(Q\) is 2-regular with \(c(Q)\) cycle components,

\[
\boxed{F_2(Q)=2^{c(Q)}}.
\]

The implementation should therefore maintain one memo table shared by every outer skeleton class, keyed by the canonical residual bipartite graph.

### Prototype validation

I implemented a direct exact prototype of this reformulation. It produces:

\[
\begin{array}{c|r|r}
C&h_C&\displaystyle\sum_{[B]}|\operatorname{Orb}(B)|F(Q_B)^2\\ \hline
2&2&288\\
3&4&28,200,960\\
4&26&29,136,487,207,403,520.
\end{array}
\]

Thus the graph/1-factorization identity passes the exact \(C=2,3,4\) gates.

I also generated the \(355\) outer classes for \(C=5\). I did not complete the \(C=5\) total with my simple, unoptimized residual-graph canonicalizer; that remaining difficulty is the repeated 1-factorization count, not generation of the outer classes.

For a serious \(C=6\) implementation, the decisive optimizations are:

* canonical augmentation of the paired-cut graphs to generate the \(63,199\) representatives directly;
* a high-performance canonical form for residual bipartite graphs;
* grouping perfect matchings by the canonical form of \(Q-M\);
* a cache shared across all top-level classes and all degrees;
* exact modular runs with CRT, followed by an independent bignum check.

This method is especially attractive because its failure modes and code path are almost disjoint from the band-transfer engine.

---

# Final assessment of B1–B3

### B1 — Low rank

No convincing evidence supports polynomial rank. The first square middle map, at \(C=3\), is already full rank. The \(C=5\) modular sketch should be run before investing further, but my expectation is that its rank will be a substantial fraction of \(76249\).

### B2 — Short factorization

There is an exact short tensor factorization through binary spin sectors, and the direct cycle formula removes repeated skeleton enumeration. Neither currently proves a polynomial frontier, because column-regularity and nearly injective joint targets remain.

### B3 — Association scheme

Yes, exactly—but at the **transition matching fiber**:

\[
H_{P,Q}=2^{\kappa(P,Q)}=2RR^{\mathsf T}.
\]

Its folded-Johnson spectrum is explicit. All scheme components are nonzero, and the rank is exactly the number of complementary skeleton pairs. The scheme therefore gives a clean algebraic description and potentially a fast matching transform, but it does not by itself diagonalize the full \(\bar K_k\).

## Best route to \(N(6)\)

The highest-payoff sequence is:

1. Replace skeleton-repeated transition generation by the perfect-matching cycle formula and simultaneously accumulate a modular rank sketch of the \(C=5\) middle map.
2. Develop the independent global-orbit/1-factorization computation, validating it at \(C=5\).
3. Run its \(63,199\) \(C=6\) outer classes and compare the result—only afterward—with the unverified Pettersen candidate.

That global reordering, rather than a hidden low rank of \(\bar K\), currently appears to be the most plausible path to an independent exact \(N(6)\).

[1]: https://encyclopedia.pub/entry/29043 "Mathematics of Sudoku | Encyclopedia MDPI"
[2]: https://alco.centre-mersenne.org/articles/10.5802/alco.104/ "https://alco.centre-mersenne.org/articles/10.5802/alco.104/"
