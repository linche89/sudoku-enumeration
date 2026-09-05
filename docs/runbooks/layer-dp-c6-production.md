# C=6 layer-DP production chain

This runbook freezes the staged S1/S2/S3 workflow.  It supplements
`og2.md`; the safety rules in `AGENTS.md`, `STATUS.md`, and
`data/checkpoints/MANIFEST.md` remain authoritative.

No command in this file authorizes a new production window.  Every writable
invocation still needs an explicit repository-owner decision, a clean tracked
worktree, the complete repository gate, time/RSS bounds, and an external
physical backup.

## Current boundary

S1 is partial at generation 37 / cursor 38 of 124.  There is no closed L4
snapshot, so S2 and S3 cannot currently pass their mandatory input checks.
The active and external generation-37 SHA-256 is:

```text
ECF0837315B0FDF8AE21C394FDA6676490E43B1828A0825529A344EC17E4E844
```

The previous local A/B fallback is generation 36 in `.b`.  Do not rename,
move, overwrite, or delete either generation.

## Append-only progress evidence

`scripts/layer_dp_progress.py` reads a version-2 checkpoint without writing
it.  It validates the 128-byte header checksum and header-derived file length,
computes SHA-256, and appends one fsync'd row to `progress.csv`.  For a new
checkpoint it also reads only the corresponding parent snapshot's `stab`
slice, so the exact number of processed non-hole parents and the mean
emissions per real parent are recorded.

Rows include generation, transition, cursor/chunk geometry, claimed entries,
holes, real states, cumulative and delta emissions/cache hits, state deltas,
elapsed time, checkpoint write time, RSS, available RAM, file length,
SHA-256, and the checkpoint's configuration/parent/payload/header hashes.
The file is append-only and duplicate appends are idempotent; a conflicting or
non-consecutive checkpoint fails closed.

Historical generations 0 through 36 were not retained individually.  The
sidecar therefore starts with `recovered_baseline` at generation 37.  Empty
delta fields on that row explicitly mean “not reconstructable”, not zero.
Do not rerun the first 38 chunks to manufacture a history.

## S1: finish L3 to L4

Before an authorized continuation, use the non-computing preparation gate:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass `
  -File scripts\run_layer_dp_c6_s1_window.ps1 `
  -AuthorizeFullC6 -PrepareOnly -ContinueExisting
```

It must recognize the externally covered generation 37 and end with
`PRODUCTION S1 PREPARATION PASSED`.  The authorized eight-hour form is:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass `
  -File scripts\run_layer_dp_c6_s1_window.ps1 `
  -AuthorizeFullC6 -ContinueExisting `
  -WindowHours 8 -HardMaxHours 10 `
  -CheckpointMinutes 40 -RssLimitGB 85
```

The controller now records a resume baseline and every subsequent durable
checkpoint in:

```text
data/logs/layer_dp_c6_s1_prod_20260802/progress.csv
```

If the sidecar cannot verify or append a checkpoint, the controller stops the
engine, preserves and externally backs up the newest durable image, and
returns failure.

When cursor 124 closes naturally, require all of the following before S2:

1. local `layer_dp_c6_s1_prod_20260802.L4.snap` exists;
2. the controller's stable D: copy with the same basename exists;
3. both files independently hash to one SHA-256;
4. the receipt says `mode=C6_PRODUCTION_L4_SNAPSHOT`;
5. the exact size, state/hole counts, orbit mass, hash, source commit, and
   command are entered in `MANIFEST.md` and a dated report;
6. a read-only snapshot round trip passes.

## S2: L4 to L5

S2 has a separate checkpoint namespace, controller, logs, progress sidecar,
and external directory.  It requires the manifest-pinned L4 hash as a
mandatory command-line value; there is no permissive default.

