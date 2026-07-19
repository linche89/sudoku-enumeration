# Reverse Row-Block Gluing

## Status

Reverse row-block gluing is exact and has been independently verified through
complete C=5.  The retained prototype is
`../../experiments/proto/reverse_glue.cpp`; it is not part of the standard
build.  It now contains a bounded C=6 frontier probe, but no complete C=6
four-row layer or outer contraction.

The verified ladder is:

```text
C=2: 1+1 -> 2
C=3: 1+2 -> 3
C=4: 2+2 -> 4
C=5: 2+2 -> 4, then 4+1 -> 5
```

Every complete C=2..4 class agrees with an independent labelled-coordinate
factorization oracle.  At C=5 all 355 triples
`(coordinate orbit size, labelled multiplicity, F(Q))` agree with
`factorization_orbit`, not only the final weighted square sum.

## Partial configurations

There are `2C` paired left vertices and `2C` symbols.  In an `L`-row partial
configuration, a symbol occurrence mask:

- has `L` bits;
- uses at most one vertex from each pair; and
- records the boxes and sides occupied by that symbol.

A configuration is the sorted multiset of the `2C` occurrence masks, so symbol
labels have already been quotiented.  Every left vertex has degree `L`.
Write `F_L(x)` for the number of ordered row factorizations of a concrete graph
represented by `x`.  In particular, `F_1=1`, while for a two-regular
bipartite graph

```text
F_2(x) = 2^(number of cycle components).
```

The signed coordinate group is

```text
G_C = C_2 wr S_C,
|G_C| = 2^C C!.
```

Thus its orders at C=4,5,6 are 384, 3,840, and 46,080.  The expression
`2^6 6!` must not be abbreviated as `26!`.

## Contingency gluing coefficient

Let the distinct masks in `x` have multiplicities `a_u`, and those in `y`
have multiplicities `b_v`.  A feasible contingency table `n_(u,v)` has these
row and column sums and uses only pairs of masks with disjoint box support.
The union masks have counts

```text
c_w = sum_(u union v = w) n_(u,v).
```

For one fixed complete graph with histogram `c`, the number of decompositions
realizing this table is

```text
                  product_w c_w!
K_c(n) = -----------------------------------.
           product_(u,v) n_(u,v)!
```

This is the coefficient used by the implementation.  The alternative factor

```text
product_u a_u! product_v b_v! / product_(u,v) n_(u,v)!
```

counts bijections between chosen symbol representatives.  It is not yet the
per-fixed-output coefficient and requires the corresponding input/output
orbit normalization.  Mixing those conventions is a normalization error.

## Coordinate-orbit normalization

For orbit representatives `x`, `y`, and `q`, let `s_x`, `s_y`, and `s_q` be
their stabilizer orders in `G_C`.  If `J(x,g y -> q)` includes the contingency
coefficient and the two partial factorization counts, then the contribution to
the factorization count of one concrete representative `q` is

```text
                 s_q
sum_(x,y) --------------- sum_(g in G_C) J(x, g y -> q).
              s_x s_y
```

Only stabilizer orders are required.  Transporters or stored stabilizer
generators are not required for correctness: the prototype obtains the orders
by direct group enumeration and checks every division exactly.

Two exact reductions are used after the basic formula is established:

1. enumerate each distinct coordinate image of `y` once; its `s_y` repeated
   group elements cancel the denominator `s_y`;
2. quotient those images by `Stab(x)`, glue one representative of each double
   coset, and multiply by its orbit size.

For a 2+2 join, `(x,y)` and `(y,x)` contribute equally.  The implementation
therefore evaluates unordered orbit pairs and doubles off-diagonal terms.  All
three reductions pass the complete C=2..4 differential gates.

After this normalization the result is `F(q)` for one concrete complete
representative.  The outer multiplicity is applied separately:

```text
w(q) = |coordinate orbit of q| * (2C)! / product_w c_w!,
N(C) = sum_[q] w(q) F(q)^2.
```

The engine can also print `w(q)F(q)`, but that is a derived diagnostic, not its
native stored convention.

## Exact two-row orbit generator

For two rows, the sorted occurrence masks are the edges of a loopless
2-regular multigraph `H` on the `2C` coordinate vertices; a length-two cycle
is a double edge.  The box partition is a perfect matching `P` disjoint from
`H`.  Coordinate-group orbits are therefore the isomorphism classes of pairs
`(H,P)`.

