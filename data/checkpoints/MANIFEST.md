# Checkpoint Manifest

## Complete C6 final certificate (2026-09-07)

The full count is closed and independently certificate-verified:

```text
N(6): 38296278920738107863746324732012492486187417600000
live final classes: 63199
labelled mass: 622345892187672576
CSV bytes: 4766612
CSV SHA256: 84F2720E7BA8296D78604153B934B7291AF93C329C07F5D46538F9D7101127C7
semantic SHA256: 292F8121743016528C0E523C4C7A14F4B4471DCA8BD93A279006DACDFF8FDB91
primary CSV: data/logs/layer_dp_c6_s3_prod_20260907/final-primary.csv
replay CSV: data/logs/layer_dp_c6_s3_prod_20260907/final-replay.csv
external CSV: D:/sudoku_FJ_checkpoint_backups/layer_dp_c6_s3_prod_20260907/layer_dp_c6_final.csv
source L5 SHA256: 4CF50FAD4F7C5DB020D30DEF258CE06AEF63125352E351D331A07BE8E173A6CF
production source commit: 4dd86b590a2531993af0d00dbb93cf9a13b4de6c
```

Both fresh final contractions produced byte-identical CSVs. Independent
semantic verification, Python/.NET exact sums, first-moment and Latin6
checks passed. All native workers and the production controller exited;
no further F5 or S3 computation is required. This is a certificate-verified
exact computation, not an independent reevaluation of every F5 value.

The separate external artifact directory is
`D:/sudoku_FJ_checkpoint_backups/layer_dp_c6_s3_prod_20260907/`.

| Local artifact | External name | Bytes | SHA-256 |
|---|---|---:|---|
| `layer_dp_c6_s3_prod_20260907_primary.L6.snap` | `primary.L6.snap` | 3286528 | `9A480B9723D36D10CBE719D7C9F8B33CA4D450AE7F095298B443D6525CEB182C` |
| `layer_dp_c6_s3_prod_20260907_replay.L6.snap` | `replay.L6.snap` | 3286632 | `66C537A68BFE41797B544580B7358274C7B75BFD7BAB5B4037DDD05656726E6C` |

The primary has 63,200 stable slots/one insertion hole; the replay has
63,202 slots/three holes. This explains their different snapshot hashes;
the complete deterministic per-class data is identical. Each namespace's
`.a` and `.L6.snap` are hard links to its closed wide generation. The two
namespace `.L5.snap` files are hard links to the immutable
`c6_reverse_f5_closed_20260907.L5.snap`, not additional physical backups.
These identities were checked with `fsutil hardlink list` on 2026-09-07.

Both namespaces also have immutable `.s3-binding.json` and `.s3-result.json`
receipts, with byte-identical external copies. Preserve them with the binary
state: they bind the actual closed L5, command, final CSV and snapshot.
The actual production executable is retained externally as
`layer_dp_gate.production.exe`, 337,628 bytes, SHA-256
`45C85C6A818893F435F0130755CA7C8FA7CDBA46BC3223F89A26F5EC4CC503B6`.

Small tracked master receipt:
`data/golden/og2-c6-final-certificate-receipt.json`, also physically copied
under the external directory with the same name. Both copies have SHA-256
`5BB7CD7F505662DFC87C19E901EE362BC3327D747FF2F0090A9921817656C10F`.
It records all individual input/output/lineage/evidence hashes, exact sums,
actual controller exit/time/memory and the final complete repository gate.
The narrowly scoped `.gitattributes` rules preserve these hash-bound bytes.

Detailed commands, checks, limitations and timings:
`docs/reports/og2/c6-finalization-20260907.md`. All earlier checkpoints and
backups below remain protected; nothing was deleted to complete this result.

## c6_shared_f4_20260905 (closed-value chunks; new route)

This namespace is NOT a closed native L4 image and is NOT a result for N(6).
The original generation-37 A/B files remain unchanged. Only their keys and
stabilizers are used; every original partial T is discarded. The one proved
missing L4 key is appended only to the engine's in-memory catalogue.

