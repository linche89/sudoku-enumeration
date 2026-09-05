# Compatible-prefix release for the exact reverse-F5 engine

Date: 2026-09-05. The maximum-missing-edge canonical prefix is now released
in the actual reverse-F5 executable after complete coefficient, final-count,
recovery and old-version compatibility gates. No C6 numerical F5 values or
N(6) were computed. The shared-F4 producer and its released executable were
not changed or rebuilt by this release.

## Exact scope of the change

`src/layer_two_missing_prefix_canon.h` is byte-identical to the previously
tested geometry-free extraction (SHA-256 below). It requires unseeded,
balanced C2..6 states in which every symbol misses exactly two boxes. It
retains all ordered maximum-missing-edge ties, both side flips and every
subsequent box choice. Other inputs use the original native canonicalizer.
In particular, the one-missing C5 L4 predecessor path uses that fallback.

The proof is Lemma A in `docs/math/two-missing-canonical-prefix.md`: all
first-column signatures tie, and the first group of the second signature
strictly prefers the largest missing-edge intersection. Every globally
minimizing coordinate transformation remains in the search, so both the
native representative and the stabilizer remain unchanged. This does not
merge distinct native responses, change any recurrence coefficient, or
introduce the distinct geometry-first representative scheme.

The actual main remains `experiments/proto/layer_reverse_f5.cpp`; no source
reorganization was mixed into the change. A narrowly scoped macro substitutes
the new function only while including `layer_reverse_f5_core.h`. The header
is compiled first, so its `::canonize` fallback cannot recurse. Native
catalogue loading, source audits, repairs, stable IDs, index keys, checkpoint
formats and lineage checks are unchanged. The build hook adds a prefix gate,
and the production controller's source-freshness list includes the new header.

The exact gate includes the actual production translation unit under
`REVERSE_F5_NO_MAIN`, including this dispatch. Its C5 L3-to-L4 coefficient
test exercises the two-missing function on a complete finite domain. The
ordinary production C5 F5 test alone would exercise only the one-missing
fallback and is not used as a substitute for that coefficient test.

## Candidate build and exact release gate

The released reverse binary was preserved until all tests passed. The
candidate build used fresh paths and these safe flags:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/build_layer_reverse.ps1 `
  -ReverseExe build/reverse-prefix-candidate-gvq4k4uy/layer_reverse_f5.exe `
  -SupportTestExe build/reverse-prefix-candidate-gvq4k4uy/layer_reverse_f5_support_test.exe `
  -PrefixGateExe build/reverse-prefix-candidate-gvq4k4uy/layer_two_missing_prefix_gate.exe
# g++ -O3 -mpopcnt -std=c++20 -fopenmp -Wall -Wextra ... -lbcrypt -lpsapi

powershell -NoProfile -ExecutionPolicy Bypass -File scripts/verify_layer_reverse.ps1 `
  -Threads 4 -SkipBuild `
  -ReverseExe build/reverse-prefix-candidate-gvq4k4uy/layer_reverse_f5.exe `
  -PrefixGateExe build/reverse-prefix-candidate-gvq4k4uy/layer_two_missing_prefix_gate.exe
```

Build exit 0, no warnings: 24.2937535 seconds, sampled aggregate process-tree
RSS peak 744,665,088 bytes, under an external 180-second/8-GiB guard.
Build logs: `data/logs/reverse-prefix-candidate-build-ny3064ik/`.

The final candidate tests used an external aggregate 360-second/8-GiB guard.
The wrapper passed in 38.4869478 seconds; its Python numerical/recovery suite
took 37.4340691 seconds. The positive C6 text probe and old-version resume
described below brought the whole guarded sequence to 40.3446421 seconds,
with sampled aggregate RSS peak 136,728,576 bytes. Every process exited 0.
The mandatory wrapper marker remains `PRODUCTION REVERSE F5 CHECKS PASSED`.

All fresh C5 references were generated independently by the original
row-incremental engine, including L3, L4, L5 and all 355 reference classes.
The added exact outputs were:

```text
C5 L3: sources=16150 queries=16150 distinct=16150 transforms=129200
full_group_referees=8
histogram=1:15013,2:977,3:1,4:123,5:3,6:3,8:20,10:2,12:1,20:4,24:3
seeded_fallback=YES bad_field=REFUSED high_bits=REFUSED padding=CHECKED

COMPLETE_C5_L3_L4 predecessors=16150 targets=17120
labelled_matchings=3375557 weak_queries=2935081
all_term_keys_stabs_coefficients_equal=YES all_F4_equal=YES
predecessor_transforms=129200 sumF4=3972941184 sumOrbitF4=14365876248576
dispatch=2951231 fallback=17120
```

The complete coefficient chain took 11.259987 engine seconds and peaked at
9,482,240 bytes. It checks every predecessor key/stabilizer, all raw-query
multiplicities, every target F4 value, uniqueness, histogram and fallback.

The existing actual production chain also passed:

```text
fresh shared F4: all17120 native weighted values agree
reverse F5: first100 IDs at1 thread, resume all355 at4 threads
fresh native L5 export: per-key AND byte-identical to independent reference
N(5)=1903816047972624930994913280000
new L5 SHA256=E6FBEFB1541E8B9B859144164283BFC0668488BDE3A03BC7ECBD9FE90DEDAD5A
```

An actual process was killed after its first committed one-ID F5 chunk;
resumption preserved that chunk's SHA and completed the same exact export.
The suite rejected bad headers/payloads, both input-lineage corruptions,
zero or nonfactorial closed values, changed chunk geometry, export
overwrite, wrong L4 SHA, missing read-only flag, input/namespace overlap,
bad target stabilizers, noncanonical target keys and invalid hole metadata.
Poisoned old target weights were erased and recomputed; legal holes were
preserved and nonzero hole values rejected. All protected input and export
SHA-256 values were unchanged. The existing native final-stage loader
accepted the new L5 export and independently reproduced all355 classes/N5.

Detailed fixtures/checks are under
`data/logs/layer-reverse-release-gate-b690f04bb0ca419e9b3604836b465725/`.
The final external command/resource receipt and authoritative wrapper stdout
are under `data/logs/reverse-prefix-final-gate-6omauh6p/`.

## C6 branch and old-version compatibility

The actual-production-TU prefix gate also ran a positive-limit text-only
probe; no full catalogue or numerical F4 values were loaded:

```powershell
build/reverse-prefix-candidate-gvq4k4uy/layer_two_missing_prefix_gate.exe 6 5 `
  input=data/logs/c6-direct-route-20260905/l5-sample-512.txt `
  limit=64 maxqueries=500000 maxseconds=120 invariance=128
