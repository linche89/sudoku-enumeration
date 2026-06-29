// reduce_main.cpp — print the 71-class job list (mult + representative).
#include "reduce.hpp"
#include <cstdio>
#include <cstdint>

int main() {
    auto classes = fj::reduce();
    uint64_t total = 0;
    for (auto& c : classes) total += c.mult;
    std::fprintf(stderr, "# classes=%zu  sum(mult)=%llu\n",
                 classes.size(), (unsigned long long)total);
    std::printf("# job list created by fj_reduce (independent reimplementation)\n");
    for (auto& c : classes)
        std::printf("./sudoku2 %5llu  %s\n",
                    (unsigned long long)c.mult, c.repr.c_str());
    return 0;
}
