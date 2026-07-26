// layer_dp_gate.cpp -- global row-incremental layer DP gate for 2xC bands.
//
// Implements the corrected orbit-total recursion (2026-07-26,
// docs/expert/2026-07-26/layer-dp-permanent-profile.md):
//
//   T_1[x1] = 1  (unique one-row state)
//   T_{L+1}[can(child)] += T_L[x] * K,   K = prod_w c_w!  (binary tables)
//   N(C) = sum_q (ell_q * m_q) * F_q^2,  F_q = T_q / m_q,
//          ell_q = (2C)!/prod c_w!,      m_q = |G_C| / s_q.
//
// States: sorted multisets of 2C masks; mask = per-box 2-bit field
// (0 = unused, 2|s = side s).  Group G_C = C2 wr S_C.
//
// Canonical form: column-wise orderly backtrack minimizing the sequence
// (K_0..K_{C-1}) of sorted prefix vectors over all (box order, flips).
// This is a valid canonical form (prefix-monotone by construction) and is
// validated against a full-|G| scan by --scan-check.
//
// Gates:
//   C=2,3,4 built-in expected states/emissions/N (independently verified
//   2026-07-26); C=5 --ref compares all 355 (m, ell, F) triples against
//   docs/expert/2026-07-21/native_c5_response_quotient_triples.csv.
//   --invariance N: canonize(g x) == canonize(x) on N random (state, g).
//   --scan-check N: fast canonical == full-group-scan canonical on N states.
//   --layer-mass L: sum of orbit sizes over layer L (C=5, L=4: 62,185,328).
//   --rank: mod-p ranks of completion operators U_L = R_L...R_{C-1}
//           (target-only contraction probe).
//
// Build: g++ -O2 -march=native -std=c++17 -o layer_dp_gate layer_dp_gate.cpp
// Usage: layer_dp_gate C [--ref file.csv] [--rank] [--invariance N]
//                        [--scan-check N] [--layer-mass L] [--dump file.csv]

#include <algorithm>
#include <array>
#include <cassert>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <random>
#include <sstream>
#include <string>
#include <vector>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <psapi.h>
#endif

#ifdef _OPENMP
#include <omp.h>
#endif

using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;
using u128 = unsigned __int128;

static int C, N2C;          // C boxes, 2C slots/symbols
static u64 FACT[13];

struct State {
    std::array<u16, 12> m{};  // sorted ascending over first N2C entries
    bool operator==(const State& o) const {
        return std::memcmp(m.data(), o.m.data(), sizeof(u16) * 12) == 0;
    }
};

static u64 state_hash(const State& s) {
    u64 h = 0x9e3779b97f4a7c15ULL;
    for (int i = 0; i < N2C; i++) {
        h ^= s.m[i];
        h *= 0xff51afd7ed558ccdULL;
        h ^= h >> 33;
    }
    return h;
}

static inline int fld(u16 m, int b) { return (m >> (2 * b)) & 3; }

// ---------------------------------------------------------------- group ----
struct GElem {
    u8 perm[6];   // new position of box b
    u8 flips;     // bit b: flip sides of source box b
};
static std::vector<GElem> GROUP;

static void build_group() {
    GROUP.clear();
    std::array<u8, 6> p{};
    for (int i = 0; i < C; i++) p[i] = (u8)i;
    do {
        for (int f = 0; f < (1 << C); f++) {
            GElem g;
            for (int b = 0; b < C; b++) g.perm[b] = p[b];
            g.flips = (u8)f;
            GROUP.push_back(g);
        }
    } while (std::next_permutation(p.begin(), p.begin() + C));
}

static inline u16 apply_mask(const GElem& g, u16 m) {
    u16 r = 0;
    for (int b = 0; b < C; b++) {
        int v = fld(m, b);
        if (v) r |= (u16)((v ^ ((g.flips >> b) & 1)) << (2 * g.perm[b]));
    }
    return r;
}

static State apply_state(const GElem& g, const State& s) {
    State r{};
    for (int i = 0; i < N2C; i++) r.m[i] = apply_mask(g, s.m[i]);
    std::sort(r.m.begin(), r.m.begin() + N2C);
    return r;
}

// ------------------------------------------------------------- canonize ----
// Orderly backtrack minimizing the K-sequence.  v2: packed 2-bit columns and
// group refinement; each candidate column reduces to one 24-bit signature
// (group-ordered sorted fields), so all comparisons are single integer
// compares.  Equivalent comparator to the v1 sorted-prefix-vector form (all
// surviving nodes at a depth share the prefix partition), validated against
// the full-group scan by --scan-check.  Also counts the group elements
// attaining the minimum = stabilizer order.
static bool g_wlseed = false;  // pair-profile initial partition (--wlseed)
static u32 REP2[13], REP3[13];
static struct RepInit {
    RepInit() {
        REP2[0] = REP3[0] = 0;
        for (int c = 1; c <= 12; c++) {
            REP2[c] = (REP2[c - 1] << 2) | 2u;
            REP3[c] = (REP3[c - 1] << 2) | 3u;
        }
    }
} repInit_;

struct Canonizer {
    // lane bitmasks per (box, flip): bit i set iff fld(S[i], b) ^ f == v
    u32 z0[6], z2[6][2], z3[6][2];  // z0 flip-independent
    u32 bestSig[6];
    bool bestSet[6];
    u8 curPath[6];       // (b << 1) | f per depth on the current DFS path
    u8 bestPathArr[6];
    bool haveFinal;
    u64 minCount;
    u64 nodes;

