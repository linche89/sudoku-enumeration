# Second bounded shared-F4 C6 window — 2026-09-05

N(6) is not computed. This window verifies real-catalogue resume and saves
one additional exact chunk; it does not close the native L4 layer.

## Executed command and bounds

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/run_layer_shared_window.ps1 `
  -FullGateEvidence data/logs/baseline-release-run-25497e0f5a044500be9fb5fc1031a361/stdout.log `
  -SharedGateEvidence data/logs/shared-final-release-run-054b103055624bfc81358dfe81ba89c6/stdout.log `
  -SourceBackup D:\sudoku_FJ_checkpoint_backups\layer_dp_c6_s1_prod_20260802\session-008-20260810-170245-layer_dp_c6_s1_prod_20260802.a `
  -MaxMinutes 3 -WorkMinutes 2 -Threads 24 -Limit 25000 -Chunk 25000 -LimitGiB 55
```

The controller and child exited 0. The child had a 120-second soft deadline,
175-second internal hard guard and 3-minute external time guard. Original
source-backup hashing and before/after chunk backups are additional wall
time. The original source was read-only and its full pinned SHA was checked;
all historical partial weights were erased before computation.

## Exact observed output

```text
RESUMED chunks=20 closed_prefix=500000 closed_representatives=83778
CLOSED_CHUNK begin=500000 count=25000 live=25000 representatives=3649
pairings=352798 F4leaves=54221713 F4nodes=242142644
closed_prefix=525000/903398621

SUMMARY status=INCOMPLETE_RESUMABLE domain=complete_native_L4
new_chunks=1 new_indices=25000 closed_prefix=525000 total_entries=903398621
closed_representatives=87427 F4_checksum_mod2_64=120918140736
compute_wall_s=0.716339 wall_s=46.116907 peak_rss_bytes=44788338688
N6=NOT_COMPUTED
```

The new representative-value checksum is 5,067,787,968. The complete retained
namespace has 21 immutable chunks plus its unchanged 256-byte manifest,
totalling 6,305,632 bytes. These are stable-ID decisions with numerical F4
only at graph representatives, not 525,000 closed native T4 values.

The independent reader found 105,290 processed IDs whose representative is
already closed, and 419,710 whose representative lies in a future chunk.
Full export must wait for all representatives. This is a catalogue prefix,
not a uniform timing sample; no whole-job runtime is inferred.

## Backup and independent integrity audit

Controller log:
`data/logs/c6-direct-route-20260905/shared-production-window2-controller.log`.

Engine stdout/stderr/RSS:
`data/logs/shared-f4-window-20260905-145352-cd47a5df238a497582627f60acf94586/`.

Physical backup root:
`D:\sudoku_FJ_checkpoint_backups\c6_shared_f4_20260905\`.
The new `20260905-145352-cd47a5df238a497582627f60acf94586-before` directory
contains all 21 previously committed files; the matching `-after` directory
contains all 22 current files. Each receipt verifies source-before,
destination and source-after SHA-256 for every file.

```text
unchanged manifest SHA256:
848B9DC72452AB389C5BF5424E264AE86C673C7FBA673F5F14FDC845BF4DEB91
after/receipt.csv SHA256:
89E3E2D3D88F87F59E747C89B695A61ABF56CCE469A29173A332DE5D76FF6BB2
independent audit JSON SHA256:
B2A6EE5D5590F597FDC4DCB15CD3769E20CC12D7DB043FB8949746B3F1CDE867
```

The independent audit read all raw chunk headers/payloads and backup receipts,
matched the contiguous prefix and exact counters, and confirmed all original
20 chunks and the manifest are byte-identical to the window-1 inventory and
backup. Its output is
`data/logs/c6-direct-route-20260905/shared-window2-independent-audit.json`.
This establishes integrity and provenance, not a separate recalculation of
all 87,427 numerical F4 values.

No original checkpoint was overwritten, moved or deleted. No native L4 export
was requested, and no shared-F4 process remained after the window.
