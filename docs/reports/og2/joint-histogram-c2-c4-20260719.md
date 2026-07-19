# Box-order joint-histogram C=2..4 decision gate

Date: 2026-07-19 (Asia/Singapore)

## Scope

This report tests whether the 2026-07-15 paired joint histogram

```text
h[S,T] = #{symbols s with used-color masks (S,T)}
```

can be propagated globally in box order for the actual squared objective.
The experiment is deliberately limited to C=2..4.  It does not evaluate C=5
or C=6 and does not read or write a graph checkpoint.

The retained implementation is:

```text
experiments/proto/joint_histogram.cpp
SHA-256 = 89330B944E59E0DC7472981F8215252006E5D0FF88BBDCA989718A8340252501
```

## Exact conventions

One transition assigns a fresh color pair `(a,b)` to every labelled symbol,
with every color occurring twice in each copy.  For the resulting 2-regular
bipartite color multigraph, every connected component supplies two compatible
shared cuts.  The transition weight is therefore the labelled allocation
multinomial times `2^components`.

The main kernel enumerates allocations by source joint type.  A deliberately
different slow kernel enumerates every labelled symbol assignment.  Their raw
target maps are compared before color canonicalization.

Color states are quotiented by `S_C x S_C` and copy swap.  Because the DP
stores orbit totals, the midpoint divisor is

```text
orbitSize(h) * (2C)! / product h[S,T]!.
```

At C=2 the three raw midpoint matrices reduce to two color/copy orbits.  Their
exact contributions are 192 and 96.  This is a direct regression for the
state-dependent orbit normalization.

## Exact results

All commands used the Windows-safe build flags:

```powershell
g++ -O3 -mpopcnt -std=c++20 -Wall -Wextra `
  experiments\proto\joint_histogram.cpp -o build\joint_histogram.exe
```

### C=2

```text
orbit states                 = 1, 2, 1
contingency leaves           = 3, 2
labelled direct leaves       = 36, 2
layer totals                 = 1, 96, 288
midpoint terms               = 192, 96
sequential total             = 288
midpoint total               = 288
known total                  = 288
```

### C=3

```text
orbit states                 = 1, 3, 3, 1
contingency leaves           = 21, 168, 3
labelled direct leaves       = 8100, 300, 3
global distinct raw targets  = 21, 21, 1
orbit transitions            = 3, 9, 3
layer totals                 = 1, 25920, 8294400, 28200960
sequential total             = 28200960
1+2 midpoint total           = 28200960
known total                  = 28200960
```

### C=4

The full differential run reported:

```text
orbit states                 = 1, 5, 141, 5, 1
contingency leaves           = 282, 252355, 28685, 5
labelled direct leaves       = 6350400, 441045, 48348, 5
raw target occurrences       = 282, 123433, 14771, 5
global distinct raw targets  = 282, 54277, 282, 1
orbit transitions            = 5, 677, 677, 5
layer totals                 =
  1,
  23224320,
  7501014097920,
  7867938845491200,
  29136487207403520
sequential total             = 29136487207403520
2+2 midpoint total           = 29136487207403520
known total                  = 29136487207403520
```

Every raw target coefficient from all 152 C=4 source-orbit transitions was
identical between the contingency and labelled-symbol kernels.  The measured
per-layer times for that differential run were 0.356338, 0.805722, 0.020928,
and 0.000019 seconds.

With the slow differential kernel disabled, the corresponding times were
0.001975, 0.752501, 0.014318, and 0.000014 seconds.  The one-second RSS sampler
observed 0.009 GiB; because the job is shorter than the sampling interval,
this is an observed sample rather than a certified process peak.

Disabling copy swap changed the orbit layers to

```text
1, 5, 232, 5, 1
```

while preserving every layer total, the midpoint result, and `N(4)`.  This is
an independent quotient ablation.

The bounded controls were also exercised directly.  `stop=1` and `stop=2`
returned closed layer summaries without entering later layers;
`maxstates=100` stopped before retaining the 141-state C=4 midpoint, and
`maxleaves=100` stopped during the first transition.  Both limit failures were
reported explicitly as `[SCALE-LIMIT]` rather than as partial exact results.

## Guarded commands

The C=4 differential and core runs were guarded at 4 GiB and five minutes:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -Command `
  "& { & '.\scripts\watch_rss.ps1' `
  -Exe '.\build\joint_histogram.exe' `
  -Arguments @('4','maxstates=2000000','maxleaves=100000000') `
  -LimitGB 4 -MaxMinutes 5 -IntervalSeconds 1 `
  -LogPath 'data\logs\joint-histogram-c4-direct-20260719.rss.csv' `
  -StdoutPath 'data\logs\joint-histogram-c4-direct-20260719.out' `
  -StderrPath 'data\logs\joint-histogram-c4-direct-20260719.err' }"

powershell -NoProfile -ExecutionPolicy Bypass -Command `
  "& { & '.\scripts\watch_rss.ps1' `
  -Exe '.\build\joint_histogram.exe' `
  -Arguments @('4','nodirect','maxstates=2000000','maxleaves=100000000') `
  -LimitGB 4 -MaxMinutes 5 -IntervalSeconds 1 `
  -LogPath 'data\logs\joint-histogram-c4-core-20260719.rss.csv' `
  -StdoutPath 'data\logs\joint-histogram-c4-core-20260719.out' `
  -StderrPath 'data\logs\joint-histogram-c4-core-20260719.err' }"
```

## Decision

The box-order joint histogram is now an exact global contraction through C=4,
not merely a proposed identity.  Its C=4 middle frontier of 141 states is much
smaller than the previously reproduced 12,856-state connectivity frontier and
the 30,360-state full count tensor at their tested C=4 central layers.

This comparison does not prove favorable C=5 or C=6 growth.  The current
prototype is intentionally incapable of either.  The result justifies one
new bounded decision experiment ahead of the operator-valued double-permanent:
implement the larger C=5 joint key, reproduce the C=2..4 gates unchanged, and
measure C=5 layer growth under explicit time, memory, state, and transition
limits.  A full C=5 total is permitted only if those bounded layers remain
controlled.  No C=6 extrapolation is justified by this report.

After the source and documentation changes, the complete repository gate was
run:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts\verify_all.ps1
```

It exited successfully in 138.4 seconds.  The gate covered the complete
C=2..5 factorization checks, canonicalization and histogram differential
tests, all 71 FJ9 reference classes, and the existing C=6 first-outer-class
checkpoint verification in read-only mode.
