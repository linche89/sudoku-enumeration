# Geometry-first RAM rekey: bounded small gates, 2026-09-05

Status: exact RAM-rekey differentials passed on complete C5 and the fixed
512-source C6 sample. **No full C6 catalogue rekey or production S2 speedup
has been established by these runs.** The full-index diagnostic is implemented
but had not been launched when this report was written.

## Meaning of the alternate key

This is Lemma B in
[`two-missing-canonical-prefix.md`](../../math/two-missing-canonical-prefix.md),
not the compatible-prefix Lemma A now used by the released reverse engine.

For a balanced layer with degree `C-2`, each symbol is absent from exactly two
boxes, and every box is absent from four symbols. The missing pairs form a
loopless 4-regular multigraph on the boxes. Canonicalize that multigraph first;
then minimize the native key over all box permutations transporting it to its
chosen representative and all side flips. The resulting key is a different
representative of **the same native orbit**. It does not quotient out box
pairings or identify different native response rows.

The allowed transporter sets are covariant under the native coordinate group.
Consequently, the new keys agree exactly when the old native keys agree.
All minimizing transformations constitute a full stabilizer coset, so the
stabilizer is unchanged. The earlier proof and complete small-layer invariance,
separation and histogram checks remain applicable; the new code reuses that
geometry implementation unchanged.

If a source catalogue has native key `k_i`, stabilizer `s_i`, and value `v_i`
at stable ID `i`, replace only its in-memory key by `g(k_i)` and rebuild the
index. A query of raw state `x` must then satisfy

```text
old_index[native(x)] = i = new_index[geometry(x)].
```

No orbit factor is introduced. Values and stabilizers stay attached to their
original IDs, and the checkpoint on disk remains in the native format.

## Retained isolated helpers

- `experiments/proto/layer_geometry_rekey_bench.cpp`: complete C5 or a bounded
  C6 sample-only index; deliberately refuses a full C6 catalogue argument.
- `experiments/proto/layer_geometry_rekey_full_bench.cpp`: one-index parallel
  diagnostic, independently gated below on complete C5. It also has an explicit
  full C6 readonly mode, not exercised in this report.

Neither helper edits a checkpoint or any released engine. Both require positive
limits. Small executions use one compute thread, at most 1 GiB and 120 seconds;
the second helper's small parallel gate uses at most two compute threads with
the same memory/time bounds. An independent watchdog samples RSS/time every
100 ms and terminates the process on violation.

The C5 source reader verifies the complete native checkpoint payload and SHA,
all source keys and exact stabilizers, support size and orbit mass, then divides
each weighted `T3` by its orbit size to obtain closed `F3`. Target native keys
must be distinct. In the C6 sample-only gate, stored values are zero and are
never treated as factorization values.

## Final small-helper execution

Build:

```powershell
g++ -O3 -mpopcnt -std=c++20 -fopenmp -Wall -Wextra experiments/proto/layer_geometry_rekey_bench.cpp -o build/layer_geometry_rekey_bench.exe -lpsapi -lbcrypt
```

Commands:

```powershell
build/layer_geometry_rekey_bench.exe 5 input=data/logs/layer-direct-gate-20260905-fixtures-v2/c5.L4.txt catalogue=data/logs/layer-direct-gate-20260905-fixtures-v2/c5.L3.snap limit=17120 maxqueries=3500000 maxseconds=120
build/layer_geometry_rekey_bench.exe 6 input=data/logs/c6-direct-route-20260905/l5-sample-512.txt calibration=data/logs/c6-direct-route-20260905/l4-sample.txt limit=512 maxqueries=3000000 maxseconds=120
```

Both exited 0. Logs are in
`data/logs/geometry-rekey-small-release-951c09a6380042e2ad192679099b225c/`.

| Exact quantity | Complete C5 L3 to L4 | C6 sample query catalogue |
| --- | ---: | ---: |
| Target sources | 17,120 | 512 |
| Rooted labelled matchings | 3,375,557 | 2,691,041 |
| Weak residual queries | 2,935,081 | 2,471,101 |
| Distinct catalogue IDs | 16,150 | 2,464,705 |
| Keys changed by rekey | 15,276 | 2,464,506 |
| ID and stabilizer agreement | Every query | Every query |
| Duplicate rebuilt keys | 0 | 0 |
| Maximum simultaneous indexes | 1 | 1 |

C5 IDs are the actual IDs in the complete source checkpoint. C6 IDs in this
table are **local sample catalogue IDs, not IDs in the full 903-million-state
catalogue**.

C5 source orbit mass was `59,661,280`. The complete F3-to-F4 differential
reproduced every one of the 17,120 target values and the complete joint
value/stabilizer histogram. Both indexes gave:

```text
sumF4=3972941184
sumOrbitF4=14365876248576
stable-ID checksum=25290434741
```

The C5 source stabilizer histogram was:

```text
1:15013,2:977,3:1,4:123,5:3,6:3,8:20,10:2,12:1,20:4,24:3
```

The C6 local catalogue stabilizer histogram was:

```text
1:2462272,2:2405,4:27,8:1
```

Checksums of the ID-indexed value and stabilizer payloads were identical before
and after rekey. For C5 this SHA-256 was
`DA493AC67A391F2F5F7A87BFC3CAA961DDBCE4823FFE30E10E8302EC7296C3DA`;
for the C6 sample it was
`DF9C34984B5F441A3428A948E2A8D1C56ADCD66F97214E9777F13D44DADC65CB`.
These fingerprints supplement, rather than replace, the individual query-ID
and exact-stabilizer comparisons.

