# Future-twin backend and bounded C=6 evaluation — 2026-07-14

## Scope

This report records the optional future-twin backend decision gates, one cold
G1 reproduction, and a bounded two-order G2 comparison.  No graph checkpoint
was passed and no full 63,199-class C=6 count was started.

The default factorization engine and checkpoint format were unchanged.

## Correctness gates

The latest binary passed:

- canonical invariance/separation for `C=2..6`;
- grouped-versus-labelled transition differentials for random `C=2..4`
  graphs;
- complete per-outer-class agreement with the current engine for `C=2..5`;
- complete joint `canonical-last` per-class agreement for `C=2..5`;
- forced weak-key fallback agreement for all C=2..4 outer classes with
  `futurecanonbudget=1`.

The complete C=5 joint check returned

```text
N(5) = 1903816047972624930994913280000
checks = 355
peakStates = 15235
expanded = 742315
canonical fallbacks = 0
maximum search nodes in one canonical call = 1135
```

The dual-engine run took 22.588 seconds; that includes the current reference
engine as well as the future-twin calculation.

With `futurecanonbudget=1`, the C=4 test deliberately used 75,310 safe weak
keys and still matched every class and the exact total.  This is the direct
test that canonical-search exhaustion loses compression only, not accuracy.

## Cold G1 reproduction

Command, wrapped by `scripts/watch_rss.ps1` with a 30-minute and 3-GiB guard:

```powershell
.\build\factorization_orbit.exe 6 limit=1 future `
    futureorder=canonical-last futureprogress
```

Result:

```text
F6(G1) = 6986348258918400
class time = 1592.867 s
peak RSS = 1796931584 bytes = 1.670 GiB
peak frontier = 10025564 states
expanded states = 21494448
grouped leaves = 351628403
canonical calls = 351628404
canonical cache hits = 2273820
canonical search nodes = 446732419
canonical fallbacks = 0
maximum nodes in one canonical call = 4837
```

The complete frontier profile was:

| processed rows | states |
|---:|---:|
| 0 | 1 |
| 1 | 1 |
| 2 | 124 |
| 3 | 19,573 |
| 4 | 530,234 |
| 5 | 10,025,564 |
| 6 | 2,550,673 |
| 7 | 8,186,542 |
| 8 | 177,683 |
| 9 | 4,038 |
| 10 | 14 |
| 11 | 1 |
| 12 | 1 |

The long apparent stall at layer 6 was not a pathological canonical call.
Progress instrumentation showed a linear scan of 10,025,564 parent states;
the maximum canonical search at that point was only 390 nodes.

Primary logs:

```text
data/logs/future-g1-canonical-last-30m-20260714.out
data/logs/future-g1-canonical-last-30m-20260714.err
data/logs/future-g1-canonical-last-30m-20260714.rss.csv
```

## Bounded G2 two-order comparison

Both runs used the same cold, read-free/write-free bounds:

```text
outer class: start=1 limit=1 (the second class, zero-based start)
time: 5 minutes
RSS: 3 GiB
checkpoint: none
```

| order | completed frontier layers | partial work at stop | peak RSS |
|---|---|---|---:|
| canonical-last | 1, 420, 26,028, 643,605, 7,630,873 | layer 6: 2,000,000 / 7,630,873 parents; 7,981,359 next states | 2.186 GiB |
| canonical-first | 1, 352, 81,663 | layer 4 not closed | 0.953 GiB |

Both runs had zero canonical fallback.  `canonical-last` is decisively the
better of these two orders, but its G2 central frontier was already much larger
than G1 at the same partial layer.  This bounded result does not establish that
G2 can be closed within the current memory limit.

Logs:

```text
data/logs/future-g2-canonical-last-5m-20260714.*
data/logs/future-g2-canonical-first-5m-20260714.*
```

## Rejected experiments

Several exact but inferior experimental choices were evaluated and then
removed from the source:

- fixed-row orders reached multi-million-state frontiers before the joint
  quotient;
- recursive row-adaptive memoization generated about five million subproblems
  in a three-minute G1 probe without closing;
- row/color-dual adaptive variants moved the explosion to `5x5` or `6x4`
  shapes and were slower on complete C=5 gates;
- canonical-first completed only 118 of 355 C=5 classes in a three-minute
  bound.

Their raw logs remain under `data/logs/` for short-term audit, but these modes
are not part of the retained implementation.

## Final repository gate and checkpoint audit

The completed working tree passed:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts\verify_all.ps1
```

The gate rebuilt all active binaries and completed successfully in 147.3
seconds. It included the full FJ9 reproduction, all 71 reference classes,
complete C=2..5 exact totals, both future-twin C=5 checks, the forced fallback
gate, and the read-only C=6 checkpoint verification.

The active checkpoint and its external backup were unchanged and identical:

```text
size = 217954462 bytes
sha256 = FE8B68DE6C15592848D7CF69BF43928C000F0B2EE59FC263A094BCA2D376A865
```

## Decision

The future-twin recurrence is now a verified optional exact backend.  It gives
an independent cold G1 reproduction but is not yet a practical G2 solver.
No projection for the full 63,199-class sum is justified from the current G2
evidence.
