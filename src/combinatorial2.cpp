// combinatorial2.cpp — M5/M6: combinatorial completion counts C(sigma) -> N0,
// no enumeration of full grids, sub-second.
//
//   C(sigma1) = sum over column-compatible (sigma2,sigma3) of B(sigma2)*B(sigma3)
//             = completion count of any top band with signature sigma1.
//   B(sigma)  = #bands with signature sigma
//             = sum over row0-transversals of 2^(#cycles of residual 2-regular graph).
//   N0 = 9! * 72 * sum_i mult_i * C_i        (mult_i = the 44-class sizes from M2).
//
// Speed: B is invariant under relabelling + column/box permutation.  After
// relabelling box1 to the standard partition P, each remaining box is one of
// only 280 "shapes" (box-partitions up to column order).  So B reduces to a
// dense 280x280 table Btab[a][b] = #bands (P, shape a, shape b), and every
// hot-loop B-query is a flat array lookup -- no hashing.  rel[i][j] precomputes
// the shape obtained by relabelling box i -> P and applying it to box j.
#include <cstdio>
#include <cstdint>
#include <vector>
#include <array>
#include <algorithm>
#include <unordered_map>
#include <fstream>
#include <string>
#include <chrono>

using Mask = int;
using Box  = std::array<Mask,3>;

static std::vector<Box> enumerate_boxes() {
    std::vector<Box> out;
    for (int c0 = 0; c0 < 512; ++c0) {
        if (__builtin_popcount(c0) != 3) continue;
        int rest = 0x1FF ^ c0;
        for (int c1 = rest; c1; c1 = (c1 - 1) & rest)
            if (__builtin_popcount(c1) == 3) out.push_back({c0, c1, rest ^ c1});
    }
    return out;
}
static inline int boxkey(const Box& b){ return (b[0] << 18) | (b[1] << 9) | b[2]; }

// ---- B(P, shapeA, shapeB) via row0-transversals then 2^cycles ---------------
static int64_t g_total; static int g_T[9]; static int g_r0[9];
static int cycles_of_residual() {
    int par[18]; for (int i = 0; i < 18; ++i) par[i] = i;
    auto find = [&](int x){ while (par[x] != x){ par[x] = par[par[x]]; x = par[x]; } return x; };
    int comps = 18;
    for (int j = 0; j < 9; ++j)
        for (int m = g_T[j] ^ g_r0[j]; m; m &= m - 1) {
            int d = __builtin_ctz(m), a = find(j), b = find(9 + d);
            if (a != b) { par[a] = b; --comps; }
        }
    return comps;
}
static void row0_dfs(int j, int used) {
    if (j == 9) { g_total += (int64_t)1 << cycles_of_residual(); return; }
    for (int m = g_T[j]; m; m &= m - 1) {
        int d0 = m & -m; if (used & d0) continue;
        g_r0[j] = d0; row0_dfs(j + 1, used | d0);
    }
}
static int64_t Bcompute(const Box& a, const Box& b) {
    int P0=(1<<0)|(1<<3)|(1<<6), P1=(1<<1)|(1<<4)|(1<<7), P2=(1<<2)|(1<<5)|(1<<8);
    g_T[0]=P0;g_T[1]=P1;g_T[2]=P2; g_T[3]=a[0];g_T[4]=a[1];g_T[5]=a[2]; g_T[6]=b[0];g_T[7]=b[1];g_T[8]=b[2];
    g_total = 0; row0_dfs(0, 0); return g_total;
}

static Box triple_to_box(const std::string& a, const std::string& b, const std::string& c) {
    auto m = [](const std::string& s){ int x = 0; for (char ch : s) x |= 1 << (ch - '1'); return x; };
    return { m(a), m(b), m(c) };
}

