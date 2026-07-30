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
- randomized kill/resume, including the forced-wide final transition;
- mode mismatch and stale-base refusal;
- fail-stop checkpoint-write error;
- corrupt newest-generation fallback;
- the C=6 G1/G2 representative bridge;
- refusal of an unbounded ordinary C=6 invocation.

This gate is also called by `scripts/verify_all.ps1`.

## C=6 boundary

The engine refuses plain `layer_dp_gate 6`.  Read-only bridge inspection and
explicitly bounded probes are allowed.  A large C=6 stage additionally
requires the owner-facing `--ack-full-c6` flag, a checkpoint base, threaded
fixed capacities, and, for the final chain, a CSV dump.  That flag is only a
local guard against accidental launch; it does not replace the repository
requirements for a recent full gate, external time/RSS bounds, free-space
review, and checkpoint protection.

The two independently known complete-class anchors are wired before any
future final result can be accepted:

```text
F6(G1) = 6986348258918400
F6(G2) = 7053808087203840
```

A complete C=6 result must also contain exactly 63,199 classes.  Neither
condition has been exercised by a full layer-DP run.

## Remaining preflight work

The route is restart-safe at C=5, but a production decision still requires:

1. a measured C=6 layer-4 capacity bound from stronger `M_4` probes;
2. an unbiased layer-4-to-layer-5 fan/canonicalization calibration;
3. explicit RAM, transient-resume RAM, disk, and wall-time preflight;
4. a bounded staged rehearsal, including deliberate interruption and
   external summation;
5. an owner decision before any multi-day C=6 layer-4 run.
