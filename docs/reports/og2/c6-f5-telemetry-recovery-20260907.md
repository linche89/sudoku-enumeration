# F5 telemetry failure and authorized suffix continuation

The 2026-09-07 morning F5 worker closed prefix 93,120,000 / 96,452,976
(96.5444549891338%) at 13:14:44. It reported INCOMPLETE_RESUMABLE,
93,119,793 live values, checksum modulo 2^64 3717224009369902080,
19,748.375077 computation seconds and 45,665,333,248 peak RSS bytes.
This is not a complete F5 catalogue or N(6).

The RSS watcher failed in Add-Content with "stream was not readable". Its
finally block waited for the still-running native worker, explaining why
telemetry stopped at 09:21 while computation continued. The native engine
retained its own time/RSS guard and stopped at its soft limit. The outer
controller ran its after-backup but did not produce a normal successful
terminal marker. Do not record this as a zero-exit controller session.

At 13:26 no morning worker/controller/launcher remained. A separate bounded
read-only audit subsequently compared all 9,313 receipt entries against both
local files and their external copies: size and SHA-256 all matched in
4.445432 seconds (120-second audit limit), 747,344,128 bytes per copy.
This is byte-integrity evidence, not independent F5 numerical reevaluation.

External receipt:
`D:/sudoku_FJ_checkpoint_backups/c6_reverse_f5_20260905/20260907-074304-4048483a50df45b6874f83f26800761b-after/receipt.csv`

Receipt SHA-256:
`34D1BB91AC042E7BB949942883F1851D6B3D2FBDF5A18F1A0E2AAF288000D7BD`

Morning logs:
`data/logs/reverse-f5-window-20260907-074304-4048483a50df45b6874f83f26800761b/`
and `data/logs/f5-manual-16b313d270234202b7ce35cb93c431a0/`.

## Repair and tests

Telemetry writing changed in `scripts/watch_rss.ps1`: an explicitly
shared, write-only .NET append stream replaces Add-Content. Telemetry errors
are reported without leaving the watchdog loop before its time/RSS checks.
Counting binaries, checkpoint formats, arithmetic and production limits are unchanged.
The live subprocess test additionally exposed the existing null-exit-code
fallback: Windows PowerShell lost the process ExitCode and the old watcher
converted null to zero. The watcher now retains the live OS handle and
refuses an unreadable exit code instead of reporting success. With that
correction the actual locked-log child exit 7 is preserved.

`scripts/test_rss_telemetry.ps1` exercises concurrent-reader writing,
exclusive-lock failure isolation and recovery. Its live finite subprocess
test locks the actual watcher log and requires recovery and preservation of
child exit 7. The progress parser/phase/native-exit tests and launcher
parameter/receipt/terminal tests also pass. The first live test attempt used
an unresolved executable name and failed before starting its synthetic child;
the corrected test uses the absolute Windows PowerShell executable path.

Final telemetry test artifacts:
`data/logs/rss-telemetry-test-4cdff3bec9d948ac99a4827e595db24b/`.
Its three terminal markers are RSS_TELEMETRY_PASS, RSS_TELEMETRY_LIVE_PASS
(actual locked-file child exit 7), and RSS_TELEMETRY_GUARD_PASS (actual
synthetic memory-limit stop 99). No production checkpoint is used by these
tests. The temporary zero-GiB test limit is confined to a sleeping synthetic
PowerShell child; the production limit stays 55 GiB.
Functional repair commit: `28b9343`.

## Authorized continuation

The owner explicitly requested completing the remaining 3,332,976 IDs.
The hidden launcher uses `run_f5.ps1 -WorkMinutes 75 -MaxMinutes 90`, with
unchanged 24 threads, 55-GiB native bound, immutable chunk resume and physical
backups. It refreshes all four bounded exact gates before its canary and
regular window. It does not export L5 or run the final N(6) stage.

An initial hidden launch at 13:30:49 failed in read-only preflight because
Start-Process inherited PowerShell 7 module paths into Windows PowerShell.
No production work started. The replacement launch at 13:31:46 explicitly
sets only its child environment to the Windows PowerShell module paths.
Launcher PID at start: 16364.

Retained launcher logs: `data/logs/f5-suffix-launch-20260907-133146/`.
Manual session: `data/logs/f5-manual-2b70307b07e6446e91130b9b4b076534/`.
At launch, gates and numerical continuation were pending; a launcher PID is
not proof of a completed gate, completed suffix, or N(6).

All four bounded gates subsequently passed with exit zero and no surviving
gate children: full repository 173.4014872 seconds / 1,134,977,024 peak bytes;
direct 184.0458238 seconds / 181,293,056 bytes; shared 23.2238660 seconds /
174,637,056 bytes; reverse 32.9924287 seconds / 171,294,720 bytes. Each gate
had its own 360-second / 6-GiB aggregate process-tree bound. The three
protected production executables retained their pinned hashes.
Machine-readable evidence is the session's `gates/receipt.json`.
The canary controller was entered after these gates, not before them.
