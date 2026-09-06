# Complete shared-F4 catalogue and independent audit — 2026-09-06

The complete F4 catalogue is closed and its independent byte/backup/alias
audit passes. N(6) remains uncomputed. No native F4 export or numerical F5
production run was started during this review.

## Exact inventory

| Quantity | Independently decoded result |
| --- | ---: |
| Complete stable-ID domain | 903,398,621 |
| Live native records | 903,398,603 |
| Insertion holes | 18 |
| Closed graph-representative F4 values | 140,069,579 |
| Live aliases resolving to closed self-representatives | 903,398,603 |
| Aliases to uncomputed representatives | 0 |
| Committed chunks | 36,136 |
| Files including manifest | 36,137 |
| Committed bytes | 10,850,034,524 |
| Representative value checksum modulo 2^64 | 194,468,287,162,752 |
| Representative bitset bytes | 112,924,828 |

Thus the F4 ID scan is exactly 100% complete. The full native response domain
is retained; sharing one F4 value among its graph-equivalent states does not
merge their downstream coefficients. The ratio
`903398603 / 140069579 = 6.4496417384` is the average number of live native
states per stored representative, not a measured whole-job speedup.

The producer independently performs its own final alias/value closure before
printing `CLOSED_F4_CATALOGUE`. The separate auditor rechecks the record-level
condition, not the graph-equivalence construction or each factorization's
arithmetic. The source support, graph-sharing proof and complete small-case
numerical/recovery gates remain part of the numerical provenance.

## Actual final computing window and the preceding interrupted one

The owner manually launched the repaired `run_continue.ps1`. Its native
controller invocation was equivalent to:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/run_layer_shared_window.ps1 `
  -FullGateEvidence data/logs/shared-window7-handoff-full-uangbw2w/stdout.log `
  -SharedGateEvidence data/logs/shared-incremental-installed-release-pq4fj7dy/stdout.log `
  -SourceBackup D:/sudoku_FJ_checkpoint_backups/layer_dp_c6_s1_prod_20260802/session-008-20260810-170245-layer_dp_c6_s1_prod_20260802.a `
  -Limit 106073621 -Chunk 25000 -Threads 24 -WorkMinutes 75 -MaxMinutes 90 -LimitGiB 55
```

This is historical evidence, not a command to rerun the now-finished scan.

```text
resumed prefix / chunks: 797325000 / 31893
resumed representatives: 122824196
new IDs / chunks: 106073621 / 4243
new representative values: 17245383
final partial chunk: [903375000,903398621), 23621 records
new computation/commit wall: 2196.059251 seconds
complete engine wall: 2258.025330 seconds
engine peak working set: 44734844928 bytes
engine status: CLOSED_F4_CATALOGUE
engine/controller exit: 0
engine stderr: empty
N6: NOT_COMPUTED
```

The final engine log was written at 09:38:15 local time; the after-backup
receipt at 09:47:06 and controller log at 09:47:07, all on 2026-09-06.

Window8's retained terminal prefix was 797,325,000, with 122,824,196
representatives and checksum 170,664,740,452,608. Its controller ended with
exit 98 after a time bound, with this nonempty engine diagnostic:

```text
BOUND time/RSS; only earlier committed chunks are accepted
```

Windows System event ID 1, provider Microsoft-Windows-Power-Troubleshooter,
reported sleep at `2026-09-05T14:34:41.199028800Z` and wake at
`2026-09-06T00:34:15.948528100Z`. Its long recorded wall interval includes
that approximately 35,974.7495-second sleep. The exit-98 window is not
silently converted into an ordinary successful window or an audited ancestor.
Its controller did report a completed physical after-backup of 31,894 files;
the subsequent full audit uses window9's before/after copies instead.

## Full independent audit, no unaudited counter inheritance

The original single-successor auditor correctly requires an independently
accepted immediately preceding report. No window8 report was fabricated and
no logs were sanitized to fit that protocol.

The new `experiments/proto/layer_shared_complete_audit.py` uses the SAME
previously qualified native reader, without recompilation. It:

1. Requires the exact full-domain file inventory and final-window before/after
   receipt inventories on separate volumes, with no existing output artifacts.
2. SHA-pins the historical window7 report and compares its 18,104 file records
   with the final files. It does not inherit any decoded counters from it.
3. Independently decodes every final raw chunk. The reader checks source,
   repair, semantics, headers, payload checksums, holes, value conventions,
   and positive self-representative values divisible by 24. It compares the
   current and physical after-copy bytes, and the physical before-copy bytes
   for the entire resumed prefix.
4. Builds the full representative bitset, rereads/rehashes all current chunks,
   and checks every live alias against that bitset.
5. Sums the independently decoded final-window prefix, matches the RESUMED
   line, all 4,243 new chunk log entries and terminal summary, and binds the
   final controller's zero exit and both backup markers to the actual paths.
6. Rechecks inventories, receipts, worker/plan SHA and final logs before
   writing a fresh complete-domain PASS report under `data/logs`.

Exact executed command:

```powershell
python experiments/proto/layer_shared_complete_audit.py `
  --namespace data/checkpoints/c6_shared_f4_20260905 `
  --backup-before D:/sudoku_FJ_checkpoint_backups/c6_shared_f4_20260905/20260906-085138-8974f5a375294f26852c7ac7392947d9-before `
  --backup-after D:/sudoku_FJ_checkpoint_backups/c6_shared_f4_20260905/20260906-085138-8974f5a375294f26852c7ac7392947d9-after `
  --historical-report data/logs/c6-direct-route-20260905/shared-window7-independent-audit.json `
  --historical-sha256 EF661DC093E9DCAC888F6934E9341C2412C7B51AFE156A6C545AE73C289972F7 `
  --stdout data/logs/shared-f4-window-20260906-085138-8974f5a375294f26852c7ac7392947d9/stdout.log `
  --controller-log data/logs/shared-manual-f24e041961594bbf9908f1749dfe9f2a.log `
  --worker build/layer_shared_bitset_audit.exe --maxseconds 180 `
  --output data/logs/c6-direct-route-20260905/shared-complete-independent-audit-20260906.json
```

