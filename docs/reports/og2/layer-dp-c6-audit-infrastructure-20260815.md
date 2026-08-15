# C=6 layer-DP audit infrastructure

Date: 2026-08-15 (Asia/Shanghai)

## Scope and boundary

This change closes four audit gaps without changing the layer-DP arithmetic,
canonical key semantics, or checkpoint format:

1. production S1 window 4 / generation 37 is distilled into the authoritative
   documentation and checkpoint manifest;
2. future durable checkpoints acquire an append-only structured progress
   sidecar;
3. the final CSV has an independent semantic certificate verifier and a
   second arbitrary-precision sum implementation;
4. the S2 and S3 production chain is frozen behind mandatory L4/L5 snapshot
   hashes and separate owner authorization.

No C=6 transition was run.  No active checkpoint was opened for writing,
moved, or replaced.  There is still no closed L4, L5, final CSV, or complete
`N(6)` result.

The functional implementation and its gates are retained in commit
`f6e928f` (`layer-dp: add production audit chain`).

## Read-only checkpoint audit

The version-2 header of the current S1 `.a` image independently decoded to:

```text
generation        = 37
transition        = 3->4
cursor            = 38/124
chunkParents      = 100000
claimed entries   = 903398620
holes             = 18
real states       = 903398602
emissions         = 704741992192
cache hits        = 34280078
payload hash64    = 0x7A259B829A24D6ED
header hash64     = 0x9317594FF1B39773
bytes             = 32522350448
```

Fresh SHA-256 passes over the local and D: copies took 29.834 and 31.348
seconds and agreed exactly at:

```text
ECF0837315B0FDF8AE21C394FDA6676490E43B1828A0825529A344EC17E4E844
```

The prior `.b` header is generation 36 / cursor 37.  Its independently
calculated SHA-256 is:

```text
B11F94B0D9A471A1FFACD340AD94FFE29874D537AB3FD2EACB3E1C8233F3FDF9
```

No `layer_dp_gate` process was running.

## Progress sidecar

`scripts/layer_dp_progress.py` performs these read-only checks before it can
append a row:

- magic, version, C, transition, counts, and chunk geometry;
- the checkpoint's internal header checksum;
- exact file length derived from the header;
- marker/header agreement for generation, cursor, and total chunks;
- complete file SHA-256;
- monotone and consecutive generation/cumulative counters;
- matching configuration and stage identity;
- exact non-hole parent count from the immutable parent's relevant `stab`
  interval.

The CSV append is flushed and fsync'd.  An identical retry is idempotent; a
conflict fails.  Controller failure to inspect or append stops the engine and
preserves/backs up the newest durable checkpoint.

The existing S1 sidecar was bootstrapped with one `recovered_baseline` row for
generation 37.  Its delta fields are empty by design.  The earlier 38 closed
chunks are not rerun and the missing per-generation counters are not guessed.

On a disposable complete C=5 `3->4` checkpoint pair, the gate inspected both
headers, appended the older generation as a baseline, and derived for the
newest final chunk:

```text
real parents             = 150
emissions delta          = 80168
emissions / real parent  = 534.453333333
```

Repeating the identical append left the sidecar at two rows.

For current 32.5-GB S1 images, SHA-256 adds about 30 seconds of read I/O per
durable checkpoint on the reference machine.  This is deliberate audit cost
and is recorded as `hash_seconds`; it does not change or duplicate the
checkpoint payload.

## Independent final certificate

`experiments/proto/s4_certificate_verify.py` treats every representative as a
witness and independently recomputes:

- complete-state legality and balance;
- the specified prefix-order canonical form under `C2 wr S_C`;
- stabilizer order and `m = 2^C C! / |Stab|`;
- `ell = (2C)! / product c_w!` from repeated words;
- representative uniqueness and deterministic qid order;
- complete labelled mass;
- exact G1/G2 representative-to-F bindings at C=6;
- the arbitrary-precision weighted square as an additional check.

The Python implementation does not call or link the C++ engine.  Its complete
C=5 gate verified all 355 rows and the known `N(5)`.  A CSV with the first
orbit size changed from 16 to 17 was rejected with independently derived
stabilizer 240 and orbit size 16.

The separate C=6 anchor self-test derives:

```text
G1: stab=120, m=384, representative=0 5 10 19 29 30 39 43 44 48 54 57
G2: stab=8,   m=5760, representative=0 5 10 23 27 28 35 44 47 48 54 57
```

The future 63,199-row gate has not been run because its input does not exist.

## Independent exact sums

The existing `s4_exact_sum.py` remains the primary external sum.  A second
implementation, `s4_exact_sum.ps1`, uses PowerShell CSV parsing and .NET
`BigInteger`; it shares no parser or arithmetic code with the Python helper.
Both reproduced:

```text
N(5) = 1903816047972624930994913280000
```

Final S3 requires both implementations to produce the same exact C=6 total.
The semantic verifier separately binds the G1/G2 values to their actual
representatives, closing the old “values merely occur somewhere” weakness.

## Frozen S2/S3 production chain

The new controllers are dormant until their exact parent snapshots exist:

- `run_layer_dp_c6_s2_window.ps1` requires a caller-supplied, 64-hex L4
  SHA-256 and matching local/D: snapshots before preflight;
- `run_layer_dp_c6_s3_finalize.ps1` similarly requires L5, uses two independent
  final checkpoint namespaces, retains both wide final snapshots, requires
  byte-identical CSVs, runs the semantic verifier and both sums, and writes an
  externally verified CSV receipt;
- exact engine commands and source Git commit are retained with the logs;
- `-PrepareOnly` performs gates and preflight but writes no checkpoint or CSV.

Neither controller grants authority.  S2 remains impossible with the current
partial S1 image, and S3 remains impossible without a closed L5.

## Verification

The proportional gate was run as:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass `
  -File scripts\verify_layer_dp.ps1 -Threads 8 -KillIterations 1
```

An initial pass completed in 40.3 seconds.  A final repeat after controller
hardening completed in 39.9 seconds, both with:

```text
LAYER-DP CHECKPOINT AND EXACTNESS GATES PASSED
```

The complete repository gate was then run as:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass `
  -File scripts\verify_all.ps1
```

It exited zero after 235 seconds and reached:

```text
ALL REPOSITORY CHECKS PASSED
```

This covered the mandatory C=2..5 values, all 71 FJ9 reference classes,
canonical invariance/separation/histogram differentials, checkpoint recovery,
the C=6 read-only G1 gate, the new certificate/sidecar tests, and both C=5
exact sums.  The dormant S2/S3 scripts were syntax-parsed; their full C=6
paths are intentionally unexecuted until manifest-pinned L4/L5 inputs exist.