The C=6-capable generator fixes one `H` for every cycle partition of `2C`,
enumerates admissible `P`, and quotients them by `Aut(H)`.  It then performs a
full `C2 wr S_C` canonical/stabilizer scan on every resulting representative.
At C=3..5 its complete sorted orbit inventory agrees with the independent
labelled-coordinate DFS.  At C=6 a count-only run of that DFS independently
confirms the summed coordinate-orbit mass.

## Verified state data

| C | split | labelled two-row configurations | two-row orbits | four-row orbits | complete classes |
|---:|---|---:|---:|---:|---:|
| 2 | `1+1` | n/a | 1 one-row orbit | n/a | 2 |
| 3 | `1+2` | 40 | 5 | n/a | 4 |
| 4 | `2+2` | 2,019 | 23 | 26 complete classes | 26 |
| 5 | `2+2`, `4+1` | 165,744 | 107 | 17,120 | 355 |
| 6 | bounded `2+2` probe only | 20,338,525 | 772 | not generated | not entered |

The complete C=5 four-row coordinate-orbit mass is 62,185,328.  The 107
two-row orbits give 5,778 unordered 2+2 orbit pairs.  Thirteen closed pair
intervals covered exactly `[0,5778)`, merged to the 17,120 four-row values,
and then produced:

```text
complete classes          = 355
labelled multiplicity sum = 1016255020032
N(5) = 1903816047972624930994913280000
```

## Engineering conclusions

The C=5 experiment confirms the mathematical route and rejects two optimistic
cost shortcuts:

- `choose(C,2) 2^2` is the number of possible masks for one symbol, not the
  number of two-row configurations or their orbits;
- a contingency recursion can be small per call, but low-stabilizer orbit
  pairs generate millions of calls and cold canonicalizations.  The measured
  maximum was 1,296 contingency leaves in one call.

The C=6 probe sharpens both points.  The 772 two-row orbits produce 298,378
unordered orbit pairs.  Exact single-placement samples emitted 2,662, 15,360,
and 27,793 contingency leaves.  In the middle sample, 15,360 leaves reduced
to 15,168 raw keys, while the first 2,000 raw keys also gave 2,000 distinct
canonical four-row representatives.  Generic cold canonicalization ran at
only hundreds of outputs per second because every miss scans all 46,080 group
images.

There are 276 trivial-stabilizer two-row orbits.  Their 38,226 unordered pairs
alone force `38,226 * 46,080 = 1,761,454,080` double-coset placements.  A
probe-only first-mask anchor can compute a canonical key in at most 2,304
candidate images without immediately computing its stabilizer.  It passed
100 full-group differential checks and reached about 8,195 keys/s, but a
100,000-leaf generic prefix still produced 99,423 distinct canonical keys.

C=5 four-row canonicalization uses an exact anchor.  Every four-row symbol
misses one of five boxes.  In a lexicographically minimal representative, the
first mask must miss the last box and use side zero in the other four boxes.
This leaves at most `10 * 2 * 4! = 480` candidates instead of all 3,840 group
elements.  A full 3,840-element scan of each newly discovered representative
independently verifies both minimality and its stabilizer.

Pair-interval files contain only closed exact partial sums.  Their reader
checks the format, identity, sorted keys, padding, masks, nonzero values, and
exact interval coverage before a merge can feed the next layer.

## C=6 boundary

The first bounded frontier decision is complete.  A cycle/matching quotient,
independently checked against the old generator at C=3..5, constructed the
exact 772-orbit C=6 two-row layer in about 1.53 seconds.  A separate raw DFS
count confirmed its coordinate mass of 20,338,525.  Sampled orbit pairs had
10, 1,448, and 43 double-coset placements; sampled individual placements had
thousands to tens of thousands of contingency leaves.  The exact stabilizer
histogram also gives the 1.761-billion-placement lower bound above.

These measurements reject brute group enumeration per orbit pair followed by
immediate output canonicalization as the C=6 implementation.  They do not
reject reverse gluing itself.  Reopening this route requires a bulk four-row
generator or lookup, delayed/external reduction, and a cheaper canonical key.
No complete C=6 four-row layer has been generated.

Pettersen's historical report of more than 900 million four-row lookup states
is a useful rough comparison point, but its exact convention and dataset are
not available locally.  It is not an acceptance oracle.  No full four-row
layer, 63,199-class contraction, or C=6 total is authorized by this method
note.  Detailed probe data are in
`../reports/og2/reverse-gluing-c6-frontier-probe-20260718.md`.
