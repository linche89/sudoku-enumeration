# Checkpoint Manifest

## factorization_orbit_c6_graphmemo.bin

```text
format magic: FJFAC01
size: 217954462 bytes
entries: 5315962
sha256: FE8B68DE6C15592848D7CF69BF43928C000F0B2EE59FC263A094BCA2D376A865
compatible implementation: 0713d27 and descendants
```

Coverage:

- C=6 outer class 1 is complete:
  `F6(G1) = 6986348258918400`.
- Outer class 2 contains its first 50 newly evaluated degree-5 values.
- Every stored value is closed and exact; no partial accumulator is trusted.

Local safety backup created before repository reorganization:

```text
E:\Code\sudoku_FJ_backup\factorization_orbit_c6_graphmemo_20260712_pre_reorg.bin
```

Both copies had the SHA-256 value above when the reorganization began.

Verification must pass `checkpointreadonly`; ordinary runs save atomically via
a temporary file and replace operation.

## layer_dp_c6_layer3_20260731.snap

```text
format magic: LDPCAN01
format version: 2
size: 443695556 bytes
stored entries: 12324873
closed states: 12324872
parallel insertion holes: 1
orbit mass: 566455903200
sha256: 1D882DB7B0B18490E981539F7C690FA90227ECEC243817F5B538BEC8DAC865B7
compatible implementation: 78835bf descendants with unchanged LDPCAN01 key semantics
```

The bounded S0 run completed both transitions exactly:

```text
1->2 emissions = 59245120
layer-2 states = 772
layer-2 orbit mass = 20338525
2->3 emissions = 2605194602
layer-3 states = 12324872
layer-3 orbit mass = 566455903200
```

It ran under a 2 GiB / 15 minute external guard, took 521.3 seconds, and
peaked at 661.1 MiB.  A read-only load/round-trip reproduced the stored state
count and orbit mass.

External physical backup:

```text
D:\sudoku_FJ_checkpoint_backups\layer_dp_c6_layer3_20260731.snap
```

Source and backup had the SHA-256 value above after copying.

## layer_dp_c6_s1_prod_20260802 (active production namespace)

The repository owner has authorized four bounded production S1 (`3->4`)
windows individually.  Each used an eight-hour target, ten-hour hard process
bound, 85-GiB RSS bound, and a 40-minute checkpoint period after the initial
recovery.  This is partial resumable work, not a closed layer or a C=6 result.

```text
checkpoint base:
E:\Code\sudoku_FJ\data\checkpoints\layer_dp_c6_s1_prod_20260802

external backup directory:
D:\sudoku_FJ_checkpoint_backups\layer_dp_c6_s1_prod_20260802

controller:
scripts\run_layer_dp_c6_s1_window.ps1
```

The immutable parent is `layer_dp_c6_layer3_20260731.snap` with the SHA-256
recorded above.  Before every resume, the controller requires an external
SHA-256-verified copy of the newest durable generation.  After every bounded
window it copies the newest successful A/B generation (or closed L4
snapshot) to the external directory and writes a receipt.  The latest
closed-window cursor and hash must be distilled here before handoff; runtime
details remain under `data/logs/layer_dp_c6_s1_prod_20260802/`.

First-window state after workstation sleep and recovery review:

```text
status: partial S1, safe to resume; NOT a closed L4
generation: 5 (.a)
cursor: 5/124 chunks, chunkParents=100000
emissionsSoFar: 92717503648
claimed entries: 903363975
parallel insertion holes: 18
real states: 903363957
local bytes: 32521103228
local SHA-256: 357ACEF8F257FAB819D53BB11E1E0AF2FBA882406F4A5196805895C9BD51274A
external SHA-256: 357ACEF8F257FAB819D53BB11E1E0AF2FBA882406F4A5196805895C9BD51274A
```

The external image and receipt are:

```text
D:\sudoku_FJ_checkpoint_backups\layer_dp_c6_s1_prod_20260802\
  session-004-recovered-gen5-layer_dp_c6_s1_prod_20260802.a
D:\sudoku_FJ_checkpoint_backups\layer_dp_c6_s1_prod_20260802\
  session-004-recovered-gen5-layer_dp_c6_s1_prod_20260802.a.sha256.txt
```

