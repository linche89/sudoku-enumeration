I read `c6-mathematical-bottleneck-question.md` first and treated `STATUS.md` as the sole verified baseline. The request correctly isolates the missing object: the shared cut must survive both copies, and retaining target identity has so far made the frontier nearly injective.  The verified C=5/C=6 measurements, including the 38,801-dimensional middle layer, the target-aware failures, and the 1.761-billion reverse-gluing bound, are as stated in `STATUS.md`. 

My result is:

1. an exact fused coefficient-extraction identity for (K_k);
2. a flattening-rank lower bound against a source–target-pair frontier;
3. a concrete reachable C=6 source giving a **5,489,549,616-state exact lower bound** for that frontier model;
4. an algebra-dimension lower bound against materializing an ordinary global coherent configuration.

The numerical witness below is a new reproducible derivation, not yet part of the authoritative project state.

## 1. Exact fused within-band identity

Let a fully labelled grade-(k) source be

[
s=\bigl((S_i,T_i)\bigr)_{i=1}^{2C},
\qquad |S_i|=|T_i|=k,
]

where (S_i,T_i\subseteq[C]) are the columns already used by symbol (i) in the two copies.

Introduce four sets of commuting row-slot variables

[
p_1,\ldots,p_C,\quad q_1,\ldots,q_C,\quad
r_1,\ldots,r_C,\quad s_1,\ldots,s_C
]

and target variables (Y_{i,U,V}). Define

[
L_i=
\sum_{\substack{a\notin S_i\ b\notin T_i}}
Y_{i,S_i\cup{a},,T_i\cup{b}}
\bigl(p_a r_b+q_a s_b\bigr).
]

Then the complete labelled transition polynomial is exactly

[
\boxed{
\Phi_s(Y)=
\left[
\prod_{c=1}^{C}p_cq_cr_cs_c
\right]
\prod_{i=1}^{2C}L_i .
}
\tag{1}
]

Here ([m]F) denotes the coefficient of monomial (m) in (F).

### Why (1) is exact

For symbol (i), choosing (p_ar_b) puts (i) in the first row of the shared cut and assigns columns (a,b) in the two copies. Choosing (q_as_b) puts it in the second row.

Extracting exactly (\prod_cp_cq_cr_cs_c) enforces:

* each column occurs exactly once in each row of copy 1;
* each column occurs exactly once in each row of copy 2;
* the same (+)/(-) choice is used in both copies.

Thus the four row assignments are bijections and the cut is shared. Conversely, every shared-cut assignment contributes one term. Replacing (p_ar_b+q_as_b) by ((p_a+q_a)(r_b+s_b)) would permit independent cuts and would compute the wrong, linearized quantity.

For a symbol histogram (h=(h_{S,T})), identify symbol variables and use

[
\Phi_h(Y)=
\left[
\prod_cp_cq_cr_cs_c
\right]
\prod_{S,T}L_{S,T}(Y)^{h_{S,T}}.
\tag{2}
]

The multinomial coefficients arising from the powers are exactly the labelled contingency multiplicities.

### Equivalent cycle-factor form

For one copy, let

[
\mathcal M(S)=
\left{
a:[2C]\to[C]:
a_i\notin S_i,;
|a^{-1}(c)|=2\ \forall c
\right},
]

and similarly (\mathcal M(T)). Each (a) induces a perfect matching (P_a) on the symbols: the two symbols assigned column (c) form one edge.

Equation (1) is equivalent to

[
\boxed{
\Phi_s(Y)=
\sum_{\substack{a\in\mathcal M(S)\b\in\mathcal M(T)}}
2^{\kappa(P_a\cup P_b)}
\prod_i
Y_{i,S_i\cup{a_i},,T_i\cup{b_i}},
}
\tag{3}
]

where (\kappa) is the number of connected components of the union of the two perfect matchings.

Indeed, (P_a\cup P_b) is a disjoint union of alternating even cycles, including doubled edges as 2-cycles. A shared cut is precisely a two-colouring that reverses across every matching edge. Each component has two choices, hence (2^\kappa).

This proves that (1) retains the square, not merely the linear marginal.

### Reduced normalization

Let (G) be the full symbol/column/copy symmetry group, and suppose reduced coordinates store total mass on each orbit (O). Define

[
(J_kx)(s)=\frac{x_{[s]}}{|[s]|},
\qquad
(P_ky)(O)=\sum_{s\in O}y(s).
]

