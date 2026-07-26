## Verdict

There **is** an exact source-aggregated, target-only arithmetic-circuit representation of the actual vector. However, it does **not** yield a scale-passing algorithm for the complete C=6 target vector.

The decisive new fact is:

[
\boxed{
\dim V_{6,2}=54{,}382{,}557,\qquad
\dim V_{6,3}=29{,}801{,}801{,}681.
}
]

Moreover, the actual reachable vector is positive on **every** one of these orbit coordinates. Thus the C=6 middle vector is not merely potentially large: it has exactly **29.8 billion nonzero reduced coordinates**.

This gives an output-size lower bound after all source weights have already been combined. It rules out any method that materializes the complete (y_{[t]}), even if it never represents a ((\text{source},\text{target})) pair. It does not rule out a bespoke global circuit that extracts only the final scalar.

The fixed-source and full-orbital obstructions in the question remain valid but deliberately narrower.  The Burnside computation below independently reproduces all verified C=2–5 joint-histogram dimensions in `STATUS.md` before producing the C=6 dimensions. 

---

## 1. Exact target-only representation of the actual vector

Let (n=2C), and introduce variables

[
X=(x_{ic}),\qquad Y=(y_{ic}),
\qquad i\in[n],\ c\in[C].
]

For a balanced cut (A\subset[n]), (|A|=C), define

[
B_A(X,Y)=
\operatorname{per}X[A,[C]]
\operatorname{per}X[A^c,[C]]
\operatorname{per}Y[A,[C]]
\operatorname{per}Y[A^c,[C]].
]

The one-band polynomial is

[
\boxed{
B_C(X,Y)=\sum_{\substack{A\subset[n]\|A|=C}}B_A(X,Y).
}
\tag{1}
]

Work in the commuting squarefree algebra

[
\mathcal Z_C=
\mathbb Z[X,Y]\big/
\left(x_{ic}^2,y_{ic}^2:i\in[n],c\in[C]\right).
\tag{2}
]

For a labelled grade-(k) state

[
s=((S_i,T_i))_{i=1}^{2C},
]

write

[
m_s=
\prod_{i=1}^{2C}
\left(\prod_{c\in S_i}x_{ic}\right)
\left(\prod_{d\in T_i}y_{id}\right).
]

Then the actual labelled coefficient after (k) bands is exactly

[
\boxed{
c_k(s)=[m_s],B_C(X,Y)^k
\quad\text{in }\mathcal Z_C.
}
\tag{3}
]

Consequently,

[
\boxed{
N(C)=
\left[
\prod_{i=1}^{2C}\prod_{c=1}^{C}x_{ic}y_{ic}
\right]B_C(X,Y)^C .
}
\tag{4}
]

This is already source-aggregated: no source row of (K_k) appears in (3). Successive closure is formally trivial,

[
P_0=1,\qquad P_{k+1}=B_CP_k,\qquad P_k=B_C^k.
\tag{5}
]

### Why it preserves the square

For one copy, each permanent assigns the symbols on one side of (A) bijectively to the (C) columns. The same cut (A) occurs in all four permanents, so the two copies cannot choose their cuts independently.

If a monomial specifies balanced maps (a,b:[2C]\to[C]), let (P_a,P_b) be the perfect matchings pairing the two symbols in every column fibre. A cut contributes precisely when it bipartitions both matchings. Since (P_a\cup P_b) is a union of alternating even cycles, its number of compatible cuts is

[
2^{\kappa(P_a\cup P_b)}.
]

Thus the coefficient in (1) is exactly the required cycle factor, not the one-copy marginal.

As a normalization check,

[
B_C(\mathbf 1,\mathbf 1)
========================

\binom{2C}{C}(C!)^4
=(2C)!(C!)^2.
\tag{6}
]

For C=5 this is (52{,}254{,}720{,}000), agreeing with the verified first-layer total. For C=6 it is

