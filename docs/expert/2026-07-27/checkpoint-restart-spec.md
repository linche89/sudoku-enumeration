# checkpoint-restart-spec (agent report, 2026-07-26 pre-flight sweep)

## Summary

Checkpoint/restart + telemetry design for the 7-day C=6 run of E:/Code/sudoku_FJ/experiments/proto/layer_dp_gate.cpp: superstep barrier-quiesce checkpointing of the child Layer (keys/T/stab + cursor; hash table rebuilt on load), atomic-rename versioned file pairs with xxh64 checksums, an exact resume story that needs no partial-chunk repair because dumps only ever contain fully-applied parents, a per-minute telemetry monitor thread, a 52 GB peak-RAM / ~72 GB peak-disk budget, and an ordered 7-patch plan estimated at 19 hours (16-24 h range).

## Details

DESIGN: checkpoint/restart + telemetry for layer_dp_gate.cpp (E:/Code/sudoku_FJ/experiments/proto/layer_dp_gate.cpp)

Grounding in the current code
- Transition driver: lines 990-1048; parent loop `for i < nParents` with `omp for schedule(dynamic,8)`; reads layers[L].keys/T/is_hole only, writes layers[L+1] only via Layer::find_or_add_mt (lines 445-480) and relaxed atomic adds to next->T (EmitCtx::leaf, lines 544-601).
- Layer (lines 380-490): keys (State = array<u16,12>, 24 B), T (u64), stab (u32, 0 = hole), table (u32, idx+1, linear probe), claimed/holes (u32, __atomic ops). Table is fully derivable from keys+stab (skip holes), so it is never dumped.
- Determinism facts: child T is a commutative sum; stab is a pure function of the key (canonize, line 347); entry ORDER and hole placement are race-dependent and semantically irrelevant. Parent layer key ORDER is race-dependent across builds -> resume must reload parents from disk, never rebuild multithreaded.
- EmitCtx rc-cache is reset per parent by prepare(); no cross-chunk state. --rank/Rrows must be disabled in production (2.1e12 stored pairs otherwise).

Production shape (C=6): 3->4: 12.3e6 parents, 2.1e12 emissions, ~3.5 days (aggregate ~144 ns/emission), child ~9.4e8 entries, cap ~1.0-1.05e9, table 2^31 slots. 4->5: 9.4e8 parents, 2.4e12 emissions, ~4 days, child ~1.0e8. 5->6: 5e9 emissions, ~15 min wall. Final contraction (63,199 classes, stab_scan over |G|=46080): minutes, no checkpoint needed.

================ (1) QUIESCE-AND-DUMP ================
Restructure the parent loop into supersteps. Chunk = 1e5 consecutive parent indices (holes included in the range; the existing is_hole skip stays). Superstep = N chunks. Outer serial loop over supersteps; inner `#pragma omp parallel for schedule(dynamic,1)` over the superstep's parent range; the parallel region's implicit barrier at region end IS the quiesce. Then the master thread dumps the child layer + cursor, then the next superstep opens a new region. Region open/close overhead is nil at these superstep lengths. schedule(dynamic,1) bounds barrier-tail waste to one parent's runtime (~25 ms average x fan-out skew; worst parents ~seconds).

Dump contents (child layer only; parent is immutable during the transition):
  keys[0..claimed), T[0..claimed), stab[0..claimed), claimed, holes, tableLog2, cursorChunk.
Dump mechanics: write to <name>.tmp with plain buffered fwrite in <=1 GiB slices, streaming xxh64 over the payload as it is written; fsync (_commit on Windows) the file; atomic rename (rename() / MoveFileExW with MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH) to trans<L+1>.g<gen>.ck; fsync the directory (POSIX); delete generations older than the last 2.

