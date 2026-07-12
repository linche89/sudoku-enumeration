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

## Required next result

The next useful mathematical answer must provide one of:

1. an exact aggregation of `sum_(M containing e) F5(G-M)` that avoids the
   roughly 20,000 residual classes; or
2. an explicit symmetry-adapted 3+3 contraction for the global coefficient,
   including bases/projectors, intermediate ranks, sparsities, and rigorous
   operation and memory estimates.

The low-rank route must first produce exact C=4 and C=5 totals and measured
channel ranks before any C=6 implementation is justified.

Raw source material is preserved under `../expert/2026-07-12/`.
