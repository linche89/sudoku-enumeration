# Seventh bounded shared-F4 C6 window — 2026-09-05

N(6) remains uncomputed. The first production window using the qualified
incremental CPU kernel completed normally, adding 296,200,000 stable IDs and
45,477,034 closed representative F4 values. Its external backup and independent
production integrity audit passed. No new production window was started during
the owner's subsequent status review.

## Exact executed command and termination

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/run_layer_shared_window.ps1 `
  -FullGateEvidence data/logs/shared-incremental-full-release-arnxql7o/stdout.log `
  -SharedGateEvidence data/logs/shared-incremental-installed-release-pq4fj7dy/stdout.log `
  -SourceBackup D:\sudoku_FJ_checkpoint_backups\layer_dp_c6_s1_prod_20260802\session-008-20260810-170245-layer_dp_c6_s1_prod_20260802.a `
  -MaxMinutes 105 -WorkMinutes 100 -Threads 24 -Limit 300000000 -Chunk 25000 -LimitGiB 55
```

The SHA-qualified installed executable was
`2B8A14606F8F78748C9338130D35551CA199E53238BEB33D95BCB05A2555AFE0`;
it was rehashed unchanged at the final review. Release and old-version
compatibility evidence are in `shared-f4-incremental-publication-20260905.md`.
No native key, weight normalization, alias or chunk-format semantics changed.

The child started at 18:13:17 local time with `workseconds=6000`,
`maxseconds=6295`, `maxrssgib=55`, `checkpointreadonly` and
`repair-l4-support`. Its original source's complete SHA was validated before
every old partial T was discarded. The single missing support witness was
appended in RAM only. The controller verified the physical source backup and
all previous chunk copies before launching.

The child reached its soft deadline, not its 300-million-ID limit. Engine
stderr was empty, both engine/controller exited zero and the after-backup
completed. The last engine log write was 19:53:23; the controller log completed
at 19:57:14. A read-only process query at 20:20 found no shared-F4 process.
The original computing session was not restarted.

## Exact retained result

```text
resumed prefix / chunks: 156375000 / 6255
resumed closed representatives: 24667321
new IDs / chunks: 296200000 / 11848
new live IDs / holes: 296199991 / 9
new closed representative values: 45477034
new representative checksum modulo 2^64: 63157842290688
current prefix: [0,452575000)
complete ID domain including holes: 903398621
live records / holes in current prefix: 452574991 / 9
committed chunks / files including manifest: 18103 / 18104
total committed bytes: 5435534624
closed representative F4 values: 70144355
representative checksum modulo 2^64: 97566141160704
new computation/commit wall: 5949.581754 seconds
complete engine wall: 6000.213190 seconds
peak working set: 44735246336 bytes
status: INCOMPLETE_RESUMABLE
N6: NOT_COMPUTED
```

The processed ID prefix is 50.09693279% of the complete ID domain. This is
not the percentage of the complete N(6) task, nor a percentage of all graph
values. Of the processed live IDs, 300,246,735 resolve to already closed
representatives and 152,328,256 refer to future representatives. These add
to 452,574,991, excluding nine insertion holes. No native F4 export exists.

## Independent production audit and physical copies

The exact executed audit was:

```powershell
python experiments/proto/layer_shared_bitset_audit.py `
  --namespace data/checkpoints/c6_shared_f4_20260905 `
  --backup-before D:/sudoku_FJ_checkpoint_backups/c6_shared_f4_20260905/20260905-181202-99e8286d25c7401f944ce14c0fc6039b-before `
  --backup-after D:/sudoku_FJ_checkpoint_backups/c6_shared_f4_20260905/20260905-181202-99e8286d25c7401f944ce14c0fc6039b-after `
  --previous-report data/logs/c6-direct-route-20260905/shared-window6-independent-audit.json `
  --previous-sha256 E4544DDE15555EA4F47A63F5405C6200119BEF8351EFA688CDBAB5834FE6F0BE `
  --worker build/layer_shared_bitset_audit.exe `
  --stdout data/logs/shared-f4-window-20260905-181202-99e8286d25c7401f944ce14c0fc6039b/stdout.log `
  --expected-prefix 452575000 --max-bytes 6442450944 --maxseconds 180 `
  --output data/logs/c6-direct-route-20260905/shared-window7-independent-audit.json
```

