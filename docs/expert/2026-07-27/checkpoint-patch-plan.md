# Checkpoint/restart patch plan for layer_dp_gate.cpp (apply-ready)

Target file: `E:/Code/sudoku_FJ/experiments/proto/layer_dp_gate.cpp`
(state as of 2026-07-27, i.e. AFTER the wide-u128 / probe / fan / m4probe / --wlseed drift).
Design source: `E:/Code/sudoku_FJ/docs/expert/2026-07-27/checkpoint-restart-spec.md`
(barrier-quiesce supersteps, verbatim child image, keep-2 versioned files, hard parent-identity check).

Build command unchanged:
`g++ -O2 -march=native -std=c++17 -fopenmp -o layer_dp_gate layer_dp_gate.cpp`

## Concurrent-edit guardrails (another engineer owns Canonizer + EmitCtx::leaf)

- Every anchor below lives in the include block, the utilities/main boundary, `main()`
  (arg parse, preflight, transition loop, final-layer sum), or is free-standing new code.
- NO anchor touches `struct Canonizer`, `canonize()`, `EmitCtx::leaf()`, `EmitCtx::rec()`,
  `EmitCtx::prepare()`, or the WL-seed block. E7 replaces the transition DRIVER only; the
  loop body statements that call into EmitCtx are copied verbatim and unchanged.
- Before applying, run the anchor-verification greps in the Appendix. Every count must be
  exactly 1. If any anchor count is 0 or >1 the other engineer has drifted the file again:
  STOP, re-read the file, re-derive only the affected anchor (the new-code blocks stay valid).
- Apply edits in the order E1..E8 (E2 defines globals/functions that E4-E8 reference);
  compile once at the end.

New CLI surface implemented:

```
--save-layer L file          write layer L verbatim after it is built (repeatable, L >= 2)
--load-layer L file          load layer L verbatim at startup; transitions start at L
--checkpoint base period-min chunked checkpointing of every transition into base.a/base.b
                             (period-min <= 0 => dump at every chunk barrier); also writes
                             base.L<K>.snap when each layer K finalizes
--ckpt-chunk P               parents per quiesce chunk (default 100000; test knob)
--resume base                resume the interrupted transition from base.{a,b} + base.L<P>.snap
--force-wide                 test-only: route the final transition through the u128 wide
                             path even for C < 6 (exercises wide checkpoint sections at C=5)
```

---

## (1) Ordered edit list

### E1. Platform fsync includes

Anchor (unique, replace whole 3-line block):

```cpp
#include <windows.h>
#include <psapi.h>
#endif
```

Replace with:

```cpp
#include <windows.h>
#include <psapi.h>
#include <io.h>
#else
#include <unistd.h>
#endif
```

### E2. Checkpoint/restart helper block (all new code, one insertion)

Anchor (unique section divider; insert the entire block BELOW, immediately BEFORE this line):

```cpp
// ------------------------------------------------------------------ main ---
```

Insert (new code, ends with a blank line before the anchor line):

