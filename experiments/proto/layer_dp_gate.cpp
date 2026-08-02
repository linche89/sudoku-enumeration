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
//   --scan-check N: orbit/stabilizer referee plus a full-group brute
//                   separation and (T,stab)-histogram differential.
//   --layer-mass L: sum of orbit sizes over layer L (C=5, L=4: 62,185,328).
//   --rank: mod-p ranks of completion operators U_L = R_L...R_{C-1}
//           (target-only contraction probe).
//
// Build: g++ -O2 -std=c++17 -fopenmp -o layer_dp_gate layer_dp_gate.cpp
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
#include <queue>
#include <random>
#include <sstream>
#include <string>
#include <vector>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <psapi.h>
#include <io.h>
#else
#include <sys/statvfs.h>
#include <unistd.h>
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
// A bounded rehearsal applies a deterministic sparse parent operator from
// layer 3 onward.  Its descendants are exact for that artificial operator,
// but are NOT production C=6 memo values.  Checkpoint fingerprints and CSV
// headers keep the lineage fail-closed and visibly distinct.
static u64 g_rehearsalDenom = 0;  // 0 = production; D >= 2 keeps about 1/D
static const u64 REHEARSAL_SEED = 0x5245484541525331ULL;  // "REHEARS1"

static u64 exact_penultimate_states(int c) {
    switch (c) {
        case 2: return 1;
        case 3: return 5;
        case 4: return 54;
        case 5: return 17120;
        case 6: return 96452755;
        default: return 0;
    }
}

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