[
248{,}314{,}429{,}440{,}000.
]

### Orbit-total normalization

Let

[
\Gamma_C=
\left(S_{2C}\times S_C\times S_C\right)\rtimes C_2,
]

where the final (C_2) exchanges the copies. Since (B_C^k) is invariant under (\Gamma_C), (c_k(s)) is constant on an orbit (O). The orbit-total coordinate is

[
x_O=|O|,c_k(s),\qquad s\in O.
\tag{7}
]

For labelled kernel (K_k(t,s)), the corresponding reduced orbit-total kernel is

[
\boxed{
\overline K_k(T,S)=
\frac1{|S|}
\sum_{\substack{s\in S\t\in T}}K_k(t,s).
}
\tag{8}
]

Equations (3) and (7) therefore perform the exact orbit-normalized update

[
y_T=\sum_S x_S\overline K_k(T,S).
]

So (1)–(8) give the requested exact target-only circuit identity. The remaining issue is coefficient extraction, not correctness or closure.

---

## 2. The actual vector has full balanced-state support

A one-copy labelled grade-(k) state is equivalently a (2C\times C) zero-one matrix (U) satisfying

[
\text{row sum}=k,\qquad \text{column sum}=2k.
\tag{9}
]

A paired state is an ordered pair ((U,V)) of such matrices on the same symbol rows.

### Theorem

Every pair ((U,V)) satisfying (9) is reachable after (k) bands, with positive coefficient in (B_C^k).

### Proof

Regard (U) as the incidence matrix of a bipartite graph with:

* (2C) symbol vertices of degree (k);
* (C) column vertices of degree (2k).

For every column vertex, split its (2k) incident edges arbitrarily into two sets of (k), and replace the column by two degree-(k) clones. The result is a (k)-regular bipartite graph with (2C) vertices on both sides.

By König’s edge-colouring theorem, a (k)-regular bipartite graph decomposes into (k) perfect matchings. ([arXiv][1]) After merging each pair of column clones, every matching becomes a map

[
a_j:[2C]\to[C]
]

with one edge at every symbol and exactly two edges at every column. Thus (U) decomposes into (k) legal balanced maps. Apply the same construction independently to (V), obtaining maps (b_j).

For each (j), the two maps induce perfect matchings (P_{a_j}) and (P_{b_j}) on the symbols. Their union consists of even cycles and therefore has a bipartition. Each cycle bipartition places one endpoint of every matching edge on either side, so the resulting cut has exactly (C) symbols on each side and is compatible with both copies.

Hence the paired decompositions can be used as the (k) bands. Every chosen cycle bipartition has positive weight. ∎

Therefore source aggregation creates no zero-support compression:

[
\boxed{
\operatorname{supp}(x_k)
========================

{\text{all balanced paired grade-}k\text{ states}}/\Gamma_C.
}
\tag{10}
]

---

## 3. Exact Burnside count of the actual C=6 support

Let (F_{C,k}(\lambda,\mu)) be the number of one-copy matrices satisfying (9) that are fixed by a row permutation of cycle type

[
\lambda=(\lambda_1,\ldots,\lambda_r)\vdash 2C
]

and a column permutation of cycle type

[
\mu=(\mu_1,\ldots,\mu_s)\vdash C.
]

For a row cycle of length (a) and a column cycle of length (b), the corresponding (a\times b) cell block splits into

[
g=\gcd(a,b)
]

cell orbits. Selecting (q) of those cell orbits adds (qb/g) ones to every row in the row cycle and (qa/g) ones to every column in the column cycle. Hence

[
\boxed{
F_{C,k}(\lambda,\mu)=
\left[
\prod_i u_i^k\prod_jv_j^{2k}
\right]
\prod_{i,j}
\left(
\sum_{q=0}^{g_{ij}}
\binom{g_{ij}}q
u_i^{q\mu_j/g_{ij}}
v_j^{q\lambda_i/g_{ij}}
\right),
}
\tag{11}
]

