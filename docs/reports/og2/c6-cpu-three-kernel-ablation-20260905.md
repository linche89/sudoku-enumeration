# Exact CPU F4 arithmetic: three-kernel ablation, 2026-09-05

The isolated incremental-DSU kernel passed complete C4/C5 four-row value
checks and the retained 1,024-state C6 sample. On that C6 sample its median
24-thread arithmetic time was 0.1219200 seconds versus 0.1757517 seconds for
the unchanged released kernel, a 1.442x scoped hot-kernel speedup. This is not
a production S1 or full N(6) speedup measurement. No released source, binary,
checkpoint or controller was changed by this experiment.

## Algorithms and exact invariant

The same executable contains three variants:

1. `RELEASED_TERNARY_DP`: the actual unchanged
   `computeDegree4RootedSplit`, included through the released shared-F4 bridge.
2. `FROZEN_LEAF_DSU_NODP`: the unchanged packed-degree, suffix-pruned core from
   the [no-DP GPU experiment](c6-gpu-no-dp-rooted4-20260905.md). It reconstructs
   both right-side disjoint-set structures at every accepted leaf.
3. `INCREMENTAL_DSU_NODP`: the new isolated header
   `experiments/proto/layer_cpu_f4_incremental_core.h`. It carries the two
   packed parent words and their combined component count at each DFS depth.

All receive the same sorted true-slot masks and the same fixed first-row
pair. The exact identity is

```text
F4(Q) = 6 * sum_H 2^(cycles(H) + cycles(Q-H)),
```

where H ranges over spanning two-factors containing the fixed first-row pair.
The factor six chooses the two colors on that pair. The suffix-pruned search
retains exactly the same leaves as the released rooted split; the earlier
no-DP report gives the degree and suffix-pruning proof.

**(A) Incremental-DSU proof.** Suppressing each degree-two left vertex produces
one edge between its two right neighbors. At depth d, keep one disjoint-set
forest for the selected edges of the first d assigned rows and another for
their complementary edges. The initial forests are identities followed by
the fixed root-row unions. On descent, copy both parent words to the next
depth and add precisely the selected and complementary row edges. Induction
on d proves that these are exactly the connectivity partitions obtained by
rebuilding all d row edges. A successful union reduces the corresponding
component count by one; a failed union does not. At a valid leaf each
suppressed multigraph is 2-regular, including possible parallel edges, so
its number of components equals its number of cycles. The accumulated
exponent is therefore identical to leaf reconstruction. Copy-on-descent
leaves the parent-depth words unchanged when a sibling is visited. No
canonicalization, multiplicity, factor six, or output-value semantics change.

The new traversal and frozen no-DP traversal have identical row choices,
suffix tests and counters. The finite gate also checks per-graph equality of
their nodes and loop iterations. Their counters need not equal the released
ternary-DP counters.

## Exact finite gate and retained artifacts

**(C) Executed, exact integer checks.** Every graph's F4 and valid-leaf count
agreed among all three variants and with the pre-existing pinned oracle.
Complete `(F4, leaves)` histograms also agreed. Every C4/C5 input's separate
`expected=F` field was checked. The released node counts agreed with the
retained released-core oracle; both no-DP variants agreed per graph in their
nodes and iterations.

| Domain | Input rows | Sum F4 | Sum valid leaves | Released nodes | No-DP nodes | No-DP iterations |
|---|---:|---:|---:|---:|---:|---:|
| Complete C4 L4 | 26 | 1,365,888 | 14,484 | 49,113 | 43,566 | 218,058 |
| Complete C5 L4 | 17,120 | 3,972,941,184 | 49,385,716 | 196,982,650 | 191,697,320 | 1,045,566,944 |
| Uniform C6 native sample | 1,024 | 1,512,032,640 | 15,919,967 | 70,460,389 | 73,817,597 | 421,203,377 |

These sums are unweighted diagnostic sums, not Sudoku counts. C6 is the
previously retained uniform native-record sample, not a full catalogue or a
uniform graph-representative sample. No oracle was silently regenerated.

Artifacts are under
`data/logs/c6-direct-route-20260905/cpu-three-kernel-ablation-v2/`:

- `c4.stdout.log`, `c5.stdout.log`, `c6.stdout.log`: every graph's true masks,
  values, leaves and traversal counters; complete histogram and every timing.
- `summary.json`: exact commands, pinned inputs/oracles/sources, all 54 timing
  phases, medians and resource evidence.
- Separate stderr logs; every accepted process exited zero and printed
  `CPU THREE KERNEL EXACT ABLATION PASSED`.

The combined suite passed in 98.964720700 seconds within one shared 120-second
deadline. Parent peak RSS was 29,241,344 bytes; the largest native child peak
was 63,229,952 bytes. The parent cap was 128 MiB and child cap 1,900 MiB, below
the aggregate 2 GiB bound. There were no GPU launches, giant catalogue reads,
checkpoint writes, or production counting processes overlapping the timing.
No benchmark process remained after completion.

