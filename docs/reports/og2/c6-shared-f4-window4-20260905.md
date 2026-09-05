# Fourth bounded shared-F4 C6 window — 2026-09-05

N(6) remains uncomputed. This real window adds 49,150,000 native-ID records
and 7,655,609 closed graph-representative F4 values. It exports no native layer.

## Preconditions and command

The complete repository gate had passed in 220.6553043 seconds under a
360-second / 8-GiB bound. Its evidence and the unchanged shared-engine gate
were explicitly supplied to the controller. The original 32,522,350,448-byte
generation-37 input and separate-volume physical backup retain SHA-256
`ECF0837315B0FDF8AE21C394FDA6676490E43B1828A0825529A344EC17E4E844`.
No original partial T value is used. The released shared executable retains
SHA-256 `45A8387A031C3236191C71C28F73773711EEC34B9A628D4E715B2FE5F8EDD4FC`.

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/run_layer_shared_window.ps1 `
  -FullGateEvidence data/logs/final-baseline-release-871e5c39f9914f13a7f27043adc4973c/stdout.log `
  -SharedGateEvidence data/logs/shared-final-release-run-054b103055624bfc81358dfe81ba89c6/stdout.log `
  -SourceBackup D:\sudoku_FJ_checkpoint_backups\layer_dp_c6_s1_prod_20260802\session-008-20260810-170245-layer_dp_c6_s1_prod_20260802.a `
  -MaxMinutes 30 -WorkMinutes 25 -Threads 24 -Limit 50000000 -Chunk 25000 -LimitGiB 55
```

The positive limit is at most 50 million additional IDs. The process stopped
normally at its soft time boundary before reaching that limit. Source-backup
hashing and before/after backup copying are outside the child's time budget.

## Exact retained result

```text
controller exit: 0
resumed prefix: 10525000
resumed chunks: 421
new chunks: 1966
new IDs: 49150000
new closed representative F4 values: 7655609
new representative-value checksum: 10692950954304

closed prefix: [0,59675000)
complete domain including holes: 903398621
live records in prefix: 59675000
immutable chunks: 2387
closed representative F4 values: 9345067
representative-value checksum modulo 2^64: 13043638453248
new computation plus commit wall: 1465.946353 seconds
complete engine wall: 1500.746988 seconds
peak working set: 44790517760 bytes
status: INCOMPLETE_RESUMABLE
N6: NOT_COMPUTED
```

This is actual factorization work, not a lookup-only timing probe. Numerical
F4 values are stored only at graph representatives; other records contain
exact aliases while keeping distinct native responses. The independent audit
found 15,991,860 processed IDs addressing already closed representatives and
43,683,140 addressing future representatives. It is therefore incorrect to
call this prefix 59,675,000 available native T4 weights.

The prefix is not a uniform sample of the remaining domain. Neither its ID
fraction nor its elapsed time is a percentage-complete or duration certificate
for N(6). Full F4 closure/export, reverse F5 and the final independently
replayed 63,199-class weighted-square sum all remain required.

## Independent integrity audit and physical copies

All 2,388 files, including the immutable manifest, occupy 716,711,328 bytes.
The controller copied and hashed both the old prefix and the resulting prefix
on D:. The independent reader checked current and physical-copy bytes, SHA
receipts, headers, payload checksums, local value conventions, exact counters
and all aliases whose targets lie in the current prefix. All 422 prior files
were unchanged. Production snapshot/test relaxations were false.

The reader also now refuses a SHA-resealed predecessor marked as a snapshot
or backup test. Separate disposable tests accepted a valid predecessor and
rejected both false production-lineage cases. This hardening does not change
the computing engine or checkpoint format.

```powershell
python experiments/proto/layer_shared_resume_audit.py `
  --namespace data/checkpoints/c6_shared_f4_20260905 `
  --backup-before D:/sudoku_FJ_checkpoint_backups/c6_shared_f4_20260905/20260905-152513-9796d789d4a444f6b4053bc84dcde8ca-before `
  --backup-after D:/sudoku_FJ_checkpoint_backups/c6_shared_f4_20260905/20260905-152513-9796d789d4a444f6b4053bc84dcde8ca-after `
  --previous-report data/logs/c6-direct-route-20260905/shared-window3-independent-audit.json `
  --previous-sha256 F2F6258AC9187DAF5D813AB30665A88A1ABE428DD9AF97D9E9BF69E15C85283D `
  --stdout data/logs/shared-f4-window-20260905-152513-9796d789d4a444f6b4053bc84dcde8ca/stdout.log `
  --expected-prefix 59675000 --max-entries 100000000 --max-bytes 1073741824 `
  --maxseconds 180 --maxrssgib 4 `
  --output data/logs/c6-direct-route-20260905/shared-window4-independent-audit.json
```

```text
audit: PASS
audit seconds: 32.5823988
audit peak RSS bytes: 502079488
namespace manifest SHA256:
848B9DC72452AB389C5BF5424E264AE86C673C7FBA673F5F14FDC845BF4DEB91
after receipt SHA256:
447DB8AF769704C521F7CE8E4464C3B1E34D5C0601C7C9A7A4AC6BFCA345CA1B
independent report SHA256:
4B800029BA9108B59E7BFBFA1853A92CBF1030CEE307E545C08FB7280FB6EA70
```

This audit establishes storage/provenance/alias consistency, not an independent
numerical reevaluation of all 9,345,067 F4 values. Factorization correctness
rests on the proved recurrence and complete coefficient/recovery gates.

Controller log:
`data/logs/c6-direct-route-20260905/shared-production-window4-controller.log`.
Engine logs and RSS:
`data/logs/shared-f4-window-20260905-152513-9796d789d4a444f6b4053bc84dcde8ca/`.
Guard tests:
`data/logs/c6-direct-route-20260905/shared-audit-chain-guard-tests/summary.json`.
Both external copies are under
`D:/sudoku_FJ_checkpoint_backups/c6_shared_f4_20260905/`, with the exact
`20260905-152513-9796d789d4a444f6b4053bc84dcde8ca-before` and `-after` names above.

The computing child and controller exited normally. Original checkpoints,
previous chunks and earlier backups were preserved. No multi-hour or overnight
computing job was launched by this window.