Dump cost estimate (3->4, full size): payload = claimed x 36 B = 9.4e8 x 36 = 33.8 GB. NVMe sustained sequential write 1.5-3 GB/s (consumer post-SLC-cache low end, datacenter high end) -> 12-23 s write; xxh64 at >10 GB/s adds <4 s (overlappable per-slice); fsync+rename ~1 s. Call it 17-34 s at end-of-run size, proportionally less early (payload grows with claimed).

Checkpoint period recommendation:
- 3->4: 123 chunks total, ~41 min wall per chunk (302,400 s / 123). N = 1 chunk => ~41 min period. Worst-case dump overhead 34 s / 2460 s = 1.4% (average ~0.7% since payload grows); crash loses <= ~42 min.
- 4->5: 9,400 chunks, ~37 s/chunk. N = 64 => ~39 min period; dump is only ~3.7 GB (1.0e8 x 36 B) => 2-4 s, overhead ~0.1%.
- 5->6: same driver, N = 64; dump is ~2.3 MB, trivial; the whole transition is ~15 min so at most one checkpoint fires.
Rule of thumb encoded in the driver: checkpoint after every superstep, size supersteps to ~40 min.

================ (2) RESUME PATH AND EXACTNESS ================
Exactness scheme chosen: BARRIER-ONLY DUMPS (option "only dump at chunk boundaries fully processed"), and on crash simply re-run everything from the last checkpoint image. This is the simplest exact scheme and strictly dominates the alternatives:
- The checkpoint is only ever taken at a full barrier after superstep S completes, so the image contains contributions from EXACTLY the parents in chunks [0, S+1) and zero contributions from any later parent. There is no such thing as a partially-contributed parent ON DISK. The in-memory partial state of the interrupted superstep dies with the crash; it was never externalized.
- On resume, chunks >= cursorChunk are re-run against the image. Because the image contains zero contributions from them, additive re-application is exact (u64 addition is commutative/associative, even mod 2^64). Re-insertion of keys first seen after the last checkpoint is harmless: stab is recomputed identically (pure function of key), and different hole/order outcomes vs. the crashed run do not affect the logical multiset {key -> (T, stab)}.
- Rejected: count-and-subtract (log per-parent (childIdx, delta) contributions and subtract on resume) — requires ~171k-entry undo logs per in-flight parent, journaling, and replay logic, all to save at most ~40 min of recompute per crash. Rejected: private per-thread child buffers merged at chunk end — doubles peak memory pressure on the hot path and complicates find_or_add_mt for zero exactness benefit.

Resume sequence (on --resume):
1. Load finalized layer snapshots layer1..layerL.snap (finalized = a checkpoint whose cursorChunk == nChunks; format is unified, see below). This restores parent key ORDER exactly, so chunk index ranges mean the same parents.
2. Find newest valid trans<L+1>.g*.ck (magic, version, headerHash, payloadHash all verified; fall back to the older generation if the newest is corrupt/torn).
3. Verify h.parentKeysHash == xxh64 over the loaded parent layer's keys bytes and h.parentCount == parent size (guards against resuming over a mismatched/rebuilt parent).
4. Reconstruct child Layer: keys/T/stab loaded straight into vectors sized to cap; table.assign(1ull<<tableLog2, 0); claimed/holes restored; then layer_rebuild_table_mt: parallel loop i in [0, claimed), skip stab[i]==0 (holes are duplicates of some winner key and MUST NOT be inserted), hash keys[i], CAS-claim a linear-probe slot with value i+1. Keys are unique among non-holes so no equality re-checks needed. ~9.0e8 random CAS into an 8.6 GB table, memory-bound: roughly 30-90 s with 16+ threads.
5. Restore emissionsSoFar/wallSoFar into the stats accumulators (so the final printed emissions line and ETA stay truthful), then enter the superstep loop at cursorChunk.
Transition finalization: the last superstep writes one final checkpoint with cursorChunk == nChunks (this IS the layer snapshot — unified format, no separate finalize copy), then deletes the older generation, then the next transition starts. layers[3] (and below) can be freed after 3->4 finalizes (keep the .snap files).