where (g_{ij}=\gcd(\lambda_i,\mu_j)).

Let

[
z_\lambda=\prod_{\ell}\ell^{m_\ell(\lambda)}m_\ell(\lambda)!,
\qquad
c_\lambda=\frac{(2C)!}{z_\lambda},
]

and similarly (c_\mu=C!/z_\mu).

### No-copy-swap contribution

Burnside gives

[
D^{\mathrm{ns}}_{C,k}
=====================

\frac1{(2C)!(C!)^2}
\sum_{\lambda\vdash2C}
c_\lambda
\left(
\sum_{\mu\vdash C}c_\mu F_{C,k}(\lambda,\mu)
\right)^2.
\tag{12}
]

### Copy-swap contribution

For a copy-swapping element ((\sigma,\alpha,\beta)\tau), a fixed pair is determined by one matrix fixed by

[
(\sigma^2,\alpha\beta).
]

For every (\gamma=\alpha\beta), there are (C!) choices of ((\alpha,\beta)). If (\lambda^{[2]}) denotes the cycle type of the square of a permutation of type (\lambda), then

[
D^{\mathrm{sw}}_{C,k}
=====================

\frac1{(2C)!C!}
\sum_{\lambda\vdash2C}
c_\lambda
\sum_{\mu\vdash C}
c_\mu F_{C,k}(\lambda^{[2]},\mu).
\tag{13}
]

The full number of reduced orbit coordinates is

[
\boxed{
D_{C,k}=\frac{D^{\mathrm{ns}}*{C,k}+D^{\mathrm{sw}}*{C,k}}2.
}
\tag{14}
]

### Exact values

| (C) | (k) | one-copy labelled states | no-swap orbits | normalized swap contribution | full paired orbits |
| --: | --: | -----------------------: | -------------: | ---------------------------: | -----------------: |
|   5 |   2 |               56,586,600 |         76,249 |                        1,353 |         **38,801** |
|   6 |   1 |                7,484,400 |             11 |                           11 |             **11** |
|   6 |   2 |          154,700,988,750 |    108,699,012 |                       66,102 |     **54,382,557** |
|   6 |   3 |        3,718,394,156,400 | 59,602,052,194 |                    1,551,168 | **29,801,801,681** |

The complete C=6 reduced layer dimensions are therefore

[
\boxed{
1,\ 11,\ 54{,}382{,}557,\ 29{,}801{,}801{,}681,
54{,}382{,}557,\ 11,\ 1.
}
\tag{15}
]

The same computation gives

[
\begin{aligned}
C=2 &: 1,2,1,\
C=3 &: 1,3,3,1,\
C=4 &: 1,5,141,5,1,\
C=5 &: 1,7,38801,38801,7,1,
\end{aligned}
]

exactly reproducing every verified project dimension.

This calculation uses only the 77 conjugacy types of (S_{12}) and the 11 conjugacy types of (S_6). It does not enumerate C=6 states, source rows, transitions, or outer classes.

The independent verifier ran here in about 12.8 seconds and 306 MiB, and has SHA-256

```text
1af665670b2392558a03f7795f02d15c4e8f6e95fa71399e3ea74e4f749800c9
```

[Download the exact Burnside verifier](sandbox:/mnt/data/c6_joint_histogram_orbit_count.py)

---

## 4. Consequences for a target-only (x_2\mapsto y_3) transform

The C=6 update entering the midpoint has:

[
\begin{aligned}
\text{actual input support} &=54{,}382{,}557,\
\text{actual output support} &=29{,}801{,}801{,}681.
\end{aligned}
]

Every one of those output coordinates is nonzero.

Therefore any algorithm that returns the complete orbit-total vector (y_3) must emit at least

[
29{,}801{,}801{,}681
]

records, regardless of how aggressively it combines sources beforehand.

Values alone would occupy approximately:

