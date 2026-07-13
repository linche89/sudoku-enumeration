# C=6 Current Mathematical Bottleneck

## What is solved

- The outer family has exactly 63,199 skeleton orbits.
- Rooted color normalization reduces perfect-matching work exactly.
- The degree-4 rooted split removes the cubic canonicalization layer.
- The first complete outer graph has
  `F6(G1) = 6986348258918400`.

## What remains

A low-symmetry outer graph produces about 20,000 nonisomorphic degree-5
residuals even after the degree-6 root pivot. Evaluating each residual is now
fast, but materializing the whole frontier is still too expensive.

There is an exact global formulation

```text
N(6) = [Omega_x Omega_y] Phi(x,y)^12,
```

and a 3,969-product-term Ryser representation of Phi. This eliminates the
63,199-loop algebraically, but no fast coefficient extraction is established.
Direct midpoint transfer and ordinary Burnside/character changes of basis have
state spaces larger than the orbit list.

## Audited future-twin route

There is a verified exact backend that attacks the single-graph wall directly. Let
`P` be the processed left vertices.  For every right column `j`, retain

```text
tau_P(j) = N(j) - P
K_j      = colors already used at j.
```

The state is the multiplicity table `n_P(tau,K)`, modulo the global color
group.  Columns with the same future neighborhood and unavailable colors have
bijections between all future completions, even if their processed
neighborhoods differed.  When the next left vertex is processed, each affected
group of multiplicity `m_g` receives a disjoint color subset of size `m_g`;
the exact labelled transition weight is `product_g m_g!`.

This recurrence does not enumerate perfect matchings or residual graphs. Its
normalization and grouped transitions pass brute-force differential tests;
the implementation agrees per outer class with the primary engine for complete
C=2..5 and independently reproduces the cold G1 value in 1592.867 seconds at
1.670 GiB peak RSS.

The actual second graph has now been measured under two five-minute, 3-GiB
bounds. `canonical-last` completed five layers and entered its central layer;
`canonical-first` completed only three. The better order accumulated 7,981,359
next states after processing 2,000,000 of 7,630,873 parents. Thus the route is
exact and implemented, but the low-symmetry central frontier remains open.

## Audited 3+3 result

For `D = Sym^3(W)`, where `W` is the 462-dimensional permutation module on
unordered balanced 6|6 cuts, the global contraction has the exact endpoint

```text
N(6) = 64 * sum_lambda d_lambda * tr(M_lambda M_lambda^T).
```

The C=6 block data have 67 nonzero irreducible types, maximum multiplicity 344,
and 544,034 symmetric block entries.  The final norm would be cheap if the
blocks were known, but constructing them remains the hard part.

The supplied exact programs were rebuilt and reproduced locally:

- C=4: exact `N(4)` and rank 630/630 modulo 1,000,003;
- C=5: exact `N(5)` and centralizer rank 154/154, implying cross-channel
  row rank 8001/8001;
- C=6: midpoint dimension 5,171,942,314, with every domain multiplicity
  accommodated by the corresponding midpoint multiplicity.

Thus the hoped-for small natural channel support is absent at C=4 and C=5,
and no representation-dimension loss is forced at C=6.  This is a decision
against ordinary low-rank channel truncation, not an impossibility theorem for
all implicit full-rank contractions.

## Required next result

The next project result must reduce the G2 central frontier. Useful candidates
must either lower its memory representation, add a provably safe quotient, or
avoid materializing the full middle layer. A longer G2 run and a spread-out
full-run sample are not justified until such a reduction passes the existing
C=2..5 and cold-G1 gates.

The 3+3 route should be reopened only if a new proposal explains how to build
or apply the full-rank block operator without materializing the 63,199 outer
coordinates or the larger midpoint algebra.

Raw source material is preserved under `../expert/2026-07-12/` and
`../expert/2026-07-13/`.  Reproduction evidence is in
`../reports/og2/c6-expert-routes-audit-20260713.md` and
`../reports/og2/future-twin-c6-20260714.md`.
