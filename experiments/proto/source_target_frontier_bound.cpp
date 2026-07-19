// Exact certificate for the fixed-source, target-distinguishing C=6
// within-band frontier lower bound.  This is a bounded decision prototype:
// it enumerates about 75,000 balanced maps per copy and never materializes
// the 5.49-billion source-target support that it certifies.

#include <algorithm>
#include <array>
#include <bit>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <numeric>
#include <stdexcept>
#include <unordered_set>
#include <utility>
#include <vector>

namespace {

constexpr int C = 6;
constexpr int N = 2 * C;
using Mask = std::uint16_t;
using Assignment = std::array<std::uint8_t, N>;
using ColumnMap = std::array<Mask, C>;
using Permutation = std::array<std::uint8_t, C>;
using TypeList = std::array<std::pair<Mask, Mask>, N>;

[[noreturn]] void fail(const char* message) {
    throw std::runtime_error(message);
}

void require(bool condition, const char* message) {
    if (!condition) fail(message);
}

Mask bit(int value) {
    return static_cast<Mask>(Mask{1} << value);
}

bool validBand(Mask cut, const Assignment& assignment) {
    std::array<int, C> total{};
    std::array<int, C> selected{};
    for (int symbol = 0; symbol < N; ++symbol) {
        const int column = assignment[symbol];
        if (column < 0 || column >= C) return false;
        ++total[column];
        if ((cut & bit(symbol)) != 0) ++selected[column];
    }
    for (int column = 0; column < C; ++column) {
        if (total[column] != 2 || selected[column] != 1) return false;
    }
    return std::popcount(cut) == C;
}

TypeList makeTypes(const Assignment& firstX, const Assignment& secondX,
                   const Assignment& firstY, const Assignment& secondY) {
    TypeList result{};
    for (int symbol = 0; symbol < N; ++symbol) {
        require(firstX[symbol] != secondX[symbol],
                "copy-X source repeats a used column");
        require(firstY[symbol] != secondY[symbol],
                "copy-Y source repeats a used column");
        result[symbol] = {
            static_cast<Mask>(bit(firstX[symbol]) | bit(secondX[symbol])),
            static_cast<Mask>(bit(firstY[symbol]) | bit(secondY[symbol]))};
    }
    return result;
}

using Matrix = std::array<std::array<std::uint8_t, N>, N>;

Matrix allowedSlotMatrix(const TypeList& types, bool secondCopy) {
    Matrix matrix{};
    for (int symbol = 0; symbol < N; ++symbol) {
        const Mask used = secondCopy ? types[symbol].second : types[symbol].first;
        for (int slot = 0; slot < N; ++slot) {
            matrix[symbol][slot] = static_cast<std::uint8_t>(
                (used & bit(slot / 2)) == 0);
        }
    }
    return matrix;
}

std::uint64_t permanentSubset(const Matrix& matrix) {
    std::vector<std::uint64_t> dp(std::size_t{1} << N);
    dp[0] = 1;
    for (int row = 0; row < N; ++row) {
        std::vector<std::uint64_t> next(std::size_t{1} << N);
        for (std::uint32_t mask = 0; mask < (std::uint32_t{1} << N); ++mask) {
            if (std::popcount(mask) != row || dp[mask] == 0) continue;
            for (int column = 0; column < N; ++column) {
                if (((mask >> column) & 1U) == 0 && matrix[row][column] != 0) {
                    next[mask | (std::uint32_t{1} << column)] += dp[mask];
                }
            }
        }
        dp.swap(next);
    }
    return dp.back();
}

std::uint64_t permanentRyser(const Matrix& matrix) {
    __int128 total = 0;
    for (std::uint32_t mask = 1; mask < (std::uint32_t{1} << N); ++mask) {
        std::uint64_t product = 1;
        for (int row = 0; row < N; ++row) {
            int rowSum = 0;
            for (int column = 0; column < N; ++column) {
                if (((mask >> column) & 1U) != 0) rowSum += matrix[row][column];
            }
            product *= static_cast<std::uint64_t>(rowSum);
        }
        if (((N - std::popcount(mask)) & 1) == 0) {
            total += product;
        } else {
            total -= product;
        }
    }
    require(total >= 0 && total <= UINT64_MAX, "Ryser permanent overflow");
    return static_cast<std::uint64_t>(total);
}

Mask transformMask(Mask mask, const Permutation& permutation) {
    Mask result = 0;
    for (int value = 0; value < C; ++value) {
        if ((mask & bit(value)) != 0) result |= bit(permutation[value]);
    }
    return result;
}

std::vector<Permutation> allPermutations() {
    Permutation permutation{};
    std::iota(permutation.begin(), permutation.end(), std::uint8_t{0});
    std::vector<Permutation> result;
    do {
        result.push_back(permutation);
    } while (std::next_permutation(permutation.begin(), permutation.end()));
    require(result.size() == 720, "wrong S6 permutation count");
    return result;
}

std::array<std::uint16_t, N> sortedTypeCodes(const TypeList& types) {
    std::array<std::uint16_t, N> result{};
    for (int symbol = 0; symbol < N; ++symbol) {
        result[symbol] = static_cast<std::uint16_t>(
            types[symbol].first | (types[symbol].second << C));
    }
    std::sort(result.begin(), result.end());
    return result;
}

std::pair<std::uint64_t, std::uint64_t> automorphismCounts(
        const TypeList& types) {
    const auto base = sortedTypeCodes(types);
    const auto permutations = allPermutations();
    std::uint64_t ordinary = 0;
    std::uint64_t swapped = 0;
    for (const auto& left : permutations) {
        for (const auto& right : permutations) {
            TypeList transformed{};
            TypeList copySwapped{};
            for (int symbol = 0; symbol < N; ++symbol) {
                transformed[symbol] = {
                    transformMask(types[symbol].first, left),
                    transformMask(types[symbol].second, right)};
                copySwapped[symbol] = {
                    transformMask(types[symbol].second, left),
                    transformMask(types[symbol].first, right)};
            }
            if (sortedTypeCodes(transformed) == base) ++ordinary;
            if (sortedTypeCodes(copySwapped) == base) ++swapped;
        }
    }
    return {ordinary, swapped};
}

void enumerateMapsRecursive(const TypeList& types, bool secondCopy, int column,
                            Mask usedSymbols, ColumnMap& current,
                            std::vector<ColumnMap>& output) {
    if (column == C) {
        require(usedSymbols == static_cast<Mask>((Mask{1} << N) - 1),
                "terminal map does not cover every symbol");
        output.push_back(current);
        return;
    }
    for (int first = 0; first < N; ++first) {
        if ((usedSymbols & bit(first)) != 0) continue;
        const Mask firstUsed = secondCopy ? types[first].second : types[first].first;
        if ((firstUsed & bit(column)) != 0) continue;
        for (int second = first + 1; second < N; ++second) {
            if ((usedSymbols & bit(second)) != 0) continue;
            const Mask secondUsed = secondCopy
                ? types[second].second : types[second].first;
            if ((secondUsed & bit(column)) != 0) continue;
            current[column] = static_cast<Mask>(bit(first) | bit(second));
            enumerateMapsRecursive(types, secondCopy, column + 1,
                                   static_cast<Mask>(usedSymbols | bit(first) |
                                                     bit(second)),
                                   current, output);
        }
    }
}

std::vector<ColumnMap> enumerateMaps(const TypeList& types, bool secondCopy) {
    std::vector<ColumnMap> result;
    result.reserve(80000);
    ColumnMap current{};
    enumerateMapsRecursive(types, secondCopy, 0, 0, current, result);
    return result;
}

struct SplitSupport {
    std::uint64_t minimum{};
    std::uint64_t maximum{};
    std::array<int, 3> minimumColumns{};
    std::array<int, 3> maximumColumns{};
};

SplitSupport splitSupport(const std::vector<ColumnMap>& maps) {
    std::vector<std::array<int, 3>> splits;
    for (int a = 0; a < C; ++a) {
        for (int b = a + 1; b < C; ++b) {
            for (int c = b + 1; c < C; ++c) splits.push_back({a, b, c});
        }
    }
    require(splits.size() == 20, "wrong three-column split count");
    std::vector<std::unordered_set<std::uint64_t>> support(splits.size());
    for (auto& set : support) set.reserve(10000);
    for (const auto& map : maps) {
        for (std::size_t index = 0; index < splits.size(); ++index) {
            const auto& columns = splits[index];
            const std::uint64_t code =
                static_cast<std::uint64_t>(map[columns[0]]) |
                (static_cast<std::uint64_t>(map[columns[1]]) << N) |
                (static_cast<std::uint64_t>(map[columns[2]]) << (2 * N));
            support[index].insert(code);
        }
    }
    SplitSupport result{};
    result.minimum = UINT64_MAX;
    for (std::size_t index = 0; index < splits.size(); ++index) {
        const std::uint64_t count = support[index].size();
        if (count < result.minimum) {
            result.minimum = count;
            result.minimumColumns = splits[index];
        }
        if (count > result.maximum) {
            result.maximum = count;
            result.maximumColumns = splits[index];
        }
    }
    return result;
}

std::uint64_t choose(int n, int k) {
    if (k < 0 || k > n) return 0;
    if (k > n - k) k = n - k;
    std::uint64_t value = 1;
    for (int i = 1; i <= k; ++i) {
        value = value * static_cast<std::uint64_t>(n - k + i) /
                static_cast<std::uint64_t>(i);
    }
    return value;
}

std::pair<std::uint64_t, std::uint64_t> resourceDimensions(int colors) {
    std::uint64_t peak = 0;
    std::uint64_t total = 0;
    for (int processed = 0; processed <= 2 * colors; ++processed) {
        std::uint64_t layer = 0;
        for (int upper = 0; upper <= colors; ++upper) {
            const int lower = processed - upper;
            if (lower < 0 || lower > colors) continue;
            const std::uint64_t a = choose(colors, upper);
            const std::uint64_t b = choose(colors, lower);
            layer += a * a * b * b;
        }
        peak = std::max(peak, layer);
        total += layer;
    }
    return {peak, total};
}

std::uint64_t factorial(int n) {
    std::uint64_t value = 1;
    for (int i = 2; i <= n; ++i) value *= static_cast<std::uint64_t>(i);
    return value;
}

}  // namespace

