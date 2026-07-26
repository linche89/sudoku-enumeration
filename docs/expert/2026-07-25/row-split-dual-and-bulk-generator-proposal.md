# Two domain transfers for the C=6 wall: Brauer-algebra duality and a graph-canonization bulk generator

Date: 2026-07-25.  Companion script: `eps_dual_identity_check.py` (exact,
passes C=2 and C=3 in 5.7 s).

## ERRATUM (2026-07-26)

External review (2026-07-26) found substantive errors.  Corrections, each
independently re-verified by scripts archived in `../2026-07-26/`:

1. **Section 4.1 normalization is wrong.**  The unit-weight transition
   `t(x,y)` fails already at C=2 (unit mass 4 vs correct orbit mass 10).
   The correct recursion is `T_L(x) = m_x F_L(x)` with `m_x = |G_C|/s_x`,
   `T_{L+1}(y) = sum_x T_L(x) R(x,y)` where `R` sums the contingency
   coefficient `K_c(n) = prod c_w! / prod n_uv!` over feasible emissions,
   and `N(C) = sum_q (ell_q/m_q) T_C(q)^2` with `ell_q = (2C)!/prod c_w!`.
   The corrected recursion (due to the reviewer) reproduces N(2), N(3),
   N(4) exactly, with per-transition emissions [4], [80, 29],
   [4752, 4630, 712] and layer state counts [1,2], [1,5,4], [1,23,54,26]
   (`../2026-07-26/layer_dp_check.py`).
2. **Section 2's eps classes are not a new system.**  Via
   `B_{j,i} = 1_{i in A_j}` the eps quotient is exactly the existing
   complete-class system: verified 2 (C=2), 4 (C=3), 26 (C=4, Burnside +
   direct DFS cross-check, `../2026-07-26/eps_orbit_check.py`).  At C=6 the
   count is therefore exactly 63,199, not "~3e4..1e5".  The identity of
   Section 2 is a rediscovery of `N = sum w F^2` in split coordinates; the
   surviving new content of this note is Section 1 (the Brauer/O(2) rank
   theorem) only.  Note also Phi is orbit-constant but not
   orbit-separating (two C=3 classes share Phi = 48).
3. **Section 3's "four corners close everything" is withdrawn as a
   theorem.**  It is a heuristic map of known walls, not a proof that no
   other exact reorganization exists.
4. **Section 4.2's M_4 bracket [1e7, 1.4e9] is withdrawn.**  The 577-figure
   is n minus unique, not a collision-pair count; the sample is a
   correlated single-placement prefix, so birthday assumptions fail; the
   C=5 leaves-per-orbit ratio is not a C=6 bound; Pettersen's ">900
   million" is not acceptance evidence under project rules.
5. **Section 4.3's cost and disk numbers are withdrawn.**  They assumed the
   broken Section 4.1 normalization and ignored the permanent lower bound
   per(A) >= 902 for 4-regular 12x12 extension matrices.

The corrected, measured replacement analysis is in
`../2026-07-26/layer-dp-permanent-profile.md`.

This note contains one new exact theorem with a verified check, one closed
explanation of an existing measured constant, and one concrete bulk-algorithm
proposal whose every parameter is measurable by bounded probes before any
C=6 commitment.

## 1. Linearizing the cycle weight: the delta=2 Brauer form

For a perfect matching `P` on the `2C` symbols define

```text
v_P = (x) over pairs {i,i'} of P of (e0 x e1 + e1 x e0)  in  (R^2)^(x 2C).
```

Then for any two matchings `P`, `Q`:

```text
<v_P, v_Q> = 2^kappa(P u Q),
```

the exact per-band cycle factor.  This is the Gram form of the Brauer
algebra at loop parameter `delta = 2`, equivalently the `O(2)`-invariant
pairing on `(R^2)^(x 2C)`.

Immediate consequence: the rank of the per-band coupling matrix over all
`(2C-1)!!` matchings equals

```text
dim Inv_O(2)((R^2)^(x 2C)) = (1/2) C(2C, C),
```

