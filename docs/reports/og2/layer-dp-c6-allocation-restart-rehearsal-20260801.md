# C=6 production-cap allocation and restart rehearsal

Date: 2026-08-01

## Outcome

The bounded C=6 layer-3-to-layer-4 rehearsal passed with the intended
production capacity:

```text
layer-4 fixed capacity = 1,350,000,000
measured peak RSS      = 61.810 GiB
external RSS limit     = 75 GiB
per-phase time limit   = 4 minutes
```

The run created alternating checkpoints, was deliberately terminated,
resumed the exact cursor and child table in place, wrote later generations,
and then fully reloaded the final retained generation without modifying it.
No complete layer 4, layer 5, final contraction, or `N(6)` run was attempted.

## Preconditions

The complete repository gate had just passed at the functional tree later
committed as `34c298f`.  The active checkpoints and external backups matched:

```text
factorization checkpoint and D: backup
FE8B68DE6C15592848D7CF69BF43928C000F0B2EE59FC263A094BCA2D376A865

layer-3 snapshot and D: backup
1D882DB7B0B18490E981539F7C690FA90227ECEC243817F5B538BEC8DAC865B7
```

Available RAM was 87.732 GiB and free E: space was 1,827.418 GiB.  The
read-only resource preflight required 69.898 GiB RAM including its 8 GiB
margin and 152.726 GiB disk including its 16 GiB margin; both passed.

## Guarded command shape

Every child process ran through `scripts/watch_rss.ps1` with `LimitGB=75`,
`MaxMinutes=4`, and five-second RSS sampling.  The initial engine arguments
were:

```text
6 --threads 24
--caps 2000,14000000,1350000000,250000000,100000
--load-layer 3 data\checkpoints\layer_dp_c6_layer3_20260731.snap
--checkpoint data\logs\layer-dp-c6-allocation-rehearsal-20260801\ck 0
--ckpt-chunk 1 --stop-after 4 --ack-full-c6
```

The resume phases replaced `--load-layer` with:

```text
--resume data\logs\layer-dp-c6-allocation-rehearsal-20260801\ck
```

Chunk size one and checkpoint period zero are deliberately pathological test
settings.  They force many quiescent writes quickly and are not production
throughput settings.

## Phase evidence

### Initial allocation and forced stop

The process loaded all 12,324,873 stored layer-3 entries, including the one
known insertion hole, and hard-linked the immutable parent snapshot into the
fresh checkpoint set.  RSS rose through 21.720 and 45.819 GiB to a stable
61.809 GiB.

It wrote generations 1 through 91.  The last completed image before the
intentional kill was:

```text
gen=91 cursor=91 entries=11,375,523 holes=0
```

The guarded process was stopped after 71.6 seconds, well before its four
minute limit.

### Resume and continued writes

The second process performed the resource preflight again and fully loaded
the newest valid payload:

```text
resuming transition 3->4 ... gen 91, cursor 91/12324873
child entries=11,375,523 holes=0
```

This exercises cap reservation before reading, payload/header hash checks,
exact parent-key hash verification, in-place child installation, hash-table
rebuild, and cursor continuation.  It then wrote through generation 146 and
was deliberately stopped after 68.1 seconds.  Peak RSS was 61.810 GiB.

### Second-generation recovery and final readback

A third process fully loaded generation 146:

```text
gen=146 cursor=146 entries=18,889,204 holes=0
```

and continued writing through generation 170.  A final process used a
60-minute checkpoint period so it could fully read generation 170 without
replacing either retained A/B image:

```text
gen=170 cursor=170 entries=21,993,609 holes=0
```

The A/B SHA-256 values were identical before and after that final readback.
All four engine stderr files were empty.  The four monitored process windows
totalled about 4.1 minutes; the complete interactive rehearsal, including
checks between phases, stayed below ten minutes.

## Retained checkpoint evidence

The ignored forensic checkpoint remains under
`data/logs/layer-dp-c6-allocation-rehearsal-20260801`.  Its final headers are:

| image | generation | cursor | entries | holes | emissions | bytes |
|---|---:|---:|---:|---:|---:|---:|
| `ck.a` | 169 | 169 | 21,812,484 | 0 | 28,591,008 | 785,249,552 |
| `ck.b` | 170 | 170 | 21,993,609 | 0 | 28,801,824 | 791,770,052 |

Both expected-file-length equations and header hashes pass.  SHA-256:

```text
ck.a 63EBEAA9C9778ACDE6E5FEC846059BF5734C4D5FB454BCD7035A0AC6A89A9D22
ck.b A30428CC9AA133360C474599C8A8A078E4A0CF19908353593ACD747BA434A0F5
```

The hard-linked `ck.L3.snap` retains the authoritative layer-3 SHA-256
`1D882DB7...65B7`.  The active source checkpoints and both D: backups were
unchanged after the rehearsal.  No layer-DP process remained.

## Decision

The production-sized layer-4 allocation and 3-to-4 checkpoint/restart path
are now exercised on real C=6 states.  The measured 61.810 GiB peak agrees
with the modeled 61.898 GiB runtime peak to 0.088 GiB.

This closes the allocation/restart rehearsal gate.  It does not measure a
production checkpoint period, long-run table-fill behavior, a completed
layer-4 state count, or the later 4-to-5 and 5-to-6 stages.  The remaining
preflight is a bounded interrupted end-to-end rehearsal plus an owner
decision before any multi-day production stage.
