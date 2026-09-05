# First bounded shared-F4 C6 production window — 2026-09-05

N(6) is not computed. This report records durable progress on the new exact
shared-value route, not a completed native layer or a duration guarantee.

## Preconditions and exact implementation

The complete repository gate passed in
`data/logs/baseline-release-run-25497e0f5a044500be9fb5fc1031a361/`.
The final hardened combined gate passed in
`data/logs/shared-final-release-run-054b103055624bfc81358dfe81ba89c6/`,
exit 0, 54.0753837 seconds including rebuild. It covers fresh complete C4/C5
values and exports, both reverse implementations, all 355 final C5 values,
exact N(5), forced-kill recovery, corruption/alias/hole/lineage refusal,
source canonical/stabilizer audits, and the complete 12,345-key C6 sample
fiber closure. The C6 sample does not stand for global completion.

The window's controller independently checked the full original external
backup SHA-256 before launch. The engine then independently validated every
byte of the original loaded input, including its SHA-256, before discarding
all original partial weights. Source native key semantics are unchanged.
Each processed source is explicitly audited for regularity, canonical key
and exact stabilizer before graph-value sharing.

## Exact command

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/run_layer_shared_window.ps1 `
  -FullGateEvidence data/logs/baseline-release-run-25497e0f5a044500be9fb5fc1031a361/stdout.log `
  -SharedGateEvidence data/logs/shared-final-release-run-054b103055624bfc81358dfe81ba89c6/stdout.log `
  -SourceBackup D:\sudoku_FJ_checkpoint_backups\layer_dp_c6_s1_prod_20260802\session-008-20260810-170245-layer_dp_c6_s1_prod_20260802.a `
  -MaxMinutes 10 -WorkMinutes 8 -Threads 24 -Limit 500000 -Chunk 25000 -LimitGiB 55
```

The controller bounds the computing child. Backup hashing and post-run copying
are additional wall time. Windows helpers are launched with hidden windows.

## Recorded result

```text
exit: 0
new immutable chunks: 20
closed stable-ID record interval: [0,500000)
live records: 500000
closed self-representative F4 values: 83778
representative F4 checksum modulo 2^64: 115850352768
computation plus chunk commits: 15.465317 seconds
complete engine wall, including source validation/index: 47.835427 seconds
peak working set: 44790128640 bytes
new committed files including manifest: 21
new committed bytes: 6005376
```

Every record has either a closed self-representative value or an exact alias
to its graph representative. Many aliases can refer to later IDs, so this is
not a claim that every processed native T4 is already numerically available.
Only closed representative F4 values, never partial sums, are serialized.

The 500,000 source IDs are a catalogue prefix, not a uniform sample. Its
representative density and evaluation cost may differ from later ranges.
Scaling this prefix's elapsed time directly to the full catalogue is not a
qualified runtime estimate. Log fields ending in `_cpu_s` are sums of timed
per-worker elapsed intervals, not operating-system CPU accounting; reported
`wall_s` is the actual end-to-end elapsed measurement.

## Durability and retained evidence

New namespace:
`data/checkpoints/c6_shared_f4_20260905/` (explicitly ignored by Git).

Controller log:
`data/logs/c6-direct-route-20260905/shared-production-window1-controller.log`.

Engine stdout, stderr and RSS:
`data/logs/shared-f4-window-20260905-143205-559e384c66c6423ebd1eee48766896bf/`.

The post-run external physical copy is
`D:\sudoku_FJ_checkpoint_backups\c6_shared_f4_20260905\20260905-143205-559e384c66c6423ebd1eee48766896bf-after\`.
Its receipt records and verifies SHA-256 on every source and copied committed
file. Receipt SHA-256:
`D96C9CEE941B72C6ADF6383DA27C73D5F22D5D60993B2ABCD4F1F520154BF854`.
Manifest SHA-256:
`848B9DC72452AB389C5BF5424E264AE86C673C7FBA673F5F14FDC845BF4DEB91`.

The original A/B inputs were not overwritten, moved or deleted. No native
production export was requested. No engine remained after normal exit.

An independent post-run reader checked every raw chunk header/payload and
every source/copy/receipt SHA-256, reproduced the contiguous prefix and all
logged counts, and independently reconstructed the support-repair hash from
all 46,080 coordinate transformations. It found 100,509 committed IDs already
referencing closed representatives and 399,491 referring to future
representatives. This is an integrity/provenance audit, not a second numerical
evaluation of the 83,778 F4 values. The retained reader is
`experiments/proto/layer_shared_checkpoint_audit.py`; its exact output is
`data/logs/c6-direct-route-20260905/shared-window1-independent-audit.json`.