```

```text
sources=64 queries=314584 distinct=314117 labelled_matchings=337680
transforms=1024 full_group_referees=8 histogram=1:314223,2:360,4:1
dispatch=314584 fallback=0 production_keys_unchanged=YES
engine_seconds=1.762505 peak_rss_bytes=24600576
```

The sample SHA was checked against
`B535C873117F8583CA5069A16D2430767DAFC79FCFE6949325ABE9B5F853CE25`.
This confirms the actual C6 dispatch branch, not a numerical F5 result.
Earlier same-table, all-query native-key/stabilizer/ID comparisons and
isolated lookup-only timings are retained separately in
`c6-compatible-prefix-lookup-20260905.md`. Their roughly 1.25x--1.28x
query speedup is not a measured end-to-end production S2 speedup.

For explicit backwards compatibility, the previous release's complete C5
namespace (eight chunks plus manifest) was physically copied to the new
test directory. The candidate resumed it with its original immutable L4/L5
inputs, `chunk=50 limit=355 threads=4 workseconds=30 maxseconds=45
maxrssgib=2 checkpointreadonly`, a fresh export path and all355 expected
values. It reported:

```text
RESUMED_F5 chunks=8 closed_prefix=355 total_entries=355
EXACT_VERIFIED_F5_NATIVE_VALUES=355
new_chunks=0 new_indices=0 closed_prefix=355
F5_checksum_mod2_64=545872980480
```

Every copied manifest/chunk SHA was unchanged. The new export was
byte-identical to the old independent C5 oracle, with the same L5 SHA above.
External elapsed was 0.0417296 seconds. Nothing in the old namespace was
modified; all experiment writes were to new disposable paths.

## Baseline regression and publication receipt

The full `powershell -NoProfile -ExecutionPolicy Bypass -File
scripts/verify_all.ps1` also passed during this release cycle, under an
independent external 360-second/aggregate-8-GiB guard. Its exact elapsed was
191.6011702 seconds, sampled aggregate RSS peak 1,106,804,736 bytes, exit 0,
and final marker `ALL REPOSITORY CHECKS PASSED`. All 82 tracked processes
exited. It included mandatory C2..5 counts, future-twin C5 differential,
all FJ9 references and the C6 G1 read-only checkpoint check. Shared-F4,
the then-released original reverse executable, and the G1 memo retained
their SHA, size and modification time. Logs are in
`data/logs/final-baseline-handoff-2d39da8955204ee9b8198c423d59c0e7/`.
This gate did not rebuild either new production engine. Its baseline
executables were fully released before the candidate numerical gate began.

After all candidate checks, the old reverse executable was copied to:

```text
build/reverse-prefix-prior-d1f3b5f3cdf64b1091daa3de43f45d3a/layer_reverse_f5.exe
SHA256=C7BDBC4DEA0FE8497787D7DB127A9B718D591BCA5F59B9394C64F6E11F295761
```

That physical copy was SHA-verified before replacing `build/layer_reverse_f5.exe`
with the exact tested candidate bytes. The candidate remains at its original
new path. The new prefix gate was copied to a previously absent standard
build path. Normal file copying preserved the compiled timestamp; no time
was manipulated to bypass controller freshness checks.

```text
released reverse bytes=544705
released reverse SHA256=E09123878A3098EFF955AA4DA1CCAF33B875F9CCEF7D9F337DF28F3ED08AFB9B
build UTC=2026-09-05T08:14:02.7501545Z
successful wrapper evidence UTC=2026-09-05T08:17:19.6957049Z
prefix-gate SHA256=7EFF14BEFFA88250A471998C9419681EDF1D718EAA86B43289038AC0EDC5E6B2
src header SHA256=450FEFB0FEB62E6E2CA29218F96101E7304FCB10A5C419F7B8CE090D2107082E
reverse source SHA256=5F00FD6B1A35CF86409B20E5759BDCE14A5C3473809CBAFFA29C11616EA3011D
unchanged shared-F4 SHA256=45A8387A031C3236191C71C28F73773711EEC34B9A628D4E715B2FE5F8EDD4FC
```

The explicit candidate-to-release receipt is
`data/logs/reverse-prefix-final-gate-6omauh6p/publication-receipt.json`;
the gate stdout SHA is
`2CEC6C63A45E7D75E0899C210700F8B0D2416325B88C74839C354E1B6E125157`.
Both released paths were rehashed after publication. PowerShell parsing,
Python parsing and `git diff --check` pass. No long-running computing
process from this release remains. No checkpoint was overwritten, moved or
deleted. The original executable is recoverable at the explicit retained
path above. Full C6 F5 and S3 remain gated on a genuinely closed F4 export
and separate authorization; this release does not claim N(6).
