# C=6 catalogue certificates and direct-layer decision experiments

Date: 2026-09-05 (Asia/Shanghai).

The complete N(6) has not been computed. This investigation constructively
closes both missing native catalogues and qualifies small-case numerical
components of a different weight calculation. Production weights remain open.
No active checkpoint has been modified, and no production transition has run.

## Exact catalogue closure

The new independent two-missing-box Burnside counter gives

```text
M4(6)             = 903398603
raw native L4     = 41602261536160
Burnside numerator= 41628607626240
group order       = 46080
```

The retained program reproduced complete C=2..5 layer counts and all twenty
direct C=4 fixed-point differentials. Its full C=6 calculation took 53.441
seconds, with at most 3,125 states in a nonidentity degree frontier. Eleven
independently obtained C=6 fixed-point values also agree. See
`../../math/two-missing-layer-burnside.md` for the bijections and exact commands.

The read-only support scanner verified the original version-2 header checksum,
chained payload checksum, exact file length, holes, and complete SHA-256. It
counts stored stabilizer masses; it does not re-prove the canonicality and
stabilizer of every stored production key.

| catalogue | stored real keys | exact complete keys | stored coordinate mass | complete coordinate mass |
|---|---:|---:|---:|---:|
| production L4 generation 37 | 903,398,602 | 903,398,603 | 41,602,261,532,320 | 41,602,261,536,160 |
| rehearsal L5 | 96,452,753 | 96,452,755 | 4,439,972,138,848 | 4,439,972,139,072 |

The source files and hashes are:

```text
data/checkpoints/layer_dp_c6_s1_prod_20260802.a
32522350448 bytes
ECF0837315B0FDF8AE21C394FDA6676490E43B1828A0825529A344EC17E4E844

data/logs/layer-dp-c6-e2e-rehearsal-20260802-1pct/s2-4to5/ck.L5.snap
3472307192 bytes
A5FDDB22F8C79FD4DDC0015795558FBF1647F0DB556248F5C9551F2DB24891DF
```

Both the root and the independent support branch rehashed the existing D:
generation-37 physical backup to the same L4 SHA-256. All production and
rehearsal source files were opened read-only. Sample and diagnostic outputs
used new paths under `data/logs/c6-direct-route-20260905/`.

### The three missing witnesses

All masks below are ordinary incidence bits in paired slots, not the checkpoint
field encoding (0 absent, 2 side zero, 3 side one).

```text
L4, stabilizer 12, orbit size 3840:
294 294 554 554 1161 1161 1360 1360 2181 2181 2640 2640

L5, stabilizer 1440, orbit size 32:
341 682 1109 1301 1349 1361 1364 2218 2602 2698 2722 2728

L5, stabilizer 240, orbit size 192:
421 602 1129 1364 1418 1574 1681 2198 2329 2402 2629 2728
```

The L5 witnesses come from two copies of a crown graph and the symmetric
order-six conference matrix. Both were expanded under all 46,080 coordinate
transformations and found absent by a complete read-only scan. Their orbit
sizes add to exactly 224, the raw mass deficit.

For L4, the unique missing orbit must have stabilizer 12, from its mass deficit
3,840. Cauchy's theorem therefore supplies an order-three automorphism. Up to
signed conjugacy it has box cycle type (3,1,1,1) or (3,3). Exhaustive generation
found exactly 280 and 41,620 fixed raw states, respectively. These 41,900 records
canonicalize to 1,060 native orbits, distributed as follows:

```text
stabilizer:  3    6   12  24  48  96  144  384
orbits:    726  237   66  16   9   2    2    2
```

All 66 stabilizer-12 candidates were independently checked by a complete
coordinate-group stabilizer scan. Expanding these candidates gave 253,440
coordinate images. A complete production-file membership scan found 65
candidates once each and exactly one absent: target 48, displayed above. The
scan took 50.343 seconds and peaked at 31,117,312 bytes.

Two earlier candidate families were explicitly eliminated: the unique
multiplicity-(4,4,4) orbit was already present at index 895,868,296, and all five
stabilizer-12 candidates with two six-vertex used-graph components were present.
These failed hypotheses did not become support assumptions.

The retained fixture is `../../../data/golden/og2-c6-support-witnesses.json`.
The exact statement is: given the established validity, uniqueness and
stabilizer guarantees for the stored keys, adjoining these disjoint witness
orbits closes both supports, by both orbit count and positive coordinate mass.
Neither original T4 accumulators nor any rehearsal weights are accepted as
closed production values. No repaired checkpoint has yet been written.

## Direct values and rooted reverse layers

For a native state x, its closed weight is `T_L(x)=m_x F_L(Q_x)`. The prototype
evaluates F4 using the retained rooted two-factor split. For a target graph of
degree d, a fixed edge e gives the independent reverse-layer expression

```text
F_d(Q) = d * sum_{perfect matchings M containing e} F_(d-1)(Q-M).
```