The original controller failed to select A/B because PowerShell parsed the
unparenthesized path-suffix array as one concatenated element.  Commit
`bfbfe4e` parenthesizes every candidate, ignores generated `.a/.b` images,
and fixes progress formatting.  A subsequent complete repository gate plus
`-PrepareOnly -ContinueExisting` recognized the local generation and its
external receipt, passed resource preflight, and wrote no checkpoint.

Prior authoritative state after production window 3:

```text
status: partial S1, safe to resume; NOT a closed L4
generation: 26 (.b)
cursor: 26/124 chunks, chunkParents=100000
emissionsSoFar: 485060060720
cacheHitsSoFar: 21049049
claimed entries: 903398613
parallel insertion holes: 18
real states: 903398595
local bytes: 32522350196
local SHA-256: D652E023F52F3E58EDFB35FA8E23195E1950B53CE7C9CBE7394FEA926E6795BF
external SHA-256: D652E023F52F3E58EDFB35FA8E23195E1950B53CE7C9CBE7394FEA926E6795BF
```

The current external image and receipt are:

```text
D:\sudoku_FJ_checkpoint_backups\layer_dp_c6_s1_prod_20260802\
  session-007-20260809-224925-layer_dp_c6_s1_prod_20260802.b
D:\sudoku_FJ_checkpoint_backups\layer_dp_c6_s1_prod_20260802\
  session-007-20260809-224925-layer_dp_c6_s1_prod_20260802.b.sha256.txt
```

Window 3 ran for 8.0682 hours, peaked at 61.854 GiB RSS, stopped at
`target_window_checkpoint`, verified the external copy, and left no engine or
controller process.  Detailed evidence is in
`docs/reports/og2/layer-dp-c6-production-s1-window3-20260809.md`.

Current authoritative state after production window 4:

```text
status: partial S1, safe to resume; NOT a closed L4
generation: 37 (.a)
cursor: 38/124 chunks, chunkParents=100000
completed parent chunks: 30.6452%
emissionsSoFar: 704741992192
cacheHitsSoFar: 34280078
claimed entries: 903398620
parallel insertion holes: 18
real states: 903398602
local bytes: 32522350448
local SHA-256: ECF0837315B0FDF8AE21C394FDA6676490E43B1828A0825529A344EC17E4E844
external SHA-256: ECF0837315B0FDF8AE21C394FDA6676490E43B1828A0825529A344EC17E4E844
```

The current external image and receipt are:

```text
D:\sudoku_FJ_checkpoint_backups\layer_dp_c6_s1_prod_20260802\
  session-008-20260810-170245-layer_dp_c6_s1_prod_20260802.a
D:\sudoku_FJ_checkpoint_backups\layer_dp_c6_s1_prod_20260802\
  session-008-20260810-170245-layer_dp_c6_s1_prod_20260802.a.sha256.txt
```

The prior local A/B fallback remains intact:

```text
generation: 36 (.b)
cursor: 37/124 chunks
emissionsSoFar: 686604415424
cacheHitsSoFar: 32904033
claimed entries / holes / real states: 903398620 / 18 / 903398602
local bytes: 32522350448
local SHA-256: B11F94B0D9A471A1FFACD340AD94FFE29874D537AB3FD2EACB3E1C8233F3FDF9
```

Window 4 ran for 8.4374 hours, peaked at 61.856 GiB RSS, stopped at
`target_window_checkpoint`, verified the external copy, and left no engine or
controller process.  A separate read-only audit on 2026-08-15 recalculated
both generation-37 SHA-256 values above.  Detailed evidence is in
`docs/reports/og2/layer-dp-c6-production-s1-window4-20260810.md`.

The append-only progress sidecar begins with a recovered generation-37
baseline at
`data/logs/layer_dp_c6_s1_prod_20260802/progress.csv`.  Empty delta fields on
that baseline are intentional: generations 0--36 were not all retained, and
no earlier chunk may be rerun merely to reconstruct performance history.