because a rotation `theta` contributes `(2 cos theta)^(2C)` with mean
`C(2C,C)` and every reflection has trace 0.  At C=6 this is `924/2 = 462`.
The project's measured "matching/Johnson exact local rank 462/462" is
therefore not an empirical accident but an invariant-theory identity; that
route's negative result is now closed by a theorem, and the same formula
gives 126 at C=5 without any computation.

## 2. The row-split dual identity (new, verified)

Expanding `N(C) = [full monomial] B_C(X,Y)^C` in the side basis `e_eps`
(one `{0,1}` label per symbol per band) gives an exact dual decomposition.

For a side table `eps = (A_1, ..., A_C)`, `A_j in C([2C], C)`, define the
C-regular bipartite graph `G_eps` on symbols `[2C]` times slots
`[C] x {0,1}` with edge `(i, (j,s))` iff `(i in A_j) == (s == 0)`.  Let
`Phi(eps)` be the number of ordered proper C-edge-colorings of `G_eps`
(equivalently ordered 1-factorizations).  Then

```text
N(C) = sum over eps of Phi(eps)^2.
```

Sudoku reading: `eps` is the family of row splits (which symbols sit in the
top row of stack 0, per band); `Phi(eps)` is the per-stack column-assignment
count given the splits; the two stacks share `eps` and are otherwise
independent, hence the square.

Checks: exact by hand at C=2 (12 tables with Phi=4, 24 with Phi=2, total
288) and exact by the companion script at C=3 (28,200,960).  Formally the
identity is a regrouping of `B_C^C`, whose coefficient semantics the project
has already verified through C=4.  `Phi(eps) >= 1` always, by König
edge-coloring, mirroring the known full-support theorem.

The number of `eps` classes under `S_12 x (C_2 wr S_6)` is at least
`924^6 / (12! * 2^6 * 6!) ~ 28,081` and is of the same scale as the 63,199
column-side classes.  This is expected: the two decompositions are exact
mirrors.

## 3. What the duality closes

Every exact reorganization of the objective is one of four corners of a
2x2: which side is skeletonized (column-side or row-split-side) times which
representation carries the square (per-class scalar or layer vector).  All
four corners now have measured or provable walls:

| corner | object | wall |
|---|---|---|
| column-side skeleton, per-class scalar | 63,199 classes, `F_6(G)^2` | ~700 s and ~1e9 residuals per class, no reuse |
| row-split skeleton, per-class scalar | ~3e4..1e5 classes, `Phi(eps)^2` | same per-class transfer as future-twin (used-color-set states), the mirror wall |
| band-vector DP | layers 1, 11, 54,382,557, 29,801,801,681 | certified full support and ~5.5e9 per-state fan-out |
| symbol-vector DP | connectivity states | 83,776 states at C=5 symbol 3, then 1.2e9 pairs |

Conclusion: no further purely algebraic regrouping (basis change, Fourier
block, signed reordering, different skeleton) should be expected to remove
the arithmetic.  The remaining freedom is the *cost model of the elementary
step*, which is where the second transfer operates.

## 4. Domain transfer 2: graph canonization + external reduction

The reverse-gluing method note states the reopening requirement: "a bulk
four-row generator, delayed/external reduction, and a cheaper canonical
key."  All three exist in the graph-isomorphism / external-algorithms
domain, and together they satisfy the three constraints posed on 2026-07-25
(no per-leaf raw lookup, no `M_6 * 46,080` orbit expansion, cross-source
aggregation from the first composite stage).

### 4.1 The construction: global row-incremental layer DP

Work with the verified partial-configuration states (multisets of `2C`
occurrence masks) and the orbit-total value convention.  Compute layers
`L = 1, 2, 3, 4, 5, 6` rows globally, never per outer class:

```text
T_1[unique one-row orbit] = 1
T_{L+1}[y] = sum over x of T_L[x] * t(x, y),
t(x, y) = #{labelled one-row extensions of rep(x) landing in orbit [y]}
```

- Transitions need no stabilizers, transporters, orbit sizes, or double
  cosets: enumerate the labelled row extensions of one canonical
  representative, canonicalize each child, emit `(child key, T_L[x])`,
  and sort-reduce.  This is strictly simpler bookkeeping than the verified
  `2+2` omega_D machinery and is exact by the same
  enumerate-distinct-images cancellation.
