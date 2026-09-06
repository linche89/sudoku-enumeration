# First manual F5 session and optional longer suffix window (2026-09-06)

Status: the complete native F4 export now exists and the first manual F5
session closed 30,200,000 IDs. F5 is incomplete; N(6) is not computed.
The owner has started another ordinary session. This change prepares the
FOLLOWING session, without stopping/restarting the active one.

## Reviewed completed session

Owner command:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\run_f5.ps1
```

Retained session directory:
`data/logs/f5-manual-a0d9e35d7f2d44bbb309864537ae243f/`.

The export controller completed at 12:16, with zero new shared chunks/IDs,
independent full SHA/header/payload/count/mass readback and a separate-volume
physical backup. Its new native F4 snapshot has 903,398,621 IDs,
903,398,603 live states, 18 holes, orbit mass 41,602,261,536,160 and
32,522,350,484 bytes. Its SHA-256 is
`7BA5E5BA3255DD17851043521F67FB4EE70F76AE565FD6CA9AD962E7D5014D94`.
The source-bound receipt is
`data/checkpoints/c6_shared_f4_closed_20260906.L4.snap.receipt.json`, SHA-256
`6A5236B3E8447FE5F66C4A36A84BB8F557DDDDB435A2C55F897024B8E58A2B9D`.
The checkpoint manifest records both physical paths. This is not a cold
independent recalculation of every numerical F4 value.

A 10,000-ID canary preceded the regular session, whose retained stdout is
`data/logs/reverse-f5-window-20260906-121946-99b4c5514bd943f1902d18e740ce48f1/stdout.log`.
Its exact terminal record was:

```text
SUMMARY status=INCOMPLETE_RESUMABLE new_chunks=3019 new_indices=30190000 closed_prefix=30200000 total_entries=96452976 live_closed=30199969 F5_checksum_mod2_64=1205041302659950080 compute_wall_s=19763.068581 total_wall_s=19802.385721 peak_rss_bytes=45665304576 source_checkpointreadonly=yes N6=NOT_COMPUTED
```

Both controller and engine exited zero, engine stderr was empty, and the
after-backup finished at 17:50. This is a successful 330-minute soft-time
window, NOT full F5 closure. The checkpoint contains 3,020 immutable chunks
and one manifest, 242,373,376 bytes per copy. Its external after-backup is
`D:/sudoku_FJ_checkpoint_backups/c6_reverse_f5_20260905/20260906-121946-99b4c5514bd943f1902d18e740ce48f1-after/`.
The receipt SHA-256 is
`0363AD06245CECAA5D973B93C43A8DEA26BFEFD1C4E909B0C45B4013F4D52D2E`.

At the idle status review, a separate read-only PowerShell check compared
all 3,021 source and backup sizes and SHA-256 values to the receipt, checked
unique receipt names and contiguous 10,000-ID chunk filenames, and passed
in 2.0070931 seconds with a 50-second bound. The 3,019 regular-window chunk
log intervals were independently parsed as contiguous after the canary.
This checks byte preservation and recorded progress, not independent F5
payload decoding or numerical reevaluation. Later live chunks must not be
deleted or rolled back to this historical reviewed prefix.

## Longer next-session command

After the CURRENT ordinary session and its after-backup have finished:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\run_f5.ps1 -WorkMinutes 450 -MaxMinutes 480
```

Only the entry script's maximum accepted parameter values change:
`WorkMinutes: 330 -> 450`, `MaxMinutes: 360 -> 480`. The DEFAULT values
remain 330/360. The underlying controller already accepted this longer
window and is unchanged. There is no new loop, retry policy, production
launch, checkpoint-format change or arithmetic change.

The requested window allows 7.5 hours before stopping new chunks and an
8-hour hard computing-child limit. It preserves the resumed 10,000-ID canary,
24 threads, 55-GiB limit, exact chunk checks, positive work limit, exclusive
launcher lock and before/after physical backups. Full F5 closure stops it
early, including a final short chunk; it does not wait out the time limit.
Gates, input hashing and backups are additional wall time. Keep the machine
awake. No system power policy is changed, and no automatic post-wake relaunch
or unconditional completion guarantee is added.

