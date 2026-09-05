# Third bounded shared-F4 C6 window — 2026-09-05

N(6) remains uncomputed. This window adds ten million native-ID records to
the actual shared-F4 computation. No complete native layer is exported.

## Preconditions and exact invocation

The preceding complete repository regression passed under a 360-second /
8-GiB external process-tree bound in 220.6553043 seconds. Its stdout is
`data/logs/final-baseline-release-871e5c39f9914f13a7f27043adc4973c/stdout.log`.
The separately released shared engine and its dependencies were unchanged.
The executable SHA remains
`45A8387A031C3236191C71C28F73773711EEC34B9A628D4E715B2FE5F8EDD4FC`.

Preflight found no counting process, about 99 GiB available physical RAM,
1.34 TB free on D: and 1.74 TB on E:. The controller verified the original
external-source backup and copied all 22 previous committed files to a new
before-backup directory before allowing writes.

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/run_layer_shared_window.ps1 `
  -FullGateEvidence data/logs/final-baseline-release-871e5c39f9914f13a7f27043adc4973c/stdout.log `
  -SharedGateEvidence data/logs/shared-final-release-run-054b103055624bfc81358dfe81ba89c6/stdout.log `
  -SourceBackup D:\sudoku_FJ_checkpoint_backups\layer_dp_c6_s1_prod_20260802\session-008-20260810-170245-layer_dp_c6_s1_prod_20260802.a `
  -MaxMinutes 10 -WorkMinutes 8 -Threads 24 -Limit 10000000 -Chunk 25000 -LimitGiB 55
```

These bounds permit at most ten million additional IDs, an eight-minute soft
work window and a ten-minute externally guarded child. Backup hashing/copying
is additional wall time. No multi-hour unattended run was started.

## Exact result

```text
controller exit: 0
resumed chunks: 21
resumed prefix: 525000
new chunks: 400
new IDs: 10000000
new closed representative F4 values: 1602031
new representative-value checksum: 2229769358208

closed prefix: [0,10525000)
complete domain including holes: 903398621
live records in current prefix: 10525000
total immutable chunks: 421
total closed representative F4 values: 1689458
total representative-value checksum modulo 2^64: 2350687498944
new computation plus commit wall: 302.293218 seconds
complete engine wall: 348.030754 seconds
peak working set: 44791013376 bytes
status: INCOMPLETE_RESUMABLE
N6: NOT_COMPUTED
```

The counter snapshot is from the actual computing engine, not a lookup-only
probe. However, only graph representatives carry numerical F4. Other native
IDs retain exact aliases and distinct eventual responses. The independent
audit found 2,156,246 processed IDs whose representative is already closed
and 8,368,754 whose representative has yet to be computed. Thus the prefix
is not a claim of 10,525,000 available native T4 values.

This remains a nonuniform catalogue prefix. Its 302-second computing time is
not a population-wide duration or a full-N(6) speedup certificate.

## Independent integrity and backup verification

The new `experiments/proto/layer_shared_resume_audit.py` reads immutable
chunks with compact u32 alias storage. It independently decodes headers and
payloads, checks the pinned input lineage and repair witness, recomputes
checksums, verifies contiguous prefix/range/value semantics and resolves every
alias pointing into the closed prefix. It binds the previous audit by full
SHA and checks every previous file is unchanged. Both external backup
inventories, bytes and SHA receipts must agree.

The actual window-3 audit passed in approximately 5.12 seconds with peak
103,464,960 bytes, under explicit 180-second / 4-GiB bounds. Production mode
used neither the old-prefix snapshot relaxation nor the same-volume test
relaxation. All 422 committed files, totalling 126,408,032 bytes, matched the
after backup; all previous 22 files matched the before backup and prior audit.

```powershell
python experiments/proto/layer_shared_resume_audit.py `
  --namespace data/checkpoints/c6_shared_f4_20260905 `
  --backup-before D:/sudoku_FJ_checkpoint_backups/c6_shared_f4_20260905/20260905-150953-5405e65ad3b14c34a45fa7b7826ee6ce-before `
  --backup-after D:/sudoku_FJ_checkpoint_backups/c6_shared_f4_20260905/20260905-150953-5405e65ad3b14c34a45fa7b7826ee6ce-after `
  --previous-report data/logs/c6-direct-route-20260905/shared-window2-independent-audit.json `
  --previous-sha256 B2A6EE5D5590F597FDC4DCB15CD3769E20CC12D7DB043FB8949746B3F1CDE867 `
  --stdout data/logs/shared-f4-window-20260905-150953-5405e65ad3b14c34a45fa7b7826ee6ce/stdout.log `
  --expected-prefix 10525000 --max-entries 20000000 --max-bytes 268435456 `
  --maxseconds 180 --maxrssgib 4 `
  --output data/logs/c6-direct-route-20260905/shared-window3-independent-audit.json
```

The report output is exclusive; use a fresh path for any repeat. Before this
run, the helper reproduced the independently audited window-2 immutable
snapshot and correctly refused same-volume production backups and exceeded
byte/time bounds. The actual time-guard kill returned 124 without a completed
report. Test transcripts are retained in
`data/logs/c6-direct-route-20260905/shared-resume-audit-tests.json`.

```text
after receipt SHA256:
A9800577A51FA80A9CB8F3F64CB8FC6F2741BBF6424246613C11BF6CB09588E2
unchanged namespace manifest SHA256:
848B9DC72452AB389C5BF5424E264AE86C673C7FBA673F5F14FDC845BF4DEB91
independent window-3 report SHA256:
F2F6258AC9187DAF5D813AB30665A88A1ABE428DD9AF97D9E9BF69E15C85283D
```

This is an integrity/provenance/alias audit, not a second numerical
calculation of all 1,689,458 F4 values. Numerical correctness relies on the
proved recurrence and retained complete coefficient/recovery release gates.

Controller log:
`data/logs/c6-direct-route-20260905/shared-production-window3-controller.log`.
Engine stdout/stderr/RSS:
`data/logs/shared-f4-window-20260905-150953-5405e65ad3b14c34a45fa7b7826ee6ce/`.
Independent report:
`data/logs/c6-direct-route-20260905/shared-window3-independent-audit.json`.
Backup root:
`D:\sudoku_FJ_checkpoint_backups\c6_shared_f4_20260905\`, with new directories
`20260905-150953-5405e65ad3b14c34a45fa7b7826ee6ce-before` and the matching
`-after` directory.

## Source provenance and remaining work

The tested counting programs, supporting proofs' finite-check implementations,
controllers, release gates and small support witness fixture were saved in
local functional commit `6d19785`. That commit did not change the released
binary or the already running program. Documentation and checkpoint manifest
updates are kept separate. No remote push was made; no binary checkpoint or
user paper directory was staged.

Full F4 alias closure, a fresh verified native export, exact reverse F5 and
the independently replayed final 63,199-class square sum all remain required.
The original A/B checkpoints and all earlier immutable chunks/backups were
preserved, and no counting process remained after this window.