- Cross-source aggregation is automatic at every layer: the sort-reduce
  merges all parents of a child before the child is ever extended.  No
  `(source, target)` pair is represented.
- No raw-key table and no `M * |G|` insertion sweep exist: the canonical
  key of a 24-vertex bipartite-with-structure state is computed per
  emission by partition-refinement canonization (nauty-class, microseconds)
  instead of a 46,080-image group scan.
- The 1,761,454,080 placement bound does not apply: it is a bound for the
  `2+2` double-coset route, and this construction never runs a `2+2` join.
- Per-leaf lookups do not exist: there are no contingency leaves, only
  emissions, and emissions are reduced by external sort, not probed against
  a resident table.
- The final layer lands directly in the 63,199 complete classes with exact
  `F_6(q)`; then `N(6) = sum w(q) F_6(q)^2` with the verified weights.

### 4.2 Size of the four-row layer: three independent signals

The feared object is `M_4`, the number of four-row coordinate orbits.

1. Birthday reanalysis of the project's own probe: the 100,000-leaf
   middle-pair prefix produced 99,423 distinct canonical keys, i.e. 577
   coincidences.  Under uniform draws this estimates an effective universe
   `C(100000, 2)/577 ~ 8.7e6`.  Caveats: single-placement locality and
   multiplicity bias push this low; it is a floor-scale, not a count.
2. C=5 concentration scaling: 122,166,792 leaves over 17,120 orbits is
   ~7,100 leaves per orbit; applying the same concentration to a ~1e13-leaf
   C=6 incidence gives `M_4 ~ 1.4e9` as a ceiling-scale.
3. Pettersen's historical "more than 900 million" lookup states sits inside
   the bracket `[1e7, 1.4e9]`, exactly where a materialized four-row layer
   would fall.

All three signals say `M_4` is an external-table object of 16-64 GB at
16-32 bytes per record, not an impossible one.  `M_3` (three-row orbits,
never counted in the project) is the other load-bearing unknown; the
expectation from the C=5 ladder is `M_3 << M_4`.

### 4.3 Bounded C=6 cost model

Let `ext_L` be the mean labelled one-row extensions of a layer-L
representative.  Total work is

```text
sum over L of  #orbits(L) * ext_L   canonicalizations + emissions,
```

dominated by layers 3 -> 4 -> 5, i.e. on the order of
`M_4 * (avg parents + avg children) ~ 1e11 +- 1` microsecond-scale
operations, plus one external sort per layer over at most a few times the
emission volume.  At `M_4 = 1e9` and 10 us per canonicalization this is
single-digit days on 24 threads and under ~100 GB of disk; at `M_4 = 1e7`
it is hours.  Every parameter (`M_3`, `M_4`, `ext_L`) is measurable by a
bounded probe before any full-layer commitment.

## 5. Decision gates

1. C=4 gate: global row-incremental DP reproduces all 26 `F(q)` and
   `N(4) = 29136487207403520`.
2. C=5 gate: reproduce all 355 `(orbit, multiplicity, F)` triples and
   `N(5)`; report emissions (predicted ~1e6..1e7 against the 122,166,792
   leaf baseline), canonicalizations, time, peak RSS.  This is the measured
   operation count requested on 2026-07-25.
3. C=6 bounded probes, in order: exact `M_3` by extending the verified
   772-orbit two-row layer one row with sort-reduce; unbiased
   capture-recapture estimate of `M_4` from disjoint placement
   neighborhoods (the existing probe was single-placement local).

No full C=6 layer generation is proposed until gates 1-3 pass.

## 6. Honest limits

- The dual identity and the 462 identity are proved; the C=5/C=6 cost
  numbers in section 4 are estimates until the gates run.
- The row-incremental DP is mathematically the same forward recursion the
  project trusts per class; the novelty is the global aggregation order and
  the canonization cost model, not new counting semantics.
- If `M_4` lands at the top of its bracket and `ext_4` is fat, the sort
  volume, not RAM, becomes the binding constraint; the C=5 gate measures
  the emission-to-orbit ratio that decides this.