First run the preparation-only form, substituting the exact future manifest
hash:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass `
  -File scripts\run_layer_dp_c6_s2_window.ps1 `
  -AuthorizeFullC6 `
  -Layer4Sha256 <64-HEX-MANIFEST-HASH> `
  -PrepareOnly
```

After separate owner authorization, start the first bounded window by
removing `-PrepareOnly`.  Later windows add `-ContinueExisting`.  The defaults
remain eight target hours, ten hard hours, 85 GiB RSS, 40-minute checkpoints,
and transition-only `--stop-after 5`.

The engine's independent Burnside boundary must reject any completed or
loaded L5 whose real-state count is not exactly:

```text
96452755
```

Before S3, require a local `.L5.snap`, a stable external copy and receipt with
`mode=C6_PRODUCTION_L5_SNAPSHOT`, matching SHA-256 values, a read-only round
trip, an updated manifest, and a dated S2 report.

## S3: L5 to final certificate

S3 is a separately authorized finalization, not a continuation of the S2
process.  Its controller requires the manifest-pinned L5 hash and runs the
resource preflight before writing anything:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass `
  -File scripts\run_layer_dp_c6_s3_finalize.ps1 `
  -AuthorizeFullC6 `
  -Layer5Sha256 <64-HEX-MANIFEST-HASH> `
  -PrepareOnly
```

After explicit authorization, remove `-PrepareOnly`.  If an externally
bounded attempt stops, rerun with `-ContinueExisting`; only its last closed
parent chunk is accepted.

The actual resume-parent full SHA must equal the supplied L5 source. Each
namespace has an immutable `.s3-binding.json`; completed CSV reuse additionally
requires its `.s3-result.json`, binding the CSV, snapshot and command record.
Legacy unbound artifacts are refused, not migrated automatically. Primary and
replay paths must be distinct and nonoverlapping, and the input backup must
be on a separate volume. See
`../reports/og2/c6-s3-lineage-handoff-20260905.md` for the exact recovery boundary.

Defaults are `-SessionMaxHours 7.5 -ReserveMinutes 15 -MaxHoursPerAttempt 3.5`.
The shared operation deadline reserves handoff time across both attempts;
it is not an absolute cap on synchronous storage operations. Use an external
watchdog and leave availability headroom for physical copying. A prepare-only
run does not create bindings, checkpoints or CSVs, but may write diagnostic logs.

The controller performs all of these steps:

1. contract the immutable L5 in the `primary` checkpoint namespace;
2. retain and externally hash the closed wide L6/final checkpoint;
3. independently contract the same immutable L5 in the `replay` namespace;
4. require both sorted 63,199-row CSVs to be byte-identical;
5. require the engine's G1 and G2 bridge checks and exact class count;
6. run the independent semantic verifier described in
   `../methods/layer-dp-certificate.md`;
7. run both the Python and PowerShell/.NET BigInteger square sums;
8. require both totals to agree with each other and with the historical
   target under verification;
9. copy the final CSV to D:, verify SHA-256, and write a receipt containing
   the source commit and both totals.

Every engine invocation writes an exact `command.txt` beside its logs.  The
final summary records the CSV and semantic hashes, L5 hash, program commit,
G1/G2 values, both independent totals, external receipt, and command-record
paths.

## Final acceptance and experimental map

Before updating `STATUS.md` with a complete `N(6)`, retain:

- both byte-identical CSVs and SHA-256;
- both final wide checkpoints and their external receipts;
- the semantic verifier log and semantic SHA-256;
- both independent summation logs;
- exact program commit and complete command records;
- S1/S2/S3 progress sidecars and RSS logs;
- complete L4 and L5 snapshot headers, hashes, state/hole counts, and any
  retained state/coefficient distribution summaries.

The global layer DP aggregates all histories.  Per-layer and per-checkpoint
work, and explicitly sampled parent difficulty, are meaningful.  There is no
canonical way to allocate the global runtime uniquely among the 63,199 final
classes, so no report may claim per-final-class timings derived after the
fact.