All timings below were concurrent with a production F4 window and are not
production extrapolations. C5 total wall time was 14.5622519 seconds, sampled
peak RSS 125,874,176 bytes. C6 total was 20.1589430 seconds, peak RSS 230,109,184
bytes. C6 compatible-prefix queries took 5.4651557 seconds; geometry-key queries
took 4.0857942 seconds. Rekey took 3.9822198 seconds and rebuilding the **small**
index took 0.1317758 seconds. The respective query DFS node counts were
43,660,709 and 32,985,190.

The small helper also refused, before source loading, all three tested invalid
requests: a full C6 catalogue argument, zero source limit, and 121 seconds.
Each refusal exited 1.

## Parallel one-index implementation gate

Build and executed command:

```powershell
g++ -O3 -mpopcnt -std=c++20 -fopenmp -Wall -Wextra experiments/proto/layer_geometry_rekey_full_bench.cpp -o build/layer_geometry_rekey_full_bench.exe -lpsapi -lbcrypt
build/layer_geometry_rekey_full_bench.exe 5 input=data/logs/layer-direct-gate-20260905-fixtures-v2/c5.L4.txt catalogue=data/logs/layer-direct-gate-20260905-fixtures-v2/c5.L3.snap limit=17120 maxqueries=3500000 maxseconds=120 maxrssgib=1 threads=2 rounds=2 checkpointreadonly
```

The final binary SHA-256 was
`9228BF8BAA8395C6E32A3CEA20241DB85887D7411DEE8C1FF22136A12BA3FE7E`.
The run exited 0 with `GEOMETRY ONE INDEX REKEY DIFFERENTIAL PASSED` after
22.7007695 seconds; sampled peak RSS was 137,117,696 bytes. Logs are in
`data/logs/geometry-one-index-c5-release-837a93c183fb4bb1bd9b580f68fef1fe/`.

This version first used the original, non-prefix native canonicalizer to
establish every source ID. One-thread and two-thread prefix passes then checked
all 2,935,081 IDs and stabilizers. After the old index was destroyed, two threads
rekeyed disjoint ID ranges, audited every source's balanced two-missing domain
and exact stabilizer, and rebuilt one CAS index with zero duplicates. All
one-/two-thread geometry queries agreed with the original native IDs and the
complete C5 value chain above. Missing explicit C6 acknowledgement and a wrong
C6 sample SHA were also tested and refused before catalogue loading.

Thread safety is by ownership: each rekey iteration owns one `keys[id]`;
stabilizers and values are immutable; each worker has independent canonicalizer
scratch; the prebuilt missing-geometry inventory is read-only. No query executes
during key mutation. Index insertion uses atomic compare-and-exchange, and all
keys are immutable again before rebuilding starts. Exceptions are collected
under a mutex and make the phase fail, rather than accepting a partial result.

## Full C6 proposal, NOT executed here

The proposed run has exactly 512 pinned sample sources, 24 compute threads,
270-second internal and 300-second external limits, and a 55-GiB RSS limit.
It must wait for explicit release of the production F4 resource slot.

It loads the existing pinned source read-only, verifies its full payload SHA,
discards all old partial T values, and applies the already certified missing-key
repair **only in RAM**. It uses the existing loader unchanged. Generic native
queries first record the exact stable IDs and stabilizers. Prefix query passes
precede rekey; geometry passes follow it. There is one whole-table rekey, no
rekey back and no second full index. Every new query must reproduce its recorded
ID and exact stabilizer; all-source balance/stabilizer checks and zero duplicate
rebuilt keys are mandatory. The ID-indexed T/stabilizer fingerprint must remain
identical. No C6 F4 or F5 value is interpreted or returned.

Raw query records require 98,844,040 bytes; recorded original query stabilizers
require another 9,884,404 bytes. ID records are included in the first figure.
These are additional to the one existing catalogue, not another large table.
Source loading/hashing/zeroing, old index construction, query phases, rekey,
new index construction, and payload fingerprints are timed separately.

The uniform native L4 sample contains 1,024 original sampled records. A
separate calibration repeated these same records 256 times: 262,144 geometry
calls took 0.450891 seconds on one compute thread, concurrent with F4. Repetition
does not increase the number of original sampled records. Literal scaling gives
about 1,554 serial CPU-seconds for 903 million keys; dividing by 24 gives an
**idealized** 64.7 seconds, not a measured parallel result or upper bound.
Validation, memory effects, source load, table rebuilding, hashing and imperfect
parallel scaling are additional. A 150–240-second planning allowance is therefore
only a heuristic; the diagnostic must fail closed at its hard bounds if it
does not finish. No projected production-hour saving is accepted at this stage.

## Input fingerprints and scope

```text
C5 L3 snapshot:
974481F15BAED8894A36DDDBA668D61180817CA18AD26FF5F1835E48B61A06AB
C5 L4 target text:
DBF5BCDEC67D5B16E9B1D1F2F1D972341A0AA029C99629DD2951F17A80848B43
C6 uniform L5 sample512:
B535C873117F8583CA5069A16D2430767DAFC79FCFE6949325ABE9B5F853CE25
C6 uniform L4 sample1024:
38F4B8EF65FA4BEEF7FB05995E2A64B7096162A5E1212A0388AF5A9E195CA99C
```

No production or checkpoint file was changed. No N(6), closed C6 F5 vector,
full-size rekey throughput, or general graph-compression gain is claimed.
The complete repository gate was not rerun for these isolated prototypes while
the production window was active; the proportional exact gates are recorded
above. Existing released engines and their dependencies were not edited.
