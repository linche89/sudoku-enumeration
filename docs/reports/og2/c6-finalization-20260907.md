# C6 complete F5 audit, export, and finalization

The owner explicitly authorized independent integrity audit, L5 export and
the complete exact N(6) finalization on 2026-09-07. That objective is now
closed and independently certificate-verified (100% complete):

```text
N(6) = 38296278920738107863746324732012492486187417600000
```

This reproduces the project's already documented historical target. It is
not a novelty claim for the decimal value. The audit scopes and exact
production evidence follow; the final two-run certificate closed at 15:28.

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

## Full S3 execution and exact result

The production source commit was `4dd86b590a2531993af0d00dbb93cf9a13b4de6c`.
The controller reran the complete repository gate successfully before its
fresh production build and passed its RAM/disk preflight. The exact launch:

```powershell
python scripts/run_guarded_step.py --seconds 9600 --gib 48 `
  --output data/logs/c6-finalization-20260907/s3-production -- `
  powershell -NoProfile -ExecutionPolicy Bypass `
  -File scripts/run_layer_dp_c6_s3_finalize.ps1 -AuthorizeFullC6 `
  -Layer5 data/checkpoints/c6_reverse_f5_closed_20260907.L5.snap `
  -Layer5Backup D:/sudoku_FJ_checkpoint_backups/c6_reverse_f5_20260905/20260907-142458-60c5a69f99fc4f98af862fa474975b7c-closed.L5.snap `
  -Layer5Sha256 4CF50FAD4F7C5DB020D30DEF258CE06AEF63125352E351D331A07BE8E173A6CF `
  -PrimaryBase data/checkpoints/layer_dp_c6_s3_prod_20260907_primary `
  -ReplayBase data/checkpoints/layer_dp_c6_s3_prod_20260907_replay `
  -ExternalBackupDir D:/sudoku_FJ_checkpoint_backups/layer_dp_c6_s3_prod_20260907 `
  -LogRoot data/logs/layer_dp_c6_s3_prod_20260907 `
  -SessionMaxHours 2.5 -MaxHoursPerAttempt 1 -ReserveMinutes 15 `
  -RssLimitGB 32 -Threads 24
```

This is the historical command, not an instruction to rerun a completed job
or overwrite its namespaces. It had a 9,600-second / 48-GiB aggregate outer
guard, a 2.5-hour controller session with 15-minute backup reserve, and a
one-hour / 32-GiB bound per native attempt. No bound was reached.

Both attempts used the same immutable L5, separate fresh bound namespaces,
24 threads, native caps `2000,14000000,1350000000,250000000,100000`,
100,000-parent chunks, 40-minute checkpoint interval, explicit `--ack-full-c6`
and `--load-layer 5`. Their exact native commands are in the primary/replay
`command.txt` files under the session log directory.

| Quantity | Primary | Replay |
|---|---:|---:|
| Native start | 14:37:10 | 15:02:28 |
| Emissions | 5,563,295,272 | 5,563,295,272 |
| Live final classes | 63,199 | 63,199 |
| Harmless insertion holes | 1 | 3 |
| Transition seconds | 1321.670 | 1331.336 |
| Canonicalizer calls | 5,563,258,549 | 5,563,258,549 |
| Native peak MiB (reported) | 4397.5 | 4397.4 |

Both native runs reported ALL GATES PASSED, correct G1/G2 bridges, complete
class count and labelled mass. They correctly refused to accumulate the
final weighted square in u128 after detecting per-term overflow; the exact
per-class m, ell and F were exported instead. No overflowing native total
was used as N(6).

The deterministic final CSVs are byte-identical despite different transient
insertion-hole positions. They have 63,199 data rows and 4,766,612 bytes:

```text
primary: data/logs/layer_dp_c6_s3_prod_20260907/final-primary.csv
replay:  data/logs/layer_dp_c6_s3_prod_20260907/final-replay.csv
CSV SHA256: 84F2720E7BA8296D78604153B934B7291AF93C329C07F5D46538F9D7101127C7
semantic SHA256: 292F8121743016528C0E523C4C7A14F4B4471DCA8BD93A279006DACDFF8FDB91
```

The independent Python semantic verifier checked every representative's
legality, prefix canonical form, stabilizer, orbit size, symbol multiplier,
uniqueness and qid order. It verified the complete labelled mass
622,345,892,187,672,576 = binom(12,6)^6. Canonicality plus uniqueness makes
the represented labelled orbits disjoint, so equality with that complete
mass certifies coverage, not just a plausible row count. It bound G1 to qid
43200 and G2 to qid 43206, with F values 6986348258918400 and
7053808087203840 respectively. CERTIFICATE PASS.

Separate Python integer and PowerShell/.NET BigInteger implementations both
computed the full square sum printed at the beginning of this report. Both
matched each other and the historical target. The semantic verifier's own
independent integer sum also agreed. No floating-point counting was used.

The controller completed with PRODUCTION S3 CERTIFICATE CLOSED and actual
exit zero. The outer process-tree receipt reports PASS, 3319.0009196 seconds
(55 minutes 19 seconds), sampled peak 4,806,529,024 aggregate bytes, and no
surviving tracked child. This time includes the fresh full gate, build,
hashes, both contractions, semantic/sum checks and backups; it excludes the
earlier F4/F5 production and this turn's preceding audit/export. It is not
a timing claim for the entire historical C6 project.

Session evidence:
`data/logs/layer_dp_c6_s3_prod_20260907/session-001/`.
The summary SHA-256 is
`226BC79EF838A58F1AA43548E3FB2AF5D2689149C05E1615836A905136D424B6`;
the outer receipt SHA-256 is
`C60DEEB0578793D1278721E2220872EAE58D80C4A581580DB38B8FA5F497F8DC`.