```cpp
// ----------------------------------------------------- checkpoint/restart ---
// Verbatim Layer serialize/deserialize + chunked transition checkpoints
// (plan of 2026-07-27; design: docs/expert/2026-07-27/checkpoint-restart-spec.md).
// Images are VERBATIM: keys/T/stab[0..n) including holes, so entry ORDER --
// and therefore parent chunk indices -- survives a round trip exactly.  The
// hash table is never dumped; it is rebuilt on load (holes skipped).  Files
// are written tmp -> fflush -> fsync -> atomic-rename-replace, guarded by
// magic + version + streamed 64-bit checksums (header and payload).

static const u64 CK_MAGIC = 0x314B434C4A464453ULL;  // "SDFJLCK1"
static const u32 CK_VERSION = 1;
static const u64 CK_SEED = 0x5344464A434B3031ULL;

static u64 ck_hash64(const void* p, size_t n, u64 seed) {
    const u8* b = (const u8*)p;
    u64 h = seed ^ (0x9e3779b97f4a7c15ULL + n);
    size_t i = 0;
    for (; i + 8 <= n; i += 8) {
        u64 w;
        std::memcpy(&w, b + i, 8);
        h ^= w;
        h *= 0xff51afd7ed558ccdULL;
        h ^= h >> 33;
    }
    u64 t = 0;
    for (size_t k = 0; i < n; i++, k += 8) t |= (u64)b[i] << k;
    h ^= t;
    h *= 0xc4ceb9fe1a85ec53ULL;
    h ^= h >> 33;
    return h;
}

struct CkptHeader {           // 120 bytes, naturally packed, little-endian
    u64 magic;
    u32 version, cVal;
    u32 layerIdx;             // layer stored in this file
    u32 parentLayer;          // transition parent L (0 = plain layer snapshot)
    u64 gen;                  // monotone generation (drives .a/.b alternation)
    u64 nEntries, holes;      // verbatim entry count INCLUDING holes
    u64 cursorChunk, nChunks, chunkParents;
    u64 emissionsSoFar, cacheHitsSoFar;
    u64 parentKeysHash;       // ck_hash64 over parent keys bytes (0 if none)
    u64 nWide;                // trailing u128 entries (wide final transition)
    u64 payloadHash;          // chained ck_hash64 over keys||T||stab||wide
    u64 headerHash;           // ck_hash64 over header with this field zeroed
};
static_assert(sizeof(CkptHeader) == 120, "CkptHeader must be padding-free");

struct CkptImage {
    CkptHeader h{};
    std::vector<State> keys;
    std::vector<u64> T;
    std::vector<u32> stab;
    std::vector<u128> wide;
};

// ---- checkpoint/restart run configuration (set from CLI in main) ----
static std::string g_ckptBase;          // --checkpoint base path ("" = off)
static double g_ckptPeriodMin = 0.0;    // <= 0: dump at every chunk barrier
static u64 g_ckptChunk = 100000;        // parents per chunk (quiesce grain)
static u64 g_ckptGen = 0;
static bool g_forceWide = false;        // --force-wide (wide-path test at C<6)
static int g_loadLayerIdx = 0;
static std::string g_loadLayerPath;
static std::string g_resumeBase;
static std::vector<std::pair<int, std::string>> g_saveLayers;
static bool g_resumePending = false;    // a --resume image awaits its transition
static CkptHeader g_resumeHdr{};
static CkptImage g_resumeCk;

static bool ck_write_file(const std::string& path, CkptHeader h,
                          const State* keys, const u64* T, const u32* stab,
                          const u128* wide) {
    h.magic = CK_MAGIC;
    h.version = CK_VERSION;
    h.cVal = (u32)C;
    u64 ph = CK_SEED;
    ph = ck_hash64(keys, (size_t)h.nEntries * sizeof(State), ph);
    ph = ck_hash64(T, (size_t)h.nEntries * sizeof(u64), ph);
    ph = ck_hash64(stab, (size_t)h.nEntries * sizeof(u32), ph);
    if (h.nWide) ph = ck_hash64(wide, (size_t)h.nWide * sizeof(u128), ph);
    h.payloadHash = ph;
    h.headerHash = 0;
    h.headerHash = ck_hash64(&h, sizeof(h), CK_SEED);
    std::string tmp = path + ".tmp";
    FILE* f = std::fopen(tmp.c_str(), "wb");
    if (!f) { std::perror(tmp.c_str()); return false; }
    bool ok = std::fwrite(&h, sizeof(h), 1, f) == 1;
    if (ok && h.nEntries) {
        ok = ok && std::fwrite(keys, sizeof(State), (size_t)h.nEntries, f) ==
                       (size_t)h.nEntries;
        ok = ok && std::fwrite(T, sizeof(u64), (size_t)h.nEntries, f) ==
                       (size_t)h.nEntries;
        ok = ok && std::fwrite(stab, sizeof(u32), (size_t)h.nEntries, f) ==
                       (size_t)h.nEntries;
    }
    if (ok && h.nWide)
        ok = ok && std::fwrite(wide, sizeof(u128), (size_t)h.nWide, f) ==
                       (size_t)h.nWide;
    ok = ok && std::fflush(f) == 0;
#ifdef _WIN32
    ok = ok && _commit(_fileno(f)) == 0;
#else
    ok = ok && fsync(fileno(f)) == 0;
#endif
    std::fclose(f);
    if (!ok) { std::remove(tmp.c_str()); return false; }
#ifdef _WIN32
    if (!MoveFileExA(tmp.c_str(), path.c_str(),
                     MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
        std::fprintf(stderr, "ckpt rename failed: %s\n", path.c_str());
        return false;
    }
#else
    if (std::rename(tmp.c_str(), path.c_str()) != 0) {
        std::perror(path.c_str());
        return false;
    }
#endif
    return true;
}

static bool ck_read_header(const std::string& path, CkptHeader& h) {
    FILE* f = std::fopen(path.c_str(), "rb");
    if (!f) return false;
    bool ok = std::fread(&h, sizeof(h), 1, f) == 1;
    std::fclose(f);
    if (!ok) return false;
    u64 want = h.headerHash;
    h.headerHash = 0;
    if (h.magic != CK_MAGIC || h.version != CK_VERSION ||
        ck_hash64(&h, sizeof(h), CK_SEED) != want)
        return false;
    h.headerHash = want;
    return true;
}

static bool ck_read_file(const std::string& path, CkptImage& im) {
    CkptHeader h;
    if (!ck_read_header(path, h)) return false;
    FILE* f = std::fopen(path.c_str(), "rb");
    if (!f) return false;
    bool ok = std::fseek(f, (long)sizeof(CkptHeader), SEEK_SET) == 0;
    im.keys.resize((size_t)h.nEntries);
    im.T.resize((size_t)h.nEntries);
    im.stab.resize((size_t)h.nEntries);
    im.wide.resize((size_t)h.nWide);
    if (ok && h.nEntries) {
        ok = ok && std::fread(im.keys.data(), sizeof(State),
                              (size_t)h.nEntries, f) == (size_t)h.nEntries;
        ok = ok && std::fread(im.T.data(), sizeof(u64), (size_t)h.nEntries,
                              f) == (size_t)h.nEntries;
        ok = ok && std::fread(im.stab.data(), sizeof(u32), (size_t)h.nEntries,
                              f) == (size_t)h.nEntries;
    }
    if (ok && h.nWide)
        ok = ok && std::fread(im.wide.data(), sizeof(u128), (size_t)h.nWide,
                              f) == (size_t)h.nWide;
    std::fclose(f);
    if (!ok) return false;
    u64 ph = CK_SEED;
    ph = ck_hash64(im.keys.data(), (size_t)h.nEntries * sizeof(State), ph);
    ph = ck_hash64(im.T.data(), (size_t)h.nEntries * sizeof(u64), ph);
    ph = ck_hash64(im.stab.data(), (size_t)h.nEntries * sizeof(u32), ph);
    if (h.nWide)
        ph = ck_hash64(im.wide.data(), (size_t)h.nWide * sizeof(u128), ph);
    if (ph != h.payloadHash) return false;
    if (h.cVal != (u32)C) return false;
    im.h = h;
    return true;
}

// install a verbatim image into a Layer, preserving entry order exactly.
// fixedCap != 0: fixed mode with that capacity (all loaded PARENTS use
// fixedCap == nEntries; a resumed threaded CHILD uses caps[L-1]).
// fixedCap == 0: dynamic single-thread child (requires holes == 0, since
// is_hole() only recognizes holes in fixed mode).
static bool layer_install(Layer& lay, CkptImage& im, size_t fixedCap) {
    size_t n = im.keys.size();
    if (fixedCap) {
        if (n > fixedCap) {
            std::fprintf(stderr, "FATAL: layer image (%zu entries) exceeds "
                                 "cap %zu\n", n, fixedCap);
            return false;
        }
        lay.init_fixed(fixedCap);
        std::copy(im.keys.begin(), im.keys.end(), lay.keys.begin());
        std::copy(im.T.begin(), im.T.end(), lay.T.begin());
        std::copy(im.stab.begin(), im.stab.end(), lay.stab.begin());
        lay.claimed = (u32)n;
        lay.holes = (u32)im.h.holes;
    } else {
        if (im.h.holes != 0) {
            std::fprintf(stderr, "FATAL: image has holes; loading it needs "
                                 "--threads > 1 with --caps (fixed mode)\n");
            return false;
        }
        lay.keys = std::move(im.keys);
        lay.T = std::move(im.T);
        lay.stab = std::move(im.stab);
        size_t sz = 64;
        while (sz < n * 2 + 16) sz <<= 1;
        lay.table.assign(sz, 0);
        lay.mask = sz - 1;
        lay.fixedCap = false;
        lay.claimed = 0;
        lay.holes = 0;
    }
    for (u32 i = 0; i < (u32)n; i++) {   // rebuild table; skip holes (stab==0)
        if (lay.stab[i] == 0) continue;
        u64 h = state_hash(lay.keys[i]) & lay.mask;
        while (lay.table[h]) h = (h + 1) & lay.mask;
        lay.table[h] = i + 1;
    }
    return true;
}

static bool layer_save_file(const std::string& path, const Layer& lay,
                            int layerIdx, const std::vector<u128>* wide) {
    CkptHeader h{};
    h.layerIdx = (u32)layerIdx;
    h.parentLayer = 0;
    h.nEntries = lay.size();
    h.holes = lay.holes;
    size_t nw = wide ? std::min(wide->size(), (size_t)h.nEntries) : 0;
    h.nWide = (u64)nw;
    bool ok = ck_write_file(path, h, lay.keys.data(), lay.T.data(),
                            lay.stab.data(), nw ? wide->data() : nullptr);
    if (ok)
        std::printf("saved layer %d (%llu entries, %u holes%s) -> %s\n",
                    layerIdx, (unsigned long long)h.nEntries, lay.holes,
                    nw ? ", wide" : "", path.c_str());
    else
        std::fprintf(stderr, "WARNING: failed to save layer %d to %s\n",
                     layerIdx, path.c_str());
    return ok;
}

static void ckpt_write_pair(int L, const Layer& parent, const Layer& child,
                            const std::vector<u128>* wide, u64 cursorChunk,
                            u64 nChunks, u64 chunkParents, u64 em, u64 hits) {
    double td0 = now_s();
    CkptHeader h{};
    h.layerIdx = (u32)(L + 1);
    h.parentLayer = (u32)L;
    h.gen = ++g_ckptGen;
    h.nEntries = child.size();
    h.holes = child.holes;
    h.cursorChunk = cursorChunk;
    h.nChunks = nChunks;
    h.chunkParents = chunkParents;
    h.emissionsSoFar = em;
    h.cacheHitsSoFar = hits;
    h.parentKeysHash = ck_hash64(parent.keys.data(),
                                 parent.size() * sizeof(State), CK_SEED);
    size_t nw = wide ? std::min(wide->size(), (size_t)h.nEntries) : 0;
    h.nWide = (u64)nw;
    std::string path = g_ckptBase + ((h.gen & 1) ? ".a" : ".b");
    if (!ck_write_file(path, h, child.keys.data(), child.T.data(),
                       child.stab.data(), nw ? wide->data() : nullptr))
        std::fprintf(stderr, "WARNING: checkpoint write failed (%s); "
                             "continuing without it\n", path.c_str());
    else
        std::printf("checkpoint gen=%llu trans=%d->%d chunk=%llu/%llu "
                    "entries=%llu -> %s (%.2fs)\n", (unsigned long long)h.gen,
                    L, L + 1, (unsigned long long)cursorChunk,
                    (unsigned long long)nChunks,
                    (unsigned long long)h.nEntries, path.c_str(),
                    now_s() - td0);
    std::fflush(stdout);
}

static bool ckpt_load_newest(const std::string& base, CkptImage& im) {
    std::string pa = base + ".a", pb = base + ".b";
    CkptHeader ha, hb;
    bool va = ck_read_header(pa, ha);
    bool vb = ck_read_header(pb, hb);
    const std::string* first = nullptr;
    const std::string* second = nullptr;
    if (va && vb) {
        first = (ha.gen >= hb.gen) ? &pa : &pb;
        second = (ha.gen >= hb.gen) ? &pb : &pa;
    } else if (va) {
        first = &pa;
    } else if (vb) {
        first = &pb;
    }
    if (first && ck_read_file(*first, im)) return true;   // newest valid gen
    if (second && ck_read_file(*second, im)) return true; // torn-dump fallback
    return false;
}

static bool ckpt_install_child(Layer& child, std::vector<u128>& wideOut,
                               CkptImage& im, size_t fixedCap) {
    // NOTE: order matters -- read im.wide before layer_install may move from
    // the other members (it never touches im.wide).
    if (!layer_install(child, im, fixedCap)) return false;
    wideOut.assign(im.wide.begin(), im.wide.end());
    return true;
}
```

