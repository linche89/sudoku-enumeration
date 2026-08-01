// layer_penultimate_burnside.cpp -- exact orbit count for layer C-1.
//
// A native penultimate-layer state has 2C symbol masks.  Every mask misses
// exactly one coordinate pair, and exactly two masks miss each pair.  Group
// those two masks into an unordered local pair.  Each local mask is then a
// binary word on the other C-1 pairs, and the native degree condition says
// that every coordinate has exactly C-1 zero-side and C-1 one-side uses.
//
// The coordinate group C2 wr S_C acts directly on these C local pairs.  For
// each signed permutation, its fixed tuples are enumerated cycle by cycle and
// joined by a small degree-vector DP.  Burnside's lemma then gives the exact
// number M_(C-1) without constructing any earlier layer.

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

constexpr int MAX_C = 6;
using u64 = std::uint64_t;
using u128 = unsigned __int128;
using Degree = std::array<std::uint8_t, MAX_C>;

struct GroupElement {
    std::array<std::uint8_t, MAX_C> perm{};
    unsigned flips = 0;
};

struct SignedClass {
    GroupElement representative{};
    u64 size = 0;
};

struct LocalPair {
    std::uint16_t a = 0;
    std::uint16_t b = 0;

    bool operator==(const LocalPair&) const = default;
};

std::string u128_string(u128 value) {
    if (!value) return "0";
    std::string out;
    while (value) {
        out.push_back(char('0' + value % 10));
        value /= 10;
    }
    std::reverse(out.begin(), out.end());
    return out;
}

u64 factorial(int n) {
    u64 out = 1;
    for (int i = 2; i <= n; ++i) out *= (u64)i;
    return out;
}

u64 signed_cycle_signature(const GroupElement& g, int C) {
    std::array<std::uint8_t, MAX_C + 1> positive{};
    std::array<std::uint8_t, MAX_C + 1> negative{};
    std::array<bool, MAX_C> seen{};
    for (int start = 0; start < C; ++start) {
        if (seen[start]) continue;
        int length = 0;
        int parity = 0;
        int p = start;
        while (!seen[p]) {
            seen[p] = true;
            ++length;
            parity ^= (g.flips >> p) & 1u;
            p = g.perm[p];
        }
        (parity ? negative : positive)[length]++;
    }

    // Every digit is at most C, so base C+1 is collision-free.
    u64 signature = 0;
    for (int length = 1; length <= C; ++length) {
        signature = signature * (u64)(C + 1) + positive[length];
        signature = signature * (u64)(C + 1) + negative[length];
    }
    return signature;
}

std::map<u64, SignedClass> signed_conjugacy_classes(int C) {
    std::array<std::uint8_t, MAX_C> perm{};
    for (int i = 0; i < MAX_C; ++i) perm[i] = (std::uint8_t)i;

    std::map<u64, SignedClass> classes;
    do {
        for (unsigned flips = 0; flips < (1u << C); ++flips) {
            GroupElement g{perm, flips};
            const u64 signature = signed_cycle_signature(g, C);
            auto [it, inserted] = classes.try_emplace(signature);
            if (inserted) it->second.representative = g;
            ++it->second.size;
        }
    } while (std::next_permutation(perm.begin(), perm.begin() + C));
    return classes;
}

LocalPair transform_pair(const LocalPair& pair, int missing,
                         const GroupElement& g, int C) {
    const int outputMissing = g.perm[missing];
    auto transform_word = [&](std::uint16_t word) {
        std::uint16_t out = 0;
        for (int q = 0; q < C; ++q) {
            if (q == missing) continue;
            const int oq = g.perm[q];
            const unsigned bit = ((word >> q) & 1u) ^
                                 ((g.flips >> oq) & 1u);
            out |= (std::uint16_t)(bit << oq);
        }
        out &= (std::uint16_t)~(1u << outputMissing);
        return out;
    };

    LocalPair out{transform_word(pair.a), transform_word(pair.b)};
    if (out.b < out.a) std::swap(out.a, out.b);
    return out;
}

void checked_add(std::map<Degree, u64>& target, const Degree& key,
                 u128 contribution) {
    const u128 value = (u128)target[key] + contribution;
    if (value > UINT64_MAX) throw std::overflow_error("fixed-count overflow");
    target[key] = (u64)value;
}