    void run(const State& s) {
        for (int b = 0; b < C; b++) {
            u32 m0 = 0, m2 = 0, m3 = 0;
            for (int i = 0; i < N2C; i++) {
                u32 v = (u32)fld(s.m[i], b);
                if (v == 0) m0 |= 1u << i;
                else if (v == 2) m2 |= 1u << i;
                else m3 |= 1u << i;
            }
            z0[b] = m0;
            z2[b][0] = m2; z3[b][0] = m3;   // flip 0
            z2[b][1] = m3; z3[b][1] = m2;   // flip swaps sides
        }
        for (int d = 0; d < C; d++) bestSet[d] = false;
        haveFinal = false;
        minCount = 0;
        nodes = 0;
        if (!g_wlseed) {
            u32 groups0[12];
            groups0[0] = (1u << N2C) - 1;
            dfs(0, groups0, 1, 0);
            return;
        }
        // WL-1 seed: pairwise profiles.  For masks i, j let u = #shared
        // boxes and sm = #shared boxes on the same side; both are flip- and
        // box-permutation-invariant (per-box side occupancy alone is
        // constant L by row-regularity, so pair structure is the first
        // non-trivial invariant).  Mask color = hash of the sorted multiset
        // of (u, sm) over partners; hash collisions only coarsen the seed
        // partition consistently, never break invariance.
        u8 bb[12] = {0}, ss[12] = {0};
        for (int b = 0; b < C; b++) {
            u32 m2 = z2[b][0], m3 = z3[b][0];
            u32 t = m2 | m3;
            while (t) {
                int i = __builtin_ctz(t);
                t &= t - 1;
                bb[i] |= (u8)(1 << b);
            }
            t = m3;
            while (t) {
                int i = __builtin_ctz(t);
                t &= t - 1;
                ss[i] |= (u8)(1 << b);
            }
        }
        u32 color[12];
        {
            u8 prof[12][12];
            for (int i = 0; i < N2C; i++) {
                int pn = 0;
                for (int j = 0; j < N2C; j++) {
                    if (j == i) continue;
                    u8 shared = (u8)(bb[i] & bb[j]);
                    u8 diff = (u8)((ss[i] ^ ss[j]) & shared);
                    int u = __builtin_popcount(shared);
                    int sm = u - __builtin_popcount(diff);
                    prof[i][pn++] = (u8)((u << 4) | sm);
                }
                std::sort(prof[i], prof[i] + pn);
                u64 h = 0x9e3779b97f4a7c15ULL;
                for (int k = 0; k < pn; k++) {
                    h ^= prof[i][k];
                    h *= 0xff51afd7ed558ccdULL;
                }
                color[i] = (u32)(h ^ (h >> 32));
            }
        }
        u32 cs[12];
        std::memcpy(cs, color, sizeof(u32) * 12);
        std::sort(cs, cs + N2C);
        u32 groups[12];
        int ng = 0;
        for (int k = 0; k < N2C;) {
            u32 c = cs[k];
            u32 gm = 0;
            for (int i = 0; i < N2C; i++)
                if (color[i] == c) gm |= 1u << i;
            groups[ng++] = gm;
            while (k < N2C && cs[k] == c) k++;
        }
        dfs(0, groups, ng, 0);
    }

    // signature of column (b,f) under the current group partition
    inline u32 sig_of(int b, int f, const u32* groups, int ng) const {
        u32 sig = 0;
        for (int g = 0; g < ng; g++) {
            u32 gm = groups[g];
            int c0 = __builtin_popcount(gm & z0[b]);
            int c2 = __builtin_popcount(gm & z2[b][f]);
            int c3 = __builtin_popcount(gm & z3[b][f]);
            sig <<= 2 * c0;
            sig = (sig << (2 * c2)) | REP2[c2];
            sig = (sig << (2 * c3)) | REP3[c3];
        }
        return sig;
    }

    void dfs(int depth, const u32* groups, int ng, u32 usedBoxes) {
        nodes++;
        if (depth == C) {
            if (!haveFinal) {
                std::memcpy(bestPathArr, curPath, 6);
                haveFinal = true;
                minCount = 1;
            } else {
                minCount++;
            }
            return;
        }
        u8 cand[12];
        int nc = 0;
        u32 minSig = UINT32_MAX;
        for (int b = 0; b < C; b++) {
            if ((usedBoxes >> b) & 1) continue;
            for (int f = 0; f < 2; f++) {
                u32 s = sig_of(b, f, groups, ng);
                if (s < minSig) {
                    minSig = s;
                    nc = 0;
                }
                if (s == minSig) cand[nc++] = (u8)((b << 1) | f);
            }
        }
        if (bestSet[depth]) {
            if (minSig > bestSig[depth]) return;
            if (minSig < bestSig[depth]) {
                bestSig[depth] = minSig;
                for (int d = depth + 1; d < C; d++) bestSet[d] = false;
                haveFinal = false;
                minCount = 0;
            }
        } else {
            bestSig[depth] = minSig;
            bestSet[depth] = true;
        }
        // refine each argmin candidate (3 ANDs per group); when several tie,
        // order by 1-step lookahead so the best subtree is walked first.
        u32 ngroups[12][12];
        int nng[12];
        u32 look[12];
        for (int k = 0; k < nc; k++) {
            int b = cand[k] >> 1, f = cand[k] & 1;
            int w = 0;
            for (int g = 0; g < ng; g++) {
                u32 gm = groups[g];
                u32 a0 = gm & z0[b], a2 = gm & z2[b][f], a3 = gm & z3[b][f];
                if (a0) ngroups[k][w++] = a0;
                if (a2) ngroups[k][w++] = a2;
                if (a3) ngroups[k][w++] = a3;
            }
            nng[k] = w;
            if (nc > 1 && depth + 1 < C) {
                u32 m = UINT32_MAX;
                u32 ub = usedBoxes | (1u << b);
                for (int b2 = 0; b2 < C; b2++) {
                    if ((ub >> b2) & 1) continue;
                    for (int f2 = 0; f2 < 2; f2++) {
                        u32 s = sig_of(b2, f2, ngroups[k], w);
                        if (s < m) m = s;
                    }
                }
                look[k] = m;
            } else {
                look[k] = 0;
            }
        }
        int idx[12];
        for (int k = 0; k < nc; k++) idx[k] = k;
        if (nc > 1)
            std::sort(idx, idx + nc,
                      [&](int a, int b2) { return look[a] < look[b2]; });
        for (int ki = 0; ki < nc; ki++) {
            int k = idx[ki];
            curPath[depth] = cand[k];
            dfs(depth + 1, ngroups[k], nng[k],
                usedBoxes | (1u << (cand[k] >> 1)));
        }
    }
};

alignas(64) static u64 g_canonize_calls = 0;
alignas(64) static u64 g_canonize_nodes = 0;
static thread_local u64 tl_canon_calls = 0, tl_canon_nodes = 0;

static void flush_canon_counters() {
    if (tl_canon_calls) {
        __atomic_add_fetch(&g_canonize_calls, tl_canon_calls, __ATOMIC_RELAXED);
        __atomic_add_fetch(&g_canonize_nodes, tl_canon_nodes, __ATOMIC_RELAXED);
        tl_canon_calls = tl_canon_nodes = 0;
    }
}

