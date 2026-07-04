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
total elapsed = 43647.1s = 12.12h
peak RSS ~= 16.33GB
raw-canon cache = 2^28 slots ~= 15.0GB
```

Per-band profile from `data/mp_q-err-20260703-213349.log`:

| band | input states | essential tasks | raw state*skel | output states | elapsed end | band time |
|---:|---:|---:|---:|---:|---:|---:|
| 0 | 1 | 1 | 252 | 7 | 3.0s | 3.0s |
| 1 | 7 | 88 | 1764 | 38801 | 98.3s | 95.3s |
| 2 | 38801 | 8458157 | 9777852 | 38801 | 39777.4s | 39679.1s |
| 3 | 38801 | 8458157 | 9777852 | 7 | 43642.7s | 3865.3s |
| 4 | 7 | 88 | 1764 | 1 | 43647.1s | 4.4s |

Notes:

- The true wall for C=5 is band 2, not band 1.
- Band 2 and band 3 have the same essential task count, but band 3 is much cheaper per task.
- The observed state-count palindrome is `7 / 38801 / 38801 / 7 / 1`.
- This run is the current local reproduction of OEIS A291187 n=5.
