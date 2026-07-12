# Repository Operating Rules

These instructions apply to the entire repository.

## 1. Read order and authority

Before changing anything, read:

1. `STATUS.md` — the only authoritative current state;
2. `docs/index.md` — documentation and provenance map;
3. `src/README.md` — active versus legacy code;
4. `docs/runbooks/og2.md` for 2xC work;
5. `data/checkpoints/MANIFEST.md` before touching C=6 state.

Authority order:

```text
exact tests and current code
  > STATUS.md / verified method notes
  > dated reports
  > raw expert responses
  > archived handoffs and research logs
```

Raw files under `docs/expert/` are research inputs, not established facts.
Files under `docs/history/` may contain obsolete commands and conclusions.

## 2. Preserve user work

- Always inspect `git status --short` before edits.
- Existing changes belong to the user unless proven otherwise.
- Do not mix an algorithm change with a directory-only reorganization.
- Stage explicit paths; never use broad staging when unrelated work exists.
- Before mass moves or cleanup, create a Git safety tag and protect any large
  ignored checkpoint with a second physical copy plus SHA-256.

## 3. Correctness is the primary constraint

The mandatory known values are:

```text
FJ9 N0 = 6670903752021072936960
N(2)   = 288
N(3)   = 28200960
N(4)   = 29136487207403520
N(5)   = 1903816047972624930994913280000
F6(G1) = 6986348258918400
```

Run the complete gate after structural changes and before handoff:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts\verify_all.ps1
```

For a small inner loop:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts\verify_og2.ps1
```

Additional requirements:

- Canonicalization changes require invariance, separation, and histogram
  differential tests, not only final totals.
- Factorization changes must pass complete C=2..5 gates.
- FJ9 changes must reproduce all 71 reference classes, `jobs2.txt`, and
  `results2.txt`.
- Checkpoint-format changes require a round-trip test on a disposable copy.
- A refactor is not complete if it only compiles; numerical gates must pass.

## 4. C=6 safety policy

- The full 63,199-class count is not authorized by an ordinary development
  request. Use a positive `limit=` for bounded experiments.
- Every writable run needs explicit time and memory bounds, a recent full gate,
  and an external checkpoint backup.
- Verification must use `checkpointreadonly`.
- Never delete, overwrite, or relocate the active checkpoint without checking
  `data/checkpoints/MANIFEST.md` and verifying SHA-256 before and after.
- Only closed exact memo values may be saved. Never trust a partial accumulator.
- Do not infer global C=6 feasibility from the highly symmetric first outer
  class; the second graph has a much larger degree-5 frontier.

## 5. Data and documentation lifecycle

- `data/golden/`: small tracked fixtures and retained outputs.
- `data/checkpoints/`: ignored binary state; tracked manifest required.
- `data/logs/`: ignored transient logs.
- `docs/reports/`: immutable dated evidence.
- `docs/methods/` and `docs/math/`: audited current conclusions.
- `docs/expert/`: verbatim external analysis with no automatic authority.
- `docs/history/`: archival context only.

Before deleting logs, distill all important commands, timings, values, and
decisions into a dated report. Large generated files must not enter ordinary
Git history.

Update `STATUS.md` only when a conclusion is verified. Do not turn a conjecture
or expert suggestion into current status merely because it sounds plausible.

## 6. Code organization

- Active programs stay in `src/`; superseded experiments stay under
  `experiments/legacy/` or `experiments/proto/`.
- Standard builds must not depend on legacy files.
- Promote legacy code only with an independent exact gate.
- Prefer small exact helpers and explicit invariants over framework-heavy
  abstractions in hot combinatorial kernels.
- Preserve checkpoint compatibility unless a versioned migration is part of
  the task.
- On Windows/MinGW, do not add `-march=native`; the known-safe build flags avoid
  the AVX stack-alignment failure documented by the project.

## 7. Commit and handoff discipline

Prefer separate commits for:

1. functional algorithm changes;
2. pure moves/renames;
3. documentation/status updates;
4. generated-data manifest changes.

Before finishing:

- run `git diff --check`;
- run the proportional verification gate;
- confirm no unintended long-running process remains;
- update `STATUS.md`, the relevant report, and checkpoint manifest if facts
  changed;
- report exact commands, results, commits, and any skipped optional gate.

Do not claim that C=6 is solved unless the complete outer sum is independently
closed and verified. The current exact achievement is the first outer class,
not the full `N(6)`.
