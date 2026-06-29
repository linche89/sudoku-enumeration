#!/usr/bin/env bash
# Build the FJ05 Sudoku enumeration reproduction.
# NOTE: do NOT use -march=native on MinGW/Windows — it enables AVX whose 32-byte
# aligned moves fault against Windows' 16-byte stack guarantee.  The DFS is pure
# scalar bit-twiddling, so we request only the scalar bit instructions we use.
set -e
mkdir -p build
FLAGS="-O3 -mtune=native -mbmi -mbmi2 -mpopcnt -mlzcnt -funroll-loops -std=c++20 -pthread -Isrc"
g++ $FLAGS src/main.cpp        -o build/fj_sudoku.exe
g++ $FLAGS src/reduce_main.cpp -o build/fj_reduce.exe
g++ $FLAGS src/bench.cpp       -o build/bench.exe
echo "built: build/fj_sudoku.exe  build/fj_reduce.exe  build/bench.exe"