Current state after the ninth bounded window and full independent audit,
2026-09-06. The shared F4 catalogue is CLOSED. The owner subsequently created
the protected native L4 export recorded in the separate section below.

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
weighted T4 image was subsequently exported to a separate, new file below.
Uncommitted temporaries are never
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

The F4 scan is finished; do not launch another computing window. Its separately
protected native export is recorded below. F5 must never use the original
partial image as a completed weight vector.
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

## c6_shared_f4_closed_20260906.L4.snap (closed native F4 export)

The owner's first manual F5 session completed the protected export, independent
full SHA/header/payload/count/mass readback and separate-volume physical copy
at 12:16 on 2026-09-06. No F4 numerical values were recalculated during export.

```text
local: data/checkpoints/c6_shared_f4_closed_20260906.L4.snap
backup: D:\sudoku_FJ_checkpoint_backups\c6_shared_f4_exports\c6_shared_f4_closed_20260906.L4.snap
bytes in each file: 32522350484
SHA-256 of each file: 7BA5E5BA3255DD17851043521F67FB4EE70F76AE565FD6CA9AD962E7D5014D94
entries / live / holes: 903398621 / 903398603 / 18
orbit mass: 41602261536160
new shared chunks / new indices during export: 0 / 0
receipt: data/checkpoints/c6_shared_f4_closed_20260906.L4.snap.receipt.json
receipt SHA-256: 6A5236B3E8447FE5F66C4A36A84BB8F557DDDDB435A2C55F897024B8E58A2B9D
export logs: data/logs/shared-f4-export-20260906-120609-f2d507515a6244b182bda1b9c07c59d1/
```

The receipt binds the fully closed shared-F4 namespace, original input and
its physical copy, producer/independent-reader binaries and fresh gates.
The reverse controller rehashes both native export copies before each run.
The original generation-37 partial image remains untouched.

## c6_reverse_f5_20260905 (complete closed-value F5 chunks)

Current complete state, independently audited 2026-09-07:

```text
prefix / complete domain: 96452976 / 96452976
live / holes: 96452755 / 221
chunk size / chunks / files: 10000 / 9646 / 9647
bytes per physical copy: 774093440
checksum modulo 2^64: 3850733904537200640
manifest SHA256: 22E81759527CD4CA2309138F0EB9930094617598FD76492882C069D48D117E30
backup: D:/sudoku_FJ_checkpoint_backups/c6_reverse_f5_20260905/20260907-134453-dfdbbc71d43147de82f6de62f1e5a01b-after/
backup receipt SHA256: 3BEB4A48D98F275F5F17300B2A10743039BEB6F13EA23AFC31F0F9FB6809CBB6
independent audit: data/logs/c6-finalization-20260907/f5-audit.json
audit SHA256: B781B8EB2ADE49FC74A872443F604F16716F2341DA4B5252CDF3D06F18976D91
```

All headers, payload hashes, input/repair bindings, hole positions, factorial
divisibility, counters and both copies were independently checked. This is
not independent numerical reevaluation of all F5. No F5 suffix remains;
do not restart the computing scan. Historical partial windows follow.

2026-09-07 reviewed extension: prefix [0,93120000), 93,119,793 live values,
207 holes, 9,312 chunks plus manifest, 747,344,128 bytes per physical copy.
Producer checksum modulo 2^64: 3717224009369902080. All 9,313 local and
external files matched their receipt sizes/SHA-256 in a separate read-only
4.445432-second audit. External directory:
`D:/sudoku_FJ_checkpoint_backups/c6_reverse_f5_20260905/20260907-074304-4048483a50df45b6874f83f26800761b-after/`.
Receipt SHA-256: `34D1BB91AC042E7BB949942883F1851D6B3D2FBDF5A18F1A0E2AAF288000D7BD`.
The native summary is INCOMPLETE_RESUMABLE; a telemetry exception means the
controller did not end normally despite completing its after-backup. This
is storage evidence, not independent numerical reevaluation. The owner has
authorized a same-namespace suffix continuation; later chunks must not be
trimmed to this reviewed prefix. See
`docs/reports/og2/c6-f5-telemetry-recovery-20260907.md`.

