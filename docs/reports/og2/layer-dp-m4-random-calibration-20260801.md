# Layer-DP random M4 and uniform 4-to-5 calibration

Date: 2026-08-01

## Outcome

The complete 58,400-parent C=6 probe passed its external resource guard and
closed the two measurement gates left open on 2026-07-31:

- a canonical-key-hash parent sample independently reproduced the historical
  strided M4 estimate;
- a nearly saturated canonical hash window supplied an approximately uniform
  global four-row sample for native 4-to-5 fan and real canonicalization
  measurements.

This is measurement evidence, not an exact enumeration of layer 4 or layer 5.
No full C=6 transition or final count was run.

## Safety and command

The worktree was clean at commit `870cdae`.  Before launch, both the active
factorization checkpoint and the retained exact layer-3 snapshot matched
their D: physical backups:

```text
factorization checkpoint SHA-256 =
FE8B68DE6C15592848D7CF69BF43928C000F0B2EE59FC263A094BCA2D376A865

layer-3 snapshot SHA-256 =
1D882DB7B0B18490E981539F7C690FA90227ECEC243817F5B538BEC8DAC865B7
```

The executable was rebuilt and run through `scripts/watch_rss.ps1` with a
12 GiB / 30 minute limit.  Engine arguments were:

```text
6 --threads 24 --caps 2000,14000000,1,1,1
--load-layer 3 data\checkpoints\layer_dp_c6_layer3_20260731.snap
--m4probe 58400 6 60000000
--m4-parent-seed 20260731
--fan-sample 20000
--fan-canon-sample 2000 10000000
```

The process exited zero.  Engine stderr was empty, observed peak RSS was
3,294,281,728 bytes (3.068 GiB), and the main M4 scan took 1,208.7 seconds.
No layer-DP process remained.  Both checkpoint hashes above were unchanged
after the run.

## Random M4 measurement

The parent sampler selected 58,400 of 12,324,872 canonical layer-3 keys by
the smallest independently mixed key hashes.  Exact output:

```text
3->4 sample emissions = 10107355744
hash window            = 2^-6
kept emissions         = 157846321
observed window keys   = 14095014
f1 / f2 / f3 / f4      = 56472 / 130360 / 231909 / 353337
f5+                    = 13322936
mean observed hits     = 11.199
```

Derived values:

```text
mean fan per sampled layer-3 parent = 173071.16
projected complete 3->4 work        = 2.1330798939e12 emissions

M4 observed-window lower estimate  = 902080896
M4 Chao1 estimate                  = 902863734.118
Chao-estimated capture             = 99.91329388%
```

The kept-emission count was 0.05136% below the uniform-hash expected value
`emissions / 64`, small enough for this capacity decision but retained as a
hash-uniformity diagnostic.

### Independent comparison with the 2026-07-27 strided run

| quantity | old strided | new key-hash | relative/difference |
|---|---:|---:|---:|
| parent sample | 58,400 | 58,400 | same size |
| emissions | 10,103,031,736 | 10,107,355,744 | +0.04280% |
| observed window keys | 14,095,055 | 14,095,014 | -41 (-0.000291%) |
| reported naive M4 | 9.021e8 | 9.021e8 | same |
| reported Chao1 M4 | 9.029e8 | 9.029e8 | same |

The two parent-selection designs are effectively indistinguishable at the
capacity scale.  This removes the material stride-bias concern and supports
the measurement-grade working value `M4 ~= 9.03e8`.  It does not turn M4
into an exact count or a theorem-level upper bound.  The conservative
1.35-billion production cap is retained.

## Uniform native 4-to-5 fan

The old 2026-07-27 calibration sampled a local four-row set captured from
150 three-row parents and found mean 2,731.  The new probe selected 20,000
canonical keys uniformly by an independent hash from the 14,095,014-key,
near-saturated M4 window:

```text
minimum / p05 / median / p95 / maximum = 216 / 1344 / 2688 / 3328 / 6912
mean                                    = 2617.482
standard error                          = 4.408
normal sampling 95% interval            = [2608.843, 2626.121]
count-only time                         = 2.7 seconds
```

The old local-capture mean was 4.1566% high.  Holding the Chao M4 estimate
fixed, the revised 4-to-5 work is:

```text
2.363230e12 emissions
sampling-only interval [2.355430e12, 2.371029e12]
```

That interval propagates only the fan sample uncertainty.  It does not
include residual M4/capture-model uncertainty.

## Real 4-to-5 canonicalization sample

An independent 2,000-key sample ran the production canonicalizing transition
into a bounded fixed table:

```text
emissions          = 5260480
distinct children  = 5098855 (96.927562% of emissions)
capacity holes     = 0
wall               = 0.509 seconds
wall per emission  = 96.7 ns
raw-cache hits     = 0.04%
canonical calls    = 5258168
mean search nodes  = 7.47
```

Multiplying the measured wall constant by the revised arithmetic work gives
2.647 days on this 24-thread host.  This is deliberately not promoted to a
production ETA: the bounded table does not model production-scale NUMA,
memory bandwidth, table-fill, checkpoint barriers, or long-run contention.
It does show that the `(C-1)`-row anchor is effective on uniformly sampled
C=6 inputs.

## Updated decision

The stronger M4 and unbiased 4-to-5 fan/canonicalization gates are complete.
The combined large-bucket arithmetic is now approximately:

```text
3->4 = 2.13308e12 emissions
4->5 = 2.36323e12 emissions
```

The next bounded work is not another repeat of this probe.  It is:

1. strengthen or guard the still-provisional layer-5 capacity;
2. implement and execute a bounded staged allocation/checkpoint/restart
   rehearsal;
3. execute the bounded end-to-end rehearsal with deliberate interruption and
   external exact summation.

No full 63,199-class C=6 run is authorized by this result.

## Transient-log provenance

The ignored run directory is
`data/logs/layer-dp-m4-random-20260801`.  Retained hashes at report time:

```text
engine.out  1184 bytes
SHA-256 52154E1E197AEB666FB92681345C75E364F002704E29AE8A781B8708FDD3162D

engine.err  0 bytes
SHA-256 E3B0C44298FC1C149AFBF4C8996FB92427AE41E4649B934CA495991B7852B855

rss.csv  14010 bytes
SHA-256 A06B134EB8A79583A392376723D5EC26E353249DEFA2BD86F8AB2538CFC019CA
```