| bytes per coordinate | values-only storage |
| -------------------: | ------------------: |
|                    8 |           222.0 GiB |
|                   16 |           444.1 GiB |
|                   24 |           666.1 GiB |
|                   32 |           888.2 GiB |
|                   64 |            1.73 TiB |

These figures exclude canonical keys, indices, sorting buffers, hash-table slack, and allocator overhead. The 16-byte line is already narrower than a general 165-bit final C=6 integer.

For comparison, the verified C=5 middle layer has only 38,801 coordinates. Thus the exact C=6 midpoint support is

[
\frac{29{,}801{,}801{,}681}{38{,}801}
\approx 768{,}068
]

times larger. At C=5 the output dimension was not the primary wall; the measured kernel support and contingency expansion were. At C=6 the output dimension itself becomes a hard scale gate.

So outcome 1 is ruled out under the contract:

> compute and materialize every (y_{[t]}), even while avoiding all source–target pairs.

A compressed arithmetic circuit for (y), or a circuit producing only the final scalar, is not ruled out.

---

## 5. A multi-source communication lower bound

Let (V_{C,k}) be the vector space of orbit-total grade-(k) vectors, so

[
\dim V_{C,k}=D_{C,k}.
]

Let (\iota) complement all used masks. Gluing independently computed left and right halves at grade (k) uses the bilinear form

[
\boxed{
G_k(u,v)=
\sum_O
\frac{u_O,v_{\iota(O)}}{|O|}.
}
\tag{16}
]

After clearing denominators by (|\Gamma_C|), its matrix is a weighted permutation matrix with nonzero stabilizer-order entries. Hence

[
\boxed{
\operatorname{rank}*{\mathbb Q}G_k=D*{C,k}.
}
\tag{17}
]

At the C=6 (3+3) cut,

[
\operatorname{rank}_{\mathbb Q}G_3
==================================

29{,}801{,}801{,}681.
\tag{18}
]

This differs from the earlier fixed-source rank theorem. Each argument (u) or (v) is already the complete aggregate of every history on its side. Therefore any bilinear communication factorization

[
G_3(u,v)=\sum_{j=1}^{m}L_j(u)R_j(v)
]

that is valid for arbitrary exact half-vectors must have

[
m\ge29{,}801{,}801{,}681.
]

The same statement gives widths 141 at the C=4 midpoint and 38,801 at the C=5 midpoint.

This rules out a small common separator/Fourier channel that is:

* linear or bilinear;
* independent of the particular numerical half-vector;
* compositional across the (3+3) cut.

It still does not rule out a circuit specialized to the one fixed all-unit-weight instance.

### Why no rank theorem can rule out the single fixed vector by itself

Once (x) is fixed, (K) restricted to (\operatorname{span}{x}) has rank at most one. A nonuniform circuit could store (Kx), or even (N(6)), as a constant.

Therefore a meaningful lower bound for the actual-vector-only contract must additionally specify at least one of:

* a uniform circuit family constructed from (C) and the local tensors;
* restrictions on allowed constants;
* a local/separator contraction model;
* correctness for symbolic or dual-number perturbations of the actual weights;
* monotonicity or another restricted arithmetic model.

Without such a model, an unconditional lower bound for the one fixed numerical input is mathematically impossible in the usual rank framework.

---

## 6. What remains of the reverse-gluing Fourier possibility

For a trivial-stabilizer two-row orbit, the orbit-total injection is uniform on the regular (G)-set. Consequently the actual distribution of relative placements is the constant function on (G). Its Fourier transform is supported only in the trivial representation.

Thus the (46{,}080)-dimensional regular block and the full (1.761)-billion-dimensional Hom space are not lower bounds for the actual uniform response. They only rule out materializing the unrestricted orbital algebra.

There is also an exact but presently non-algorithmic response-space reduction.

Let

[
V_2=\mathbb Q^{772}
]