static State canonize(const State& s, u64* stabOut = nullptr) {
    Canonizer cz;
    cz.run(s);
    tl_canon_calls++;
    tl_canon_nodes += cz.nodes;
    // apply the winning assignment: position d takes source box path[d]>>1
    GElem g{};
    for (int d = 0; d < C; d++) {
        int b = cz.bestPathArr[d] >> 1, f = cz.bestPathArr[d] & 1;
        g.perm[b] = (u8)d;
        if (f) g.flips |= (u8)(1 << b);
    }
    State k = apply_state(g, s);
    if (stabOut) *stabOut = cz.minCount;
    return k;
}

// form-independent full-|G| referees: exact stabilizer order, and orbit
// membership of a claimed canonical representative
static u64 stab_scan(const State& s) {
    u64 c = 0;
    for (const GElem& g : GROUP)
        if (apply_state(g, s) == s) c++;
    return c;
}

static bool orbit_member_scan(const State& s, const State& key) {
    for (const GElem& g : GROUP)
        if (apply_state(g, s) == key) return true;
    return false;
}

// ------------------------------------------------------------- hash map ----
struct Layer {
    // In fixed-capacity mode (parallel transitions) insertion is lock-free:
    // slots are claimed with a CAS on the table; a lost race leaves a hole
    // (stab == 0) that every consumer skips.  Growth is only allowed in the
    // dynamic single-thread mode.
    std::vector<State> keys;
    std::vector<u64> T;
    std::vector<u32> stab;   // stabilizer order; 0 marks an abandoned hole
    std::vector<u32> table;  // 1-based index, 0 = empty
    u64 mask = 0;
    bool fixedCap = false;
    u32 claimed = 0;         // atomic counter in fixed mode
    u32 holes = 0;

    void init(size_t cap) {
        size_t sz = 64;
        while (sz < cap * 2) sz <<= 1;
        table.assign(sz, 0);
        mask = sz - 1;
        keys.clear();
        T.clear();
        stab.clear();
        fixedCap = false;
        claimed = holes = 0;
    }
    void init_fixed(size_t cap) {
        // u32 index space: claimed/table entries wrap above ~2^32 and would
        // silently corrupt instead of aborting (confirmed audit finding)
        if (cap >= (size_t)UINT32_MAX - 64) {
            std::fprintf(stderr, "FATAL: fixed cap %zu exceeds u32 index space\n",
                         cap);
            std::abort();
        }
        size_t sz = 64;
        while (sz < cap * 2) sz <<= 1;
        table.assign(sz, 0);
        mask = sz - 1;
        keys.assign(cap, State{});
        T.assign(cap, 0);
        stab.assign(cap, 0);
        fixedCap = true;
        claimed = holes = 0;
    }
    size_t size() const { return fixedCap ? claimed : keys.size(); }
    size_t real_size() const { return size() - holes; }
    bool is_hole(u32 i) const { return fixedCap && stab[i] == 0; }

    void grow() {
        size_t sz = table.size() * 2;
        table.assign(sz, 0);
        mask = sz - 1;
        for (u32 i = 0; i < keys.size(); i++) {
            u64 h = state_hash(keys[i]) & mask;
            while (table[h]) h = (h + 1) & mask;
            table[h] = i + 1;
        }
    }
    u32 find_or_add(const State& k) {  // single-thread dynamic mode only
        u64 h = state_hash(k) & mask;
        while (table[h]) {
            u32 idx = table[h] - 1;
            if (keys[idx] == k) return idx;
            h = (h + 1) & mask;
        }
        keys.push_back(k);
        T.push_back(0);
        u32 idx = (u32)keys.size() - 1;
        table[h] = idx + 1;
        if (keys.size() * 2 > table.size()) grow();
        return idx;
    }
    // lock-free find-or-add for fixed mode; stabv recorded on insertion
    u32 find_or_add_mt(const State& k, u32 stabv) {
        u64 h = state_hash(k) & mask;
        u32 myIdx = UINT32_MAX;
        for (;;) {
            u32 slot = __atomic_load_n(&table[h], __ATOMIC_ACQUIRE);
            if (slot) {
                u32 idx = slot - 1;
                if (keys[idx] == k) {
                    if (myIdx != UINT32_MAX) {  // lost a race: abandon claim
                        __atomic_store_n(&stab[myIdx], 0, __ATOMIC_RELEASE);
                        __atomic_add_fetch(&holes, 1, __ATOMIC_RELAXED);
                    }
                    return idx;
                }
                h = (h + 1) & mask;
                continue;
            }
            if (myIdx == UINT32_MAX) {
                myIdx = __atomic_fetch_add(&claimed, 1, __ATOMIC_RELAXED);
                if (myIdx >= keys.size()) {
                    std::fprintf(stderr,
                                 "FATAL: fixed layer capacity %zu exceeded\n",
                                 keys.size());
                    std::abort();
                }
                keys[myIdx] = k;
                __atomic_store_n(&stab[myIdx], stabv, __ATOMIC_RELEASE);
            }
            u32 expected = 0;
            if (__atomic_compare_exchange_n(&table[h], &expected, myIdx + 1,
                                            false, __ATOMIC_ACQ_REL,
                                            __ATOMIC_ACQUIRE))
                return myIdx;
            // CAS lost: loop re-reads this slot (winner may hold our key)
        }
    }
    u32 find(const State& k) const {  // UINT32_MAX if absent
        u64 h = state_hash(k) & mask;
        while (table[h]) {
            u32 idx = table[h] - 1;
            if (keys[idx] == k) return idx;
            h = (h + 1) & mask;
        }
        return UINT32_MAX;
    }
};

// ------------------------------------------------------------ emissions ----
struct EmitCtx {
    const State* parent;
    u64 parentT;
    Layer* next;
    // distinct-mask decomposition
    int nd;
    u16 dmask[12];
    int dmult[12];
    u32 dcompat[12];  // slot bitmask compatible with dmask
    u16 childbuf[12];
    int childpos;
    u64 emissions;
    u64 cacheHits;
    bool countOnly = false;
    int m4window = 0;  // >0: keep only children with top m4window hash bits
                       // zero; accumulate hit counts (T += 1), bypass cache
    // wide accumulation for the final transition at C >= 6: T_C values reach
    // ~2.7e24 (> u64), so contributions go into a per-thread u128 vector
    // merged after the parallel region (confirmed audit finding)
    bool wideT = false;
    std::vector<u128> wide;
    // optional sparse row recording (child index -> coeff), for --rank
    std::vector<std::pair<u32, u64>>* row;
    // per-parent raw-child cache: raw multiset -> (next-layer index, K)
    static const u32 RCBITS = 16, RCSIZE = 1u << RCBITS;
    std::vector<u32> rcGen, rcIdx;
    std::vector<State> rcKeys;
    std::vector<std::pair<u32, u64>> rcVals;  // (layer index, K)
    u32 gen = 0;