Then the normalization-safe reduced operator is

[
\boxed{\bar K_k=P_{k+1},K_k,J_k.}
\tag{4}
]

Thus (1) or (2), followed by (P_{k+1}), applies the exact reduced operator. This formulation automatically produces the state-dependent orbit-size factors needed at midpoint contraction.

As a normalization check, at grade zero,

[
\sum_{t}K_0(t,e)
================

\binom{2C}{C}(C!)^4
=(2C)!(C!)^2.
\tag{5}
]

For C=5 this is

[
52,254,720,000,
]

exactly the verified first-layer total in `STATUS.md`.  For C=6 it is

[
248,314,429,440,000.
]

## 2. The scalar frontier is small; its target payload is not

If (1) is evaluated symbol by symbol while discarding the target variables, a resource state after (t) symbols is four column masks ((P,Q,R,S)) satisfying

[
|P|=|R|=a,\qquad |Q|=|S|=t-a.
]

The ambient number of such states is

[
R_C(t)=
\sum_a
\binom Ca^2\binom C{t-a}^2.
\tag{6}
]

Consequently,

[
\sum_{t=0}^{2C}R_C(t)=\binom{2C}{C}^2.
\tag{7}
]

The peak and total scalar-resource dimensions are therefore:

| (C) | peak (R_C(t)) | total over all (t) |
| --: | ------------: | -----------------: |
|   5 |        21,252 |             63,504 |
|   6 |       263,844 |            853,776 |

At grade (k), a symbol has at most

[
2(C-k)^2
]

possible ((\text{side},a,b)) choices. This gives the following crude but realistic scalar-update ceilings:

| (C) | (k) | branching | scalar updates |
| --: | --: | --------: | -------------: |
|   5 |   1 |        32 |      2,032,128 |
|   5 |   2 |        18 |      1,143,072 |
|   6 |   2 |        32 |     27,320,832 |
|   6 |   3 |        18 |     15,367,968 |

So the shared cut and all four row-permanent resource constraints are not themselves the large state space. This is consistent with the verified 49,890-state scalar allocation DP. The explosion occurs when each resource coefficient carries enough target information to continue the square; the existing C=5 target-aware lift reaches 6,323,400 terminal states from 6,516,556 leaves. 

## 3. Exact support and rank lower bound for a joint source–target frontier

For a source-side mask list (S=(S_i)), form a (2C\times2C) matrix (B_S) whose columns are two distinguishable slots for every column (c):

[
B_S[i,(c,\epsilon)]=
\begin{cases}
1,&c\notin S_i,\
0,&c\in S_i.
\end{cases}
]

Every map (a\in\mathcal M(S)) has exactly (2^C) lifts to the distinguishable slots. Therefore

[
\boxed{
|\mathcal M(S)|=\frac{\operatorname{per}(B_S)}{2^C}.
}
\tag{8}
]

Since every pair of perfect matchings has at least one shared cut, and since the labelled target determines (a_i) and (b_i) by set difference, the exact labelled target support from (s=(S,T)) is

[
\boxed{
T(s)=
\frac{\operatorname{per}(B_S)\operatorname{per}(B_T)}{4^C}.
}
\tag{9}
]

Every one of these targets has a positive coefficient (2^\kappa).

### Universal permanent bounds

A valid grade-(k) state makes (B_S) and (B_T) (d)-regular with

[
n=2C,\qquad d=2(C-k).
]

Van der Waerden gives

[
\operatorname{per}(B_S)
\ge n!\left(\frac dn\right)^n,
]

while Bregman gives

[
\operatorname{per}(B_S)
\le (d!)^{n/d}.
]

After the (2^C) slot quotient, the following are rigorous brackets for the labelled target support of every source:

| (C) | (k) |            labelled support (T(s)) |
| --: | --: | ---------------------------------: |
|   5 |   1 |      (148,279,329) – (318,765,316) |
|   5 |   2 |            (470,596) – (3,265,249) |
|   6 |   2 | (3,327,559,225) – (16,002,756,004) |
|   6 |   3 |         (3,341,584) – (65,610,000) |

These are before source/target orbit aggregation.

### Flattening-rank theorem

Fix a labelled source (s), and let (\mathcal P_r(s)) be the extendable prefixes after processing (r) designated columns in each copy.

