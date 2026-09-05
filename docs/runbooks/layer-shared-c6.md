# Bounded C=6 shared-F4 windows

This runbook concerns **only the four-row factorization stage**. It does not
run a complete C=6 count, close the five-row layer, or authorize an unbounded
job. The production F5 stage is not covered here. Consult `../../STATUS.md`
and `../../data/checkpoints/MANIFEST.md` before each owner-approved window.

## What is computed and what is preserved

The source is the original, partial generation-37 catalogue:

```text
data/checkpoints/layer_dp_c6_s1_prod_20260802.a
bytes  32522350448
SHA256 ECF0837315B0FDF8AE21C394FDA6676490E43B1828A0825529A344EC17E4E844
```

It is opened read-only. The loader checks the native header, payload and full
SHA-256, then discards every historical T value. A certified missing support
key is added in memory only. Existing native IDs and 18 insertion holes are
preserved. The repaired domain therefore has 903,398,621 IDs and 903,398,603
live states; progress measured in IDs includes the holes.

For each live ID, the engine independently checks its canonical key and exact
stabilizer, enumerates its complete graph-equivalence fiber, and records its
representative ID. Only the representative computes and stores a positive,
closed exact F4. Other members retain their distinct native IDs and aliases;
their zero value fields mean "use the representative", not "F4 equals zero".
An alias may point into a later, not-yet-processed chunk. This is safe partial
progress, but not yet a complete native weight vector.

The new namespace contains immutable `manifest.bin` and
`chunk-<16 hexadecimal digits>.bin` files. Headers bind the source SHA, source
header, repair witness, algorithm and chunk geometry. A whole chunk must
finish before its temporary is flushed and renamed without replacement.
Existing committed files are never overwritten. Checksums, ranges, holes and
record meanings are checked on resume; final alias closure is checked only
after every ID is processed.

No old production accumulator is continued or accepted by this route. No
rehearsal weights are used. Even a complete F4 catalogue is **not N(6)**.
The mathematical distinction between sharing F4 and retaining native
responses is explained in `../math/native-graph-value-sharing.md`.

## Verification prerequisites

Use the known-safe builds; do not add `-march=native`. The full repository,
direct-layer and shared-layer gates must have passed in the current source
tree before production use. The shared gate checks complete C4/C5 native
values, new export payloads, all 355 downstream C5 values and N(5), changed
thread counts, actual kill/resume, original-weight poisoning, holes,
checksum-valid invalid aliases, bad source stabilizers and keys, namespace
concurrency, and overwrite rejection.

An example fresh verification session, from the repository root, is:

```powershell
$gateDir = Join-Path (Get-Location).Path ("data/logs/shared-preflight-" + [guid]::NewGuid().ToString("N"))
New-Item -ItemType Directory -Path $gateDir | Out-Null

powershell -NoProfile -ExecutionPolicy Bypass -File scripts/verify_all.ps1 *> (Join-Path $gateDir "full.log")
if ($LASTEXITCODE -ne 0) { throw "Full repository gate failed" }

powershell -NoProfile -ExecutionPolicy Bypass -File scripts/verify_layer_direct.ps1 -Threads 4 *> (Join-Path $gateDir "direct.log")
if ($LASTEXITCODE -ne 0) { throw "Direct-layer gate failed" }

powershell -NoProfile -ExecutionPolicy Bypass -File scripts/verify_layer_shared.ps1 -Threads 4 *> (Join-Path $gateDir "shared.log")
if ($LASTEXITCODE -ne 0) { throw "Shared-layer gate failed" }
```

The shared verifier builds the shared and reverse-F5 programs; it requires
the reference layer engine and native-gather binary built by the preceding
gates. Its optional `-C6SampleText` input must be a bounded, independently
valued complete-fiber **text sample**, not a production checkpoint. That
additional test remains sample-only and does not certify the full C6 values.

The window controller requires explicit full/shared log paths with successful
markers less than 24 hours old. It rejects a shared executable rebuilt after
its supplied gate and listed source dependencies changed after compilation.
Do not reuse a success marker as evidence for a different executable or tree.
Retain all logs; a failed gate is not permission to bypass the check.

## Protected source and separate output

The original source needs an existing, separate-volume physical copy with the
same 32,522,350,448-byte length and pinned SHA. The manifest records this copy:

```text
D:/sudoku_FJ_checkpoint_backups/layer_dp_c6_s1_prod_20260802/
session-008-20260810-170245-layer_dp_c6_s1_prod_20260802.a
```

