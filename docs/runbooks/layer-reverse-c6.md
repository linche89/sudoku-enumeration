# Bounded C=6 reverse-F5 windows

This is the new **S2 computation of closed five-row values**, not a final
Sudoku count. It consumes a newly verified closed native L4 snapshot and
reuses the old rehearsal L5 file **only as a support catalogue**. No C6 F5
values have yet been computed by this engine. The complete C5 end-to-end and
recovery release gate and a read-only C6 support-loader smoke have passed.
The exact evidence and the distinction from a C6 computation are below.

Read `../../STATUS.md`, `../../data/checkpoints/MANIFEST.md` and `og2.md`
before a computing window. The user has authorized completing the exact C6
goal; this runbook does not extend the agreed computing availability or
bypass the safety gates. The first real F5 run is a 10,000-ID pilot with an
eight-minute soft and ten-minute hard child limit. Review its exact results,
memory and backup evidence before scaling within the agreed daily window.

## Input integrity is not a proof that weights are closed

Two separate trust boundaries matter:

1. A full SHA-256, native header hash and native payload hash identify and
   protect the exact input bytes. They do **not** prove that its stored
   numbers are completed factorization values.
2. The L4 input must be an independently verified export from a completely
   closed F4 run: all native aliases have resolved to closed representatives,
   the export is a new production-config snapshot, and its readback and
   physical backup have been verified. Its SHA is a required command-line
   argument; there is no default or guessed future hash.

The original generation-37 L4 file is **not** an acceptable closed-value
input. Do not rename it, change its header, or pass its SHA as a future L4
export hash. Its historical T values remain incomplete.

`layer_reverse_f5` requires a plain production L4 snapshot (`parentLayer=0`,
layer 4, narrow payload), all 903,398,603 live native states, exact raw orbit
mass 41,602,261,536,160, unique stored keys and valid stabilizer divisors. It
validates the complete file SHA against `-Layer4Sha256`. For every live ID it
then computes

```text
m4 = 46080 / stabilizer
F4 = T4 / m4
```

The division must be exact, F4 must be positive and divisible by 24, and hole
weights must be zero. These are additional rejection checks, not a
replacement for the closed-export provenance. The in-memory predecessor
buffer contains unweighted F4 at every native ID, not the old weighted T4 or
a partly filled representative cache.

The second input is pinned independently:

```text
local:
data/logs/layer-dp-c6-e2e-rehearsal-20260802-1pct/s2-4to5/ck.L5.snap

required separate-volume physical copy:
D:/sudoku_FJ_checkpoint_backups/c6_layer5_support_20260905/ck.L5.rehearsal.snap

bytes:   3472307192
SHA256:  A5FDDB22F8C79FD4DDC0015795558FBF1647F0DB556248F5C9551F2DB24891DF
entries: 96452974
holes:   221
live:    96452753
```

The loader reads this file under its actual rehearsal-denominator-100
configuration, validates its header, payload and full SHA, and erases **all**
historical T5 values. It restores production configuration before any new
arithmetic or export. It builds a temporary index to reject duplicates,
adds the two proven missing native states in memory only, and releases that
L5 index. Nothing is patched in the original file or its backup.

The repaired stable-ID domain is:

```text
entries including holes: 96452976
live native states:      96452755
raw orbit mass:          4439972139072
new IDs:                 96452974, 96452975
new stabilizers:         1440, 240
repair fingerprint:      11401178190082244558
```

The witness geometry and membership evidence are retained in
`../math/two-missing-layer-burnside.md` and
`../../data/golden/og2-c6-support-witnesses.json`.

## Exact computation and durable progress

For each target L5 state, the engine first independently recomputes its native
canonical key and stabilizer and requires equality with the catalogue. It
uses the already gated rooted matching enumeration and existing native L4
canonicalizer:

```text
F5(target) = 5 * sum(rooted labelled matching multiplicity * F4(residual))
```

There is no additional orbit, pairing or symbol-factorial weight inside this
sum. Missing or zero predecessors are fatal. Accumulation is in u128 with a
checked u64 result. Each live closed F5 must be positive and divisible by 120;
permuting its five ordered colors acts freely. The analogous four-color
condition gives the F4 divisibility check above. Every hole has value zero.

