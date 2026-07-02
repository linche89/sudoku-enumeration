# OG-2 Runbook

This file records the current safe workflow for the 2xC enumeration work.

## Build

From the repository root:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts\build_og2.ps1
```

This builds:

- `build/ms_fast.exe`: single-thread reference engine.
- `build/mp_fast.exe`: OpenMP engine with global raw deduplication. Fastest for C<=5, memory-heavy.
- `build/mp_c6.exe`: streaming engine with bounded memory. Slower, intended for large-C experiments.
- `build/canon_refine2.exe`: standalone refinement-canon validator.

Do not use `-march=native` on this Windows/MinGW setup.

## Short Verification Gate

Run this before trusting any later count:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts\verify_og2.ps1
```

Current gate:

- `ms_fast C=2` must print `288 [OK]`.
- `ms_fast C=3` must print `28200960 [OK]`.
- `mp_fast C=4` must print `29136487207403520 [OK]`.
- `mp_c6 C=4` must print `29136487207403520 [OK]`.
- `mp_fast C=4 difftest` must have zero histogram mismatches.
- `mp_c6 C=4 canontest` must pass refinement-key invariance and small-C separation checks.

Note: `mp_c6 canontest` checks orbit-key correctness, not equality with the lexicographic brute key.
The refinement key is intentionally a different canonical representative.

## C=5 Baseline Run

Start with a short probe so the machine does not disappear into an uncontrolled long run:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts\watch_rss.ps1 `
  -Exe .\build\mp_fast.exe `
  -Arguments 5 `
  -LimitGB 58 `
  -IntervalSeconds 5 `
  -MaxMinutes 10
```

Probe result on 2026-07-01 with `mp_fast`:

- Band 0 completed in about 1.2s with 7 states.
- A 2-minute band-1 probe reached source state 2/7, class 94, about 20.9M distinct raw states.
- RSS reached about 2.5GB by the 2-minute stop.
- This is consistent with the expected memory-heavy but feasible C=5 profile; the probe had not
  reached phase B yet, so canon throughput still needs a longer bounded probe before an unattended run.
- A 15-minute band-1 probe reached source state 4/7, class 353, about 97.1M distinct raw states.
  RSS reached about 11.2GB. This still had not reached phase B, but memory growth remained below
  the 58GB guard.
- `watch_rss.ps1` redirects child output to `data/*-out-*.log` and `data/*-err-*.log`, with RSS
  samples in `data/*-rss-*.log`.

If the probe shows normal progress and RSS stays safely below the limit, use the fast
global-dedup engine for the full run:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts\watch_rss.ps1 `
  -Exe .\build\mp_fast.exe `
  -Arguments 5 `
  -LimitGB 58 `
  -IntervalSeconds 5
```

Expected final value:

```text
C=5: N=1903816047972624930994913280000
  expect 1903816047972624930994913280000 [OK]
```

If RSS approaches the limit or the machine becomes sluggish, stop the run and use the streaming engine:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts\watch_rss.ps1 `
  -Exe .\build\mp_c6.exe `
  -Arguments 5 `
  -LimitGB 58 `
  -IntervalSeconds 5
```

`mp_c6` should use much less memory but is slower.

## C=6 Policy

Do not start a C=6 production run until:

1. The short verification gate passes in the same working tree.
2. C=5 has been reproduced with the expected value.
3. The chosen engine has usable progress logs and RSS logs.

The current multiset-state engines are correct but canon-bound. A C=6 value from this line should be
treated as a candidate until cross-checked by an independent method or a much stronger recomputation.
