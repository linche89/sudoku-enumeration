# C=6 production S1 generation-37 preparation audit

Date: 2026-08-15 (Asia/Shanghai)

## Command and scope

After the audit-chain commits, the current recovery path was exercised with:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass `
  -File scripts\run_layer_dp_c6_s1_window.ps1 `
  -AuthorizeFullC6 -PrepareOnly -ContinueExisting
```

The tracked worktree was clean at HEAD `5e8d837`.  The unrelated user-owned
untracked directory `paper/c5-open-verification/` was ignored by the
controller and not modified.

`-PrepareOnly` cannot start the S1 engine and reaches its return point before
the progress-baseline append.  This audit therefore had no authority or path
to advance a chunk.

## Passed gates

Session 009 reran the complete repository gate.  Its retained stdout ends:

```text
== C=6 checkpoint read-only ==
...
ALL REPOSITORY CHECKS PASSED
```

The controller then:

- recognized that the existing D: receipt covers local generation 37;
- rebuilt `layer_dp_gate.exe` and the penultimate-layer counter successfully;
- found no running `layer_dp_gate` process.

The full-gate logs are retained under:

```text
data/logs/layer_dp_c6_s1_prod_20260802/session-009/
```

## Fail-closed memory result

The controller requires at least 80 GiB available RAM before even running the
target-volume resource preflight.  Available RAM initially sampled 71.875 GiB
and fluctuated below the threshold.  After the fixed ten-minute wait it was
49.223 GiB, so the controller returned:

```text
available RAM 49.223 GiB did not recover to 80 GiB within ten minutes
```

This is an environmental readiness refusal, not a numerical or recovery
failure.  The resource preflight was not reached and the S1 engine was never
started.

## State preservation and next action

No A/B image, checkpoint header, progress row, or external backup was written
or replaced.  Generation 37 remains the current image and generation 36
remains the previous local fallback.  No computation process remained after
the controller exited.

Before any future authorized S1 window, release enough memory and rerun the
same `-PrepareOnly -ContinueExisting` command.  Do not lower
`StartAvailableGB=80`: the transition resource model requires 69.898 GiB
including margin, and the higher startup threshold protects operating
headroom before allocation.
