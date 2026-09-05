# Full C6 geometry RAM-rekey diagnostic, 2026-09-05

**Passed:** the complete repaired L4 support was rekeyed in RAM, preserving
all stable IDs and exact stabilizers, with zero duplicate rebuilt keys. Every
one of the 2,471,101 sampled predecessor queries returned its original native
ID and stabilizer. This establishes an exact, measured lookup optimization
candidate, **not a completed F4/F5 evaluation, production S2 speedup, or N(6)**.

This is a new report. The earlier
[`small-gate report`](c6-geometry-ram-rekey-small-gate-20260905.md) remains an
immutable account of the tests completed before this full diagnostic.

## Authority, preflight and bounds

One full diagnostic was explicitly authorized after the fifth F4 window,
its backups/audits, and the complete repository gate had terminated. No other
heavy workload was active. There was no retry, cap increase, source edit,
production-engine rebuild, or checkpoint write for this diagnostic.

Before launch, the frozen executable SHA and pinned sample SHA matched; the
source and retained external copy both had 32,522,350,448 bytes; generation,
entry and hole fields agreed with the protected source. Available RAM was
106,891,554,816 bytes. The executable then validated the complete source
payload/checksums and full SHA before accepting query results.

A preflight-only assertion initially confused the documented key/value tag
with the physical header magic. Current code and the 128 consumed header bytes
agree on:

```text
CK_MAGIC     = 0x314B434C4A464453 = SDFJLCK1
CK_VERSION   = 2
CK_ALGO_TAG  = 0x4C445043414E3031 = LDPCAN01
```

`LDPCAN01` identifies key/value semantics, not the physical file magic. This
was corrected in the read-only preflight interpretation; no binary or source
checkpoint was changed. The actual full diagnostic was launched once.

The internal watchdog enforced 270 seconds and 55 GiB. An independent inline
Python/psutil 7.2.2 guard enforced 300 seconds and **aggregate** 55 GiB across
the executable and descendants, sampling every 0.2 seconds. It tracked PID and
creation time, would kill the exact remaining tree on a bound failure, and
confirmed no surviving descendant after completion. Output files were created
exclusively in a new log directory; the process had no visible window.

## Exact command and provenance

```powershell
E:\Code\sudoku_FJ\build\layer_geometry_rekey_full_bench.exe 6 input=E:\Code\sudoku_FJ\data\logs\c6-direct-route-20260905\l5-sample-512.txt catalogue=E:\Code\sudoku_FJ\data\checkpoints\layer_dp_c6_s1_prod_20260802.a limit=512 maxqueries=3000000 maxseconds=270 maxrssgib=55 threads=24 rounds=3 checkpointreadonly repair-l4-support ack-large-c6
```

Executable SHA-256:

```text
9228BF8BAA8395C6E32A3CEA20241DB85887D7411DEE8C1FF22136A12BA3FE7E
```

The unchanged isolated sources were
`experiments/proto/layer_geometry_rekey_full_bench.cpp` and
`experiments/proto/layer_geometry_rekey_bench.cpp`, with respective SHA-256:

```text
80D3B6C51B55239FFA1B436BF9ADFCCE384D18536828628D07CC4E3517584EE5
512DAA62B4A73A329F1713BA28A96B0F52E9475F7B732F77046A7D7497071D5B
```

The consumed native source SHA was:

```text
ECF0837315B0FDF8AE21C394FDA6676490E43B1828A0825529A344EC17E4E844
```

The external copy existed at:

```text
D:\sudoku_FJ_checkpoint_backups\layer_dp_c6_s1_prod_20260802\session-008-20260810-170245-layer_dp_c6_s1_prod_20260802.a
```

The fixed 512-source uniform-record sample SHA was:

```text
B535C873117F8583CA5069A16D2430767DAFC79FCFE6949325ABE9B5F853CE25
```

All logs are retained under:

```text
data/logs/geometry-one-index-c6-full-a56833f5dffe4c19913a32f2cc24cc56/
  stdout.log
  stderr.log
  rss.jsonl
  guard-summary.json
```

`stdout.log` SHA-256 is
`B3CCED5077B0FB9444BD16BBAE14B45E4C76FE63E7A1E3049427B305BA9B64C2`;
`guard-summary.json` SHA-256 is
`0E6D76CC2012C2352F843331D375544D7EDEAB88B443007FE6CB7E2305AFAF5B`.

## Complete-support and query certificates

The original protected source had 903,398,620 IDs, 18 holes, and 903,398,602
live native keys. Its old T payload was validated for file integrity and then
entirely zeroed. The already proved missing native L4 witness was appended
**only in RAM**, using the unchanged loader's
`L4-stab12-order3-witness-v1` repair. Thus the loaded support was:

```text
IDs including holes = 903398621
live keys           = 903398603
holes               = 18
```

The original generic native canonicalizer established every reference query
ID and stabilizer. Compatible-prefix queries matched those references before
the key replacement. The old index was then destroyed; keys were transformed
in place, independently by ID, using the exact geometry-first representative
of the **same native orbit**. Every live source passed the balanced two-missing
domain check and exact stored-stabilizer comparison. One new index was built
using duplicate-checked atomic insertions.

```text
live keys rekeyed                 = 903398603
holes preserved                  = 18
representative keys changed      = 903363712
source canonicalization DFS nodes= 12198810650
duplicate rebuilt keys           = 0
maximum simultaneous indexes     = 1
```

The ordered ID-indexed T/stabilizer/size/hole fingerprint was identical before
and after the transformation:

```text
AAB6D16334FF076EC62950E294840308A752C123BB5D8F428AAFA08BCFBCF8BA
```

