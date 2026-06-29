// reduce.hpp — Phase 2+3 of the FJ pipeline.
//
//   * generate() enumerates the 36288 lexicographically-reduced band
//     configurations (B1 frozen, B2/B3 columns sorted, B2<B3).
//   * reduce()   collapses those 36288 into 71 equivalence classes whose
//     members provably complete to a full grid in the same number of ways,
//     returning one representative per class together with its multiplicity
//     (the count of catalogue entries it stands for; the multiplicities sum
//     to 36288).
//
// The equivalence generators are exactly those of Felgenhauer–Jarvis:
//   R  : permute the three band rows           (then renormalize)
//   C  : permute the three columns of B1        (then renormalize)
//   B  : permute the three stacks B1,B2,B3       (then renormalize)
//   kxn: relabel a 2xk / kx2 sub-rectangle whose two rows carry a cyclic
//        rotation of the same k symbols (2x2, 2x3, 3x2, 4x2).
// Their reflexive–symmetric–transitive closure has 71 classes.
#pragma once
#include "band.hpp"
#include <vector>
#include <string>
#include <unordered_map>
#include <numeric>
#include <cstdint>

namespace fj {

struct Class {
    uint64_t mult;       // # of the 36288 catalogue entries in this class
    std::string repr;    // lexicographically smallest member, "[...,...,...]"
    Band band;           // that representative, decoded
};

// ---- generation of the 36288 catalogue ------------------------------------
// The first row of B2/B3 is one of 10 canonical patterns (B2 holds the
// smallest free digit 3, columns sorted, B2 before B3).  Rows 1 and 2 are then
// filled by ordinary Latin/box-constrained backtracking.
inline void gen_rows(Band& b, std::vector<Band>& out,
                     int row, int col,
                     int rowmask[3], int colmask[9], int boxmask[3]) {
    if (row == 3) { out.push_back(b); return; }
    if (col == 9) { gen_rows(b, out, row + 1, 3, rowmask, colmask, boxmask); return; }
    int box = col / 3;                       // box index among B1,B2,B3 (0,1,2)
    int used = rowmask[row] | colmask[col] | boxmask[box];
    for (int d = 0; d < 9; ++d) {
        int bit = 1 << d;
        if (used & bit) continue;
        b.v[row][col] = d;
        rowmask[row] |= bit; colmask[col] |= bit; boxmask[box] |= bit;
        gen_rows(b, out, row, col + 1, rowmask, colmask, boxmask);
        rowmask[row] ^= bit; colmask[col] ^= bit; boxmask[box] ^= bit;
    }
    b.v[row][col] = -1;
}

inline std::vector<Band> generate() {
    static const int rem[10][6] = {
        {3,4,5,6,7,8},{3,4,6,5,7,8},{3,4,7,5,6,8},{3,4,8,5,6,7},{3,5,6,4,7,8},
        {3,5,7,4,6,8},{3,5,8,4,6,7},{3,6,7,4,5,8},{3,6,8,4,5,7},{3,7,8,4,5,6},
    };
    std::vector<Band> out;
    out.reserve(36288);
    for (int h = 0; h < 10; ++h) {
        Band b;
        int rowmask[3] = {0,0,0}, colmask[9] = {0,0,0,0,0,0,0,0,0}, boxmask[3] = {0,0,0};
        // B1 occupies rows/cols/boxes
        for (int i = 0; i < 3; ++i)
            for (int j = 0; j < 3; ++j) {
                int bit = 1 << b.v[i][j];
                rowmask[i] |= bit; colmask[j] |= bit; boxmask[0] |= bit;
            }
        // fixed first row of B2/B3
        for (int x = 3; x < 9; ++x) {
            int d = rem[h][x - 3];
            b.v[0][x] = d;
            int bit = 1 << d;
            rowmask[0] |= bit; colmask[x] |= bit; boxmask[x / 3] |= bit;
        }
        gen_rows(b, out, 1, 3, rowmask, colmask, boxmask);
    }
    return out;
}

// ---- union–find over the catalogue ----------------------------------------
class DSU {
    std::vector<uint32_t> parent;
    std::vector<uint64_t> sz;
public:
    explicit DSU(size_t n) : parent(n), sz(n, 1) {
        std::iota(parent.begin(), parent.end(), 0u);
    }
    uint32_t find(uint32_t x) {
        while (parent[x] != x) { parent[x] = parent[parent[x]]; x = parent[x]; }
        return x;
    }
    void unite(uint32_t a, uint32_t b) {
        a = find(a); b = find(b);
        if (a == b) return;
        if (a > b) std::swap(a, b);    // smaller index becomes root (lex-min repr)
        parent[b] = a; sz[a] += sz[b];
    }
    uint64_t size_of(uint32_t r) const { return sz[r]; }
};

// Emit every equivalent of band b reachable by one atomic move; `emit` does the
// normalize+lookup+union.  Keeping this in one place documents the move set.
template <class F>
inline void atomic_moves(const Band& b, F&& emit) {
    // R: row permutations (the two transpositions generate S3 over iterations)
    { Band t = b; t.swap_row(0, 1); emit(t); }
    { Band t = b; t.swap_row(0, 2); emit(t); }
    { Band t = b; t.swap_row(1, 2); emit(t); }
    // C: column permutations of B1
    { Band t = b; t.swap_col(0, 1); emit(t); }
    { Band t = b; t.swap_col(0, 2); emit(t); }
    { Band t = b; t.swap_col(1, 2); emit(t); }
    // B: stack permutations B1,B2,B3
    { Band t = b; t.swap_box(0, 1); emit(t); }
    { Band t = b; t.swap_box(0, 2); emit(t); }
    { Band t = b; t.swap_box(1, 2); emit(t); }

    // 2x2 rectangle: two columns x1,x2 carry the same pair of symbols in rows
    // y1,y2 but transposed -> relabel that pair.
    for (int x1 = 0; x1 < 8; ++x1)
        for (int y1 = 0; y1 < 2; ++y1)
            for (int x2 = x1 + 1; x2 < 9; ++x2)
                for (int y2 = y1 + 1; y2 < 3; ++y2)
                    if (b.v[y1][x1] == b.v[y2][x2] && b.v[y1][x2] == b.v[y2][x1]) {
                        Band t = b;
                        std::swap(t.v[y1][x1], t.v[y1][x2]);
                        std::swap(t.v[y2][x1], t.v[y2][x2]);
                        emit(t);
                    }
    // 2x3 rectangle: three columns carry a 3-cycle of symbols across the rows.
    for (int x1 = 0; x1 < 8; ++x1)
        for (int x2 = x1 + 1; x2 < 9; ++x2)
            if (b.v[0][x1] == b.v[1][x2] && b.v[1][x1] == b.v[2][x2] && b.v[2][x1] == b.v[0][x2]) {
                Band t = b;
                t.swap_col(x1, x2);
                emit(t);
            }
    // 3x2 rectangle: one column from each stack, two rows, 3-cycle of symbols.
    for (int x1 = 0; x1 < 3; ++x1)
        for (int x2 = 3; x2 < 6; ++x2)
            for (int x3 = 6; x3 < 9; ++x3)
                for (int y1 = 0; y1 < 2; ++y1)
                    for (int y2 = y1 + 1; y2 < 3; ++y2)
                        if (b.v[y1][x1] == b.v[y2][x2] &&
                            b.v[y1][x2] == b.v[y2][x3] &&
                            b.v[y1][x3] == b.v[y2][x1]) {
                            Band t = b;
                            std::swap(t.v[y1][x1], t.v[y2][x1]);
                            std::swap(t.v[y1][x2], t.v[y2][x2]);
                            std::swap(t.v[y1][x3], t.v[y2][x3]);
                            emit(t);
                        }
    // General row-subset move (subsumes 2x2 / 3x2 / 4x2 / ... nxk row swaps).
    // For two band rows y1,y2 and any column subset S, if the rows carry the
    // SAME SET of symbols across S, then swapping those entries is a product of
    // disjoint cyclic relabellings (one per cycle of the induced permutation of
    // symbols) and hence preserves the completion count.  Felgenhauer noted this
    // generalisation adds nothing beyond 2x2..4x2 (the complement identity
    // S <-> S^c plus a full row swap); we use it because it is unambiguous and
    // closes the relation to exactly 71 classes without any special cases.
    for (int S = 1; S < 512; ++S) {
        int m[3] = {0, 0, 0};
        for (int y = 0; y < 3; ++y)
            for (int x = 0; x < 9; ++x)
                if (S & (1 << x)) m[y] |= 1 << b.v[y][x];
        for (int y1 = 0; y1 < 2; ++y1)
            for (int y2 = y1 + 1; y2 < 3; ++y2)
                if (m[y1] == m[y2]) {
                    Band t = b;
                    for (int x = 0; x < 9; ++x)
                        if (S & (1 << x)) std::swap(t.v[y1][x], t.v[y2][x]);
                    emit(t);
                }
    }
}

inline std::vector<Class> reduce() {
    std::vector<Band> cat = generate();
    const size_t N = cat.size();                      // expected 36288

    // index by key, in lexicographic order so root == lex-min representative
    std::vector<std::string> keys(N);
    for (size_t i = 0; i < N; ++i) keys[i] = cat[i].key();
    std::vector<uint32_t> order(N);
    std::iota(order.begin(), order.end(), 0u);
    std::sort(order.begin(), order.end(),
              [&](uint32_t a, uint32_t b) { return keys[a] < keys[b]; });
    std::vector<Band> sorted(N);
    std::unordered_map<std::string, uint32_t> id;
    id.reserve(N * 2);
    for (size_t r = 0; r < N; ++r) {
        sorted[r] = cat[order[r]];
        id.emplace(keys[order[r]], (uint32_t)r);
    }

    DSU dsu(N);
    for (size_t i = 0; i < N; ++i) {
        atomic_moves(sorted[i], [&](Band t) {
            t.normalize();
            auto it = id.find(t.key());
            // normalized result is always a catalogue member
            dsu.unite((uint32_t)i, it->second);
        });
    }

    std::vector<Class> out;
    for (size_t i = 0; i < N; ++i)
        if (dsu.find((uint32_t)i) == i)
            out.push_back(Class{dsu.size_of((uint32_t)i), sorted[i].pretty(), sorted[i]});
    // already in lex order because i ascends with lex order
    return out;
}

} // namespace fj