### E3. Usage string

Anchor (unique, replace):

```cpp
        std::fprintf(stderr, "usage: %s C [--ref f] [--rank] [--invariance N] "
                             "[--scan-check N] [--layer-mass L] [--dump f]\n",
                     argv[0]);
```

Replace with:

```cpp
        std::fprintf(stderr, "usage: %s C [--ref f] [--rank] [--invariance N] "
                             "[--scan-check N] [--layer-mass L] [--dump f]\n"
                             "  ckpt: [--save-layer L file] [--load-layer L "
                             "file] [--checkpoint base period-min]\n"
                             "        [--ckpt-chunk P] [--resume base] "
                             "[--force-wide]\n",
                     argv[0]);
```

### E4. CLI flag parsing

Anchor (unique; INSERT the new code immediately BEFORE this line, at the same 8-space indent):

```cpp
        else if (s == "--caps" && a + 1 < argc) {
```

Insert:

```cpp
        else if (s == "--save-layer" && a + 2 < argc) {
            int sl = std::atoi(argv[++a]);
            g_saveLayers.push_back({sl, std::string(argv[++a])});
        }
        else if (s == "--load-layer" && a + 2 < argc) {
            g_loadLayerIdx = std::atoi(argv[++a]);
            g_loadLayerPath = argv[++a];
        }
        else if (s == "--checkpoint" && a + 2 < argc) {
            g_ckptBase = argv[++a];
            g_ckptPeriodMin = std::atof(argv[++a]);
        }
        else if (s == "--ckpt-chunk" && a + 1 < argc)
            g_ckptChunk = std::stoull(argv[++a]);
        else if (s == "--resume" && a + 1 < argc) g_resumeBase = argv[++a];
        else if (s == "--force-wide") g_forceWide = true;
```