Every ordered factorization has exactly one color containing e; color symmetry
gives the factor d. Labelled matching multiplicities must be retained when
equal residual multisets are grouped. There is no extra native orbit factor
inside this graph recurrence. Native weights are restored only as m times F.

`layer_direct_gate.py` creates fresh complete C=2..5 snapshots with the existing
row engine, validates their geometry, decodes every exact T/m, and checks each
value with the new direct and reverse evaluators. C=5 also uses the independent
355-class reference fixture. The full run passed every native coefficient,
including 16,150 three-row values, 17,120 four-row values, and 355 final values.
It independently recovered all known N(2)..N(5) square sums.

The graph-key reverse method is numerically correct but expensive. A second
prototype reuses the unchanged native canonicalizer and the complete native
predecessor key table. It passed every C=2..5 final value and all 17,120 C=5
four-row values in bounded batches. It avoids unpaired graph classification
on every reverse-layer lookup.

## Bounded C=6 measurements

The support scanner selected 1,024 L4 and 64 L5 records uniformly with replacement
using `std::mt19937_64`, seed 20260905, rejecting insertion holes. These are
actual native-catalogue samples, not synthetic switch-chain samples. All sample
records were independently checked for native legality. The sole missing L4
orbit affects any bounded mean in [0,1] by at most 1/903398603.

| experiment | measured result |
|---|---|
| 128 cold L4 evaluations | F4 kernel 0.357082 s; graph canonicalization 0.005955 s; separate exact leaf pre-count 0.170601 s |
| 1,024 cold F4, 1 thread | kernel wall 3.067956 s |
| same 1,024, 8 threads | kernel wall 0.377489 s |
| same 1,024, 24 threads | kernel wall 0.169946 s; peak approximately 189 MB |
| same 16 L5 sources, graph-key gather | 84,032 rooted matchings, 72,089 weak residuals; graph canon 2.168982 s; total 2.24611 s |
| same 16 L5 sources, native-key gather | identical matching/residual counts; native canon 0.15848 s; total 0.19138 s |
| all 64 sampled L5 sources, native gather | 337,680 rooted matchings; 314,584 weak residuals; native canon 0.69147 s; total 0.85806 s |

All 1,024 F4 values, split-leaf counts and node counts agreed exactly between
1, 8 and 24 threads. Full parallel small-case checks passed 26 C=4 values and
17,120 C=5 values. Deliberate record, time and RSS bound violations failed
closed. The returned external A/B witnesses were also independently evaluated
as F4=1,806,336 and 1,403,904 by the retained C++ kernel.

The F4 parallel benchmark separately pre-counts split leaves to enforce its
hard record budget. At 24 threads that preflight took 0.58865 seconds; total
wall time was 0.7605 seconds, not 0.169946 seconds. The latter isolates a kernel
that a future bounded production implementation might use with different
work-accounting infrastructure. It must not be represented as completed
end-to-end production performance.

Using only that measured kernel wall rate, all 903,398,603 native F4 values
would require approximately 41.65 hours. This is a sample-based extrapolation,
not a duration guarantee, and excludes catalogue handling and checkpoint I/O.
It offers only a modest improvement over roughly 55 remaining S1 hours if
every native state is evaluated independently.

The 64-source reverse sample projects about 5.0891e11 rooted matching records,
against the historical forward S2 estimate of 2.36323e12 emissions. These are
different record types. No fivefold runtime claim follows. In particular the
small-table timings do not yet charge the full production random-access table.

## Graph sharing: real samples, not a production speedup claim

An independent NetworkX referee exhaustively computed graph automorphisms and
admissible pairing orbits on the first 256 actual uniform L4 samples. It first
passed the complete C4 L3 census: 54 native states form 38 graph classes with
fixed bipartitions, or 33 when whole transpose is allowed.

Writing b for the exact number of native orbits in a graph fiber, the exact
population identity is `H/M = E_native[1/b]`. The observed data were:

| graph equivalence | sample mean 1/b | sample standard error | reciprocal mean |
|---|---:|---:|---:|
| preserve bipartitions | 0.2740088331 | 0.0152407606 | 3.6495 |
| allow whole transpose | 0.1562159491 | 0.0097389115 | 6.4014 |

The normal-approximation 95% intervals on the reciprocal are 3.29--4.10 and
5.70--7.29. These estimate graph inventory compression, not achieved cache hit
rates or time savings. Classification, alias lookup, memory, and execution
ordering still need to be charged.

## Verification and remaining production gate

The baseline `scripts/verify_all.ps1` completed successfully in this working
session: C=2..5, all 71 FJ9 classes with jobs2/results2, canonicalization and
checkpoint regressions, and the read-only G1 checkpoint test. The retained
factorization checkpoint hash remained unchanged.

The new programs are isolated decision/verification tools. Production key
semantics, factorization kernels, and checkpoint formats are unchanged.
`watch_rss.ps1` was adjusted only to launch its background process with a hidden
window; its PowerShell parser check passed.

Next gates are exact pairing-based graph aliases, full-size native lookup cost,
and a safe resumable closed-value implementation if those costs justify it.
The complete N(6) remains unclosed.

