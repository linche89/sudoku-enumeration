# C=6 production S1 window 1

Date: 2026-08-02 to 2026-08-03 (Asia/Shanghai)

## Outcome

The first owner-authorized production `3->4` window did not close S1.  It
retained five exact parent-chunk commits and is safe to resume:

```text
newest generation       = 5 (.a)
cursor                  = 5/124
chunk parents           = 100000
emissions               = 92717503648
claimed L4 entries      = 903363975
parallel insertion holes= 18
real L4 states          = 903363957
checkpoint bytes        = 32521103228
checkpoint SHA-256      = 357ACEF8F257FAB819D53BB11E1E0AF2FBA882406F4A5196805895C9BD51274A
```

There is no `L4.snap`; none of these numbers is a complete-layer result.

## Launch and safety gates

The controller started from clean commit `2e5246c` at 12:49:25 on
2026-08-02 with:

```text
threads             = 24
caps                = 2000,14000000,1350000000,250000000,100000
checkpoint period   = 40 minutes, at the next completed chunk boundary
chunk parents       = 100000
target window       = 8 hours
hard process bound  = 10 hours
RSS bound           = 85 GiB
minimum available RAM = 8 GiB
```

Immediately before launch, `verify_all.ps1` printed
`ALL REPOSITORY CHECKS PASSED`.  The automatic S1 resource preflight reported
69.898 GiB RAM and 152.726 GiB disk required with margin, against 96.079 GiB
available RAM and 1,755.442 GiB available on E:.  It passed.  Peak measured
engine RSS was 61.856 GiB and engine stderr remained empty.

## Durable generations

The engine emitted the following successful markers:

| generation | cursor | claimed entries | write time |
|---:|---:|---:|---:|
| 1 | 1/124 | 900,304,885 | 22.12 s |
| 2 | 2/124 | 902,826,214 | 27.11 s |
| 3 | 3/124 | 903,144,370 | 21.92 s |
| 4 | 4/124 | 903,283,618 | 21.55 s |
| 5 | 5/124 | 903,363,975 | 30.44 s |

Direct 128-byte header inspection gave 18 holes, 903,363,957 real states,
92,717,503,648 emissions, and 5,187,318 cache hits in generation 5.  The
previous generation 4 remains locally valid at cursor 4/124.

## Sleep and stop behavior

The workstation slept and returned to the login session approximately twice.
The RSS sampler began at 12:49:35 on 2026-08-02 and next observed the process
after the accumulated wall interval at 07:55:15 on 2026-08-03.  On wake, the
controller saw that the ten-hour hard wall bound had passed and terminated
the engine.  This is fail-safe behavior: a sleeping interval does not count
as accepted arithmetic, and only the last fully written chunk cursor is
retained.

No `layer_dp_gate` or controller process remained after review.

## Controller defect and recovery

The initial controller then failed its post-stop backup step even though A/B
both existed.  In a PowerShell array, the unparenthesized expressions
`$base + ".a", $base + ".b"` had been parsed as one concatenated array
element, so durable-image discovery returned no match.  The arithmetic engine
and both generation files were unaffected.

Commit `bfbfe4e`:

- parenthesizes every path-suffix expression before array construction;
- adds generated checkpoint `.a/.b` files to `.gitignore`;
- fixes human-readable controller progress formatting.

The newest local image was then copied to the separate D: volume and both
copies independently hashed to:

```text
357ACEF8F257FAB819D53BB11E1E0AF2FBA882406F4A5196805895C9BD51274A
```

The external image is
`D:\sudoku_FJ_checkpoint_backups\layer_dp_c6_s1_prod_20260802\session-004-recovered-gen5-layer_dp_c6_s1_prod_20260802.a`,
with an adjacent receipt.

Finally, the corrected controller ran
`-PrepareOnly -ContinueExisting`.  It reran the complete repository gate,
recognized that the external receipt already covers local generation 5,
passed the C=6 S1 resource preflight, and exited without starting an engine
or changing A/B.

## Decision boundary

Window 1 is closed as a safe partial result, not as S1 completion.  Resume
must use the same production base, capacities, chunk size, and ordinary
production mode with `-ContinueExisting`.  A second production window needs
an explicit owner decision.
