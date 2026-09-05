# Reverse-F5 implementation release and bounded C6 evidence — 2026-09-05

N(6) remains uncomputed. The actual new reverse-F5 engine has passed complete
C5 and recovery gates; no C6 numerical F5 chunk has been produced.

## Exact arithmetic and source boundary

The new `experiments/proto/layer_reverse_f5.cpp` consumes only a verified,
fully closed native L4 export. Its stored weighted values are divided exactly
by their native orbit sizes before use. The rooted recurrence is
`F5(Q) = 5 * sum_M F4(Q-M)`, where M ranges over labelled perfect matchings
containing a fixed edge; grouped duplicates carry their exact multiplicity.
It retains every distinct native target response.

The old L5 rehearsal snapshot is only a support catalogue. Its header,
payload, SHA, key uniqueness and exact inventory are checked, every old T5
is erased, the two proved absent keys are appended in RAM, and production
configuration is restored. A valid input hash is an integrity guarantee,
not proof that arbitrary supplied weights are mathematically closed.

New immutable F5 chunks bind both input SHA values, both native header hashes,
the support-repair fingerprint and chunk geometry. Live records are positive
closed u64 F5 values divisible by 120; holes are zero. Intermediate sums use
u128 with checked output conversion. Export requires full closure and checks
every key, stabilizer and T5 on native readback before final nonreplacing
rename. No original checkpoint or format is modified.

