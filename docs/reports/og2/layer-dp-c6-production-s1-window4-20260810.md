# C=6 production S1 window 4

Date: 2026-08-10 (Asia/Shanghai)

## Outcome

The fourth owner-authorized production `3->4` window resumed generation 26
at cursor 26/124 and stopped normally at the first durable checkpoint after
its eight-hour target:

```text
newest generation        = 37 (.a)
cursor                   = 38/124
completed parent chunks  = 30.6452%
chunk parents            = 100000
emissions                = 704741992192
cache hits               = 34280078
claimed L4 entries       = 903398620
parallel insertion holes = 18
real L4 states           = 903398602
checkpoint bytes         = 32522350448
checkpoint SHA-256       = ECF0837315B0FDF8AE21C394FDA6676490E43B1828A0825529A344EC17E4E844
```

This is safe partial S1 progress, not a closed L4 snapshot and not an `N(6)`
result.  The `T4` values remain partial until all 124 parent chunks close.

## Recovery and gates

The controller ran from commit
`5b5a988f06c2e831416e89b1a715a0e1a5c58e41`.  Before starting the engine it:

- completed `scripts/verify_all.ps1` successfully;
- verified that the external receipt covered local generation 26;
- rebuilt the layer-DP programs with the repository-safe flags;
- passed the transition `3->4` resource preflight with 94.085 GiB available
  RAM and 1,636.758 GiB available checkpoint-volume space.

The exact resumed command was:

```powershell
E:\Code\sudoku_FJ\build\layer_dp_gate.exe 6 `
  --threads 24 `
  --caps 2000,14000000,1350000000,250000000,100000 `
  --ckpt-chunk 100000 `
  --checkpoint E:\Code\sudoku_FJ\data\checkpoints\layer_dp_c6_s1_prod_20260802 40 `
  --stop-after 4 --ack-full-c6 `
  --resume E:\Code\sudoku_FJ\data\checkpoints\layer_dp_c6_s1_prod_20260802
```

The engine explicitly loaded generation 26 at cursor 26/124 with 903,398,613
claimed entries and 18 holes.  No closed prefix before cursor 26 was
recomputed.

## Window measurements

The engine ran from 08:36:05 to 17:02:20, or 8.4374 hours.  The cursor
advanced by 12 chunks and the cumulative counters changed by:

```text
emissions delta          = 219681931472
cache-hit delta          = 13231029
claimed-entry delta      = 7
real-state delta         = 7
```

Eleven checkpoint images, generations 27 through 37, were written.  One
image covered two newly closed chunks: generation 33 advanced from cursor 32
to cursor 34 because the time period had not elapsed at the intervening chunk
barrier.  Checkpoint write times were:

```text
21.84, 21.48, 21.14, 21.22, 22.46, 22.38,
22.29, 22.27, 22.29, 22.99, 21.55 seconds
```

Their minimum/mean/maximum were 21.14 / 21.992 / 22.99 seconds.  Peak RSS was
61.856 GiB; the minimum sampled available RAM was 32.454 GiB.  Engine and
controller stderr were empty.  The controller stopped with
`target_window_checkpoint` and produced no L4 snapshot.

## A/B recovery and external backup

Generation 36 remains locally as the previous `.b` fallback:

```text
generation               = 36 (.b)
cursor                   = 37/124
emissions                = 686604415424
cache hits               = 32904033
claimed / holes / real   = 903398620 / 18 / 903398602
bytes                    = 32522350448
SHA-256                  = B11F94B0D9A471A1FFACD340AD94FFE29874D537AB3FD2EACB3E1C8233F3FDF9
```

The controller copied generation 37 to:

```text
D:\sudoku_FJ_checkpoint_backups\layer_dp_c6_s1_prod_20260802\
session-008-20260810-170245-layer_dp_c6_s1_prod_20260802.a
```

and wrote the adjacent receipt.  A separate read-only audit on 2026-08-15
rehashed both the current local image and that D: copy.  Both independently
gave:

```text
ECF0837315B0FDF8AE21C394FDA6676490E43B1828A0825529A344EC17E4E844
```

The same audit decoded and validated the header values above and found no
running `layer_dp_gate` process.

## Historical-statistics limitation

The retained A/B images contain cumulative counters, not a complete
checkpoint-by-checkpoint history.  The stdout markers preserve generation,
cursor, claimed entries, path, and write duration, but not every generation's
emissions, cache hits, holes, or RSS-at-commit.  Those missing fields cannot
be reconstructed exactly and do not justify rerunning chunks 0 through 37.

An append-only sidecar introduced after this audit starts with a clearly
marked recovered baseline at generation 37.  It does not pretend to recover
the absent earlier per-checkpoint map.

## Decision boundary

Window 4 is closed as safe partial S1 progress.  A future S1 window must
resume generation 37 with the same production fingerprint and
`-ContinueExisting`.  S2 remains forbidden until cursor 124 closes, the
engine creates a complete `.L4.snap`, and both its local and external hashes
are entered in `data/checkpoints/MANIFEST.md`.
