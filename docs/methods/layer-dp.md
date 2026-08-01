# Global row-incremental layer DP

## Status

`experiments/proto/layer_dp_gate.cpp` is a qualified C=6 implementation
candidate, not a completed C=6 result.  It is exact and independently gated
through C=5.  Its checkpoint/restart path has passed repeated process-kill,
wide-accumulator, corrupt-generation fallback, and snapshot round-trip tests.
No full C=6 layer-4 construction or 63,199-class contraction has been run.

The raw design notes under `docs/expert/2026-07-26/` and
`docs/expert/2026-07-27/` explain the route's provenance.  This file records
only the behavior retained by the current code and tests.

## Exact recurrence

Let `x` be a canonical native `L`-row state, let

```text
m_x = |G_C| / |Stab(x)|,
T_L(x) = m_x F_L(x),
```

and let `R_L(x,y)` be the sum of the exact native contingency coefficient
`K_c` over all one-row extensions of the concrete representative `x` whose
output canonicalizes to `y`.  The implemented recurrence is

```text
T_1(x_1) = 1,
T_{L+1}(y) = sum_x T_L(x) R_L(x,y).
```

For a complete class `q`,

```text
F_C(q) = T_C(q) / m_q,
N(C) = sum_q m_q ell(q) F_C(q)^2.
```

The division by `m_q` is checked exactly.  For C=6 the last transition uses
`u128` accumulators and writes exact per-class `(m,ell,F)` values; the final
square sum is deliberately external because an individual `m ell F^2` term
can exceed `u128`.

## Canonical states

A state is a sorted multiset of `2C` occurrence masks.  The coordinate group
is `C2 wr S_C`.  The production canonicalizer uses a prefix-refining orderly
search.  At layer `C-1` it additionally orders boxes by an invariant
missing-pair color; flips remain unrestricted, so stabilizer counting is not
replaced by a transporter guess.

`--invariance` checks equality under random group actions.  `--scan-check`
uses the full group as an independent referee for orbit membership and
stabilizer order, then compares the stored partition and `(T,stab)`
histogram with a definition-independent lexicographic full-group
canonicalizer.  Thus the retained gate covers invariance, separation, and a
histogram differential rather than relying only on final totals.

## Checkpoint format and recovery

Checkpoint format version 2 stores:

- the complete child arrays `keys`, `T`, `stab`, and final-layer `u128`
  values when present;
- parent/child layer indices, chunk geometry, completed-chunk cursor,
  emission counters, and the immutable parent-key hash;
- a configuration fingerprint covering the state representation,
  canonical-key semantics, `--wlseed`, and wide-accumulator mode;
- header and payload hashes plus an exact expected-file-length check.

Each parent chunk finishes at a closed OpenMP barrier before a checkpoint is
written.  The write is `tmp -> flush -> fsync/_commit -> atomic replace`.
Alternating `.a` and `.b` generations preserve an older valid image.  Resume
loads the newest fully valid generation, falls back if it is torn or corrupt,
loads the exact parent snapshot, verifies its ordered-key hash, rebuilds the
hash table, and continues at the first unapplied chunk.

On Windows, replacement is retried for at most five seconds only for the
documented transient access, sharing, lock, busy, or mapped-file errors.
Other errors fail immediately, and a persistent transient error still fails
closed.  The gate holds both generations open until the first retry is
observed, releases them, and then requires the resumed result to match the
golden class dump.

For a resumed fixed-capacity child, the loader reserves the final cap before
reading the compact payload.  Ownership of those buffers is moved into the
live layer and extended in place before its hash table is built.  The compact
checkpoint arrays therefore do not coexist with a second full-cap copy.

A completed checkpoint is made into the next immutable `.Lk.snap` with an
atomic hard link when the filesystem supports it.  This avoids rewriting a
large finalized layer and remains valid after later `.a`/`.b` replacements.
Cross-volume or unsupported cases fall back to a full checked write.

Checkpoint I/O failure is fail-stop.  A new run refuses a base containing any
existing generation or snapshot; use a fresh base or explicitly resume.
`--resume` and `--checkpoint` must name the same base, and mode/configuration
mismatches are rejected.

## Reproduction gate

Use the repository-safe compiler flags; do not add `-march=native` on the
documented Windows/MinGW workstation.

```powershell
powershell -NoProfile -ExecutionPolicy Bypass `
  -File scripts\build_layer_dp.ps1

powershell -NoProfile -ExecutionPolicy Bypass `
  -File scripts\verify_layer_dp.ps1 -Threads 8 -KillIterations 2
```

The gate covers:

- exact C=2, C=3, and C=4 totals;
- all 355 C=5 `(m,ell,F)` triples and exact `N(5)`;
- canonical invariance, full-group stabilizer/orbit checks, separation, and
  histogram differential;
- layer snapshot round trip and external exact summation;
- randomized kill/resume, including the forced-wide final transition and a
  deterministic Windows sharing-lock retry;
