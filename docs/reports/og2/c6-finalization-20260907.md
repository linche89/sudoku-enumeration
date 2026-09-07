# C6 complete F5 audit, export, and finalization

The owner explicitly authorized independent integrity audit, L5 export and
the complete exact N(6) finalization on 2026-09-07. No final N(6) is claimed
until the full two-run certificate and independent sums below are closed.

## Complete F5 integrity

The final native worker closed all 96,452,976 stable IDs, including 221 holes,
with 96,452,755 live F5 values. It reported CLOSED_F5_CATALOGUE, checksum
modulo 2^64 3850733904537200640, zero exit and empty stderr. The after-backup
finished at 14:14:30. All 9,647 files are physically retained under:
`D:/sudoku_FJ_checkpoint_backups/c6_reverse_f5_20260905/20260907-134453-dfdbbc71d43147de82f6de62f1e5a01b-after/`.

The independent Python reader `experiments/proto/layer_reverse_complete_audit.py`
checked every chunk header and payload using the separately implemented
Python checksum, exact contiguous inventory, both input header/SHA lineages,
support repair identity, every live/hole position, positive values divisible
by 120, all local/whole checksums, and byte/SHA equality with the external
receipt. It read the original support's full SHA and stabilizer array,
appending exactly the two previously proved missing IDs/stabilizers.
No native producer reader or factorization calculation was invoked.

Result: PASS in 23.2662485 seconds, sampled peak RSS 32,276,480 bytes under
180 seconds / 1 GiB. Bytes per physical copy: 774,093,440. The finite parser
test accepted one correct fixture and rejected 13 mutations/support errors.
This proves complete storage/lineage/value-shape integrity, NOT independent
numerical reevaluation of all F5 values.

Evidence: `data/logs/c6-finalization-20260907/f5-audit.json` and `.log`.
Audit JSON SHA: `B781B8EB2ADE49FC74A872443F604F16716F2341DA4B5252CDF3D06F18976D91`.
Manifest SHA: `22E81759527CD4CA2309138F0EB9930094617598FD76492882C069D48D117E30`.
Backup receipt SHA: `3BEB4A48D98F275F5F17300B2A10743039BEB6F13EA23AFC31F0F9FB6809CBB6`.

## Protected export

The existing reverse controller was invoked with the 2026-09-07 fresh
full/reverse gates, limit=10000, chunk=10000, 24 threads, 55-GiB native bound,
12-minute soft / 15-minute hard child bound and a new output path:
`data/checkpoints/c6_reverse_f5_closed_20260907.L5.snap`.
An external process-tree guard additionally allows at most 1,200 seconds and
60 GiB including input hashing and before/after backup. The previous complete
F5 physical copy was independently audited before this run.

The producer resumed all 9,646 chunks and performed ZERO new chunks/indices.
It exported all weights, reread all keys/stabilizers/weights with the native
reader, and reported a complete catalogue with zero exit. Native elapsed was
45.564395 seconds, peak 45,665,345,536 bytes. Export SHA:
`4CF50FAD4F7C5DB020D30DEF258CE06AEF63125352E351D331A07BE8E173A6CF`.
Export physical backup and independent weighted readback remain separate
requirements; the producer's marker alone does not complete finalization.

Export controller logs:
`data/logs/c6-finalization-20260907/export/` and
`data/logs/reverse-f5-window-20260907-142458-60c5a69f99fc4f98af862fa474975b7c/`.
Exact command and external guard bounds/exit are retained in each guarded
step's `receipt.json`; no production checkpoint is overwritten.

The export controller subsequently completed both the chunk after-backup
and the separate-volume native L5 copy, with REVERSE_WINDOW_END exit=0.
The complete externally guarded export took 335.6324574 seconds and peaked
at 45,929,070,592 aggregate bytes; no tracked child survived.
Native L5 backup:
`D:/sudoku_FJ_checkpoint_backups/c6_reverse_f5_20260905/20260907-142458-60c5a69f99fc4f98af862fa474975b7c-closed.L5.snap`.
Local and backup lengths are both 3,472,307,264 bytes with the SHA above.

Two independent readbacks passed:

- `layer_support_probe --checkpointreadonly` streamed the full native image,
  independently reproduced SHA/header/payload hashes and the exact orbit
  mass 4,439,972,139,072 in 2.4436 seconds (11,649,024 native peak bytes).
- `layer_reverse_export_audit.py` checked both full SHAs, all original key
  bytes against the immutable support, all 96,452,976 stabilizers and weighted
  T5 values against the independently audited chunks, every hole, and both
  appended stabilizers. It passed in 45.0512073 seconds, sampled peak
  39,215,104 bytes, under 180 seconds / 1 GiB (external 210 seconds / 2 GiB).

Artifacts: `native-export-readback/`, `weighted-export-readback/` and
`l5-export-audit.json` under `data/logs/c6-finalization-20260907/`.
The weighted readback proves serialization/normalization against the closed
F5 values, not a second calculation of their mathematical values.

## Final controller qualification

The complete disposable C5 lineage test passed in 6.909338 seconds, peak
182,345,728 bytes under a 180-second/4-GiB external guard: two identical
355-class CSVs, semantic verifier, Python and .NET integer sums, rejection
tests and interrupted-publication recovery. The actual controller-function
test initially failed because it expected an English sharing-violation
message on a Chinese Windows installation. Only that test was changed to
check stable Win32 error code 32. The corrected test passed in 3.5500961
seconds, peak 203,866,112 bytes under 120 seconds / 4 GiB. This was not a
numerical mismatch or a change to production counting/controller arithmetic.

Artifacts: `data/logs/c6-finalization-20260907/s3-lineage-gate/`,
`s3-controller-gate/` (failed localized assertion), and
`s3-controller-gate-localized/` (PASS).

`scripts/run_guarded_step.py` reuses the existing process-tree guard and
records one explicit command per new log directory. Its finite smoke tests
preserved actual child exit codes 0 and 7; production uses expected exit 0.