## Independent additional mathematical checks

The independent first-moment recurrence counted permutation arrays and
their side orientations without reading any F4/F5 table. It obtained

```text
permutation arrays A6 = 70957164389662881792000
sum(m ell F6) = 2^36 A6 = 4876139207527966044188061990912000
```

That first moment matched both complete C6 CSVs. The independently enumerated
9,408 reduced Latin6 squares give 812,851,200 labelled Latin squares and
four proved family anchors. All four matched, at qids 0, 4, 134 and 3520.
All 63,199 F values were positive and divisible by 6!=720. The full two-CSV
additional check passed in 0.9402174 seconds, sampled peak 49,238,016 bytes,
under 30 seconds / 1 GiB. Its C2 self-test accepted one fixture/rejected 12
mutations, and its complete C5 comparison passed before the C6 application.
Evidence: `data/logs/c6-finalization-20260907/moment-c6-complete/`.

A separate terminal-transpose check covered all 388 transposable C6 classes:
96 were self-transpose, and 146 pairs exchanged classes with equal F values.
This was also checked on all 355 C5 classes. The related structure theorem,
full finite code, proof scopes and actual higher-C table-size lower bounds
are in `docs/math/higher-c-structure.md`; retained transcripts and source
hashes are in `data/golden/og2-higher-c-structure-20260907.json`.

The initial first-moment build could not find Boost. The already audited
local GMP compatibility header was used, then retained with the unchanged
recurrence under `experiments/proto/` and independently rebuilt/rerun for
C=2..6. The C2/C3/C5/C6 outputs were byte-identical to the earlier builds.
The failure log is retained as `linear-build/`; successful production-neutral
rebuilds are under `linear-retained-build/` and `linear-retained-c*/`.
This was not a failure or modification of the production counting engine.

Scope: the two final contractions are separate executions of the same exact
engine from the same closed F5 values. The independent audits verify storage,
normalization, complete representative coverage and external summation;
the moments and anchors add independent mathematical constraints. They do
not constitute a second, independent numerical recomputation of every F4/F5
value or every F6 factorization from scratch. No fast N7--N9 algorithm is
claimed by the separate research result.

## Protected final artifacts

The final CSV, both wide snapshots, immutable binding/result receipts, and
the actual production executable are retained on the separate D: volume:
`D:/sudoku_FJ_checkpoint_backups/layer_dp_c6_s3_prod_20260907/`.

| Artifact | Bytes | SHA-256 |
|---|---:|---|
| `layer_dp_c6_final.csv` | 4766612 | `84F2720E7BA8296D78604153B934B7291AF93C329C07F5D46538F9D7101127C7` |
| `primary.L6.snap` | 3286528 | `9A480B9723D36D10CBE719D7C9F8B33CA4D450AE7F095298B443D6525CEB182C` |
| `replay.L6.snap` | 3286632 | `66C537A68BFE41797B544580B7358274C7B75BFD7BAB5B4037DDD05656726E6C` |
| `layer_dp_gate.production.exe` | 337628 | `45C85C6A818893F435F0130755CA7C8FA7CDBA46BC3223F89A26F5EC4CC503B6` |

Both local/external L5 SHAs were rechecked unchanged at controller closure.
The parent-stage `.L5.snap` names are local hard-link aliases of that same
immutable export, not independent backups. Each final `.L6.snap` is likewise
linked to its namespace's closed `.a` generation. The different snapshot
SHAs reflect insertion ordering/holes, not a difference in final class data.
No original checkpoint, old generation, user file or log was deleted.

The small tracked final receipt is
`data/golden/og2-c6-final-certificate-receipt.json`; the complete checkpoint
inventory remains `data/checkpoints/MANIFEST.md`. Large CSV/binary payloads
stay out of ordinary Git history.

The master final receipt and its D: physical copy have SHA-256
`5BB7CD7F505662DFC87C19E901EE362BC3327D747FF2F0090A9921817656C10F`.
The listed hard-link identities were independently checked with
`fsutil hardlink list`. Narrow `.gitattributes` rules preserve exact bytes
of SHA-bound receipts across Git checkout; retained auxiliary source files
use LF, matching their recorded source hashes. No checkpoint format changed.

## Final handoff gate

After the S3 controller and both native workers exited, the complete gate
was run again (with the actual production executable already protected on D:):

```powershell
python scripts/run_guarded_step.py --seconds 360 --gib 8 `
  --output data/logs/c6-finalization-20260907/final-verify-all -- `
  powershell -NoProfile -ExecutionPolicy Bypass -File scripts/verify_all.ps1
```

PASS: 171.5692108 seconds, sampled peak 1,132,478,464 aggregate bytes,
actual exit zero, no surviving tracked processes. The full C2--C5 and FJ9
gates, canonical/checkpoint recovery tests and the optional read-only C6
graph-memo reload all passed. No optional gate was skipped. Production
checkpoint bytes were not changed by verification. `git diff --check` passed;
the pre-existing untracked user files were preserved and excluded from staging.

Commit evidence: `1e8f7ed` added the independent F5/export audit helpers;
`4dd86b5` certified the closed L5 and is the production source commit;
`c07b67a` retained the terminal/higher-C checking code; `c897312` archived the
final certificate/lineage metadata and byte-preservation rules. Three copied
research files had an extra EOF blank line; only those blank lines were
removed, their affected finite checks/build were rerun successfully under
30 seconds / 1 GiB, and the retained-source hashes were refreshed. No counting
arithmetic changed. All 13 protected source/receipt Git blobs were checked
byte-identical to their working files before the metadata commit. Current
status/method documentation is committed separately from the data evidence.
