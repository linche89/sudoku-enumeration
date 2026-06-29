// bench.cpp — single-representative timing + correctness probe.
#include "count.hpp"
#include <chrono>
#include <cstdio>
using namespace fj;

int main(int argc, char** argv) {
    int idx = (argc > 1) ? std::atoi(argv[1]) : 0;
    auto classes = reduce();
    State base; base_state(classes[idx].band, base);
    auto t0 = std::chrono::steady_clock::now();
    uint64_t sol = 0;
    for (int h = 0; h < 10; ++h) sol += count_branch(base, h);
    auto t1 = std::chrono::steady_clock::now();
    double s = std::chrono::duration<double>(t1 - t0).count();
    std::printf("class %d  repr %s\n", idx, classes[idx].repr.c_str());
    std::printf("sol = %llu   time = %.3f s   leaves/s = %.2f M\n",
                (unsigned long long)sol, s, sol / s / 1e6);
    return 0;
}
