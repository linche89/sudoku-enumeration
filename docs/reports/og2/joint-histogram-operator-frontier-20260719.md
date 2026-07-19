# Joint-histogram operator frontier decision

Date: 2026-07-19 (Asia/Singapore)

## Scope

This report closes the bounded decision experiment requested by
`STATUS.md`: lift the scalar residual-degree cost memo to an exact operator
that retains the raw target histogram and the cycle weight, check it against
independent kernels at C=3 and C=4, and measure all seven C=5 layer-1 source
orbits.  It does not close the C=5 layer, run C=6, or access a graph
checkpoint.

The retained implementation is:

```text
experiments/proto/joint_histogram.cpp
SHA-256 = 54858CFE4E704CE211126E5EAE6765ACA892F20B12F7B96F6B55B8668754F059
```

## Exact operator state

The scalar cost memo stored only the source-type index and the remaining
degree two on each new color.  The lifted key stores three exact fields:

```text
(raw labelled target histogram,
 remaining A/B color degrees,
 pairing of live degree-one path endpoints)
```

Each completed cycle is removed and multiplies the coefficient by two.  A
source histogram type is allocated over its legal new-color pairs with the
same exact multinomial coefficient as the existing contingency enumerator.
Identical lifted keys are summed after every source type.

The C=5 command line requires explicit source, state, generated-record, and
source-type limits.  `operatorrawprobe` validates a completed terminal
frontier and reports its exact raw support without starting the much more
expensive color-canonical reduction.

## Differential gates

The prototype was rebuilt on Windows/MinGW with:

```powershell
g++ -O3 -mpopcnt -std=c++20 -Wall -Wextra `
  experiments\proto\joint_histogram.cpp -o build\joint_histogram.exe
```

The operator was selected independently at every possible C=4 target layer:

```powershell
.\build\joint_histogram.exe 3 stop=2 operatorfinal
.\build\joint_histogram.exe 4 stop=1 operatorfinal
.\build\joint_histogram.exe 4 stop=2 operatorfinal
.\build\joint_histogram.exe 4 stop=3 operatorfinal
.\build\joint_histogram.exe 4 operatorfinal
```

For every source and every labelled raw target, its coefficient agreed with
both the contingency-table kernel and the independent labelled-symbol kernel.
The C=4 layer-2 operator generated 559,056 merged records and had a maximum
frontier of 53,970 states, versus 252,355 complete contingency leaves.  The
complete command retained the orbit layers `1,5,141,5,1`, midpoint equality,
and the known total:

```text
N(4) = 29136487207403520
```

Thus the negative C=5 result below is a scale result for an exact kernel, not
a coefficient mismatch.

## Bounded C=5 measurements

Every run used `scripts\watch_rss.ps1` with a 4-GiB RSS limit, a two-minute
limit, one selected source, and internal limits of 10,000,000 states and
100,000,000 generated records.  The last exact completed prefix for each
source was:

| source | exact leaves | last completed types | exact frontier | next result | observed RSS |
|---:|---:|---:|---:|---|---:|
| 0 | 96,418,768 | 5/9 | 2,192,677 | over 10,000,000 at type 6 | 1.631 GiB |
| 1 | 25,051,600 | 4/7 | 3,471,714 | over 10,000,000 at type 5 | 1.748 GiB |
| 2 | 6,516,556 | 5/5 | 6,323,400 | terminal frontier complete | 1.385 GiB |
| 3 | 189,227,536 | 6/10 | 4,316,759 | over 10,000,000 at type 7 | 1.843 GiB |
| 4 | 96,418,768 | 5/9 | 2,284,919 | over 10,000,000 at type 6 | 1.673 GiB |
| 5 | 189,227,536 | 6/10 | 4,309,000 | over 10,000,000 at type 7 | 1.843 GiB |
| 6 | 49,140,784 | 5/8 | 6,580,732 | over 10,000,000 at type 6 | 2.158 GiB |

RSS values are one-second observations, not certified sub-second peaks.  A
scale-limit row establishes only that the next frontier exceeds ten million;
it does not estimate its final size.

The complete raw-frontier command for source 2 was:

```powershell
.\scripts\watch_rss.ps1 `
  -Exe '.\build\joint_histogram.exe' `
  -Arguments @('5','stop=2','sourcestart=2','sourceprobe=1',`
               'operatorfinal','operatorrawprobe',`
               'operatormaxstates=10000000',`
               'operatormaxrecords=100000000','operatorstoptypes=5',`
               'canoncheck=100') `
  -LimitGB 4 -MaxMinutes 2 -IntervalSeconds 1 `
  -LogPath 'data\logs\joint-histogram-c5-operator-s2-raw-complete-20260719.rss.csv' `
  -StdoutPath 'data\logs\joint-histogram-c5-operator-s2-raw-complete-20260719.out' `
  -StderrPath 'data\logs\joint-histogram-c5-operator-s2-raw-complete-20260719.err'
```

It completed in 8.65537 seconds and reported:

```text
scalar residual memo states = 4251
contingency leaves          = 6516556
generated operator records = 11235864
terminal raw frontier      = 6323400
raw-frontier / leaves      = 97.0359%
```

The earlier streaming kernel reduces the same source to only 20,318
color-canonical targets.  The gap is real, but the simple residual operator
does not expose that quotient internally: it reaches 6.32 million labelled
terminal targets before color canonicalization.

## Decision

The target-labelled residual operator passes its complete mathematical gates
and fails its C=5 scale gate.  Six source probes cross ten million states
before termination, and the one completed source is already 97.0359% unique
relative to the original leaves.  This is the near-injective growth specified
in advance as the kill condition.  Increasing the hash-table limit would
enumerate the missing target identity rather than supply the needed quotient.

This result rejects only this labelled operator lift.  It does not reject the
box-order identity, a future rigorously normalized orbit-aware or
target-centric transform, or the separate symbol-synchronous operator-valued
double-permanent proposal.  The latter still has no local implementation or
frontier measurement and becomes the next bounded global decision experiment.