The new namespace stores `manifest.bin` and
`f5-chunk-<16 hexadecimal digits>.bin`. This is a distinct format from the
shared-F4 alias chunks: every F5 payload record is just one closed u64 F5,
with zero reserved for holes. A 256-byte header binds both input SHA-256
values, both original native header hashes, the support-repair fingerprint,
value semantics, stable-ID count and chunk geometry. Payload/header hashes,
exact file lengths, value positivity, factorial divisibility and diagnostic
totals are checked on resume.

A whole target chunk must complete before its exclusive temporary is
flushed and renamed without replacement. Committed files are immutable.
Resume accepts only a contiguous committed prefix and does not recompute it.
A missing interior chunk, invalid payload, changed input or incompatible
geometry is an error; do not edit manifests to force acceptance. Uncommitted
temporaries are retained for audit, not automatically promoted or deleted.

`-Limit` counts **additional IDs**, including holes; it is not a total live
state count. The soft deadline stops new chunks, while the hard time/RSS
guard may interrupt one unfinished chunk. Earlier committed chunks remain
usable. An `INCOMPLETE_RESUMABLE` exit can be a normal bounded result, but is
not a closed L5 and cannot be exported as one.

## Build, exact gates and optional loader smoke

The reverse build does not rebuild or alter the separately released shared-F4
executable. Use the repository-safe flags; never add `-march=native`.

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/build_layer_reverse.ps1
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/verify_layer_reverse.ps1 -Threads 4
```

The complete repository, direct and shared gates must already have qualified
their current dependencies. Before a computing window, retain a successful
complete repository log, the complete reverse verifier stdout log, and its
printed `driver.log` path. `-ReverseGateEvidence` must point to the wrapper
stdout log containing `PRODUCTION REVERSE F5 CHECKS PASSED`. The separate
`driver.log` is retained detailed evidence, but its different marker is not
accepted by the controller.
The controller requires success markers less than 24 hours old, rejects a
reverse executable rebuilt after its gate, and checks that listed source
dependencies have not changed after compilation. A success marker from a
different binary or source tree is not evidence for the current one.

The reverse release gate constructs a fresh shared-F4 export, consumes its
actual weighted T4 via exact orbit division, checks every one of the 355 C5
F5 values against the independent reference, and exercises resumability and
rejection paths. Retain its complete machine-readable summary and logs;
consult the dated report for the exact executed test inventory and hashes.

The 2026-09-05 release passed with these retained artifacts:

```text
directory:
data/logs/layer-reverse-release-gate-152f39785b8f4499a368afa5be923244
execution log: driver.log
machine summary: fixtures/checks.json
controller-accepted wrapper stdout log:
data/logs/reverse-final-release-run-6dc7d6dcbfab432fbccafa3272457656/stdout.log
release executable SHA256:
C7BDBC4DEA0FE8497787D7DB127A9B718D591BCA5F59B9394C64F6E11F295761
fresh shared C5 L4 snapshot SHA256:
E60114E228A328CE6734B1F68E0136017193D7DD20181F3AD97BE3186127D04A
new C5 L5 export, byte-identical to the independently generated reference:
E6FBEFB1541E8B9B859144164283BFC0668488BDE3A03BC7ECBD9FE90DEDAD5A
independent final N(5): 1903816047972624930994913280000
```

The test driver took 25.680 seconds, including fresh C5 reference generation.
It checked all 17,120 new weighted L4 values and all 355 computed F5 values,
native final-stage load compatibility, actual kill/resume with a preserved
closed prefix, both input lineages, corruption and semantic rejection,
factorial guards, holes, old-target-weight erasure and overwrite refusal.
All input files remained unchanged. Its summary explicitly records
`production_c6_checkpoint_io: false`. These historical hashes identify this
release, not a substitute for a fresh required gate after code changes.

This optional test reads only the actual C6 L5 support and never needs a fake
closed L4 input:

```powershell
build/layer_reverse_f5_support_test.exe `
  data/logs/layer-dp-c6-e2e-rehearsal-20260802-1pct/s2-4to5/ck.L5.snap `
  checkpointreadonly