### E5. Preflight refusals

Anchor (unique, replace the whole block -- it is the caps sanity check; append the new
preflight AFTER it inside the replacement):

```cpp
    if (nthreads > 1 && caps.size() < (size_t)(C - 1)) {
        std::fprintf(stderr, "--threads needs --caps c2,...,c%d (child layer "
                             "capacities)\n", C);
        return 2;
    }
```

Replace with:

```cpp
    if (nthreads > 1 && caps.size() < (size_t)(C - 1)) {
        std::fprintf(stderr, "--threads needs --caps c2,...,c%d (child layer "
                             "capacities)\n", C);
        return 2;
    }
    // ---- checkpoint/restart preflight (checkpoint-restart-spec.md) ----
    if (doRank && (!g_ckptBase.empty() || !g_resumeBase.empty() ||
                   g_loadLayerIdx > 0)) {
        std::fprintf(stderr, "--rank records full transition rows in memory; "
                             "it cannot survive checkpoint/restart or a "
                             "partial layer chain (drop --rank)\n");
        return 2;
    }
    if ((!g_ckptBase.empty() || !g_resumeBase.empty()) &&
        (probeLayer > 0 || m4K > 0)) {
        std::fprintf(stderr, "--checkpoint/--resume cover full transitions "
                             "only; not compatible with --probe/--m4probe\n");
        return 2;
    }
    if ((g_loadLayerIdx > 0 || !g_resumeBase.empty()) &&
        (invarianceN > 0 || scanCheckN > 0 || layerMassL > 0)) {
        std::fprintf(stderr, "--invariance/--scan-check/--layer-mass need "
                             "every layer in memory; not compatible with "
                             "--load-layer/--resume\n");
        return 2;
    }
    if (g_ckptChunk == 0) {
        std::fprintf(stderr, "--ckpt-chunk must be > 0\n");
        return 2;
    }
    if (g_loadLayerIdx >= C && C >= 6) {
        std::fprintf(stderr, "--load-layer %d at C>=6 cannot restore the "
                             "final wide sums standalone; load layer %d and "
                             "re-run the last transition\n", C, C - 1);
        return 2;
    }
    if (!g_resumeBase.empty() && g_ckptBase.empty())
        g_ckptBase = g_resumeBase;   // resumed runs keep checkpointing on
```

### E6. Load-layer / resume startup + loop start

Anchor (unique 2-line block, replace):

```cpp
    double t_all0 = now_s();
    for (int L = 1; L < C; L++) {
```

Replace with:

```cpp
    int startL = 1;
    if (!g_resumeBase.empty()) {
        // ---- resume: newest valid checkpoint of the interrupted transition
        if (!ckpt_load_newest(g_resumeBase, g_resumeCk)) {
            std::fprintf(stderr, "FATAL: no valid checkpoint pair at "
                                 "%s.{a,b}\n", g_resumeBase.c_str());
            return 12;
        }
        g_resumeHdr = g_resumeCk.h;
        g_ckptGen = g_resumeHdr.gen;
        int P = (int)g_resumeHdr.parentLayer;
        if (P < 1 || P >= C) {
            std::fprintf(stderr, "FATAL: checkpoint parent layer %d out of "
                                 "range for C=%d\n", P, C);
            return 12;
        }
        if (P > 1) {   // layer 1 is rebuilt deterministically above
            std::string psnap =
                g_resumeBase + ".L" + std::to_string(P) + ".snap";
            CkptImage pim;
            if (!ck_read_file(psnap, pim) || (int)pim.h.layerIdx != P) {
                std::fprintf(stderr, "FATAL: cannot load parent snapshot %s "
                                     "(resume must NEVER rebuild parents)\n",
                             psnap.c_str());
                return 12;
            }
            size_t pn = pim.keys.size();
            if (!layer_install(layers[P], pim, pn ? pn : 1)) return 12;
        }
        startL = P;
        g_resumePending = true;
        std::printf("resuming transition %d->%d from %s (gen %llu, cursor "
                    "%llu/%llu)\n", P, P + 1, g_resumeBase.c_str(),
                    (unsigned long long)g_resumeHdr.gen,
                    (unsigned long long)g_resumeHdr.cursorChunk,
                    (unsigned long long)g_resumeHdr.nChunks);
    } else if (g_loadLayerIdx > 0) {
        if (g_loadLayerIdx > C) {
            std::fprintf(stderr, "FATAL: --load-layer index out of range\n");
            return 12;
        }
        CkptImage im;
        if (!ck_read_file(g_loadLayerPath, im) ||
            (int)im.h.layerIdx != g_loadLayerIdx) {
            std::fprintf(stderr, "FATAL: cannot load layer %d from %s\n",
                         g_loadLayerIdx, g_loadLayerPath.c_str());
            return 12;
        }
        size_t ln = im.keys.size();
        if (!im.wide.empty())
            gWide.assign(im.wide.begin(), im.wide.end());
        if (!layer_install(layers[g_loadLayerIdx], im, ln ? ln : 1))
            return 12;
        startL = g_loadLayerIdx;
        std::printf("loaded layer %d from %s (%zu entries, %u holes)\n",
                    g_loadLayerIdx, g_loadLayerPath.c_str(),
                    layers[g_loadLayerIdx].size(),
                    layers[g_loadLayerIdx].holes);
    }
    double t_all0 = now_s();
    for (int L = startL; L < C; L++) {
```

### E7. Superstep transition driver (the core edit)

Anchor: replace this ENTIRE block exactly as it appears today (lines ~1064-1135; unique as
a whole -- it is the only `reduction(+ : totEmissions, totHits)` region in the file):

