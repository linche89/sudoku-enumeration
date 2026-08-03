# C=6 production S1 window 2

Date: 2026-08-03 (Asia/Shanghai)

## Outcome

The second owner-authorized production `3->4` window resumed generation 5 at
cursor 5/124 and closed normally at the first durable checkpoint after its
eight-hour target:

```text
newest generation        = 15 (.a)
cursor                   = 15/124
chunk parents            = 100000
emissions                = 280014646848
cache hits               = 12576350
claimed L4 entries       = 903398591
parallel insertion holes = 18
real L4 states           = 903398573
checkpoint bytes         = 32522349404
checkpoint SHA-256       = 99C2203AE846EC78627EA8C3774340E12C3F99C2C5DA6817C92217A7600218C8
```

This is 15/124 durable chunks, 12.171% of the parent indices and 13.127% of
the retained 2.13308e12-emission projection.  It is not a closed L4 result.

## Recovery and gates

The controller started from clean commit `b24d7b1` at 08:29:20.  Before
starting the engine it:

- reran `verify_all.ps1` to `ALL REPOSITORY CHECKS PASSED`;
- hashed local generation 5 and matched its external receipt;
- passed the C=6 S1 resource preflight;
- verified 94.592 GiB available RAM and 1,694.868 GiB available disk.

The engine then printed:

```text
resuming transition 3->4 ... (gen 5, cursor 5/124)
resume: transition 3->4 at chunk 5/124
  (child entries=903363975 holes=18)
```

Thus no completed prefix was recomputed.

## Window measurements

The session ended at 16:30:47 after 8.0241 hours.  It added ten durable
chunks, generations 6 through 15.  Their write times were:

```text
21.66, 23.11, 21.99, 22.23, 22.45,
21.88, 28.63, 23.11, 23.30, 21.17 seconds
```

Peak RSS was 61.853 GiB.  The last sampled working set was 46.620 GiB with
49.631 GiB available system RAM.  Engine and controller stderr were empty.

At eight hours the controller recorded nine new checkpoints, waited for the
next marker, observed generation 15, and stopped the engine with reason
`target_window_checkpoint`.  No engine or controller process remained.

## External backup

The controller copied generation 15 to:

```text
D:\sudoku_FJ_checkpoint_backups\layer_dp_c6_s1_prod_20260802\
session-006-20260803-163112-layer_dp_c6_s1_prod_20260802.a
```

It independently hashed the local and external files to

```text
99C2203AE846EC78627EA8C3774340E12C3F99C2C5DA6817C92217A7600218C8
```

and wrote the adjacent `.sha256.txt` receipt.  The immutable L3 source and
its original D: backup retained their authoritative SHA-256
`1D882DB7B0B18490E981539F7C690FA90227ECEC243817F5B538BEC8DAC865B7`.

## Decision boundary

Window 2 is closed as safe partial S1 progress.  Resume must use generation
15 with the same production configuration and `-ContinueExisting`.  A third
window needs an explicit owner decision; S2 remains out of scope until all
124 S1 chunks close and an exact L4 snapshot is verified.
