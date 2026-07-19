#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <numeric>
#include <stdexcept>
#include <unordered_map>
#include <utility>
#include <vector>

#include <boost/multiprecision/cpp_int.hpp>

using boost::multiprecision::cpp_int;

namespace {
constexpr int MAX_C = 6;
constexpr int MAX_MASK = 1 << MAX_C;
using Hist = std::array<std::uint8_t, MAX_MASK>;
using Perm = std::array<std::uint8_t, MAX_C>;

struct HistHash {
    std::size_t operator()(Hist const& h) const noexcept {
        std::uint64_t x = 0x9e3779b97f4a7c15ULL;
        for (std::uint8_t v : h) {
            x ^= static_cast<std::uint64_t>(v) + 0x9e3779b97f4a7c15ULL +
                 (x << 6) + (x >> 2);
        }
        return static_cast<std::size_t>(x);
    }
};

struct Canonicalizer {
    explicit Canonicalizer(int c) : c(c), full_mask((1 << c) - 1) {
        for (int mask = 0; mask <= full_mask; ++mask) {
            masks_by_size[__builtin_popcount(static_cast<unsigned>(mask))].push_back(mask);
        }

        std::vector<int> p(c);
        std::iota(p.begin(), p.end(), 0);
        do {
            Perm q{};
            for (int i = 0; i < c; ++i) q[i] = static_cast<std::uint8_t>(p[i]);
            perms.push_back(q);

            std::array<std::uint8_t, MAX_MASK> image{};
            for (int mask = 0; mask <= full_mask; ++mask) {
                int mapped = 0;
                for (int a = 0; a < c; ++a) {
                    if (mask & (1 << a)) mapped |= 1 << q[a];
                }
                image[mask] = static_cast<std::uint8_t>(mapped);
            }
            mask_image.push_back(image);
        } while (std::next_permutation(p.begin(), p.end()));
    }

    Hist canonical(const Hist& h, int subset_size) const {
        Hist best{};
        bool first = true;
        auto const& active = masks_by_size[subset_size];

        for (auto const& image : mask_image) {
            Hist candidate{};
            for (int mask : active) candidate[image[mask]] = h[mask];

            bool less = false;
            bool greater = false;
            for (int mask : active) {
                if (candidate[mask] < best[mask]) { less = true; break; }
                if (candidate[mask] > best[mask]) { greater = true; break; }
            }
            if (first || (less && !greater)) {
                best = candidate;
                first = false;
            }
        }
        return best;
    }

    int c;
    int full_mask;
    std::array<std::vector<int>, MAX_C + 1> masks_by_size;
    std::vector<Perm> perms;
    std::vector<std::array<std::uint8_t, MAX_MASK>> mask_image;
};

std::uint64_t factorial(int n) {
    std::uint64_t z = 1;
    for (int i = 2; i <= n; ++i) z *= static_cast<std::uint64_t>(i);
    return z;
}

struct TransitionStats {
    int from_row{};
    std::size_t source_orbits{};
    std::size_t target_orbits{};
    std::uint64_t contingency_leaves{};
    std::uint64_t raw_target_occurrences{};
    std::size_t globally_distinct_raw_targets{};
    std::uint64_t nonzero_orbit_transitions{};
};

// For one source histogram h, enumerate the bounded contingency tables
// n_{S,a}.  Each source type S has row sum h_S; each color a has column
// sum 2; n_{S,a}=0 when a is already in S.  A complete table contributes
// prod_S h_S!/prod_a n_{S,a}! labeled assignments.
struct ContingencyEnumerator {
    explicit ContingencyEnumerator(int c, const Hist& source) : c(c) {
        residual.fill(2);
        for (int i = 0; i <= 2*c; ++i) fact[i] = factorial(i);
        for (int mask = 0; mask < (1 << c); ++mask) {
            if (source[mask] != 0) types.emplace_back(mask, source[mask]);
        }
        std::sort(types.begin(), types.end(), [&](auto x, auto y) {
            const int allowed_x = c - __builtin_popcount(static_cast<unsigned>(x.first));
            const int allowed_y = c - __builtin_popcount(static_cast<unsigned>(y.first));
            if (allowed_x != allowed_y) return allowed_x < allowed_y;
            return x.second > y.second;
        });
        raw_targets.reserve(4096);
    }

    void run() { recurse_types(0, 1); }

    void recurse_types(int type_index, std::uint64_t path_weight) {
        if (type_index == static_cast<int>(types.size())) {
            for (int a = 0; a < c; ++a) if (residual[a] != 0) return;
            raw_targets[target] += path_weight;
            ++leaves;
            return;
        }

        const auto [mask, multiplicity] = types[type_index];
        int available = 0;
        for (int a = 0; a < c; ++a) {
            if ((mask & (1 << a)) == 0) available += residual[a];
        }
        if (available < multiplicity) return;

        std::array<std::uint8_t, MAX_C> allocation{};
        enumerate_allocation(type_index, mask, multiplicity, 0, multiplicity,
                             1, path_weight, allocation);
    }

