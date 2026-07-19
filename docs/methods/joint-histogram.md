# Box-order joint-histogram method

`experiments/proto/joint_histogram.cpp` is an exact global decision prototype
for the squared 2xC objective.  It is verified through C=4 and is not part of
the standard build.  C=5 and C=6 have not been attempted by this method.

## State and transition

Advance complete row pairs rather than individual symbols.  After `r` row
pairs, each labelled symbol has used-color masks `(S,T)` in the two copies.
The exact symbol quotient is

```text
h[S,T] = number of symbols whose two used-color masks are (S,T).
```

All masks in layer `r` have size `r`.  In each transition every symbol receives
one new color in each copy, and every color is used exactly twice in each
copy.  For one labelled assignment `(a_s,b_s)`, form the bipartite multigraph
whose edge for symbol `s` joins `a_s` to `b_s`.  It is 2-regular.  The exact
number of shared balanced cuts compatible with the assignment is

```text
2^(number of connected components).
```

The prototype enumerates integer allocations from source types `(S,T)` to
new-color pairs `(a,b)`.  Each allocation receives its exact multinomial
weight, followed by the component factor above.  At C=2..4 an independent
slow path assigns colors to every labelled symbol and agrees with the
contingency path on every raw target coefficient.

## Color quotient and midpoint

States are canonicalized under independent color permutations in the two
copies and optional copy swap.  Stored coefficients are orbit totals, not
representative coefficients.  If

```text
m(h) = (2C)! / product_(S,T) h[S,T]!
```

is the number of labelled symbol assignments represented by a raw histogram,
the exact midpoint contraction for orbit-total vectors is

```text
N(C) = sum_[h] L([h]) R([complement(h)]) /
              (orbitSize(h) * m(h)).
```

The orbit-size factor is state-dependent.  Omitting it gives 480 rather than
288 already at C=2.  The prototype checks every division for exactness and
compares the midpoint result with a complete sequential contraction.

## Verified gates

```text
C=2 orbit layers: 1, 2, 1
C=3 orbit layers: 1, 3, 3, 1
C=4 orbit layers: 1, 5, 141, 5, 1
```

The complete results are the mandatory known values:

```text
N(2) = 288
N(3) = 28200960
N(4) = 29136487207403520
```

Disabling copy swap preserves all totals and changes the C=4 middle layer
from 141 to 232 states.  The complete C=4 contingency run takes under one
second on the reference Windows machine; this is a small-C correctness and
scale result only.

## Boundary

The current key packs at most the 36 joint types needed at the C=4 midpoint,
and the canonicalizer explicitly scans the C=4 color group.  It intentionally
refuses C>4.  The next authorized question is a separately bounded C=5
frontier implementation with a larger key and a cheaper canonicalizer.  No
C=6 run, outer-class loop, or checkpoint access is provided by this prototype.

Exact measurements and reproduction commands are in
`../reports/og2/joint-histogram-c2-c4-20260719.md`.
