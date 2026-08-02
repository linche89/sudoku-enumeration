# C=6 bounded interrupted end-to-end rehearsal

Date: 2026-08-02 (Asia/Singapore)

## Outcome

The production-shaped bounded rehearsal passed the complete staged path:

```text
exact L3 seed
  -> sparse 3->4 with deliberate kill and exact resume
  -> sparse 4->5 in a fresh process
  -> sparse 5->6 and watermarked CSV
  -> independent 5->6 replay
  -> external arbitrary-precision weighted-square checksum
```

The deterministic rehearsal kept approximately one canonical parent in 100
at every transition from layer 3 onward.  It is an exact computation for that
artificial sparse operator, but it is not a complete C=6 layer chain and the
result is not `N(6)`.

The final two sorted CSV files were byte-identical:

```text
captured complete classes = 63,117
CSV SHA-256               = B247C170370D7936D3406E485BF4A50E27198BB2AD86F16CDE32B19F87038A96
rehearsal checksum        = 38528041484076505706899541562753024000
```

The CSV column is named `F_rehearsal`, and the external tool printed
`NOT N(6)` before and beside the checksum.

## Safety mechanism

Commit `88b61dc` introduced `--rehearsal-denom D` and the driver
`scripts/rehearse_layer_dp_c6.ps1`.  The mode has three fail-closed
distinctions from production:

1. parent selection is deterministic from the canonical key, layer domain,
   fixed rehearsal seed, and denominator;
2. layer-4 and later checkpoint fingerprints bind the rehearsal lineage and
   denominator, so production mode or a different denominator refuses them;
3. final CSV uses `F_rehearsal`, accepted by the external summation helper
   only with its explicit `--rehearsal` flag.

The complete exact production L3 snapshot remains importable because layers
1 through 3 retain the original version-2 production fingerprint.  Every
descendant written by the sparse operator is isolated from production.

The C=5 regression gate deliberately interrupts and resumes a denominator-2
chain, checks production-lineage and denominator-mismatch refusals, replays
the final stage byte-for-byte, and reproduces the fixed rehearsal checksum

```text
119726523775193340292890624000
```

over 354 captured classes.  The ordinary complete C=5 result remains
unchanged.

## Pre-run gates and command

Before the writable C=6 rehearsal:

- `scripts/verify_layer_dp.ps1 -Threads 8 -KillIterations 1` passed in
  36.2 seconds;
- `scripts/verify_all.ps1` passed with exit code 0 in 181 seconds;
- the active and external-backup L3 snapshots both had SHA-256
  `1D882DB7B0B18490E981539F7C690FA90227ECEC243817F5B538BEC8DAC865B7`;
- a denominator-100,000 C=6 smoke rehearsal passed the same interrupted
  S1-S4 mechanics under an 8-GiB guard.

The retained run used:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass `
  -File scripts\rehearse_layer_dp_c6.ps1 `
  -Threads 24 -Denom 100 `
  -CheckpointMinutes 10 -ChunkParents 100000 `
  -RssLimitGB 85 -MaxStageMinutes 240 `
  -RunDir data/logs/layer-dp-c6-e2e-rehearsal-20260802-1pct
```

Capacities were the retained production plan:

```text
2000,14000000,1350000000,250000000,100000
```

## Deliberate interruption and continuation hardening

The first S1 process wrote generation 1 at chunk 23 of 124:

```text
entries          = 885,171,971
checkpoint bytes = 31,866,191,084 (29.68 GiB)
checkpoint write = 35.00 seconds
elapsed to kill  = 11.213 minutes
peak RSS         = 61.859 GiB
```

The driver observed the durable `checkpoint gen=` marker and then forcibly
terminated the process.  Its first immediate resume attempt was correctly
rejected by the engine's resource preflight: Windows had not yet reclaimed
the killed process's 62-GiB working set, so only 34.437 GiB was reported
available.  No checkpoint was modified.

This operational finding produced commits `acc0ae0` and `244ef7a`:

- continuation now waits, for at most five minutes, until at least 80 GiB is
  available before starting the next large process;
- a null Windows PowerShell `Start-Process.ExitCode` is accepted only if the
  engine emitted a terminal success marker;
- `-ContinueExisting` resumes an existing S1/S2/S3 generation or skips an
  already closed stage;
- retained RSS logs repopulate the summary across separate invocations.

After RAM recovered to 93.877 GiB, generation 1 loaded and resumed at chunk
23 without recomputing the completed prefix.

## Stage measurements

| stage | selected parents | emissions | closed children | holes | engine transition time | ns/emission | sampled wall | peak RSS |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| killed S1 prefix | n/a | prefix | 885,171,870 at gen1 | 101 | interrupted | n/a | 11.213 min | 61.859 GiB |
| S1 3->4, complete sparse operator | 123,571 / 12,324,872 | 21,387,180,240 | 903,346,741 | 102 | 2,573.085 s after resume | n/a; old log rate is resume-biased | 54.373 min across killed + resumed processes | 61.861 GiB |
| S2 4->5 | 9,029,054 / 903,346,741 | 23,637,232,504 | 96,452,753 | 221 | 2,004.952 s | 84.8 | 34.555 min | 48.703 GiB |
| S3 5->6 | 964,867 / 96,452,753 | 55,653,192 | 63,117 | 2 | 16.811 s transition | 302.1 | 3.270 min including full stabilizer scan | 4.294 GiB |
| S3 replay | same | 55,653,192 | 63,117 | 3 | 16.557 s transition | 297.5 | 3.204 min including full stabilizer scan | 4.294 GiB |