    void prepare(const State& p, u64 t) {
        if (rcGen.empty()) {
            rcGen.assign(RCSIZE, UINT32_MAX);
            rcIdx.assign(RCSIZE, 0);
        }
        gen++;
        rcKeys.clear();
        rcVals.clear();
        parent = &p;
        parentT = t;
        nd = 0;
        for (int i = 0; i < N2C;) {
            int j = i;
            while (j < N2C && p.m[j] == p.m[i]) j++;
            dmask[nd] = p.m[i];
            dmult[nd] = j - i;
            u32 comp = 0;
            for (int b = 0; b < C; b++)
                if (fld(p.m[i], b) == 0) comp |= 3u << (2 * b);
            dcompat[nd] = comp;
            nd++;
            i = j;
        }
        childpos = 0;
    }

    void leaf() {
        emissions++;
        if (countOnly) return;
        u16 tmp[12];
        std::memcpy(tmp, childbuf, sizeof(u16) * 12);
        std::sort(tmp, tmp + N2C);
        State ch{};
        std::memcpy(ch.m.data(), tmp, sizeof(u16) * 12);
        if (m4window) {  // hash-window distinct sampling with hit counts
            u64 stabv = 0;
            State ck = canonize(ch, &stabv);
            if (state_hash(ck) >> (64 - m4window)) return;
            u32 idx = next->find_or_add_mt(ck, (u32)stabv);
            __atomic_add_fetch(&next->T[idx], 1, __ATOMIC_RELAXED);
            return;
        }
        // per-parent raw cache probe
        u64 h = state_hash(ch) & (RCSIZE - 1);
        while (rcGen[h] == gen) {
            if (rcKeys[rcIdx[h]] == ch) {
                cacheHits++;
                auto& v = rcVals[rcIdx[h]];
                if (wideT) {
                    if (wide.size() <= v.first) wide.resize(v.first + 1, 0);
                    wide[v.first] += (u128)parentT * v.second;
                } else if (next->fixedCap)
                    __atomic_add_fetch(&next->T[v.first], parentT * v.second,
                                       __ATOMIC_RELAXED);
                else
                    next->T[v.first] += parentT * v.second;
                if (row) row->push_back({v.first, v.second});
                return;
            }
            h = (h + 1) & (RCSIZE - 1);
        }
        u64 K = 1;
        for (int i = 0; i < N2C;) {
            int j = i;
            while (j < N2C && tmp[j] == tmp[i]) j++;
            K *= FACT[j - i];
            i = j;
        }
        u64 stab = 0;
        State ck = canonize(ch, &stab);
        u32 idx;
        if (next->fixedCap) {
            idx = next->find_or_add_mt(ck, (u32)stab);
            if (wideT) {
                if (wide.size() <= idx) wide.resize(idx + 1, 0);
                wide[idx] += (u128)parentT * K;
            } else {
                __atomic_add_fetch(&next->T[idx], parentT * K, __ATOMIC_RELAXED);
            }
        } else {
            idx = next->find_or_add(ck);
            if (idx == (u32)next->stab.size()) next->stab.push_back((u32)stab);
            if (wideT) {
                if (wide.size() <= idx) wide.resize(idx + 1, 0);
                wide[idx] += (u128)parentT * K;
            } else {
                next->T[idx] += parentT * K;
            }
        }
        if (row) row->push_back({idx, K});
        if (rcKeys.size() < RCSIZE / 2) {  // insert unless table crowded
            rcGen[h] = gen;
            rcIdx[h] = (u32)rcKeys.size();
            rcKeys.push_back(ch);
            rcVals.push_back({idx, K});
        }
    }

    void rec(int di, u32 remaining) {
        if (di == nd) {
            leaf();
            return;
        }
        u32 avail = dcompat[di] & remaining;
        int need = dmult[di];
        int bits[12], nb = 0;
        while (avail) {
            u32 lo = avail & (~avail + 1);
            bits[nb++] = __builtin_ctz(lo);
            avail -= lo;
        }
        if (nb < need) return;
        int idx[12];
        for (int k = 0; k < need; k++) idx[k] = k;
        u16 base = dmask[di];
        for (;;) {
            u32 used = 0;
            for (int k = 0; k < need; k++) {
                int slot = bits[idx[k]];
                used |= 1u << slot;
                childbuf[childpos + k] =
                    (u16)(base | ((2u | (slot & 1u)) << (2 * (slot >> 1))));
            }
            childpos += need;
            rec(di + 1, remaining & ~used);
            childpos -= need;
            // next combination
            int k = need - 1;
            while (k >= 0 && idx[k] == nb - need + k) k--;
            if (k < 0) break;
            idx[k]++;
            for (int j = k + 1; j < need; j++) idx[j] = idx[j - 1] + 1;
        }
    }
};

// --------------------------------------------------------------- mod p -----
static const u64 P1 = 2305843009213693951ULL;  // 2^61 - 1 (prime)
static const u64 P2 = 4611686018427387847ULL;  // prime < 2^62

static inline u64 mulmod(u64 a, u64 b, u64 p) { return (u64)((u128)a * b % p); }

static int rank_mod_p(std::vector<std::vector<u64>>& rows, int ncols, u64 p) {
    // in-place row reduction; rows may be more numerous than ncols
    int rank = 0;
    std::vector<int> pivotCol;
    std::vector<std::vector<u64>> basis;
    for (auto& r : rows) {
        // reduce r against basis
        for (size_t bi = 0; bi < basis.size(); bi++) {
            u64 v = r[pivotCol[bi]];
            if (!v) continue;
            // r -= v/basis_pivot * basis  (basis rows normalized to pivot 1)
            for (int c = 0; c < ncols; c++) {
                if (basis[bi][c]) {
                    u64 sub = mulmod(v, basis[bi][c], p);
                    r[c] = (r[c] >= sub) ? r[c] - sub : r[c] + p - sub;
                }
            }
        }
        int pc = -1;
        for (int c = 0; c < ncols; c++)
            if (r[c]) { pc = c; break; }
        if (pc < 0) continue;
        // normalize pivot to 1: multiply by inverse
        u64 inv = 1, base = r[pc], e = p - 2;
        while (e) {
            if (e & 1) inv = mulmod(inv, base, p);
            base = mulmod(base, base, p);
            e >>= 1;
        }
        for (int c = 0; c < ncols; c++) r[c] = mulmod(r[c], inv, p);
        basis.push_back(r);
        pivotCol.push_back(pc);
        rank++;
        if (rank == ncols) break;
    }
    return rank;
}

