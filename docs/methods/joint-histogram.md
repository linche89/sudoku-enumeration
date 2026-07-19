# Box-order joint-histogram method

`experiments/proto/joint_histogram.cpp` is an exact global decision prototype
for the squared 2xC objective.  It is verified completely through C=4 and has
bounded C=5 frontier measurements.  Its target-labelled residual-operator
lift is also exact through C=4 but fails its C=5 scale gate.  It is not part
of the standard build and has not run C=6.

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

## Bounded C=5 result

The enlarged key has 100 active joint types at layers 2 and 3.  The exact
C=5 first layer closes as

```text
orbit layers through layer 1 = 1, 7
contingency leaves           = 6210
layer total                  = 52254720000
```

The canonicalizer uses an occupied anchor and sparse images, while an
independent complete 28,800-image scan checks bounded C=5 samples.  Raw maps
can be reduced in fixed-size batches, so a deterministic five-million-leaf
layer-2 prefix used only about 0.038 GiB observed RSS.  That prefix had
4,227,388 batched raw entries but only 38,373 canonical targets.

The complete layer-2 allocation inventory is nevertheless large.  A scalar
residual-degree DP, checked against the old C=3/C=4 leaf totals, gives exactly

```text
seven source states          = 7
contingency leaves           = 652001548
scalar memo states           = 49890
maximum memo states/source   = 8296
```

This scalar DP is only a cost oracle: it drops both target identity and the
cycle operator.  It proves that residual-degree bookkeeping is highly
compressible, but it does not compute the layer coefficients.  One complete
selected source transition used 6,516,556 leaves and reduced to 20,318
canonical targets in 189.839 seconds.

## Residual-operator decision

The scalar memo was lifted exactly to retain the raw target histogram,
remaining color degrees, and the pairing of live path endpoints.  It agrees
per source and per labelled raw target with both independent transition
kernels at C=3 and C=4.  At the C=4 layer-2 transition its peak frontier is
53,970 states.

The C=5 growth is not controlled.  With a ten-million-state hard limit, six
of the seven layer-1 sources cross the limit before their transition closes.
The remaining source closes with 6,323,400 distinct raw target states from
6,516,556 contingency leaves, or 97.0359% raw uniqueness.  Although those raw
targets later reduce to 20,318 color-canonical targets, this operator reaches
the near-injective labelled frontier first.  It therefore fails the
predeclared scale gate.

## Boundary

The box-order identity and canonical target representation remain exact, but
neither the leaf enumerator nor the target-labelled residual operator is a
plausible C=6 kernel.  An orbit-aware partial operator would require a new
exact normalization: a full color action can move the fixed source and the
processed source-type prefix, so simply canonicalizing partial targets is not
valid.  No such sufficient quotient is currently established.

This result is separate from the symbol-synchronous operator-valued
double-permanent subset DP.  That proposal acts on the connectivity
recurrence's one-symbol transition and remains unimplemented.

The mathematical box-order interfaces are known, but the affordable kernels
for all passes are not.  No C=6 run, outer-class loop, or checkpoint access is
provided by this prototype.

Exact measurements and reproduction commands are in
`../reports/og2/joint-histogram-c2-c4-20260719.md` and
`../reports/og2/joint-histogram-c5-frontier-20260719.md`.  The lifted-operator
gate is recorded in
`../reports/og2/joint-histogram-operator-frontier-20260719.md`.