All 18,104 committed files, headers/payloads, value/alias conventions, prior
audit lineage, physical copies and logged counters agreed. All 6,256 previous
files remained byte-identical. This is a production PASS, not a snapshot,
qualification replay or protocol test. Complete-domain closure remains false.

The bounded audit took 65.3714846 seconds, including 64.4282 worker seconds.
Worker and parent peaks were 76,378,112 and 116,891,648 bytes; each was capped
at 1 GiB. The representative bitset used 56,571,875 bytes. This independent
reader does not include counting-engine source and opens no full original
catalogue. It checks file integrity and aliases, not an independent numerical
reevaluation of every saved F4. Integrity is established at the verified reads,
not as permanently immutable or simultaneously locked storage.

```text
unchanged namespace manifest SHA256:
848B9DC72452AB389C5BF5424E264AE86C673C7FBA673F5F14FDC845BF4DEB91
before receipt / preceding after receipt SHA256:
E54BADC15712081634993B6CF1C70753F32DEBF21F5B7D273951F8456948EFF9
after receipt SHA256:
CBF8655AEF4CCFBC336A13511BBD3AB4B071E8F9637C84A85220571F81B64A16
independent production audit SHA256:
EF661DC093E9DCAC888F6934E9341C2412C7B51AFE156A6C545AE73C289972F7
audit worker SHA256:
DD541C9318334DCA8763FEBD66E60B81F51CF35D2887456EEBFD9DD804F7D4C4
engine stdout SHA256:
1435D39912CE29AEF50EF291137BCDCCC2F914F3D528CFB51BE7E4DB965EBC21
controller log SHA256:
19174E0183338CB8FF608AA83E317B0369E41EBE95A70BB4841A48A489D664E8
```

Controller log:
`data/logs/c6-direct-route-20260905/shared-production-window7-controller.log`.
Engine logs and RSS:
`data/logs/shared-f4-window-20260905-181202-99e8286d25c7401f944ce14c0fc6039b/`.
The before/after external directories are those named in the audit command.
No original source, prior chunk or old physical copy was removed or replaced.

## Measured throughput and a scoped planning estimate

Window6 scanned 48,375,000 IDs in 1,500.277530 engine seconds: 32,244.0342
IDs/s. Window7 scanned 296,200,000 in 6,000.213190 seconds: 49,364.9126 IDs/s.
The observed ratio is 1.53097818x; excluding load/resume time gives 1.50729370x.
Both exclude controller backup overhead. The different ID intervals have
different representative proportions, and window6 also overlapped bounded
diagnostics. This is not a controlled same-input attribution of all the gain
to the new arithmetic kernel, nor a total N(6) speedup measurement.

There remain exactly 450,823,621 unprocessed IDs. A linear extrapolation of
window7's engine-wall rate gives 2.53679743 hours for that F4 scan; the
compute-only rate gives 2.51539124 hours. These are planning estimates, not
bounds: the native order is nonuniform, later costs may differ, and backups,
audits, closed native export, numerical reverse F5 and the final independent
outer contractions are not included. No end-to-end N(6) deadline is claimed.

## Final handoff verification

On the owner's return, the complete repository gate was run again:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/verify_all.ps1
```

It passed in 174.5688544 seconds under the existing 360-second / 6-GiB
aggregate process-tree guard. Sampled aggregate peak was 1,134,751,744 bytes;
all 107 tracked processes exited, with no guard firing or surviving child.
The gate covers the mandatory C2..5 values, all C5 class comparisons, native
canonicalization and recovery differentials, FJ9 reproduction and the
read-only G1 memo check. Shared/reverse production executables and the G1
memo retained their before/after SHA. No arithmetic source was changed.

Logs: `data/logs/shared-window7-handoff-full-uangbw2w/`.
Receipt SHA256:
`2AD5EA1E7CF1AE69FD42CB0DB297F7B864486A567C9FFAA959F1F0B1D4DBD60C`.
Outer stdout SHA256:
`365698CA48111565C69F4F712A02774C1E6B181BC720E46E3D69E20B2050EB58`.

The original background terminal returned exit zero when collected. Final
process checks found no shared-F4/reverse-F5 engine or shared-window
controller. `git diff --check` passed. This review updates only status,
checkpoint metadata and documentation; the owner's unrelated untracked
files are preserved. No next production window, F4 export or numerical F5
job was started. The existing installed-path shared gate is retained rather
than redundantly rerun, since its SHA-pinned arithmetic is unchanged.