Pass its complete path as `-SourceBackup`. The controller hashes that copy
before launching; the engine independently validates the local original.
Neither path is an output target.

The default new namespace and backup root are:

```text
E:/Code/sudoku_FJ/data/checkpoints/c6_shared_f4_20260905
D:/sudoku_FJ_checkpoint_backups/c6_shared_f4_20260905
```

Use a dedicated namespace, never a repository root, home directory or original
input path. `-NamespacePath` may be repository-relative; `-BackupRoot` must be
absolute and on a separate volume. Check free disk space for both copies of
the accumulated namespace and the next window before resuming. Backup phases
copy all committed files, not only the newly added suffix. Keep binary chunks
ignored by Git; retain their provenance and backup receipts in the checkpoint
manifest rather than adding them to ordinary Git history.

## First bounded window

These are the initial pilot limits, not a throughput promise or permission for
an unbounded job. Use them within the owner's authorized C6 work and the
currently available computing window:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/run_layer_shared_window.ps1 `
  -FullGateEvidence (Join-Path $gateDir "full.log") `
  -SharedGateEvidence (Join-Path $gateDir "shared.log") `
  -SourceBackup "D:/sudoku_FJ_checkpoint_backups/layer_dp_c6_s1_prod_20260802/session-008-20260810-170245-layer_dp_c6_s1_prod_20260802.a" `
  -NamespacePath "data/checkpoints/c6_shared_f4_20260905" `
  -BackupRoot "D:/sudoku_FJ_checkpoint_backups/c6_shared_f4_20260905" `
  -Threads 24 -Limit 500000 -Chunk 25000 -WorkMinutes 8 -MaxMinutes 10 -LimitGiB 55