## Continuation: exact graph aliases and full catalogue lookup

The subsequent pairing-fiber implementation passed complete C4 L3 and C5
L3/L4 partition tests. Fixed-bipartition/transpose-merged graph counts are
38/33, 1,160/721 and 14,237/12,543 respectively. The first 256 actual uniform
C6 samples agreed field by field with the independent NetworkX referee.
All 1,024 samples used 13,936 admissible pairings; complete fiber construction
took 0.0387401 seconds, of which native canonicalization was 0.0316959 seconds.
The observed transpose-merged reciprocal-fiber mean was 0.153300223777,
standard error 0.005018933011. Its reciprocal is approximately 6.52; this is
not an end-to-end runtime multiplier.

The full generation-37 C6 catalogue was then loaded and indexed read-only,
with the single missing key appended only in RAM. All 903,398,602 original
live keys were distinct. Every one of 314,584 predecessor queries from the
64-source sample hit. Read/payload validation took 12.37 seconds, 24-thread
indexing 7.78 seconds, and the query phase 0.940 seconds. Total elapsed was
21.09 seconds and peak working set 41,119,461,376 bytes (38.30 GiB). Original
partial T values were never interpreted as F. An independent post-run full
SHA-256 agreed with the pinned original digest. Logs are
`data/logs/c6-direct-route-20260905/catalog-lookup-c6.{out,err,rss.csv}`.

## Continuation: one resumable shared-value implementation

`layer_shared_f4.cpp` includes the unchanged native key implementation and
calls the existing degree-four kernel through a separate translation unit.
For every native ID it records the exact minimum-fiber representative ID;
only self-representatives evaluate F4. Source native T is checked as part of
the original byte image, then unconditionally erased before use.

Every immutable chunk contains complete alias decisions and closed
representative values, not a partial numeric sum. Its 256-byte header binds
the exact source SHA-256, source header, support-repair key/stabilizer,
semantic version, stable-ID range and chunk geometry. Payload and header
checksums are verified on resume. Commits use an exclusive temporary,
durable flush and non-replacing rename. A source directory cannot be its
writable chunk namespace. Native export is permitted only after every alias
points to a closed self-representative; target weights are filled separately
from the immutable F4 values.

The complete new gate passed against fresh C4/C5 references. It compares every
exported native key, stabilizer and T, reconstructs every F4 from the NEW
export, and uses those new values in all 355 C5 reverse calculations and an
independent exact N(5) sum. It also tests changed-thread resume, an actual
killed process, unchanged hashes of earlier chunks, poisoned original T,
insertion-hole round-trips, checksum-valid invalid aliases/cycles,
header/payload/lineage corruption, concurrent namespace ownership and export
overwrite refusal. These tests use disposable small-C files only.

Retained complete release logs:

- `data/logs/direct-release-run-b741244578fe468b963df3a7d8c6e3d4/`:
  complete direct-layer release, exit 0, 229.428282 seconds.
- `data/logs/baseline-release-run-25497e0f5a044500be9fb5fc1031a361/`:
  complete repository gate, exit 0, 200.7522464 seconds; optional G1 read-only
  checkpoint gate ran and its full SHA remained unchanged.
- `data/logs/shared-release-run-159da81fb5d649559513d284b3203b72/`:
  initial combined shared-value release, exit 0, 47.1578395 seconds including
  rebuild. A subsequent per-source canonical/stabilizer audit hardening is
  independently small-case checked; its final integrated gate is recorded
  separately when completed.

## Continuation: complete-fiber C6 sample, not global closure

The retained generator `layer_shared_sample.py` expanded the 1,024 uniform
native samples into the union of all their exact graph fibers: 12,345 distinct
native keys in 1,024 graph fibers. Every native value was independently cold
computed with the retained degree-four kernel. The expected-value text is
`data/logs/c6-direct-route-20260905/shared-sample-1024/closure-expected.txt`,
SHA-256 `EFE67D58768282DD34BE074191718D5A73B45AAFA7D7C9180C96B8C8B89236CB`.

The SAME combined engine processed that sample with 24 threads, persisted
13 complete chunks, and checked all 12,345 expected values. It closed exactly
1,024 representatives, whose F4 checksum was 1,512,032,640. End-to-end elapsed
was 0.451629 seconds with a 71,962,624-byte peak. Log:
`data/logs/c6-direct-route-20260905/shared-closure-1024-v1.log`.
The program explicitly labels this a sample and forbids native production
export from this input mode.

That expanded sample is size-biased and its per-native throughput must NOT be
scaled to the whole population. Timing the chosen minimum representatives
and weighting each original uniform draw by its reciprocal fiber size gives
mean `t(rep)/b = 0.0004159235767613867` seconds, standard error
`0.000014347947052703898`. The corresponding representative-kernel estimate
is 104.37 single-thread hours over the complete native population. This is
CPU-work evidence, not a multicore wall-time forecast; classification,
source audits, random access, I/O and parallel efficiency remain separate.
