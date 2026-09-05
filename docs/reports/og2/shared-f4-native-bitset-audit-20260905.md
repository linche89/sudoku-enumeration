# Independent native shared-F4 bitset audit qualification — 2026-09-05

The new independent audit reproduces the existing 59,675,000-ID audit exactly
without retaining a full alias array. This is byte/backup/alias integrity
verification, **not independent recalculation of F4** and not a result for N(6).
No producer, checkpoint, controller, or existing audit helper was changed.

## Implementation and scope

The new files are:

- `experiments/proto/layer_shared_bitset_audit.cpp`: independently decoded raw
  headers/payloads, Windows SHA-256, hard native bounds, two-pass closure.
- `experiments/proto/layer_shared_bitset_audit.py`: SHA-pinned production
  predecessor admission, exact receipt/inventory plan, child invocation,
  worker/plan post-run SHA, resumed/new/terminal counter checks, exclusive report.
- `experiments/proto/layer_shared_bitset_audit_gate.py`: disposable synthetic and
  historical-protocol qualification; all generated files stay under `data/logs`.

The first native pass reads and hashes each current file and physical after
copy, plus the before copy for previous files, checking exact byte equality.
It independently checks all raw format, lineage, payload and local value
conditions. It marks a bit iff a record is its own alias and stores a positive
multiple-of-24 F4. The second pass rereads and rehashes current files and checks
all aliases landing inside the prefix against this bitset. Aliases beyond the
prefix are retained as future/uncomputed, never treated as closed.

The full-domain bitset payload is exactly 112,924,828 bytes. No full-domain
timing was measured. Both parent and worker have hard bounds of at most 180 s
and 1 GiB individually; actual qualification was far below the combined 2-GiB cap.
Storage claims concern verified content/provenance at the reads, not a
simultaneous or permanently immutable namespace snapshot. SHA links to past
receipts retain the ordinary collision-resistance assumption.

New production report type: `shared-f4-native-bitset-resume-v1`. Only an exact
recognized historical production scope or this explicit type with all required
false qualification flags is accepted as a predecessor. Same-prefix replay or
`--protocol-test` always emits `TEST_ONLY_PASS`, which cannot enter that chain.
The existing array audit is unchanged.

## Executed finite gates

Build and retained final small gate:

```powershell
g++ -O3 -mpopcnt -std=c++20 experiments/proto/layer_shared_bitset_audit.cpp -o build/layer_shared_bitset_audit.exe -lbcrypt -lpsapi
python experiments/proto/layer_shared_bitset_audit_gate.py --worker build/layer_shared_bitset_audit.exe --historical-window2 --output-dir data/logs/c6-direct-route-20260905/shared-native-bitset-gate-v3
```

Exact outcomes:

- 10 accepted native 50,000-record fixtures agreed with the independent direct
  array predicate and all exact counters, including holes, future aliases,
  many disconnected random components, and u64 additive-checksum wrap.
- 38 malformed native cases rejected: local values/aliases/cycles; resealed
  incorrect lineage, range, counters and reserved bytes; payload/header hashes;
  manifest record fields; missing/trailing/malformed plan information; physical
  current/before/after corruption; trailing input bytes; invalid SHA/time/RSS.
- 1,008 finite algebraic fixtures agreed between the original direct predicate
  and independent bitset predicate (252 valid and 252 of each of three failures).
- 10 poisoned or incomplete predecessor forms rejected. Both a recognized
  historical predecessor and a complete recognized new type were admitted.
- The actual historical window1-to-window2 protocol, using a disposable local
  copy and both original external backups, reproduced all prior exact totals.

The final small gate took 4.067 s, peak parent RSS 68,169,728 bytes. Its summary is
`data/logs/c6-direct-route-20260905/shared-native-bitset-gate-v3/summary.json`,
SHA-256 `5213C398F1C0EF6734CA9F8599544F3804520147826A39BDBBFCD8A5F10C56CD`.

## Actual window4 differential

The same-prefix test pinned the already accepted window4 JSON:

```powershell
python experiments/proto/layer_shared_bitset_audit.py --namespace data/checkpoints/c6_shared_f4_20260905 --backup-after D:/sudoku_FJ_checkpoint_backups/c6_shared_f4_20260905/20260905-152513-9796d789d4a444f6b4053bc84dcde8ca-after --previous-report data/logs/c6-direct-route-20260905/shared-window4-independent-audit.json --previous-sha256 4B800029BA9108B59E7BFBFA1853A92CBF1030CEE307E545C08FB7280FB6EA70 --worker build/layer_shared_bitset_audit.exe --expected-prefix 59675000 --max-bytes 1073741824 --maxseconds 180 --qualification-replay --output data/logs/c6-direct-route-20260905/shared-window4-native-bitset-qualification.json
```