Last reviewed completed manual session, 2026-09-06 at 17:50. This is a
resumable numerical F5 prefix, NOT a complete F5 layer or N(6).

```text
directory: data/checkpoints/c6_reverse_f5_20260905/
closed stable-ID prefix: [0,30200000)
complete stable-ID domain: 96452976
live closed values / holes in reviewed prefix: 30199969 / 31
complete live domain / holes: 96452755 / 221
progress by stable IDs: 31.310594294156356%
chunk size: 10000
committed chunks / files including manifest: 3020 / 3021
bytes per physical copy: 242373376
F5_checksum_mod2_64: 1205041302659950080
controller/engine exit: 0 / 0
engine status: INCOMPLETE_RESUMABLE
engine stderr: empty
backup: D:\sudoku_FJ_checkpoint_backups\c6_reverse_f5_20260905\20260906-121946-99b4c5514bd943f1902d18e740ce48f1-after\
receipt.csv SHA-256: 0363AD06245CECAA5D973B93C43A8DEA26BFEFD1C4E909B0C45B4013F4D52D2E
session logs: data/logs/f5-manual-a0d9e35d7f2d44bbb309864537ae243f/
engine logs: data/logs/reverse-f5-window-20260906-121946-99b4c5514bd943f1902d18e740ce48f1/
```

A separate read-only check compared every local and backup file's size and
SHA-256 with the receipt, and checked contiguous 10,000-ID chunk filenames.
All matched. It did not independently decode/recalculate all F5 payload
values; resume performs the engine's existing full chunk validation.

The owner started a later ordinary session at 18:28 on 2026-09-06. The live
namespace may therefore contain more chunks than this reviewed session.
Do not overwrite/trim it to the recorded prefix or treat an in-progress
suffix as a reviewed completed-window result. The next command may use
`run_f5.ps1 -WorkMinutes 450 -MaxMinutes 480` only after that session stops.
The old namespace, all committed chunks and all physical copies are retained.

## c6_reverse_f5_closed_20260907.L5.snap (closed native production L5)

```text
local: data/checkpoints/c6_reverse_f5_closed_20260907.L5.snap
backup: D:/sudoku_FJ_checkpoint_backups/c6_reverse_f5_20260905/20260907-142458-60c5a69f99fc4f98af862fa474975b7c-closed.L5.snap
bytes per copy: 3472307264
SHA256 per copy: 4CF50FAD4F7C5DB020D30DEF258CE06AEF63125352E351D331A07BE8E173A6CF
entries / live / holes: 96452976 / 96452755 / 221
raw orbit mass: 4439972139072
semantics: plain narrow native L5, production LDPCAN01, T5 = orbit size * closed F5
new F5 chunks / IDs during export: 0 / 0
independent weighted audit: data/logs/c6-finalization-20260907/l5-export-audit.json
weighted audit SHA256: 3BAC093B49EA5C2E9636E979CAFBAAFA8919D62EA3272693AE343DB26B294260
closed-input receipt: data/golden/og2-c6-closed-l5-export-receipt.json
```

The native producer's full roundtrip, independent native header/payload/SHA
reader, both full physical-copy SHAs and independent all-record weighted
readback passed. This is an accepted closed input for the owner-authorized
final stage, not N(6). Inputs and original F4/F5 chunk namespaces remain
unchanged. Detailed receipts/commands are in
`docs/reports/og2/c6-finalization-20260907.md`.

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

The `data/checkpoints/c6_reverse_f5_20260905/` namespace is ignored by Git;
its owner-run numerical production prefix is recorded above. It consumes
the new verified closed F4 export, never partial generation-37 T4 values.

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
