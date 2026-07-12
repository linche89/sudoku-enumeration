# Yes: a future-twin orbit recurrence

There is an exact simultaneous recurrence that never generates a perfect matching (M), never constructs (Q_B-M), and never calls (F_5) separately. It processes the vertices of (U) one at a time and retains only the information needed by the unprocessed part of the common parent.

The key compression is **stage-dependent**: two columns may be merged whenever they have the same neighborhood among the unprocessed (U)-vertices, even when they are not twins—or even interchangeable—inside the original (Q_B). This is strictly coarser than ordinary graph isomorphism.

The qualification is that the resulting frontier is not uniformly tiny. For a low-symmetry (C=6) test, a good elimination order gave a peak of about (3.0\times 10^4) very small states. Thus the method removes the expensive (20{,}000) separate (F_5) evaluations, but it does not prove a universal sub-(20{,}000) state bound for every (B).

## 1. A direct rooted coefficient formula

Write (e_{rj}) for the edge joining (j) to ((r,b_{rj})). A proper (C)-edge-coloring can be represented by an array

[
x_{rj}\in[C]
]

such that:

[
(x_{1j},\ldots,x_{Cj})
]

is a permutation for every column (j), and, for every ((r,\varepsilon)),

[
{x_{rj}:b_{rj}=\varepsilon}=[C].
]

Introduce variables (y_{j,c}), with (j\in[2C]) and (c\in[C]). For (u\in U), define

[
P_u(\mathbf y)
==============

\operatorname{per}\bigl(y_{j,c}\bigr)_{
j\in N(u),,c\in[C]
}.
]

A term of (P_u) assigns the (C) colors bijectively to the (C) edges incident with (u). Consequently,

[
F_C(Q_B)
========

\left[\prod_{j=1}^{2C}\prod_{c=1}^C y_{j,c}\right]
\prod_{u\in U}P_u(\mathbf y).
\tag{4}
]

For a root edge (e=(u_0,j_0)), with its designated color taken to be (1), replace (P_{u_0}) by

[
P_{u_0}^{e,1}(\mathbf y)
========================

y_{j_0,1}
\operatorname{per}
\bigl(y_{j,c}\bigr)_{
j\in N(u_0)\setminus{j_0},,
c\in[C]\setminus{1}
}.
]

Then, directly,

[
S(Q_B,e)
========

\left[\prod_{j,c}y_{j,c}\right]
P_{u_0}^{e,1}(\mathbf y)
\prod_{u\ne u_0}P_u(\mathbf y).
\tag{5}
]

Thus the desired quantity is a single squarefree coefficient of a product of twelve (6\times6) permanents.

This is a fixed-shape partial-Latin-rectangle coefficient extraction. Permanent and multisymmetric-function formulas of this general kind go back to MacMahon and Gessel; recent work extending them to partial Latin rectangles also emphasizes that the raw summation domains can become larger than the objects being counted, with substantial cancellation. ([arXiv][1])

## 2. The exact state

Let (P\subseteq U) be the set of (U)-vertices already processed. A partial coloring assigns colors to every edge incident with a vertex of (P), subject to:

1. every processed (u) uses every color once;
2. no column (j) has received the same color twice.

For each column (j), define

[
K_j={\text{colors already used at }j}\subseteq[C],
]

and its **future type**

[
\tau_P(j)=N(j)\setminus P.
\tag{6}
]

The future type is precisely the set of unprocessed (U)-vertices still adjacent to (j).

The state is the multiplicity table

[
n_P(\tau,K)
===========

#{j:\tau_P(j)=\tau,\ K_j=K}.
\tag{7}
]

Equivalently, it is the multiset

[
\mathcal X_P
============

{!{,(\tau_P(j),K_j):j\in[2C],}!}.
]

The following invariants hold automatically:

[
|K|=C-|\tau|
\quad\text{whenever }n_P(\tau,K)>0,
\tag{8}
]

and, for every color (c),

[
\sum_{\tau,K:,c\in K} n_P(\tau,K)=|P|.
\tag{9}
]

The second identity follows because every processed (u) uses color (c) exactly once.