int main() {
    try {
        const Mask cut1 = static_cast<Mask>(
            bit(0) | bit(1) | bit(3) | bit(6) | bit(7) | bit(9));
        const Mask cut2 = static_cast<Mask>(
            bit(3) | bit(4) | bit(6) | bit(7) | bit(9) | bit(10));
        const Assignment firstX = {0,2,4,3,0,3,5,1,2,4,5,1};
        const Assignment firstY = {3,5,2,4,3,5,2,0,4,1,1,0};
        const Assignment secondX = {5,4,2,5,1,1,0,4,3,3,2,0};
        const Assignment secondY = {5,0,1,3,1,4,0,5,2,2,4,3};

        require(validBand(cut1, firstX), "invalid first copy-X band");
        require(validBand(cut1, firstY), "invalid first copy-Y band");
        require(validBand(cut2, secondX), "invalid second copy-X band");
        require(validBand(cut2, secondY), "invalid second copy-Y band");
        const TypeList types = makeTypes(firstX, secondX, firstY, secondY);

        std::unordered_set<std::uint16_t> distinctTypes;
        for (const auto& [left, right] : types) {
            distinctTypes.insert(static_cast<std::uint16_t>(left | (right << C)));
        }
        require(distinctTypes.size() == N, "source types are not all distinct");

        const auto [ordinaryAutomorphisms, swappedAutomorphisms] =
            automorphismCounts(types);
        require(ordinaryAutomorphisms == 1, "source has a nontrivial automorphism");
        require(swappedAutomorphisms == 0, "source has a copy-swapping automorphism");

        const Matrix matrixX = allowedSlotMatrix(types, false);
        const Matrix matrixY = allowedSlotMatrix(types, true);
        const std::uint64_t permanentX = permanentSubset(matrixX);
        const std::uint64_t permanentY = permanentSubset(matrixY);
        require(permanentRyser(matrixX) == permanentX,
                "copy-X permanent algorithms disagree");
        require(permanentRyser(matrixY) == permanentY,
                "copy-Y permanent algorithms disagree");
        require(permanentX == 4743616, "unexpected copy-X permanent");
        require(permanentY == 4740096, "unexpected copy-Y permanent");
        require(permanentX % (std::uint64_t{1} << C) == 0,
                "copy-X slot quotient is not exact");
        require(permanentY % (std::uint64_t{1} << C) == 0,
                "copy-Y slot quotient is not exact");

        const auto mapsX = enumerateMaps(types, false);
        const auto mapsY = enumerateMaps(types, true);
        require(mapsX.size() == permanentX / (std::uint64_t{1} << C),
                "copy-X map enumeration disagrees with permanent");
        require(mapsY.size() == permanentY / (std::uint64_t{1} << C),
                "copy-Y map enumeration disagrees with permanent");
        require(mapsX.size() == 74119, "unexpected copy-X map count");
        require(mapsY.size() == 74064, "unexpected copy-Y map count");

        const SplitSupport splitX = splitSupport(mapsX);
        const SplitSupport splitY = splitSupport(mapsY);
        require(splitX.minimum == 6488 && splitX.maximum == 7806,
                "unexpected copy-X split support");
        require(splitY.minimum == 6503 && splitY.maximum == 7806,
                "unexpected copy-Y split support");

        const std::uint64_t terminalSupport =
            static_cast<std::uint64_t>(mapsX.size()) * mapsY.size();
        const std::uint64_t splitRankLower = splitX.minimum * splitY.minimum;
        require(terminalSupport == 5489549616ULL,
                "unexpected terminal target support");
        require(splitRankLower == 42191464ULL,
                "unexpected fixed-split rank lower bound");

        const auto resource5 = resourceDimensions(5);
        const auto resource6 = resourceDimensions(6);
        require(resource5 == std::pair<std::uint64_t, std::uint64_t>{21252, 63504},
                "unexpected C=5 scalar resource dimensions");
        require(resource6 == std::pair<std::uint64_t, std::uint64_t>{263844, 853776},
                "unexpected C=6 scalar resource dimensions");
        const std::uint64_t gradeZero6 = factorial(12) * factorial(6) * factorial(6);
        require(gradeZero6 == 248314429440000ULL,
                "unexpected C=6 grade-zero transition mass");

        constexpr std::uint64_t trivialOrbitPairs = 276ULL * 277ULL / 2ULL;
        constexpr std::uint64_t coordinateGroupOrder = 46080;
        constexpr std::uint64_t orbitalDimensionLower =
            trivialOrbitPairs * coordinateGroupOrder;
        static_assert(trivialOrbitPairs == 38226);
        static_assert(orbitalDimensionLower == 1761454080ULL);

        std::printf("sourceTargetFrontierBound C=6 reachable=1 types=%zu "
                    "automorphisms=%llu swapAutomorphisms=%llu\n",
                    distinctTypes.size(),
                    static_cast<unsigned long long>(ordinaryAutomorphisms),
                    static_cast<unsigned long long>(swappedAutomorphisms));
        std::printf("permanents X=%llu Y=%llu maps X=%zu Y=%zu\n",
                    static_cast<unsigned long long>(permanentX),
                    static_cast<unsigned long long>(permanentY),
                    mapsX.size(), mapsY.size());
        std::printf("splitSupport X=%llu..%llu Y=%llu..%llu "
                    "rankLower=%llu\n",
                    static_cast<unsigned long long>(splitX.minimum),
                    static_cast<unsigned long long>(splitX.maximum),
                    static_cast<unsigned long long>(splitY.minimum),
                    static_cast<unsigned long long>(splitY.maximum),
                    static_cast<unsigned long long>(splitRankLower));
        std::printf("terminalSupport=%llu recordBytes16=%llu "
                    "recordGiB16=%.3f\n",
                    static_cast<unsigned long long>(terminalSupport),
                    static_cast<unsigned long long>(terminalSupport * 16ULL),
                    static_cast<double>(terminalSupport * 16ULL) /
                        static_cast<double>(std::uint64_t{1} << 30));
        std::printf("scalarResources C5=%llu/%llu C6=%llu/%llu "
                    "gradeZeroC6=%llu\n",
                    static_cast<unsigned long long>(resource5.first),
                    static_cast<unsigned long long>(resource5.second),
                    static_cast<unsigned long long>(resource6.first),
                    static_cast<unsigned long long>(resource6.second),
                    static_cast<unsigned long long>(gradeZero6));
        std::printf("orbitalDimensionLower=%llu [OK]\n",
                    static_cast<unsigned long long>(orbitalDimensionLower));
        return 0;
    } catch (const std::exception& error) {
        std::fprintf(stderr, "sourceTargetFrontierBound failed: %s\n", error.what());
        return 1;
    }
}