```cpp
        double t0 = now_s();
        u64 canon0 = g_canonize_calls;
        size_t nParents = layers[L].size();
        if (nthreads > 1) layers[L + 1].init_fixed(caps[L - 1]);
        else layers[L + 1].init(nParents * 4 + 64);
        bool record = doRank;
        if (record) Rrows[L].resize(nParents);
        u64 totEmissions = 0, totHits = 0;
#ifdef _OPENMP
#pragma omp parallel num_threads(nthreads) if (nthreads > 1) \
    reduction(+ : totEmissions, totHits)
#endif
        {
            EmitCtx ctx;
            ctx.next = &layers[L + 1];
            ctx.emissions = 0;
            ctx.cacheHits = 0;
            ctx.row = nullptr;
            ctx.wideT = (L + 1 == C && C >= 6);
            if (ctx.wideT && layers[L + 1].fixedCap)
                ctx.wide.assign(layers[L + 1].keys.size(), 0);
#ifdef _OPENMP
#pragma omp for schedule(dynamic, 8)
#endif
            for (long long i = 0; i < (long long)nParents; i++) {
                if (layers[L].is_hole((u32)i)) continue;
                ctx.row = record ? &Rrows[L][i] : nullptr;
                ctx.prepare(layers[L].keys[i], layers[L].T[i]);
                ctx.rec(0, (1u << N2C) - 1);
                if (record) {  // merge duplicate children within the row
                    auto& r = Rrows[L][i];
                    std::sort(r.begin(), r.end());
                    size_t w = 0;
                    for (size_t j = 0; j < r.size();) {
                        size_t k = j;
                        u64 sum = 0;
                        while (k < r.size() && r[k].first == r[j].first)
                            sum += r[k++].second;
                        r[w++] = {r[j].first, sum};
                        j = k;
                    }
                    r.resize(w);
                }
            }
            totEmissions += ctx.emissions;
            totHits += ctx.cacheHits;
            if (ctx.wideT) {
#ifdef _OPENMP
#pragma omp critical(wide_merge)
#endif
                {
                    if (gWide.size() < ctx.wide.size())
                        gWide.resize(ctx.wide.size(), 0);
                    for (size_t i = 0; i < ctx.wide.size(); i++)
                        gWide[i] += ctx.wide[i];
                }
            }
            flush_canon_counters();
        }
        flush_canon_counters();
        emissionsPerT.push_back(totEmissions);
        canonizePerT.push_back(g_canonize_calls - canon0);
        timePerT.push_back(now_s() - t0);
        std::printf("transition %d->%d: emissions=%llu children=%zu "
                    "(holes=%u) time=%.3fs  (%.1f ns/emission, %.1f%% cache "
                    "hits)\n",
                    L, L + 1, (unsigned long long)totEmissions,
                    layers[L + 1].real_size(), layers[L + 1].holes,
                    timePerT.back(),
                    totEmissions ? timePerT.back() * 1e9 / totEmissions : 0.0,
                    totEmissions ? 100.0 * totHits / totEmissions : 0.0);
        std::fflush(stdout);
```

Replace with (the parent-loop BODY -- prepare/rec/record merge -- and the wide-merge
critical are copied verbatim; only the driver around them changes):