- mode mismatch and stale-base refusal;
- fail-stop checkpoint-write error;
- corrupt newest-generation fallback;
- read-only resource accounting and a staged layer-3-to-4 stop;
- the C=6 G1/G2 representative bridge;
- refusal of an acknowledged monolithic C=6 large layer;
- refusal of an unbounded ordinary C=6 invocation.

This gate is also called by `scripts/verify_all.ps1`.

## Uniform bounded calibration

The historical `--probe 3 ... --fan-sample ...` path samples four-row states
captured from a small set of three-row parents.  It is useful for local work
but is not a uniform sample of the global four-row orbit layer.

For the production calibration, `--m4-parent-seed S` selects layer-3 parents
by the smallest independently mixed canonical-key hashes.  When combined
with a high-capture `--m4probe`, `--fan-sample N` then selects four-row keys
uniformly from the captured canonical hash window.  Optional
`--fan-canon-sample N CAP` runs the real canonicalizing 4-to-5 kernel on an
independent uniform key sample and reports time, emissions, distinct children,
holes, cache hits, calls, and search nodes.

The fan estimate represents the global layer only when the M4 window is near
saturation.  The engine prints this limitation and never upgrades a killed
prefix to a result.

The completed C=6 run used 58,400 canonical-key-hash parents and captured
99.9133% of the estimated window population under Chao1.  It found:

```text
M4 observed-window lower estimate = 902080896
M4 Chao1 estimate                = 902863734
uniform 4->5 fan mean / SE       = 2617.482 / 4.408
projected 4->5 emissions         = 2.36323e12
canonical 4->5 sample            = 5260480 emissions at 96.7 ns/emission
```

The random and historical strided windows differed by only 41 observed keys
out of about 14.1 million, removing the material stride-bias concern.  M4 is
still an estimate rather than an exact layer count.  The fan interval is a
sampling interval and does not include residual capture-model uncertainty;
the timing does not include production-scale NUMA, table-fill, or checkpoint
effects.

## C=6 boundary

The engine refuses plain `layer_dp_gate 6`.  Read-only bridge inspection and
explicitly bounded probes are allowed.  A large C=6 stage additionally
requires the owner-facing `--ack-full-c6` flag, a checkpoint base, threaded
fixed capacities, and, for the final chain, a CSV dump.  That flag is only a
local guard against accidental launch; it does not replace the repository
requirements for a recent full gate, external time/RSS bounds, free-space
review, and checkpoint protection.

Large work is restricted to one transition per process: load or resume layer
3 and stop at 4, load or resume layer 4 and stop at 5, then load or resume
layer 5 for the final transition.  The engine runs the resource preflight
automatically before any such acknowledged stage.

The two independently known complete-class anchors are wired before any
future final result can be accepted:

```text
F6(G1) = 6986348258918400
F6(G2) = 7053808087203840
```

A complete C=6 result must also contain exactly 63,199 classes.  Neither
condition has been exercised by a full layer-DP run.

## Resource preflight

`--resource-preflight L` is read-only.  It requires the intended thread count,
all fixed capacities, and a checkpoint base so RAM and the actual target
volume can be inspected.  It creates no checkpoint files.

The capacity-worst-case model includes:

- parent and child arrays and hash tables;
- per-thread raw-child caches;
- per-thread and global `u128` vectors on the final transition;
- cap-reserved in-place resume loading;
- retained earlier A/B generations;
- the active A/B pair plus the third full `.tmp` image present during atomic
  replacement;
- an 8 GiB or 10% RAM margin and a 16 GiB or 10% disk margin, whichever is
  larger.

With the still-provisional capacities
`2000,14000000,1350000000,250000000,100000`, 24 threads, and the reference
125.650 GiB host, the measured plan on 2026-07-31 was:

| transition | peak RAM | RAM with margin | retained disk peak | disk with margin |
|---|---:|---:|---:|---:|
| 3->4 | 61.898 GiB | 69.898 GiB | 136.726 GiB | 152.726 GiB |
| 4->5 | 71.685 GiB | 79.685 GiB | 116.609 GiB | 132.609 GiB |
| 5->6 | 10.465 GiB | 18.465 GiB | 108.242 GiB | 124.242 GiB |

These are capacity bounds, not measured production allocation.  The
1.35-billion layer-4 cap is now supported by the completed M4/random-fan
measurement.  The 250-million layer-5 cap remains provisional.

## Remaining preflight work

The route is restart-safe at C=5, and the M4 plus uniform 4-to-5 measurement
gates are complete.  A production decision still requires:

1. a stronger bound or guarded measurement for the provisional layer-5 cap;
2. a bounded staged allocation/restart rehearsal under the resource guard;
3. a bounded end-to-end rehearsal, including deliberate interruption and
   external summation;
4. an owner decision before any multi-day C=6 layer-4 run.