It passed in 166.5885719 seconds end-to-end (165.431 worker seconds), under
the unchanged 180-second guard. Worker peak was 145,981,440 bytes; parent
peak was 171,200,512 bytes. Each process had a 1-GiB hard bound. The full
bitset used 112,924,828 bytes. The prior physical copy contains 9,576,064,864
bytes; current twice plus before/after reads total 42,126,168,436 committed
bytes, excluding small plans and metadata.

The audit never opens the original 32.5-GB catalogue and never writes any
production checkpoint, chunk, backup or counting accumulator. Its certificate
describes contents at the verified reads, not a simultaneous or permanently
immutable filesystem snapshot. The export controller must still lock and
revalidate its inputs. No independent recalculation of all 140 million F4
values is claimed.

## Verification and retained pins

Before the production audit:

```powershell
python experiments/proto/test_layer_shared_complete_audit.py
python experiments/proto/layer_shared_bitset_audit_gate.py `
  --worker build/layer_shared_bitset_audit.exe `
  --output-dir data/logs/shared-complete-native-gate-20260906
```

All six new test methods passed, including changed counters/log entries,
missing or duplicate terminal data, incorrect scope, unclosed aliases,
controller exit 98, wrong backup paths/counts and UTF-8/UTF-16 controller
logs. The unchanged reader's synthetic gate accepted 10 direct-array cases,
rejected 38 malformed native cases and 10 invalid predecessor variants, and
retained all 1,008 exact algebraic differential cases. Its runtime was
3.5049591 seconds and parent peak 68,132,864 bytes under a 120-second/1-GiB
guard; all constructed checkpoints were disposable test data under logs.

