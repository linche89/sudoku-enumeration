// sudoku_rc.cpp — general counter for 2xC Sudoku (boxes 2 rows x C cols),
#include <cmath>
#include <string>
// via the transpose identity  #(2xC) = #(Cx2).
//
// A Cx2 Sudoku grid (2C x 2C) has exactly TWO bands (each C rows x 2C cols).
// The completion count therefore needs no C-fold convolution -- the second
// band's column-content is the per-column complement of the first:
//
//     N = sum over band signatures sigma of  B(sigma) * B(complement(sigma)),
//
// where B(sigma) = number of valid Cx2-bands realising the column signature.
// A band has C boxes (each C rows x 2 cols); box b owns columns 2b, 2b+1, whose
// C-element symbol sets partition {0..2C-1}.  So a signature is one C-subset
// S_b per box (column 2b), with column 2b+1 = complement of S_b.
//
// This generalises the 9x9 engine; for 2x3 it must reproduce 28,200,960.
#include <cstdio>
#include <cstdint>
#include <vector>
#include <cstdlib>

static int C, M;                 // M = 2C symbols / columns
static int FULL;                 // (1<<M)-1
static const int* g_colset;      // M column subsets (masks)
static int rem_[16];             // working remaining-subset per column

// B(sigma): count fillings of the C-row band.  Generalises the 9x9 trick:
// enumerate the first C-2 rows as perfect matchings; the final 2-regular
// residual (2 rows left) is a union of cycles contributing 2^(#cycles).
static int residual_cycles() {                       // rem_[] is now 2-regular
    int par[32]; for (int i = 0; i < 2*M; ++i) par[i] = i;
    auto find = [&](int x){ while (par[x] != x){ par[x] = par[par[x]]; x = par[x]; } return x; };
    int comps = 2 * M;
    for (int col = 0; col < M; ++col)
        for (int s = rem_[col]; s; s &= s - 1) {
            int sym = __builtin_ctz(s), a = find(col), b = find(M + sym);
            if (a != b) { par[a] = b; --comps; }
        }
    return comps;                                    // = number of cycles
}
static int64_t enum_row(int rowsLeft, int col, int usedRow);
static int64_t Bfill(int rowsLeft) {
    if (rowsLeft == 2) return (int64_t)1 << residual_cycles();
    if (rowsLeft == 1) return 1;
    return enum_row(rowsLeft, 0, 0);
}
static int64_t enum_row(int rowsLeft, int col, int usedRow) {
    if (col == M) return Bfill(rowsLeft - 1);
    int64_t s = 0;
    int avail = rem_[col] & ~usedRow;
    while (avail) {
        int bit = avail & -avail; avail -= bit;
        rem_[col] ^= bit;
        s += enum_row(rowsLeft, col + 1, usedRow | bit);
        rem_[col] ^= bit;
    }
    return s;
}
static int64_t Bcount(const int* colset) {
    for (int j = 0; j < M; ++j) rem_[j] = colset[j];
    return Bfill(C);
}

// B is invariant under column permutation, so memoise by the sorted column
// subsets (packed into a 128-bit key; valid through C=5, i.e. M<=10).
#include <map>
#include <algorithm>
static std::map<unsigned __int128, int64_t> Bmemo;
static int64_t Bget(int* colset) {
    int s[16]; for (int j = 0; j < M; ++j) s[j] = colset[j];
    std::sort(s, s + M);
    unsigned __int128 key = 0;
    for (int j = 0; j < M; ++j) key = (key << M) | (unsigned)s[j];
    auto it = Bmemo.find(key);
    if (it != Bmemo.end()) return it->second;
    int64_t v = Bcount(colset);
    Bmemo.emplace(key, v);
    return v;
}

int main(int argc, char** argv) {
    C = (argc > 1) ? std::atoi(argv[1]) : 3;
    M = 2 * C; FULL = (1 << M) - 1;

    // all C-subsets of {0..M-1}
    std::vector<int> subs;
    for (int m = 0; m <= FULL; ++m) if (__builtin_popcount(m) == C) subs.push_back(m);
    std::fprintf(stderr, "C=%d  M=%d  C-subsets=%zu  raw sigma space=%.3g\n",
                 C, M, subs.size(), std::pow((double)subs.size(), C));

    // iterate sigma = (S_0,...,S_{C-1}); accumulate B(sigma)*B(sigma_bar)
    // enumerate the C-box choices via a mixed-radix counter
    // N = sum over ordered sigma of B(sigma)^2.  Group by the unordered
    // multiset of the C box-subsets: iterate non-decreasing index tuples and
    // weight by the number of orderings (C! / prod run-length!).
    auto fact = [](int n){ int64_t f = 1; for (int i = 2; i <= n; ++i) f *= i; return f; };
    const int ns = (int)subs.size();
    std::vector<int> idx(C, 0);
    unsigned __int128 N = 0;
    int colset[16];
    int64_t multisets = 0;
    for (;;) {
        for (int b = 0; b < C; ++b) { int S = subs[idx[b]]; colset[2*b] = S; colset[2*b+1] = FULL ^ S; }
        int64_t b1 = Bcount(colset);
        if (b1) {
            int64_t mult = fact(C);
            for (int r = 0; r < C; ) { int s = r; while (s + 1 < C && idx[s+1] == idx[r]) ++s; mult /= fact(s - r + 1); r = s + 1; }
            N += (unsigned __int128)(uint64_t)(mult * b1) * (uint64_t)b1;   // mult * B^2
        }
        ++multisets;
        int p = C - 1; while (p >= 0 && idx[p] == ns - 1) --p;
        if (p < 0) break;
        ++idx[p]; for (int q = p + 1; q < C; ++q) idx[q] = idx[p];
    }

    auto u128 = [](unsigned __int128 x){ std::string s; if(!x)s="0"; while(x){ s=char('0'+(int)(x%10))+s; x/=10;} return s; };
    std::printf("2x%d (grid %dx%d): N = %s\n", C, M, M, u128(N).c_str());
    std::fprintf(stderr, "multisets iterated = %lld\n", (long long)multisets);

    const char* expect = (C==3)?"28200960" : (C==4)?"29136487207403520" :
                         (C==5)?"1903816047972624930994913280000" : nullptr;
    if (expect) std::printf("expected 2x%d = %s  [%s]\n", C, expect,
                            u128(N)==expect ? "OK" : "MISMATCH");
    return 0;
}