```

It has fixed 120-second/6-GiB limits and requires 8 GiB available RAM. The
executed smoke took 4.21 seconds and peaked at 4,553,400,320 bytes; it checked
the original full SHA, duplicate-free live keys, erasure of every old T5,
both exact appended IDs, repaired count/mass and restoration of production
configuration. This is a support certificate, **not** a calculation of F5.

## First bounded C6 F5 pilot

There is deliberately no ready-to-run closed-L4 filename or SHA below. Fill
them only from the verified complete export and its manifest/backup receipt.
Both L4 and L5 support require separate-volume copies before launch.

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/run_layer_reverse_window.ps1 `
  -FullGateEvidence "<CURRENT_FULL_GATE_LOG>" `
  -ReverseGateEvidence "<CURRENT_REVERSE_GATE_WRAPPER_STDOUT_LOG>" `
  -Layer4 "<NEW_VERIFIED_CLOSED_L4_SNAPSHOT>" `
  -Layer4Backup "<MATCHING_SEPARATE_VOLUME_L4_COPY>" `
  -Layer4Sha256 "<64_HEX_CLOSED_L4_SHA256>" `
  -SupportBackup "D:/sudoku_FJ_checkpoint_backups/c6_layer5_support_20260905/ck.L5.rehearsal.snap" `
  -NamespacePath "data/checkpoints/c6_reverse_f5_20260905" `
  -BackupRoot "D:/sudoku_FJ_checkpoint_backups/c6_reverse_f5_20260905" `
  -Threads 24 -Limit 10000 -Chunk 10000 -WorkMinutes 8 -MaxMinutes 10 -LimitGiB 55
```

Do not pass `-ExportComplete` for this pilot. The loader checks currently
available RAM and charges both catalogues plus temporary index construction;
the active L4 index alone is large. Do not overlap the pilot with another
large counting process. In the earlier **lookup-only** test, 512 uniformly
sampled L5 records produced 2,471,101 successful queries in 0.432451 seconds
at 24 threads. It did not read F4 values or compute F5 and is not an
end-to-end production timing or a guarantee for a longer run.

The controller hashes both source/copy pairs before launching, backs up any
committed prefix to a new `*-before` directory, and copies committed chunks
to a new `*-after` directory on completion or child failure. Each backup
phase checks source-before, destination and source-after SHA and writes a
receipt. It launches the child hidden. A failed backup is not a protected
handoff. Killing the controller itself or losing power cannot be guaranteed
to run its `finally` backup; the next approved resume must complete the
before-backup first.

Input hashing and before/after physical copying are **outside** `-MaxMinutes`.
Loading, payload/SHA checks and resume validation are inside the child time
budget. An eight-hour machine-availability window therefore must leave room
for gates, hashing and backups; do not assign all eight hours to computation.
The controller's wider parameter ranges do not replace review of the pilot
or the agreed daily limit. Every later window still needs a positive limit.

## Complete export and unchanged S3 handoff

Only when every live target has a closed F5 may the engine export to a **new**
path. It computes `T5 = (46080/stabilizer) * F5` with checked u64 conversion,
requires production configuration, and writes a plain narrow native layer-5
snapshot. The large L4 catalogue is released before export readback. The
native reader reopens the private export and checks every key, stabilizer and
weight before a nonreplacing final rename. The controller's
`-ExportComplete` option then requires the explicit closed-catalogue and
native-readback success records, rechecks the reported export SHA, and
creates a new external `<window-stamp>-closed.L5.snap` copy. An existing
output file alone is not a completion certificate. Neither this export nor
its roundtrip is N(6).

Before S3, record the complete L5 count/mass, source and output hashes, both
input provenances, source commit, successful reverse gate, export readback and
separate-volume L5 receipt in the manifest and a dated report. Require a clean
committed source tree for the existing finalization controller. Do not hide
uncommitted changes or bypass its safeguards.

The existing S3 accepts explicit input-path overrides, so no S3 algorithm or
checkpoint-format change is needed:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/run_layer_dp_c6_s3_finalize.ps1 `
  -AuthorizeFullC6 `
  -Layer5 "<NEW_VERIFIED_CLOSED_L5_SNAPSHOT>" `
  -Layer5Backup "<MATCHING_SEPARATE_VOLUME_L5_COPY>" `
  -Layer5Sha256 "<64_HEX_CLOSED_L5_SHA256>" `
  -PrepareOnly
```

This preparation command does not execute finalization. The user's complete
C6 objective covers the outcome, but the actual S3 execution still needs its
explicit controller flag, the required clean-source and input gates, and an
adequate computing window. It must perform its independent replay, all 63,199
class checks, G1/G2 bridges, semantic certificate verification, both exact
square sums and physical backups. See `layer-dp-c6-production.md` for those
final acceptance requirements. Do not claim N(6) from a support catalogue,
closed F4 layer, closed F5 layer, or bounded timing run.