static inline void sort_masks(u16* a, int n) {
    // Fixed tiny arrays (n <= 12): explicit insertion sort avoids the
    // libstdc++ 16-element introsort lookahead warning on State::m and is the
    // same asymptotic hot-path strategy used by std::sort at this size.
    for (int i = 1; i < n; i++) {
        const u16 v = a[i];
        int j = i;
        while (j > 0 && v < a[j - 1]) {
            a[j] = a[j - 1];
            j--;
        }
        a[j] = v;
    }
}

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
    sort_masks(r.m.data(), N2C);
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
    // (C-1)-row anchor: every symbol misses exactly one box, every box is
    // missed by exactly 2.  Boxes get G-invariant colors from missing-pair
    // agreement invariants (L1 + one WL round); the search assigns boxes in
    // non-decreasing color order only.  Box permutations in any stabilizer
    // preserve colors, so the restricted assignment set is a union of full
    // cosets and minCount still equals the stabilizer order.  Flips remain
    // unrestricted (a flip-forcing anchor is NOT coset-safe for stab
    // counting; see 2026-07-27 runbook notes).
    bool anchored;
    u32 anchorCol[6];
    u8 missBoxArr[12];

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
        // (C-1)-row anchor setup
        anchored = false;
        {
            int L = 0;
            for (int b = 0; b < C; b++)
                if (fld(s.m[0], b)) L++;
            if (L == C - 1) {
                int mx[6], my[6];
                bool ok = true;
                for (int b = 0; b < C; b++) {
                    u32 t = z0[b];
                    if (__builtin_popcount(t) != 2) { ok = false; break; }
                    mx[b] = __builtin_ctz(t);
                    t &= t - 1;
                    my[b] = __builtin_ctz(t);
                }
                if (ok) {
                    for (int b = 0; b < C; b++) {
                        missBoxArr[mx[b]] = (u8)b;
                        missBoxArr[my[b]] = (u8)b;
                    }
                    auto agree = [&](int x, int y, int c) -> u32 {
                        return (((z2[c][0] >> x) & (z2[c][0] >> y)) |
                                ((z3[c][0] >> x) & (z3[c][0] >> y))) & 1u;
                    };
                    u32 a[6];
                    for (int b = 0; b < C; b++) {
                        a[b] = 0;
                        for (int c = 0; c < C; c++)
                            if (c != b) a[b] += agree(mx[b], my[b], c);
                    }
                    for (int b = 0; b < C; b++) {
                        u16 e[6];
                        int ne = 0;
                        for (int c = 0; c < C; c++)
                            if (c != b)
                                e[ne++] = (u16)((a[c] << 2) |
                                                (agree(mx[b], my[b], c) << 1) |
                                                agree(mx[c], my[c], b));
                        sort_masks(e, ne);
                        u64 h = 0x9e3779b97f4a7c15ULL ^ (u64)a[b];
                        for (int k = 0; k < ne; k++) {
                            h ^= e[k];
                            h *= 0xff51afd7ed558ccdULL;
                        }
                        anchorCol[b] = (u32)(h ^ (h >> 32));
                    }
                    anchored = true;
                }
            }
        }
        if (anchored) {
            // seed mask groups by missing-box color (G-invariant), groups
            // ordered by ascending color value
            // 16 slots avoid libstdc++'s 16-element insertion-sort lookahead
            // warning while N2C itself remains bounded by 12.
            u32 mcol[16] = {}, cs[16] = {};
            for (int i = 0; i < N2C; i++) mcol[i] = anchorCol[missBoxArr[i]];
            std::memcpy(cs, mcol, sizeof(u32) * (size_t)N2C);
            std::sort(cs, cs + N2C);
            u32 groups[12];
            int ng = 0;
            for (int k = 0; k < N2C;) {
                u32 c = cs[k];
                u32 gm = 0;
                for (int i = 0; i < N2C; i++)
                    if (mcol[i] == c) gm |= 1u << i;
                groups[ng++] = gm;
                while (k < N2C && cs[k] == c) k++;
            }
            dfs(0, groups, ng, 0);
            return;
        }
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
        u32 color[16] = {};
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
        u32 cs[16] = {};
        std::memcpy(cs, color, sizeof(u32) * (size_t)N2C);
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
        u32 minCol = UINT32_MAX;
        if (anchored) {
            for (int b = 0; b < C; b++)
                if (!((usedBoxes >> b) & 1) && anchorCol[b] < minCol)
                    minCol = anchorCol[b];
        }
        for (int b = 0; b < C; b++) {
            if ((usedBoxes >> b) & 1) continue;
            if (anchored && anchorCol[b] != minCol) continue;
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
                u32 mc2 = UINT32_MAX;
                if (anchored) {
                    for (int b2 = 0; b2 < C; b2++)
                        if (!((ub >> b2) & 1) && anchorCol[b2] < mc2)
                            mc2 = anchorCol[b2];
                }
                for (int b2 = 0; b2 < C; b2++) {
                    if ((ub >> b2) & 1) continue;
                    if (anchored && anchorCol[b2] != mc2) continue;
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

static bool state_less(const State& a, const State& b) {
    return std::lexicographical_compare(
        a.m.begin(), a.m.begin() + N2C,
        b.m.begin(), b.m.begin() + N2C);
}

// Definition-independent brute representative used only by --scan-check.
// The production canonical form is intentionally not required to equal this
// lexicographic representative; it must induce exactly the same orbit
// partition.
static State canon_scan(const State& s) {
    State best{};
    bool have = false;
    for (const GElem& g : GROUP) {
        State t = apply_state(g, s);
        if (!have || state_less(t, best)) {
            best = t;
            have = true;
        }
    }
    return best;
}

// Exact bridge from the factorization_orbit C=6 graph convention to this
// engine's complete native state.  side0[b] is the 12-bit symbol set in the
// first slot of box b; its complement occupies the second slot.
static State complete_state_from_side0(
    const std::array<u16, 6>& side0) {
    State s{};
    for (int b = 0; b < C; b++) {
        if (__builtin_popcount((u32)side0[b]) != C) {
            std::fprintf(stderr, "FATAL: malformed known-class side mask\n");
            std::abort();
        }
    }
    for (int sym = 0; sym < N2C; sym++) {
        u16 mask = 0;
        for (int b = 0; b < C; b++) {
            const int side = ((side0[b] >> sym) & 1u) ? 0 : 1;
            mask |= (u16)((2u | (u32)side) << (2 * b));
        }
        s.m[sym] = mask;
    }
    sort_masks(s.m.data(), N2C);
    return canonize(s);
}

static std::pair<State, State> known_c6_keys() {
    if (C != 6) {
        std::fprintf(stderr, "FATAL: C=6 known-class bridge used at C=%d\n", C);
        std::abort();
    }
    // Transposes of the audited factorization_orbit G1/G2 adjacency rows.
    const std::array<u16, 6> g1 = {
        0x95A, 0x4F2, 0x56C, 0x9B4, 0xE38, 0xFC0,
    };
    const std::array<u16, 6> g2 = {
        0x8EA, 0x572, 0x95C, 0xDA4, 0xE38, 0xFC0,
    };
    State k1 = complete_state_from_side0(g1);
    State k2 = complete_state_from_side0(g2);
    if (k1 == k2) {
        std::fprintf(stderr, "FATAL: C=6 G1/G2 bridge keys collapsed\n");
        std::abort();
    }
    return {k1, k2};
}

static void print_complete_words(const State& s) {
    for (int i = 0; i < N2C; i++) {
        u32 w = 0;
        for (int b = 0; b < C; b++)
            if (fld(s.m[i], b) == 3) w |= 1u << b;
        std::printf("%s%u", i ? " " : "", w);
    }
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

// Deterministic approximately-uniform sampling by canonical key rather than
// by insertion/table position.  Keeping the k smallest independently mixed
// key hashes makes the selected set insensitive to parallel insertion order
// and avoids the structural assumptions of the older strided probes.
static u64 sample_mix64(u64 x) {
    x += 0x9e3779b97f4a7c15ULL;
    x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ULL;
    x = (x ^ (x >> 27)) * 0x94d049bb133111ebULL;
    return x ^ (x >> 31);
}

static bool rehearsal_parent_selected(const State& key, int parentLayer) {
    if (!g_rehearsalDenom || parentLayer < 3) return true;
    const u64 domain =
        REHEARSAL_SEED ^ ((u64)parentLayer * 0xd6e8feb86659fd93ULL);
    return sample_mix64(state_hash(key) ^ domain) % g_rehearsalDenom == 0;
}

static std::vector<u32> layer_key_hash_sample(const Layer& lay, size_t take,
                                              u64 seed) {
    take = std::min(take, lay.real_size());
    std::priority_queue<std::pair<u64, u32>> selected;
    for (u32 i = 0; i < lay.size(); i++) {
        if (lay.is_hole(i)) continue;
        const std::pair<u64, u32> candidate = {
            sample_mix64(state_hash(lay.keys[i]) ^ seed), i
        };
        if (selected.size() < take) {
            selected.push(candidate);
        } else if (candidate < selected.top()) {
            selected.pop();
            selected.push(candidate);
        }
    }
    std::vector<u32> out;
    out.reserve(selected.size());
    while (!selected.empty()) {
        out.push_back(selected.top().second);
        selected.pop();
    }
    std::sort(out.begin(), out.end());
    return out;
}

static std::vector<u32> layer_strided_sample(const Layer& lay, size_t take) {
    const size_t np = lay.size();
    take = std::min(take, np);
    std::vector<u32> out;
    if (!take) return out;
    const size_t stride = np / take;
    out.reserve(take);
    for (size_t s = 0; s < take; s++) {
        size_t i = s * stride;
        while (i < np && lay.is_hole((u32)i)) i++;
        if (i >= np) continue;
        if (s + 1 < take && i >= (s + 1) * stride) continue;
        out.push_back((u32)i);
    }
    return out;
}

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
        sort_masks(tmp, N2C);
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

// ----------------------------------------------------- checkpoint/restart ---
// Verbatim Layer serialize/deserialize + chunked transition checkpoints
// (plan of 2026-07-27; design: docs/expert/2026-07-27/checkpoint-restart-spec.md).
// Images are VERBATIM: keys/T/stab[0..n) including holes, so entry ORDER --
// and therefore parent chunk indices -- survives a round trip exactly.  The
// hash table is never dumped; it is rebuilt on load (holes skipped).  Files
// are written tmp -> fflush -> fsync -> atomic-rename-replace, guarded by
// magic + version + streamed 64-bit checksums (header and payload).

static const u64 CK_MAGIC = 0x314B434C4A464453ULL;  // "SDFJLCK1"
static const u32 CK_VERSION = 2;
static const u64 CK_SEED = 0x5344464A434B3031ULL;
// Bump whenever State, canonical-key semantics, or transition arithmetic
// changes incompatibly.  CK_VERSION covers the byte format; this tag covers
// the mathematical meaning of a stored key/value pair.
static const u64 CK_ALGO_TAG = 0x4C445043414E3031ULL;  // "LDPCAN01"

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

struct CkptHeader {           // 128 bytes, naturally packed, little-endian
    u64 magic;
    u32 version, cVal;
    u32 layerIdx;             // layer stored in this file
    u32 parentLayer;          // transition parent L (0 = plain layer snapshot)
    u64 configHash;           // algorithm tag + key-affecting runtime modes
    u64 gen;                  // monotone generation (drives .a/.b alternation)
    u64 nEntries, holes;      // verbatim entry count INCLUDING holes
    u64 cursorChunk, nChunks, chunkParents;
    u64 emissionsSoFar, cacheHitsSoFar;
    u64 parentKeysHash;       // ck_hash64 over parent keys bytes (0 if none)
    u64 nWide;                // trailing u128 entries (wide final transition)
    u64 payloadHash;          // chained ck_hash64 over keys||T||stab||wide
    u64 headerHash;           // ck_hash64 over header with this field zeroed
};
static_assert(sizeof(CkptHeader) == 128, "CkptHeader must be padding-free");

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
static std::string g_lastCkptPath;      // newest fully written generation

static bool ck_path_exists(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    return f.good();
}

// ----------------------------------------------------- resource preflight ---
// Capacity-worst-case accounting for a staged L->L+1 process.  This includes
// the resident parent and child hash tables, final-layer wide vectors,
// per-thread emission caches, cap-reserved in-place resume loading, and the
// third on-disk image that exists while a new .tmp checkpoint is written
// beside both A/B generations.
struct ResourceFootprint {
    u64 cap = 0;
    u64 arrays = 0;
    u64 table = 0;
    u64 wide = 0;
    u64 resident = 0;
    u64 image = 0;
};

static u64 resource_table_slots(u64 cap) {
    u64 slots = 64;
    while (slots < cap * 2) slots <<= 1;
    return slots;
}

static ResourceFootprint resource_layer(int layer, u64 cap) {
    ResourceFootprint r;
    r.cap = cap;
    r.arrays = cap * ((u64)sizeof(State) + sizeof(u64) + sizeof(u32));
    r.table = resource_table_slots(cap) * sizeof(u32);
    const bool wideLayer =
        layer == C && (C >= 6 || g_forceWide);
    r.wide = wideLayer ? cap * sizeof(u128) : 0;
    r.resident = r.arrays + r.table + r.wide;
    r.image = sizeof(CkptHeader) + r.arrays + r.wide;
    return r;
}

static std::string resource_parent_dir(const std::string& path) {
    const size_t pos = path.find_last_of("/\\");
    if (pos == std::string::npos) return ".";
    if (pos == 0) return path.substr(0, 1);
    if (pos == 2 && path.size() >= 3 && path[1] == ':')
        return path.substr(0, 3);
    return path.substr(0, pos);
}

static bool resource_host_bytes(const std::string& checkpointBase,
                                u64& totalRam, u64& availRam, u64& freeDisk,
                                std::string& diskDir) {
    diskDir = resource_parent_dir(checkpointBase);
#ifdef _WIN32
    MEMORYSTATUSEX ms{};
    ms.dwLength = sizeof(ms);
    if (!GlobalMemoryStatusEx(&ms)) return false;
    totalRam = ms.ullTotalPhys;
    availRam = ms.ullAvailPhys;
    ULARGE_INTEGER callerFree{}, totalBytes{}, totalFree{};
    if (!GetDiskFreeSpaceExA(diskDir.c_str(), &callerFree,
                             &totalBytes, &totalFree))
        return false;
    freeDisk = callerFree.QuadPart;
#else
    const long pages = sysconf(_SC_PHYS_PAGES);
    const long availPages = sysconf(_SC_AVPHYS_PAGES);
    const long pageSize = sysconf(_SC_PAGESIZE);
    if (pages <= 0 || availPages < 0 || pageSize <= 0) return false;
    totalRam = (u64)pages * (u64)pageSize;
    availRam = (u64)availPages * (u64)pageSize;
    struct statvfs sv {};
    if (statvfs(diskDir.c_str(), &sv) != 0) return false;
    freeDisk = (u64)sv.f_bavail * (u64)sv.f_frsize;
#endif
    return true;
}

static double resource_gib(u64 bytes) {
    return (double)bytes / (1024.0 * 1024.0 * 1024.0);
}

static int resource_preflight(int L, int nthreads,
                              const std::vector<size_t>& caps) {
    auto capFor = [&](int layer) -> u64 {
        return layer == 1 ? 1ULL : (u64)caps[(size_t)layer - 2];
    };
    const ResourceFootprint parent = resource_layer(L, capFor(L));
    const ResourceFootprint child = resource_layer(L + 1, capFor(L + 1));

    const u64 rcPerThread =
        2ULL * (1ULL << 16) * sizeof(u32) +
        (1ULL << 15) *
            ((u64)sizeof(State) + sizeof(std::pair<u32, u64>));
    const u64 threadCaches = rcPerThread * (u64)nthreads;
    const u64 wideThreadScratch = child.wide * (u64)nthreads;
    const u64 runtimePeak =
        parent.resident + child.resident + threadCaches + wideThreadScratch;
    // ck_read_file reserves the final cap and layer_install moves those
    // buffers into the child, so the compact serialized payload is not a
    // second resident copy.  Table construction and arrays coexist here.
    const u64 resumeLoadPeak = parent.resident + child.resident;
    const u64 ramPeak = std::max(runtimePeak, resumeLoadPeak);
    const u64 eightGiB = 8ULL << 30;
    const u64 ramMargin = std::max(eightGiB, ramPeak / 10);
    const u64 ramRequired = ramPeak + ramMargin;

    // Retained-chain upper bound: every completed earlier stage keeps two
    // physical A/B images (its snapshot hard-links one), while the active
    // stage temporarily has A + B + tmp.
    u64 retainedDisk = 0;
    for (int layer = 2; layer <= L; layer++)
        retainedDisk += 2 * resource_layer(layer, capFor(layer)).image;
    const u64 diskPeak = retainedDisk + 3 * child.image;
    const u64 sixteenGiB = 16ULL << 30;
    const u64 diskMargin = std::max(sixteenGiB, diskPeak / 10);
    const u64 diskRequired = diskPeak + diskMargin;

    u64 totalRam = 0, availRam = 0, freeDisk = 0;
    std::string diskDir;
    if (!resource_host_bytes(g_ckptBase, totalRam, availRam, freeDisk,
                             diskDir)) {
        std::fprintf(stderr,
                     "resource preflight cannot query RAM or checkpoint "
                     "volume for %s\n", g_ckptBase.c_str());
        return 2;
    }
    const bool totalOk = totalRam >= ramRequired;
    const bool availOk = availRam >= ramRequired;
    const bool diskOk = freeDisk >= diskRequired;
    std::printf("RESOURCE PREFLIGHT C=%d transition %d->%d "
                "(capacity-worst-case)\n", C, L, L + 1);
    std::printf("  parent layer %d: cap=%llu resident=%.3f GiB "
                "(arrays %.3f, table %.3f)\n",
                L, (unsigned long long)parent.cap,
                resource_gib(parent.resident),
                resource_gib(parent.arrays), resource_gib(parent.table));
    std::printf("  child  layer %d: cap=%llu resident=%.3f GiB "
                "(arrays %.3f, table %.3f, wide %.3f)\n",
                L + 1, (unsigned long long)child.cap,
                resource_gib(child.resident),
                resource_gib(child.arrays), resource_gib(child.table),
                resource_gib(child.wide));
    std::printf("  runtime peak: %.3f GiB (thread caches %.3f, "
                "wide scratch %.3f)\n",
                resource_gib(runtimePeak), resource_gib(threadCaches),
                resource_gib(wideThreadScratch));
    std::printf("  resume-load peak: %.3f GiB "
                "(cap-reserved image moved in place)\n",
                resource_gib(resumeLoadPeak));
    std::printf("  RAM required with margin: %.3f GiB "
                "(peak %.3f + margin %.3f)\n",
                resource_gib(ramRequired), resource_gib(ramPeak),
                resource_gib(ramMargin));
    std::printf("  host RAM: total %.3f GiB [%s], available %.3f GiB [%s]\n",
                resource_gib(totalRam), totalOk ? "OK" : "FAIL",
                resource_gib(availRam), availOk ? "OK" : "FAIL");
    std::printf("  retained-chain disk peak: %.3f GiB "
                "(includes active A+B+tmp)\n", resource_gib(diskPeak));
    std::printf("  disk required with margin: %.3f GiB; "
                "available %.3f GiB at %s [%s]\n",
                resource_gib(diskRequired), resource_gib(freeDisk),
                diskDir.c_str(), diskOk ? "OK" : "FAIL");
    const bool ok = totalOk && availOk && diskOk;
    std::printf("RESOURCE PREFLIGHT %s\n", ok ? "PASS" : "FAIL");
    return ok ? 0 : 13;
}

static u64 ck_config_hash(u32 layerIdx) {
    // Layers 1..3 remain the complete exact production seed and can be
    // imported read-only into a rehearsal.  Every descendant from layer 4
    // onward is bound to the rehearsal denominator and cannot be opened by a
    // production invocation (or by a rehearsal with a different fraction).
    const bool sparseLineage = g_rehearsalDenom && layerIdx >= 4;
    const u64 baseWords[] = {
        CK_ALGO_TAG,
        (u64)C,
        (u64)N2C,
        (u64)sizeof(State),
        g_wlseed ? 1ULL : 0ULL,
        g_forceWide ? 1ULL : 0ULL,
    };
    const u64 base = ck_hash64(baseWords, sizeof(baseWords), CK_SEED);
    if (!sparseLineage) return base;  // byte-compatible with production v2
    const u64 lineageWords[] = {REHEARSAL_SEED, g_rehearsalDenom};
    return ck_hash64(lineageWords, sizeof(lineageWords), base);
}

static bool ck_header_sane(const CkptHeader& h, u64* expectedBytes = nullptr) {
    if (h.magic != CK_MAGIC || h.version != CK_VERSION ||
        h.cVal != (u32)C || h.configHash != ck_config_hash(h.layerIdx))
        return false;
    if (h.layerIdx < 1 || h.layerIdx > (u32)C ||
        h.parentLayer >= (u32)C)
        return false;
    if (h.parentLayer && h.layerIdx != h.parentLayer + 1)
        return false;
    if (h.nEntries >= (u64)UINT32_MAX - 64 || h.holes > h.nEntries ||
        h.nWide > h.nEntries)
        return false;
    if (h.parentLayer) {
        if (!h.chunkParents || !h.nChunks || h.cursorChunk > h.nChunks)
            return false;
    } else if (h.cursorChunk || h.nChunks || h.chunkParents ||
               h.parentKeysHash) {
        return false;
    }
    const u64 perEntry =
        (u64)sizeof(State) + (u64)sizeof(u64) + (u64)sizeof(u32);
    const u64 bytes = (u64)sizeof(CkptHeader) +
                      h.nEntries * perEntry + h.nWide * (u64)sizeof(u128);
    if (expectedBytes) *expectedBytes = bytes;
    return true;
}

// NOTE (2026-07-27 apply): the checkpoint file functions are noinline AND
// compiled without AVX.  GCC 13.2 mingw at -O2 -march=native (znver4)
// expands 120-byte CkptHeader block copies/zeroing with ALIGNED 32/64-byte
// vmovdqa(64) stores into stack slots, but mingw-SEH frames are never
// dynamically realigned beyond the ABI's 16 bytes (GCC PR99234 class), and
// Windows randomizes the initial rsp, so the stores fault intermittently
// (SIGSEGV; confirmed by disassembly at three distinct sites: inlined into
// main, standalone ckpt_write_pair zmm, and layer_save_file ymm; the -O0
// build is correct, so this is a compiler codegen bug, not a logic bug).
// no-avx forces struct copies down to 16-byte ops, the only width whose
// stack alignment mingw-SEH actually guarantees.  These functions are
// I/O-bound; the ISA restriction costs nothing.
#if defined(__GNUC__) && defined(__x86_64__)
#define CK_NOINLINE __attribute__((noinline, target("no-avx")))
#elif defined(__GNUC__)
#define CK_NOINLINE __attribute__((noinline))
#else
#define CK_NOINLINE
#endif

#ifdef _WIN32
CK_NOINLINE
static bool ck_move_replace(const std::string& from,
                            const std::string& to,
                            const char* what) {
    DWORD last = ERROR_SUCCESS;
    for (int attempt = 0; attempt <= 50; attempt++) {
        if (MoveFileExA(from.c_str(), to.c_str(),
                        MOVEFILE_REPLACE_EXISTING |
                        MOVEFILE_WRITE_THROUGH))
            return true;
        last = GetLastError();
        const bool transient =
            last == ERROR_ACCESS_DENIED ||
            last == ERROR_SHARING_VIOLATION ||
            last == ERROR_LOCK_VIOLATION ||
            last == ERROR_BUSY ||
            last == ERROR_USER_MAPPED_FILE;
        if (!transient || attempt == 50) break;
        if (attempt == 0)
            std::fprintf(stderr,
                         "%s replace temporarily blocked (winerr=%lu); "
                         "retrying for up to 5 seconds\n",
                         what, (unsigned long)last);
        if (attempt == 0) std::fflush(stderr);
        Sleep(100);
    }
    char msg[256] = {};
    FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM |
                       FORMAT_MESSAGE_IGNORE_INSERTS,
                   nullptr, last, 0, msg, sizeof(msg), nullptr);
    std::fprintf(stderr,
                 "%s replace failed: %s -> %s (winerr=%lu %s)\n",
                 what, from.c_str(), to.c_str(),
                 (unsigned long)last, msg);
    return false;
}
#endif

CK_NOINLINE
static bool ck_write_file(const std::string& path, CkptHeader h,
                          const State* keys, const u64* T, const u32* stab,
                          const u128* wide) {
    h.magic = CK_MAGIC;
    h.version = CK_VERSION;
    h.cVal = (u32)C;
    h.configHash = ck_config_hash(h.layerIdx);
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
    if (!ck_move_replace(tmp, path, "checkpoint")) {
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

CK_NOINLINE
static bool ck_read_header(const std::string& path, CkptHeader& h) {
    FILE* f = std::fopen(path.c_str(), "rb");
    if (!f) return false;
    bool ok = std::fread(&h, sizeof(h), 1, f) == 1;
    std::fclose(f);
    if (!ok) return false;
    u64 want = h.headerHash;
    h.headerHash = 0;
    if (ck_hash64(&h, sizeof(h), CK_SEED) != want)
        return false;
    h.headerHash = want;
    return ck_header_sane(h);
}

CK_NOINLINE
static bool ck_read_file(const std::string& path, CkptImage& im,
                         size_t reserveEntries = 0) {
    CkptHeader h;
    if (!ck_read_header(path, h)) return false;
    FILE* f = std::fopen(path.c_str(), "rb");
    if (!f) return false;
    u64 expectedBytes = 0;
    if (!ck_header_sane(h, &expectedBytes)) {
        std::fclose(f);
        return false;
    }
#ifdef _WIN32
    bool ok = _fseeki64(f, 0, SEEK_END) == 0;
    const __int64 endPos = ok ? _ftelli64(f) : -1;
    ok = ok && endPos >= 0 && (u64)endPos == expectedBytes &&
         _fseeki64(f, (__int64)sizeof(CkptHeader), SEEK_SET) == 0;
#else
    bool ok = fseeko(f, 0, SEEK_END) == 0;
    const off_t endPos = ok ? ftello(f) : (off_t)-1;
    ok = ok && endPos >= 0 && (u64)endPos == expectedBytes &&
         fseeko(f, (off_t)sizeof(CkptHeader), SEEK_SET) == 0;
#endif
    if (!ok) {
        std::fclose(f);
        return false;
    }
    const size_t n = (size_t)h.nEntries;
    const size_t reserveN = std::max(n, reserveEntries);
    im.keys.clear();
    im.T.clear();
    im.stab.clear();
    im.wide.clear();
    if (im.keys.capacity() < reserveN) im.keys.reserve(reserveN);
    if (im.T.capacity() < reserveN) im.T.reserve(reserveN);
    if (im.stab.capacity() < reserveN) im.stab.reserve(reserveN);
    if (h.nWide && im.wide.capacity() < reserveN)
        im.wide.reserve(reserveN);
    im.keys.resize(n);
    im.T.resize(n);
    im.stab.resize(n);
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
    im.h = h;
    return true;
}

// install a verbatim image into a Layer, preserving entry order exactly.
// fixedCap != 0: fixed mode with that capacity (all loaded PARENTS use
// fixedCap == nEntries; a resumed threaded CHILD uses caps[L-1]).
// fixedCap == 0: dynamic single-thread child (requires holes == 0, since
// is_hole() only recognizes holes in fixed mode).
CK_NOINLINE
static bool layer_install(Layer& lay, CkptImage& im, size_t fixedCap) {
    size_t n = im.keys.size();
    if (fixedCap) {
        if (n > fixedCap) {
            std::fprintf(stderr, "FATAL: layer image (%zu entries) exceeds "
                                 "cap %zu\n", n, fixedCap);
            return false;
        }
        const bool movable =
            im.keys.capacity() >= fixedCap &&
            im.T.capacity() >= fixedCap &&
            im.stab.capacity() >= fixedCap;
        if (movable) {
            // Finalized parents use fixedCap == n.  Resumed children are read
            // with fixedCap reserved up front.  Both can therefore transfer
            // ownership and grow logically to cap without retaining a second
            // 36-byte-per-entry image during table construction.
            lay.keys = std::move(im.keys);
            lay.T = std::move(im.T);
            lay.stab = std::move(im.stab);
            lay.keys.resize(fixedCap);
            lay.T.resize(fixedCap, 0);
            lay.stab.resize(fixedCap, 0);
            size_t sz = 64;
            while (sz < fixedCap * 2) sz <<= 1;
            lay.table.assign(sz, 0);
            lay.mask = sz - 1;
            lay.fixedCap = true;
            lay.claimed = lay.holes = 0;
        } else {
            lay.init_fixed(fixedCap);
            std::copy(im.keys.begin(), im.keys.end(), lay.keys.begin());
            std::copy(im.T.begin(), im.T.end(), lay.T.begin());
            std::copy(im.stab.begin(), im.stab.end(), lay.stab.begin());
        }
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

CK_NOINLINE
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

// A finalized transition checkpoint already contains a complete, durable
// layer image.  Preserve it as the next transition's immutable parent
// snapshot with an atomic hard link instead of writing the 30+ GiB payload a
// second time.  Replacing base.a/base.b later does not change the linked inode.
CK_NOINLINE
static bool ck_snapshot_link(const std::string& source,
                             const std::string& snapshot) {
    const std::string tmp = snapshot + ".tmp.link";
    std::remove(tmp.c_str());
#ifdef _WIN32
    if (!CreateHardLinkA(tmp.c_str(), source.c_str(), nullptr))
        return false;
    if (!ck_move_replace(tmp, snapshot, "snapshot-link")) {
        std::remove(tmp.c_str());
        return false;
    }
#else
    if (link(source.c_str(), tmp.c_str()) != 0)
        return false;
    if (std::rename(tmp.c_str(), snapshot.c_str()) != 0) {
        std::remove(tmp.c_str());
        return false;
    }
#endif
    std::printf("linked finalized checkpoint %s -> %s\n",
                source.c_str(), snapshot.c_str());
    return true;
}

CK_NOINLINE
static bool ckpt_write_pair(int L, const Layer& parent, const Layer& child,
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
    bool ok = ck_write_file(path, h, child.keys.data(), child.T.data(),
                            child.stab.data(), nw ? wide->data() : nullptr);
    if (!ok)
        std::fprintf(stderr, "WARNING: checkpoint write failed (%s); "
                             "stopping with the previous generation intact\n",
                     path.c_str());
    else
        std::printf("checkpoint gen=%llu trans=%d->%d chunk=%llu/%llu "
                    "entries=%llu -> %s (%.2fs)\n", (unsigned long long)h.gen,
                    L, L + 1, (unsigned long long)cursorChunk,
                    (unsigned long long)nChunks,
                    (unsigned long long)h.nEntries, path.c_str(),
                     now_s() - td0);
    if (ok) g_lastCkptPath = path;
    std::fflush(stdout);
    return ok;
}

CK_NOINLINE
static bool ckpt_load_newest(const std::string& base, CkptImage& im,
                             const std::vector<size_t>* capsHint = nullptr) {
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
    auto readCandidate = [&](const std::string& path,
                             const CkptHeader& h) -> bool {
        size_t reserveEntries = 0;
        if (capsHint && h.layerIdx >= 2 &&
            (size_t)h.layerIdx - 2 < capsHint->size()) {
            const size_t cap = (*capsHint)[(size_t)h.layerIdx - 2];
            if (cap >= (size_t)h.nEntries) reserveEntries = cap;
        }
        return ck_read_file(path, im, reserveEntries);
    };
    const CkptHeader* firstHdr =
        first == &pa ? &ha : first == &pb ? &hb : nullptr;
    const CkptHeader* secondHdr =
        second == &pa ? &ha : second == &pb ? &hb : nullptr;
    if (first && firstHdr && readCandidate(*first, *firstHdr)) {
        g_lastCkptPath = *first;
        return true;   // newest valid gen
    }
    if (second && secondHdr && readCandidate(*second, *secondHdr)) {
        g_lastCkptPath = *second;
        return true;   // torn-dump fallback
    }
    return false;
}

CK_NOINLINE
static bool ckpt_peek_stage(const std::string& base, int& parentLayer) {
    CkptHeader ha, hb;
    const bool va = ck_read_header(base + ".a", ha);
    const bool vb = ck_read_header(base + ".b", hb);
    if (!va && !vb) return false;
    if (va && vb && ha.parentLayer != hb.parentLayer) {
        std::fprintf(stderr,
                     "checkpoint generations span different transitions; "
                     "staged resume requires a fresh base per transition\n");
        return false;
    }
    const CkptHeader& h =
        !vb || (va && ha.gen >= hb.gen) ? ha : hb;
    parentLayer = (int)h.parentLayer;
    return parentLayer >= 1 && parentLayer < C;
}

CK_NOINLINE
static bool ckpt_install_child(Layer& child, std::vector<u128>& wideOut,
                               CkptImage& im, size_t fixedCap) {
    if (fixedCap &&
        (im.keys.capacity() < fixedCap ||
         im.T.capacity() < fixedCap ||
         im.stab.capacity() < fixedCap ||
         (!im.wide.empty() && im.wide.capacity() < fixedCap))) {
        std::fprintf(stderr,
                     "FATAL: resumed child was not cap-reserved; refusing "
                     "a duplicate-payload install\n");
        return false;
    }
    if (!im.wide.empty()) {
        wideOut = std::move(im.wide);
        if (fixedCap) wideOut.resize(fixedCap, 0);
    } else {
        wideOut.clear();
    }
    if (!layer_install(child, im, fixedCap)) return false;
    return true;
}

// release a CkptImage without materializing a CkptImage temporary in the
// caller's frame (see the CK_NOINLINE compiler-bug note above).
CK_NOINLINE
static void ckpt_release(CkptImage& im) {
    CkptImage tmp;
    std::swap(tmp.h, im.h);
    im.keys = std::vector<State>();
    im.T = std::vector<u64>();
    im.stab = std::vector<u32>();
    im.wide = std::vector<u128>();
}

// ------------------------------------------------------------------ main ---
int main(int argc, char** argv) {
    if (argc < 2) {
        std::fprintf(stderr, "usage: %s C [--ref f] [--rank] [--invariance N] "
                             "[--scan-check N] [--layer-mass L] [--dump f]\n"
                             "  ckpt: [--save-layer L file] [--load-layer L "
                             "file] [--checkpoint base period-min]\n"
                             "        [--ckpt-chunk P] [--resume base] "
                             "[--force-wide]\n"
                             "  resources: [--resource-preflight L]\n"
                             "  probes: [--m4probe K W CAP "
                             "[--m4-parent-seed S] [--fan-sample N] "
                             "[--fan-canon-sample N CAP]]\n"
                             "  C6 safety: [--bridge-only] [--ack-full-c6]\n"
                             "  dress rehearsal: [--rehearsal-denom D]\n",
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
    bool m4ParentSeedSet = false;
    u64 m4ParentSeed = 0;      // key-hash parent sample instead of table stride
    long fanCanonSample = 0;   // real canonicalized 4->5 calibration parents
    size_t fanCanonCap = 0;    // bounded layer-5 table for that calibration
    bool bridgeOnly = false;
    bool ackFullC6 = false;
    int resourcePreflightL = -1;
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
        else if (s == "--m4-parent-seed" && a + 1 < argc) {
            m4ParentSeedSet = true;
            m4ParentSeed = std::stoull(argv[++a]);
        }
        else if (s == "--fan-canon-sample" && a + 2 < argc) {
            fanCanonSample = std::atol(argv[++a]);
            fanCanonCap = std::stoull(argv[++a]);
        }
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
        else if (s == "--rehearsal-denom" && a + 1 < argc)
            g_rehearsalDenom = std::stoull(argv[++a]);
        else if (s == "--bridge-only") bridgeOnly = true;
        else if (s == "--ack-full-c6") ackFullC6 = true;
        else if (s == "--resource-preflight" && a + 1 < argc)
            resourcePreflightL = std::atoi(argv[++a]);
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
    if (nthreads < 1) {
        std::fprintf(stderr, "--threads must be positive\n");
        return 2;
    }
    if (g_rehearsalDenom == 1) {
        std::fprintf(stderr, "--rehearsal-denom must be 0/off or >= 2\n");
        return 2;
    }
    const bool rehearsalMode = g_rehearsalDenom >= 2;
    if (rehearsalMode && C < 5) {
        std::fprintf(stderr,
                     "--rehearsal-denom is supported at C=5 (gate) or C=6\n");
        return 2;
    }
    if (m4K < 0 || m4W < 0 || m4W >= 64 ||
        (m4K > 0 && (!m4W || !m4Cap))) {
        std::fprintf(stderr,
                     "--m4probe requires positive K/W/CAP with W < 64\n");
        return 2;
    }
    if ((m4ParentSeedSet || fanCanonSample || fanCanonCap) && !m4K) {
        std::fprintf(stderr,
                     "--m4-parent-seed/--fan-canon-sample require "
                     "--m4probe\n");
        return 2;
    }
    if (fanCanonSample < 0 ||
        ((fanCanonSample > 0) != (fanCanonCap > 0))) {
        std::fprintf(stderr,
                     "--fan-canon-sample requires positive N and CAP\n");
        return 2;
    }
    if (nthreads > 1 && caps.size() < (size_t)(C - 1)) {
        std::fprintf(stderr, "--threads needs --caps c2,...,c%d (child layer "
                             "capacities)\n", C);
        return 2;
    }
    for (size_t i = 0; i < caps.size(); i++) {
        if (!caps[i] || caps[i] >= (size_t)UINT32_MAX - 64) {
            std::fprintf(stderr, "--caps entry %zu is outside 1..2^32-65\n",
                         i + 1);
            return 2;
        }
    }
    if (resourcePreflightL >= 0) {
        if (resourcePreflightL < 1 || resourcePreflightL >= C) {
            std::fprintf(stderr,
                         "--resource-preflight layer must be in 1..%d\n",
                         C - 1);
            return 2;
        }
        if (nthreads <= 1 || caps.size() < (size_t)(C - 1) ||
            g_ckptBase.empty()) {
            std::fprintf(stderr,
                         "--resource-preflight requires --threads > 1, "
                         "complete --caps, and --checkpoint on the target "
                         "volume\n");
            return 2;
        }
        if (!g_resumeBase.empty() || g_loadLayerIdx || !g_saveLayers.empty() ||
            doRank || !refPath.empty() || !dumpPath.empty() ||
            invarianceN || scanCheckN || layerMassL || probeLayer ||
            m4K || stopAfter || fanSample || bridgeOnly) {
            std::fprintf(stderr,
                         "--resource-preflight is a read-only dry run; "
                         "remove execution, load/resume, and verification "
                         "modes\n");
            return 2;
        }
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
    if (g_loadLayerIdx && !g_resumeBase.empty()) {
        std::fprintf(stderr, "--load-layer and --resume are mutually exclusive\n");
        return 2;
    }
    if (!g_resumeBase.empty() && !g_ckptBase.empty() &&
        g_resumeBase != g_ckptBase) {
        std::fprintf(stderr, "--resume and --checkpoint must name the same "
                             "base path\n");
        return 2;
    }
    if (g_loadLayerIdx && (g_loadLayerIdx < 2 || g_loadLayerIdx > C)) {
        std::fprintf(stderr, "--load-layer index must be in 2..%d\n", C);
        return 2;
    }
    for (const auto& sv : g_saveLayers) {
        if (sv.first < 2 || sv.first > C) {
            std::fprintf(stderr, "--save-layer index must be in 2..%d\n", C);
            return 2;
        }
    }
    if (g_loadLayerIdx >= C && C >= 6) {
        std::fprintf(stderr, "--load-layer %d at C>=6 cannot restore the "
                             "final wide sums standalone; load layer %d and "
                             "re-run the last transition\n", C, C - 1);
        return 2;
    }
    if (!g_resumeBase.empty())
        g_ckptBase = g_resumeBase;   // resumed runs keep checkpointing on
    if (resourcePreflightL < 0 &&
        g_resumeBase.empty() && !g_ckptBase.empty()) {
        bool stale = ck_path_exists(g_ckptBase + ".a") ||
                     ck_path_exists(g_ckptBase + ".b");
        for (int L = 2; L <= C && !stale; L++)
            stale = ck_path_exists(
                g_ckptBase + ".L" + std::to_string(L) + ".snap");
        if (stale) {
            std::fprintf(stderr,
                         "checkpoint base already has files; choose a fresh "
                         "base or use --resume (existing files are never "
                         "overwritten by a new run)\n");
            return 2;
        }
    }
    if (bridgeOnly && C != 6) {
        std::fprintf(stderr, "--bridge-only requires C=6\n");
        return 2;
    }
    if (ackFullC6 && C != 6) {
        std::fprintf(stderr, "--ack-full-c6 is meaningful only at C=6\n");
        return 2;
    }
    if (rehearsalMode) {
        if (ackFullC6 || bridgeOnly || resourcePreflightL >= 0 || m4K ||
            probeLayer || doRank || !refPath.empty() || invarianceN ||
            scanCheckN || layerMassL || fanSample) {
            std::fprintf(stderr,
                         "--rehearsal-denom is an isolated staged mode; "
                         "remove production authorization, probes, refs, "
                         "rank, and whole-chain verification flags\n");
            return 2;
        }
        if (g_ckptBase.empty() || nthreads <= 1 ||
            caps.size() < (size_t)(C - 1) ||
            (g_loadLayerIdx == 0 && g_resumeBase.empty())) {
            std::fprintf(stderr,
                         "bounded rehearsal requires --checkpoint, --threads "
                         "> 1, complete --caps, and a loaded/resumed layer\n");
            return 2;
        }
        int stageL = g_loadLayerIdx;
        if (!g_resumeBase.empty() &&
            !ckpt_peek_stage(g_resumeBase, stageL)) {
            std::fprintf(stderr,
                         "cannot determine rehearsal stage from checkpoint "
                         "headers\n");
            return 12;
        }
        if (stageL < 3 || stageL >= C) {
            std::fprintf(stderr,
                         "bounded rehearsal must start from layer 3..%d\n",
                         C - 1);
            return 2;
        }
        const int requiredStop = stageL < C - 1 ? stageL + 1 : 0;
        if (stopAfter != requiredStop) {
            std::fprintf(stderr,
                         "rehearsal transition %d->%d requires %s; one "
                         "transition per process\n",
                         stageL, stageL + 1,
                         requiredStop
                             ? ("--stop-after " +
                                std::to_string(requiredStop)).c_str()
                             : "no --stop-after (final stage)");
            return 2;
        }
        if (!requiredStop && dumpPath.empty()) {
            std::fprintf(stderr,
                         "final rehearsal stage requires --dump for external "
                         "checksum\n");
            return 2;
        }
        if (C == 6) {
            const int resourceRc = resource_preflight(stageL, nthreads, caps);
            if (resourceRc != 0) return resourceRc;
        }
        std::printf("BOUNDED REHEARSAL lineage: keep canonical-key hash "
                    "1/%llu from each parent layer >= 3; outputs are NOT "
                    "production memo values\n",
                    (unsigned long long)g_rehearsalDenom);
    }
    const bool boundedC6 =
        rehearsalMode || bridgeOnly || resourcePreflightL >= 0 || m4K > 0 ||
        (stopAfter > 0 && stopAfter <= 3) ||
        (probeLayer > 0 && probeParents > 0 &&
         (probeLayer <= 3 || g_loadLayerIdx == probeLayer));
    const bool reachesC6Final =
        C == 6 && resourcePreflightL < 0 &&
        !bridgeOnly && !m4K && !probeLayer && !stopAfter;
    const bool buildsLargeC6Layer =
        C == 6 &&
        (reachesC6Final || stopAfter >= 4 ||
         (probeLayer >= 4 && g_loadLayerIdx != probeLayer));
    if (C == 6 && !boundedC6 && !ackFullC6) {
        std::fprintf(stderr,
                     "unbounded C=6 transition refused; use a positive "
                     "--probe/--m4probe, --stop-after <= 3, --bridge-only, "
                     "or explicit owner-approved --ack-full-c6\n");
        return 2;
    }
    if (buildsLargeC6Layer && ackFullC6) {
        if (g_ckptBase.empty() || nthreads <= 1 ||
            caps.size() < (size_t)(C - 1)) {
            std::fprintf(stderr,
                         "large C=6 stages require --checkpoint plus threaded "
                         "fixed --caps\n");
            return 2;
        }
        int stageL = g_loadLayerIdx;
        if (!g_resumeBase.empty() &&
            !ckpt_peek_stage(g_resumeBase, stageL)) {
            std::fprintf(stderr,
                         "cannot determine the staged transition from the "
                         "resume checkpoint headers\n");
            return 12;
        }
        if (stageL < 3 || stageL >= C) {
            std::fprintf(stderr,
                         "large C=6 work must start from a loaded/resumed "
                         "layer 3, 4, or 5 snapshot; never run it as a "
                         "monolith\n");
            return 2;
        }
        const int requiredStop = stageL < C - 1 ? stageL + 1 : 0;
        if (stopAfter != requiredStop) {
            std::fprintf(stderr,
                         "staged C=6 transition %d->%d requires %s; "
                         "one large transition per process\n",
                         stageL, stageL + 1,
                         requiredStop
                             ? ("--stop-after " +
                                std::to_string(requiredStop)).c_str()
                             : "no --stop-after (final stage)");
            return 2;
        }
        if (reachesC6Final && dumpPath.empty()) {
            std::fprintf(stderr,
                         "a full C=6 chain requires --dump for external exact "
                         "summation\n");
            return 2;
        }
        const int resourceRc = resource_preflight(stageL, nthreads, caps);
        if (resourceRc != 0) return resourceRc;
    }
    if (resourcePreflightL >= 0)
        return resource_preflight(resourcePreflightL, nthreads, caps);

    build_group();
    const u64 Gorder = GROUP.size();

    if (bridgeOnly) {
        auto kk = known_c6_keys();
        flush_canon_counters();
        std::printf("C=6 G1 canonical representative words: ");
        print_complete_words(kk.first);
        std::printf("  stab=%llu  expected_F=6986348258918400\n",
                    (unsigned long long)stab_scan(kk.first));
        std::printf("C=6 G2 canonical representative words: ");
        print_complete_words(kk.second);
        std::printf("  stab=%llu  expected_F=7053808087203840\n",
                    (unsigned long long)stab_scan(kk.second));
        std::printf("C=6 known-class bridge: distinct and well-formed OK\n");
        return 0;
    }

    // layer 1: unique state of singletons
    State x1{};
    for (int i = 0; i < N2C; i++) {
        int b = i >> 1, s = i & 1;
        x1.m[i] = (u16)((2u | (u32)s) << (2 * b));
    }
    sort_masks(x1.m.data(), N2C);
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

    int startL = 1;
    if (!g_resumeBase.empty()) {
        // ---- resume: newest valid checkpoint of the interrupted transition
        const std::vector<size_t>* capsHint =
            nthreads > 1 ? &caps : nullptr;
        if (!ckpt_load_newest(g_resumeBase, g_resumeCk, capsHint)) {
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
            // heap-backed image: keep the checkpoint header out of main's
            // stack frame (GCC znver4 codegen bug, see CK_NOINLINE note)
            std::vector<CkptImage> pimBox(1);
            CkptImage& pim = pimBox[0];
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
        // heap-backed image: keep the checkpoint header out of main's stack
        // frame (GCC znver4 codegen bug, see CK_NOINLINE note)
        std::vector<CkptImage> imBox(1);
        CkptImage& im = imBox[0];
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
        // A transition checkpoint names its immutable parent by
        // <base>.L<P>.snap.  When a staged run starts from an independently
        // named snapshot, make the checkpoint set self-contained now rather
        // than discovering the missing parent only after a crash.
        if (!g_ckptBase.empty()) {
            std::string psnap =
                g_ckptBase + ".L" + std::to_string(g_loadLayerIdx) + ".snap";
            if (psnap != g_loadLayerPath &&
                !ck_snapshot_link(g_loadLayerPath, psnap) &&
                !layer_save_file(psnap, layers[g_loadLayerIdx],
                                 g_loadLayerIdx,
                                 gWide.empty() ? nullptr : &gWide)) {
                return 12;
            }
        }
    }
    auto checkPenultimateLayer = [&](int layer) {
        if (layer != C - 1) return true;
        if (rehearsalMode) {
            std::printf("REHEARSAL: penultimate layer %d has %zu sparse-"
                        "lineage states; production M_%d anchor intentionally "
                        "not applied\n",
                        layer, layers[layer].real_size(), layer);
            return true;
        }
        const u64 expected = exact_penultimate_states(C);
        const size_t observed = layers[layer].real_size();
        const bool ok = observed == expected;
        std::printf("penultimate layer %d states=%zu expected=%llu [%s]\n",
                    layer, observed, (unsigned long long)expected,
                    ok ? "OK" : "MISMATCH");
        if (!ok)
            std::fprintf(stderr,
                         "FATAL: exact penultimate-layer state-count anchor "
                         "failed\n");
        return ok;
    };
    if (!checkPenultimateLayer(startL)) return 9;
    double t_all0 = now_s();
    for (int L = startL; L < C; L++) {
        if (m4K > 0 && L >= 3) {
            // ---- hash-window M_4 probe: sample layer-3 parents, keep only
            // ---- children whose canonical hash lands in a 2^-m4W window,
            // ---- estimate M_4 from the windowed hit histogram.
            Layer& L3 = layers[3];
            std::vector<u32> parentSample =
                m4ParentSeedSet
                    ? layer_key_hash_sample(L3, (size_t)m4K, m4ParentSeed)
                    : layer_strided_sample(L3, (size_t)m4K);
            size_t take = parentSample.size();
            if (!take) {
                std::fprintf(stderr, "m4probe selected no layer-3 parents\n");
                return 2;
            }
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
                    size_t i = parentSample[(size_t)s];
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
                        take, L3.real_size(), m4W, (unsigned long long)totEm,
                        (unsigned long long)H, totEm / scale, dt);
            std::printf("m4probe: parent_sample=%s",
                        m4ParentSeedSet ? "canonical-key-hash" : "strided");
            if (m4ParentSeedSet)
                std::printf(" seed=%llu",
                            (unsigned long long)m4ParentSeed);
            std::printf("\n");
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
            if (fanSample > 0) {
                const u64 fanSeed =
                    (m4ParentSeedSet ? m4ParentSeed : 20260731ULL) ^
                    0xd1b54a32d192ed03ULL;
                std::vector<u32> fanParents =
                    layer_key_hash_sample(win, (size_t)fanSample, fanSeed);
                EmitCtx fctx;
                fctx.next = nullptr;
                fctx.emissions = 0;
                fctx.cacheHits = 0;
                fctx.countOnly = true;
                fctx.row = nullptr;
                std::vector<u64> fans;
                fans.reserve(fanParents.size());
                double tf0 = now_s();
                u64 prev = 0;
                long double sum = 0, sumSq = 0;
                for (u32 i : fanParents) {
                    fctx.prepare(win.keys[i], 0);
                    fctx.rec(0, (1u << N2C) - 1);
                    u64 fan = fctx.emissions - prev;
                    prev = fctx.emissions;
                    fans.push_back(fan);
                    sum += fan;
                    sumSq += (long double)fan * fan;
                }
                std::sort(fans.begin(), fans.end());
                const size_t nf = fans.size();
                const long double mean = nf ? sum / nf : 0;
                long double var = 0;
                if (nf > 1)
                    var = (sumSq - sum * sum / nf) / (nf - 1);
                if (var < 0) var = 0;
                const long double se = nf ? std::sqrt(var / nf) : 0;
                std::printf("m4probe fan 4->5: sample=canonical-key-hash "
                            "from captured hash-window states n=%zu/%zu "
                            "seed=%llu\n",
                            nf, win.real_size(),
                            (unsigned long long)fanSeed);
                std::printf("m4probe fan 4->5: min=%llu p05=%llu med=%llu "
                            "p95=%llu max=%llu mean=%.3Lf se=%.3Lf "
                            "normal95=[%.3Lf,%.3Lf] time=%.1fs\n",
                            nf ? (unsigned long long)fans.front() : 0ULL,
                            nf ? (unsigned long long)fans[nf / 20] : 0ULL,
                            nf ? (unsigned long long)fans[nf / 2] : 0ULL,
                            nf ? (unsigned long long)fans[(nf * 19) / 20]
                               : 0ULL,
                            nf ? (unsigned long long)fans.back() : 0ULL,
                            mean, se, mean - 1.96L * se,
                            mean + 1.96L * se, now_s() - tf0);
            }
            if (fanCanonSample > 0) {
                const u64 canonSeed =
                    (m4ParentSeedSet ? m4ParentSeed : 20260731ULL) ^
                    0x94d049bb133111ebULL;
                std::vector<u32> canonParents =
                    layer_key_hash_sample(win, (size_t)fanCanonSample,
                                          canonSeed);
                Layer calib;
                calib.init_fixed(fanCanonCap);
                u64 calibEm = 0, calibHits = 0;
                const u64 canon0 = g_canonize_calls;
                const u64 nodes0 = g_canonize_nodes;
                const double tc0 = now_s();
#ifdef _OPENMP
#pragma omp parallel num_threads(nthreads) if (nthreads > 1) \
    reduction(+ : calibEm, calibHits)
#endif
                {
                    EmitCtx cctx;
                    cctx.next = &calib;
                    cctx.emissions = 0;
                    cctx.cacheHits = 0;
                    cctx.row = nullptr;
#ifdef _OPENMP
#pragma omp for schedule(dynamic, 4)
#endif
                    for (long long s = 0;
                         s < (long long)canonParents.size(); s++) {
                        const u32 i = canonParents[(size_t)s];
                        cctx.prepare(win.keys[i], 1);
                        cctx.rec(0, (1u << N2C) - 1);
                    }
                    calibEm += cctx.emissions;
                    calibHits += cctx.cacheHits;
                    flush_canon_counters();
                }
                const double tcd = now_s() - tc0;
                const u64 canonCalls = g_canonize_calls - canon0;
                const u64 canonNodes = g_canonize_nodes - nodes0;
                std::printf("m4probe canonical 4->5: "
                            "sample=canonical-key-hash n=%zu seed=%llu "
                            "emissions=%llu distinct=%zu holes=%u "
                            "time=%.3fs ns/em=%.1f cache_hits=%.2f%% "
                            "canon_calls=%llu avg_nodes=%.2f\n",
                            canonParents.size(),
                            (unsigned long long)canonSeed,
                            (unsigned long long)calibEm, calib.real_size(),
                            calib.holes, tcd,
                            calibEm ? tcd * 1e9 / calibEm : 0.0,
                            calibEm ? 100.0 * calibHits / calibEm : 0.0,
                            (unsigned long long)canonCalls,
                            canonCalls ? (double)canonNodes / canonCalls
                                       : 0.0);
            }
            std::printf("caveats: the parent sample covers only part of "
                        "layer 3; heterogeneous in-degree biases the "
                        "Poisson value low.  Fan estimates are uniform over "
                        "captured keys, and become global only as hash-window "
                        "capture saturates.\n");
            return 0;
        }
        if (stopAfter > 0 && L >= stopAfter) {
            // ---- bounded stop: report built layers, masses, fan sample ----
            for (int l = 1; l <= stopAfter; l++) {
                if (layers[l].size() == 0) {
                    std::printf("layer %d: not loaded in staged process\n", l);
                    continue;
                }
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
            if (fanSample > 0) {
                // count-only fan of the REAL (L+1)-layer children just built:
                // this is the bounded (L+1)->(L+2) calibration (runbook 5.4)
                Layer& lay = layers[L + 1];
                size_t cnp = lay.size();
                size_t ctake = std::min((size_t)fanSample, cnp);
                size_t cstride = cnp / ctake;
                EmitCtx fctx;
                fctx.next = nullptr;
                fctx.emissions = 0;
                fctx.cacheHits = 0;
                fctx.countOnly = true;
                fctx.row = nullptr;
                std::vector<u64> fans;
                fans.reserve(ctake);
                double tf0 = now_s();
                u64 prev = 0;
                for (size_t s = 0; s < ctake; s++) {
                    size_t i = s * cstride;
                    while (i < cnp && lay.is_hole((u32)i)) i++;
                    if (i >= cnp) continue;
                    if (s + 1 < ctake && i >= (s + 1) * cstride) continue;
                    fctx.prepare(lay.keys[i], 0);
                    fctx.rec(0, (1u << N2C) - 1);
                    fans.push_back(fctx.emissions - prev);
                    prev = fctx.emissions;
                }
                ctake = fans.size();
                std::sort(fans.begin(), fans.end());
                u128 tot = 0;
                for (u64 v : fans) tot += v;
                double mean = ctake ? (double)(u64)(tot / ctake) : 0.0;
                std::printf("calib fan %d->%d over %zu real layer-%d states: "
                            "min=%llu med=%llu mean=%.1f max=%llu  "
                            "sample_time=%.1fs\n",
                            L + 1, L + 2, ctake, L + 1,
                            ctake ? (unsigned long long)fans.front() : 0ULL,
                            ctake ? (unsigned long long)fans[ctake / 2] : 0ULL,
                            mean,
                            ctake ? (unsigned long long)fans.back() : 0ULL,
                            now_s() - tf0);
            }
            return 0;
        }
        double t0 = now_s();
        u64 canon0 = g_canonize_calls;
        size_t nParents = layers[L].size();
        bool record = doRank;
        if (record) Rrows[L].resize(nParents);
        u64 totEmissions = 0, totHits = 0;
        u64 rehearsalSelected = 0;
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
            ckpt_release(g_resumeCk);   // release the image copy
            ckCursor = g_resumeHdr.cursorChunk;
            totEmissions = g_resumeHdr.emissionsSoFar;
            totHits = g_resumeHdr.cacheHitsSoFar;
            if (rehearsalMode) {
                const u64 prefix = std::min<u64>(
                    (u64)nParents, ckCursor * ckChunk);
                for (u64 i = 0; i < prefix; i++)
                    if (!layers[L].is_hole((u32)i) &&
                        rehearsal_parent_selected(layers[L].keys[i], L))
                        rehearsalSelected++;
            }
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
    reduction(+ : totEmissions, totHits, rehearsalSelected)
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
                    if (rehearsalMode &&
                        !rehearsal_parent_selected(layers[L].keys[i], L))
                        continue;
                    if (rehearsalMode) rehearsalSelected++;
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
                if (!ckpt_write_pair(L, layers[L], layers[L + 1],
                                     wideHere ? &gWide : nullptr, ckStep + 1,
                                     nChunks, ckChunk, totEmissions, totHits))
                    return 12;
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
        if (rehearsalMode)
            std::printf("rehearsal parent selection %d->%d: %llu/%zu "
                        "canonical parents (hash fraction 1/%llu)\n",
                        L, L + 1,
                        (unsigned long long)rehearsalSelected,
                        layers[L].real_size(),
                        (unsigned long long)g_rehearsalDenom);
        std::fflush(stdout);
        if (!checkPenultimateLayer(L + 1)) return 9;
        if (ckptHere) {   // finalized child = next transition's parent snapshot
            const std::string snap =
                g_ckptBase + ".L" + std::to_string(L + 1) + ".snap";
            if ((g_lastCkptPath.empty() ||
                 !ck_snapshot_link(g_lastCkptPath, snap)) &&
                !layer_save_file(snap, layers[L + 1], L + 1,
                                 wideHere ? &gWide : nullptr))
                return 12;
        }
        for (size_t si = 0; si < g_saveLayers.size(); si++)
            if (g_saveLayers[si].first == L + 1)
                if (!layer_save_file(g_saveLayers[si].second,
                                     layers[L + 1], L + 1,
                                     wideHere ? &gWide : nullptr))
                    return 12;
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
        u128 Tw = (C >= 6 || g_forceWide)
                      ? (i < gWide.size() ? gWide[i] : (u128)0)
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
    if (C == 6 && !rehearsalMode) {
        if (classesOut.size() != 63199) {
            std::fprintf(stderr,
                         "C=6 complete class count %zu != 63199\n",
                         classesOut.size());
            return 3;
        }
        auto kk = known_c6_keys();
        const u64 expected[2] = {
            6986348258918400ULL,
            7053808087203840ULL,
        };
        const State keys[2] = {kk.first, kk.second};
        for (int k = 0; k < 2; k++) {
            auto it = std::find_if(
                classesOut.begin(), classesOut.end(),
                [&](const ClassOut& c) { return c.q == keys[k]; });
            if (it == classesOut.end() || it->F != expected[k]) {
                std::fprintf(stderr,
                             "C=6 G%d bridge check failed (expected F=%llu)\n",
                             k + 1, (unsigned long long)expected[k]);
                return 3;
            }
            std::printf("C=6 G%d bridge F=%llu: OK\n", k + 1,
                        (unsigned long long)it->F);
        }
    }
    std::printf("layers (states): ");
    for (int L = 1; L <= C; L++) std::printf("%zu ", layers[L].real_size());
    std::printf("\nemissions: ");
    for (u64 e : emissionsPerT) std::printf("%llu ", (unsigned long long)e);
    std::printf("\ncomplete classes = %zu\n", classesOut.size());
    std::printf("sum_w (labelled multiplicity sum) = %s\n", u128_str(sumW).c_str());
    if (rehearsalMode && C < 6)
        std::printf("rehearsal weighted-square checksum = %s "
                    "(NOT N(%d))\n", u128_str(N).c_str(), C);
    else if (rehearsalMode)
        std::printf("rehearsal weighted-square checksum: sum the watermarked "
                    "--dump CSV externally (NOT N(%d))\n", C);
    else if (C < 6)
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
    if (!rehearsalMode &&
        (C == 2 || C == 3 || C == 4 || C == 5)) {
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

        // Separation + coefficient-histogram differential against the
        // definition-independent full-group lexicographic representative.
        // Orbit membership above rules out over-merging; this comparison
        // catches a canonicalizer that splits one true orbit into two stored
        // keys.  The aggregated (T,stab) multiset additionally detects a
        // partition mismatch even when the number of keys happens to agree.
        struct SampleRef { int layer; u32 index; };
        std::vector<SampleRef> sample;
        const size_t quota =
            ((size_t)scanCheckN + (size_t)C - 1) / (size_t)C;
        for (int L = 1; L <= C; L++) {
            Layer& lay = layers[L];
            const size_t real = lay.real_size();
            const size_t take = std::min(quota, real);
            if (!take) continue;
            size_t nextRank = 0;
            size_t seen = 0;
            size_t picked = 0;
            for (u32 i = 0; i < (u32)lay.size() && picked < take; i++) {
                if (lay.is_hole(i)) continue;
                if (seen == nextRank) {
                    sample.push_back({L, i});
                    picked++;
                    nextRank = (picked * real) / take;
                }
                seen++;
            }
        }
        Layer bruteKeys;
        bruteKeys.init(sample.size() * 2 + 16);
        std::vector<State> firstFast;
        std::vector<u32> bruteStab;
        std::vector<std::pair<u64, u32>> fastHist;
        size_t conflicts = 0;
        for (const SampleRef& sr : sample) {
            Layer& lay = layers[sr.layer];
            const State& fast = lay.keys[sr.index];
            State brute = canon_scan(fast);
            const size_t oldSize = bruteKeys.size();
            u32 bi = bruteKeys.find_or_add(brute);
            if ((size_t)bi == oldSize) {
                firstFast.push_back(fast);
                bruteStab.push_back(lay.stab[sr.index]);
            } else if (!(firstFast[bi] == fast)) {
                conflicts++;
            }
            bruteKeys.T[bi] += lay.T[sr.index];
            fastHist.push_back({lay.T[sr.index], lay.stab[sr.index]});
        }
        std::vector<std::pair<u64, u32>> bruteHist;
        bruteHist.reserve(bruteKeys.size());
        for (u32 i = 0; i < (u32)bruteKeys.size(); i++)
            bruteHist.push_back({bruteKeys.T[i], bruteStab[i]});
        std::sort(fastHist.begin(), fastHist.end());
        std::sort(bruteHist.begin(), bruteHist.end());
        const bool sepOk =
            conflicts == 0 && bruteKeys.size() == sample.size();
        const bool histOk = fastHist == bruteHist;
        std::printf("canonical separation/histogram differential: "
                    "fast=%zu brute=%zu conflicts=%zu histogram=%s %s\n",
                    sample.size(), bruteKeys.size(), conflicts,
                    histOk ? "MATCH" : "MISMATCH",
                    (sepOk && histOk) ? "OK" : "FAIL");
        if (!sepOk || !histOk) return 6;
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
            sort_masks(ref.m.data(), N2C);
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
             "labelled_multiplicity,"
          << (rehearsalMode ? "F_rehearsal" : "F") << "\n";
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

    if (rehearsalMode)
        std::printf("BOUNDED REHEARSAL COMPLETED for C=%d (NOT N(%d))\n",
                    C, C);
    else
        std::printf("ALL GATES PASSED for C=%d\n", C);
    return 0;
}