be the C=6 two-row orbit-total space. Write

[
A:\operatorname{Sym}^2V_2\longrightarrow V_4
]

for exact (2+2) gluing, and let

[
B:V_4\times V_2\longrightarrow\mathbb Q
]

be the exact (4+2) join, with the same orbit-total injection/projection normalization as (8). Compose before materializing (V_4):

[
\boxed{
T(u,v)(w)=B(A(u,v),w).
}
\tag{19}
]

Then

[
T:\operatorname{Sym}^2V_2\longrightarrow V_2^*
]

has response dimension at most

[
\boxed{772.}
]

For the actual two-row vector (a),

[
N(6)=T(a,a)(a).
\tag{20}
]

Flattened over unordered source pairs, the complete projected kernel has at most

[
772\binom{772+1}{2}
===================

230{,}347{,}816
]

coefficients. If full three-block symmetry is incorporated, the number of independent cubic coefficients is at most

[
\binom{772+2}{3}
================

76{,}981{,}524.
]

Once this tensor were available, dense application would be on the order of (77)–(230) million exact multiply-adds, rather than billions of four-row coordinates.

However, this is only an exact specification of the right response object. It is not yet the requested construction: no method is presently supplied for obtaining those projected coefficients without traversing the known relative placements or an equivalent triple-incidence set.

For the completely fixed actual (a), the response (B(-,a)) spans only one vector, so merely announcing a one-dimensional response subspace would be tautological. The missing property is a compact way to evaluate that response on every implicitly generated four-row configuration.

---

## 7. Smallest exact falsification gates

### Explicit target-vector claims

Run the Burnside verifier. A C=6 method claiming to represent the complete grade-3 vector by fewer than

[
29{,}801{,}801{,}681
]

ordinary orbit coordinates is false. This test performs no band transitions.

### Common communication-channel claims

At C=4, form the 141-coordinate complement pairing (16). It is a weighted permutation matrix of exact rank 141. A claimed general midpoint channel of width below 141 is already falsified there.

The corresponding C=5 rank is exactly 38,801, and the C=6 rank is exactly 29,801,801,681.

### Actual-only structural circuit claims

Propagate dual numbers through the proposed circuit:

[
x\longmapsto x+\varepsilon e_O,\qquad \varepsilon^2=0.
]

At C=3 and C=4, compare both the actual output and every directional derivative against the retained explicit matrices. A construction that works only after substituting all actual constants, but fails these derivatives, is a fixed-value identity rather than a reusable transform.

### Projected reverse-gluing claims

The smallest relevant gate is C=5:

1. compose the verified (2+2) gluing and (4+1) response directly;
2. retain only the final response channels, never a four-row orbit key;
3. reproduce every verified class triple and (N(5));
4. report projected nonzeros, arithmetic operations, time, and peak memory.

Only after that exact gate should bounded C=6 pair intervals test whether direct projection into the 772 response channels removes the placement arithmetic rather than merely avoiding four-row storage.

---

## Conclusion

Two exact conclusions now hold after combining the actual source weights:

[
\boxed{
\text{The C=6 actual midpoint vector has exactly }
29{,}801{,}801{,}681
\text{ nonzero orbit coordinates.}
}
]

and

[
\boxed{
\text{Any compositional linear/bilinear }3+3\text{ communication space
has dimension at least }29{,}801{,}801{,}681.
}
]

Therefore a complete target-only vector is not a viable C=6 object, even though no source–target pairs are materialized.

The remaining viable contract is narrower still: a uniform circuit that extracts the final coefficient of (B_C^C), or constructs the directly projected (2+2+2) response tensor, without expanding the 29.8-billion midpoint basis and without enumerating the 1.761-billion relative placements. The present lower bounds do not rule out such a genuinely global scalar circuit.

[1]: https://arxiv.org/pdf/2105.05286 "https://arxiv.org/pdf/2105.05286"
