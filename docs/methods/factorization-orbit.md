# Factorization-Orbit Method

`src/factorization_orbit.cpp` is the primary exact 2xC engine.

For every outer skeleton B it constructs the C-regular bipartite graph Q_B and
computes its ordered one-factorization count F_C(Q_B). The final identity is

```text
N(C) = sum_[B] labelledMultiplicity(B) * F_C(Q_B)^2.
```

The key rooted recurrence is

```text
F_d(G) = d * sum_(perfect matchings M containing e) F_(d-1)(G-M).
```

At degree four the `rooted4` path uses the exact two-color split

```text
F_4(G) = 6 * sum_H 2^(c(H) + c(G-H)),
```

where H is a spanning 2-factor containing a fixed root-edge pair. Ternary
right-degree states guide output enumeration, while two rollback disjoint-set
structures count cycles without constructing cubic residual graphs.

## Exact gates

```powershell
.\build\factorization_orbit.exe 2 pivot rooted4
.\build\factorization_orbit.exe 3 pivot rooted4
.\build\factorization_orbit.exe 4 pivot rooted4
.\build\factorization_orbit.exe 5 pivot rooted4
```

Every command must end with `[OK]` for its known value.

## C=6 checkpoint verification

```powershell
.\build\factorization_orbit.exe 6 limit=1 canonbudget=100 `
  pivotinner parallelparents rooted4 parentchunk=128 `
  checkpoint=data\checkpoints\factorization_orbit_c6_graphmemo.bin `
  checkpointreadonly
```

The expected first value is `6986348258918400`. Never verify a preserved
checkpoint without `checkpointreadonly`.

Full measurements and failed alternatives are retained in
`../reports/og2/factorization-c6-20260712.md`.