```cpp
        double t0 = now_s();
        u64 canon0 = g_canonize_calls;
        size_t nParents = layers[L].size();
        bool record = doRank;
        if (record) Rrows[L].resize(nParents);
        u64 totEmissions = 0, totHits = 0;
        const bool wideHere = (L + 1 == C && (C >= 6 || g_forceWide));
        const bool ckptHere = !g_ckptBase.empty();
        const u64 ckChunk = g_ckptChunk;
        const u64 nChunks = (u64)(nParents + ckChunk - 1) / ckChunk;
        u64 ckCursor = 0;
        if (g_resumePending && g_resumeHdr.parentLayer == (u32)L) {
            if (g_resumeHdr.chunkParents != ckChunk ||
                g_resumeHdr.nChunks != nChunks) {
                std::fprintf(stderr, "FATAL: resume chunking mismatch "
                                     "(ckpt %llu parents/chunk, %llu chunks; "
                                     "run %llu, %llu)\n",
                             (unsigned long long)g_resumeHdr.chunkParents,
                             (unsigned long long)g_resumeHdr.nChunks,
                             (unsigned long long)ckChunk,
                             (unsigned long long)nChunks);
                return 12;
            }
            u64 ph = ck_hash64(layers[L].keys.data(),
                               layers[L].size() * sizeof(State), CK_SEED);
            if (ph != g_resumeHdr.parentKeysHash) {
                std::fprintf(stderr, "FATAL: parent keys hash mismatch on "
                                     "resume of transition %d->%d (chunk "
                                     "indices would mis-map)\n", L, L + 1);
                return 12;
            }
            size_t childCap = (nthreads > 1) ? caps[L - 1] : 0;
            if (!ckpt_install_child(layers[L + 1], gWide, g_resumeCk,
                                    childCap)) {
                std::fprintf(stderr, "FATAL: cannot install resumed child\n");
                return 12;
            }
            g_resumeCk = CkptImage{};   // release the image copy
            ckCursor = g_resumeHdr.cursorChunk;
            totEmissions = g_resumeHdr.emissionsSoFar;
            totHits = g_resumeHdr.cacheHitsSoFar;
            g_resumePending = false;
            std::printf("resume: transition %d->%d at chunk %llu/%llu "
                        "(child entries=%zu holes=%u)\n", L, L + 1,
                        (unsigned long long)ckCursor,
                        (unsigned long long)nChunks, layers[L + 1].size(),
                        layers[L + 1].holes);
            std::fflush(stdout);
        } else {
            if (nthreads > 1) layers[L + 1].init_fixed(caps[L - 1]);
            else layers[L + 1].init(nParents * 4 + 64);
        }
        double lastCkT = now_s();
        for (u64 ckStep = ckCursor; ckStep < nChunks; ckStep++) {
            const long long lo = (long long)(ckStep * ckChunk);
            const long long hi = (long long)std::min<u64>(
                (u64)nParents, (ckStep + 1) * ckChunk);
#ifdef _OPENMP
#pragma omp parallel num_threads(nthreads) if (nthreads > 1) \
    reduction(+ : totEmissions, totHits)
#endif
            {
                EmitCtx ctx;
                ctx.next = &layers[L + 1];
                ctx.emissions = 0;
                ctx.cacheHits = 0;
                ctx.row = nullptr;
                ctx.wideT = wideHere;
                if (ctx.wideT && layers[L + 1].fixedCap)
                    ctx.wide.assign(layers[L + 1].keys.size(), 0);
#ifdef _OPENMP
#pragma omp for schedule(dynamic, 8)
#endif
                for (long long i = lo; i < hi; i++) {
                    if (layers[L].is_hole((u32)i)) continue;
                    ctx.row = record ? &Rrows[L][i] : nullptr;
                    ctx.prepare(layers[L].keys[i], layers[L].T[i]);
                    ctx.rec(0, (1u << N2C) - 1);
                    if (record) {  // merge duplicate children within the row
                        auto& r = Rrows[L][i];
                        std::sort(r.begin(), r.end());
                        size_t w = 0;
                        for (size_t j = 0; j < r.size();) {
                            size_t k = j;
                            u64 sum = 0;
                            while (k < r.size() && r[k].first == r[j].first)
                                sum += r[k++].second;
                            r[w++] = {r[j].first, sum};
                            j = k;
                        }
                        r.resize(w);
                    }
                }
                totEmissions += ctx.emissions;
                totHits += ctx.cacheHits;
                if (ctx.wideT) {
#ifdef _OPENMP
#pragma omp critical(wide_merge)
#endif
                    {
                        if (gWide.size() < ctx.wide.size())
                            gWide.resize(ctx.wide.size(), 0);
                        for (size_t i = 0; i < ctx.wide.size(); i++)
                            gWide[i] += ctx.wide[i];
                    }
                }
                flush_canon_counters();
            }
            // QUIESCE: the parallel region above has closed (implicit
            // barrier + flush).  Every parent in chunks [0, ckStep] is fully
            // applied to layers[L+1] -- and, in wide mode, merged into gWide
            // -- and no parent from a later chunk has started.  Safe dump.
            if (ckptHere &&
                (ckStep + 1 == nChunks ||
                 now_s() - lastCkT >= g_ckptPeriodMin * 60.0)) {
                ckpt_write_pair(L, layers[L], layers[L + 1],
                                wideHere ? &gWide : nullptr, ckStep + 1,
                                nChunks, ckChunk, totEmissions, totHits);
                lastCkT = now_s();
            }
        }
        flush_canon_counters();
        emissionsPerT.push_back(totEmissions);
        canonizePerT.push_back(g_canonize_calls - canon0);
        timePerT.push_back(now_s() - t0);
        std::printf("transition %d->%d: emissions=%llu children=%zu "
                    "(holes=%u) time=%.3fs  (%.1f ns/emission, %.1f%% cache "
                    "hits)\n",
                    L, L + 1, (unsigned long long)totEmissions,
                    layers[L + 1].real_size(), layers[L + 1].holes,
                    timePerT.back(),
                    totEmissions ? timePerT.back() * 1e9 / totEmissions : 0.0,
                    totEmissions ? 100.0 * totHits / totEmissions : 0.0);
        std::fflush(stdout);
        if (ckptHere)   // finalized child = next transition's parent snapshot
            layer_save_file(g_ckptBase + ".L" + std::to_string(L + 1) +
                                ".snap",
                            layers[L + 1], L + 1,
                            wideHere ? &gWide : nullptr);
        for (size_t si = 0; si < g_saveLayers.size(); si++)
            if (g_saveLayers[si].first == L + 1)
                layer_save_file(g_saveLayers[si].second, layers[L + 1], L + 1,
                                wideHere ? &gWide : nullptr);
```

### E8. Wide-path final sum under --force-wide

Anchor (unique, replace):

```cpp
        u128 Tw = (C >= 6) ? (i < gWide.size() ? gWide[i] : (u128)0)
                           : (u128)fin.T[i];
```

Replace with:

```cpp
        u128 Tw = (C >= 6 || g_forceWide)
                      ? (i < gWide.size() ? gWide[i] : (u128)0)
                      : (u128)fin.T[i];
```

---

## (2) Resume-correctness argument (restated for the CURRENT code)

The invariant: a checkpoint image on disk contains the contributions of EXACTLY the
parents in chunks `[0, cursorChunk)`, each applied exactly once, and nothing else.
Resume re-runs chunks `>= cursorChunk` against that image. Point by point against the
code as it stands today:

1. **Barrier-only dumps.** `ckpt_write_pair` is called by the (serial) master thread
   only after the per-chunk `#pragma omp parallel` region has CLOSED. A region close is
   a full barrier plus an implied flush, so all worker writes into `layers[L+1]` --
   key claims via `find_or_add_mt`'s CAS + release-store of `stab`, and relaxed
   `__atomic_add_fetch` on `T` -- are complete and visible before the dump reads them.
   The outer chunk loop is serial, so no parent from a later chunk has begun. A parent
   is never split across chunks, so every parent is all-or-nothing in the image. The
   in-memory partial state of a killed chunk was never externalized; re-running that
   chunk after resume adds each of its parents' contributions exactly once (u64 addition
   is commutative/associative even mod 2^64, so ordering and interleaving are irrelevant).

2. **Holes.** Fixed-mode insertion races abandon claims as holes (`stab == 0`, T stays 0,
   key bytes present). The image is VERBATIM over `[0, claimed)` including holes, so
   entry order -- the only thing chunk indices and `gWide` indices depend on -- survives
   a round trip bit-exactly. On load the table is rebuilt skipping `stab == 0`, exactly
   like live consumers (`is_hole`) skip them; the logical multiset {key -> (T, stab)} is
   unchanged. Post-checkpoint insertions (and post-checkpoint holes) of the crashed run
   die with it -- they are recreated on resume, possibly at different indices with
   different hole patterns, which is semantically irrelevant and costs only a little
   capacity headroom. `claimed`/`holes` are restored from the header, so `size()`/
   `real_size()` stay truthful. A holed image can only be reloaded in fixed mode
   (`layer_install` refuses dynamic install when `holes != 0`, because `is_hole()` only
   works when `fixedCap` is set).

3. **stab is pure.** `stab` is the canonizer's `minCount`, a pure function of the key.
   Keys first inserted after resume get identical stab values to what the crashed run
   would have computed. (The concurrent canonizer refactor MUST preserve this purity;
   that is already a hard invariant of the program, gated by --scan-check.)