================ FILE FORMAT (unified checkpoint/snapshot) ================
Little-endian, fixed 4096-B header then three raw arrays:
  offset 0    : CkptHeader (below)
  offset 4096 : keys payload   claimed x 24 B
  then        : T payload      claimed x 8 B
  then        : stab payload   claimed x 4 B

struct CkptHeader {                    // all u64/u32/double, packed to 4 KiB with zero padding
  u64 magic;                           // 0x314B434C4B445353 "SSDKLCK1"
  u32 version;                         // 1
  u32 c;                               // 6
  u32 layerIdx;                        // child layer stored (2..6)
  u32 flags;                           // bit0 reserved (completeness is cursorChunk==nChunks)
  u64 gen;                             // monotone generation counter
  u64 claimed, holes;                  // stored as u64, cast-checked to u32 on load
  u64 tableLog2;                       // 31 for layer 4
  u64 chunkParents, cursorChunk, nChunks;
  u64 parentCount, parentKeysHash;     // xxh64 over parent keys bytes; 0 for layerIdx<=2
  u64 emissionsSoFar; double wallSoFar;
  u64 keysBytes, tBytes, stabBytes;    // redundancy check vs claimed
  u64 payloadHash;                     // xxh64(seed=gen) streamed over keys||T||stab
  u64 headerHash;                      // xxh64 over the header with this field zeroed
};
Directory layout: ckpt/ { layer1.snap layer2.snap layer3.snap layer4.snap layer5.snap trans5.g0007.ck trans5.g0008.ck run.log } where layerK.snap is just the finalized .ck renamed. Torn-write safety: tmp + fsync + atomic rename + keep-last-2 means a crash at ANY instant leaves at least one fully-valid prior generation; checksums detect torn/bit-rotted files and trigger fallback to the older generation.

================ FUNCTION SIGNATURES (patch surface) ================
// hashing (self-contained ~40-line xxh64, or CRC32C via _mm_crc32_u64; no deps)
static u64 xxh64(const void* p, size_t n, u64 seed);
struct Xxh64Stream { void update(const void*, size_t); u64 digest(); };

// checkpoint I/O
static bool ckpt_write(const Layer& child, CkptHeader h, const std::string& dir, std::string& err);            // tmp+hash+fsync+rename+prune(2)
static bool ckpt_load_latest(const std::string& dir, u32 layerIdx, Layer& child, CkptHeader& h, std::string& err); // newest valid gen, fallback to older
static bool ckpt_load_snapshot(const std::string& dir, u32 layerIdx, Layer& out, CkptHeader& h, std::string& err); // layerK.snap
static void layer_rebuild_table_mt(Layer& L, int nthreads);   // parallel CAS insert of non-hole indices
static u64  layer_keys_hash(const Layer& L);                  // xxh64 over keys[0..size) bytes

// driver (extracted from main lines 990-1048)
struct RunCkptCfg { std::string dir; u64 chunkParents = 100000; u32 chunksPerCkpt = 1; bool resume = false; };
struct TransStats  { u64 emissions = 0, cacheHits = 0; double seconds = 0; };
static void run_transition_ckpt(int L, Layer& parent, Layer& child, size_t childCap,
                                int nthreads, const RunCkptCfg& cfg, TransStats& st);

// telemetry
static size_t cur_rss_bytes();       // GetProcessMemoryInfo WorkingSetSize / parse /proc/self/statm
struct Telemetry {
  void start(const std::string& logPath, int transL, u64 parentsTotal, double emTotalEst,
             const std::atomic<u64>* parentsDone, const std::atomic<u64>* emissions, const Layer* child);
  void note_ckpt(u64 gen, double dumpSecs);
  void stop_and_flush();
};                                    // owns a std::thread waking every 60 s