After the full audit, the repository regression was rerun:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/verify_all.ps1
```

It passed in 174.1988362 seconds under the existing TreeBound
360-second/6-GiB aggregate guard. Peak sampled aggregate RSS was
1,135,423,488 bytes. All 119 tracked processes exited; no guard fired and no
survivors required termination. This reproduces mandatory C2..5 counts,
complete C5 comparisons, canonicalization/histogram and recovery checks,
FJ9 and the read-only G1 checkpoint value. Protected shared-F4, reverse-F5
and audit executables and the G1 memo retained their before/after SHA.
The optional full C6 F4 numerical recomputation was not run; the existing
qualified production arithmetic was not changed or rebuilt.

```text
namespace manifest SHA256:
848B9DC72452AB389C5BF5424E264AE86C673C7FBA673F5F14FDC845BF4DEB91
complete independent audit JSON SHA256:
8FAA333C7F4CC38D6733AA94A3DCFE31D2E469D602B3A212065DE70EAF001741
window9 before receipt SHA256:
D3953840456D97F9A1BE5198DA4ED32C5A8FA6171A125374ECCA9EFD9BA45DEF
window9 after receipt SHA256:
C0FBBDF098186FCA77E6C47E2CA64A2FD6DA60151FB51E08CB3B1ECC1EC6284E
independent audit worker SHA256:
DD541C9318334DCA8763FEBD66E60B81F51CF35D2887456EEBFD9DD804F7D4C4
full-domain representative bitset SHA256:
220D8B3F326E7838D2CF845C96DFCAFAC4513AC07AA4BCC7CB13872409E0A390
final engine stdout SHA256:
7D796D0343777CE7DA0D426969F3A7797ACC9722425E28E9077EDC66E85E0E85
final controller log SHA256:
8D003855DB4A36C41B816A5647FCC0B7B0525A1DB09112D492B0F528CD236109
native synthetic gate summary SHA256:
F6256919A30C45CD30C4CB27CE8EE55F1547F295D75BDF6902295254640CE14C
complete repository stdout SHA256:
1A0BE2558C5262B29C4440890CC606EAD9B8D5D5ED402857DD1CAA926F4EBC1B
complete repository receipt SHA256:
03D465FD553DA1595370F2B16815BDD4DED52F8504EA359B54DD4DD0068274D9
```

Regression logs/receipt: `data/logs/shared-complete-handoff-full-20260906/`.
The new audit source and test source SHA256 values at this run were,
respectively, `0A661253A50ADF4F7330D48147F1066C77299A93091AD608914DACB9DCE65E5B`
and `7210C41CCD823B9F375CDB53AE319748D807C18EF54CEB187443D39AC9BE9C1F`.

## Overall progress: planning estimate, not an exact completion fraction

The user also requested an overall C6 completion percentage. The exact stage
fractions are F4 scan/alias closure **100%**, new-route numerical F5 **0%**,
and final complete outer-sum certificate **not produced**. State counts across
F4 and F5 are not comparable units of cost and are not added to invent a
90%-complete overall indicator.

For a provisional **new-route main-computation-time** indicator only:

```text
sum of all nine logged F4 engine wall times: 56069.912856 seconds
subtract the observed window8 sleep: approximately 35974.7495 seconds
awake engine wall time: approximately 20095.1634 seconds = 5.581990 hours

512-source F5 native query probe: 0.4324510 seconds
512-source released-prefix query probe: 0.3306939 seconds
live F5 population: 96452755
query-only population extrapolations: 22.629715 / 17.304871 hours

5.581990 / (5.581990 + 22.629715) = 19.7861%
5.581990 / (5.581990 + 17.304871) = 24.3895%
```

The source probes are documented in `c6-reverse-f5-release-20260905.md` and
`c6-compatible-prefix-lookup-20260905.md`; the compatible prefix's actual
release is in `c6-reverse-prefix-release-20260905.md`. These are sample-based
**query-only** extrapolations, excluding numerical F4 reads, F5 accumulation,
audits, backup/export, final contractions and sample variability. The ratio
does not count earlier research, old-route computation or catalogue-building
time as new-route main computation. It is not an operation-count theorem,
confidence interval, measured production ETA, or guaranteed monotone progress
bar. Excluded work can lower it, and future optimization can raise it.

Accordingly, **about 20% overall** is a provisional planning shorthand, not
an independently verified whole-task fraction. Recalibrate it using a bounded
real numerical F5 pilot after the closed F4 export. Do not quote 100% F4 as
100% N(6), or turn the 17–23-hour query estimates into a promised finish time.

## Handoff boundary

STATUS and the checkpoint manifest now record the complete audited F4
namespace. No checkpoint format, arithmetic kernel or production executable
was changed. All original data, earlier backups and unrelated user files are
retained. A fresh protected F4 export plus independent readback is next;
then the separately gated bounded F5 pilot, remaining F5 work and two final
exact contractions. None was automatically launched by this audit request.