int main() {
    auto t0 = std::chrono::steady_clock::now();
    auto boxes = enumerate_boxes();
    const int NB = (int)boxes.size();                       // 1680
    std::unordered_map<int,int> bidx; bidx.reserve(NB*2);
    for (int i = 0; i < NB; ++i) bidx[boxkey(boxes[i])] = i;

    // shapes: column-sorted box-partitions (280 of them); shapeid map
    std::vector<Box> shapes;
    std::unordered_map<int,int> shapeid;                    // colsorted boxkey -> 0..279
    auto shape_of = [&](Box b)->int{ std::sort(b.begin(),b.end()); int k=boxkey(b);
        auto it=shapeid.find(k); if(it!=shapeid.end())return it->second; int id=(int)shapes.size(); shapeid[k]=id; shapes.push_back(b); return id; };
    for (const auto& b : boxes) shape_of(b);
    const int NS = (int)shapes.size();                      // 280

    // compatibility (indices) and their per-column complement indices
    std::vector<std::vector<int>> compat(NB), compl_(NB);
    for (int i = 0; i < NB; ++i)
        for (int j = 0; j < NB; ++j)
            if (!(boxes[i][0]&boxes[j][0]) && !(boxes[i][1]&boxes[j][1]) && !(boxes[i][2]&boxes[j][2])) {
                compat[i].push_back(j);
                Box w={0x1FF^boxes[i][0]^boxes[j][0],0x1FF^boxes[i][1]^boxes[j][1],0x1FF^boxes[i][2]^boxes[j][2]};
                compl_[i].push_back(bidx[boxkey(w)]);
            }
    Box P={(1<<0)|(1<<3)|(1<<6),(1<<1)|(1<<4)|(1<<7),(1<<2)|(1<<5)|(1<<8)};
    int Pidx = bidx[boxkey(P)];

    // rel[i*NB+j] = shape id after relabelling box i -> P, applied to box j.
    // Only the rows for anchors used in the hot loop (compat(P) and compl(P))
    // are ever read, so we build just those.
    std::vector<uint16_t> rel((size_t)NB * NB);
    std::vector<char> need(NB, 0);
    for (int a : compat[Pidx]) need[a] = 1;
    for (int a : compl_[Pidx]) need[a] = 1;
    for (int i = 0; i < NB; ++i) {
        if (!need[i]) continue;
        Box s = boxes[i]; std::sort(s.begin(), s.end());
        int map[9];
        for (int k = 0; k < 3; ++k){ int col=s[k],t=0; for(int x=col;x;x&=x-1) map[__builtin_ctz(x)]=k+3*t++; }
        uint16_t* row = &rel[(size_t)i*NB];
        for (int j = 0; j < NB; ++j) {
            Box r; for(int c=0;c<3;c++){int v=0;for(int x=boxes[j][c];x;x&=x-1)v|=1<<map[__builtin_ctz(x)];r[c]=v;}
            row[j] = (uint16_t)shape_of(r);
        }
    }

    // dense Btab[a*NS+b] = #bands (P, shape a, shape b)
    std::vector<int64_t> Btab((size_t)NS * NS, 0);
    for (int a = 0; a < NS; ++a)
        for (int b = a; b < NS; ++b) {
            int64_t v = Bcompute(shapes[a], shapes[b]);
            Btab[(size_t)a*NS+b] = v; Btab[(size_t)b*NS+a] = v;
        }
    double t_pre = std::chrono::duration<double>(std::chrono::steady_clock::now()-t0).count();

    auto computeC = [&](int b2, int b3) -> int64_t {
        const auto& C1=compat[Pidx]; const auto& W1=compl_[Pidx];
        const auto& C2=compat[b2];   const auto& W2=compl_[b2];
        const auto& C3=compat[b3];   const auto& W3=compl_[b3];
        const int64_t* B = Btab.data();
        int64_t total = 0;
        for (size_t i1 = 0; i1 < C1.size(); ++i1) {
            const uint16_t* rU = &rel[(size_t)C1[i1]*NB];
            const uint16_t* rW = &rel[(size_t)W1[i1]*NB];
            for (size_t i2 = 0; i2 < C2.size(); ++i2) {
                const int64_t* BU = &B[(size_t)rU[C2[i2]] * NS];
                const int64_t* BW = &B[(size_t)rW[W2[i2]] * NS];
                int64_t acc = 0;
                for (size_t i3 = 0; i3 < C3.size(); ++i3)
                    acc += BU[rU[C3[i3]]] * BW[rW[W3[i3]]];
                total += acc;
            }
        }
        return total;
    };

    struct Rep { std::string t[6]; long long mult, sol; };
    std::vector<Rep> reps;
    { std::ifstream in("data/ed44.txt"); Rep r;
      while (in >> r.t[0]>>r.t[1]>>r.t[2]>>r.t[3]>>r.t[4]>>r.t[5]>>r.mult>>r.sol) reps.push_back(r); }

    int ok = 0; unsigned __int128 sum_mult_C = 0;
    std::printf("  # : representative          :        C_i  : 72*sol_i   : ok\n");
    for (size_t i = 0; i < reps.size(); ++i) {
        const Rep& r = reps[i];
        int b2 = bidx[boxkey(triple_to_box(r.t[0],r.t[1],r.t[2]))];
        int b3 = bidx[boxkey(triple_to_box(r.t[3],r.t[4],r.t[5]))];
        int64_t C = computeC(b2, b3), expect = 72LL * r.sol;
        bool good = (C == expect); ok += good;
        sum_mult_C += (unsigned __int128)(uint64_t)r.mult * (uint64_t)C;
        std::printf("%3zu : %s %s %s %s %s %s : %11lld : %10lld : %s\n", i+1,
                    r.t[0].c_str(),r.t[1].c_str(),r.t[2].c_str(),r.t[3].c_str(),r.t[4].c_str(),r.t[5].c_str(),
                    (long long)C, (long long)expect, good?"OK":"BAD");
    }
    unsigned __int128 N0 = (unsigned __int128)362880u * 72u * sum_mult_C;
    auto u128=[](unsigned __int128 x){std::string s;if(!x)s="0";while(x){s=char('0'+(int)(x%10))+s;x/=10;}return s;};
    double t_all = std::chrono::duration<double>(std::chrono::steady_clock::now()-t0).count();
    std::printf("--------------------------------------------------------------\n");
    std::printf("classes verified C_i == 72*sol_i : %d/%zu\n", ok, reps.size());
    std::printf("N0 = 9!*72*sum(mult*C) = %s\n", u128(N0).c_str());
    std::printf("                expected         = 6670903752021072936960  [%s]\n",
                u128(N0) == "6670903752021072936960" ? "OK" : "MISMATCH");
    std::printf("shapes=%d  precompute=%.3f s  total=%.3f s\n", NS, t_pre, t_all);
    return 0;
}