4. **Parent identity is load-bearing.** Chunk index ranges name parents only relative to
   the exact parent array of the original build, hole positions included; parent entry
   order is race-nondeterministic across rebuilds. Therefore resume NEVER rebuilds the
   parent: it reloads `base.L<P>.snap` verbatim (written at the previous transition's
   finalize, or trivially rebuilt for P=1 which is a single deterministic state), and the
   transition hard-fails unless `ck_hash64(parent.keys[0..size()))` equals the
   `parentKeysHash` stored in the checkpoint. Chunk size and chunk count are also stored
   and hard-checked. Thread count MAY change across resume (assignment of parents to
   threads never matters, only chunk boundaries), with one restriction enforced by
   layer_install: an image with holes needs a fixed-mode (threaded) run.

5. **Fixed caps.** A resumed child is installed with `init_fixed(caps[L-1])` and
   `claimed` restored; `caps` may be RAISED on resume (cap governs only the table size
   and the abort threshold in `find_or_add_mt`, not semantics) -- this is the escape
   hatch for a capacity abort. `init_fixed`'s u32-index-space guard still applies.
   Lowering caps below `claimed` is refused by `layer_install`.

6. **Wide-u128 final transition.** In `wideT` mode (now `L+1 == C && (C >= 6 ||
   --force-wide)`) child T contributions bypass `next->T` and accumulate in per-thread
   `ctx.wide`, merged into `gWide` under `omp critical(wide_merge)` INSIDE the region,
   i.e. strictly before the region-close barrier. With the per-chunk-region driver of
   E7, each chunk's region constructs fresh zeroed `ctx.wide` vectors and merges its own
   deltas once -- so after chunk `ckStep` closes, `gWide` holds exactly the chunks
   `[0, ckStep]` contributions (u128 addition commutative/associative; no double-merge
   possible because ctx dies with its region). The checkpoint therefore appends
   `gWide[0..min(|gWide|, claimed))` as a fourth payload section (`nWide`), and resume
   restores it before re-entering the loop. `gWide` is indexed by child layer index,
   which the verbatim child image preserves, so wide sums stay aligned with keys/stab.
   Entries beyond the dumped prefix are zero by construction (fixed mode sizes ctx.wide
   to cap and [claimed, cap) is never touched; dynamic mode grows lazily) and are
   regenerated as zero on resume. The finalized `base.L<C>.snap` also carries the wide
   section, keeping the crash window between final checkpoint and snapshot safe.

7. **Rrows/--rank is DISABLED under checkpointing** (preflight hard refusal, E5).
   Two independent reasons: (a) `Rrows[L][i]` rows live only in memory and are not
   checkpointed, so parents completed before a crash would resume with empty rows --
   silently wrong ranks; (b) rows store CHILD INDICES of the crashed run's child layer;
   even if dumped, post-resume insertion order (holes, ordering) can differ, mis-mapping
   indices. Refusing `--rank` alongside `--checkpoint/--resume/--load-layer` (it also
   needs the full layer chain) is the only exact option, and matches the spec's
   production rule (2.1e12 stored pairs make --rank a non-starter at C=6 anyway).

8. **Crash-window coverage.** The last chunk of every transition forces a checkpoint with
   `cursorChunk == nChunks`, then the layer snapshot is written. Crash between those two
   writes, or anywhere before the NEXT transition's first checkpoint: resume loads the
   final checkpoint, finds the loop range empty, re-prints the transition line, rewrites
   the snapshot, and proceeds -- exactly-once is preserved because the next transition's
   partial in-memory child was never externalized. Torn dumps are covered by
   tmp+fflush+`_commit`/`fsync`+atomic-rename-replace plus the .a/.b generation pair:
   the file being replaced is never the file that was fsynced last generation, so at any
   kill instant at least one fully-valid image exists; header/payload checksums make the
   loader fall back to the older generation on a torn newest.

9. **What is deliberately NOT checkpointed** (all provably outside N): the hash table
   (rebuilt), canonize call/node counters, `timePerT` (restarts at resume), peak RSS,
   layers below the resumed parent (final gates that need them are refused under
   resume/load), and the per-parent EmitCtx rc-cache (reset by `prepare()` per parent --
   no cross-chunk state by construction). `emissionsSoFar`/`cacheHitsSoFar` are restored
   so the printed totals equal a clean run's (fan-out and per-parent cache hits are
   deterministic per parent, and each parent is counted exactly once across the
   crash/resume boundary).

## (3) Validation recipe: C=5 kill-loop

Reference truths: `N(5) = 1903816047972624930994913280000`; 355 (m, ell, F) triples in
`E:/Code/sudoku_FJ/docs/expert/2026-07-21/native_c5_response_quotient_triples.csv`;
`--dump` output is sorted by canonical key, hence byte-deterministic across any
thread/crash schedule -- a clean-run dump is a golden file.

Step 0 -- rebuild + no-op parity (restructure must be invisible without flags):

```bash
cd E:/Code/sudoku_FJ/experiments/proto
g++ -O2 -march=native -std=c++17 -fopenmp -o layer_dp_gate_ck layer_dp_gate.cpp
./layer_dp_gate_ck 4                       # expect N check OK, ALL GATES PASSED
./layer_dp_gate_ck 5 --ref ../../docs/expert/2026-07-21/native_c5_response_quotient_triples.csv \
    --dump golden_c5_dump.csv              # serial golden; note "layers (states):" line
# threaded parity: set CAPS to ~2x the printed layer sizes (layers 2..5), e.g.
# CAPS=<2*s2>,<2*s3>,<2*s4>,<2*s5>
./layer_dp_gate_ck 5 --threads 8 --caps $CAPS --ref ...csv   # same N, 355/355 OK
```

Step 1 -- serialize round trip + save/load flags:

```bash
./layer_dp_gate_ck 5 --threads 8 --caps $CAPS --save-layer 3 l3.snap --ref ...csv
./layer_dp_gate_ck 5 --threads 8 --caps $CAPS --load-layer 3 l3.snap --ref ...csv --dump d.csv
cmp d.csv golden_c5_dump.csv               # byte-identical class table
```

