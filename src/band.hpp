// band.hpp — representation and canonicalization of a Sudoku "top band".
//
// A top band is the upper three rows (rows 0..2) of a 9x9 grid, i.e. blocks
// B1,B2,B3.  Throughout we work with the first block B1 frozen to the
// canonical form
//        0 1 2          1 2 3   (displayed +1)
//        3 4 5    <-->  4 5 6
//        6 7 8          7 8 9
// so a band is fully described by columns 3..8 of its three rows (the
// contents of B2 and B3).  Digits are stored 0..8 internally and printed 1..9.
//
// This file is part of an independent reimplementation of the
// Felgenhauer–Jarvis (2005) Sudoku enumeration.  It mirrors the mathematics of
// Bertram Felgenhauer's sudoku_equiv.cc but is written from scratch.
#pragma once
#include <algorithm>
#include <string>
#include <cstdint>

namespace fj {

struct Band {
    int v[3][9];

    // Construct the canonical band whose B2/B3 are still empty (-1).
    Band() {
        for (int i = 0; i < 3; ++i) {
            for (int j = 0; j < 3; ++j) v[i][j] = 3 * i + j;   // B1 canonical
            for (int j = 3; j < 9; ++j) v[i][j] = -1;          // B2,B3 empty
        }
    }

    // ---- elementary symmetry operations -------------------------------------
    void swap_col(int a, int b) { for (int k = 0; k < 3; ++k) std::swap(v[k][a], v[k][b]); }
    void swap_row(int a, int b) { for (int k = 0; k < 9; ++k) std::swap(v[a][k], v[b][k]); }
    void swap_box(int a, int b) { for (int k = 0; k < 3; ++k) swap_col(a * 3 + k, b * 3 + k); }

    // Canonical form: (1) relabel so B1 reads 123/456/789, (2) sort the columns
    // of B2 and of B3 by their top entry, (3) put the lexicographically smaller
    // of B2,B3 first.  Any band maps to exactly one of the 36288 catalogue
    // entries under this normalization.
    void normalize() {
        int trans[9];
        for (int i = 0; i < 3; ++i)
            for (int j = 0; j < 3; ++j)
                trans[v[i][j]] = 3 * i + j;
        for (int i = 0; i < 3; ++i)
            for (int j = 0; j < 9; ++j)
                v[i][j] = trans[v[i][j]];

        if (v[0][3] > v[0][4]) swap_col(3, 4);     // sort B2 columns (bubble of 3)
        if (v[0][4] > v[0][5]) swap_col(4, 5);
        if (v[0][3] > v[0][4]) swap_col(3, 4);
        if (v[0][6] > v[0][7]) swap_col(6, 7);     // sort B3 columns
        if (v[0][7] > v[0][8]) swap_col(7, 8);
        if (v[0][6] > v[0][7]) swap_col(6, 7);
        if (v[0][3] > v[0][6]) swap_box(1, 2);     // order B2 before B3
    }

    // 18-character key of B2/B3 (rows 0..2, columns 3..8), digits '1'..'9'.
    std::string key() const {
        std::string s;
        s.reserve(18);
        for (int i = 0; i < 3; ++i)
            for (int j = 3; j < 9; ++j)
                s.push_back(char('1' + v[i][j]));
        return s;
    }

    // Human/oracle-compatible form, e.g. "[456789,789123,123456]".
    std::string pretty() const {
        std::string s = "[";
        for (int i = 0; i < 3; ++i) {
            if (i) s.push_back(',');
            for (int j = 3; j < 9; ++j) s.push_back(char('1' + v[i][j]));
        }
        s.push_back(']');
        return s;
    }
};

} // namespace fj
