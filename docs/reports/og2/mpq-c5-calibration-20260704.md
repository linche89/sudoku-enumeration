# mp_q C=5 calibration, 2026-07-04

Command:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts\watch_rss.ps1 -Exe .\build\mp_q.exe -Arguments 5 -LimitGB 110 -IntervalSeconds 30
```

Result:

```text
C=5: N=1903816047972624930994913280000
  expect 1903816047972624930994913280000 [OK]
```

Runtime:

```text
logged elapsed = 43647.1s = 12.12h
system sleep/hibernate interval = 2026-07-04 01:22:54 +08:00 .. 08:03:38 +08:00
sleep/hibernate duration = 24044.185s = 6.679h
active elapsed = 19602.9s = 5.45h
peak RSS ~= 16.33GB
raw-canon cache = 2^28 slots ~= 15.0GB
```

The sleep/hibernate interval came from Windows System events:

- `Microsoft-Windows-Kernel-Power` event 42 at 2026-07-04 01:22:56, sleep reason `Application API`.
- `Microsoft-Windows-Power-Troubleshooter` event 1 at 2026-07-04 08:03:39, sleep time `2026-07-03T17:22:54.033918600Z`, wake time `2026-07-04T00:03:38.218622000Z`.
- `data/mp_q-rss-20260703-213349.log` has a matching sampling gap from `2026-07-04T01:22:46.4301948+08:00` to `2026-07-04T08:03:53.2279186+08:00`.

Per-band profile from `data/mp_q-err-20260703-213349.log`, corrected for that sleep/hibernate interval:

| band | input states | essential tasks | raw state*skel | output states | active elapsed end | active band time | share | essential/s |
|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| 0 | 1 | 1 | 252 | 7 | 3.0s | 3.0s | 0.0% | |
| 1 | 7 | 88 | 1764 | 38801 | 98.3s | 95.3s | 0.5% | |
| 2 | 38801 | 8458157 | 9777852 | 38801 | 15733.2s | 15634.9s | 79.8% | 541.0 |
| 3 | 38801 | 8458157 | 9777852 | 7 | 19598.5s | 3865.3s | 19.7% | 2188.2 |
| 4 | 7 | 88 | 1764 | 1 | 19602.9s | 4.4s | 0.0% | |

Notes:

- The true wall for C=5 is band 2, not band 1.
- The earlier uncorrected 12.12h / 39679.1s band-2 figure included 6.679h of system sleep/hibernate.
- Band 2 and band 3 have the same essential task count, but band 3 is about 4.0x cheaper per task after sleep correction.
- The apparent late band-2 slowdown around `ess 7602176` was an artifact of the sleep/hibernate interval, not a sideMemo collapse.
- In C=5, dual mode can skip band 3 and band 4.  On this corrected profile that is about a 19.7% active-time saving, not the earlier 8-9% estimate.
- The observed state-count palindrome is `7 / 38801 / 38801 / 7 / 1`.
- This run is the current local reproduction of OEIS A291187 n=5.
