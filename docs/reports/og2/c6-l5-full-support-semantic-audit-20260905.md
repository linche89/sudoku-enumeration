# Complete repaired L5 support semantic audit

Date: 2026-09-05. **PASS:** every one of the 96,452,755 live repaired C6 L5
records passed native legality, canonical-representative, exact stabilizer and
stable-ID membership checks. No F5 value was computed, no rehearsal weight
was trusted, and no input, released source or released executable was changed.

This is an all-record audit using the existing native canonicalizer, **not an
independent implementation of canonicalization**. It strengthens the support
certificate; it does not close the numerical L5 layer or N(6).

## Previously unverified premise

The actual reverse engine's `load_support()` checks the original image's
header, payload, exact full SHA, raw-key uniqueness, stored stabilizer mass
and holes. It erases all T entries and appends the two proved absent witnesses
only in RAM. At C6 its index rebuild deliberately does not call
`audit_native()` for every original record: the repair witnesses and later
computed numerical targets receive that check separately.

Thus the previous support-only load was not an all-record proof of original
canonicality or stabilizers. The diagnostic here closes that particular gap
without waiting for numerical F5 computation and without loading any L4 input.

## Diagnostic and exact acceptance conditions

The new isolated translation unit
`experiments/proto/layer_support_semantic_audit.cpp` includes the unchanged
actual `layer_reverse_f5.cpp` under `REVERSE_F5_NO_MAIN`. It invokes only the
support loader and the reusable `audit_all()` helper, not the numerical
entrypoint, reverse worker, chunk writer or exporter.

After the loader returns, the helper rebuilds the released L5 raw-key index
without changing any keys or stable IDs. It then checks every ID exactly once:

1. T must be zero, including insertion holes.
2. For every live record, `audit_native(state,5,storedStab)` verifies ordered
   valid native fields, degree five in every row and slot, zero padding,
   equality to the existing LDPCAN01 canonical representative, and the exact
   recomputed coordinate stabilizer.
3. Lookup of that same canonical key must return the record's own stable ID.
4. Live/hole counts, exact orbit mass and canonical-call count must agree
   with complete coverage and the independently established complete inventory.

Raw index rebuilding rejects duplicates. Because every live key is also a
canonical fixed point, this establishes coordinate-orbit uniqueness, not
merely distinct byte strings. The penultimate anchor is the existing native
`C-1` algorithm; the separately released two-missing prefix is not applicable
to these L5 source audits.

Canonical-call and DFS-node counters are collected independently per worker.
On an exception the helper cannot print the success marker. A partial or
timed-out scan is never a full-support certificate.

## Input identity and read-only protection

```text
original:
E:\Code\sudoku_FJ\data\logs\layer-dp-c6-e2e-rehearsal-20260802-1pct\s2-4to5\ck.L5.snap

separate-volume physical copy:
D:\sudoku_FJ_checkpoint_backups\c6_layer5_support_20260905\ck.L5.rehearsal.snap

bytes in each file = 3472307192
SHA256 of each    = A5FDDB22F8C79FD4DDC0015795558FBF1647F0DB556248F5C9551F2DB24891DF
```

Both original files remained unchanged. The executable retains Windows read
handles on the original and physical copy for its entire operation, denying
writes and deletion. Each path component is opened root-to-leaf with
`FILE_FLAG_OPEN_REPARSE_POINT`; reparse handles are rejected before descent,
and retained ancestor handles deny directory rename/delete. The two file
handles must report different volume serial numbers.

The backup is fully SHA-checked before loading. The actual loader authenticates
all original bytes, including old T, against the same full SHA before clearing
T. At successful completion the executable hashes both protected file handles
again. These final checks completed; success is not based only on size/mtime.
The outer controller additionally recorded unchanged source/copy metadata.

The original source contains 96,452,974 IDs, including 221 holes and
96,452,753 live keys. Only RAM is repaired:

```text
new IDs          = 96452974, 96452975
new stabilizers  = 1440, 240
repair hash      = 11401178190082244558
runtime rehearsal denominator after loading = 0
```