Consider any fixed-order linear column frontier whose final coordinates distinguish labelled targets. Split its coefficient tensor after those (r) columns. A full target determines the two symbols assigned every processed column in both copies. Hence distinct prefixes have disjoint completion-and-target supports.

Therefore the prefix/suffix coefficient flattening has linearly independent rows, over (\mathbb Q) or any odd characteristic:

[
\boxed{
\operatorname{width}_r\ge |\mathcal P_r(s)|.
}
\tag{10}
]

If the retained coordinate is the diagonal orbit of ((s,\text{partial target})), let (H_s) be the stabilizer of (s). The same proof gives

[
\boxed{
\operatorname{width}_r
\ge
|H_s\backslash\mathcal P_r(s)|.
}
\tag{11}
]

At the terminal layer,

[
\boxed{
\operatorname{width}_{\rm terminal}
\ge
\left\lceil \frac{T(s)}{|H_s|}\right\rceil .
}
\tag{12}
]

This is a rank/communication lower bound, not merely a claim that hash keys happen to be distinct. It is unaffected by cancellation or transition reordering within the fixed column order.

It is also deliberately scoped: it applies when the retained semantics distinguish the joint source–target orbit. It does **not** prove that (\bar K_k) itself has rank (T(s)), because a genuinely global transform may sum many source–target pair orbits directly into one target-only orbit coefficient. That is the remaining escape route.

## 4. A concrete reachable C=6 falsification witness

The following C=6 grade-2 source is reachable by two legal shared-cut bands. Symbols and columns are numbered (1,\ldots,12) and (1,\ldots,6).

The resulting source types, written (S/T), are

```text
16/46  35/16  35/23  46/45  12/24  24/56
16/13  25/16  34/35  45/23  36/25  12/14
```

A reachability certificate is:

```text
A1 = {1,2,4,7,8,10}
c1 = (1,3,5,4,1,4,6,2,3,5,6,2)
d1 = (4,6,3,5,4,6,3,1,5,2,2,1)

A2 = {4,5,7,8,10,11}
c2 = (6,5,3,6,2,2,1,5,4,4,3,1)
d2 = (6,1,2,4,2,5,1,6,3,3,5,4)
```

For each band and copy, every column occurs twice and its two symbols lie on opposite sides of the indicated cut. Also (c_{1,i}\ne c_{2,i}) and (d_{1,i}\ne d_{2,i}), so this is a valid grade-2 state.

All twelve ((S_i,T_i)) types are distinct. An exhaustive check of

[
S_6\times S_6\rtimes C_2
]

—(720^2) non-swapping transformations and (720^2) copy-swapping transformations—finds only the identity and no copy swap. Hence the full source stabilizer is trivial:

[
|H_s|=1.
]

Two independent exact permanent computations give

[
\operatorname{per}(B_S)=4,743,616,
\qquad
\operatorname{per}(B_T)=4,740,096.
]

Thus

[
|\mathcal M(S)|=\frac{4,743,616}{64}=74,119,
]

[
|\mathcal M(T)|=\frac{4,740,096}{64}=74,064,
]

and therefore

[
\boxed{
T(s)=74,119\cdot74,064
=5,489,549,616.
}
\tag{13}
]

Because (H_s) is trivial, these are also 5,489,549,616 distinct diagonal source–target orbits.

For a fixed (3+3) column split, minimizing over all choices of three columns in each copy gives

[
\min_{|J|=3}|\mathcal P^S_J|=6,488,
\qquad
\min_{|K|=3}|\mathcal P^T_K|=6,503,
]

so every such fixed split has flattening rank at least

[
\boxed{
6,488\cdot6,503=42,191,464.
}
\tag{14}
]

Thus:

* a fixed-order source–target-pair frontier already needs at least 42.2 million channels at the (3+3) split;
* its terminal pair space has 5.49 billion coordinates;
* even a 16-byte terminal record would require about 81.8 GiB before allocator, hash-table, or big-integer overhead.

For comparison, an analogous reachable trivial-stabilizer C=5 grade-2 witness has

[
|\mathcal M(S)|=|\mathcal M(T)|=960,
]

hence

[
T(s)=921,600,
]

and minimum fixed (3+3) split rank (419^2=175,561). The C=5-to-C=6 increase for this exact pair of witnesses is therefore about (5{,}957\times) at the terminal support.

