# C=6: Can the actual multi-source vector be contracted target-only?

We seek an independent exact verification of the historical value of `N(6)`
for 2x6 Sudoku.  The objective is a weighted square over 63,199 outer classes,
not the easier linear sum.  Complete C=2..5 values and two individual C=6
outer classes are verified, but no global C=6 implementation has passed its
scale gate.

This question follows a new negative result that substantially narrows the
remaining problem.  Please do not propose another fixed-source enumeration;
the issue is now whether different sources can be aggregated *before* their
individual target distributions are represented.

## Newly certified obstruction

Fix a fully labelled grade-`k` source

```text
s = ((S_i,T_i))_(i=1..2C),  |S_i|=|T_i|=k.
```

Let `M(S)` be the balanced maps from the `2C` symbols to `C` columns that avoid
the used masks `S_i`, with exactly two symbols mapped to every column; define
`M(T)` similarly.  For fixed `s`, every pair of new maps determines a unique
labelled target, and the target recovers that pair by set difference.  All
cycle weights are positive.  Therefore the exact terminal support is

```text
T(s) = |M(S)| |M(T)|,
|M(S)| = permanent(B_S) / 2^C.
```

At any fixed column cut, distinct extendable prefixes have disjoint nonzero
completion-and-target support.  Hence the corresponding linear flattening has
row rank equal to the number of such prefixes, modulo only the stabilizer of
the source.

We independently verified a reachable C=6 grade-2 source with trivial
stabilizer for which

```text
|M(S)| = 74,119
|M(T)| = 74,064
terminal target support = 5,489,549,616
minimum supports over all three-column subsets = 6,488 and 6,503
minimum rank over every fixed 3+3 split = 42,191,464.
```

Thus any fixed-source linear frontier that returns the complete target
polynomial is already too large.  This covers ordinary column DP and signed
Ryser/Glynn reorderings with the same output semantics.  External sorting or
another traversal cannot alter this rank.

There is a related obstruction to materializing a full coherent-configuration
or orbital algebra for reverse gluing.  The verified C=6 two-row inventory has
276 trivial-stabilizer orbits.  Their 38,226 unordered pairs alone expose

```text
38,226 * 46,080 = 1,761,454,080
```

relative-placement coordinates.  Fourier decomposition changes basis but
does not shrink this full equivariant Hom space.

These are deliberately limited statements.  They do **not** lower-bound a
circuit that applies the reduced band kernel to the actual multi-source
vector and combines sources before representing a `(source,target)` pair.
They also do not rule out a small subspace containing only the specific final
weighted responses of the regular reverse-gluing blocks.

## Exact remaining question

Let `x_[s]` be the actual reachable orbit-total coefficient vector at grade
`k`, and let `Kbar_k([t],[s])` be the exact orbit-normalized paired band kernel.
Can one compute

```text
y_[t] = sum_[s] x_[s] Kbar_k([t],[s])
```

directly, without materializing individual kernel rows, nonzero
`(source,target)` pairs, or an arbitrary vector in the full orbital algebra?
A fast full-rank transform is acceptable; we are not requiring low rank.

We would value one of the following exact outcomes:

1. A target-only transform for the actual reachable `x`, with an explicit
   sufficient representation, transition or circuit, orbit normalization,
   proof of closure across successive bands, and operation/memory bound.
2. A construction of a common small Fourier/communication response subspace
   for the regular reverse-gluing blocks that contains everything needed by
   the final weighted contraction, together with its dimension and a way to
   apply the remaining bands.
3. A lower bound that still applies after the actual source weights are
   combined and therefore rules out one or both of these narrower output
   contracts.

## Acceptance and falsification gates

An answer must preserve the square and its exact cycle/orbit normalization; a
one-copy linear marginal is not relevant.  It must distinguish an algorithm
for an arbitrary source row from an algorithm for the actual multi-source
vector.  Claims based only on cache reuse, sorting, traversal order, a
single-copy orbit basis, or the full orbital algebra do not address the
question.

Before C=6, the construction should reproduce complete C=3 and C=4 output
vectors (and every matrix entry if it claims a general operator identity).
It should then give a bounded C=5 actual-vector measurement: representation
dimension, arithmetic operations or emitted records, time, and peak memory.
Finally it should state a concrete C=6 cost projection.  No full C=6 run is
authorized; an exact theorem or a small-C falsifiable construction is the
desired next step.
