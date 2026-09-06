# Read-only live F5 progress (2026-09-06)

Status: the owner-run F5 session remains in progress. This change adds an
advisory progress view, not a numerical counting method or a C6 certificate.

## Scope and commands

For the CURRENT running job, open a separate PowerShell terminal:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\show_f5_progress.ps1 -Watch
```

The default interval is 15 seconds. Without `-Watch` it prints once and exits.
Stopping this viewer has no process-control relationship with the existing
launcher, controller or engine. The auto-selected session has a directory
name matching `f5-manual-<32 hex digits>`; explicitly selecting `-SessionPath`
is also supported. Qualification/test folders are excluded from auto-selection.

FUTURE `run_f5.ps1` sessions get the same on-screen summary in place of the
RSS-only flood. Every original line is retained by the existing raw-log tee;
terminal and error lines still appear immediately. The native child exit
code, allowed-98 handling, final result validation and backup controls are
unchanged. Advisory display exceptions are caught without cancelling the
controller. The current running script is not hot-reloaded.

`scripts/layer_f5_progress.ps1` reads only bounded log heads/tails and read-only
process metadata. It opens no binary catalogue/checkpoint, performs no bulk
backup/hash scan, spawns no observer child, and never starts/stops a worker.
The separate CLI is an explicit owner-controlled observer, not a background
job installed by the agent.

## Meaning of progress and estimates

Only complete `CLOSED_F5_CHUNK`, `RESUMED_F5` or scope-consistent `SUMMARY`
records contribute a saved prefix. A half-written unrecognized tail is ignored.
Wrong domains, noncontiguous sampled chunks, wrong chunk geometry and invalid
prefix/scope combinations are refused. This validation is for a display:
it does NOT independently decode every stored chunk or recalculate its F5.

The denominator is 96,452,976 stable IDs, including 221 holes, not all
63,199 final classes and not an overall N(6) completion percentage.
Recent throughput is `sum(count)/sum(wall_s)` for up to 30 last complete
chunks. Estimated remaining F5 computation is `(total-prefix)/throughput`.
It excludes loading, future gates/backups, sleep and final N(6) work, and
does not assume every later ID has the same cost as the sample.

The separate child-window countdown uses the actual logged `workseconds`
and a matching live worker's start time. PID, executable name and creation
time are checked against the RSS log and stdout creation time. A heartbeat
older than 60 seconds is not presented as a fresh running-window estimate.
100% in the producer's log remains `...AUDIT_PENDING`; a completed numerical
summary before the controller's backup marker is explicitly backup-pending.

## Executed finite tests

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/test_layer_f5_progress.ps1
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/test_run_f5_launcher.ps1
```

Actual progress test markers:

```text
F5_PROGRESS_PARSER_PASS accepted=8 rejected=9 checkpoint_IO=none
F5_PROGRESS_PHASE_PASS cases=7 checkpoint_IO=none
F5_PROGRESS_NATIVE_LOGGING_PASS exits=0,98,7 display_failure_isolated=yes raw_RSS_and_terminal_preserved=yes checkpoint_IO=none
```

The parser fixtures include resumed prefixes, weighted recent rates, the
30-chunk cap, a final short chunk, a partial line, non-English numeric locale,
and malformed domain/geometry/order/scope rejection. In-memory phase fixtures
cover active computation, stale heartbeat, backup-pending at 100%, logged
closure awaiting audit, hard stop and canary/main transition.

Real finite PowerShell children deliberately returned 0, 98 and 7 while the
display reader deliberately threw an exception. Their original exits and raw
RSS/final-backup-test markers survived. They are SYNTHETIC children, not C6
controllers or backups. Retained logs:
`data/logs/f5-progress-tests-df0a5d786d0c4823b2b49aeff7e7075a/`.

The existing launcher tests also passed:

```text
F5_TIME_BOUNDS_SYNTHETIC_PASS accepted=5 rejected=7 child_450_480=accepted production_IO=none
F5_RECEIPT_SYNTHETIC_PASS accepted=1 rejected=22 production_IO=none
F5_WINDOW_SYNTHETIC_PASS accepted=3 rejected=8 bounded_stop_not_success=1 production_IO=none
```

