// reduce44.cpp — Milestone M2: reduce to the 44 completion-count classes.
//
// The number of ways a top band completes to a full grid depends ONLY on the
// band's column-signature — the unordered triple of digits sitting in each
// column.  (Row arrangements within the band do not change which digits occupy
// a column, and the completion count is a column-wise function.)  So we work
// with signatures, not full bands:
//
//   1. generate the 36288 canonical bands (B1 = 123/456/789, B2/B3 column-sorted)
//   2. map each to its canonical B2/B3 column-signature (6 sorted triples)
//   3. deduplicate -> 22266 distinct signatures, each weighted by how many of
//      the 36288 bands carry it
//   4. colour the signatures into orbits under the completion-count-preserving
//      symmetry: relabel digits + permute the three stacks + permute columns
//      within a stack (rows act trivially on signatures, but the row arrangement
//      of the anchored stack selects the relabelling).  -> 44 orbits.
//
// Representative = lexicographically smallest signature in the orbit;
// multiplicity = number of the 36288 bands in the orbit.
//
// This is an independent reimplementation of the reduction in Ed Russell's
// reference/equiv.c; the output table is verified against it.
#include "reduce.hpp"          // fj::generate()
#include <array>
#include <map>
#include <vector>
#include <cstdio>
#include <cstdint>

using Sig = std::array<int, 6>;   // six packed column-triples (B2,B3), e.g. 0x124

// pack three digits (1..9) into 0x(d1)(d2)(d3) with d1<d2<d3
static int pack(int a, int b, int c) {
    int present[10] = {0}, packed = 0;
    present[a] = present[b] = present[c] = 1;
    for (int i = 1; i <= 9; ++i) if (present[i]) packed = (packed << 4) + i;
    return packed;
}

// canonicalise a 6-triple signature: sort columns within each box, order the
// two boxes (identical to equiv.c::reporder)
static void reporder(Sig& r) {
    auto sw = [&](int i, int j){ if (r[i] > r[j]) std::swap(r[i], r[j]); };
    sw(0,1); sw(0,2); sw(1,2);
    sw(3,4); sw(3,5); sw(4,5);
    if (r[0]*0x1000 + r[1] > r[3]*0x1000 + r[4]) {
        std::swap(r[0], r[3]); std::swap(r[1], r[4]); std::swap(r[2], r[5]);
    }
}

int main() {
    // ---- steps 1-3: signatures with weights ------------------------------
    auto bands = fj::generate();                         // 36288
    std::map<Sig, long> weight;                          // sorted -> equiv.c order
    for (const auto& b : bands) {
        Sig s;
        for (int c = 3; c < 9; ++c)
            s[c - 3] = pack(b.v[0][c] + 1, b.v[1][c] + 1, b.v[2][c] + 1);
        reporder(s);
        ++weight[s];
    }
    const int N = (int)weight.size();
    std::vector<Sig> sig(N);
    std::vector<long> num(N);
    std::map<Sig, int> index;                            // findrep
    {
        int i = 0;
        for (auto& kv : weight) { sig[i] = kv.first; num[i] = kv.second; index[kv.first] = i; ++i; }
    }
    long bandsum = 0; for (long n : num) bandsum += n;
    std::fprintf(stderr, "distinct signatures = %d (expect 22266); band total = %ld (expect 36288)\n",
                 N, bandsum);

    // ---- step 4: colour into orbits --------------------------------------
    static const int perm[18] = { 0,1,2, 0,2,1, 1,0,2, 1,2,0, 2,0,1, 2,1,0 };
    std::vector<int> colour(N);
    for (int i = 0; i < N; ++i) colour[i] = i;

    // colour the whole orbit of signature `rep` with `true_colour`
    auto permute = [&](const Sig& rep, int true_colour) {
        int box[3][9];
        for (int i = 0; i < 9; ++i) box[i / 3][i % 3] = i + 1;       // B1 canonical
        for (int i = 0; i < 6; ++i) {
            box[0][i + 3] =  rep[i] >> 8;
            box[1][i + 3] = (rep[i] >> 4) & 15;
            box[2][i + 3] =  rep[i] & 15;
        }
        for (int ordbox = 0; ordbox < 3; ++ordbox)
        for (const int* colp = perm; colp < perm + 18; colp += 3)
        for (const int* cp0 = perm; cp0 < perm + 18; cp0 += 3)
        for (const int* cp1 = perm; cp1 < perm + 18; cp1 += 3)
        for (const int* cp2 = perm; cp2 < perm + 18; cp2 += 3) {
            const int* cellp[3] = { cp0, cp1, cp2 };
            int map[10];
            for (int i = 0; i < 9; ++i) {
                int x = i % 3, y = i / 3;
                map[ box[ cellp[x][y] ][ 3 * ordbox + colp[x] ] ] = i + 1;
            }
            Sig tmp;
            for (int i = 0; i < 6; ++i) {
                int col = 3 * ((ordbox + 1 + (i / 3)) % 3) + i % 3;
                tmp[i] = pack(map[box[0][col]], map[box[1][col]], map[box[2][col]]);
            }
            reporder(tmp);
            auto it = index.find(tmp);
            if (it != index.end()) colour[it->second] = true_colour;
        }
    };

    // ---- emit the 44-class table (idx : rep : mult), equiv.c format ------
    int classnum = 0;
    long total = 0;
    for (int i = 0; i < N; ++i) {
        if (colour[i] != i) continue;                    // already coloured earlier
        permute(sig[i], i);
        long mult = 0;
        for (int j = i; j < N; ++j) if (colour[j] == i) mult += num[j];
        ++classnum;
        total += mult;
        std::printf("%3d : %3x %3x %3x %3x %3x %3x : %4ld\n",
                    classnum, sig[i][0], sig[i][1], sig[i][2],
                    sig[i][3], sig[i][4], sig[i][5], mult);
    }
    std::fprintf(stderr, "classes = %d (expect 44); sum(mult) = %ld (expect 36288)\n",
                 classnum, total);
    return 0;
}
