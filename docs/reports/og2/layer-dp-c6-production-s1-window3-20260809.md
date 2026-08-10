# C=6 production S1 window 3

Date: 2026-08-09 (Asia/Shanghai)

## Outcome

The third owner-authorized production `3->4` window resumed generation 15 at
cursor 15/124 and closed normally at the first durable checkpoint after its
eight-hour target:

```text
newest generation        = 26 (.b)
cursor                   = 26/124
chunk parents            = 100000
emissions                = 485060060720
cache hits               = 21049049
claimed L4 entries       = 903398613
parallel insertion holes = 18
real L4 states           = 903398595
checkpoint bytes         = 32522350196
checkpoint SHA-256       = D652E023F52F3E58EDFB35FA8E23195E1950B53CE7C9CBE7394FEA926E6795BF
```

This is 2,600,000 of 12,324,872 parent indices, or 21.096%, and 22.740% of
the retained 2.13308e12-emission projection.  It is not a closed L4 result.
Although the canonical-key inventory changed by only 22 real states during
this window, the stored `T4` values remain partial until all 124 chunks close.

## Recovery and gates

The controller started from clean commit `6b1944f` at 14:44:52.  Before
starting the engine it:

- reran `verify_all.ps1` to `ALL REPOSITORY CHECKS PASSED`;
- hashed local generation 15 and matched its external receipt;
- passed the C=6 S1 resource preflight;
- verified 87.248 GiB available RAM and 1,636.862 GiB available disk.

The engine then printed:

```text
resuming transition 3->4 ... (gen 15, cursor 15/124)
resume: transition 3->4 at chunk 15/124
  (child entries=903398591 holes=18)
```

Thus no completed prefix was recomputed.

## Window measurements

The session ended at 22:48:58 after 8.0682 hours.  It added eleven durable
chunks, generations 16 through 26, and 205,045,413,872 emissions.  Their
write times were:

```text
35.23, 22.13, 21.59, 27.49, 27.30, 23.54,
23.33, 23.71, 22.74, 24.85, 25.11 seconds
```

Peak RSS was 61.854 GiB.  The last sampled working set was 46.620 GiB.
Engine and controller stderr were empty.

At eight hours the controller recorded ten new checkpoints, waited for the
next marker, observed generation 26, and stopped the engine with reason
`target_window_checkpoint`.  No engine or controller process remained.

## External backup

The controller copied generation 26 to:

```text
D:\sudoku_FJ_checkpoint_backups\layer_dp_c6_s1_prod_20260802\
session-007-20260809-224925-layer_dp_c6_s1_prod_20260802.b
```

It independently hashed the local and external files to

```text
D652E023F52F3E58EDFB35FA8E23195E1950B53CE7C9CBE7394FEA926E6795BF
```

and wrote the adjacent `.sha256.txt` receipt.  The immutable L3 source and
its original D: backup retained their authoritative SHA-256
`1D882DB7B0B18490E981539F7C690FA90227ECEC243817F5B538BEC8DAC865B7`.

## Decision boundary

Window 3 is closed as safe partial S1 progress.  Resume must use generation
26 with the same production configuration and `-ContinueExisting`.  On
2026-08-10 the repository owner authorized another window with the same
eight-hour target, ten-hour hard bound, 85-GiB RSS guard, and 40-minute
checkpoint period.  S2 remains out of scope until all 124 S1 chunks close
and an exact L4 snapshot is verified.