This makes the C=5/C=6 assessment fairly sharp:

* the scalar resource frontier grows from about (2.1\times10^4) to (2.6\times10^5) peak states;
* a representative trivial-stabilizer target-pair frontier grows from under one million to over five billion.

The missing compression must therefore occur **after summing distinct source–target pair orbits into target-only orbit coefficients**, not by a better canonical representation of those pair orbits.

## 5. Consequence for global coherent-configuration gluing

For two (G)-orbits (O_i\simeq G/H_i) and (O_j\simeq G/H_j),

[
\dim \operatorname{Hom}_G
\bigl(\mathbb Q[O_j],\mathbb Q[O_i]\bigr)
=========================================

|H_i\backslash G/H_j|.
\tag{15}
]

When both stabilizers are trivial, this dimension is (|G|): the orbitals are indexed by relative group elements.

The verified reverse-gluing inventory contains 276 trivial-stabilizer two-row orbits. Their

[
\binom{276+1}{2}=38,226
]

unordered pairs account for

[
38,226\cdot46,080
=================

1,761,454,080
]

relative-placement classes. 

Therefore:

[
\boxed{
\text{Any coherent configuration retaining the full relative-placement
semantics has dimension at least }1,761,454,080.
}
\tag{16}
]

This is an algebra-dimension obstruction, not merely an implementation timing result. Passing to irreducible/Fourier blocks is only a basis change: for each regular block,

[
\sum_\lambda d_\lambda^2=|G|=46,080.
]

So an independent Fourier transform for every orbit pair does not remove the 1.761-billion total dimension. A surviving global incidence route must prove that the **specific downstream weighted response** lies in a much smaller common subspace across many pair blocks. Ordinary Burnside decomposition or a full orbital algebra does not supply that quotient.

The observed 99.4% canonical uniqueness is evidence that the four-row-output quotient may also be large, but it remains heuristic rather than a proof. 

## 6. Smallest exact falsification tests

### For the fused source–target frontier

The smallest C=6 falsifier is the witness above. It needs no C=6 state inventory and no transition expansion:

1. Verify the two cuts and four assignment vectors.
2. Check the (720^2\times2) colour/copy transformations; the stabilizer must be trivial.
3. Compute the two (12\times12) permanents by Ryser or subset DP.
4. Confirm (74,119), (74,064), and their product (5,489,549,616).

A proposed diagonal source–target frontier that claims fewer terminal equivalence classes for this source is exactly wrong. A fixed (3+3) column frontier claiming width below (42,191,464) is exactly wrong.

This test is far smaller than enumerating even one percent of the claimed frontier.

### For an alleged target-only implicit transform

The exactness gate remains:

1. For every C=3 and C=4 source/target pair, recover the two added-column maps by set difference.
2. Reject unless both maps have fibres of size two.
3. Form the two perfect matchings and assign coefficient (2^\kappa).
4. Aggregate with the orbit-total normalization (4).
5. Compare every canonical matrix entry with the retained explicit matrices.
6. Only after exact entrywise agreement, reproduce the verified C=5 total

[
1903816047972624930994913280000.
]

The C=5 middle map is (38,801\times38,801) with certified rational rank at least 1,024, so a successful transform must be a genuinely fast full-rank-style circuit, not a few-hundred-channel factorization. 

### For a proposed global reverse-gluing quotient

Enumerate all 46,080 placements of **one** trivial-stabilizer C=6 orbit pair and compute the exact number and rank of its distinct four-row incidence columns. This is the smallest exact check of a claimed relative-placement quotient. It does not require the remaining 38,225 pairs or the full four-row layer.

## Verdict

The coefficient-extraction identity (1) is an exact implicit contraction of the shared cut and all four permanent constraints, but its scalar efficiency does not extend to a monomial or diagonal source–target-pair representation. The explicit C=6 witness proves that such a representation can require **5.49 billion terminal classes for one reachable source**.

Likewise, the verified trivial-stabilizer inventory proves that an ordinary coherent-configuration realization of global reverse gluing has dimension at least **1.761 billion**.

The two avenues not ruled out are therefore very specific:

* a target-only orbit transform that algebraically sums many distinct source–target pair orbits before materializing them; or
* a global reverse-gluing operator whose downstream response has a provably small common Fourier/communication span across the regular orbit blocks.

Neither a different traversal of the pair states nor a materialized orbital basis can supply that compression.