## Disposable complete-C5 qualification

Before root code review and the one authorized C6 invocation, the retained
Python gate built a new diagnostic executable in a fresh directory and tested
disposable complete C5 L4/L5 copies. C5 L5 uses the actual reverse support
loader. C5 L4 uses the existing zero-T support loader and exercises the
one-missing `C-1` anchor that is also used by C6 L5.

```powershell
python experiments/proto/layer_support_semantic_audit_gate.py `
  --l4 data/logs/layer-reverse-release-gate-b690f04bb0ca419e9b3604836b465725/fixtures/new-shared.L4.snap `
  --l5 data/logs/layer-reverse-release-gate-b690f04bb0ca419e9b3604836b465725/fixtures/reference-c5.L5.snap
```

The isolated build used `-O3 -mpopcnt -std=c++20 -fopenmp -Wall -Wextra`
and linked `-lbcrypt -lpsapi`; it did not use `-march=native` or overwrite
any released executable. Compiler stderr was empty.

The complete 1-thread and 2-thread results agreed:

| C5 layer | Live records / exact ID hits | Exact orbit mass | Canonical calls | DFS nodes |
| --- | ---: | ---: | ---: | ---: |
| L4 | 17,120 | 62,185,328 | 17,120 | 135,453 |
| L5 | 355 | 589,392 | 355 | 16,910 |

All T entries were zero. Nine malformed in-memory cases were refused in both
domains: wrong stabilizer, duplicate key, nonzero live T, invalid field,
nonzero padding, unbalanced slots, a valid but noncanonical coordinate image,
an unaccounted insertion hole, and nonzero hole T. An accounted zero-T hole
was accepted by the generic helper, and restoring all original in-memory
bytes restored full acceptance. Separate process tests refused a missing
readonly flag, wrong declared full SHA and insufficient positive record limit.

The complete build/test sequence exited successfully in 11.4941247 seconds
under an external 120-second / 2-GiB aggregate guard. Sampled aggregate peak,
including the compiler and Python parent, was 707,149,824 bytes. All 26 tracked
processes exited. Protected released binaries, included production sources
and original C5 fixtures retained identical SHA, size and mtime.

Small-gate logs: `data/logs/support-semantic-small-6bqmcu6y/`.
Its `receipt.json` SHA is
`97B6BFF9F6AE708F8D8C52ED6075E62C1D5D2EBC89E458E65798D4D1AC14EF5D`.

## One authorized full-C6 execution

The arithmetic candidate gate had terminated. The existing F4 window 6 was
still running, and this audit was explicitly permitted as a bounded
four-thread correctness check alongside it. It is **not an isolated timing
benchmark**, and no population runtime or F4/F5 speedup is inferred.

The frozen diagnostic SHA and source SHA were checked again before launch.
Available physical RAM was 61,598,400,512 bytes. One hidden process was
launched, with no retry or raised limit:

```powershell
& build/support-semantic-candidate-kyyxvz5r/layer_support_semantic_audit.exe `
  6 5 `
  input=E:\Code\sudoku_FJ\data\logs\layer-dp-c6-e2e-rehearsal-20260802-1pct\s2-4to5\ck.L5.snap `
  backup=D:\sudoku_FJ_checkpoint_backups\c6_layer5_support_20260905\ck.L5.rehearsal.snap `
  sha256=A5FDDB22F8C79FD4DDC0015795558FBF1647F0DB556248F5C9551F2DB24891DF `
  limit=96452976 threads=4 maxseconds=170 maxrssgib=8 checkpointreadonly