    void enumerate_allocation(int type_index, int mask, int multiplicity,
                              int color, int units_left,
                              std::uint64_t denominator,
                              std::uint64_t path_weight,
                              std::array<std::uint8_t, MAX_C>& allocation) {
        if (color == c) {
            if (units_left != 0) return;

            for (int a = 0; a < c; ++a) {
                residual[a] -= allocation[a];
                if (allocation[a] != 0) {
                    target[mask | (1 << a)] += allocation[a];
                }
            }

            const std::uint64_t multinomial = fact[multiplicity] / denominator;
            recurse_types(type_index + 1, path_weight * multinomial);

            for (int a = 0; a < c; ++a) {
                if (allocation[a] != 0) {
                    target[mask | (1 << a)] -= allocation[a];
                }
                residual[a] += allocation[a];
            }
            return;
        }

        if (mask & (1 << color)) {
            allocation[color] = 0;
            enumerate_allocation(type_index, mask, multiplicity, color + 1,
                                 units_left, denominator, path_weight, allocation);
            return;
        }

        int later_capacity = 0;
        for (int a = color + 1; a < c; ++a) {
            if ((mask & (1 << a)) == 0) later_capacity += residual[a];
        }
        const int lo = std::max(0, units_left - later_capacity);
        const int hi = std::min<int>(residual[color], units_left);
        for (int x = lo; x <= hi; ++x) {
            allocation[color] = static_cast<std::uint8_t>(x);
            enumerate_allocation(type_index, mask, multiplicity, color + 1,
                                 units_left - x, denominator * fact[x],
                                 path_weight, allocation);
        }
        allocation[color] = 0;
    }

    int c;
    std::vector<std::pair<int,int>> types;
    std::array<std::uint8_t, MAX_C> residual{};
    std::array<std::uint64_t, 2 * MAX_C + 1> fact{};
    Hist target{};
    std::unordered_map<Hist, std::uint64_t, HistHash> raw_targets;
    std::uint64_t leaves = 0;
};

struct Result {
    cpp_int ordered_permutation_arrays;
    cpp_int weighted_linear_sum;
    std::vector<std::size_t> orbit_counts;
    std::vector<cpp_int> layer_totals;
    std::vector<TransitionStats> transition_stats;
};

Result compute(int c) {
    Canonicalizer canon(c);
    std::unordered_map<Hist, cpp_int, HistHash> current, next;

    Hist start{};
    start[0] = static_cast<std::uint8_t>(2*c);
    current.emplace(canon.canonical(start, 0), cpp_int(1));

    Result result;
    result.orbit_counts.push_back(1);
    result.layer_totals.push_back(cpp_int(1));

    for (int r = 0; r < c; ++r) {
        next.clear();
        std::unordered_map<Hist, Hist, HistHash> canonical_cache;
        canonical_cache.reserve(60000);

        TransitionStats stats;
        stats.from_row = r;
        stats.source_orbits = current.size();

        for (auto const& [source, source_orbit_total] : current) {
            ContingencyEnumerator enumerator(c, source);
            enumerator.run();
            stats.contingency_leaves += enumerator.leaves;
            stats.raw_target_occurrences += enumerator.raw_targets.size();

            std::unordered_map<Hist, std::uint64_t, HistHash> orbit_transition;
            orbit_transition.reserve(enumerator.raw_targets.size());
            for (auto const& [raw_target, multiplicity] : enumerator.raw_targets) {
                auto [it, inserted] = canonical_cache.emplace(raw_target, Hist{});
                if (inserted) it->second = canon.canonical(raw_target, r + 1);
                orbit_transition[it->second] += multiplicity;
            }

            stats.nonzero_orbit_transitions += orbit_transition.size();
            for (auto const& [target, multiplicity] : orbit_transition) {
                next[target] += source_orbit_total * multiplicity;
            }
        }

        stats.globally_distinct_raw_targets = canonical_cache.size();
        current.swap(next);
        stats.target_orbits = current.size();
        result.transition_stats.push_back(stats);
        result.orbit_counts.push_back(current.size());
        cpp_int layer_total = 0;
        for (auto const& [state, weight] : current) {
            (void)state;
            layer_total += weight;
        }
        result.layer_totals.push_back(layer_total);
    }

    if (current.size() != 1) throw std::runtime_error("terminal orbit is not unique");
    result.ordered_permutation_arrays = current.begin()->second;
    result.weighted_linear_sum = result.ordered_permutation_arrays << (c*c);
    return result;
}
} // namespace

int main(int argc, char** argv) {
    try {
        const int c = (argc > 1) ? std::atoi(argv[1]) : 6;
        if (c < 1 || c > MAX_C) throw std::runtime_error("C must lie in [1,6]");

        const Result result = compute(c);
        std::cout << "C=" << c << "\n";
        std::cout << "orbit_counts_rows_0_to_C=";
        for (std::size_t i = 0; i < result.orbit_counts.size(); ++i) {
            if (i) std::cout << ',';
            std::cout << result.orbit_counts[i];
        }
        std::cout << "\n";
        std::cout << "layer_totals_rows_0_to_C=";
        for (std::size_t i = 0; i < result.layer_totals.size(); ++i) {
            if (i) std::cout << ',';
            std::cout << result.layer_totals[i];
        }
        std::cout << "\n";
        std::cout << "from_row,source_orbits,target_orbits,contingency_leaves,"
                     "raw_target_occurrences,globally_distinct_raw_targets,"
                     "nonzero_orbit_transitions\n";
        for (auto const& s : result.transition_stats) {
            std::cout << s.from_row << ',' << s.source_orbits << ',' << s.target_orbits << ','
                      << s.contingency_leaves << ',' << s.raw_target_occurrences << ','
                      << s.globally_distinct_raw_targets << ','
                      << s.nonzero_orbit_transitions << "\n";
        }
        std::cout << "A_C=" << result.ordered_permutation_arrays << "\n";
        std::cout << "N_linear_C=2^(C^2)*A_C=" << result.weighted_linear_sum << "\n";
        return 0;
    } catch (std::exception const& e) {
        std::cerr << "error: " << e.what() << "\n";
        return 1;
    }
}