One initial compile failed for a missing standard include; this was corrected.
An early runner invocation before the asynchronous compilation had completed
failed with `FileNotFoundError` before numerical work. Only the terminal
`v2` artifacts above are accepted evidence.

## Same-binary timing protocol and result

All three implementations are compiled into one binary using the same safe
flags. For each of one and 24 threads, a persistent OpenMP team first warms
all three kernels, including the released thread-local DP workspace. Three
cyclic orders balance each variant across all three timing positions:
old/leaf/incremental, leaf/incremental/old, incremental/old/leaf.

The timed region includes worksharing/barriers and exact checks on every
result, but excludes parsing, CSV/histogram formatting and file output.
There is no per-node clock call charged only to one arithmetic core: all
variants share an external/native watchdog; no-DP retains the declared
2-million-node experimental limit and integer overflow checks. All graphs
closed below that limit. C4 repeats the complete list 32 times per phase;
C5 and C6 each make one complete pass per phase.

**(D) Performance measurement, not a universal complexity theorem.** Medians
of the three phases, in seconds:

| Input and calls per phase | Threads | Released DP | Leaf DSU | Incremental DSU | Released / incremental |
|---|---:|---:|---:|---:|---:|
| C4, 832 calls | 1 | 0.0567513 | 0.0426734 | 0.0284725 | 1.993x |
| C4, 832 calls | 24 | 0.0036856 | 0.0026513 | 0.0018740 | 1.967x |
| C5, 17,120 calls | 1 | 9.4260908 | 7.3216023 | 5.7902770 | 1.628x |
| C5, 17,120 calls | 24 | 0.4825588 | 0.3768052 | 0.3001838 | 1.608x |
| C6 sample, 1,024 calls | 1 | 3.3581287 | 2.7118399 | 2.2722856 | 1.478x |
| C6 sample, 1,024 calls | 24 | 0.1757517 | 0.1437145 | 0.1219200 | 1.442x |

The C6 single-thread released phases were 2.8750977, 3.3801002 and 3.3581287
seconds, so the displayed median is not a confidence interval or a precision
claim. Its incremental phases were 2.3098437, 2.2722856 and 2.2286897 seconds.
The C6 24-thread released phases were 0.1738980, 0.1757517 and 0.1771040;
incremental phases were 0.1219200, 0.1216121 and 0.1239836.

The observed C6 no-DP traversal has more nodes than the released traversal,
yet runs faster. The third variant then removes repeated leaf-level unions
without changing that traversal. This supports a real arithmetic optimization
candidate. It does not establish the speed of production graph representatives,
whose distribution differs, nor include graph-fiber canonicalization, catalogue
lookup, chunk I/O, startup, backup or S2/S3. No projected complete N(6) time is
deduced. Promotion requires independent actual-engine integration, recovery
and numerical gates; this report alone does not promote the candidate.

## Reproduction and fingerprints

```powershell
g++ -O3 -mpopcnt -std=c++20 -fopenmp experiments/proto/layer_cpu_f4_ablation.cpp -o build/layer_cpu_f4_ablation.exe -lbcrypt -lpsapi
python experiments/proto/layer_cpu_f4_ablation_gate.py --worker build/layer_cpu_f4_ablation.exe --output-dir data/logs/c6-direct-route-20260905/cpu-three-kernel-ablation-v2
```

Use a fresh output directory for a new run; the accepted directory is immutable.
The gate pins exact input/oracle hashes and emits its expanded bounded child
commands. The full repository gate was not rerun for this isolated probe;
released code was unchanged. The complete C4/C5 L4 arithmetic and C6 sample
differential gate above is the proportional verification performed here.

```text
incremental core:
477495EDA0AB7E34A6AB1A766498E14B7D45ECF92B361F19956BB78DCA599A03
same-binary C++ harness:
3C1A6BD025C5290E8F05FD4372D413C0B5117287F709DB59D38E8E65262195E5
Python bounded gate:
ECFEC1B2F22FA0ABC27B72E90F46E4D3E92E771E2A976B11DDE5A936DC337695
compiled binary:
B49FD7C49DA0C4AD8F9E94C2F6B0862E9983D1852FFCF5300D5A3B4A0C53205A
accepted summary.json:
45A7A0024E2DB70C0DC334F286A34EE2D3251DA1AC72480C0AC3A40FC6088CB4
C4 stdout:
7CE93631134D2023518FDBCEF7CB04D37C5658AE2B6C6965B48C7303B18A12BC
C5 stdout:
92AD1E0034B39F5EE0518FC58EFB30C979F1B2F4864A7E339E44624763D0C7D6
C6 stdout:
DB7DB8965F0B49FC8527ACF3E71392EA8F15AD7390CE630982886C03A50365CA
```