The sampled watcher time totals 95.402 minutes.  Wall time from the original
S1 launch through the final checksum was about 99 minutes, including the
intentional kill, diagnosis, script patch, RAM reclamation, and restart.

The two S3 executions differed only in harmless parallel insertion holes.
They produced the same real state set, coefficients, sorted CSV bytes, and
external checksum.

The S1 engine line printed 120.3 ns/emission, but that rate is not a valid
kernel measurement: the resumed process divided its post-resume time by the
cumulative emission counter, including the killed prefix.  The two guarded
S1 process windows totalled 54.373 minutes, or 152.5 ns per cumulative
emission including allocation, load, checkpoint, and restart overhead.  The
diagnostic was corrected after the run to report prior and this-process
emissions separately; no recurrence or stored value changed.

## Checkpoint-period evidence

S1 completed generations at cursors 23, 47, 71, 96, and 124.  Its checkpoint
writes took:

```text
35.00, 24.62, 23.23, 22.69, 23.15 seconds
```

The finalized S1 checkpoint header was:

```text
gen=5 cursor=124/124 entries=903346843 holes=102
emissions=21387180240 chunkParents=100000
```

S2 completed generations at cursors 2643, 5335, 8146, and 9034.  Writes took:

```text
7.25, 7.30, 8.36, 6.30 seconds
```

Its finalized header was:

```text
gen=4 cursor=9034/9034 entries=96452974 holes=221
emissions=23637232504 chunkParents=100000
```

Thus a 10-minute period costs roughly 4% steady-state checkpoint overhead in
S1 and 1-1.5% in S2.  For a multi-day production run, 40 minutes is the
recommended owner choice: measured steady-state overhead falls below about
1% in S1 while the maximum recomputation window remains about 40 minutes.
This is a runbook recommendation, not an encoded default or authorization.

## Arithmetic cross-checks

The rehearsal supplied two much larger uniform-hash fan measurements than
the earlier probes:

```text
S1 mean fan = 21,387,180,240 / 123,571 = 173,076.0473
S2 mean fan = 23,637,232,504 / 9,029,054 = 2,617.9080
```

The S2 mean differs by only 0.426 (0.0163%) from the prior independent
20,000-state mean 2,617.482.  This strongly corroborates that calibration,
but the sparse rehearsal is still not a theorem-level full-layer count.

At S2 closure there were 96,452,753 real L5 states, exactly two fewer than
the independent complete Burnside value 96,452,755.  The final chain reached
63,117 of the 63,199 complete coordinate classes and captured labelled mass

```text
622345240533139200 <= 622345892187672576.
```

These deficits are useful positive evidence that the lineage watermark is
semantically real: the output cannot be mistaken for a complete C=6 vector.

## External summation and integrity

`s4_exact_sum.py --rehearsal --classes 63117` checked contiguous qids,
representative width, positive weights, the rehearsal CSV watermark, and the
captured-mass upper bound before computing:

```text
rehearsal_checksum(6) =
38528041484076505706899541562753024000  (NOT N(6))
```

The active L3 snapshot and its D: backup retained their authoritative hash
before and after the run.  All engine stderr files were empty, no
`layer_dp_gate` process remained, and the ordinary Git worktree contained no
generated checkpoint files because `data/logs/` is ignored.

After correcting the resume-rate diagnostic, the proportional layer gate

```powershell
powershell -NoProfile -ExecutionPolicy Bypass `
  -File scripts\verify_layer_dp.ps1 -Threads 8 -KillIterations 1
```

passed in 36.0 seconds.  The complete repository gate

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts\verify_all.ps1
```

then passed with exit code 0 in 176.6 seconds.  The diagnostic change affects
only the printed denominator for a resumed process; checkpoint bytes and
arithmetic are unchanged.

## Decision

The bounded interrupted end-to-end preflight is closed.  It exercised every
S1-S4 control and data-plane boundary at C=6, including a real durable kill,
resume from a 29.68-GiB image, stage-separated snapshots, wide final
accumulation, complete-class scanning, replay determinism, and external exact
arithmetic.

It does not authorize or constitute a complete C=6 count.  The only remaining
pre-production gate is the repository owner's explicit decision to commit
the multi-day S1 resource run, including the checkpoint-period choice.

## Retained transient evidence

The ignored evidence directory is:

```text
data/logs/layer-dp-c6-e2e-rehearsal-20260802-1pct
```

It contains both S1 and S2 A/B generations, hard-linked finalized parent
snapshots, the two S3 checkpoint chains and CSVs, RSS logs, stdout/stderr,
the external-checksum log, and `summary.txt`.  The two large closed snapshots
are rehearsal lineage only and must never be promoted to
`data/checkpoints/` or used with production mode.