Step 2 -- wide-path parity (exercises u128 accumulation + wide checkpoint sections):

```bash
./layer_dp_gate_ck 5 --threads 8 --caps $CAPS --force-wide --ref ...csv   # same N, 355/355
```

Step 3 -- the kill-loop proper (20 iterations, random SIGKILL, resume each time).
`--checkpoint ck 0` dumps at EVERY chunk barrier and `--ckpt-chunk 500` forces many
chunks per transition at C=5 scale (the production 100k default would make C=5
transitions single-chunk and never test mid-transition resume). Save as
`kill_loop_c5.sh`, run from `experiments/proto` in Git Bash (kill -9 is a hard
TerminateProcess on Windows -- exactly the crash model we want; mid-dump kills are
covered statistically by the every-chunk dump cadence):

```bash
#!/usr/bin/env bash
set -u
BIN=./layer_dp_gate_ck
REF=../../docs/expert/2026-07-21/native_c5_response_quotient_triples.csv
CAPS="$1"; THREADS=${2:-8}
EXPN='N(5) = 1903816047972624930994913280000'
for it in $(seq 1 20); do
  D=ck_it$it; rm -rf "$D"; mkdir -p "$D"
  ARGS="5 --threads $THREADS --caps $CAPS --ref $REF --dump $D/dump.csv \
        --checkpoint $D/ck 0 --ckpt-chunk 500"
  $BIN $ARGS > "$D/log0" 2>&1 & PID=$!
  k=0
  while :; do
    sleep $(awk -v s=$RANDOM 'BEGIN{srand(s); printf "%.2f", 0.3+rand()*4}')
    kill -0 $PID 2>/dev/null || break            # finished on its own
    kill -9 $PID 2>/dev/null; wait $PID 2>/dev/null
    k=$((k+1))
    if [ -f "$D/ck.a" ] || [ -f "$D/ck.b" ]; then R="--resume $D/ck"; else R=""; fi
    $BIN $ARGS $R > "$D/log$k" 2>&1 & PID=$!
  done
  wait $PID; RC=$?
  LAST=$(ls "$D"/log* | sort -V | tail -1)
  if [ $RC -ne 0 ] || ! grep -qF "$EXPN" "$LAST" \
     || ! grep -q 'reference triples: 355 rows, 355 matched OK' "$LAST" \
     || ! grep -q 'ALL GATES PASSED' "$LAST"; then
    echo "ITER $it FAILED after $k kills (rc=$RC, log=$LAST)"; exit 1
  fi
  cmp -s "$D/dump.csv" golden_c5_dump.csv || { echo "ITER $it dump differs"; exit 1; }
  echo "ITER $it OK after $k kills"
done
echo "KILL-LOOP PASSED (20 iterations)"
```

Pass criteria per iteration: exit 0; exact N(5); `reference triples: 355 rows, 355
matched OK`; `ALL GATES PASSED`; `--dump` byte-identical to the golden clean-run dump
(this last check is the strong one -- it compares the full canonical (key, m, ell, F)
table, i.e. every T and every stab, not just the aggregate). Sanity across the soak:
`grep -h 'resume: transition' ck_it*/log* | sort | uniq -c` must show resumes landing at
nonzero cursors in BOTH the 3->4 and 4->5 transitions (proves mid-transition coverage),
and at least one log should show the loader falling back or a `.tmp` file left behind
(mid-dump kill). Optionally repeat 5 iterations with `--force-wide` appended to ARGS to
soak the wide checkpoint sections under kills.

## (4) Effort estimate (apply + validate, wall clock)

- Apply E1-E8 (blocks above are paste-ready; E7 is the only delicate one): 2-3 h
- Compile, fix pedantic breakage, C=2/3/4 smoke: 0.5-1 h
- Step 0-2 parity gates (serial + threaded + save/load + force-wide): ~1 h
- Step 3 kill-loop, 20 iterations (C=5 threaded run is minutes; kills roughly double
  each iteration): 2-5 h, mostly unattended
- Total: ~6-10 h wall, ~4-5 h attended. (Consistent with the spec's 19 h estimate for
  the full P1-P7 scope; this plan implements P1-P4+P7 and defers telemetry/preflight
  P5-P6.)

Known accepted costs / follow-ups (do not block apply): final-checkpoint + snapshot
double-writes the finalized child (~2x34 GB once per transition at C=6 scale -- can
later be a rename of the final checkpoint); `parentKeysHash` is recomputed per dump
(~0.1 s per 295 MB at 3->4 -- can be cached per transition); a resumed run's
`emissions:` summary line lists only transitions from the resumed one onward.

## Appendix: pre-apply anchor verification (run IMMEDIATELY before applying)

```bash
cd E:/Code/sudoku_FJ/experiments/proto
grep -cF 'include <psapi.h>' layer_dp_gate.cpp                                   # 1 (E1)
grep -cF -- '-- main ---' layer_dp_gate.cpp                                      # 1 (E2)
grep -cF 'layer-mass L] [--dump f]' layer_dp_gate.cpp                            # 1 (E3)
grep -cF -- '--caps" && a + 1 < argc' layer_dp_gate.cpp                          # 1 (E4)
grep -cF 'caps.size() < (size_t)(C - 1)' layer_dp_gate.cpp                       # 1 (E5)
grep -cF 'double t_all0 = now_s();' layer_dp_gate.cpp                            # 1 (E6)
grep -cF 'double t0 = now_s();' layer_dp_gate.cpp                                # 1 (E7)
grep -cF 'reduction(+ : totEmissions, totHits)' layer_dp_gate.cpp                # 1 (E7)
grep -cF 'u128 Tw = (C >= 6)' layer_dp_gate.cpp                                  # 1 (E8)
```

All counts must be exactly 1. E7 additionally requires the whole old block to match
byte-for-byte; if the concurrent editor touched anything inside the transition driver,
re-read lines around `double t0 = now_s();` and re-derive only the old-block text -- the
replacement block remains valid as long as `EmitCtx`'s public fields (`next`, `row`,
`wideT`, `wide`, `emissions`, `cacheHits`, `prepare`, `rec`) keep their signatures.
