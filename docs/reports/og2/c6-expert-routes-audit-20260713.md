# Audit of the P1 and P2 C=6 expert routes

Date: 2026-07-13 (Asia/Singapore)

## Scope

This report separates raw external proposals from conclusions reproduced or
audited in this repository.  It covers:

- the future-twin recurrence in
  `docs/expert/2026-07-12/p1_response.md`;
- the 3+3 symmetry-contraction response and code bundle under
  `docs/expert/2026-07-13/`.

No complete C=6 outer sum was attempted or claimed.

## P1: future-twin recurrence

For a fixed C-regular bipartite graph, process left vertices in a chosen order.
After processing `P`, a right column `j` is represented by

```text
(tau_P(j), K_j),
tau_P(j) = N(j) - P,
K_j      = the colors already used at j.
```

The state is the multiset of these records, modulo a global color permutation.
Two columns with the same record have identical adjacency and color
availability for every future step, so exchanging them gives a bijection of
future completions.  For an affected group of `m_g` labelled columns, choosing
its set of `m_g` colors and then assigning those colors to the labels contributes
the exact factor `m_g!`.  Disjoint group color sets partition all C colors.

This gives an exact direct edge-coloring DP.  It does not enumerate a perfect
matching, construct `Q-M`, or evaluate the roughly 20,000 degree-5 residual
classes separately.

The sufficiency and multiplicity arguments were reviewed independently.  A
temporary no-file Python implementation was compared with a separate raw
perfect-matching recurrence on random balanced graphs:

| C | individual `F_C` values | result |
|---:|---|---|
| 2 | 2, 2 | all equal |
| 3 | 72, 48, 48 | all equal |
| 4 | 39,936; 32,256 | all equal |

The two C=4 future-twin peak state counts were 52 and 142.  This is a spot
check, not the mandatory repository gate.  In particular, the response's
30,421-state C=6 profile used a random graph rather than the actual second
outer class, and its transition bound does not include the cost of naively
trying all 720 color permutations for every target key.

Decision: the recurrence is sufficiently explicit and credible to implement,
but it is not yet a verified active method or a basis for a C=6 runtime claim.

## P2: supplied bundle

The original ZIP was preserved with SHA-256

```text
EDAA5FD3EDF00916C407DF64C87E3719B4EFC21DBEF1FCD472C799008DFDAAEA
```

It was extracted into a fresh system temporary directory and built without
modifying the repository.  Environment:

```text
MinGW-w64 GCC 13.2.0
Python 3.13.5
```

Commands:

```powershell
g++ -O3 -std=c++17 validate_c4.cpp -o validate_c4.exe
g++ -O3 -std=c++17 validate_c5.cpp -o validate_c5.exe
g++ -O3 -std=c++17 rank_channels_c5.cpp -o rank_channels_c5.exe
g++ -O3 -std=c++17 count_boundary_occupancy_orbits.cpp -o count_occ.exe

.\validate_c4.exe
.\validate_c5.exe
.\rank_channels_c5.exe
.\count_occ.exe
python symmetry_blocks.py --boundary --csv audit_c6_blocks.csv
```

Observed results and wall times:

| computation | wall time | exact result |
|---|---:|---|
| C=4 boundary contraction | 1.1 s | `N4=29136487207403520`, rank 630/630 |
| balanced occupancy orbits | 1.1 s | 49,755 occupancies, 132 color orbits |
| C=6 character decomposition | 3.8 s | 67 types, 544,034 symmetric entries |
| C=5 2+3 validation | 81.4 s | 355 classes and exact known `N(5)` |
| C=5 channel certificate | 91.2 s | centralizer rank 154/154, row rank 8001/8001 |

The regenerated C=6 CSV matched the supplied CSV exactly, with SHA-256

```text
59AF4F1767587F9575F53A1E3A6C3F3AFC13070F3A36CAB0F4F30E0E518F45B7
```

## Audited P2 conclusion

For `D = Sym^3(W)`, with `W` the 462-dimensional module on unordered balanced
6|6 cuts, the exact block endpoint is

```text
N(6) = 64 * sum_lambda d_lambda * tr(M_lambda M_lambda^T).
```

The final block norm is small once the `M_lambda` are known.  Constructing the
blocks is not solved.  Exact smaller cases show that the natural coloring maps
do not have the hoped-for low channel rank:

- C=4 has rank 630/630 over Q, certified by full rank modulo 1,000,003;
- C=5 has full row rank 8001/8001, certified through the 154-dimensional
  centralizer algebra;
- at C=6 the 5,171,942,314-dimensional color-invariant midpoint contains at
  least the domain multiplicity for every one of the 67 occurring types.

The local cut-to-matching transform is also full rank on all four irreducible
summands.  Therefore ordinary irreducible-channel truncation has neither
small-C evidence nor a representation-dimension mechanism at C=6.

This is not a proof that the C=6 coloring map itself is full rank, nor that all
3+3 algorithms are impossible.  A fast implicit transform for a full-rank
operator could still exist.  The supplied work does not construct the C=6
blocks or compute `N(6)`.

## Route decision

The immediate project objective is to implement and falsify the P1
future-twin evaluator against complete exact gates.  The natural P2 low-rank
implementation is paused.  It should be reopened only for a proposal that
constructs or applies the full-rank block operator without materializing the
63,199 outer coordinates or the much larger midpoint algebra.
