// combinatorial2.cpp — M5: compute completion counts C(sigma) combinatorially,
// with NO enumeration of full grids, and aggregate to N0.
//
//   C(sigma1) = sum over (sigma2, sigma3) that column-partition sigma1 of
//               B(sigma2) * B(sigma3)
//             = completion count of any top band with signature sigma1.
//
// For a fixed sigma1, sigma2's box k must be column-disjoint from sigma1's box
// k (independently per box); sigma3 is then forced as the per-column complement.
// So sigma2 ranges over compat(box1) x compat(box2) x compat(box3).
//
// Verification gates:
//   * C_i == 72 * (equiv.c solution count)   for all 44 classes
//     (equiv.c counts completions with the 6 lower rows order-reduced by 72)
//   * N0 = 9! * 72 * sum_i mult_i * C_i       (mult_i = the 44-class sizes)
//
// All accumulation uses int64_t / unsigned __int128 (MinGW `long` is 32-bit).
#include <cstdio>
#include <cstdint>
#include <vector>
#include <array>
#include <algorithm>
#include <map>
#include <fstream>
#include <string>

using Mask = int;
using Box  = std::array<Mask,3>;
static inline int lowbit(int x){ return x & -x; }

// ---- 1680 box-partitions ---------------------------------------------------
static std::vector<Box> enumerate_boxes() {
    std::vector<Box> out;
    for (int c0 = 0; c0 < 512; ++c0) {
        if (__builtin_popcount(c0) != 3) continue;
        int rest = 0x1FF ^ c0;
        for (int c1 = rest; c1; c1 = (c1 - 1) & rest) {
            if (__builtin_popcount(c1) != 3) continue;
            out.push_back({c0, c1, rest ^ c1});
        }
    }
    return out;
}

// ---- B(sigma): valid bands with the 9 given column-triples (memoised) ------
// B is invariant under permuting columns within a box and permuting the three
// boxes, so we key by the sorted "shape-id" triple (shape = a box-partition up
// to column order; there are 280 of them).  O(1) hash lookup, computed on miss.
#include <unordered_map>
static std::unordered_map<int, int64_t> Bmemo;
static std::map<Box,int>* g_shape = nullptr;   // sorted-box -> shape id (0..279)

// B(sigma) = number of valid bands with column-triples T[0..8].
// Place row0 (a transversal); the residual (2 digits per column) is a 2-regular
// bipartite graph = disjoint cycles, and its proper 2-colourings (giving the
// ordered rows 1,2) number exactly 2^(#cycles).  So B = sum over row0 of 2^c.
static int64_t g_total; static int g_T[9]; static int g_r0[9];
static int cycles_of_residual() {
    // union-find over 9 column-nodes (0..8) and 9 digit-nodes (9..17)
    int par[18]; for (int i = 0; i < 18; ++i) par[i] = i;
    auto find = [&](int x){ while (par[x] != x) { par[x] = par[par[x]]; x = par[x]; } return x; };
    int comps = 18;
    for (int j = 0; j < 9; ++j) {
        int res = g_T[j] ^ g_r0[j];          // 2 residual digits in column j
        for (int m = res; m; m &= m - 1) {
            int d = __builtin_ctz(m);
            int a = find(j), b = find(9 + d);
            if (a != b) { par[a] = b; --comps; }
        }
    }
    return comps;                            // = number of cycles
}
static void row0_dfs(int j, int used) {
    if (j == 9) { g_total += (int64_t)1 << cycles_of_residual(); return; }
    for (int m = g_T[j]; m; m &= m - 1) {
        int d0 = m & -m; if (used & d0) continue;
        g_r0[j] = d0; row0_dfs(j + 1, used | d0);
    }
}
static inline int shapeid(const Box& b) {
    Box s = b; std::sort(s.begin(), s.end());
    return g_shape->at(s);
}
static int64_t Bsig(const Box& b1, const Box& b2, const Box& b3) {
    int s[3] = { shapeid(b1), shapeid(b2), shapeid(b3) };
    std::sort(s, s + 3);
    int key = (s[0] * 280 + s[1]) * 280 + s[2];
    auto it = Bmemo.find(key);
    if (it != Bmemo.end()) return it->second;
    int T[9] = { b1[0],b1[1],b1[2], b2[0],b2[1],b2[2], b3[0],b3[1],b3[2] };
    for (int i = 0; i < 9; ++i) g_T[i] = T[i];
    g_total = 0; row0_dfs(0, 0);
    Bmemo.emplace(key, g_total);
    return g_total;
}