u64 fixed_penultimate_states(const GroupElement& g, int C) {
    std::array<bool, MAX_C> seen{};
    std::vector<std::vector<int>> cycles;
    for (int start = 0; start < C; ++start) {
        if (seen[start]) continue;
        std::vector<int> cycle;
        int p = start;
        while (!seen[p]) {
            seen[p] = true;
            cycle.push_back(p);
            p = g.perm[p];
        }
        cycles.push_back(std::move(cycle));
    }

    const int targetDegree = C - 1;
    std::vector<std::map<Degree, u64>> cycleBlocks;
    cycleBlocks.reserve(cycles.size());

    for (const auto& cycle : cycles) {
        const int start = cycle.front();
        std::vector<std::uint16_t> words;
        for (unsigned word = 0; word < (1u << C); ++word) {
            if (((word >> start) & 1u) == 0)
                words.push_back((std::uint16_t)word);
        }

        std::map<Degree, u64> blocks;
        for (size_t ia = 0; ia < words.size(); ++ia) {
            for (size_t ib = ia; ib < words.size(); ++ib) {
                const LocalPair seed{words[ia], words[ib]};
                LocalPair pair = seed;
                int missing = start;
                Degree degree{};

                for (size_t step = 0; step < cycle.size(); ++step) {
                    for (std::uint16_t word : {pair.a, pair.b}) {
                        for (int q = 0; q < C; ++q) {
                            if (q != missing)
                                degree[q] += (word >> q) & 1u;
                        }
                    }
                    pair = transform_pair(pair, missing, g, C);
                    missing = g.perm[missing];
                }

                if (missing != start || !(pair == seed)) continue;
                bool admissible = true;
                for (int q = 0; q < C; ++q)
                    admissible &= degree[q] <= targetDegree;
                if (admissible) ++blocks[degree];
            }
        }
        cycleBlocks.push_back(std::move(blocks));
    }

    std::map<Degree, u64> dp;
    dp[Degree{}] = 1;
    for (const auto& blocks : cycleBlocks) {
        std::map<Degree, u64> next;
        for (const auto& [have, ways] : dp) {
            for (const auto& [add, blockWays] : blocks) {
                Degree joined{};
                bool admissible = true;
                for (int q = 0; q < C; ++q) {
                    const int value = have[q] + add[q];
                    if (value > targetDegree) {
                        admissible = false;
                        break;
                    }
                    joined[q] = (std::uint8_t)value;
                }
                if (admissible)
                    checked_add(next, joined, (u128)ways * blockWays);
            }
        }
        dp = std::move(next);
    }

    Degree target{};
    for (int q = 0; q < C; ++q) target[q] = (std::uint8_t)targetDegree;
    const auto found = dp.find(target);
    return found == dp.end() ? 0 : found->second;
}

u64 known_small_count(int C) {
    switch (C) {
        case 2: return 1;
        case 3: return 5;
        case 4: return 54;
        case 5: return 17120;
        default: return 0;
    }
}

u64 count_penultimate_orbits(int C) {
    const auto started = std::chrono::steady_clock::now();
    const auto classes = signed_conjugacy_classes(C);
    const u64 groupOrder = (1ULL << C) * factorial(C);
    u64 representedGroup = 0;
    u128 burnsideSum = 0;

    for (const auto& [signature, cls] : classes) {
        (void)signature;
        representedGroup += cls.size;
        const u64 fixed = fixed_penultimate_states(cls.representative, C);
        burnsideSum += (u128)cls.size * fixed;
    }
    if (representedGroup != groupOrder)
        throw std::runtime_error("signed conjugacy classes do not cover group");
    if (burnsideSum % groupOrder)
        throw std::runtime_error("Burnside sum is not divisible by group order");
    const u128 result128 = burnsideSum / groupOrder;
    if (result128 > UINT64_MAX)
        throw std::overflow_error("orbit count does not fit uint64");
    const u64 result = (u64)result128;
    const u64 expected = known_small_count(C);
    if (expected && result != expected)
        throw std::runtime_error("small-C penultimate-layer gate failed");

    const double seconds = std::chrono::duration<double>(
        std::chrono::steady_clock::now() - started).count();
    std::printf("penultimate C=%d layer=%d signed_classes=%zu group=%llu "
                "burnside_sum=%s M_%d=%llu",
                C, C - 1, classes.size(),
                (unsigned long long)groupOrder,
                u128_string(burnsideSum).c_str(), C - 1,
                (unsigned long long)result);
    if (expected) std::printf(" expected=%llu [OK]", (unsigned long long)expected);
    std::printf(" time=%.3fs\n", seconds);
    return result;
}

}  // namespace

int main(int argc, char** argv) {
    try {
        if (argc > 2) {
            std::fprintf(stderr, "usage: %s [C]\n", argv[0]);
            return 2;
        }
        if (argc == 2) {
            const int C = std::atoi(argv[1]);
            if (C < 2 || C > MAX_C) {
                std::fprintf(stderr, "C must be in 2..6\n");
                return 2;
            }
            count_penultimate_orbits(C);
        } else {
            for (int C = 2; C <= MAX_C; ++C) count_penultimate_orbits(C);
        }
        std::printf("PENULTIMATE-LAYER BURNSIDE GATES PASSED\n");
        return 0;
    } catch (const std::exception& e) {
        std::fprintf(stderr, "FATAL: %s\n", e.what());
        return 1;
    }
}
