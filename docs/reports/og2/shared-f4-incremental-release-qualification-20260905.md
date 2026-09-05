# Shared F4 incremental kernel: final candidate qualification, 2026-09-05

The final source-selected candidate passed complete actual-engine numerical,
export and recovery gates, plus old-version chunk compatibility. It was not
published by this qualification. The live C6 window6 continued with the
unchanged previously released binary; all timings below are concurrent
resource evidence, not speedup measurements.

This is a new immutable report. The earlier
[candidate integration report](shared-f4-incremental-candidate-gate-20260905.md)
and [three-kernel ablation](c6-cpu-three-kernel-ablation-20260905.md) remain
unchanged. The exact arithmetic and bridge proof are those already recorded
there; this run adds final source/build selection qualification, not new
mathematics.

## Final selected source and build

The initial functional candidate was committed as `0d09387`. Before this
qualification, the parent prepared these narrowly scoped source changes:

- The standard `build_layer_shared.ps1` selects the incremental bridge by
  default, retaining `-ReferenceKernel` for the unchanged old bridge.
- The controller's freshness checks include the incremental bridge and both
  packed-DSU header dependencies.
- The candidate wrapper SHA-pins both scripts and uses `-Wall -Wextra` in
  addition to the existing safe optimization/OpenMP flags.
- The bridge's introductory comment reflects the selected standard build;
  its arithmetic, initialization, domain/overflow checks and API are unchanged.

Only a fresh candidate executable was compiled, directly from the actual
unchanged shared main and final bridge. Neither the standard build script nor
the controller was executed. No released executable was replaced.

```text
Final candidate:
build/shared-incremental-candidate-b88fafa2456046d19c4ae43934412a25/layer_shared_f4_incremental.exe
SHA256:
2B8A14606F8F78748C9338130D35551CA199E53238BEB33D95BCB05A2555AFE0

unchanged live/released build/layer_shared_f4.exe SHA256:
45A8387A031C3236191C71C28F73773711EEC34B9A628D4E715B2FE5F8EDD4FC
```

## Exact executed gate

The single authorized wrapper invocation was:

```powershell
python experiments/proto/layer_shared_incremental_candidate_gate.py --old-fixtures data/logs/layer-shared-release-gate-e820c59908624c1590491e55ab9ae4b1/fixtures --output-dir data/logs/c6-direct-route-20260905/shared-incremental-final-qualification-v1
```

Its retained compile command used
`g++ -O3 -mpopcnt -std=c++20 -fopenmp -Wall -Wextra`, the actual
`experiments/proto/layer_shared_f4.cpp` and final incremental bridge, and
`-lbcrypt -lpsapi`. Compilation succeeded with an empty compiler log.

**(C) Exact finite certificate.** All 32 existing actual-engine gate checks
passed. Fresh independent complete C4/C5 references agreed with every new
26/17,120 native F4 value and weighted export. All 355 downstream F5 values
and the exact square sum reproduced
`N(5)=1903816047972624930994913280000`. The reverse core agreed with one and
four threads. Source-weight discard, hole preservation, overwrite refusal,
header/payload/lineage/alias/source corruption, concurrent namespace refusal,
and real kill/resume tests all passed with the same checks as the earlier
integration gate.

The independently valued C6 complete-fiber TEXT sample also passed:

```text
domain=sample_only
exact_verified_native_values=12345
new_chunks=13
new_indices=12345
closed_representatives=1024
F4_checksum_mod2_64=1512032640
N6=NOT_COMPUTED
```

No complete C6 source catalogue or production namespace was opened by the
candidate. The sample is not a new C6 population or complete count.

The separate compatibility step copied the old completed C5 namespace into
the new disposable output directory. It resumed all 35 old chunks, required
`new_chunks=0`, `new_indices=0`, and verified all 17,120 native values. All
copied committed bytes were preserved. Independent native readback found
identical keys, stabilizers and weighted values; the old and new exports were
also byte-identical, both SHA-256
`E60114E228A328CE6734B1F68E0136017193D7DD20181F3AD97BE3186127D04A`.

## Bounds and immutable evidence

The one shared bound was 360 seconds and 6 GiB sampled aggregate RSS across
the Python parent and tracked compiler/gate descendants. It covered all three
child commands. Total elapsed time was 44.7071671 seconds and sampled peak
aggregate RSS was 702,050,304 bytes. All children exited zero; all tracked
survivor lists were empty. Every pinned source, script, fixture and released
binary was unchanged after the qualification, and the final candidate binary
was rehashed after all tests.

Artifacts:
`data/logs/c6-direct-route-20260905/shared-incremental-final-qualification-v1/`.
The receipt retains the complete expanded commands, before/after SHA pins,
resources, process cleanup and compatibility evidence.

```text
receipt.json SHA256:
77B0939DA12C6F17278E402E5073DBF2AD5B0C3E064B9CF81B9DF0252085A49D
actual-shared-gate.log SHA256:
56CA0CD97F7711A439799B08A11A793DAB06A441590FA068EDE52D4FDD9932C0
fixtures/checks.json SHA256:
597A02DFD2F2A8BB1A0A5AB932EFEC43CF8817210D5CCF19D4D01E10B58981FE

final incremental bridge SHA256:
5A2FF6035474A6565C0D2D27F281DE5AB642C855BFEF10FC4F3B49CE9A67F8D2
incremental arithmetic core SHA256:
477495EDA0AB7E34A6AB1A766498E14B7D45ECF92B361F19956BB78DCA599A03
final bounded wrapper SHA256:
9DC84721FFDADBA62ED3E46D2DBCABDA4244527620D3666E936C522972DFECEC
build_layer_shared.ps1 SHA256:
1302115D0EDD989C797D0BA1278F8D7DBDEB8EDC4999A091A4194F217F5B1476
run_layer_shared_window.ps1 SHA256:
2AC038009BB7AA81DF479257E4361FE1BDD45A357705762D7352EA8498773272
```

This report's terminal status is **candidate qualified, unpublished**.
Publication remains contingent on parent review, window6 and backup completion,
a fresh final full repository gate, and a current-path `SkipBuild` shared gate
after any byte-for-byte publication. Those later actions are not claimed here.