The observed compute throughput was 1,527.596784 IDs/s. A linear extrapolation
gives 17.53898 hours for all F5, or about 6.5--7.0 computing hours for a
remaining 37%--40%. This is not a uniform-work proof, an end-to-end N(6)
runtime bound or a guarantee that the long window will finish. The ordinary
protected stop/resume behavior still applies if it reaches a guard.

## Executed script qualification

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/test_run_f5_launcher.ps1
```

Actual terminal test markers (the 100% fixtures are synthetic, NOT C6 data):

```text
F5_TIME_BOUNDS_SYNTHETIC_PASS accepted=5 rejected=7 child_450_480=accepted production_IO=none
F5_RECEIPT_SYNTHETIC_PASS accepted=1 rejected=22 production_IO=none
F5_WINDOW_SYNTHETIC_PASS accepted=3 rejected=8 bounded_stop_not_success=1 production_IO=none
```

The new tests execute only extracted real parameter blocks and the existing
time-ordering guard. They check defaults, long and short bounds, rejection of
out-of-range/inconsistent times, and acceptance by the unchanged underlying
controller. Terminal fixtures additionally cover final 2,976-ID completion
and zero-new-work reopening of an already complete catalogue. They never
execute either script's production main body.

An actual read-only duplicate-launch test while the owner's launcher held
its mutex used:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\run_f5.ps1 -CheckOnly -WorkMinutes 450 -MaxMinutes 480
```

It exited 1 with `Another run_f5 launcher holds the session lock.` This is
the EXPECTED refusal, not a full preflight pass. No second engine was started.

The owner's second session, started at 18:28:35 (launcher PID 52948), retained
four fresh successful gates in
`data/logs/f5-manual-560543e769a241399a115b102cf5d948/gates/`:

| Gate | Seconds | Peak aggregate bytes | Exit |
|---|---:|---:|---:|
| full | 191.8480415 | 1,134,718,976 | 0 |
| direct | 212.1942641 | 186,449,920 | 0 |
| shared | 26.5929003 | 171,708,416 | 0 |
| reverse | 36.1941838 | 172,478,464 | 0 |

Each used the existing 360-second / 6-GiB process-tree bound and left no
surviving gate child. All four log SHA values and success markers were
checked against their receipt, whose SHA-256 is
`0CEDD40FED28C0766C1C3C64315DAD666F996F6A3D46E4632740AEDFD74F23FE`.
These gates were launched by the OWNER'S existing session, not by this edit.
No duplicate full numerical gate was launched alongside it for this
parameter-only change. Every future manual invocation still runs fresh gates.

During handoff the owner queried the apparent exit between stages. The second
session's canary log
`data/logs/reverse-f5-window-20260906-183731-b5d2bc7795974b76af0a30b5867cc31c/stdout.log`
confirms one new 10,000-ID chunk, normal exit and prefix 30,210,000. Its
controller then automatically started the ordinary window. A process query
at 18:41:49 confirmed worker PID 25292, created at 18:41:16, under controller
PID 40600 and original launcher PID 52948. Both the command and the new
controller log retain `WorkMinutes=330`, `MaxMinutes=360`
(`workseconds=19800`, `maxseconds=21595` in the engine). Its log directory is
`data/logs/reverse-f5-window-20260906-184034-5c2daf1985d44d4a88c35be9f4e5f06e/`.
Thus the canary's exit was a planned stage transition, not interruption by
the parameter edit. The ordinary window remains running, not certified done.

The protected executable hashes remain:

```text
shared F4: 2B8A14606F8F78748C9338130D35551CA199E53238BEB33D95BCB05A2555AFE0
reverse F5: E09123878A3098EFF955AA4DA1CCAF33B875F9CCEF7D9F337DF28F3ED08AFB9B
export reader: 5E92E26EA5D2516B7B0A37C6C6CCEF3F7C42E55EFEA1576857A3264642888961
unchanged reverse controller: 92AB945D0D572D1D70D75B1C01FAD384281ACBA1089773B94604CD2CCE3551C2
```

No production checkpoint was written by this change. The owner-run session
is intentionally left in place. F5 closure still requires its final prefix
and backup checks; independent audit, native L5 export and both final N(6)
contractions remain separate downstream work.
