# Pair-tail external future-twin C=6 results — 2026-07-14

## Scope

This report records the implementation and exact evaluation of the paired-row
tail ideas suggested in `docs/expert/2026-07-14/`.  Those files were treated as
research input.  Every retained claim below was established by the current C++
implementation and exact local gates.

No full 63,199-class run was started.  Every C=6 command used a positive
`limit=`, an RSS guard, and a time guard.  The graph-memo checkpoint was not
passed to the future-twin engine and was not modified.

## Retained implementation

`src/future_twin.hpp` now contains:

- paired row orders with a mandatory mate step;
- an exact six-row three-pair tail;
- an exact seven-row tail with pair-symmetry kernel caching;
- adaptive selection of the first row in each pair;
- 41-byte external state records, local sort/reduce, and multi-pass merge;
- immutable external-layer and tail manifests with mass and file audits;
- parent-boundary generation checkpoints, including safe restart after a
  process is killed during a parent, run flush, or merge;
- OpenMP tail evaluation with serial exact `unsigned __int128` commit;
- configurable `futuretailchunkrecords=` for the parallel locality/load-balance
  tradeoff; the verified default is 240,000 records.

Generation restart was tested by intentionally stopping after one committed
parent and requiring an exact restart from that parent.  The external selftest
also exercises partial tail restart and completed read-only restart.

## Correctness gates

After the final code change, the complete gate passed:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts\verify_all.ps1
```

Wall time was 127.4 seconds.  The gate included the exact C=2..5 totals,
future-twin per-class differentials, canonical invariance and separation,
external generation/tail restart tests, all 71 FJ9 reference classes, and the
read-only C=6 graph-checkpoint gate.

The mandatory totals remained:

```text
N(2) = 288
N(3) = 28200960
N(4) = 29136487207403520
N(5) = 1903816047972624930994913280000
```

## External G1 result

The seven-row pair-tail engine independently returned

```text
F6(G1) = 6986348258918400
```

Its final external frontier had 56,461,626 records, 2,152,940,361,120 total
weight, and a 2,314,926,706-byte layer file.  The tail checkpoint was resumed
at 22,400,000 records and completed exactly.  A subsequent
`checkpointreadonly` run returned the same value without writing state.

## Exact G2 layer construction

The retained G2 job is:

```text
data/logs/future-external-c6-g2-adaptive-tail7-layers2-20260714/class-2
```

The pair-adaptive prefix profile was:

| processed rows | distinct states |
|---:|---:|
| 0 | 1 |
| 1 | 1 |
| 2 | 352 |
| 3 | 77,956 |
| 4 | 9,664,963 |
| 5 | 221,438,460 |

Layer 5 was produced from 607,148,632 raw records in 1,258 locally reduced
runs.  The exact audits were:

```text
parent records            = 9,664,963
parent mass               = 31,417,989,120
child distinct records    = 221,438,460
child mass                = 2,009,438,804,160
layer-05.bin bytes         = 9,078,976,900
total external bytes made = 52,138,814,395
layer-04.bin sha256        = 06550599BEC66ABF56AADAD9526377B6086452A4C0AFFF4FD4627CB670A3A197
layer-05.bin sha256        = 070755E6AA02A28AA7923FC13B35FBA5D643AC54012C54A93BB1727D7A020527
```

The first bounded process stopped with an uncommitted run suffix.  Restart
loaded the 3,700,000-parent manifest, rejected the suffix, and continued from
exactly that boundary.  Later stops resumed at 4,700,000 parents and then from
immutable tail checkpoints.  This is direct production evidence for the
parent-boundary transaction protocol in addition to the small exact selftest.

## Tail calibration

All calibrations were sequential bounded continuations of the same exact job,
so adjacent regions were not identical workloads.  They are operational
measurements, not a controlled microbenchmark.

The useful observations were:

- 200,000,000 cached kernel records used 2.182 GB peak RSS and did not improve
  throughput over 100,000,000 records;
- 1,000,000-record OpenMP batches improved locality substantially but exposed
  severe static-load imbalance in a difficult sorted region;
- 240,000-record batches retained locality while reducing that imbalance;
  one five-minute continuation advanced the committed boundary from 169M to
  185M, or 3.2M records/minute, at 1.161 GB peak RSS;
- the final 185M-to-end continuation took 712.568 seconds of tail time and
  peaked at 1,216,040,960 bytes.

The retained production settings are 32 tail threads, 100,000,000 total cached
kernel records, 240,000 records per parallel batch, and a 1,000,000-record
immutable tail-checkpoint interval.

## Exact G2 result

The complete job returned

```text
F6(G2) = 7053808087203840
```

Final exact tail statistics were:

```text
tail states              = 221,438,460
zero tail states         = 295
support total            = 799,150,597,270
mean support             = 3608.906
maximum support          = 11,936
local assignments        = 872,224,884,432
maximum tail value       = 14,980
kernel hits/lookups      = 223,048,814 / 442,876,920
kernel evictions         = 219,349,943
```

`COMMITTED.manifest` and the final
`tail-checkpoint-000221438460.manifest` both store the result above.  A separate
read-only reopening used `checkpointreadonly`, restored all 221,438,460 tail
records from the final manifest, and returned the same value in 7.617 seconds
of class time (15.9 seconds including the watcher process).

## Checkpoint audit

After all runs, the active graph checkpoint and its external safety copy still
had identical size and SHA-256:

```text
size   = 217954462 bytes
sha256 = FE8B68DE6C15592848D7CF69BF43928C000F0B2EE59FC263A094BCA2D376A865
```

## Decision

The expert ideas did provide a real route forward: paired tails plus exact
external reduction remove the G2 RAM wall, and both G1 and G2 are now closed by
the optional backend.  They do not solve the complete C=6 problem.  G2 alone
has a 221,438,460-state tail and a 9.08 GB committed frontier.  Repeating a
class-local job independently for 63,199 outer classes is not a credible full
sum strategy, and G1/G2 do not establish the distribution of later classes.

The next justified experiment is a small positive-limit calibration over
additional outer classes, coupled with work on cross-class reuse or a stronger
global quotient.  A full outer sum remains unauthorized.
