# Layer-DP S0 snapshot and random calibration handoff

Date: 2026-07-31

## Outcome

A bounded S0 run generated and independently reloaded the complete native
C=6 layer 3.  The next M4/4-to-5 probe was improved to use canonical-key
hash samples, but its C=6 execution was stopped at the owner's request before
completion.  No partial M4 or fan statistic is accepted.

## Why the probe changed

The retained 2026-07-27 4-to-5 calibration built 24,810,566 four-row states
from 150 strided three-row parents and sampled 20,000 entries from that local
capture.  Its mean of 2,731 native tables per state is useful evidence, but
the captured set is weighted by reachability from those parents and is not a
uniform sample of the global four-row orbit layer.

The new bounded path has two independent canonical-key selections:

1. `--m4-parent-seed` chooses layer-3 parents by the smallest mixed hashes of
   their canonical keys, independent of parallel insertion order.
2. After the hash-window M4 pass, `--fan-sample` chooses four-row keys by a
   separate mixed key hash.  At the 4x probe's near-saturated capture this is
   an approximately uniform global orbit sample.

`--fan-canon-sample N CAP` independently samples captured four-row keys and
runs the real canonicalizing 4-to-5 kernel into a bounded table.  It reports
the arithmetic fan and the actual canonicalization constant separately.

The C=5 gate exercises all three modes.  The complete repository gate then
passed, including canonical invariance/separation/histogram differentials,
all exact C=2..5 totals, both FJ9 routes, and read-only C=6 checkpoint
verification.

## Checkpoint protection

Before the writable S0 run, the active factorization checkpoint was copied
to:

```text
D:\sudoku_FJ_checkpoint_backups\
  factorization_orbit_c6_graphmemo_20260731_pre_layerdp.bin
```

Both copies had SHA-256:

```text
FE8B68DE6C15592848D7CF69BF43928C000F0B2EE59FC263A094BCA2D376A865
```

## Exact S0 result

The run used 24 threads under a 2 GiB / 15 minute `watch_rss.ps1` guard:

```text
1->2 emissions = 59245120
layer-2 states = 772
layer-2 orbit mass = 20338525 [OK]
2->3 emissions = 2605194602
layer-3 states = 12324872
parallel insertion holes = 1
layer-3 orbit mass = 566455903200
wall = 521.3 seconds
peak RSS = 661.1 MiB
```

Snapshot:

```text
data\checkpoints\layer_dp_c6_layer3_20260731.snap
size = 443695556 bytes
SHA-256 =
1D882DB7B0B18490E981539F7C690FA90227ECEC243817F5B538BEC8DAC865B7
```

Its physical backup under `D:\sudoku_FJ_checkpoint_backups\` has the same
size and hash.  A read-only load reported 12,324,872 states and orbit mass
566,455,903,200 again.

## Stopped M4 run

The random 4x probe was launched with a 12 GiB / 30 minute guard.  It reached
approximately 3.068 GiB RSS and was explicitly terminated after about
45 seconds when the owner deferred the long test.  No layer-DP process
remained, the mode writes no checkpoint, and the output contained no
completed histogram.  Therefore it contributes no mathematical result.

Tomorrow's run can start directly from the retained layer-3 snapshot and
should take about 20 minutes rather than rebuilding S0 first.  The exact
guarded command is in `docs/runbooks/og2.md`.
