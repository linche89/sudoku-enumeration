# Exact penultimate-layer orbit count

## Result

For the native, symbol-quotiented row-incremental state space of the `2xC`
problem, let `M_L(C)` be the number of coordinate orbits after `L` rows.  The
penultimate C=6 layer has exactly

```text
M_5(6) = 96,452,755.
```

This is an exact Burnside count, not a sampling estimate.  The retained
implementation is
`experiments/proto/layer_penultimate_burnside.cpp`.

## Penultimate-state representation

A native state is a multiset of `2C` occurrence masks.  At layer `C-1`, each
mask uses exactly one side in `C-1` boxes and misses exactly one box.  Every
box is missed by exactly two masks, because its two sides each have degree
`C-1`.

Group the two masks missing box `p` into an unordered local pair `A_p`.  Each
member of `A_p` is a binary word on the other `C-1` boxes recording its side
choice.  Therefore a penultimate state is a tuple

```text
(A_0, ..., A_(C-1))
```

of `C` unordered pairs, subject to one balance condition: at every box `q`,
the `2(C-1)` masks not missing `q` contain exactly `C-1` zero-side and `C-1`
one-side choices.

This description counts exactly the reachable native states.  After giving
the multiset copies temporary labels, their incidence with the `2C` slots is
a `(C-1)`-regular bipartite graph.  Every regular bipartite graph decomposes
into perfect matchings, so the balanced tuple can be realized by `C-1` rows.
No extra non-reachable objects enter the count.

## Fixed points of a signed coordinate permutation

The coordinate group is

```text
G_C = C2 wr S_C,   |G_C| = 2^C C!.
```

Fix a signed permutation `g`.  Along a cycle of its box permutation, choosing
the unordered local pair at one box determines all remaining local pairs in
that cycle.  The choice is retained only when propagation around the cycle
returns to the original pair.  There are at most

```text
choose(2^(C-1) + 1, 2)
```

seed pairs for a cycle; this is 528 at C=6.

For every closed cycle choice, the implementation records its contribution
to the `C` one-side degrees.  A small exact degree-vector DP combines the box
cycles and retains total degree `C-1` in every coordinate.  This gives
`Fix(g)`.

Fixed-point counts depend only on the signed cycle type: positive and
negative cycles of each length.  At C=6 the 46,080 group elements form 65
such conjugacy classes.  Burnside's lemma then gives

```text
M_(C-1)(C) = (1 / (2^C C!)) * sum_(g in G_C) Fix(g).
```

The exact C=6 numerator and quotient are

```text
sum_g Fix(g) = 4,444,542,950,400
|G_6|        = 46,080
M_5(6)       = 96,452,755.
```

## Independent gates

The same implementation reproduces the independently materialized small-C
penultimate layers:

| C | layer | exact Burnside count | layer-DP count |
|---:|---:|---:|---:|
| 2 | 1 | 1 | 1 |
| 3 | 2 | 5 | 5 |
| 4 | 3 | 54 | 54 |
| 5 | 4 | 17,120 | 17,120 |

The C=5 value is the complete retained four-row orbit table, so this is a
full-state-space differential rather than a final-total-only check.  The
counter is part of `scripts/verify_layer_dp.ps1` and therefore of the full
repository gate.

The production layer-DP engine also checks the corresponding exact count
whenever it constructs or loads a penultimate layer.  A C=6 layer-5 snapshot
with any real-state count other than 96,452,755 fails before the final
transition.

## Scope

This result determines the number of canonical layer-5 keys and validates
the real-state portion of its capacity.  It does not compute the layer-5
weights `T_5`, replace the 4-to-5 transition, or determine `N(6)`.  Parallel
insertion holes and production allocation/checkpoint behavior remain
operational matters covered by the staged rehearsal.
