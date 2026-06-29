// combinatorial.cpp — M4: B(sigma), the number of bands per column-signature.
//
// A band's column-signature is its 9 digit-triples (one 3-element set per
// column), grouped into 3 boxes that each partition {1..9}.  B(sigma) counts
// the valid bands realising it:  fillings of the 3x9 band where column j holds
// exactly the digits of triple T[j] and every row is a permutation of 1..9.
//
// A band is fixed by choosing row0 and row1 (row2 is forced and is provably a
// permutation): for each column pick distinct row0[j],row1[j] in T[j] so that
// row0 and row1 are each permutations.  Box validity is already encoded in the
// signature, so B(sigma) = #{(row0,row1) transversals, disjoint per column}.
//
// Digits are 0..8 (bit i = digit i+1).  A box-partition is an ordered split of
// {0..8} into three column-triples; there are 9!/(3!)^3 = 1680 of them.
//
// M4 verification gate: with box1 fixed to the canonical partition, summing
// B over all 1680x1680 (box2,box3) signatures must give the number of valid
// bands whose first box has that column content = 948109639680 / 1680
// = 564350976.
#include <cstdio>
#include <cstdint>
#include <vector>
#include <array>
#include <algorithm>
#include <map>

using Mask = int;                 // 9-bit digit set
using Box  = std::array<Mask,3>;  // a box-partition: 3 column-triples

static inline int lowbit(int x){ return x & -x; }
static inline int ctz(unsigned x){ return __builtin_ctz(x); }

// ---- enumerate the 1680 box-partitions -------------------------------------
static std::vector<Box> enumerate_boxes() {
    std::vector<Box> out;
    for (int c0 = 0; c0 < 512; ++c0) {
        if (__builtin_popcount(c0) != 3) continue;
        int rest1 = 0x1FF ^ c0;
        // c1 subset of rest1, size 3
        for (int c1 = rest1; c1; c1 = (c1 - 1) & rest1) {
            if (__builtin_popcount(c1) != 3) continue;
            int c2 = rest1 ^ c1;
            // enforce c1<c2 to avoid double-listing the unordered {c1,c2}? No:
            // box-partition columns are ORDERED, so all (c0,c1,c2) are distinct.
            out.push_back({c0, c1, c2});
        }
    }
    return out;
}

// ---- B(sigma): count bands with the given 9 column-triples -----------------
// columns T[0..8]; recursively assign (row0,row1) per column.
static long Bsig_dfs(const Mask* T, int j, int used0, int used1) {
    if (j == 9) return 1;
    long cnt = 0;
    int t = T[j];
    for (int a = t; a; a &= a - 1) {           // row0[j] = bit a0
        int a0 = lowbit(a);
        if (used0 & a0) continue;
        int rest = t ^ a0;
        for (int b = rest; b; b &= b - 1) {    // row1[j] = bit b0 (distinct col)
            int b0 = lowbit(b);
            if (used1 & b0) continue;
            cnt += Bsig_dfs(T, j + 1, used0 | a0, used1 | b0);
        }
    }
    return cnt;
}
static long Bsig(const Box& b1, const Box& b2, const Box& b3) {
    Mask T[9] = { b1[0],b1[1],b1[2], b2[0],b2[1],b2[2], b3[0],b3[1],b3[2] };
    return Bsig_dfs(T, 0, 0, 0);
}

// canonical id of a box-partition under column permutation (sort the 3 masks)
static int canon_box(const Box& b, std::map<Box,int>& tbl) {
    Box s = b; std::sort(s.begin(), s.end());
    auto it = tbl.find(s);
    if (it != tbl.end()) return it->second;
    int id = (int)tbl.size(); tbl[s] = id; return id;
}

int main() {
    auto boxes = enumerate_boxes();
    std::fprintf(stderr, "box-partitions = %zu (expect 1680)\n", boxes.size());

    // canonical box ids (under column perm)
    std::map<Box,int> canontbl;
    std::vector<int> cid(boxes.size());
    for (size_t i = 0; i < boxes.size(); ++i) cid[i] = canon_box(boxes[i], canontbl);
    std::fprintf(stderr, "distinct box-shapes (under col perm) = %zu\n", canontbl.size());

    // box1 = canonical partition: columns {0,3,6},{1,4,7},{2,5,8}
    Box P = { (1<<0)|(1<<3)|(1<<6), (1<<1)|(1<<4)|(1<<7), (1<<2)|(1<<5)|(1<<8) };

    // gate: sum B(P, box2, box3) over all box2,box3, memoised by (cid2,cid3)
    std::map<std::pair<int,int>,long> memo;
    long total = 0;
    for (size_t i2 = 0; i2 < boxes.size(); ++i2) {
        for (size_t i3 = 0; i3 < boxes.size(); ++i3) {
            int a = cid[i2], b = cid[i3];
            if (a > b) std::swap(a, b);
            auto key = std::make_pair(a, b);
            auto it = memo.find(key);
            long v;
            if (it != memo.end()) v = it->second;
            else { v = Bsig(P, boxes[i2], boxes[i3]); memo[key] = v; }
            total += v;
        }
    }
    std::fprintf(stderr, "memo entries = %zu\n", memo.size());
    std::printf("sum B(P,box2,box3) = %ld   expect 564350976   [%s]\n",
                total, total == 564350976 ? "OK" : "MISMATCH");
    std::printf("  (x1680 = %ld   expect 948109639680 [%s])\n",
                total * 1680, total * 1680 == 948109639680L ? "OK" : "MISMATCH");
    return 0;
}
