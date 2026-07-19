#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <numeric>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include <boost/multiprecision/cpp_int.hpp>

using boost::multiprecision::cpp_int;

namespace {

constexpr int MAX_N = 6;
using Matrix = std::array<std::uint8_t, MAX_N * MAX_N>;
using Perm = std::array<std::uint8_t, MAX_N>;

struct Canonicalizer {
    explicit Canonicalizer(int n) : n_(n) {
        if (n_ < 1 || n_ > MAX_N) {
            throw std::runtime_error("n must lie in [1,6]");
        }
        pow3_[0] = 1;
        for (int i = 1; i <= MAX_N * MAX_N; ++i) {
            pow3_[i] = 3ULL * pow3_[i - 1];
        }
        column_base_ = pow3_[n_];

        std::vector<int> p(n_);
        std::iota(p.begin(), p.end(), 0);
        do {
            Perm q{};
            for (int i = 0; i < n_; ++i) q[i] = static_cast<std::uint8_t>(p[i]);
            permutations_.push_back(q);
        } while (std::next_permutation(p.begin(), p.end()));
    }

    const std::vector<Perm>& permutations() const { return permutations_; }

    std::uint64_t raw_code(const Matrix& a) const {
        std::uint64_t code = 0;
        std::uint64_t place = 1;
        for (int i = 0; i < n_ * n_; ++i) {
            code += static_cast<std::uint64_t>(a[i]) * place;
            place *= 3ULL;
        }
        return code;
    }

    // Canonical form for the action A -> P A Q of S_n x S_n.
    // For each row permutation, lexicographically sorting the column vectors
    // gives the least representative over all column permutations. We then
    // take the least of those n! representatives.
    std::uint64_t canonical_code(const Matrix& a) {
        const std::uint64_t raw = raw_code(a);
        if (auto it = cache_.find(raw); it != cache_.end()) return it->second;

        bool first = true;
        std::array<std::uint16_t, MAX_N> best{};

        for (const Perm& row_perm : permutations_) {
            std::array<std::uint16_t, MAX_N> columns{};
            for (int c = 0; c < n_; ++c) {
                std::uint16_t code = 0;
                for (int i = 0; i < n_; ++i) {
                    code = static_cast<std::uint16_t>(3U * code + a[row_perm[i] * n_ + c]);
                }
                columns[c] = code;
            }
            std::sort(columns.begin(), columns.begin() + n_);
            if (first || std::lexicographical_compare(
                    columns.begin(), columns.begin() + n_,
                    best.begin(), best.begin() + n_)) {
                best = columns;
                first = false;
            }
        }

        std::uint64_t code = 0;
        for (int c = 0; c < n_; ++c) code = code * column_base_ + best[c];
        cache_.emplace(raw, code);
        return code;
    }

    Matrix decode_canonical(std::uint64_t code) const {
        std::array<std::uint16_t, MAX_N> columns{};
        for (int c = n_ - 1; c >= 0; --c) {
            columns[c] = static_cast<std::uint16_t>(code % column_base_);
            code /= column_base_;
        }
        Matrix a{};
        for (int c = 0; c < n_; ++c) {
            std::uint16_t v = columns[c];
            for (int r = n_ - 1; r >= 0; --r) {
                a[r * n_ + c] = static_cast<std::uint8_t>(v % 3U);
                v /= 3U;
            }
        }
        return a;
    }

