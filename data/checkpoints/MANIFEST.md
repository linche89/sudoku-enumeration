# Checkpoint Manifest

## c6_shared_f4_20260905 (closed-value chunks; new route)

This namespace is NOT a closed native L4 image and is NOT a result for N(6).
The original generation-37 A/B files remain unchanged. Only their keys and
stabilizers are used; every original partial T is discarded. The one proved
missing L4 key is appended only to the engine's in-memory catalogue.

Current state after the ninth bounded window and full independent audit,
2026-09-06. The shared F4 catalogue is CLOSED; a native L4 export is still absent.

```text
directory: data/checkpoints/c6_shared_f4_20260905/
format: SFR4MT01 manifest / SFR4CK01 immutable chunks, version 1
semantics: SF4MIN01 (exact minimum-fiber aliases plus closed F4 at representatives)
complete support IDs including holes: 903398621
complete live support: 903398603
closed record prefix: [0,903398621)
live records in prefix: 903398603
holes in prefix: 18
chunk size: 25000
committed chunks: 36136
closed representative F4 values: 140069579
sum of representative F4 values (modulo 2^64): 194468287162752
live aliases resolving to closed representatives: 903398603
aliases to future/uncomputed representatives: 0
files including manifest: 36137
total bytes: 10850034524
manifest SHA-256: 848B9DC72452AB389C5BF5424E264AE86C673C7FBA673F5F14FDC845BF4DEB91
original input SHA-256: ECF0837315B0FDF8AE21C394FDA6676490E43B1828A0825529A344EC17E4E844
```

A nonrepresentative record contains a proved alias, not its own numerical
value. Every live alias now resolves to a closed self-representative, checked
both by the producer and by a separate full-domain bitset reader. The native
weighted T4 image has not been exported. Uncommitted temporaries are never
accepted, and no original partial T4 accumulator is used.

External physical backup, independently hashed source/copy on every file:

```text
D:\sudoku_FJ_checkpoint_backups\c6_shared_f4_20260905\
  20260906-085138-8974f5a375294f26852c7ac7392947d9-after\
receipt.csv SHA-256:
C0FBBDF098186FCA77E6C47E2CA64A2FD6DA60151FB51E08CB3B1ECC1EC6284E
```

The receipt lists the individual SHA-256 of all 36,137 committed files.
Window9 used `limit=106073621`, `chunk=25000`, 24 threads, a 55 GiB guard,
75-minute soft stop and 90-minute hard child bound. It resumed 31,893 chunks,
added 4,243 chunks (including the final 23,621-record chunk), and exited zero
after 2,258.025330 engine seconds. New computation/commit took 2,196.059251
seconds. Peak working set was 44,734,844,928 bytes. Engine completion was at
09:38 and after-backup at 09:47 local time on 2026-09-06. No computing process
remained. Backup work lies outside the child time budget.

Window8 had retained prefix 797,325,000 after overnight sleep, but its
controller exit 98 is NOT a normal successful termination. The complete
audit redecoded every record; it does not inherit unaudited window8 counters.

This window used the newly qualified incremental bridge, installed executable
SHA-256 `2B8A14606F8F78748C9338130D35551CA199E53238BEB33D95BCB05A2555AFE0`.
Native keys, alias/value rules and chunk formats are unchanged. Both old-binary
rollback copies and all previous checkpoint copies remain retained.

The F4 scan is finished; do not launch another computing window. The next
handoff is the separately protected native export in
`docs/runbooks/layer-shared-c6.md`, not F5 on the original partial image.
The `.gitignore` explicitly excludes this binary namespace.
Evidence: `docs/reports/og2/c6-shared-f4-window1-20260905.md`,
`docs/reports/og2/c6-shared-f4-window2-20260905.md`,
`docs/reports/og2/c6-shared-f4-window3-20260905.md`,
`docs/reports/og2/c6-shared-f4-window4-20260905.md`,
`docs/reports/og2/c6-shared-f4-window5-20260905.md`,
`docs/reports/og2/c6-shared-f4-window6-20260905.md` and
`docs/reports/og2/c6-shared-f4-window7-20260905.md` and
`docs/reports/og2/c6-shared-f4-complete-audit-20260906.md`. Earlier external copies
are retained; no original source or earlier committed chunk was overwritten.

The independent two-pass native bitset audit reproduced every record,
header/payload hash, and both final-window physical copies; all 18,104 files
from the SHA-pinned window7 audit are unchanged. All 903,398,603 live IDs
resolve to closed representatives; unresolved aliases are zero. This is file
integrity and complete internal alias-closure evidence, not independent
numerical reevaluation of all F4. No full source catalogue was opened by it.
Elapsed time was 166.588572 seconds, with a 180-second guard and separate
1-GiB worker/parent bounds. Production report:
`data/logs/c6-direct-route-20260905/shared-complete-independent-audit-20260906.json`,
SHA-256 `8FAA333C7F4CC38D6733AA94A3DCFE31D2E469D602B3A212065DE70EAF001741`.

## C6 rehearsal L5 support (read-only input to the new route)

This is NOT a closed production L5. Only its catalogue keys/stabilizers may
be reused, after erasing every old rehearsal T5 value. The two proved absent
keys are added in RAM; neither source file nor physical copy is patched.

```text
source: data/logs/layer-dp-c6-e2e-rehearsal-20260802-1pct/s2-4to5/ck.L5.snap
physical backup: D:\sudoku_FJ_checkpoint_backups\c6_layer5_support_20260905\ck.L5.rehearsal.snap
bytes in each file: 3472307192
SHA-256 of each file: A5FDDB22F8C79FD4DDC0015795558FBF1647F0DB556248F5C9551F2DB24891DF
source configuration: rehearsal denominator 100, generation 4
source entries / holes / live: 96452974 / 221 / 96452753
RAM-repaired entries / holes / live: 96452976 / 221 / 96452755
new IDs: 96452974, 96452975
new stabilizers: 1440, 240
repaired raw orbit mass: 4439972139072
repair fingerprint: 11401178190082244558
```

The new separate-volume copy was physically created and both full files were
SHA-verified on 2026-09-05. The actual reverse-engine support loader checked
the full image and SHA, zero duplicate original keys, old-weight erasure and
production-configuration restoration in a bounded read-only smoke. See
`docs/reports/og2/c6-reverse-f5-release-20260905.md`.

The reserved `data/checkpoints/c6_reverse_f5_20260905/` namespace is ignored
by Git but has no C6 production chunks yet. It must consume a NEW verified
closed F4 export, never the partial generation-37 T4 values.

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
format magic: SDFJLCK1 (0x314B434C4A464453)
format version: 2
key/value algorithm tag: LDPCAN01 (0x4C445043414E3031)
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