CLI additions: --ckpt-dir D  --ckpt-chunk P (default 100000)  --ckpt-every N (default 1)  --resume  --log F. Workers add two relaxed fetch_adds per parent (g_parentsDone += 1, g_emissions += per-parent delta from ctx.emissions): ~2 RMW per ~171k emissions, unmeasurable.

================ (3) TELEMETRY ================
Dedicated std::thread (not an OpenMP thread), 60 s cadence, appends one line to ckpt/run.log and fflushes:
  ts=2026-07-28T04:12:00Z trans=3-4 chunk=57/123 parents=5700000/12300000 em=9.73e11/2.10e12 rate_em=6.94e6/s rate_par=41.2/s eta=39.4h claimed=612345678/1050000000 holes=1234 rss=47.9G ckpt_gen=57 ckpt_age=12.3m dump_last=18.2s
ETA from emissions fraction when a fan-sample extrapolated total is supplied (--fan-sample machinery already exists, lines 903-942), else from parent fraction; print both rates. claimed/holes read with __atomic_load_n relaxed from the live child Layer; rss via cur_rss_bytes(). emissionsSoFar restored from the header keeps rates/ETA continuous across resume. The monitor also alarms (log line prefixed ALERT) when claimed/cap > 0.90 (capacity-abort early warning, see risk 1) and when rate drops >30% below trailing average.

================ (4) MEMORY PLAN CHECK ================
During 3->4 (checkpoints active):
  layer3: 12.3e6 x (24+8+4) = 443 MB + table 2^25 x 4 = 134 MB            = 0.58 GB
  layer4 (cap 1.05e9): keys 25.2 + T 8.4 + stab 4.2 + table 2^31 x 4 = 8.6 = 46.4 GB
  EmitCtx per thread (rcGen/rcIdx/rcKeys/rcVals): ~2-3 MB x threads        = ~0.05 GB
  checkpoint buffering: ZERO extra RAM — fwrite streams directly from the quiesced keys/T/stab vectors (8 MB stream buffer). No 34 GB staging copy exists by design.
  Peak ~47.1 GB (+ OS page cache during dumps).
During 4->5 (true peak): layer4 46.4 + layer5 (cap 1.2e8: 4.3 GB arrays + 2^28 x 4 = 1.1 GB table) 5.4 + layer3 freed (or 0.6 if kept) => ~52 GB. Verdict: fits a 64 GB box with ~12 GB headroom for OS + page cache; 128 GB is comfortable. Mitigate page-cache blowup during 34 GB dumps with per-slice flush + posix_fadvise(DONTNEED) / FILE_FLAG_WRITE_THROUGH-ish periodic flush. Disk: rolling 2 x 34 GB trans4 generations + snapshots ~0.5 GB => ~69 GB peak during late 3->4, ~72 GB briefly at 4->5 start; provision >= 150 GB free NVMe. Note: init_fixed's assign() zero-fills ~38 GB single-threaded (~10-20 s, and usefully commits pages up front so OOM surfaces on day 0, not day 3).

================ ORDERED PATCH PLAN + EFFORT ================
P1 (3 h) Extract run_transition_ckpt from main; superstep loop + schedule(dynamic,1) inner region + atomic progress counters; carry emissions totals across supersteps. Gate: C=4 and C=5 full runs reproduce N bit-identically with --ckpt-every 0 (driver restructure is a no-op).
P2 (5 h) xxh64 + CkptHeader + ckpt_write/ckpt_load_latest/ckpt_load_snapshot with tmp+fsync+rename+prune, sliced I/O, Windows/POSIX fsync-rename variants. Gate: C=5 dump/load round trip, byte-compare arrays, downstream transition reproduces N.
P3 (2 h) layer_rebuild_table_mt + layer_keys_hash. Gate: rebuilt table gives identical find() results vs serial grow()-style rebuild on C=5 layers; timing sanity on synthetic 1e8 entries.
P4 (3 h) Resume wiring: CLI flags, snapshot chain load, parent-hash verification, cursor skip, finalize-as-last-checkpoint + generation pruning + freeing layers <= L-1 post-finalize.
P5 (2 h) Telemetry monitor thread + counters + RSS + log line + capacity/rate alerts.
P6 (1 h) Preflight: compute RAM/disk footprint from --caps, check free space/RAM, refuse to start (or warn) if insufficient; refuse --rank with --ckpt-dir.
P7 (3 h) Kill-test harness: C=5 run with --ckpt-chunk 2000 --ckpt-every 1 under a loop that SIGKILLs at random offsets (including mid-dump) and resumes, 30+ iterations; assert final N(5) = 1903816047972624930994913280000 and --ref triple match every time; one soak comparing canonically-sorted (key,T,stab) sets between a clean run and a killed-resumed run.
TOTAL ~19 h (range 16-24 h).