An additional full resumed-protocol qualification also checked the window4
before backup, all producer resumed/new/terminal counters, and empty stderr:

```powershell
python experiments/proto/layer_shared_bitset_audit.py --namespace data/checkpoints/c6_shared_f4_20260905 --backup-before D:/sudoku_FJ_checkpoint_backups/c6_shared_f4_20260905/20260905-152513-9796d789d4a444f6b4053bc84dcde8ca-before --backup-after D:/sudoku_FJ_checkpoint_backups/c6_shared_f4_20260905/20260905-152513-9796d789d4a444f6b4053bc84dcde8ca-after --previous-report data/logs/c6-direct-route-20260905/shared-window3-independent-audit.json --previous-sha256 F2F6258AC9187DAF5D813AB30665A88A1ABE428DD9AF97D9E9BF69E15C85283D --worker build/layer_shared_bitset_audit.exe --stdout data/logs/shared-f4-window-20260905-152513-9796d789d4a444f6b4053bc84dcde8ca/stdout.log --expected-prefix 59675000 --max-bytes 1073741824 --maxseconds 180 --protocol-test --output data/logs/c6-direct-route-20260905/shared-window4-native-bitset-resume-qualification.json
```

Both passed with `TEST_ONLY_PASS`. The resumed-protocol result agreed with all
17 selected previous/new totals and receipt hashes, and all 2,388 complete
per-file records from the original accepted audit.

| Exact quantity | Result |
| --- | ---: |
| Contiguous prefix | 59,675,000 |
| Chunks / total files | 2,387 / 2,388 |
| Current committed bytes | 716,711,328 |
| Closed representative F4 values | 9,345,067 |
| Additive value checksum | 13,043,638,453,248 |
| Aliases to closed representatives | 15,991,860 |
| Future/uncomputed aliases | 43,683,140 |
| Holes in this prefix | 0 |
| New chunks / IDs | 1,966 / 49,150,000 |
| New representatives / checksum | 7,655,609 / 10,692,950,954,304 |
| Bitset payload | 7,459,375bytes |

Replay: 7.863 s end-to-end, 7.397 s worker. Full resumed protocol: 8.388 s
end-to-end, 7.908 s worker; worker peak 15,769,600 bytes, parent peak 62,271,488 bytes.
These bounded timings are not a guarantee or extrapolation for the full domain.
Both modes produced bitset SHA
`EE6C4ADCA59B476ED2CA63DD53974C0C1668563FC63722D45DBAAEB641CA57F7`.

Retained qualification JSON hashes:

- `shared-window4-native-bitset-qualification.json`:
  `2C0E6D91989B6A08C4FE83B3EC6DE9AB5D3ACF9775C526AC0368CE777C314351`.
- `shared-window4-native-bitset-resume-qualification.json`:
  `43CFB85AC0CEEE39077577185D9F8976B1AFD4C9353D78FF7D4C3E3FF785D660`.

Each JSON retains its exact native command, plan SHA, worker SHA, all per-file
SHA/payload hashes and counters, and the pinned predecessor and receipt hashes.
Plans and raw native JSONL/stderr are retained alongside each report.

## Frozen source identities and handoff

| File | SHA-256 |
| --- | --- |
| `layer_shared_bitset_audit.cpp` | `68D2444D2767101F93035410E0F5C540C40C42F669327B1C61DBE59FB8052CF5` |
| `layer_shared_bitset_audit.py` | `EC679E2EB7C4E7A4E652E05F0D0D103238A8191EE59A7EE03C35EACE95C44A5C` |
| `layer_shared_bitset_audit_gate.py` | `0C9A5DE00448706DCF671D4184E8B41AA89F372DC5E41096697396720C77E992` |
| `build/layer_shared_bitset_audit.exe` | `DD541C9318334DCA8763FEBD66E60B81F51CF35D2887456EEBFD9DD804F7D4C4` |

`git diff --check` passed; no native audit process remained. No counting kernel
changed, so the independent helper's own exact gate was run rather than another
full numerical counting gate. Full903m wall time, production use beyond the
observed prefix, and resumable closed audit shards remain unqualified/unmeasured.
