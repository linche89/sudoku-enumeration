# Layer-DP takeover and checkpoint gate

Date: 2026-07-30

## Scope

This audit took over the incomplete working-tree continuation of
`experiments/proto/layer_dp_gate.cpp` after the Fable commits ending at
`20437f9`.  It did not start a full C=6 count and did not read, write, move, or
replace the active factorization checkpoint.

Before editing, the uncommitted source and its patch against `HEAD` were
copied to:

```text
C:\Users\CHOPPE~1\AppData\Local\Temp\
  sudoku-layerdp-takeover-20260730-232056
```

The source SHA-256 was
`1E80E5AFD13AD4B6B89D45BBBD2B6000B33B4EB787F054B8E5C07EC8A4BEF444`;
the saved patch SHA-256 was
`2B043D594C45FD401DB0410B5205DC4AD3ACABA55CD74101996546D3F2CFFE9A`.

## Audit findings

The committed engine already contained the corrected global
row-incremental recurrence and the measured C=6 scale probes.  The
working-tree continuation contained a useful `(C-1)`-row anchored
canonicalizer and a first checkpoint implementation, but the operational
handoff was not complete:

- the raw expert build command still used the workstation's unsafe
  `-march=native`;
- the checkpoint image did not bind itself to key-affecting runtime modes or
  reject trailing/truncated bytes by exact length;
- checkpoint and snapshot write failures could leave a run continuing
  without the promised recovery guarantee;
- a completed large checkpoint was rewritten as a separate layer snapshot;
- stale bases, mismatched resume bases, and accidental plain C=6 launch were
  insufficiently guarded;
- the anchor had total/invariance evidence but no explicit separation and
  histogram differential in this engine;
- the C=6 G1/G2 class-key bridge had not been wired into the final acceptance
  path.

## Retained implementation

The audited working tree now has:

- repository-safe build and verification scripts;
- checkpoint format v2 with an algorithm/configuration fingerprint, strict
  header sanity, exact file length, header hash, payload hash, parent-key
  hash, and alternating atomic generations;
- fail-stop handling for checkpoint and snapshot write failures;
- exact parent-snapshot loading and table rebuilding on resume;
- atomic hard-link promotion of a completed checkpoint to `.Lk.snap`, with a
  checked full-write fallback;
- stale-base refusal and exact CLI-mode validation;
- an explicit C=6 safety gate (`--bridge-only`, bounded probes, and
  `--ack-full-c6` for large stages);
- G1/G2 native complete-state keys, exact stabilizers 120 and 8, final
  per-class value assertions, and the 63,199-class assertion;
- full-group orbit/stabilizer checks plus brute partition separation and
  `(T,stab)` histogram differential.

The checkpoint algorithm tag is `LDPCAN01`; it must change whenever the state
representation, canonical key, or transition arithmetic changes
incompatibly.

## Exact gate results

The direct C=5 gate reported:

```text
states    = 1, 107, 16150, 17120, 355
emissions = 440192, 2324325, 11957632, 462403
N(5)      = 1903816047972624930994913280000
reference triples = 355/355
invariance failures = 0/2000
scan failures       = 0/1000
separation: fast=708, brute=708, conflicts=0
histogram = MATCH
```

The 355-row reference fixture is pinned by SHA-256
`D2FDEB354ED4C1443E9870B5727CE35C88BA6B392C6DAA32CD92D2601BCDB1F5`;
the gate refuses a modified raw-expert copy.  Its per-class agreement with
the primary factorization route was already established by the retained
C=5 differential evidence.

The full reusable gate was:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass `
  -File scripts\verify_layer_dp.ps1 -Threads 8 -KillIterations 20 `
  -KeepArtifacts
```

It completed in 75.4 seconds and passed:

- C=2..5 exactness and canonicalization checks;
- all C=5 class triples and external `S4` exact summation;
- snapshot SHA-256 round trip;
- 20 deterministic-random process kills followed by exact resume;
- forced-`u128` parity and a killed final-transition resume;
- checkpoint-mode mismatch refusal;
- corrupt-newest-generation fallback to the older generation;
- C=6 G1/G2 bridge and unbounded-C=6 refusal.

A subsequent two-kill run after adding stale-base and write-failure tests
also passed those two new fail-stop cases.  The retained artifacts from the
20-kill run were under:

```text
C:\Users\ChopperLin\AppData\Local\Temp\
  sudoku-layer-dp-gate-8ac2f99b201141c98f26c2c81d14f229
```

`fsutil hardlink list` showed the source layer-3 snapshot shared with all 20
self-contained checkpoint directories and the wide layer-4 snapshot shared
with its resume directory.  The corrupt-generation fallback then loaded a
copied `.L4.snap` after later A/B rotation and reproduced the exact class
dump.

The independent read-only factorization inspection:

```powershell
.\build\factorization_orbit.exe 6 inspect start=0 limit=2 checkpointreadonly
```

reported outer orbit sizes 384 and 5,760.  With
`|G_6| = 46,080`, these independently imply stabilizers 120 and 8, matching
the two bridged native keys.

## Repository integration and bounded C=6 smoke

`scripts/verify_all.ps1` now calls the two-kill layer-DP gate.  Both that
script and the dedicated gate use a local .NET SHA-256 helper so verification
does not depend on PowerShell module auto-loading.  The complete repository
gate finished with:

```text
LAYER-DP CHECKPOINT AND EXACTNESS GATES PASSED
factorization C=2..5 [OK]
FJ9 full reproduction [OK]
FJ9 independent combinatorial route [OK]
C=6 checkpoint read-only [OK]
ALL REPOSITORY CHECKS PASSED
```

The active factorization checkpoint remained
217,954,462 bytes with SHA-256
`FE8B68DE6C15592848D7CF69BF43928C000F0B2EE59FC263A094BCA2D376A865`.

After that full gate, the first bounded C=6 stage was repeated under an
external 2 GiB / 5 minute guard:

```powershell
& .\scripts\watch_rss.ps1 `
  -Exe .\build\layer_dp_gate.exe `
  -Arguments @(
    '6','--stop-after','2','--threads','8',
    '--caps','2000,14000000,1,1,1') `
  -LimitGB 2 -MaxMinutes 5 -IntervalSeconds 1
```

It stopped before 2-to-3 and reported:

```text
1->2 emissions = 59,245,120
layer-2 states = 772
layer-2 orbit mass = 20,338,525 [OK]
wall = 123.334 s
peak RSS = 8,331,264 bytes
```

No C=6 layer file or checkpoint was created.

## Decision

Checkpoint/restart and the `(C-1)`-row anchor are retained as verified C=5
components of the global layer-DP candidate.  This closes a real part of the
preflight program, but it does not authorize or establish a full C=6 result.

Still open are the stronger `M_4` capacity measurement, unbiased 4-to-5
calibration, explicit production resource preflight, and the bounded staged
rehearsal.  Resume also has a transient child-image allocation in addition
to the target fixed-capacity layer; production RAM planning must include that
peak rather than quoting only steady-state dictionary size.
