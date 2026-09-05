# Incremental-DSU shared F4: candidate-only integration, 2026-09-05

The separately linked incremental-DSU bridge passed the actual shared-F4
engine's complete C4/C5 exactness, export, downstream N5 and recovery gates,
plus the 12,345-state C6 complete-fiber sample. Old-version C5 chunks resumed
without computing new records and produced a byte-identical closed export.
This qualifies an isolated candidate, not a production publication. The
released engine and active C6 namespace were not changed by this task.

## Exact API replacement and safety scope

New file `experiments/proto/layer_shared_f4_incremental_bridge.cpp` implements
the existing `shared_f4_bridge.h` API and is linked with the unchanged actual
`layer_shared_f4.cpp`. It uses the exact incremental core and proof from
[the same-binary CPU ablation](c6-cpu-three-kernel-ablation-20260905.md).
No canonicalizer, graph-fiber representative, native ID, coefficient,
checkpoint format, resume rule or output meaning changes.

**(A) Closed-value semantics.** Initialization is protected by a mutex and an
atomic published dimension. C must be 4, 5 or 6; changing C inside a live
process is refused. The bridge independently validates graph size, mask
domain, row degree four and column degree four, then sorts masks in the same
order as the released bridge. It calls the unchanged incremental core with
`UINT64_MAX` as its node limit. The experimental two-million-node cutoff is
not used. Every nonzero core status is rejected; only a positive, fully
closed F4 divisible by 24 and no larger than `24^(2C)` is returned.

The factor 24 divisibility follows from the free permutation action on four
edge colors: every color occurs at each degree-four vertex. The storage bound
follows by ignoring right-side constraints and assigning the four colors to
each left vertex's edges, at most `(4!)^(2C)` assignments. `24^12` fits in u64;
the bridge checks the multiplication bound explicitly. The core also checks
the running sum and final multiplication by six before accepting a value.

**(A) Finite-work bound.** With the first row pair fixed, each remaining row
has at most six pair choices. For n=2C<=12 the no-pruning search has at most
`6^(n-1)` leaves and `sum(d=0..n-1)6^d <= 435356467` entered prefix nodes.
Each node tries at most six outgoing choices and has a bounded completion
step, so traversal counters cannot approach u64 overflow. Pruning only
reduces these counts. This removes reliance on an empirical per-graph node
cap; it is not a promise of low worst-case runtime.

The actual main's unchanged wall/RSS watchdog and closed-chunk transaction
bound the process. A hard interruption can discard the current uncommitted
chunk but cannot turn a partial arithmetic accumulator into a saved value.
The new bridge has no checkpoint, memo, GPU or network access.

## Executed candidate gate

The retained wrapper is
`experiments/proto/layer_shared_incremental_candidate_gate.py`. It compiled
only the candidate into a unique build directory and invoked the existing
`layer_shared_gate.py --shared-exe <candidate> --threads 4`. It then copied a
small previously completed C5 namespace into its disposable output directory
for old-version compatibility. The wrapper SHA-pinned released files, both
included arithmetic cores, the actual gate, old fixtures and C6 sample before
and after, and checked the candidate binary again after use.

**(C) Exact finite results.** All 32 actual-gate checks passed:

- Fresh independent complete C4 and C5 references reproduced their known
  counts. The candidate's 26 C4 and 17,120 C5 F4/native weighted values agreed
  exactly with independent exported-image readback.
- New C5 L4 output supplied all 355 downstream F5 classes and gave
  `N(5)=1903816047972624930994913280000`. The actual reverse core agreed with
  every class using one and four threads.
- Deliberate termination after the first 100-state C5 chunk resumed to full
  agreement and preserved all earlier committed bytes. Concurrent namespace
  access was refused; an orphan temporary was not accepted.
- Malformed header, payload, lineage, alias range, alias-to-hole, hole record,
  alias cycle, chunk geometry, source stabilizer and noncanonical source were
  rejected. Poisoned old T weights were discarded and exact values recomputed.
  Export overwrite was refused without changing the original image.
- All 12,345 native keys in the independently valued C6 complete-fiber TEXT
  sample agreed. The candidate computed 1,024 graph representatives, whose
  unweighted F4 checksum was 1,512,032,640. The output explicitly remained
  `domain=sample_only`, `N6=NOT_COMPUTED`; no complete C6 catalogue was opened.

The additional old-version compatibility test resumed 35 old C5 chunks,
verified all 17,120 native values and reported:

```text
new_chunks=0
new_indices=0
closed_prefix=17120
closed_representatives=12543
F4_checksum_mod2_64=2852302464
```

Every copied old chunk remained SHA-identical. Independent readback found
identical keys, stabilizers and weighted values. Both old and new complete
exports had SHA-256
`E60114E228A328CE6734B1F68E0136017193D7DD20181F3AD97BE3186127D04A`.

## Bounds, provenance and publication status

One shared 360-second / 6-GiB sampled aggregate process-tree guard covered
compilation, the actual gate, and compatibility. The guard tracked only its
own descendants using PID plus creation time, included the Python parent RSS,
sampled every 75 ms, and retained commands, exits, peaks and survivor lists.
Actual elapsed time was 44.4549712 seconds; sampled aggregate peak was
691,953,664 bytes, including compilation. All children exited and every
survivor list was empty. No unrelated production process was controlled.

These checks ran concurrently with the unchanged released C6 F4 window6.
Their durations are resource evidence only, not speedup measurements. The
nonconcurrent hot-kernel timing evidence is in the separate ablation report.

Reproduction (use a fresh output directory):

```powershell
python experiments/proto/layer_shared_incremental_candidate_gate.py --old-fixtures data/logs/layer-shared-release-gate-e820c59908624c1590491e55ab9ae4b1/fixtures --output-dir data/logs/c6-direct-route-20260905/shared-incremental-candidate-v1
```

The wrapper retains the expanded compile and gate commands. The successful
binary is
`build/shared-incremental-candidate-ab431aac5c6643e3adf21f0dc8150708/layer_shared_f4_incremental.exe`.
Artifacts are under
`data/logs/c6-direct-route-20260905/shared-incremental-candidate-v1/`, including
`receipt.json`, `actual-shared-gate.log`, `fixtures/checks.json` and
`old-version-resume.log`.

```text
candidate bridge SHA256:
29FDF9AB35E7854E6CA17BACE90B46F639D6EC2127F933D703E9EDBC57697C3F
candidate bounded wrapper SHA256:
8E6B4032BA8E5AAE3A4C39D26A338F539CAA7DCD95A95129F6718233A07CA74E
candidate binary SHA256:
E4CBAE32917BAD8731BDC051A705DF6A5C48760340F86ED5A2EAD58894EB5032
receipt.json SHA256:
3AF1D123E351E4C71BE981BB176024B8EE616AA3CA18A2304B0D00C2085FE869
fixtures/checks.json SHA256:
2A8A231919AA64FB2707270A909A7AB07E88E77DC38BA5CC771A0ED910A969C0
unchanged released shared-F4 binary SHA256:
45A8387A031C3236191C71C28F73773711EEC34B9A628D4E715B2FE5F8EDD4FC
```

The full repository gate was not rerun or a released binary rebuilt during
the live production window. Parent review, the window's terminal state and a
fresh final full gate remain prerequisites to any promotion. This report
neither changes that decision nor claims a complete C6 F4 export or N(6).
