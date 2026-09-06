# Full-scale shared-F4 audit design — bounded independent branch

The two-pass native streaming audit is implemented and independently qualified.
On 2026-09-06 it passed the complete 903,398,621-ID domain in 166.588572
seconds, certifying complete internal alias/value closure and physical-copy
integrity, not independent reevaluation of all F4 arithmetic. The resumable
shard design remains unimplemented and was not needed. No producer was changed.

## 1. What an incremental audit may inherit

An accepted previous production report supplies a source/repair/semantics
fingerprint, a contiguous audited prefix, exact local-record counters, and a
SHA-256 digest and length for every committed file. A successor must pin the
previous report itself by SHA-256 and reject test/snapshot reports.

For an OLD current file, recompute its whole-file SHA-256 and length and compare
them with that pinned digest. Then its prior header, payload-checksum,
local-value, and counter checks may be inherited. Repeating the old Python
per-record loops is unnecessary. Merely trusting a filename, timestamp, receipt
line, or the application's append-only convention is insufficient.

This transfer of trust uses ordinary SHA-256 collision-resistance assumptions;
it is not an unconditional mathematical deduction that equal hashes imply equal
bytes. Comparing the current and physical-backup byte streams directly can
prove equality of those streams, but a retained digest is still what links them
to a past audit. The mathematical counting identities remain exact; the storage
integrity assumption must not be silently upgraded into such an identity.

For each NEW chunk, independently check the full header/lineage, exact range and
length, payload checksum, alias domain, hole/value convention, positive
self-representative values divisible by 24, and diagnostic counters. Check the
before receipt inventory against the old file list, and the after inventory
against the enlarged list. Read and verify the physical copies, not only CSV
digests. Add new counters to inherited old counters and compare the producer's
resumed, per-new-chunk, and terminal counters separately.

The existing helper rejects prior reports with either `prefix_snapshot` or
`backup_snapshot_test` true. Legacy reports lacking both flags remain accepted.
The new native-audit orchestrator additionally requires one of the three exact
historical audit scopes, or the recognized explicit type
`shared-f4-native-bitset-resume-v1` with all qualification flags false and its
alias check present. A `TEST_ONLY_PASS` node is never a production predecessor.

## 2. Important non-inheritance: enlarged alias closure

An old prefix's resolved-alias count does NOT automatically update when its
prefix grows. Old aliases that formerly pointed outside the prefix may now
resolve; new aliases landing inside the prefix also need a target check.

Therefore an incremental local-record audit must set
`global_alias_closure_checked=false` and omit current resolved-alias totals
unless a separate exact closure operation has actually been performed. It may
retain old counts explicitly as historical evidence. This avoids pretending
that local new-record checks establish the enlarged global closure.

## 3. Exact two-pass closure lemma (self-contained)

Let A[i] be the alias and V[i] the stored representative-only value, on the full
ID domain 0..n-1. First check every local record: holes have NONE/0; live aliases
are in the domain; self aliases have positive values; other aliases have zero
values. Define the bitset

    R[j] = 1 iff A[j] = j and V[j] > 0.

Then the condition that every live alias addresses a closed representative is
equivalent to

    R[A[i]] = 1 for every live i.

Indeed, the definition of R makes membership exactly the conjunction
A[A[i]]=A[i] and V[A[i]]>0. This proves both directions. No orbit weighting,
graph quotient, numerical approximation, or probabilistic test is involved.

For n=903,398,621, the bitset payload is exactly 112,924,828 bytes. There is no
need to retain a 3,613,594,484-byte u32 alias array. A source chunk and a small
I/O buffer are the only other per-record storage needed.

This verifies internal alias/value closure, not the factorization arithmetic or
the graph-equivalence assertion independently of the producer and its proofs.

## 4. Optional bounded shard protocol, not yet implemented

Old-file inheritance still costs real SHA/I/O work. Near the full domain,
current+before+after streams can approach 32.5 GB, and the implemented second
pass rereads current bytes. No claim is made that one native invocation can
finish the complete domain within 180 seconds.

Partition work by explicit committed-file ranges and byte budgets:

1. Verify old file SHA/physical-copy equality in bounded shards. Each completed
   shard certificate binds the complete receipt/lineage root and exact covered
   filename range. Do not emit a successful global report until all expected
   file ranges form an exact disjoint cover.
2. Verify new chunks in bounded record-count shards. Persist only fully checked
   range certificates; interrupted partial accumulators are not accepted.
3. Build representative bitset slabs from fully checked source chunks. Each
   completed slab binds the source chunk SHA, ID range, semantics, bitset SHA,
   and exact representative population. A 25,000-ID slab needs 3,125 payload
   bytes. Assemble/read-map the complete bitset only after all ranges cover the
   full domain and the summed population matches the audited representative
   count.
4. Verify alias membership in bounded source-ID shards against that complete,
   read-only, SHA-bound bitset. Each successful certificate binds both its
   source chunk hashes and the complete bitset root. Full closure is declared
   only after the verified ID ranges cover the whole domain with no gaps or
   overlaps and all hole/live totals agree.

Use at most 1 GiB / 120 seconds per qualification test, retaining finite caps on
new records, shard bytes, and files. A watchdog failure yields no accepted shard.
An orchestration layer can resume from completed certificates; it need not
retain an in-progress bitset as if it were complete.

