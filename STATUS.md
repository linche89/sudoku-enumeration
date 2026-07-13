# Current Project Status

Last updated: 2026-07-14

This is the single authoritative status page. Dated reports preserve evidence;
historical handoffs and raw expert responses are not current project state.

## Stable results

The FJ05 9x9 reproduction is complete and independently cross-checked against
all 71 reference classes:

```text
N0 = 6670903752021072936960
```

The exact 2xC counts are verified by multiple engines:

| C | N(C) |
|---:|---:|
| 2 | 288 |
| 3 | 28200960 |
| 4 | 29136487207403520 |
| 5 | 1903816047972624930994913280000 |

The primary exact engine is `src/factorization_orbit.cpp`. Five fresh C=5
full gates averaged about 8.66 seconds with `pivot rooted4` on the reference
machine.  A same-binary controlled ablation measured factor-stage times of
53.35 seconds for the plain recurrence, 36.93 seconds with only the rooted
edge pivot, 11.48 seconds with only the degree-four split, and 8.58 seconds
with both.  See `docs/reports/og2/c5-paper-ablation-20260712.md`.

The optional future-twin backend independently agrees with the primary engine
on every outer class for complete C=2..5. It is selected explicitly with
`future`; the default engine and checkpoint format are unchanged.

## Historical C=6 target

The C=6 decimal value is not an unpublished target.  Kjell Fredrik Pettersen
announced the following count on the New Sudoku Players' Forum on 2006-11-14:

```text
N(6) = 38296278920738107863746324732012492486187417600000
```

The same thread reports 63,199 outer band-configuration classes and describes
the weighted square sum used to obtain the result.  No buildable historical
source or independent executable verification has been located in the current
literature audit.  This project's C=6 objective is therefore an independent,
open verification of that historical claim, not discovery of a new integer.
See `docs/reports/og2/literature-audit-20260712.md`.

## C=6 frontier

There are exactly 63,199 outer skeleton orbits. The first one is closed:

```text
F6(G1) = 6986348258918400
class time = 706.652 s
peak working set = 665.6 MiB
```

The future-twin backend now gives an independent cold reproduction with no
checkpoint input:

```text
F6(G1) = 6986348258918400
class time = 1592.867 s
peak working set = 1.670 GiB
peak frontier = 10025564 states
canonical fallbacks = 0
```

The second, lower-symmetry graph exposes the remaining wall:

```text
top mode       raw F5 residuals   strong F5 classes
pivotinner               192960               23732
pivot                     31920               19850
```

Its first 50 new F5 values are closed. A straight hard run remains a multi-hour
operation, so no full C=6 count is currently in progress.

A cold five-minute future-twin probe of this second graph completed frontiers
of 1, 420, 26,028, 643,605, and 7,630,873 states with `canonical-last`. At the
bound it had processed 2,000,000 of the 7,630,873 parents in the next layer and
had accumulated 7,981,359 next states at 2.186 GiB. Under the same bound,
`canonical-first` completed only three layers (1, 352, 81,663). Thus the last
order is clearly better, but the G2 central frontier remains unresolved.

## Audited route decision

The single-graph aggregation question now has a verified optional backend. A
future-twin recurrence processes the 12 left vertices directly and identifies
right columns by their remaining neighborhood and used-color set
`(tau, K)`.  It never enumerates a top perfect matching, constructs a residual
`Q-M`, or calls `F5` separately. Canonical keys, labelled transition
multiplicities, complete per-class C=2..5 agreement, a forced safe-fallback
gate, and the cold G1 value have all been verified. Joint canonicalization is
essential for C=6, but the bounded G2 result shows that it does not by itself
remove the low-symmetry central frontier.

The proposed natural 3+3 low-rank route did not pass its required small-C
decision gate.  The exact symmetry-block endpoint exists, but reproduced
certificates give full rank 630/630 at C=4 and full row rank 8001/8001 at C=5.
At C=6 every domain multiplicity fits inside the midpoint module, so symmetry
forces no channel loss.  This rules out ordinary irreducible-channel
truncation as a justified C=6 implementation route; it does not rule out a
different fast implicit contraction for a full-rank operator.

The external derivations, supplied code, local reproduction commands, hashes,
and limitations are recorded in
`docs/reports/og2/c6-expert-routes-audit-20260713.md`.
The implemented recurrence and its C=6 evidence are recorded in
`docs/methods/future-twin.md` and
`docs/reports/og2/future-twin-c6-20260714.md`.

## Immediate objective

The future-twin implementation gates are complete. The next technical target
is the G2 central frontier: reduce its memory footprint or find a stronger
exact quotient before attempting a longer G2 closure. The present bounded
evidence does not justify a spread-out full-run projection.

No full 63,199-class C=6 run is authorized.

## Current checkpoint

The active checkpoint is documented in
`data/checkpoints/MANIFEST.md`. It contains 5,315,962 exact memo states and is
not stored in Git. Use `checkpointreadonly` for verification so a read-only
gate cannot rewrite it.

## Active implementation tracks

- `factorization_orbit`: primary C=2..6 exact factorization/orbit route.
- `multiset_q`: independent transfer-kernel research and cross-check route.
- `multiset_fast` / `multiset_c6`: older independent exact validators.
- `main.cpp`: completed FJ05 9x9 reproduction.

See `docs/index.md` for the evidence and provenance map.