## Executed release gate

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/verify_layer_reverse.ps1 -Threads 4
```

Exit 0, 40.8867173 seconds including compilation; Python suite elapsed 25.680
seconds. Official wrapper stdout:
`data/logs/reverse-final-release-run-6dc7d6dcbfab432fbccafa3272457656/stdout.log`.

Detailed disposable evidence:
`data/logs/layer-reverse-release-gate-152f39785b8f4499a368afa5be923244/`.

```text
PASS kill-resume-readback all355_values=1 byte_identical=1
N5=1903816047972624930994913280000
ACTUAL REVERSE F5 END-TO-END AND RECOVERY GATES PASSED seconds=25.680
PRODUCTION REVERSE F5 CHECKS PASSED
```

The fresh chain starts with a NEW shared-F4 export, checks every one of 355
C5 F5 values, produces a byte-identical L5 against the independent reference,
recovers exact N(5), and is accepted by the existing native final-layer
consumer. It also exercises changed-thread resume, actual forced termination
after a committed chunk, unchanged old-chunk hashes, both input lineages,
poisoned old support weights (erased, not trusted), holes, noncanonical keys,
wrong stabilizers, nonfactorial values, corrupt payload/header, read-only
protection and overwrite refusal.

```text
released actual reverse executable SHA256:
C7BDBC4DEA0FE8497787D7DB127A9B718D591BCA5F59B9394C64F6E11F295761
fresh C5 L4 export SHA256:
E60114E228A328CE6734B1F68E0136017193D7DD20181F3AD97BE3186127D04A
fresh C5 L5 export SHA256:
E6FBEFB1541E8B9B859144164283BFC0668488BDE3A03BC7ECBD9FE90DEDAD5A
```

## Full-size C6 lookup-only scaling

The already executed reverse-core probe used 512 uniform native L5 samples,
with replacement from the actual catalogue, rejecting holes, seed 20260905.
Its input is `data/logs/c6-direct-route-20260905/l5-sample-512.txt`.
Each run rebuilt the complete L4 native index from the SHA-verified read-only
original catalogue plus the one proved missing key. All historical T4 values
were discarded.

| Threads | Query wall seconds | Whole-process seconds |
| --- | ---: | ---: |
| 1 | 7.7614696 | 131.2995742 |
| 8 | 0.9908844 | 38.3521091 |
| 24 | 0.4324510 | 32.0084893 |

Every run had exactly 2,691,041 labelled matchings, 2,471,101 weak residual
queries, 2,471,101 hits, zero misses, 53,161,937 canonicalization nodes and
lookup checksum 11,399,444,842,143,351,229. Peak working set at 24 threads was
41,132,830,720 bytes. Logs are
`data/logs/c6-direct-route-20260905/reverse-f5-c6-t{1,8,24}.out` with adjacent
stderr/RSS files.

The query phase scales by about 17.95 from one to 24 threads on this sample.
It does NOT read numerical F4 values, accumulate F5 values, serialize new
chunks or produce N(6). Straight population extrapolation is about 22.63
hours of query work, not a production-duration guarantee; numerical reads,
accumulation, audits, I/O and sample variability remain uncharged.

## Actual L5 support and physical protection

The bounded actual support-loader smoke ran
`build/layer_reverse_f5_support_test.exe` on the original L5 support with
`checkpointreadonly`. It enforces a 120-second/6-GiB bound and 8 GiB free-RAM
preflight. The executed smoke took about 4.21 seconds and peaked at
4,553,400,320 bytes. It independently checked the full source SHA, zero
duplicate original keys, erasure of all old T5 values, appended IDs
96,452,974 and 96,452,975, repaired live count 96,452,755, repaired orbit mass
4,439,972,139,072 and production-configuration restoration. This produced no
F5 value. The repair fingerprint is 11,401,178,190,082,244,558.

A new physical backup was made and independently full-SHA-verified:

```text
source:
data/logs/layer-dp-c6-e2e-rehearsal-20260802-1pct/s2-4to5/ck.L5.snap
copy:
D:\sudoku_FJ_checkpoint_backups\c6_layer5_support_20260905\ck.L5.rehearsal.snap
bytes each: 3472307192
SHA256 each:
A5FDDB22F8C79FD4DDC0015795558FBF1647F0DB556248F5C9551F2DB24891DF
```

Both full hashes were rechecked after release. Neither input was patched.

## Remaining release boundary

`scripts/run_layer_reverse_window.ps1` has passed independent source review
and PowerShell parsing, including the actual wrapper success marker, producer
chunk names, separate-volume before/after backups and timestamped completed
export copies checked against the producer's readback SHA. It has NOT run a
C6 F5 production window: no closed C6 F4 export exists yet.

Follow `docs/runbooks/layer-reverse-c6.md` only after full F4 closure,
readback and separate-volume backup. The first numerical F5 C6 pilot must
use a positive limit and explicit time/RSS bounds. The existing S3 still
requires independent full replay, all 63,199 final classes and exact final
certificate checks. No stage described here substitutes for those checks.

## Final repository regression

After the new implementation and controller sources were frozen, the complete
`scripts/verify_all.ps1` was rerun under an external 360-second and aggregate
8-GiB process-tree guard. It exited successfully with
`ALL REPOSITORY CHECKS PASSED`. It includes the complete mandatory C2..5
counts, independent future-twin C5 checks, all FJ9 references and the C6 G1
checkpoint-readonly gate. The retained stdout/stderr are in
`data/logs/final-baseline-release-871e5c39f9914f13a7f27043adc4973c/`.

Exact external elapsed was 220.6553043 seconds, with sampled aggregate
descendant working-set peak 1,089,761,280 bytes. Both released executables
retained their pre-gate SHA, size and modification timestamp. The shared-F4
executable SHA is
`45A8387A031C3236191C71C28F73773711EEC34B9A628D4E715B2FE5F8EDD4FC`;
the reverse-F5 SHA is the release digest above. The read-only C6 graph memo
retained SHA
`FE8B68DE6C15592848D7CF69BF43928C000F0B2EE59FC263A094BCA2D376A865`.

Both new window controllers pass PowerShell parsing and independent review.
`git diff --check` passes, with only the repository's LF/CRLF conversion
warnings. No counting process remained at handoff. No commit was created;
all unrelated user work, including `paper/c5-open-verification/`, was left
untouched. No full C6 F5 or S3 gate was run because its closed-layer input
does not yet exist.
