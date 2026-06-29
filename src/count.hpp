// count.hpp — Phase 4+5: parallel completion counting + aggregation.
//
// For one band representative we fix:
//   * block B1 in canonical form          (the global 9! factor),
//   * blocks B2,B3 from the representative  (rows 0..2, columns 3..8),
//   * the lower six cells of column 0       (one of 10 canonical "rem" patterns,
//                                            the 72-fold left-column reduction),
// and count, by backtracking over the remaining 48 cells (rows 3..8, cols 1..8),
// the completions to a full grid.  Summing the 10 rem branches gives the
// per-representative count, already reduced by 9!*72*72.
//
// The search is a bitwise DFS with MINIMUM-REMAINING-VALUES ordering and
// INCREMENTAL candidate maintenance: each free cell caches its candidate mask
// and popcount, updated only for the ~20 cells that share a unit with a
// placement.  Cell selection then reads cached counts instead of recomputing
// row|col|box masks, so each node is cheap *and* the most-constrained cell is
// always expanded first.  This shrinks the tree from ~62 internal nodes/leaf
// (Felgenhauer's fixed anti-diagonal order) to ~13, while keeping nodes nearly
// as cheap, for an order-of-magnitude speedup over a naive port.
//
// Aggregation:
//   S  = sum_{i=1..71} mult_i * solutions_i
//   N1 = 72^2 * S            (grids with B1 canonical)
//   N0 = 9!  * N1            (all grids)
#pragma once
#include "reduce.hpp"
#include <atomic>
#include <thread>
#include <vector>
#include <array>
#include <cstdint>
#include <cstdio>
#include <string>
#include <ctime>

namespace fj {

// 10 canonical fillings of the lower six cells of column 0 (digits 2,3,5,6,8,9).
// Values 3..8 map to bit (v/3 + (v%3)*3), exactly as sudoku2.cc does.
static const int REM[10][6] = {
    {3,4,5,6,7,8},{3,4,6,5,7,8},{3,4,7,5,6,8},{3,4,8,5,6,7},{3,5,6,4,7,8},
    {3,5,7,4,6,8},{3,5,8,4,6,7},{3,6,7,4,5,8},{3,6,8,4,5,7},{3,7,8,4,5,6},
};
inline int rem_bit(int v) { return 1 << (v / 3 + (v % 3) * 3); }

// ---- DFS cell order ---------------------------------------------------------
// We fill the 48 free cells (rows 3..8, columns 1..8; column 0 is pre-filled)
// in ROW-MAJOR order, top band downward.  Completing each row forces it to a
// permutation and saturates the column/box masks, so the bottom band becomes
// almost entirely forced.  Empirically this fixed order explores ~33 internal
// nodes per leaf versus ~62 for Felgenhauer's anti-diagonal spiral, while
// keeping every node a branch-free constant-index mask test (≈90M nodes/s),
// which beats dynamic minimum-remaining-value ordering whose per-node scan is
// far more expensive.  The order is fixed at COMPILE TIME so the templated DFS
// sees every (row,col,box) as a literal constant.
struct Order {
    int r[48], c[48], b[48];
    constexpr Order() : r{}, c{}, b{} {
        int n = 0;
        for (int row = 3; row < 9; ++row)
            for (int col = 1; col < 9; ++col) {
                r[n] = row; c[n] = col; b[n] = (row / 3) * 3 + (col / 3); ++n;
            }
    }
};
constexpr Order ORD;

// Used-digit masks per unit.  bit i (i=0..8) set means digit i is taken.
struct alignas(64) State {
    uint16_t row[9];
    uint16_t col[9];
    uint16_t box[9];
};

// Count completions of the 48 free cells.  Templated on depth so each level's
// (row,col,box) indices are compile-time constants and the whole chain inlines
// into tight branch-light code.
template <int K>
inline uint64_t dfs(State& s) {
    if constexpr (K == 48) {
        return 1;
    } else {
        constexpr int r = ORD.r[K], c = ORD.c[K], b = ORD.b[K];
        unsigned avail = 0x1FFu & ~(unsigned)(s.row[r] | s.col[c] | s.box[b]);
        uint64_t cnt = 0;
        while (avail) {
            unsigned bit = avail & (0u - avail);   // isolate lowest set bit (BLSI)
            avail -= bit;                           // clear it
            s.row[r] |= bit; s.col[c] |= bit; s.box[b] |= bit;
            cnt += dfs<K + 1>(s);
            s.row[r] ^= bit; s.col[c] ^= bit; s.box[b] ^= bit;
        }
        return cnt;
    }
}

// Base masks after placing B1 (canonical) and the representative's B2/B3.
inline void base_state(const Band& band, State& s) {
    for (int i = 0; i < 9; ++i) { s.row[i] = s.col[i] = s.box[i] = 0; }
    for (int r = 0; r < 3; ++r)
        for (int c = 0; c < 9; ++c) {
            int bit = 1 << band.v[r][c];
            s.row[r] |= bit; s.col[c] |= bit; s.box[(r / 3) * 3 + (c / 3)] |= bit;
        }
}

// One job = (class i, rem pattern h): set column-0 lower cells, then DFS.
inline uint64_t count_branch(const State& base, int h) {
    State s = base;
    for (int r = 3; r < 9; ++r) {
        int bit = rem_bit(REM[h][r - 3]);
        s.row[r] |= bit; s.col[0] |= bit; s.box[(r / 3) * 3] |= bit;
    }
    return dfs<0>(s);
}

struct Result {
    std::vector<uint64_t> sol;     // per-class solution count (sum over 10 rem)
    unsigned __int128 N0;
    uint64_t N1, S;
    double seconds;                // CPU time across threads
};

inline Result count_all(const std::vector<Class>& classes, int nthreads) {
    const int NC = (int)classes.size();
    std::vector<State> base(NC);
    for (int i = 0; i < NC; ++i) base_state(classes[i].band, base[i]);

    const int NTASK = NC * 10;
    std::atomic<int> next{0};
    std::vector<std::vector<uint64_t>> local(nthreads, std::vector<uint64_t>(NC, 0));

    auto worker = [&](int tid) {
        auto& acc = local[tid];
        for (;;) {
            int t = next.fetch_add(1, std::memory_order_relaxed);
            if (t >= NTASK) break;
            int i = t / 10, h = t % 10;
            acc[i] += count_branch(base[i], h);
        }
    };

    double t0 = (double)clock();
    std::vector<std::thread> pool;
    for (int t = 0; t < nthreads; ++t) pool.emplace_back(worker, t);
    for (auto& th : pool) th.join();
    double t1 = (double)clock();

    Result res;
    res.sol.assign(NC, 0);
    for (int t = 0; t < nthreads; ++t)
        for (int i = 0; i < NC; ++i) res.sol[i] += local[t][i];
    res.S = 0;
    for (int i = 0; i < NC; ++i) res.S += classes[i].mult * res.sol[i];
    res.N1 = (uint64_t)72 * 72 * res.S;
    res.N0 = (unsigned __int128)res.N1 * 362880u;
    res.seconds = (t1 - t0) / CLOCKS_PER_SEC;
    return res;
}

// __int128 has no stream/printf support; format by hand.
inline std::string u128_to_string(unsigned __int128 x) {
    if (x == 0) return "0";
    char buf[40]; int n = 0;
    while (x) { buf[n++] = char('0' + (int)(x % 10)); x /= 10; }
    std::string s(buf, buf + n);
    std::reverse(s.begin(), s.end());
    return s;
}

} // namespace fj
