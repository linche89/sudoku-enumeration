# Prototype and Decision Engines

This directory keeps research prototypes and differential tests that still
have audit value.  Most are outside the normal build.  The layer-DP candidate
is the exception: the full repository verification builds and gates it through
its dedicated scripts, while its source remains here until the C=6 preflight
is complete.

Highlights:

- `layer_dp_gate.cpp` - the active global row-incremental C=6 candidate.
  It is exact through C=5 and has a gated, fail-stop checkpoint/restart path,
  but no full C=6 layer-4 or final count has been run.  Build and verify it
  only through `../../scripts/build_layer_dp.ps1` and
  `../../scripts/verify_layer_dp.ps1`; see
  `../../docs/methods/layer-dp.md`.
- `source_target_frontier_bound.cpp` - a fast arithmetic certificate for the
  fixed-source target-frontier lower bound.  It derives a reachable C=6
  grade-2 witness, proves its stabilizer is trivial, cross-checks two permanent
  algorithms against direct balanced-map enumeration, and verifies all 20
  three-column split supports.  It allocates no C=6 layer and touches no
  checkpoint.  See
  `../../docs/reports/og2/source-target-frontier-lower-bound-20260720.md`.
- `band_kernel_rank.cpp` - complete explicit reduced band-kernel matrices and
  exact rank certificates through C=4, plus a safety-bounded C=5 signed target
  CountSketch.  The retained 1,024-square sketch proves the `38801 x 38801`
  middle map has rational rank at least 1,024; it does not claim full rank.
  See `../../docs/reports/og2/band-kernel-rank-20260719.md`.
- `connectivity_operator.cpp` - exact Windows decision prototype for the
  symbol-synchronous connectivity recurrence and its proposed
  double-permanent subset transition.  Direct/subset raw coefficients agree
  through C=4; bounded C=5 probes prove the labelled subset frontier is
  injective and non-compressive.  It is hard-limited to C<=5; see
  `../../docs/methods/connectivity-operator.md`.
- `joint_histogram.cpp` - exact box-order joint used-color histogram for the
  squared objective.  It differentially reproduces every raw transition and
  the sequential/midpoint totals through C=4.  Its bounded C=5 modes close the
  seven-state first layer, stream deterministic layer-2 prefixes, and count
  the exact 652,001,548-leaf allocation inventory with a scalar cost DP.  It
  cannot launch C=6; see `../../docs/methods/joint-histogram.md`.
- `reverse_glue.cpp` - exact reverse row-block gluing decision engine.  It
  differentially closes C=2..4, reproduces all 355 C=5 classes through
  `2+2 -> 4` and `4+1 -> 5`, and supports closed pair-interval files.  Its
  safety-bounded C=6 mode constructs the exact two-row orbit inventory and
  samples four-row placements; it cannot launch a complete C=6 layer or write
  a C=6 partial file.  See `../../docs/methods/reverse-gluing.md`.
- `fast_hist.py` - Python differential test for the within-side histogram aggregation used by the multiset engines.
- `refprof.cpp` - refinement-canon residual profiler for realistic DP states.
- `aggtest.cpp` and `aggderive.cpp` - aggregation shortcut tests referenced in the handoff notes.
- `canon_check.py` and `canon_bb.py` - small canon experiments.
- `m4.txt` - compact C=4 prototype run summary.

Generated logs from this area should stay local and are ignored by `.gitignore`.
