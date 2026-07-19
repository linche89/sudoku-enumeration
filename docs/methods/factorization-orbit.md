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

## Bounded F4-lookup coverage diagnostic

`f4coveragecheck` expands two successive rooted matching levels and
differentially compares the resulting exact value with the ordinary engine.
It is a permanent C=4 short gate:

```powershell
.\build\factorization_orbit.exe 4 f4coveragecheck `
  f4coveragemaxstates=100000 f4coveragemaxrecords=1000000 `
  pivot rooted4
```

At C=6 the same split is diagnostic only: it computes the exact number of
uncolored spanning two-factors, samples canonical degree-five parents, and
measures degree-four graph-memo coverage.  It never inserts memo values or
accumulates `F6`/`N(6)`.  A probe requires all of:

```text
checkpoint=EXISTING_PATH checkpointreadonly
positive limit=, f4coverageparents=,
f4coveragemaxstates=, and f4coveragemaxrecords=
```

The hard maxima are 64 classes, 1,000 sampled parents, 2,000,000 states, and
100,000,000 records.  Strong canonical keys are mandatory; a fallback aborts
the probe and requires a larger `canonbudget`.  The exact gates and negative
later-class reuse decision are in
`../reports/og2/f4-lookup-coverage-20260719.md`.

Full measurements and failed alternatives are retained in
`../reports/og2/factorization-c6-20260712.md`.
