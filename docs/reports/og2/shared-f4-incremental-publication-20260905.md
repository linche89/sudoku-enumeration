# Incremental shared-F4 production publication — 2026-09-05

The qualified incremental packed-DSU kernel is now installed at the production
shared-F4 path. Complete installed-path exactness/recovery checks passed.
No complete C6 F4 export, numerical F5 layer or N(6) is claimed here.

## Qualification and preserved semantics

The final candidate and its unchanged arithmetic are documented in
`shared-f4-incremental-release-qualification-20260905.md`. That immutable
report precedes publication. Complete C4/C5 values, every downstream C5 class,
the exact N(5), 12,345 independently valued C6 sample states, and zero-new-work
resume/export of an old-version C5 namespace all passed.

Only the bridge's arithmetic implementation changes. Native keys, IDs,
stabilizers, weights, minimum-fiber alias conventions and immutable chunk
formats remain unchanged. The original bridge remains selectable with
`scripts/build_layer_shared.ps1 -ReferenceKernel`. Do not build either variant
over a live computing executable.

The balanced isolated C6 sample measured a 1.442x arithmetic-kernel improvement
at 24 threads. This is not a measured production-window or total N(6) speedup.
See `c6-cpu-three-kernel-ablation-20260905.md` for exact scope and timings.

## Fresh full repository gate

After window6, its external backup and the independent prefix audit terminated,
the exact command was:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/verify_all.ps1
```

The existing `TreeBound` helper imposed 360 seconds and 6 GiB aggregate RSS
on the Python parent and tracked process tree. All required C2..5 values,
canonicalization differentials, layer recovery checks, FJ9 reproduction and
read-only G1 checkpoint check passed. Elapsed time was 192.0563915 seconds,
sampled aggregate peak 1,132,818,432 bytes, exit 0, 113 tracked processes,
no surviving descendant and no guard firing. The full gate does not rebuild
the shared executable.

Logs: `data/logs/shared-incremental-full-release-arnxql7o/`.
Receipt SHA256:
`28476E7B2E00A0EA33CF31CC135375E6C2B892B2489E62BC8489E13627AC996D`.
Outer stdout SHA256:
`6C34A9BC50D1F3A1AB496AF5B931B9A0D321414FB881D61B090570A1F2BD8CBC`.

## Recoverable byte-for-byte publication

Immediately before publication, all 57 final-qualification SHA pins were
rechecked, including the prior production binary and source/build/controller
files. The window6 audit SHA and fresh full-gate receipt were checked, and no
installed shared-F4 process remained.

The candidate was copied to a fresh build-local temporary and hashed. The old
installed executable was first physically copied to a fresh external path
and both old copies were hashed. `System.IO.File.Replace` then atomically
installed the staged candidate while retaining an additional local rollback
copy. Installed, candidate, local rollback and external rollback hashes were
all rechecked. No checkpoint file was replaced or removed.

```text
installed: build/layer_shared_f4.exe
bytes: 576287
SHA256: 2B8A14606F8F78748C9338130D35551CA199E53238BEB33D95BCB05A2555AFE0

candidate:
build/shared-incremental-candidate-b88fafa2456046d19c4ae43934412a25/layer_shared_f4_incremental.exe

local old executable:
build/20260905-incremental-1304cd84a90747b6adc847583dd62f15/layer_shared_f4_reference.exe
external old executable:
D:/sudoku_FJ_checkpoint_backups/shared_f4_executables/20260905-incremental-1304cd84a90747b6adc847583dd62f15/layer_shared_f4_reference.exe
old bytes: 1587447
old SHA256: 45A8387A031C3236191C71C28F73773711EEC34B9A628D4E715B2FE5F8EDD4FC
```

The four-file publication receipt is
`build/20260905-incremental-1304cd84a90747b6adc847583dd62f15/publication.csv`,
SHA256 `97D2337618C81364DB335A03BB2358F056BBA20DCE9B10F6E80C937C2406B237`.
The candidate's original mtime was preserved; all listed controller source
dependencies predate it. The build-selection change is commit `7a93256`.

## Actual installed-path gate

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/verify_layer_shared.ps1 `
  -SkipBuild -Threads 4 -SecondsPerProcess 120 `
  -C6SampleText data/logs/c6-direct-route-20260905/shared-sample-1024/closure-expected.txt
```

The installed binary's SHA was checked before and after. The same external
360-second / 6-GiB aggregate guard covered this invocation. All 32 exactness,
export and recovery checks passed, including all complete small-domain values
and the C6 complete-fiber sample. Elapsed time was 27.4173113 seconds,
sampled aggregate peak 173,465,600 bytes, exit 0, 14 tracked processes and no
surviving descendant. The nested gate reported 26.339 seconds.

Outer logs: `data/logs/shared-incremental-installed-release-pq4fj7dy/`.
Receipt SHA256:
`E33712B9075D4F2F9A83CB1A36D27CD9898A8FBD53466C39851B6ED3C7D78D73`.
Outer stdout SHA256:
`D44BA273D6F3F64409FE828E2822A06AAB1CA926574CB6F7EE8D0464FCA85884`.
Nested fixtures: `data/logs/layer-shared-release-gate-30c9273c1e03412bb291d5e2778155fb/`.

The next bounded controller must consume the **outer** stdout paths above,
which carry the required successful gate markers. Its timestamp checks are
not substitutes for this retained SHA provenance. No intervening executable
build or arithmetic source edit is permitted without requalification.
