# Fifth bounded shared-F4 C6 window — 2026-09-05

N(6) remains uncomputed. This window adds 48,325,000 stable-ID records and
7,682,792 closed graph-representative F4 values. No native layer is exported.

## Preconditions and executed command

The complete repository gate passed in 191.6011702 seconds under a
360-second / 8-GiB external process-tree guard before this window. The
shared-engine release gate also passed. The original generation-37 source
and its separate-volume copy retain full SHA-256
`ECF0837315B0FDF8AE21C394FDA6676490E43B1828A0825529A344EC17E4E844`.
All old partial T values are discarded. The unchanged shared executable SHA is
`45A8387A031C3236191C71C28F73773711EEC34B9A628D4E715B2FE5F8EDD4FC`.

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/run_layer_shared_window.ps1 `
  -FullGateEvidence data/logs/final-baseline-handoff-2d39da8955204ee9b8198c423d59c0e7/stdout.log `
  -SharedGateEvidence data/logs/shared-final-release-run-054b103055624bfc81358dfe81ba89c6/stdout.log `
  -SourceBackup D:\sudoku_FJ_checkpoint_backups\layer_dp_c6_s1_prod_20260802\session-008-20260810-170245-layer_dp_c6_s1_prod_20260802.a `
  -MaxMinutes 30 -WorkMinutes 25 -Threads 24 -Limit 50000000 -Chunk 25000 -LimitGiB 55
```

The child stopped normally at its soft deadline before exhausting the positive
limit. Input-backup hashing and before/after copying are outside the child
budget. Controller and child exited 0; neither remains running.

## Exact retained result

```text
resumed prefix / chunks: 59675000 / 2387
new IDs / chunks: 48325000 / 1933
new closed representatives: 7682792
new representative checksum modulo 2^64: 10725876756480
current prefix: [0,108000000)
complete ID domain including holes: 903398621
live records in prefix / holes: 108000000 / 0
committed chunks / files including manifest: 4320 / 4321
total bytes: 1297106176
closed representative F4 values: 17027859
representative checksum modulo 2^64: 23769515209728
new computation/commit wall: 1465.255134 seconds
complete engine wall: 1500.045880 seconds
peak working set: 44791046144 bytes
status: INCOMPLETE_RESUMABLE
N6: NOT_COMPUTED
```

This is real factorization work, not a lookup-only benchmark. Exact aliases
retain distinct native responses, and their representatives may be outside
the processed prefix. There are 34,895,143 processed IDs already addressing
closed representatives and 73,104,857 addressing future representatives.
Thus this is not 108 million available native T4 weights. The prefix is
nonuniform and does not certify total runtime or N(6) percentage complete.

## Independent audit and physical copies

The independently implemented two-pass native reader checked every current
file, all local header/payload/value conventions, every alias pointing into
the current prefix, SHA-pinned previous audit lineage, both physical copies
and preservation of all 2,388 previous files. It opens no original catalogue
and uses no counting-engine source. This is the first production use of the
previously qualified bitset reader, not its earlier TEST_ONLY_PASS replay.

```powershell
python experiments/proto/layer_shared_bitset_audit.py `
  --namespace data/checkpoints/c6_shared_f4_20260905 `
  --backup-before D:/sudoku_FJ_checkpoint_backups/c6_shared_f4_20260905/20260905-163018-799440dfd9bb496ea730129838706504-before `
  --backup-after D:/sudoku_FJ_checkpoint_backups/c6_shared_f4_20260905/20260905-163018-799440dfd9bb496ea730129838706504-after `
  --previous-report data/logs/c6-direct-route-20260905/shared-window4-independent-audit.json `
  --previous-sha256 4B800029BA9108B59E7BFBFA1853A92CBF1030CEE307E545C08FB7280FB6EA70 `
  --worker build/layer_shared_bitset_audit.exe `
  --stdout data/logs/shared-f4-window-20260905-163018-799440dfd9bb496ea730129838706504/stdout.log `
  --expected-prefix 108000000 --max-bytes 2147483648 --maxseconds 180 `
  --output data/logs/c6-direct-route-20260905/shared-window5-independent-audit.json
```

The audit passed in 15.782692 seconds (worker 15.2331 seconds), with
23,482,368-byte worker and 68,964,352-byte parent peaks. Its representative
bitset is 13,500,000 bytes. Qualification, protocol-test and snapshot flags
are all false; complete-domain closure is false. Storage integrity is checked
at the reads, not asserted as eternal or simultaneously locked immutability.
It does not independently reevaluate the 17,027,859 numerical F4 values.

```text
namespace manifest SHA256:
848B9DC72452AB389C5BF5424E264AE86C673C7FBA673F5F14FDC845BF4DEB91
after-backup receipt SHA256:
927CECD30305BC8A84161983414B0EC87C1CD9F821EEAB1E0FA75A3248984E72
independent production audit SHA256:
93BC52CB7234383405EC4BA8A18FF175E0E49FC478BA136DE136CEDA6C110CA4
audit worker SHA256:
DD541C9318334DCA8763FEBD66E60B81F51CF35D2887456EEBFD9DD804F7D4C4
```

Controller log:
`data/logs/c6-direct-route-20260905/shared-production-window5-controller.log`.
Engine logs and RSS:
`data/logs/shared-f4-window-20260905-163018-799440dfd9bb496ea730129838706504/`.
External copies use the exact `-before` and `-after` directories in the audit
command. Original sources, previous chunks and earlier backups are preserved.

Full F4 closure/export, numerical reverse F5 and the independently replayed
63,199-class final weighted-square sum remain prerequisites for N(6).

## Final repository handoff

After the computing process and physical backup finished, the exact command
`powershell -NoProfile -ExecutionPolicy Bypass -File scripts/verify_all.ps1`
passed again in 190.6233420 seconds, with 1,108,054,016-byte aggregate peak
under an external 360-second / 8-GiB guard. All 105 tracked processes exited;
the receipt reports no survivors and no guard firing. Logs and receipt are
`data/logs/final-s3-repository-gate-qc5onj0x/`. The released shared/reverse
executables and original G1 graph memo retained their SHA, size and mtime.

Local functional commits are `90b59f0` (isolated bounded rekey/GPU probes)
and `ee8128c` (S3 source-lineage controller); checkpoint metadata is separately
retained in `909c97f`. These do not change the window's counting executable.
The full C6 geometry-rekey benchmark was deliberately not started for this
handoff. GPU prototypes were not promoted: their bounded batch was incomplete.
Existing untracked global-pivot prototypes and user paper work were preserved.
