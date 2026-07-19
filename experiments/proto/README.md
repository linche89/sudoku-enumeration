# Prototype Archive

This directory keeps research prototypes and differential tests that still have audit value.

These files are not part of the normal build. They are retained because the
legacy handoff and source comments cite them as evidence for rejected
shortcuts, profiling results, or validation of helper identities.

Highlights:

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