Certificates bind content, not eternal filesystem immutability. Before export
or reuse, revalidate the relevant current bytes against their pinned hashes.
For a simultaneous filesystem snapshot, hold deny-write/delete file handles
(and suitable directory handles) while verifying and consuming them, or use an
equivalent immutable/content-addressed snapshot. A new process cannot infer
continued immutability merely because an earlier process once held a lock.

## 5. Executed finite evidence

`experiments/proto/layer_shared_bitset_probe.py` opened only the first 21 chunks
of the immutable window3 external backup, covering the already-audited 525,000
ID prefix. Both passes rechecked file SHA-256. The independent bitset reference
reproduced all previous array-audit results:

    representative bitset bytes = 65,625
    closed representatives      = 87,427
    resolved aliases            = 105,290
    future aliases              = 419,710
    holes                       = 0

The report is
`data/logs/c6-direct-route-20260905/shared-bitset-prefix-probe.json`. It is marked
`TEST_ONLY_PASS`, not a production audit node. The run took approximately
0.320 seconds and peaked at 37,502,976 bytes.

The same run tested 1,008 synthetic full-domain fixtures against the direct
array definition: 252 valid cases, 252 missing representative values, 252
out-of-domain aliases, and 252 two-cycles. Both exact predicates agreed with
every specified outcome. This validates the reference identity on those finite
fixtures; it is not a full-scale performance benchmark.

`data/logs/c6-direct-route-20260905/shared-audit-chain-guard-tests/summary.json`
retains the separately executed
acceptance tests: a valid SHA-resealed false-flag predecessor reproduces all
window2 counters, and two independently SHA-resealed predecessors with either
snapshot/test flag true are rejected before producing an output report.

## 6. Implemented bounded successor and current limit

`experiments/proto/layer_shared_bitset_audit.cpp` independently parses the raw
format without producer includes. Its first pass checks every header, payload,
local record, and current/before/after physical copy, and builds only the
representative bitset. The second pass rereads and rehashes current files and
checks every alias into the current prefix against that bitset. Future aliases
remain explicitly uncomputed. The Python orchestrator pins the predecessor,
inventories and receipts, checks resumed/new/terminal counters, and rehashes the
worker and plan before accepting its output. The old array audit is preserved.

The native synthetic gate accepted 10 exact direct-array fixtures of 50,000
records each, rejected 38 malformed cases and 10 invalid predecessor variants,
and retained the 1,008-case algebraic differential. It also reproduced the
historical window1-to-window2 protocol using both physical backups.

Both a same-prefix qualification pinned to the window4 audit and the complete
window3-to-window4 protocol qualification agreed with all 2,388 file records and
all current/new counters. The latter took 8.388 seconds end-to-end, with a
15,769,600-byte worker peak and a 62,271,488-byte parent peak. Its bitset payload
was 7,459,375 bytes. All these qualification reports are `TEST_ONLY_PASS` and
are not production ancestors. Exact commands and hashes are retained in
`docs/reports/og2/shared-f4-native-bitset-audit-20260905.md`.

Production invocations remain capped at 180 seconds, with separate 1-GiB parent
and worker guards, exact domain/file/byte limits, and fail-closed output. The
full-domain bitset size is proved above; the completed full-domain measurement
is recorded below. No resumable shard implementation was added speculatively. File
certificates describe verified content/provenance at the reads, not a
simultaneously locked or permanently immutable namespace; the export controller
provides stronger locking when it consumes a complete catalogue.

## 7. Completed full-domain audit without counter inheritance (2026-09-06)

Window8 ended under a hard time bound after overnight sleep, before receiving
an independent prefix audit. Window9 then completed the domain. The strict
single-successor audit remains unchanged: no fake window8 predecessor or
sanitized stderr was inserted into its chain.

`experiments/proto/layer_shared_complete_audit.py` instead reuses the SAME
SHA-qualified native reader to decode every record, build the full bitset,
recheck every alias and compare the physical before/after copies for window9.
It independently sums the decoded prefix counters to check window9's RESUMED
line, all new chunk counters and final summary. A pinned window7 report is
used only to check preservation of its 18,104 files, never to supply counters.
The final controller must report matching before/after copies and exit zero;
the final engine stderr must be empty. New parser/counter refusal tests and
the existing 10/38 native acceptance/rejection gate pass.

All 903,398,603 live aliases resolved to closed self-representatives, with
18 holes, 140,069,579 representative values and zero unresolved aliases.
The unchanged reader used a 112,924,828-byte bitset. Its worker time was
165.431 seconds; end-to-end time was 166.588572 seconds under the existing
180-second limit. Worker/parent peaks were 145,981,440 / 171,200,512 bytes,
each capped at 1 GiB. Every committed byte was re-read; no local-record or
closure counter was inherited from an unaudited computation.

This does not independently reprove graph-fiber equivalence or recalculate
each F4, export weighted native T4, compute F5, or close N(6). The record-level
source/repair/semantics checks and storage-snapshot caveats above still apply.
Exact commands, hashes and limits are in
`../reports/og2/c6-shared-f4-complete-audit-20260906.md`.
