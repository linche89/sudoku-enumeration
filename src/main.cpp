// main.cpp — FJ05 Sudoku grid enumeration, end to end.
//
//   Phase 1  freeze block B1                       -> factor 9!
//   Phase 2  enumerate the 36288 reduced bands       (generate)
//   Phase 3  collapse to 71 equivalence classes        (reduce)
//   Phase 4  parallel bitwise DFS over each class       (count_all)
//   Phase 5  aggregate  N0 = 9! * 72^2 * sum(mult*sol)
//
// Usage:
//   fj_sudoku [--threads N] [--emit-jobs] [--ref DIR] [--quiet]
#include "count.hpp"
#include <chrono>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <map>
#include <string>

using namespace fj;

static std::map<std::string, uint64_t> parse_pairs(const std::string& path, bool results) {
    // jobs2.txt:    ./sudoku2  <mult>  [repr]
    // results2.txt: [repr]: <mult> * <sol>
    std::map<std::string, uint64_t> out;
    std::ifstream in(path);
    if (!in) return out;
    std::string line;
    while (std::getline(in, line)) {
        if (line.empty() || line[0] == '#') continue;
        auto lb = line.find('['), rb = line.find(']');
        if (lb == std::string::npos || rb == std::string::npos) continue;
        std::string repr = line.substr(lb, rb - lb + 1);
        uint64_t value = 0;
        try {
            if (results) {
                auto star = line.find('*', rb);
                if (star == std::string::npos) continue;
                value = std::stoull(line.substr(star + 1));        // the sol count
            } else {
                // the multiplicity is the digit run immediately before '['
                size_t e = lb; while (e > 0 && !std::isdigit((unsigned char)line[e - 1])) --e;
                size_t s = e; while (s > 0 && std::isdigit((unsigned char)line[s - 1])) --s;
                if (s >= e) continue;
                value = std::stoull(line.substr(s, e - s));
            }
        } catch (...) { continue; }
        out[repr] = value;
    }
    return out;
}

int main(int argc, char** argv) {
    int nthreads = (int)std::thread::hardware_concurrency();
    if (nthreads <= 0) nthreads = 4;
    bool emit_jobs = false, quiet = false;
    std::string ref = "reference";
    int limit = 0;   // debug: count only the first `limit` classes (0 = all)
    for (int a = 1; a < argc; ++a) {
        if (!std::strcmp(argv[a], "--threads") && a + 1 < argc) nthreads = std::atoi(argv[++a]);
        else if (!std::strcmp(argv[a], "--emit-jobs")) emit_jobs = true;
        else if (!std::strcmp(argv[a], "--ref") && a + 1 < argc) ref = argv[++a];
        else if (!std::strcmp(argv[a], "--limit") && a + 1 < argc) limit = std::atoi(argv[++a]);
        else if (!std::strcmp(argv[a], "--quiet")) quiet = true;
    }

    auto wall0 = std::chrono::steady_clock::now();

    // -------- Phases 1-3: reduction ----------------------------------------
    auto classes = reduce();
    uint64_t summ = 0;
    for (auto& c : classes) summ += c.mult;

    if (emit_jobs) {
        std::printf("# job list created by fj_sudoku (independent reimplementation)\n");
        for (auto& c : classes)
            std::printf("./sudoku2 %5llu  %s\n", (unsigned long long)c.mult, c.repr.c_str());
        return 0;
    }

    // -------- Phases 4-5: counting + aggregation ---------------------------
    if (limit > 0 && limit < (int)classes.size()) classes.resize(limit);  // debug smoke test
    Result R = count_all(classes, nthreads);
    auto wall1 = std::chrono::steady_clock::now();
    double wall = std::chrono::duration<double>(wall1 - wall0).count();

    // -------- verification --------------------------------------------------
    const unsigned __int128 KNOWN_N0 =
        (unsigned __int128)18383222420692992ull * 362880ull;     // = 6670903752021072936960
    const uint64_t KNOWN_N1 = 18383222420692992ull;
    const uint64_t KNOWN_S  = 3546146300288ull;

    auto ref_jobs = parse_pairs(ref + "/jobs2.txt", false);
    auto ref_res  = parse_pairs(ref + "/results2.txt", true);

    int mult_match = 0, sol_match = 0, mult_tot = 0, sol_tot = 0;
    for (auto& c : classes) {
        if (auto it = ref_jobs.find(c.repr); it != ref_jobs.end()) {
            ++mult_tot; if (it->second == c.mult) ++mult_match;
        }
    }
    for (size_t i = 0; i < classes.size(); ++i) {
        if (auto it = ref_res.find(classes[i].repr); it != ref_res.end()) {
            ++sol_tot; if (it->second == R.sol[i]) ++sol_match;
        }
    }

    if (!quiet) {
        std::printf("================ FJ05 Sudoku enumeration ================\n");
        std::printf("threads                : %d\n", nthreads);
        std::printf("equivalence classes    : %zu  (expected 71)\n", classes.size());
        std::printf("sum of multiplicities  : %llu  (expected 36288)\n", (unsigned long long)summ);
        std::printf("--------------------------------------------------------\n");
        std::printf("S  = sum(mult*sol)     : %llu\n", (unsigned long long)R.S);
        std::printf("                expected: %llu  [%s]\n", (unsigned long long)KNOWN_S,
                    R.S == KNOWN_S ? "OK" : "MISMATCH");
        std::printf("N1 = 72^2 * S          : %llu\n", (unsigned long long)R.N1);
        std::printf("                expected: %llu  [%s]\n", (unsigned long long)KNOWN_N1,
                    R.N1 == KNOWN_N1 ? "OK" : "MISMATCH");
        std::printf("N0 = 9!  * N1          : %s\n", u128_to_string(R.N0).c_str());
        std::printf("                expected: %s  [%s]\n", u128_to_string(KNOWN_N0).c_str(),
                    R.N0 == KNOWN_N0 ? "OK" : "MISMATCH");
        std::printf("--------------------------------------------------------\n");
        std::printf("cross-check vs jobs2.txt   : %d/%d multiplicities match\n", mult_match, mult_tot);
        std::printf("cross-check vs results2.txt: %d/%d solution counts match\n", sol_match, sol_tot);
        std::printf("--------------------------------------------------------\n");
        std::printf("wall-clock time        : %.3f s  (%d threads)\n", wall, nthreads);
        std::printf("=========================================================\n");
    }

    bool ok = classes.size() == 71 && summ == 36288 && R.S == KNOWN_S &&
              R.N1 == KNOWN_N1 && R.N0 == KNOWN_N0 &&
              (mult_tot == 0 || mult_match == mult_tot) &&
              (sol_tot == 0 || sol_match == sol_tot);
    if (!quiet) std::printf("RESULT: %s\n", ok ? "ALL CHECKS PASSED" : "FAILURE");
    return ok ? 0 : 1;
}
