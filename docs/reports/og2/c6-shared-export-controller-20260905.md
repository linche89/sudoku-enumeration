# Closed-only shared-F4 export controller qualification

Date: 2026-09-05. The separate controller passes complete C5 export and
safety tests. No C6 native F4 export exists; N(6) remains uncomputed.

## Scope and prerequisite

The released shared producer's `checkpointreadonly` switch protects its
original native input, not the new shared-value namespace. A disposable C5
prefix probe actually committed one new 500-ID chunk despite that switch.
Thus `checkpointreadonly export=...` alone is not an export-only operation.

The new `scripts/export_layer_shared.ps1` leaves the producer unchanged. Its
independent header preflight requires the entire contiguous source-matched
namespace. It retains deny-write/delete handles on both copies of all input
files and pins their directory ancestors while checking and consuming them.
It requires full alias closure with zero new chunks/IDs, a new non-overwriting
native output, independent streaming serialization/count/mass readback, and
a fresh separately hashed physical output copy. A receipt is created only
after all those conditions pass. Independent read-only review found no
concrete blocker in that control flow.

The complete command shape and remaining full-scale handle/I/O limitations
are in `../../runbooks/layer-shared-c6.md`, section "Export a completely
closed F4 namespace". The current C6 prefix fails its full-domain prerequisite.

## Executed qualification

```powershell
python experiments/proto/layer_shared_export_gate.py `
  --fixtures data/logs/layer-shared-release-gate-e820c59908624c1590491e55ab9ae4b1/fixtures `
  --full-gate data/logs/final-baseline-release-871e5c39f9914f13a7f27043adc4973c/stdout.log `
  --shared-gate data/logs/shared-final-release-run-054b103055624bfc81358dfe81ba89c6/stdout.log `
  --direct-gate data/logs/direct-release-run-b741244578fe468b963df3a7d8c6e3d4/stdout.log
```

The harness constructs a fresh independent C5 row-incremental oracle and
compares all 17,120 exported native keys, stabilizers and exact T values.
Only disposable C5 files are writable. The actual producer invocation used
`chunk=500 limit=500 threads=1 workseconds=1 maxseconds=55 maxrssgib=2`,
with a one-minute external child guard. Readback used a 30-second internal
bound and 2 GiB; the harness bounds each controller process by 90 seconds.

```text
complete-export: exit 0
fresh-oracle comparison: all 17120 keys/stabilizers/T equal
new_chunks: 0
new_indices: 0
exported bytes: 616448
live / holes: 17120 / 0
raw orbit mass: 62185328
local and external output SHA256:
E60114E228A328CE6734B1F68E0136017193D7DD20181F3AD97BE3186127D04A
positive controller wall: 3.2969228 seconds
overwrite-refused: exit 1, expected reason matched
incomplete-refused: exit 1, expected reason matched
internal-gap-refused: exit 1, expected reason matched
existing-writer-refused: exit 1, expected reason matched
```

The harness separately checks that subsequent input writes/deletions are
denied while the controller holds its locks and that original committed
chunks and released engine bytes remain unchanged. It never deletes a
production input to construct a missing-chunk fixture.

Primary artifacts:

- `data/logs/shared-export-gate-2ztu1df8/checks.json` and corresponding logs.
- `data/logs/shared-export-gate-2ztu1df8/new-closed.L4.snap.receipt.json`.
- `data/logs/shared-f4-export-20260905-154043-4cfc9792d8834444856496aa7f41e6a2/`.
- Physical copies under `D:/sudoku_FJ_checkpoint_backups/shared-export-gate-e04112fc32d54a26899c0eca5f120a69/`.

The unchanged producer SHA is
`45A8387A031C3236191C71C28F73773711EEC34B9A628D4E715B2FE5F8EDD4FC`;
the independent support-reader SHA is
`5E92E26EA5D2516B7B0A37C6C6CCEF3F7C42E55EFEA1576857A3264642888961`.

Independent readback checks the full file's SHA, header/payload checksums,
support counts and stored-stabilizer mass. It does not independently
reevaluate all C6 F4 values or verify every future C6 T against its alias
chunk. Those stronger numerical provenance claims are not made here.
No full-scale 36,136-chunk handle or 32.53-GB export timing is established.