Here T was **all zero after erasure**, not a closed C6 F4 vector. The earlier
complete C5 gate exercised preservation and use of actual nonzero closed F3
values. This full diagnostic did not interpret any old T as F4 or produce F5.

### Additional exact consequence: the L4 support premises are now checked

The full scan also supplies a stronger support certificate than raw-key
uniqueness. Its domain validator checked every live state's fields, degree,
padding and slot balance. Each is therefore a legal native two-missing
multiset, whether or not its stored ordering is the original canonical minimum.
By the proved separation part of Lemma B, two such states have the same
geometry key if and only if they are in the same native coordinate orbit.
Zero duplicate geometry keys thus proves **native-orbit uniqueness** over the
entire repaired support, not merely distinct byte strings. The minimizing-path
count recomputed every exact coordinate stabilizer and agreed with its stored
value.

The independently established
[`two-missing Burnside count`](../../math/two-missing-layer-burnside.md) is
903,398,603 native L4 orbits. The repaired catalogue now contains exactly that
many legal, pairwise inequivalent orbits. Equality of these finite cardinalities
proves that the repaired support is complete. In particular, the original file
is missing exactly the one appended witness orbit; that conclusion no longer
needs to assume the old forward enumerator's legality or orbit-uniqueness
guarantees.

This deduction relies on the proved/audited geometry canonicalizer and the
independent Burnside count; it is not an independent reimplementation of every
shared primitive. Original-native canonical **minimality** was not rechecked
for every source, and is not needed for this completeness argument. Legality
as a multiset also does not require a particular row ordering. The regular
bipartite matching-decomposition invariant underlying native states supplies
their ordered-row realizability, as in the earlier support proof.

This strengthened certificate applies to **L4 only**. No analogous all-orbit
L5 audit was performed here. Clearing T for a performance-only diagnostic does
not close, recompute, or validate any numerical C6 F4 value.

The fixed target sample generated:

```text
source targets              = 512
rooted labelled matchings    = 2691041
weak predecessor queries    = 2471101
recorded query bytes        = 98844040
recorded stabilizer bytes   = 9884404
ID/multiplicity checksum    = 1203952348773697
```

Every geometry query matched the **actual full-catalogue native ID** and exact
original stabilizer, individually, in every query pass. These are not local
sample IDs. The displayed checksum supplements the per-query comparisons; it
is not the sole certificate. The factorization values themselves were not read.

## Charged phase times

All times are seconds. No competing heavy workload was active.

| Phase | Seconds |
| --- | ---: |
| Rooted sample query generation, one thread | 0.7450968 |
| Source payload reading/checking | 12.1150504 |
| Full source SHA-256 | 12.1873931 |
| Erase old T | 0.1622997 |
| Initial native index | 8.3055322 |
| Total catalogue load | 32.7707630 |
| Original generic-native reference queries, 24 threads | 0.3603792 |
| T/stabilizer fingerprint before | 4.0985936 |
| Full per-ID domain/stabilizer-audited rekey, 24 threads | 67.3544150 |
| Rebuild one geometry-key index, 24 threads | 7.6278576 |
| T/stabilizer fingerprint after | 4.1444481 |

The one-time rekey plus new index construction cost **74.9822726 seconds**.
The initial index is needed by this differential benchmark to obtain native
reference IDs; whether a production implementation can safely omit that
initial index requires its own integration review and gate.

| Query phase | Compatible prefix | Geometry RAM key |
| --- | ---: | ---: |
| One thread | 5.6823696 | 4.6656597 |
| 24 threads, pass 1 | 0.2526301 | 0.1911474 |
| 24 threads, pass 2 | 0.2526630 | 0.1929301 |
| 24 threads, pass 3 | 0.2545579 | 0.1948317 |
| Mean of the three 24-thread passes | 0.2532836667 | 0.1929697333 |
| DFS nodes per complete query pass | 43,660,709 | 32,985,190 |

The observed 24-thread query-phase ratio is **1.31256**, or **23.8128% less
elapsed time**. The one-thread ratio is 1.21791. The rounds reuse the same
sample and are not independent population samples. Prefix phases necessarily
preceded geometry phases because the benchmark forbade another whole-table
rekey; this was not an interleaved A/B experiment.

These are canonicalization-plus-membership query timings on pre-generated raw
residuals. They exclude production per-source matching generation, ordinary
value arithmetic, chunking, serialization, resume and export costs. The measured
ratio must not be called a 1.31-times production S2 speedup or converted directly
into a promised number of saved workstation hours.

## Terminal status and integrity scope

```text
start (UTC+08:00)    = 2026-09-05T17:20:30.183406+08:00
end (UTC+08:00)      = 2026-09-05T17:22:40.381009+08:00
engine wall seconds = 128.7954786
external seconds    = 130.20440219999
exit code           = 0
external guard      = no violation
engine peak bytes   = 41230544896
aggregate peak bytes= 41237979136 (38.4058609 GiB)
tracked processes   = 2
surviving processes = 0
stderr              = empty
```

The final marker was `GEOMETRY ONE INDEX REKEY DIFFERENTIAL PASSED`.
Observed RSS dropped by about 8.59 GB when the old index was freed, then rose
again when the new index was built. The source size/mtime were unchanged;
executable and sample hashes were rechecked and unchanged. The source's full
SHA authenticates the bytes consumed by the loader; the final freshness check
is size/mtime, not a second full-file hash or a persistent deny-write lock.
No checkpoint was opened for writing, replaced, deleted, or moved.

The result justifies considering a separately gated, RAM-only optional S2
integration. It does not justify changing disk key semantics, recomputing F4,
discarding native response distinctions, altering chunks, or declaring N(6)
closed. Released engines were not changed or rebuilt for this report.
