# Full-scale shared-F4 audit design — bounded independent branch

This is a design and a bounded reference test, not a full-scale implementation
or a certificate that the F4 catalogue is complete. No producer was changed.

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

The existing helper now rejects prior reports with either `prefix_snapshot` or
`backup_snapshot_test` true. Legacy reports lacking both flags remain accepted;
future schemas should require explicit false flags and a recognized production
audit type. A `TEST_ONLY_PASS` node is never a production predecessor.

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

## 4. Bounded full-scale protocol, without lifting all limits

Old-file inheritance still costs real SHA/I/O work. Near the full domain,
current+before+after streams can approach 32.5 GB. No claim is made that one
Python invocation can finish that work within 120 or 180 seconds.

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

## 6. Current decision

The existing bounded full-prefix helper remains suitable for window4 once its
writer has stopped, with explicit max-entries=100,000,000 and an adequate byte
budget for its at-most-60,525,000 prefix. Its actual 180-second/4-GiB guard stays
in force. No bound is lifted for the 903-million full domain here.

The incremental receipt-chain/sharded protocol above is not yet promoted or
implemented as a complete production audit. The bitset reference and admission
hardening are the only new executable changes in this branch.