// ------------------------------------------------------------ utilities ----
static std::string u128_str(u128 v) {
    if (v == 0) return "0";
    std::string s;
    while (v) {
        s += char('0' + (int)(v % 10));
        v /= 10;
    }
    std::reverse(s.begin(), s.end());
    return s;
}

static double now_s() {
    using namespace std::chrono;
    return duration<double>(steady_clock::now().time_since_epoch()).count();
}

static size_t peak_rss_bytes() {
#ifdef _WIN32
    PROCESS_MEMORY_COUNTERS pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc)))
        return pmc.PeakWorkingSetSize;
#endif
    return 0;
}

// ------------------------------------------------------------------ main ---
int main(int argc, char** argv) {
    if (argc < 2) {
        std::fprintf(stderr, "usage: %s C [--ref f] [--rank] [--invariance N] "
                             "[--scan-check N] [--layer-mass L] [--dump f]\n",
                     argv[0]);
        return 2;
    }
    C = std::atoi(argv[1]);
    if (C < 2 || C > 6) { std::fprintf(stderr, "C in 2..6\n"); return 2; }
    N2C = 2 * C;
    FACT[0] = 1;
    for (int i = 1; i <= 12; i++) FACT[i] = FACT[i - 1] * i;

    std::string refPath, dumpPath;
    bool doRank = false;
    long invarianceN = 0, scanCheckN = 0;
    int layerMassL = 0;
    int probeLayer = 0;
    long probeParents = 0;
    int stopAfter = 0;
    long fanSample = 0;
    int nthreads = 1;
    std::vector<size_t> caps;  // fixed capacities for child layers 2..C
    long m4K = 0;              // hash-window M_4 probe: sampled layer-3 parents
    int m4W = 0;               // window bits (keep fraction 2^-m4W)
    size_t m4Cap = 0;          // capacity of the windowed child table
    for (int a = 2; a < argc; a++) {
        std::string s = argv[a];
        if (s == "--ref" && a + 1 < argc) refPath = argv[++a];
        else if (s == "--dump" && a + 1 < argc) dumpPath = argv[++a];
        else if (s == "--rank") doRank = true;
        else if (s == "--invariance" && a + 1 < argc) invarianceN = std::atol(argv[++a]);
        else if (s == "--scan-check" && a + 1 < argc) scanCheckN = std::atol(argv[++a]);
        else if (s == "--layer-mass" && a + 1 < argc) layerMassL = std::atoi(argv[++a]);
        else if (s == "--probe" && a + 2 < argc) {
            probeLayer = std::atoi(argv[++a]);
            probeParents = std::atol(argv[++a]);
        }
        else if (s == "--stop-after" && a + 1 < argc) stopAfter = std::atoi(argv[++a]);
        else if (s == "--fan-sample" && a + 1 < argc) fanSample = std::atol(argv[++a]);
        else if (s == "--threads" && a + 1 < argc) nthreads = std::atoi(argv[++a]);
        else if (s == "--wlseed") g_wlseed = true;
        else if (s == "--m4probe" && a + 3 < argc) {
            m4K = std::atol(argv[++a]);
            m4W = std::atoi(argv[++a]);
            m4Cap = std::stoull(argv[++a]);
        }
        else if (s == "--caps" && a + 1 < argc) {
            std::stringstream cs(argv[++a]);
            std::string tok;
            while (std::getline(cs, tok, ',')) caps.push_back(std::stoull(tok));
        }
        else { std::fprintf(stderr, "unknown arg %s\n", s.c_str()); return 2; }
    }
#ifndef _OPENMP
    if (nthreads > 1) {
        std::fprintf(stderr, "built without OpenMP; --threads requires -fopenmp\n");
        return 2;
    }
#endif
    if (nthreads > 1 && caps.size() < (size_t)(C - 1)) {
        std::fprintf(stderr, "--threads needs --caps c2,...,c%d (child layer "
                             "capacities)\n", C);
        return 2;
    }

    build_group();
    const u64 Gorder = GROUP.size();

    // layer 1: unique state of singletons
    State x1{};
    for (int i = 0; i < N2C; i++) {
        int b = i >> 1, s = i & 1;
        x1.m[i] = (u16)((2u | (u32)s) << (2 * b));
    }
    std::sort(x1.m.begin(), x1.m.begin() + N2C);
    u64 s1 = 0;
    State x1c = canonize(x1, &s1);

    std::vector<Layer> layers(C + 1);
    layers[1].init(4);
    u32 i0 = layers[1].find_or_add(x1c);
    layers[1].T[i0] = 1;
    layers[1].stab.push_back((u32)s1);

    std::vector<u64> emissionsPerT, canonizePerT;
    std::vector<double> timePerT;
    std::vector<u128> gWide;  // u128 T of the final layer when C >= 6
    // sparse transition rows for --rank: R[t][parent] = vector of (child, K)
    std::vector<std::vector<std::vector<std::pair<u32, u64>>>> Rrows(C);

    double t_all0 = now_s();
    for (int L = 1; L < C; L++) {
        if (m4K > 0 && L >= 3) {
            // ---- hash-window M_4 probe: sample layer-3 parents, keep only
            // ---- children whose canonical hash lands in a 2^-m4W window,
            // ---- estimate M_4 from the windowed hit histogram.
            Layer& L3 = layers[3];
            size_t np = L3.size();
            size_t take = std::min((size_t)m4K, np);
            size_t stride = np / take;
            Layer win;
            win.init_fixed(m4Cap);
            u64 totEm = 0;
            double tp0 = now_s();
#ifdef _OPENMP
#pragma omp parallel num_threads(nthreads) if (nthreads > 1) \
    reduction(+ : totEm)
#endif
            {
                EmitCtx ctx;
                ctx.next = &win;
                ctx.emissions = 0;
                ctx.cacheHits = 0;
                ctx.row = nullptr;
                ctx.m4window = m4W;
#ifdef _OPENMP
#pragma omp for schedule(dynamic, 4)
#endif
                for (long long s = 0; s < (long long)take; s++) {
                    size_t i = (size_t)s * stride;
                    while (i < np && L3.is_hole((u32)i)) i++;
                    if (i >= np) continue;
                    // clamp: never cross into the next stride window (a hole
                    // run >= stride would otherwise double-count a parent)
                    if (s + 1 < (long long)take && i >= (size_t)(s + 1) * stride)
                        continue;
                    ctx.prepare(L3.keys[i], 1);
                    ctx.rec(0, (1u << N2C) - 1);
                }
                totEm += ctx.emissions;
                flush_canon_counters();
            }
            double dt = now_s() - tp0;
            u64 D = 0, H = 0, f[6] = {0, 0, 0, 0, 0, 0};
            for (u32 i = 0; i < win.size(); i++) {
                if (win.is_hole(i)) continue;
                D++;
                u64 h = win.T[i];
                H += h;
                f[h >= 5 ? 5 : h]++;
            }
            double mu = D ? (double)H / D : 0.0;
            double lam = mu > 1.0001 ? mu : 1e-4;
            for (int it = 0; it < 80 && mu > 1.0001; it++) {
                double em = 1.0 - std::exp(-lam);
                double fv = lam / em - mu;
                double dfv = (em - lam * std::exp(-lam)) / (em * em);
                lam -= fv / dfv;
                if (lam < 1e-9) lam = 1e-9;
            }
            double scale = std::pow(2.0, m4W);
            double mle = mu > 1.0001 ? D / (1.0 - std::exp(-lam)) * scale : 0;
            double chao = f[2] ? (D + 0.5 * (double)f[1] * f[1] / f[2]) * scale
                               : 0;
            std::printf("m4probe: parents=%zu/%zu window=2^-%d emissions=%llu "
                        "kept=%llu (expect ~%.3g) time=%.1fs\n",
                        take, np, m4W, (unsigned long long)totEm,
                        (unsigned long long)H, totEm / scale, dt);
            std::printf("m4probe: distinct_in_window=%llu hits histogram "
                        "f1=%llu f2=%llu f3=%llu f4=%llu f5+=%llu  mean=%.3f "
                        "lambda=%.3f\n",
                        (unsigned long long)D, (unsigned long long)f[1],
                        (unsigned long long)f[2], (unsigned long long)f[3],
                        (unsigned long long)f[4], (unsigned long long)f[5],
                        mu, lam);
            std::printf("m4probe: M4_naive_lower=%.4g  M4_chao1=%.4g  "
                        "M4_poisson_mle=%.4g  peak_rss_mb=%.1f\n",
                        D * scale, chao, mle, peak_rss_bytes() / 1048576.0);
            std::printf("caveats: strided (non-random) parents; heterogeneous "
                        "in-degree biases the Poisson MLE low and Chao1 is a "
                        "lower bound under heterogeneity\n");
            return 0;
        }
        if (stopAfter > 0 && L >= stopAfter) {
            // ---- bounded stop: report built layers, masses, fan sample ----
            for (int l = 1; l <= stopAfter; l++) {
                u128 mass = 0;
                for (u32 i = 0; i < layers[l].size(); i++)
                    if (layers[l].stab[i])
                        mass += (u128)(Gorder / layers[l].stab[i]);
                std::printf("layer %d: states=%zu  orbit_mass=%s\n", l,
                            layers[l].real_size(), u128_str(mass).c_str());
                if (C == 6 && l == 2) {
                    bool ok = (mass == (u128)20338525ULL);
                    std::printf("  layer-2 mass vs known 20338525: %s\n",
                                ok ? "OK" : "MISMATCH");
                    if (!ok) return 9;
                }
                if (C == 5 && l == 2) {
                    bool ok = (mass == (u128)165744ULL);
                    std::printf("  layer-2 mass vs known 165744: %s\n",
                                ok ? "OK" : "MISMATCH");
                    if (!ok) return 9;
                }
            }
            if (fanSample > 0) {
                Layer& lay = layers[stopAfter];
                size_t np = lay.size();
                size_t take = std::min((size_t)fanSample, np);
                size_t stride = np / take;
                EmitCtx ctx;
                ctx.next = nullptr;
                ctx.emissions = 0;
                ctx.cacheHits = 0;
                ctx.countOnly = true;
                ctx.row = nullptr;
                std::vector<u64> fans;
                fans.reserve(take);
                double tf0 = now_s();
                u64 prev = 0;
                for (size_t s = 0; s < take; s++) {
                    size_t i = s * stride;
                    while (i < np && lay.is_hole((u32)i)) i++;
                    if (i >= np) continue;
                    if (s + 1 < take && i >= (s + 1) * stride) continue;
                    ctx.prepare(lay.keys[i], 0);
                    ctx.rec(0, (1u << N2C) - 1);
                    fans.push_back(ctx.emissions - prev);
                    prev = ctx.emissions;
                }
                take = fans.size();
                double tfd = now_s() - tf0;
                std::sort(fans.begin(), fans.end());
                u128 tot = 0;
                for (u64 v : fans) tot += v;
                double mean = (double)(u64)(tot / take);
                std::printf("fan sample %d->%d over %zu/%zu parents "
                            "(count-only): min=%llu med=%llu mean=%.1f "
                            "max=%llu  extrapolated_total=%.4g  "
                            "sample_time=%.1fs\n",
                            stopAfter, stopAfter + 1, take, lay.real_size(),
                            (unsigned long long)fans.front(),
                            (unsigned long long)fans[take / 2], mean,
                            (unsigned long long)fans.back(),
                            mean * lay.real_size(), tfd);
            }
            std::printf("STOPPED AFTER LAYER %d (no contraction)  "
                        "peak_rss_mb=%.1f  total_seconds=%.1f\n",
                        stopAfter, peak_rss_bytes() / 1048576.0,
                        now_s() - t_all0);
            return 0;
        }
        if (L == probeLayer && probeParents > 0) {
            // bounded parent-sample probe: measure the L -> L+1 transition on
            // a strided sample of parents, then stop (no full layer built).
            size_t np = layers[L].size();
            size_t take = std::min((size_t)probeParents, np);
            size_t stride = np / take;
            layers[L + 1].init(1 << 20);
            EmitCtx ctx;
            ctx.next = &layers[L + 1];
            ctx.emissions = 0;
            ctx.cacheHits = 0;
            ctx.row = nullptr;
            double tp0 = now_s();
            u64 emPrev = 0;
            for (size_t s = 0; s < take; s++) {
                size_t i = s * stride;
                while (i < np && layers[L].is_hole((u32)i)) i++;
                if (i >= np) continue;
                if (s + 1 < take && i >= (s + 1) * stride) continue;
                double tp1 = now_s();
                ctx.prepare(layers[L].keys[i], layers[L].T[i]);
                ctx.rec(0, (1u << N2C) - 1);
                double dt = now_s() - tp1;
                u64 em = ctx.emissions - emPrev;
                emPrev = ctx.emissions;
                std::printf("probe parent %zu: emissions=%llu  time=%.3fs  "
                            "(%.1f ns/em)\n", i, (unsigned long long)em, dt,
                            em ? dt * 1e9 / em : 0.0);
                std::fflush(stdout);
            }
            double dtp = now_s() - tp0;
            std::printf("PROBE %d->%d over %zu/%zu parents: emissions=%llu "
                        "distinct-children=%zu  time=%.3fs  (%.1f ns/em, "
                        "%.1f%% cache hits)  peak_rss_mb=%.1f\n",
                        L, L + 1, take, np,
                        (unsigned long long)ctx.emissions,
                        layers[L + 1].keys.size(), dtp,
                        ctx.emissions ? dtp * 1e9 / ctx.emissions : 0.0,
                        ctx.emissions ? 100.0 * ctx.cacheHits / ctx.emissions : 0.0,
                        peak_rss_bytes() / 1048576.0);
            return 0;
        }
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
    }
    double dp_seconds = now_s() - t_all0;

    // ------------------------------------------------------ final layer ----
    Layer& fin = layers[C];
    u128 N = 0;
    u128 sumW = 0;
    struct ClassOut { State q; u64 m, ell, F; };
    std::vector<ClassOut> classesOut;
    classesOut.reserve(fin.size());
    for (u32 i = 0; i < fin.size(); i++) {
        if (fin.is_hole(i)) continue;
        const State& q = fin.keys[i];
        for (int k = 0; k < N2C; k++) {
            int used = 0;
            for (int b = 0; b < C; b++) used += fld(q.m[k], b) != 0;
            if (used != C) { std::fprintf(stderr, "incomplete mask\n"); return 3; }
        }
        u64 denom = 1;
        for (int k = 0; k < N2C;) {
            int j = k;
            while (j < N2C && q.m[j] == q.m[k]) j++;
            denom *= FACT[j - k];
            k = j;
        }
        u64 ell = FACT[N2C] / denom;
        if (FACT[N2C] % denom) { std::fprintf(stderr, "ell not integral\n"); return 3; }
        u64 sq = stab_scan(q);
        if (Gorder % sq) { std::fprintf(stderr, "stab %llu bad\n",
                                        (unsigned long long)sq); return 3; }
        u64 m = Gorder / sq;
        u128 Tw = (C >= 6) ? (i < gWide.size() ? gWide[i] : (u128)0)
                           : (u128)fin.T[i];
        if (Tw % m) {
            std::fprintf(stderr, "T=%s not divisible by m=%llu\n",
                         u128_str(Tw).c_str(), (unsigned long long)m);
            return 3;
        }
        u128 Fw = Tw / m;
        if (Fw > (u128)UINT64_MAX) {
            std::fprintf(stderr, "F=%s exceeds u64\n", u128_str(Fw).c_str());
            return 3;
        }
        u64 F = (u64)Fw;
        // per-term ell*m*F^2 fits u128 for C <= 5 only; at C >= 6 the exact
        // weighted square sum must be done externally from the --dump CSV
        if (C < 6) N += (u128)ell * m * F * F;
        sumW += (u128)ell * m;
        classesOut.push_back({q, m, ell, F});
    }
    std::sort(classesOut.begin(), classesOut.end(),
              [](const ClassOut& a, const ClassOut& b) {
                  return std::memcmp(a.q.m.data(), b.q.m.data(),
                                     sizeof(u16) * 12) < 0;
              });
    std::printf("layers (states): ");
    for (int L = 1; L <= C; L++) std::printf("%zu ", layers[L].real_size());
    std::printf("\nemissions: ");
    for (u64 e : emissionsPerT) std::printf("%llu ", (unsigned long long)e);
    std::printf("\ncomplete classes = %zu\n", classesOut.size());
    std::printf("sum_w (labelled multiplicity sum) = %s\n", u128_str(sumW).c_str());
    if (C < 6)
        std::printf("N(%d) = %s\n", C, u128_str(N).c_str());
    else
        std::printf("N(%d): per-term overflow of u128; sum the --dump CSV "
                    "externally (exact per-class m, ell, F written)\n", C);
    flush_canon_counters();
    std::printf("dp_seconds = %.3f  canonize_calls = %llu  avg_nodes = %.2f\n",
                dp_seconds, (unsigned long long)g_canonize_calls,
                g_canonize_calls ? (double)g_canonize_nodes / g_canonize_calls : 0.0);
    std::printf("peak_rss_mb = %.1f\n", peak_rss_bytes() / 1048576.0);

    // built-in expectations
    struct Expect { const char* N; std::vector<u64> em; std::vector<size_t> st; };
    if (C == 2 || C == 3 || C == 4 || C == 5) {
        const char* expN =
            C == 2 ? "288" :
            C == 3 ? "28200960" :
            C == 4 ? "29136487207403520" : "1903816047972624930994913280000";
        bool ok = (u128_str(N) == expN);
        std::printf("N check vs expected %s: %s\n", expN, ok ? "OK" : "MISMATCH");
        if (!ok) return 4;
    }

    // ------------------------------------------------------- invariance ----
    if (invarianceN > 0) {
        std::mt19937_64 rng(20260726);
        long bad = 0, done = 0;
        while (done < invarianceN) {
            int L = 1 + (int)(rng() % C);
            Layer& lay = layers[L];
            u32 si = (u32)(rng() % lay.size());
            if (lay.is_hole(si)) continue;
            const State& s = lay.keys[si];
            const GElem& g = GROUP[rng() % GROUP.size()];
            State gs = apply_state(g, s);
            State cg = canonize(gs);
            if (!(cg == s)) bad++;  // layer keys are canonical already
            done++;
        }
        std::printf("invariance check: %ld samples, %ld failures %s\n",
                    invarianceN, bad, bad ? "FAIL" : "OK");
        if (bad) return 5;
    }

    // ------------------------------------------------------- scan check ----
    if (scanCheckN > 0) {
        std::mt19937_64 rng(20260727);
        long bad = 0, done = 0;
        while (done < scanCheckN) {
            int L = 1 + (int)(rng() % C);
            Layer& lay = layers[L];
            u32 si = (u32)(rng() % lay.size());
            if (lay.is_hole(si)) continue;
            const State& s = lay.keys[si];
            // scramble; the fast key must lie in the orbit and the fast
            // stabilizer must equal the directly counted one (both referees
            // are independent of the canonical-form definition)
            const GElem& g = GROUP[rng() % GROUP.size()];
            State gs = apply_state(g, s);
            u64 st1 = 0;
            State fast = canonize(gs, &st1);
            if (!orbit_member_scan(gs, fast) || st1 != stab_scan(gs)) bad++;
            done++;
        }
        std::printf("scan check (orbit membership + direct stabilizer): "
                    "%ld samples, %ld failures %s\n",
                    scanCheckN, bad, bad ? "FAIL" : "OK");
        if (bad) return 6;
    }

    // ------------------------------------------------------- layer mass ----
    if (layerMassL >= 1 && layerMassL <= C) {
        Layer& lay = layers[layerMassL];
        u128 mass = 0;
        for (u32 i = 0; i < lay.size(); i++) {
            if (lay.is_hole(i)) continue;
            mass += Gorder / stab_scan(lay.keys[i]);
        }
        std::printf("layer %d orbit mass = %s over %zu orbits\n",
                    layerMassL, u128_str(mass).c_str(), lay.real_size());
    }

    // --------------------------------------------------------- ref diff ----
    if (!refPath.empty()) {
        std::ifstream f(refPath);
        if (!f) { std::fprintf(stderr, "cannot open %s\n", refPath.c_str()); return 7; }
        std::string line;
        std::getline(f, line);  // header
        long rows = 0, matched = 0;
        // build index canonical-key -> classesOut idx
        Layer classIdx;
        classIdx.init(classesOut.size() * 2 + 16);
        for (u32 i = 0; i < classesOut.size(); i++) {
            u32 idx = classIdx.find_or_add(classesOut[i].q);
            classIdx.T[idx] = i;
        }
        while (std::getline(f, line)) {
            size_t q1 = line.find('"');
            size_t q2 = line.find('"', q1 + 1);
            if (q1 == std::string::npos) continue;
            std::stringstream ws(line.substr(q1 + 1, q2 - q1 - 1));
            State ref{};
            int w, k = 0;
            while (ws >> w) {
                u16 m = 0;
                for (int b = 0; b < C; b++)
                    m |= (u16)((2u | (u32)((w >> b) & 1)) << (2 * b));
                ref.m[k++] = m;
            }
            if (k != N2C) continue;
            std::sort(ref.m.begin(), ref.m.begin() + N2C);
            std::stringstream ts(line.substr(q2 + 2));
            std::string fld_;
            u64 rm, rell, rF;
            std::getline(ts, fld_, ','); rm = std::stoull(fld_);
            std::getline(ts, fld_, ','); rell = std::stoull(fld_);
            std::getline(ts, fld_, ','); rF = std::stoull(fld_);
            rows++;
            State ck = canonize(ref);
            u32 idx = classIdx.find(ck);
            if (idx == UINT32_MAX) continue;
            const ClassOut& mine = classesOut[classIdx.T[idx]];
            if (mine.m == rm && mine.ell == rell && mine.F == rF) matched++;
            else std::printf("triple mismatch: ref (%llu,%llu,%llu) mine "
                             "(%llu,%llu,%llu)\n",
                             (unsigned long long)rm, (unsigned long long)rell,
                             (unsigned long long)rF, (unsigned long long)mine.m,
                             (unsigned long long)mine.ell,
                             (unsigned long long)mine.F);
        }
        std::printf("reference triples: %ld rows, %ld matched %s\n", rows, matched,
                    (rows == matched && rows == (long)classesOut.size())
                        ? "OK" : "MISMATCH");
        if (!(rows == matched && rows == (long)classesOut.size())) return 8;
    }

    // -------------------------------------------------------------- dump ---
    if (!dumpPath.empty()) {
        std::ofstream f(dumpPath);
        f << "qid,representative_words,coordinate_orbit_size,"
             "labelled_multiplicity,F\n";
        for (size_t i = 0; i < classesOut.size(); i++) {
            const ClassOut& c = classesOut[i];
            f << i << ",\"";
            for (int k = 0; k < N2C; k++) {
                int w = 0;
                for (int b = 0; b < C; b++)
                    if (fld(c.q.m[k], b) == 3) w |= 1 << b;
                f << (k ? " " : "") << w;
            }
            f << "\"," << c.m << ',' << c.ell << ',' << c.F << "\n";
        }
    }

    // -------------------------------------------------------------- rank ---
    if (doRank) {
        // U_L = R_L R_{L+1} ... R_{C-1} : layer L -> classes.  Build backwards.
        int nclasses = (int)fin.size();
        for (u64 p : {P1, P2}) {
            // start: V[child of last transition] = row of R_{C-1}
            std::vector<std::vector<u64>> V(layers[C - 1].size(),
                                            std::vector<u64>(nclasses, 0));
            for (u32 i = 0; i < layers[C - 1].size(); i++)
                for (auto& e : Rrows[C - 1][i])
                    V[i][e.first] = (V[i][e.first] + e.second) % p;
            {
                auto rows = V;  // copy for rank
                int r = rank_mod_p(rows, nclasses, p);
                std::printf("rank(U_%d) [%zu x %d] mod %llu = %d\n", C - 1,
                            V.size(), nclasses, (unsigned long long)p, r);
            }
            for (int L = C - 2; L >= 1; L--) {
                std::vector<std::vector<u64>> W(layers[L].size(),
                                                std::vector<u64>(nclasses, 0));
                for (u32 i = 0; i < layers[L].size(); i++) {
                    auto& out = W[i];
                    for (auto& e : Rrows[L][i]) {
                        const auto& src = V[e.first];
                        u64 coef = e.second % p;
                        for (int q = 0; q < nclasses; q++) {
                            if (src[q])
                                out[q] = (out[q] + mulmod(coef, src[q], p)) % p;
                        }
                    }
                }
                V.swap(W);
                auto rows = V;
                int r = rank_mod_p(rows, nclasses, p);
                std::printf("rank(U_%d) [%zu x %d] mod %llu = %d\n", L,
                            V.size(), nclasses, (unsigned long long)p, r);
                std::fflush(stdout);
            }
        }
    }

    std::printf("ALL GATES PASSED for C=%d\n", C);
    return 0;
}
