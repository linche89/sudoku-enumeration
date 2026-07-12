# Prototype Archive

This directory keeps research prototypes and differential tests that still have audit value.

These files are not part of the normal build. They are retained because the
legacy handoff and source comments cite them as evidence for rejected
shortcuts, profiling results, or validation of helper identities.

Highlights:

- `fast_hist.py` - Python differential test for the within-side histogram aggregation used by the multiset engines.
- `refprof.cpp` - refinement-canon residual profiler for realistic DP states.
- `aggtest.cpp` and `aggderive.cpp` - aggregation shortcut tests referenced in the handoff notes.
- `canon_check.py` and `canon_bb.py` - small canon experiments.
- `m4.txt` - compact C=4 prototype run summary.

Generated logs from this area should stay local and are ignored by `.gitignore`.