    void clear_cache() { cache_.clear(); }
    std::size_t cache_size() const { return cache_.size(); }

private:
    int n_;
    std::uint64_t column_base_{};
    std::uint64_t pow3_[MAX_N * MAX_N + 1]{};
    std::vector<Perm> permutations_;
    std::unordered_map<std::uint64_t, std::uint64_t> cache_;
};

std::uint64_t factorial_u64(int n) {
    std::uint64_t x = 1;
    for (int i = 2; i <= n; ++i) x *= static_cast<std::uint64_t>(i);
    return x;
}

std::uint64_t orbit_size(const Matrix& a, int n, const std::vector<Perm>& perms) {
    // |Aut(A)| = sum over row permutations preserving the multiset of columns
    // of product_v m_v!, where m_v are column-vector multiplicities.
    auto column_multiset = [&](const Matrix& m, const Perm* rp) {
        std::array<std::uint16_t, MAX_N> cols{};
        for (int c = 0; c < n; ++c) {
            std::uint16_t code = 0;
            for (int r = 0; r < n; ++r) {
                const int rr = rp ? (*rp)[r] : r;
                code = static_cast<std::uint16_t>(3U * code + m[rr * n + c]);
            }
            cols[c] = code;
        }
        std::sort(cols.begin(), cols.begin() + n);
        return cols;
    };

    const auto original = column_multiset(a, nullptr);
    std::uint64_t aut = 0;
    for (const Perm& rp : perms) {
        const auto transformed = column_multiset(a, &rp);
        if (!std::equal(original.begin(), original.begin() + n, transformed.begin())) continue;

        std::uint64_t ways = 1;
        for (int i = 0; i < n;) {
            int j = i + 1;
            while (j < n && original[j] == original[i]) ++j;
            ways *= factorial_u64(j - i);
            i = j;
        }
        aut += ways;
    }
    const std::uint64_t group_order = factorial_u64(n) * factorial_u64(n);
    assert(group_order % aut == 0);
    return group_order / aut;
}

Matrix complement(const Matrix& a, int n) {
    Matrix b{};
    for (int i = 0; i < n * n; ++i) b[i] = static_cast<std::uint8_t>(2 - a[i]);
    return b;
}

struct LayerStats {
    int from_k{};
    std::size_t source_orbits{};
    std::size_t target_orbits{};
    std::uint64_t valid_permutation_trials{};
    std::uint64_t nonzero_orbit_transitions{};
    std::size_t unique_raw_targets{};
};

struct Result {
    cpp_int ordered_decompositions;
    cpp_int weighted_linear_sum;
    std::vector<std::size_t> orbit_counts;
    std::vector<LayerStats> layer_stats;
};

Result compute(int n) {
    Canonicalizer canon(n);
    const auto& perms = canon.permutations();

    std::unordered_map<std::uint64_t, cpp_int> current, next;
    Matrix zero{};
    current.emplace(canon.canonical_code(zero), cpp_int(1));

    Result result;
    result.orbit_counts.push_back(1);

    // Only advance to the balanced midpoint k=n. The second half is paired by
    // A <-> 2J-A, giving an exact meet-in-the-middle contraction.
    for (int k = 0; k < n; ++k) {
        next.clear();
        next.reserve(current.size() * 4 + 32);
        canon.clear_cache();

        LayerStats stats;
        stats.from_k = k;
        stats.source_orbits = current.size();

        for (const auto& [source_code, source_weight] : current) {
            const Matrix source = canon.decode_canonical(source_code);
            std::unordered_map<std::uint64_t, std::uint16_t> target_multiplicity;
            target_multiplicity.reserve(perms.size());

            for (const Perm& p : perms) {
                bool legal = true;
                for (int r = 0; r < n; ++r) {
                    if (source[r * n + p[r]] == 2) {
                        legal = false;
                        break;
                    }
                }
                if (!legal) continue;

                Matrix target = source;
                for (int r = 0; r < n; ++r) ++target[r * n + p[r]];
                const std::uint64_t target_code = canon.canonical_code(target);
                ++target_multiplicity[target_code];
                ++stats.valid_permutation_trials;
            }

            stats.nonzero_orbit_transitions += target_multiplicity.size();
            for (const auto& [target_code, multiplicity] : target_multiplicity) {
                next[target_code] += source_weight * multiplicity;
            }
        }

        stats.target_orbits = next.size();
        stats.unique_raw_targets = canon.cache_size();
        result.layer_stats.push_back(stats);
        current.swap(next);
        result.orbit_counts.push_back(current.size());
    }

    cpp_int midpoint_sum = 0;
    for (const auto& [code, orbit_total] : current) {
        const Matrix a = canon.decode_canonical(code);
        const std::uint64_t os = orbit_size(a, n, perms);
        assert(orbit_total % os == 0);
        const cpp_int per_labeled_state = orbit_total / os;

        const Matrix comp = complement(a, n);
        const std::uint64_t comp_code = canon.canonical_code(comp);
        const auto it = current.find(comp_code);
        if (it == current.end()) throw std::runtime_error("complement midpoint orbit missing");
        assert(it->second % os == 0); // complement has the same orbit size
        const cpp_int comp_per_labeled_state = it->second / os;

        midpoint_sum += cpp_int(os) * per_labeled_state * comp_per_labeled_state;
    }

    result.ordered_decompositions = midpoint_sum;
    result.weighted_linear_sum = midpoint_sum << (n * n); // multiply by 2^(n^2)
    return result;
}

} // namespace

int main(int argc, char** argv) {
    try {
        const int n = (argc >= 2) ? std::atoi(argv[1]) : 6;
        const Result result = compute(n);

        std::cout << "C=" << n << "\n";
        std::cout << "orbit_counts_k_0_to_C=";
        for (std::size_t i = 0; i < result.orbit_counts.size(); ++i) {
            if (i) std::cout << ',';
            std::cout << result.orbit_counts[i];
        }
        std::cout << "\n";
        std::cout << "from_k,source_orbits,target_orbits,valid_perm_trials,nonzero_orbit_transitions,unique_raw_targets\n";
        for (const LayerStats& s : result.layer_stats) {
            std::cout << s.from_k << ',' << s.source_orbits << ',' << s.target_orbits << ','
                      << s.valid_permutation_trials << ',' << s.nonzero_orbit_transitions << ','
                      << s.unique_raw_targets << "\n";
        }
        std::cout << "A_C=" << result.ordered_decompositions << "\n";
        std::cout << "N_linear_C=2^(C^2)*A_C=" << result.weighted_linear_sum << "\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "error: " << e.what() << "\n";
        return 1;
    }
}
