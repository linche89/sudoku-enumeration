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