================ (5) RISK LIST ================
1. Capacity abort (find_or_add_mt aborts at cap, line 465): if distinct children exceed the 1.05e9 estimate, the run dies — but now loses <= 1 period; telemetry ALERT at 90% gives ~hours of warning to restart with a larger cap FROM THE LAST CHECKPOINT (cap/tableLog2 are load-time parameters; loading into a bigger cap is supported by construction).
2. u64 overflow of T (parentT*K accumulation): silent wraparound would corrupt N despite exact checkpointing; the final T%m divisibility gate (line 1080) catches many but not all corruptions. Mitigation: one-off magnitude audit of max layer-4/5 T, plus an #ifdef CKPT_OVERFLOW_CHECK build using __builtin_mul_overflow/__builtin_add_overflow on a sampled subset.
3. Parent-order nondeterminism: resuming against a REBUILT (not loaded) parent layer silently mis-maps chunks. Mitigated structurally by parentKeysHash verification (hard fail).
4. Torn/corrupt checkpoint (crash mid-dump, power loss): tmp+fsync+atomic-rename+dir-fsync+keep-2+payloadHash; worst case falls back one generation (loses 2 periods).
5. Barrier-tail imbalance at quiesce: heavy-tailed fan-out (fan-sample shows large max/median) can idle threads before each barrier; schedule(dynamic,1) bounds it to one parent; monitor logs superstep tail time so period/chunk can be retuned.
6. Page-cache pressure on a 64 GB box during 34 GB dumps can evict hot table pages and cause a post-dump throughput dip: per-slice flush/fadvise; or accept ~1 min of re-warm per checkpoint.
7. Host interruptions over 7 days (Windows Update auto-restart, sleep, OOM-killer siblings): disable auto-restart/sleep, run detached with the log as the only console; checkpointing converts these from fatal to <= 1-period losses.
8. Non-ECC RAM over ~5e17 DRAM-seconds: checksums protect disk images only, not in-memory state; prefer an ECC machine; the end-of-run gates (T%m, layer-2 mass, C=5 pipeline rehearsal) are the backstop.
9. NUMA first-touch: init_fixed zero-fills 38 GB on one node; on 2-socket hosts, parallel first-touch initialization is a follow-up optimization (perf risk only, not correctness).
10. Operator error: resuming with different C/--caps/chunkParents than the checkpoint — all are in the header and hard-checked on load; --rank + production is refused in preflight (P6).
11. u32 index headroom: claimed 9.4e8 and cap 1.05e9 fit u32 and the table's idx+1 encoding (< 2^32-1); header stores u64 and load cast-checks, so a future cap bump past 4.29e9 fails loudly, not silently.

## Verdict

FEASIBLE-AS-SPECIFIED — barrier-quiesce checkpointing is exact with no count-and-subtract machinery; dump cost is 17-34 s at full 34 GB size (<1.5% overhead at a ~40 min period); peak RAM ~52 GB fits a 64 GB machine with modest headroom; estimated 19 hours (16-24 h) implementation including kill-loop validation at C=5.

