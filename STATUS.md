# Current Project Status

Last updated: 2026-07-12

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

The primary exact engine is `src/factorization_orbit.cpp`. Its current C=5
full gate takes about 9.2 seconds with `pivot rooted4`.

## C=6 frontier

There are exactly 63,199 outer skeleton orbits. The first one is closed:

```text
F6(G1) = 6986348258918400
class time = 706.652 s
peak working set = 665.6 MiB
```

The second, lower-symmetry graph exposes the remaining wall:

```text
top mode       raw F5 residuals   strong F5 classes
pivotinner               192960               23732
pivot                     31920               19850
```

Its first 50 new F5 values are closed. A straight hard run remains a multi-hour
operation, so no full C=6 count is currently in progress.

## Current mathematical question

The rooted color-block identities have removed the cubic canonicalization
layer. The unsolved problem is now global aggregation: either compress the
roughly 20,000 strong F5 residuals of a low-symmetry outer graph, or evaluate
the exact one-shot coefficient

```text
[Omega_x Omega_y] Phi(x,y)^12
```

without expanding its enormous midpoint state space. The proposed
symmetry-adapted 3+3 low-rank contraction is a research program, not yet an
algorithm; explicit bases, contraction maps, and measured C=4/5 channel ranks
are still missing.

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
