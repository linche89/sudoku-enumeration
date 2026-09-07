# Data Policy

- `golden/` contains small, Git-tracked verification fixtures and retained
  outputs.
- `checkpoints/` contains large generated binary state. Binary files are
  ignored; `MANIFEST.md` is tracked.
- `logs/` is transient and ignored. Important measurements must be distilled
  into `docs/reports/` before logs are removed.

Do not commit generated checkpoints or bulk logs to ordinary Git. Preserve a
second physical copy of any checkpoint that represents unrecoverable compute,
and verify it with SHA-256.

The complete 63,199-class C6 final table and standalone verification programs
are distributed in the
[C6 certificate release](https://github.com/linche89/sudoku-enumeration/releases/tag/c6-verified-2026-09-07),
not as a bulk Git blob. The archive contains no large intermediate checkpoint.
See [the reproducibility guide](../docs/reproducibility.md) for its scope and
commands, and [the checkpoint manifest](checkpoints/MANIFEST.md) for the
unchanged local production artifacts and the public-package identity.
