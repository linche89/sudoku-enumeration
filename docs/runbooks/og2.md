# OG-2 Runbook

This is the safe workflow for exact 2xC work. Current results and bottlenecks
live in `../../STATUS.md`; dated timings live under `../reports/og2/`.

## Build

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts\build_og2.ps1
```

Built programs:

- `factorization_orbit.exe` — primary exact route.
- `mp_q.exe` — independent transfer-kernel research route.
- `ms_fast.exe`, `mp_fast.exe`, `mp_c6.exe` — independent validators.
- `canon_refine2.exe` — canonicalization validator.

## Short gate

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts\verify_og2.ps1
```

This checks C=2..4 across independent engines and runs canonicalization and
histogram differential tests.

## Primary exact gate

```powershell
.\build\factorization_orbit.exe 2 pivot rooted4
.\build\factorization_orbit.exe 3 pivot rooted4
.\build\factorization_orbit.exe 4 pivot rooted4
.\build\factorization_orbit.exe 5 pivot rooted4
```

Expected values are listed in `../../STATUS.md`. Any mismatch is a hard stop.

## Read-only C=6 gate

```powershell
.\build\factorization_orbit.exe 6 limit=1 canonbudget=100 `
  canoncachecap=300000 pivotinner parallelparents rooted4 parentchunk=128 `
  checkpoint=data\checkpoints\factorization_orbit_c6_graphmemo.bin `
  checkpointreadonly
```

Expected first value:

```text
F=6986348258918400
```

`checkpointreadonly` is mandatory for verification. It prevents a different
canonicalization budget or cache policy from rewriting an otherwise valid
checkpoint with additional equivalent key forms.

## Cold future-twin gate

The optional backend never accepts a graph checkpoint:

```powershell
.\build\factorization_orbit.exe 6 limit=1 future `
  futureorder=canonical-last futureprogress
```

Expected first value:

```text
F=6986348258918400
```

The reference run took 1592.867 seconds and peaked at 1.670 GiB. Wrap any
repeat with explicit time and RSS limits. For a bounded probe of the second
outer class, add `start=1 limit=1`; do not remove the positive limit.

## Writable C=6 experiments

Before a writable run:

1. pass `scripts/verify_all.ps1` in the same working tree;
2. verify the checkpoint SHA-256 against `data/checkpoints/MANIFEST.md`;
3. create a second physical copy;
4. use a positive `limit=` and an explicit memory/time guard;
5. save only at closed degree-5 or degree-4-parent boundaries;
6. distill results into a dated report before deleting logs.

Do not start a full 63,199-class run. The current low-symmetry frontier remains
mathematically unresolved.