```

Here `-Limit` bounds additional IDs, not total catalogue size. A limit must
permit at least one complete chunk. The engine starts no new chunk after its
soft work deadline and never deliberately saves an unfinished chunk. The hard
time/RSS guards may kill an unfinished chunk; earlier committed files remain
recoverable. Loading and namespace validation consume part of the child's
time budget. The process is launched hidden.

The controller's child-process ceiling is 480 minutes: do not remove that
eight-hour interlock. **Source-backup hashing and before/after backup copying
are outside `-MaxMinutes`.** If the machine is available for eight hours total,
do not assign all 480 minutes to the computing child. Reserve time for gates,
loading, source hashing and two backup phases; for example a 450-minute hard
child ceiling with a shorter soft window leaves some headroom, but is not a
guarantee that the entire workflow ends within eight hours. Use measured backup
times and a smaller owner-approved work window when necessary.

## Resume, stop and handoff

Reuse the same source, repair convention, namespace and `-Chunk`. Thread count,
additional-ID limit and time window may change. An algorithm or chunk-geometry
mismatch is a hard refusal; never edit the manifest to force acceptance.
The program reconstructs the contiguous committed prefix, validates it, and
recomputes only the uncommitted suffix. Do not delete old chunks to recover
space or restart from an earlier cursor.

Before launching, the controller copies any existing committed namespace to a
fresh `*-before` directory. In its `finally` block it copies committed files to
a fresh `*-after` directory. For each copy it compares the source hash before
copying, destination hash, and source hash afterward; `receipt.csv` is written
only after that phase succeeds. A before-backup failure prevents launch. An
after-backup failure is an error, not a completed protected handoff.

Prefer the configured soft stop and allow the backup phase to finish. A power
failure, OS termination or force-kill of the **controller itself** cannot be
made to execute `finally`. In that case retain all files and logs. The next
approved resume must complete its before-backup before doing new computation.
Uncommitted `.tmp-*` files are not valid progress and are not promoted or
automatically removed. Preserve them until the incident has been audited.

For each window retain and review:

- The controller log and its `WINDOW_END` exit code; nonzero is not success.
- `stdout.log`, `stderr.log` and `rss.csv` under the printed `WINDOW logs=...`
  directory, including the loaded source SHA, per-chunk metrics and summary.
- The closed ID prefix, closed representative count, and explicitly incomplete
  or complete F4-catalogue status. `INCOMPLETE_RESUMABLE` may be a normal bounded
  exit; it is not a closed layer.
- Successful external backup directories and their complete SHA receipts.
- Confirmation that the child and controller have exited before the next job.

Distill measured results into a dated report and update the authoritative
checkpoint manifest and STATUS only after verification. This controller does
not request native snapshot export. Complete alias closure, a new non-overwriting
export, independent readback and a separately qualified F5 stage are required
before moving downstream. No command in this runbook closes the final outer
square sum or establishes N(6).

## Export a completely closed F4 namespace

The separate `scripts/export_layer_shared.ps1` controller performs this
handoff without changing or rebuilding the released producer. It is **not**
another computing window and must not be used until all 903,398,621 IDs are
committed and the complete namespace has a separate-volume physical backup.
Current partial prefixes are not eligible. There has been no production C6
export test or completed C6 native F4 export.

Do not invoke the producer with only `checkpointreadonly export=...` and
assume it is read-only with respect to chunks. A bounded C5 check showed that
this combination can compute and commit missing chunks. The new controller
instead requires complete, contiguous, source-matched chunk headers before
launch. It retains Windows deny-write/delete handles on the original source,
its physical copy, and every local and backed-up chunk throughout export and
verification. Directory ancestors are pinned root-to-leaf and opened reparse
points are rejected. A disappearing or modified input therefore cannot turn
this operation into a computing resume. Any malformed chunk is rejected by
the released producer before its computation loop.

The producer must report full alias closure, `new_chunks=0`, and
`new_indices=0`. It writes only a new native snapshot path. The controller
then uses the independently implemented streaming `layer_support_probe` to
verify the complete SHA-256, header hash, payload hash, entry/hole/live counts,
and stored-stabilizer orbit mass. It verifies a plain production L4 header and
creates a new physical output copy on a separate volume, checking both hashes
before writing an exclusive JSON receipt beside the local export.

The streaming reader verifies serialization and the recorded support mass;
it does **not** independently recompute every C6 F4 value or compare every
exported weight with the alias chunks. Numerical provenance comes from the
closed chunk computation and full producer alias closure, qualified by the
complete C5 gate. That gate compares every one of 17,120 native keys,
stabilizers and weighted T values against a fresh independent row-incremental
oracle. It also exercises incomplete suffixes, internal gaps, overwrite
refusal, existing-writer conflict, and denial of subsequent writes/deletion.

Once closure is genuinely available, the command shape is:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/export_layer_shared.ps1 `
  -FullGateEvidence "PATH_TO_RECENT_FULL_GATE.log" `
  -SharedGateEvidence "PATH_TO_RECENT_SHARED_GATE.log" `
  -DirectGateEvidence "PATH_TO_RECENT_DIRECT_GATE.log" `
  -SourceBackup "D:/sudoku_FJ_checkpoint_backups/layer_dp_c6_s1_prod_20260802/session-008-20260810-170245-layer_dp_c6_s1_prod_20260802.a" `
  -NamespacePath "data/checkpoints/c6_shared_f4_20260905" `
  -NamespaceBackup "D:/PATH_TO_COMPLETE_VERIFIED_CHUNK_COPY" `
  -ManifestSha256 "EXACT_SHA256_OF_COMMITTED_MANIFEST" `
  -OutputPath "E:/PATH_TO_NEW_CLOSED_L4.snap" `
  -OutputBackup "D:/PATH_TO_NEW_CLOSED_L4.snap" `
  -Chunk 25000 -Threads 24 -MaxMinutes 30 -AuditMaxSeconds 300 -LimitGiB 55
```

The placeholders are intentional: no future output SHA or complete-namespace
backup is invented. Both output parent directories must already exist, and
the local output, external output, and JSON receipt must not exist. The
controller verifies both original-source hashes and every committed input
file against its physical copy. It checks output/backup disk headroom. Keep
at least one full new 32.53-GB native file's space on each volume, in addition
to the original input and immutable chunks.

`-MaxMinutes` is the producer-child bound; `-AuditMaxSeconds` is the streaming
readback bound. Pre/post source and chunk hashing, backup copying and receipt
work are additional overhead. These example limits are interlocks, not a
measured full-C6 export-duration guarantee; reserve them within the owner's
daily availability. Roughly 36,136 chunks require about 72,272 retained file
handles for both copies; one-byte stream buffers avoid a 4-KiB buffer per
file. Full-scale handle/runtime behavior has not yet been exercised.

A failed operation produces no accepted handoff receipt. Native or private
temporary files may remain and must be retained for audit, not overwritten
or promoted. A successfully verified export is still only closed F4, not
closed F5 or N(6). Record the new output SHA and external copy in the manifest
before using the independently qualified reverse-F5 controller described in
`layer-reverse-c6.md`.