static Box complement(const Box& t, const Box& u) {       // per-column complement
    return { 0x1FF ^ t[0] ^ u[0], 0x1FF ^ t[1] ^ u[1], 0x1FF ^ t[2] ^ u[2] };
}

static Box triple_to_box(const std::string a, const std::string b, const std::string c) {
    auto m = [](const std::string& s){ int x = 0; for (char ch : s) x |= 1 << (ch - '1'); return x; };
    return { m(a), m(b), m(c) };
}

int main() {
    auto boxes = enumerate_boxes();
    // shape table: sorted box-partition -> id (0..279); used for B memo key
    std::map<Box,int> shape;
    for (const auto& b : boxes) { Box s = b; std::sort(s.begin(), s.end()); if (!shape.count(s)) shape[s] = (int)shape.size(); }
    g_shape = &shape;
    std::fprintf(stderr, "box shapes = %zu\n", shape.size());
    // compat[i] = box-partitions column-disjoint from boxes[i]
    std::vector<std::vector<Box>> compat(boxes.size());
    for (size_t i = 0; i < boxes.size(); ++i)
        for (size_t j = 0; j < boxes.size(); ++j)
            if (!(boxes[i][0] & boxes[j][0]) && !(boxes[i][1] & boxes[j][1]) && !(boxes[i][2] & boxes[j][2]))
                compat[i].push_back(boxes[j]);
    // index a box-partition -> its position (to fetch compat)
    std::map<Box,int> boxidx;
    for (size_t i = 0; i < boxes.size(); ++i) boxidx[boxes[i]] = (int)i;
    auto compat_of = [&](const Box& b) -> const std::vector<Box>& { return compat[boxidx[b]]; };

    Box P = { (1<<0)|(1<<3)|(1<<6), (1<<1)|(1<<4)|(1<<7), (1<<2)|(1<<5)|(1<<8) };
    std::fprintf(stderr, "|compat(P)| = %zu\n", compat_of(P).size());

    // C(sigma1) for sigma1 = (P, b2, b3)
    auto computeC = [&](const Box& b2, const Box& b3) -> int64_t {
        const auto& C1 = compat_of(P);
        const auto& C2 = compat_of(b2);
        const auto& C3 = compat_of(b3);
        int64_t total = 0;
        for (const Box& u1 : C1) {
            Box w1 = complement(P, u1);
            for (const Box& u2 : C2) {
                Box w2 = complement(b2, u2);
                for (const Box& u3 : C3) {
                    Box w3 = complement(b3, u3);
                    total += Bsig(u1, u2, u3) * Bsig(w1, w2, w3);
                }
            }
        }
        return total;
    };

    // read the 44 classes (rep triples, mult, equiv.c solcount)
    std::ifstream in("data/ed44.txt");
    std::string t0,t1,t2,t3,t4,t5; long long mult, sol;
    int ok = 0, n = 0;
    unsigned __int128 sum_mult_C = 0;
    std::printf("  # : representative          :        C_i  : 72*sol_i   : ok\n");
    while (in >> t0 >> t1 >> t2 >> t3 >> t4 >> t5 >> mult >> sol) {
        Box b2 = triple_to_box(t0, t1, t2);
        Box b3 = triple_to_box(t3, t4, t5);
        int64_t C = computeC(b2, b3);
        int64_t expect = 72LL * sol;
        bool good = (C == expect);
        ok += good; ++n;
        sum_mult_C += (unsigned __int128)(uint64_t)mult * (uint64_t)C;
        std::printf("%3d : %s %s %s %s %s %s : %11lld : %10lld : %s\n",
                    n, t0.c_str(),t1.c_str(),t2.c_str(),t3.c_str(),t4.c_str(),t5.c_str(),
                    (long long)C, (long long)expect, good ? "OK" : "BAD");
    }
    // N0 = 9! * 72 * sum mult_i C_i
    unsigned __int128 N0 = (unsigned __int128)362880u * 72u * sum_mult_C;
    auto u128 = [](unsigned __int128 x){ std::string s; if(!x) s="0"; while(x){ s=char('0'+(int)(x%10))+s; x/=10;} return s; };
    std::printf("--------------------------------------------------------------\n");
    std::printf("classes verified C_i == 72*sol_i : %d/%d\n", ok, n);
    std::printf("N0 = 9!*72*sum(mult*C) = %s\n", u128(N0).c_str());
    std::printf("                expected         = 6670903752021072936960  [%s]\n",
                u128(N0) == "6670903752021072936960" ? "OK" : "MISMATCH");
    std::fprintf(stderr, "B memo entries = %zu\n", Bmemo.size());
    return 0;
}