```

The internal watchdog enforced 170 seconds and 8 GiB. The external
Python/psutil guard enforced 180 seconds and aggregate 8 GiB, used
`CREATE_NO_WINDOW` and exclusive logs, tracked PID plus creation time, and
would stop only this diagnostic's exact process tree. The separate live F4
process was never included in that tree.

Complete executed output:

```text
L5_SUPPORT sha256=A5FDDB22F8C79FD4DDC0015795558FBF1647F0DB556248F5C9551F2DB24891DF live=96452755 mass=4439972139072 old_T=erased duplicates=0 repair_hash=11401178190082244558 runtime_rehearsal_denom=0 wall_s=7.22306
SUPPORT_SEMANTIC_AUDIT C=6 degree=5 entries=96452976 live=96452755 holes=221 mass=4439972139072 audit_calls=96452755 audit_nodes=706422194 exact_ID_hits=96452755 zero_T_entries=96452976 threads=4 load_wall_s=7.2232575 audit_wall_s=46.0386132 total_wall_s=58.8030052 peak_rss_bytes=4560756736 source_sha256=A5FDDB22F8C79FD4DDC0015795558FBF1647F0DB556248F5C9551F2DB24891DF native_algorithm=REUSED old_T=ERASED checkpointreadonly=yes file_writes=none F5=NOT_COMPUTED
ALL RECORD SUPPORT SEMANTICS PASSED
```

The attempt began at `2026-09-05T17:47:43.703376+08:00` and ended at
`2026-09-05T17:48:42.871491+08:00`. External elapsed time was 59.1531340
seconds; sampled aggregate RSS peak was 4,591,161,344 bytes. Exit code was 0,
neither guard fired, stderr was empty and both tracked processes exited.
There were no surviving descendants and no second attempt.

Full logs are `data/logs/support-semantic-c6-full-q2izz9bt/`:

```text
receipt.json SHA256
8928C5618E74F8A53691516BC55EB5A1D4B5560820C07D0538A91BE6835566F7
c6-all-records.stdout.log SHA256
89DEDB3EB201A037F18F49F4CFA4CCA308A890C608388B416BDA3B908AB68E5C
```

## Exact consequence and limitations

The [independent penultimate Burnside calculation](../../math/penultimate-layer-burnside.md)
counts exactly 96,452,755 coordinate orbits of balanced native L5 states.
The audit now verifies that the repaired catalogue contains exactly that many
legal, pairwise inequivalent orbits, with the correct stabilizer for each.
Consequently it is the complete support. The original file lacks exactly the
two witness orbits appended by the unchanged loader; this conclusion no
longer needs to assume the forward enumerator produced legal canonical
representatives and correct stabilizers for every stored record.

Balanced degree-five incidence is a regular bipartite graph, so the matching
decomposition used in the Burnside state model supplies native-row
realizability. The proof depends on the already qualified native canonical
algorithm and independent Burnside count. Re-executing that algorithm over
every record is not a second independent canonicalization implementation.

Together with the separately reported complete L4 geometry-orbit audit, both
reused support inventories now have all-record semantic evidence. Neither
audit evaluates the numerical F4/F5 weights. In particular every T in this
diagnostic was zero, no closed numerical L5 was exported, and no final
63,199-class contraction or N(6) result was produced.

## Frozen implementation and untouched releases

```text
new diagnostic executable
build/support-semantic-candidate-kyyxvz5r/layer_support_semantic_audit.exe
3980C4A012871857D2699FFF2501AF9703BF9FD66ABB34C61EC05A26DDFFD598

experiments/proto/layer_support_semantic_audit.cpp
C4FEA9F68B95A902919103EECDB511B2B41B181E4B7FBB8866E3C1FAAE26BC69
experiments/proto/layer_support_semantic_audit_gate.py
7C62BDB7386A452608B8FDD48ACA060E2BBD94AB17A9022137B66A5E2920143B

unchanged build/layer_shared_f4.exe
45A8387A031C3236191C71C28F73773711EEC34B9A628D4E715B2FE5F8EDD4FC
unchanged build/layer_reverse_f5.exe
E09123878A3098EFF955AA4DA1CCAF33B875F9CCEF7D9F337DF28F3ED08AFB9B
unchanged build/layer_dp_gate.exe
5F9BCC3410E5F2914884148B7944B6ECD2338747FE9C310AA64442596B5C10A8
```

All these pins retained their size and mtime as well as SHA. The complete
repository gate was not rebuilt/rerun during the live F4 window; this isolated
diagnostic changed no counting code or released binary and received the
complete small-domain gate plus this single authorized full support audit.