## Actual read-only observations

At 23:07:29 the viewer read 54,800,000 IDs / 56.82%, about 1,543 IDs/s,
estimated remaining F5 compute time 7h31m, child-window time 1h04m and RSS
38.60 GiB. This was a point-in-time display, not a completed-window audit.

Five successive read-only polls at 23:11 took 0.182520 seconds total,
0.036504 seconds per poll, with a 134,893,568-byte reader peak. They read
55,140,000 committed IDs and matched worker PID 25292. No checkpoint was
opened and no production process was started. This is a local overhead
observation, not a guaranteed latency bound on all filesystems.

A bounded watch test also returned normally after two snapshots:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\show_f5_progress.ps1 -Watch -IntervalSeconds 5 -Updates 2
```

It displayed 55,150,000 IDs / 57.18% at 23:11:16 and 23:11:22 while the
same worker remained running. No monitoring process from that test remains.

## Repository gate and protected production files

The first full numerical repository gate used the existing 360-second /
6-GiB process-tree guard and `verify_all.ps1 -Threads 4`. It reached the
FJ9 full-reproduction stage after the C2--5 checks, then the aggregate time
guard stopped it. This attempt is INCOMPLETE, NOT a successful full gate;
there was no reported numerical mismatch before that stop. The retained
partial log is `data/logs/f5-progress-full-gate-20260906/stdout.log`.
Its outer error was `ValueError: aggregate 360s deadline`; the empty
`guard-result.json` from that first harness must not be accepted as evidence.
The gate's FJ9 child was cleaned up; owner-run F5 worker PID 25292 remained.

A second bounded run uses the UNCHANGED default `verify_all.ps1` command
(24 requested threads), rather than the slower four-thread override, and
retains its logs/guard result in
`data/logs/f5-progress-full-gate-20260906-retry24/`. Its verified outcome
was PASS: 255.4768223 seconds, 1,133,367,296-byte peak aggregate RSS,
119 tracked processes, exit zero, no surviving gate descendants and all
five protected files unchanged. The log ends `ALL REPOSITORY CHECKS PASSED`.
It reproduces the required C2--5 and FJ9 results, plus the bounded read-only
C6 G1 check; it does not compute N(6). No optional component of that full
repository gate was skipped.

```text
successful full-log SHA256:
94E00CDC6F13A014E312091DDBDBE8C9EAA37DB1BDC9AA60A90F04C64F2BC728
raw guard console SHA256:
0E98487FC3C7CF84AF758ED3FAF15D6788FC1E7C5078C23E4951972244EB576B
normalized receipt.json SHA256:
896C43B2DC0C8EFA9036847028CD0D674B093D44BDE9378776667EADC577B34F
incomplete first-attempt full-log SHA256:
70FD5F36E67DF7DB9A87466CA46B79CF651D9A4DE5FEBF236F6235BB898406D4
```

The retry's `guard-result.json` is retained raw console output: it begins
with `PASS stdout` before the JSON object. The separately extracted,
validated `receipt.json` is plain JSON and also pins the two original log
hashes. Neither file is an F5 checkpoint or a certificate of full C6 closure.
The display tests and successful full gate are separate evidence; the first
timed-out attempt is not upgraded to a pass.

The protected files are unchanged by this feature:

```text
shared F4: 2B8A14606F8F78748C9338130D35551CA199E53238BEB33D95BCB05A2555AFE0
reverse F5: E09123878A3098EFF955AA4DA1CCAF33B875F9CCEF7D9F337DF28F3ED08AFB9B
export reader: 5E92E26EA5D2516B7B0A37C6C6CCEF3F7C42E55EFEA1576857A3264642888961
reverse controller: 92AB945D0D572D1D70D75B1C01FAD384281ACBA1089773B94604CD2CCE3551C2
watchdog: 4FD37C0C2FA7E29A769099779C6A2A3FC8B3B9C88B3D63788BD05A236BA1BE07
```

Current checkpoint manifest facts are unchanged: the reviewed completed
session is still the earlier 30,200,000-ID prefix, and the active session's
later log progress is not promoted into an independent numerical certificate.
