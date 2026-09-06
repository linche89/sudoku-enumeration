# Same-command manual F5 continuation — 2026-09-06

The local launcher is prepared and verified in read-only/synthetic modes.
No C6 native F4 export, numerical F5 pilot or long production window was
started by the agent. The complete F4 audit remains the verified current
achievement; N(6) remains uncomputed.

## Owner request and actual behavior

After the full F4 audit, the owner requested a PowerShell script they could
run locally without continuous agent monitoring, then clarified that the
same script must resume across sessions and workstation sleep. The helper is
`run_f5.ps1` at the repository root:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\run_f5.ps1
```

This is a same-command, **manually restarted** continuation, not a scheduled
task or an unbounded auto-restart service. It performs:

1. Single-session/process checks, pinned complete F4 audit/backup/build
   checks and disk/required-file preflight. `-CheckOnly` stops here without
   gates, export, production work or backup creation.
2. Fresh full, direct, shared and reverse gates via
   `scripts/prepare_layer_f5_manual_gates.py`. Each has a separate
   360-second/6-GiB aggregate process-tree guard. The three specialized gates
   use `-SkipBuild`; the qualified production binaries are SHA-pinned before
   and after each gate.
3. The existing protected `scripts/export_layer_shared.ps1` if no valid
   complete F4 export receipt exists. It holds read/delete protections on
   source and chunk files, requires zero new counting, independently reads
   back the native export and makes a separate-volume physical copy. The
   helper obtains the future SHA from this receipt, never from the original
   partial generation-37 weights.
4. A 10,000-additional-ID F5 canary through the existing controller, with
   24 threads, 55 GiB, an 8-minute soft and 10-minute hard child limit.
   On subsequent invocations this is itself a resumed canary, not a restart
   of the first 10,000 IDs. The native engine checks every old header/payload,
   input/repair binding and value convention before accepting any old value.
5. After exact prefix/scope, zero-exit, empty-stderr, resource and backup
   checks, one regular F5 window. Defaults are `WorkMinutes=330`,
   `MaxMinutes=360`, 24 threads, 55 GiB, `chunk=10000` and positive additional
   limit 96,452,976. The engine stops at the domain end or soft deadline.
6. The controller copies committed chunks with source-before/copy/source-after
   SHA checks. The wrapper displays the F5 ID completion fraction and stops.

Rerun the same command on the next computing day. Paths and cursor do not
need editing. A shorter regular window can be requested with
`-WorkMinutes 45 -MaxMinutes 60`. Input hashing, backup, verification and
export overhead are additional to the regular computing-child bound; the
default leaves headroom within the owner's approximately eight-hour daily
availability, but does not make an absolute whole-session deadline promise.

If sleep causes a time-bound exit 98 on wake, the helper checks for the
controller's terminal after-backup evidence and stops without calling that
window successful. It tells the owner to rerun the SAME command, at which
point the native engine validates/resumes committed chunks. It neither
automatically relaunches on wake nor retries time/RSS failures indefinitely.
Only an uncommitted chunk must be recomputed. The first F4 export is NOT a
resumable chunk operation: interrupted exports without valid receipts are
preserved for review, not automatically deleted, overwritten or promoted.

At complete F5 closure the script stops. Native L5 export, independent full
F5 audit and final independent N(6) contractions are not silently launched.

## Fixed inputs and prospective outputs

All current F4 pins and physical copies are in
`c6-shared-f4-complete-audit-20260906.md` and the checkpoint manifest.
The proposed first export paths, **not yet existing production outputs**, are:

```text
E:/Code/sudoku_FJ/data/checkpoints/c6_shared_f4_closed_20260906.L4.snap
D:/sudoku_FJ_checkpoint_backups/c6_shared_f4_exports/c6_shared_f4_closed_20260906.L4.snap
```

Its local `.receipt.json` must bind source SHA, manifest, both namespaces,
exact entry/live/hole/mass counts, 32,522,350,484-byte length, production and
reader executable SHA, zero newly computed F4 IDs and independent readback.
Both full output hashes are rechecked by the F5 controller before computing.

The F5 chunk namespace remains the reserved
`data/checkpoints/c6_reverse_f5_20260905/`, backed up under the corresponding
directory on D:. Only closed immutable values are accepted. All original
checkpoints, earlier copies and native formats are unchanged.

## Executed verification

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\run_f5.ps1 -CheckOnly
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/test_run_f5_launcher.ps1
python scripts/prepare_layer_f5_manual_gates.py --output-dir data/logs/f5-manual-launcher-qualification-20260906
```

The live read-only preflight passed under Windows PowerShell 5.1. It creates
no persistent files or production processes. AST parsing and separate
mocked active-worker/controller refusal checks passed. The selected Python
interpreter imports `psutil` successfully. Multiple installed `python.exe`
paths are resolved to the FIRST command, not accidentally concatenated.

Synthetic receipt tests accepted one valid fixture and rejected 22 changed
field/length cases. Synthetic window tests accepted a 10,000-ID result and
rejected eight altered domain/prefix/resource/backup/scope cases. A synthetic
exit-98 window returned no successful result and would not advance to the
regular window. These fixtures do not access production files and do not
claim that real F5 work was executed.

All four actual gate wrappers passed:

| Gate | Wall seconds | Sampled aggregate peak bytes | Tracked processes |
| --- | ---: | ---: | ---: |
| Full repository | 183.525693 | 1,130,708,992 | 114 |
| Direct layer | 200.097378 | 180,912,128 | 42 |
| Shared F4 | 22.997036 | 170,565,632 | 12 |
| Reverse F5 | 32.416516 | 171,249,664 | 15 |

Each had a 360-second/6-GiB aggregate guard. Every process exited zero;
none required termination and none survived. These include complete small-C
exactness, known totals, checkpoint corruption/kill/resume gates, native
export comparisons, and the bounded G1 read-only checkpoint check. No full
C6 numerical F5 execution is implied by these small-case checks.

Fresh gate receipt:
`data/logs/f5-manual-launcher-qualification-20260906/receipt.json`

```text
receipt SHA256:
B17D5ABE5D8FED282643033A397423B1051C8C2262C9217BD773EFB54BC91DF8
unchanged shared F4 executable SHA256:
2B8A14606F8F78748C9338130D35551CA199E53238BEB33D95BCB05A2555AFE0
unchanged reverse F5 executable SHA256:
E09123878A3098EFF955AA4DA1CCAF33B875F9CCEF7D9F337DF28F3ED08AFB9B
unchanged independent export reader SHA256:
5E92E26EA5D2516B7B0A37C6C6CCEF3F7C42E55EFEA1576857A3264642888961
```

The prepared launcher does not modify algorithms or checkpoint formats.
Its full production C6 export/canary/regular-window sequence has not been
executed here. `git diff --check` and final no-production-process checks
passed. Unrelated user files were preserved. Overall progress remains F4
100%, numerical new-route F5 0%, with about 20% only a provisional
new-route computation-time planning estimate, not an exact whole-task fraction.