### Color quotient

For the unrooted computation, states are taken modulo (S_C), acting by

[
K\longmapsto \pi(K).
]

For the direct rooted computation, color (1) is distinguished, so the acting group is (S_{C-1}).

A convenient canonical key is

[
\operatorname{can}(n)
=====================

\min_{\pi\in\Gamma}
\operatorname{sort}
\left(
\underbrace{(\tau,\pi K),\ldots,(\tau,\pi K)}
_{n(\tau,K)\text{ copies}}
\right),
\tag{10}
]

where (\Gamma=S_C) or (S_{C-1}).

## 3. Why the state is sufficient

Suppose two exact partial boundary configurations have the same table (7), after a color permutation. Their columns can be paired so that paired columns have the same ((\tau,K)).

For a paired pair (j,j'):

* they have exactly the same adjacency to every unprocessed (u), because their future types agree;
* exactly the same colors are unavailable at them, because their (K)-sets agree.

Therefore swapping paired columns gives a bijection between all future completions.

Crucially, the columns need not have had the same neighborhoods among the **processed** vertices. Those differences have already been consumed by the partial coloring and are accounted for in the state weight. This is why the quotient is larger than the automorphism group of (Q_B).

## 4. The transfer recurrence

Let (u\notin P) be the next (U)-vertex. Consider the affected groups

[
\mathcal G_u(n)
===============

{g=(\tau,K):u\in\tau,\ n(\tau,K)>0}.
]

Write

[
m_g=n(\tau,K).
]

Because (\deg(u)=C),

[
\sum_{g\in\mathcal G_u(n)}m_g=C.
\tag{11}
]

For every affected group (g=(\tau,K)), choose a set (A_g) of colors satisfying

[
A_g\subseteq[C]\setminus K,
\qquad
|A_g|=m_g,
\qquad
\bigsqcup_{g\in\mathcal G_u(n)}A_g=[C].
\tag{12}
]

Interpretation: the (m_g) columns in group (g) receive precisely the colors in (A_g).

For each (c\in A_g), create one updated column record

[
(\tau\setminus{u},K\cup{c}).
\tag{13}
]

Unaffected records remain unchanged. Equal resulting records are merged.

Because the original columns are labeled, the set (A_g) can be bijected to the (m_g) actual columns in

[
m_g!
]

ways. Hence the multiplicity of the grouped transition is

[
\mu(n,\mathcal A)
=================

\prod_{g\in\mathcal G_u(n)}m_g!.
\tag{14}
]

Let (T_u(n,\mathcal A)) denote the raw table obtained from (13). If (W_P([n])) is the total number of partial labeled colorings represented by the canonical state ([n]), then

[
\boxed{
W_{P\cup{u}}([n'])
;{+}{=};
W_P([n])
\sum_{\substack{\mathcal A\text{ satisfies }(12)\
[T_u(n,\mathcal A)]=[n']}}
\prod_{g\in\mathcal G_u(n)}m_g!
}.
\tag{15}
]

This is the simultaneous recurrence.

In matrix form, for a chosen order (u_1,\ldots,u_{2C}),

[
\mathbf w_{p+1}=T_{u_{p+1}}\mathbf w_p,
\qquad
F_C(Q_B)
========

\mathbf e_{\rm term}^{\mathsf T}
T_{u_{2C}}\cdots T_{u_1}
\mathbf e_{\rm init}.
\tag{16}
]

For the rooted calculation, the first transfer is replaced by the root-restricted transfer described below.

## 5. Rooted version

Take (u_0) first. Before its transfer, distinguish the root column (j_0) from the other columns, even when it shares their future type.

Force

[
j_0\longmapsto 1.
]

The remaining (C-1) columns incident with (u_0) receive colors (2,\ldots,C) bijectively. After this first transfer:

* the root-column marker can be erased;
* color (1) remains marked;
* subsequent states are quotiented by (S_{C-1}).

The terminal weight is exactly

[
S(Q_B,e).
]

For the fastest implementation, it is usually better to use the full (S_C) quotient, compute (F_C(Q_B)), and then return (F_C(Q_B)/C). The substantive computation is still recurrence (15), not the identity (S=F/C).

## 6. Correctness

Every complete edge-coloring gives a unique path through the recurrence:

* when (u) is processed, the colors appearing on the columns in a group (g) determine (A_g);
* the actual bijection between those colors and the labeled columns contributes one of the (m_g!) choices;
* condition (c\notin K) guarantees that no color is repeated at a column.

Conversely, a sequence of transitions chooses a bijection between colors and incident edges at every (u). At termination, every column has received (C) distinct colors. Since there are exactly (C) colors, each column has all colors exactly once.

Thus the transitions are in weight-preserving bijection with proper labeled edge-colorings. The root restriction forces the designated first color class to contain (e), so its terminal weight is (3).

## 7. Number of transitions per state

Suppose the affected group multiplicities are (m_1,\ldots,m_q). Ignoring the availability restrictions (c\notin K_i), the number of grouped allocations is

[
\binom{C}{m_1,\ldots,m_q}
=========================

\frac{C!}{\prod_i m_i!}.
\tag{17}
]

Restrictions only decrease this number. Therefore

[
a(n,u)\le C!.
\tag{18}
]

The weight per allocation is (\prod_i m_i!). In particular, the recurrence processes at most

[
24,\quad120,\quad720
]

allocations per state for (C=4,5,6), respectively.

No permanent larger than (6\times6), graph isomorphism test, or recursive (F_5) evaluation occurs inside a transition.

## 8. State-space estimate and elimination order

For a fixed processed set (P), let

[
m_\tau(P)=#{j:\tau_P(j)=\tau}.
]

Every column in a (\tau)-group has a used-color set of size

[
k_\tau=C-|\tau|,
]

so there are

[
q_\tau=\binom C{k_\tau}
]

possible (K)-sets. Before imposing (9), reachability, or the color quotient, the number of possible tables is bounded by

[
\Phi(P)
=======

\prod_{\tau}
\binom{q_\tau+m_\tau(P)-1}{m_\tau(P)}.
\tag{19}
]

A tighter exact ambient count, still before reachability and the (S_C)-quotient, is

[
\left[
x_1^{|P|}\cdots x_C^{|P|}
\right]
\prod_{\tau}
h_{m_\tau(P)}
\left(
\left{
\prod_{c\in K}x_c:
|K|=C-|\tau|
\right}
\right),
\tag{20}
]

where (h_m) is the complete homogeneous symmetric polynomial.

### Choosing the order

Processing whole complementary pairs consecutively is generally not optimal. Since there are only (12) vertices in (U), an order can be chosen by a (2^{12})-state minimax calculation.

Let

[
d(\varnothing)=\log\Phi(\varnothing)
]

and

[
d(P)
====

\min_{u\in P}
\max\left{
d(P\setminus{u}),
\log\Phi(P)
\right}.
\tag{21}
]

An argmin traceback produces an order intended to minimize the largest frontier. Its cost is only

[
O(12\cdot2^{12})
]

evaluations of (19). Exact reachable-state counts can subsequently refine this order by beam search over subsets (P).

## 9. Why processing complete pairs can fail

After processing three complete complementary pairs, every column has exactly three used colors. A midpoint state then has the form

[
n_{\beta,K},
\qquad
\beta\in{0,1}^3,
\qquad
K\in\binom{[6]}3,
\tag{22}
]

where (\beta) is the column’s remaining three-bit future type.

If the (\beta)-stratification is completely forgotten, the possible color-set multisets are counted by

[
\left[
z^{12}x_1^6\cdots x_6^6
\right]
\prod_{K\in\binom{[6]}3}
\frac{1}{1-z\prod_{c\in K}x_c}.
\tag{23}
]

This coefficient is

[
49{,}755.
]

After quotienting by (S_6), Burnside’s lemma gives only

[
132
]

orbits. So color symmetry and complete column interchangeability would indeed give a very small middle space.

The obstruction is the future-type stratification (\beta). For example, when the six nonempty future-type classes have multiplicities

[
(3,3,2,2,1,1),
]

the analogous ambient Burnside count is

[
46{,}031{,}591
]

color orbits before reachability constraints. This explains why a naive three-pairs-versus-three-pairs split can explode even though the unstratified skeleton space has only (132) types.

Interleaving the individual (U)-vertices avoids this balanced midpoint.

## 10. A (C=6) prototype profile

On one random balanced, low-symmetry test matrix—not the unspecified (B) from the question—the order selected from the future-type bound produced the unrooted state counts

[
\begin{aligned}
(N_0,\ldots,N_{12})
={}&(
1,,
1,,
137,,
2129,,
23097,,
30421,,
13874,\
&11495,,
7262,,
1445,,
20,,
1,,
1).
\end{aligned}
\tag{24}
]

Thus:

[
N_{\max}=30{,}421,
\qquad
\sum_pN_p=89{,}884.
]

The crude allocation bound is therefore

[
720\sum_{p=0}^{11}N_p
<
6.5\times10^7
]

grouped allocation attempts; availability restrictions make the actual number lower.

A state can be packed as twelve sorted records ((\tau,K)). With a 32-bit record and a 128-bit weight, the raw peak storage in this example is only a few megabytes; allowing for open-addressing hash-table overhead, a budget of roughly (10) MB per active layer is conservative.

Exact counts fit in unsigned 128-bit arithmetic because

[
F_6(Q_B)\le(6!)^{12}=720^{12}<2^{114}.
\tag{25}
]

## 11. Implementation pseudocode

```text
function CountFactorizations(B, order):
    # Full S_C quotient. For direct rooting, use S_{C-1}
    # and apply the special root transition first.

    initial_records = []
    for column j:
        tau = N(j)                    # no U vertex processed
        initial_records.append((tau, empty_set))

    DP = { Canonical(initial_records) : 1 }

    for u in order:
        NEXT = empty hash map

        for (state, weight) in DP:
            groups = multiplicity_table(state)

            affected = []
            unaffected_records = []

            for ((tau, K), m) in groups:
                if u in tau:
                    affected.append((tau, K, m))
                else:
                    append m copies of (tau, K)
                        to unaffected_records

            multiplicity =
                product factorial(m) over affected groups

            Search(i, remaining_colors, updated_records):
                if i == len(affected):
                    if remaining_colors is not empty:
                        return

                    raw = unaffected_records + updated_records
                    key = Canonical(raw)
                    NEXT[key] += weight * multiplicity
                    return

                (tau, K, m) = affected[i]

                for A subset of (remaining_colors \ K)
                      with |A| = m:

                    records2 = updated_records

                    for c in A:
                        records2.append(
                            (tau \ {u}, K union {c})
                        )

                    Search(
                        i + 1,
                        remaining_colors \ A,
                        records2
                    )

            Search(0, [C], empty list)

        DP = NEXT

    return weight of the unique terminal state
```

For (C=4,5), the same code applies unchanged. The order optimizers have only (2^8=256) and (2^{10}=1024) processed subsets, and the local branching bounds are (24) and (120).

## Conclusion

This provides all of the requested ingredients:

* a precise state (n_{\tau,K});
* an exact sparse transfer recurrence (15);
* a rooted version evaluating (S(Q_B,e)) directly;
* a proof by a weight-preserving bijection;
* an explicit state-space bound and an order-selection recurrence;
* (O(C!\sum_pN_p)) grouped transitions and (O(N_{\max})) memory;
* a direct (C=4,5,6) implementation.

It is a genuine aggregation over the common parent: no perfect matching (M) and no residual (Q_B-M) is ever materialized. The stage-dependent future-twin quotient also captures equivalences unavailable to ordinary graph isomorphism.

What it does **not** establish is a (B)-independent theorem that (N_{\max}\ll19{,}850). For generic low-symmetry (B), the exact frontier may still be on the order of tens of thousands. The gain is that these are tiny boundary tables with at most (720) local extensions, rather than tens of thousands of separately evaluated degree-five residual graphs.

[1]: https://arxiv.org/html/2607.05214v1 "Counting partial Latin rectangles and tridimensional rook placements with multisymmetric functions"
