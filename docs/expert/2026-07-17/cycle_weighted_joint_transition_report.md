# Exact cycle-weighted joint transition without the full joint histogram

## Status

The cycle factor can be applied exactly without storing the row-pair joint histogram `h_{S,T}`.  The strongest tested implementation is a symbol-synchronous dynamic program whose per-band state is a degree vector plus a pairing of the degree-one endpoints of the current paths.  Closed cycles are deleted immediately and contribute a factor 2.

The implementation reproduces the squared totals for C=2,3,4.  It has not yet completed C=5.  At C=5 its exact symmetry-reduced frontier has 83,776 states after three of the ten labeled symbols, so a second mechanism—an implicit application of the double-permutation sum—is required.

## Cycle identity

For one band, let `c_s,d_s in [C]` be the two stack-column assignments of symbol `s`, with every color occurring twice in each coordinate.  Let `H(c,d)` be the 2-regular bipartite multigraph with edge `s` from `c_s` to `d_s`.  The common skeleton choices are the perfect matchings of `H`; every cycle has two alternating perfect matchings.  Therefore the band weight is

\[
2^{\kappa(H(c,d))}.
\]

Equivalently,

\[
2^{\kappa(H)}
=\#\{(P,Q):[c_s\in P]=[d_s\in Q]\ \forall s\}.
\]

The compatible pairs `(P,Q)` are exactly the unions of connected cycle components.

## Symbol-synchronous formulation

Each labeled symbol `s=1,...,2C` chooses two permutations

\[
\sigma_s,\tau_s\in S_C.
\]

In band `r`, insert the edge `(sigma_s(r),tau_s(r))`.  Degree capacities are two.  At the end every band is 2-regular and the exact contribution is the product of the accumulated cycle factors.

## Exact local quotient

A partial max-degree-two bipartite graph is represented by:

1. the degree 0,1,2 of every left and right color vertex;
2. a mate involution on the degree-one endpoints: two endpoints are mates exactly when they are the endpoints of the same live path.

Closed cycles are absent from the state and multiply the coefficient by two when they are closed.

For a new edge `u-v`:

- 0--0: create a path and pair `u,v`;
- 1--0 or 0--1: extend a path, replacing the old endpoint by the new endpoint;
- 1--1 on different paths: merge the paths and pair the two other endpoints;
- 1--1 on the same path: close a cycle, remove both endpoints, and multiply by 2;
- any endpoint of degree 2: illegal.

Future transitions depend only on this data.  Thus it is an exact strong lumping, not an approximation.

The global state is the tuple of the C local states, canonicalized under band permutations, independent color permutations in the two stacks, and stack swap.  Orbit totals are stored, so no symmetry factor is divided out.

## Computed frontiers

| C | Exact quotient-state counts by processed symbol | Result |
|---|---|---|
| 2 | 1, 1, 3, 1, 1 | 288 |
| 3 | 1, 1, 8, 18, 19, 1, 1 | 28,200,960 |
| 4 | 1, 1, 28, 700, 12,856, 9,708, 155, 1, 1 | 29,136,487,207,403,520 |
| 5 | 1, 1, 93, 83,776 (through symbol 3 only) | not completed |

The checked C=4 run used 15.23 seconds wall time and 76,508 kB peak RSS.  The checked C=5 probe through symbol 3 used 6.56 seconds and 63,204 kB peak RSS.

## C=5 initialization

The first symbol can be fixed to `(id,id)` and restored by the exact factor `(C!)^2`.  The second relative pair `(p,q)` is classified under simultaneous conjugation and stack swap.  For C=5 there are 96 such relative pair orbits, which merge to 93 exact connectivity states after applying the global canonicalization.

## Current bottleneck

From the 83,776-state C=5 layer, a direct next transition tests up to

\[
83,776(5!)^2=1,206,374,400
\]

permutation pairs before legality and merging.  Therefore the connectivity quotient alone does not pass the C=5 validation gate.

The next exact experiment is to apply the one-symbol operator

\[
\mathcal T=\sum_{\sigma,\tau\in S_C}\bigotimes_{r=1}^C L_{r,\sigma(r),\tau(r)}
\]

as an operator-valued double permanent using a subset-mask frontier over used left and right colors.  This must be measured on the 83,776-state layer.  If the intermediate frontier remains much smaller than the 14,400 completed permutation pairs per source, C=5 is plausible; if it is near-injective, this route should be abandoned in favor of the exact Kraus/representation channel decomposition.
