// Exact decision prototype for the symbol-synchronous connectivity recurrence
// and its proposed operator-valued double-permanent transition.
//
// This file is intentionally limited to C <= 5.  C=5 runs beyond the verified
// three-symbol frontier require an explicit source prefix and operator limits.

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <mutex>
#include <numeric>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#ifdef _OPENMP
#include <omp.h>
#endif

namespace {

constexpr int MAX_C = 5;
constexpr int MAX_E = 2 * MAX_C;
constexpr int BITS_PER_E = 6;
constexpr int MAX_BITS = MAX_C * MAX_E * BITS_PER_E;
constexpr int MAX_WORDS = (MAX_BITS + 63) / 64;

using Count = unsigned __int128;

class ScaleLimit : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

std::string countToString(Count value) {
    if (value == 0) return "0";
    std::string result;
    while (value != 0) {
        result.push_back(static_cast<char>('0' + value % 10));
        value /= 10;
    }
    std::reverse(result.begin(), result.end());
    return result;
}

Count parseCount(const std::string& text) {
    Count value = 0;
    for (char ch : text) {
        if (ch < '0' || ch > '9') throw std::runtime_error("invalid count");
        value = value * 10 + static_cast<unsigned>(ch - '0');
    }
    return value;
}

void multiplyAdd(Count& target, Count left, Count right = 1) {
    constexpr Count maximum = ~static_cast<Count>(0);
    if (right != 0 && left > maximum / right) {
        throw std::overflow_error("128-bit multiplication overflow");
    }
    const Count product = left * right;
    if (target > maximum - product) {
        throw std::overflow_error("128-bit accumulation overflow");
    }
    target += product;
}

struct State {
    std::array<std::uint8_t, MAX_C * MAX_E> degree{};
    std::array<std::int8_t, MAX_C * MAX_E> mate{};

    State() { mate.fill(-1); }
};

struct Key {
    std::array<std::uint64_t, MAX_WORDS> words{};

    bool operator==(const Key& other) const noexcept {
        return words == other.words;
    }
};

struct KeyHash {
    std::size_t operator()(const Key& key) const noexcept {
        std::uint64_t hash = 0x9e3779b97f4a7c15ULL;
        for (std::uint64_t word : key.words) {
            word ^= word >> 30;
            word *= 0xbf58476d1ce4e5b9ULL;
            word ^= word >> 27;
            word *= 0x94d049bb133111ebULL;
            word ^= word >> 31;
            hash ^= word + 0x9e3779b97f4a7c15ULL +
                    (hash << 6) + (hash >> 2);
        }
        return static_cast<std::size_t>(hash);
    }
};

bool keyLess(const Key& left, const Key& right) {
    return left.words < right.words;
}

int endpointIndex(int c, int band, int endpoint) {
    return band * (2 * c) + endpoint;
}

Key pack(const State& state, int c) {
    Key key;
    int bit = 0;
    for (int band = 0; band < c; ++band) {
        for (int endpoint = 0; endpoint < 2 * c; ++endpoint) {
            const int index = endpointIndex(c, band, endpoint);
            const std::uint64_t value =
                static_cast<std::uint64_t>(state.degree[index]) |
                (static_cast<std::uint64_t>(state.mate[index] + 1) << 2);
            const int word = bit >> 6;
            const int offset = bit & 63;
            key.words[word] |= value << offset;
            if (offset > 64 - BITS_PER_E) {
                key.words[word + 1] |= value >> (64 - offset);
            }
            bit += BITS_PER_E;
        }
    }
    return key;
}

State unpack(const Key& key, int c) {
    State state;
    int bit = 0;
    for (int band = 0; band < c; ++band) {
        for (int endpoint = 0; endpoint < 2 * c; ++endpoint) {
            const int word = bit >> 6;
            const int offset = bit & 63;
            std::uint64_t value = key.words[word] >> offset;
            if (offset > 64 - BITS_PER_E) {
                value |= key.words[word + 1] << (64 - offset);
            }
            value &= 63;
            const int index = endpointIndex(c, band, endpoint);
            state.degree[index] = static_cast<std::uint8_t>(value & 3);
            state.mate[index] = static_cast<std::int8_t>((value >> 2) - 1);
            bit += BITS_PER_E;
        }
    }
    return state;
}

struct Signature {
    std::vector<int> values;

    bool operator<(const Signature& other) const {
        return values < other.values;
    }
    bool operator==(const Signature& other) const {
        return values == other.values;
    }
};

class Canonicalizer {
public:
    Canonicalizer(int c, bool allowSwap) : c_(c), allowSwap_(allowSwap) {}

    Key canonical(const State& state) {
        Key first = canonicalOneOrientation(state);
        if (!allowSwap_) return first;
        Key second = canonicalOneOrientation(swapped(state));
        return semanticLess(second, first) ? second : first;
    }

    std::uint64_t nodes() const { return nodes_; }

private:
    template <class Colors>
    int colorCount(const Colors& colors) const {
        int maximum = -1;
        for (int i = 0; i < c_; ++i) maximum = std::max(maximum, int(colors[i]));
        return maximum + 1;
    }

    template <class BandColors, class LeftColors, class RightColors>
    Signature bandSignature(const State& state, int band,
                            const BandColors& bandColors,
                            const LeftColors& leftColors,
                            const RightColors& rightColors) const {
        Signature signature;
        signature.values.push_back(bandColors[band]);
        const int leftCount = colorCount(leftColors);
        const int rightCount = colorCount(rightColors);
        for (int side = 0; side < 2; ++side) {
            const int count = side == 0 ? leftCount : rightCount;
            for (int colorClass = 0; colorClass < count; ++colorClass) {
                for (int degree = 0; degree < 3; ++degree) {
                    int total = 0;
                    for (int color = 0; color < c_; ++color) {
                        const int actualClass = side == 0
                                                    ? leftColors[color]
                                                    : rightColors[color];
                        const int endpoint = side * c_ + color;
                        if (actualClass == colorClass &&
                            state.degree[endpointIndex(c_, band, endpoint)] == degree) {
                            ++total;
                        }
                    }
                    signature.values.push_back(total);
                }
            }
        }

        struct Category {
            int side = 0;
            int color = 0;
            bool operator<(const Category& other) const {
                return side < other.side ||
                       (side == other.side && color < other.color);
            }
        };
        std::vector<std::array<int, 4>> pathTypes;
        for (int endpoint = 0; endpoint < 2 * c_; ++endpoint) {
            const int index = endpointIndex(c_, band, endpoint);
            if (state.degree[index] != 1) continue;
            const int mate = state.mate[index];
            if (endpoint >= mate) continue;
            Category first{endpoint / c_,
                           endpoint < c_ ? leftColors[endpoint]
                                         : rightColors[endpoint - c_]};
            Category second{mate / c_,
                            mate < c_ ? leftColors[mate]
                                      : rightColors[mate - c_]};
            if (second < first) std::swap(first, second);
            pathTypes.push_back({first.side, first.color,
                                 second.side, second.color});
        }
        std::sort(pathTypes.begin(), pathTypes.end());
        signature.values.push_back(-1);
        for (const auto& type : pathTypes) {
            for (int value : type) signature.values.push_back(value);
        }
        return signature;
    }

    template <class BandColors, class LeftColors, class RightColors>
    Signature colorSignature(const State& state, int side, int color,
                             const BandColors& bandColors,
                             const LeftColors& leftColors,
                             const RightColors& rightColors) const {
        Signature signature;
        const auto& ownColors = side == 0 ? leftColors : rightColors;
        signature.values.push_back(ownColors[color]);
        const int bandCount = colorCount(bandColors);
        const int leftCount = colorCount(leftColors);
        const int rightCount = colorCount(rightColors);
        for (int bandClass = 0; bandClass < bandCount; ++bandClass) {
            for (int degree = 0; degree < 3; ++degree) {
                int total = 0;
                for (int band = 0; band < c_; ++band) {
                    const int endpoint = side * c_ + color;
                    if (bandColors[band] == bandClass &&
                        state.degree[endpointIndex(c_, band, endpoint)] == degree) {
                        ++total;
                    }
                }
                signature.values.push_back(total);
            }
        }

        std::vector<std::array<int, 3>> mateTypes;
        for (int band = 0; band < c_; ++band) {
            const int endpoint = side * c_ + color;
            const int index = endpointIndex(c_, band, endpoint);
            if (state.degree[index] != 1) continue;
            const int mate = state.mate[index];
            const int mateSide = mate / c_;
            const int mateColor = mate < c_ ? leftColors[mate]
                                             : rightColors[mate - c_];
            mateTypes.push_back({bandColors[band], mateSide, mateColor});
        }
        std::sort(mateTypes.begin(), mateTypes.end());
        signature.values.push_back(-1);
        for (const auto& type : mateTypes) {
            for (int value : type) signature.values.push_back(value);
        }
        signature.values.push_back(-2);
        signature.values.push_back(leftCount);
        signature.values.push_back(rightCount);
        return signature;
    }

    template <class Colors>
    Colors recolor(const std::array<Signature, MAX_C>& signatures) const {
        std::vector<int> order(c_);
        std::iota(order.begin(), order.end(), 0);
        std::sort(order.begin(), order.end(), [&](int left, int right) {
            if (signatures[left] == signatures[right]) return left < right;
            return signatures[left] < signatures[right];
        });
        Colors result{};
        int colorClass = 0;
        result[order[0]] = 0;
        for (int i = 1; i < c_; ++i) {
            if (!(signatures[order[i]] == signatures[order[i - 1]])) {
                ++colorClass;
            }
            result[order[i]] = static_cast<std::uint8_t>(colorClass);
        }
        return result;
    }

    void refine(const State& state,
                std::array<std::uint8_t, MAX_C>& bandColors,
                std::array<std::uint8_t, MAX_C>& leftColors,
                std::array<std::uint8_t, MAX_C>& rightColors) const {
        while (true) {
            std::array<Signature, MAX_C> bandSignatures;
            std::array<Signature, MAX_C> leftSignatures;
            std::array<Signature, MAX_C> rightSignatures;
            for (int i = 0; i < c_; ++i) {
                bandSignatures[i] = bandSignature(
                    state, i, bandColors, leftColors, rightColors);
                leftSignatures[i] = colorSignature(
                    state, 0, i, bandColors, leftColors, rightColors);
                rightSignatures[i] = colorSignature(
                    state, 1, i, bandColors, leftColors, rightColors);
            }
            const auto newBand =
                recolor<std::array<std::uint8_t, MAX_C>>(bandSignatures);
            const auto newLeft =
                recolor<std::array<std::uint8_t, MAX_C>>(leftSignatures);
            const auto newRight =
                recolor<std::array<std::uint8_t, MAX_C>>(rightSignatures);
            const bool changed = newBand != bandColors ||
                                 newLeft != leftColors ||
                                 newRight != rightColors;
            bandColors = newBand;
            leftColors = newLeft;
            rightColors = newRight;
            if (!changed) return;
        }
    }

    template <class BandColors, class LeftColors, class RightColors>
    Key encode(const State& state, const BandColors& bandColors,
               const LeftColors& leftColors,
               const RightColors& rightColors) const {
        std::array<int, MAX_C> oldBand{};
        std::array<int, MAX_C> oldLeft{};
        std::array<int, MAX_C> oldRight{};
        for (int i = 0; i < c_; ++i) {
            oldBand[bandColors[i]] = i;
            oldLeft[leftColors[i]] = i;
            oldRight[rightColors[i]] = i;
        }
        State transformed;
        for (int newBand = 0; newBand < c_; ++newBand) {
            const int sourceBand = oldBand[newBand];
            for (int newEndpoint = 0; newEndpoint < 2 * c_; ++newEndpoint) {
                const int sourceEndpoint = newEndpoint < c_
                                               ? oldLeft[newEndpoint]
                                               : c_ + oldRight[newEndpoint - c_];
                const int sourceIndex =
                    endpointIndex(c_, sourceBand, sourceEndpoint);
                const int targetIndex =
                    endpointIndex(c_, newBand, newEndpoint);
                transformed.degree[targetIndex] = state.degree[sourceIndex];
                const int sourceMate = state.mate[sourceIndex];
                if (sourceMate >= 0) {
                    transformed.mate[targetIndex] = static_cast<std::int8_t>(
                        sourceMate < c_ ? leftColors[sourceMate]
                                        : c_ + rightColors[sourceMate - c_]);
                }
            }
        }
        return pack(transformed, c_);
    }

    bool semanticLess(const Key& left, const Key& right) const {
        const State first = unpack(left, c_);
        const State second = unpack(right, c_);
        for (int band = 0; band < c_; ++band) {
            for (int endpoint = 0; endpoint < 2 * c_; ++endpoint) {
                const int index = endpointIndex(c_, band, endpoint);
                if (first.degree[index] != second.degree[index]) {
                    return first.degree[index] < second.degree[index];
                }
                if (first.mate[index] != second.mate[index]) {
                    return first.mate[index] < second.mate[index];
                }
            }
        }
        return false;
    }

    Key search(const State& state,
               std::array<std::uint8_t, MAX_C> bandColors,
               std::array<std::uint8_t, MAX_C> leftColors,
               std::array<std::uint8_t, MAX_C> rightColors) {
        ++nodes_;
        refine(state, bandColors, leftColors, rightColors);
        if (colorCount(bandColors) == c_ &&
            colorCount(leftColors) == c_ &&
            colorCount(rightColors) == c_) {
            return encode(state, bandColors, leftColors, rightColors);
        }

        int bestPart = -1;
        int bestColor = -1;
        int bestSize = std::numeric_limits<int>::max();
        auto scan = [&](int part, const auto& colors) {
            std::array<int, MAX_C> counts{};
            for (int i = 0; i < c_; ++i) ++counts[colors[i]];
            for (int color = 0; color < c_; ++color) {
                if (counts[color] > 1 && counts[color] < bestSize) {
                    bestPart = part;
                    bestColor = color;
                    bestSize = counts[color];
                }
            }
        };
        scan(0, bandColors);
        scan(1, leftColors);
        scan(2, rightColors);

        bool first = true;
        Key best;
        const auto& base = bestPart == 0 ? bandColors
                           : bestPart == 1 ? leftColors
                                           : rightColors;
        for (int vertex = 0; vertex < c_; ++vertex) {
            if (base[vertex] != bestColor) continue;
            auto nextBand = bandColors;
            auto nextLeft = leftColors;
            auto nextRight = rightColors;
            auto& selected = bestPart == 0 ? nextBand
                             : bestPart == 1 ? nextLeft
                                             : nextRight;
            selected[vertex] = static_cast<std::uint8_t>(colorCount(selected));
            const Key candidate = search(
                state, nextBand, nextLeft, nextRight);
            if (first || semanticLess(candidate, best)) {
                best = candidate;
                first = false;
            }
        }
        return best;
    }

    Key canonicalOneOrientation(const State& state) {
        std::array<std::uint8_t, MAX_C> colors{};
        return search(state, colors, colors, colors);
    }

    State swapped(const State& state) const {
        State result;
        for (int band = 0; band < c_; ++band) {
            for (int endpoint = 0; endpoint < 2 * c_; ++endpoint) {
                const int targetEndpoint = endpoint < c_ ? c_ + endpoint
                                                         : endpoint - c_;
                const int sourceIndex = endpointIndex(c_, band, endpoint);
                const int targetIndex = endpointIndex(c_, band, targetEndpoint);
                result.degree[targetIndex] = state.degree[sourceIndex];
                const int mate = state.mate[sourceIndex];
                if (mate >= 0) {
                    result.mate[targetIndex] = static_cast<std::int8_t>(
                        mate < c_ ? c_ + mate : mate - c_);
                }
            }
        }
        return result;
    }

    int c_;
    bool allowSwap_;
    std::uint64_t nodes_ = 0;
};

using Permutation = std::array<std::uint8_t, MAX_C>;

std::vector<Permutation> permutations(int c) {
    std::vector<Permutation> result;
    std::vector<int> values(c);
    std::iota(values.begin(), values.end(), 0);
    do {
        Permutation permutation{};
        for (int i = 0; i < c; ++i) {
            permutation[i] = static_cast<std::uint8_t>(values[i]);
        }
        result.push_back(permutation);
    } while (std::next_permutation(values.begin(), values.end()));
    return result;
}

// Adds one edge to a band.  Returns 0 if illegal, otherwise the exact factor
// one or two produced by the path/cycle quotient.
int addEdge(State& state, int c, int band, int leftColor, int rightColor) {
    const int left = leftColor;
    const int right = c + rightColor;
    const int leftIndex = endpointIndex(c, band, left);
    const int rightIndex = endpointIndex(c, band, right);
    const int leftDegree = state.degree[leftIndex];
    const int rightDegree = state.degree[rightIndex];
    if (leftDegree >= 2 || rightDegree >= 2) return 0;

    if (leftDegree == 0 && rightDegree == 0) {
        state.degree[leftIndex] = state.degree[rightIndex] = 1;
        state.mate[leftIndex] = static_cast<std::int8_t>(right);
        state.mate[rightIndex] = static_cast<std::int8_t>(left);
        return 1;
    }
    if (leftDegree == 1 && rightDegree == 0) {
        const int other = state.mate[leftIndex];
        if (other < 0) throw std::runtime_error("missing left path endpoint");
        state.degree[leftIndex] = 2;
        state.mate[leftIndex] = -1;
        state.degree[rightIndex] = 1;
        state.mate[rightIndex] = static_cast<std::int8_t>(other);
        state.mate[endpointIndex(c, band, other)] =
            static_cast<std::int8_t>(right);
        return 1;
    }
    if (leftDegree == 0 && rightDegree == 1) {
        const int other = state.mate[rightIndex];
        if (other < 0) throw std::runtime_error("missing right path endpoint");
        state.degree[rightIndex] = 2;
        state.mate[rightIndex] = -1;
        state.degree[leftIndex] = 1;
        state.mate[leftIndex] = static_cast<std::int8_t>(other);
        state.mate[endpointIndex(c, band, other)] =
            static_cast<std::int8_t>(left);
        return 1;
    }

    const int otherLeft = state.mate[leftIndex];
    const int otherRight = state.mate[rightIndex];
    if (otherLeft < 0 || otherRight < 0) {
        throw std::runtime_error("missing joined path endpoint");
    }
    state.degree[leftIndex] = state.degree[rightIndex] = 2;
    state.mate[leftIndex] = state.mate[rightIndex] = -1;
    if (otherLeft == right) {
        if (otherRight != left) {
            throw std::runtime_error("cycle endpoint mismatch");
        }
        return 2;
    }
    state.mate[endpointIndex(c, band, otherLeft)] =
        static_cast<std::int8_t>(otherRight);
    state.mate[endpointIndex(c, band, otherRight)] =
        static_cast<std::int8_t>(otherLeft);
    return 1;
}

struct RawValue {
    Count weight = 0;
    std::uint64_t paths = 0;
};

using RawMap = std::unordered_map<Key, RawValue, KeyHash>;
using CountMap = std::unordered_map<Key, Count, KeyHash>;

struct TransitionStats {
    std::uint64_t validMoves = 0;
    std::uint64_t generatedRecords = 0;
    std::uint64_t mergedPrefixPlacements = 0;
    std::size_t peakFrontier = 0;
    std::vector<std::size_t> frontiers;
    std::vector<std::uint64_t> placements;
};

struct TransitionResult {
    RawMap raw;
    TransitionStats stats;
};

TransitionResult directTransition(
    const State& source, int c, const std::vector<Permutation>& permutationList) {
    TransitionResult result;
    result.raw.reserve(permutationList.size() * permutationList.size());
    for (const Permutation& left : permutationList) {
        for (const Permutation& right : permutationList) {
            State target = source;
            int factor = 1;
            for (int band = 0; band < c; ++band) {
                const int local = addEdge(
                    target, c, band, left[band], right[band]);
                if (local == 0) {
                    factor = 0;
                    break;
                }
                factor *= local;
            }
            if (factor == 0) continue;
            RawValue& value = result.raw[pack(target, c)];
            multiplyAdd(value.weight, static_cast<Count>(factor));
            ++value.paths;
            ++result.stats.validMoves;
        }
    }
    result.stats.generatedRecords = result.stats.validMoves;
    result.stats.peakFrontier = result.raw.size();
    result.stats.frontiers.push_back(result.raw.size());
    result.stats.placements.push_back(result.stats.validMoves);
    return result;
}

struct PartialKey {
    Key state;
    std::uint8_t usedLeft = 0;
    std::uint8_t usedRight = 0;

    bool operator==(const PartialKey& other) const noexcept {
        return state == other.state && usedLeft == other.usedLeft &&
               usedRight == other.usedRight;
    }
};

struct PartialKeyHash {
    std::size_t operator()(const PartialKey& key) const noexcept {
        std::uint64_t hash = static_cast<std::uint64_t>(KeyHash{}(key.state));
        hash ^= static_cast<std::uint64_t>(key.usedLeft) << 48;
        hash ^= static_cast<std::uint64_t>(key.usedRight) << 56;
        return static_cast<std::size_t>(hash);
    }
};

using PartialMap = std::unordered_map<PartialKey, RawValue, PartialKeyHash>;

TransitionResult subsetTransition(const State& source, int c,
                                  std::size_t maxStates,
                                  std::uint64_t maxRecords) {
    // The partial target stays labelled.  Canonicalizing it alone would also
    // move the fixed source and is not an equivariant per-source quotient.
    // Frontier/placement counts below measure whether exact labelled states
    // nevertheless merge before the completed double permanent.
    PartialMap current;
    PartialKey start;
    start.state = pack(source, c);
    current.emplace(start, RawValue{1, 1});

    TransitionResult result;
    result.stats.peakFrontier = 1;
    for (int band = 0; band < c; ++band) {
        PartialMap next;
        next.reserve(std::min<std::size_t>(maxStates, current.size() * 4 + 64));
        for (const auto& [partial, value] : current) {
            const State base = unpack(partial.state, c);
            for (int left = 0; left < c; ++left) {
                if (partial.usedLeft & (1U << left)) continue;
                for (int right = 0; right < c; ++right) {
                    if (partial.usedRight & (1U << right)) continue;
                    State target = base;
                    const int factor = addEdge(target, c, band, left, right);
                    if (factor == 0) continue;
                    PartialKey output;
                    output.state = pack(target, c);
                    output.usedLeft = static_cast<std::uint8_t>(
                        partial.usedLeft | (1U << left));
                    output.usedRight = static_cast<std::uint8_t>(
                        partial.usedRight | (1U << right));
                    RawValue& destination = next[output];
                    multiplyAdd(destination.weight, value.weight,
                                static_cast<Count>(factor));
                    if (destination.paths >
                        std::numeric_limits<std::uint64_t>::max() - value.paths) {
                        throw std::overflow_error("partial path-count overflow");
                    }
                    destination.paths += value.paths;
                    ++result.stats.generatedRecords;
                    if (maxRecords != 0 &&
                        result.stats.generatedRecords > maxRecords) {
                        throw ScaleLimit("subset generated-record limit exceeded");
                    }
                    if (next.size() > maxStates) {
                        throw ScaleLimit("subset state limit exceeded");
                    }
                }
            }
        }
        current.swap(next);
        result.stats.peakFrontier =
            std::max(result.stats.peakFrontier, current.size());
        std::uint64_t placements = 0;
        for (const auto& [key, value] : current) {
            (void)key;
            if (placements >
                std::numeric_limits<std::uint64_t>::max() - value.paths) {
                throw std::overflow_error("frontier placement-count overflow");
            }
            placements += value.paths;
        }
        result.stats.frontiers.push_back(current.size());
        result.stats.placements.push_back(placements);
        if (placements < current.size()) {
            throw std::runtime_error("subset frontier exceeds placement count");
        }
        result.stats.mergedPrefixPlacements +=
            placements - static_cast<std::uint64_t>(current.size());
    }

    result.raw.reserve(current.size());
    const std::uint8_t fullMask = static_cast<std::uint8_t>((1U << c) - 1);
    for (const auto& [partial, value] : current) {
        if (partial.usedLeft != fullMask || partial.usedRight != fullMask) {
            throw std::runtime_error("incomplete terminal subset mask");
        }
        auto [it, inserted] = result.raw.emplace(partial.state, value);
        if (!inserted) {
            multiplyAdd(it->second.weight, value.weight);
            if (it->second.paths >
                std::numeric_limits<std::uint64_t>::max() - value.paths) {
                throw std::overflow_error("terminal path-count overflow");
            }
            it->second.paths += value.paths;
        }
        if (result.stats.validMoves >
            std::numeric_limits<std::uint64_t>::max() - value.paths) {
            throw std::overflow_error("valid-move count overflow");
        }
        result.stats.validMoves += value.paths;
    }
    return result;
}

void compareRaw(const TransitionResult& direct,
                const TransitionResult& subset, int c, int symbol) {
    if (direct.stats.validMoves != subset.stats.validMoves ||
        direct.raw.size() != subset.raw.size()) {
        throw std::runtime_error(
            "direct/subset raw support mismatch at C=" + std::to_string(c) +
            " symbol=" + std::to_string(symbol));
    }
    for (const auto& [key, value] : direct.raw) {
        const auto found = subset.raw.find(key);
        if (found == subset.raw.end() ||
            found->second.weight != value.weight ||
            found->second.paths != value.paths) {
            throw std::runtime_error(
                "direct/subset raw coefficient mismatch at C=" +
                std::to_string(c) + " symbol=" + std::to_string(symbol));
        }
    }
}

CountMap reduceRaw(const RawMap& raw, Canonicalizer& canonicalizer, int c) {
    CountMap result;
    result.reserve(raw.size());
    for (const auto& [key, value] : raw) {
        const Key canonical = canonicalizer.canonical(unpack(key, c));
        multiplyAdd(result[canonical], value.weight);
    }
    return result;
}

Permutation inversePermutation(const Permutation& permutation, int c) {
    Permutation inverse{};
    for (int i = 0; i < c; ++i) inverse[permutation[i]] = i;
    return inverse;
}

Permutation composePermutation(const Permutation& first,
                               const Permutation& second, int c) {
    Permutation result{};
    for (int i = 0; i < c; ++i) result[i] = first[second[i]];
    return result;
}

struct PairKey {
    std::array<std::uint8_t, 2 * MAX_C> values{};

    bool operator==(const PairKey& other) const noexcept {
        return values == other.values;
    }
};

struct PairKeyHash {
    std::size_t operator()(const PairKey& key) const noexcept {
        std::uint64_t hash = 0;
        for (std::uint8_t value : key.values) hash = hash * 13 + value + 1;
        return static_cast<std::size_t>(hash);
    }
};

PairKey canonicalPair(const Permutation& left, const Permutation& right, int c,
                      const std::vector<Permutation>& permutationList) {
    PairKey best;
    bool first = true;
    for (const Permutation& conjugator : permutationList) {
        const Permutation inverse = inversePermutation(conjugator, c);
        const Permutation transformedLeft = composePermutation(
            composePermutation(conjugator, left, c), inverse, c);
        const Permutation transformedRight = composePermutation(
            composePermutation(conjugator, right, c), inverse, c);
        PairKey ordinary;
        PairKey swapped;
        for (int i = 0; i < c; ++i) {
            ordinary.values[i] = transformedLeft[i];
            ordinary.values[MAX_C + i] = transformedRight[i];
            swapped.values[i] = transformedRight[i];
            swapped.values[MAX_C + i] = transformedLeft[i];
        }
        if (first || ordinary.values < best.values) {
            best = ordinary;
            first = false;
        }
        if (swapped.values < best.values) best = swapped;
    }
    return best;
}

CountMap initializeTwoSymbols(int c, bool allowSwap,
                              const std::vector<Permutation>& permutationList) {
    std::unordered_map<PairKey, std::uint64_t, PairKeyHash> multiplicities;
    for (const Permutation& left : permutationList) {
        for (const Permutation& right : permutationList) {
            ++multiplicities[canonicalPair(
                left, right, c, permutationList)];
        }
    }

    Canonicalizer canonicalizer(c, allowSwap);
    CountMap current;
    const Count firstMultiplicity = static_cast<Count>(permutationList.size()) *
                                    permutationList.size();
    for (const auto& [pair, multiplicity] : multiplicities) {
        Permutation left{};
        Permutation right{};
        for (int i = 0; i < c; ++i) {
            left[i] = pair.values[i];
            right[i] = pair.values[MAX_C + i];
        }
        State state;
        for (int band = 0; band < c; ++band) {
            if (addEdge(state, c, band, band, band) != 1) {
                throw std::runtime_error("invalid first-symbol initializer");
            }
        }
        int factor = 1;
        for (int band = 0; band < c; ++band) {
            const int local = addEdge(
                state, c, band, left[band], right[band]);
            if (local == 0) {
                throw std::runtime_error("invalid second-symbol initializer");
            }
            factor *= local;
        }
        multiplyAdd(current[canonicalizer.canonical(state)], firstMultiplicity,
                    static_cast<Count>(multiplicity) * factor);
    }
    return current;
}

CountMap initializeTwoSymbolsDirect(
    int c, bool allowSwap,
    const std::vector<Permutation>& permutationList) {
    State empty;
    Canonicalizer initialCanonicalizer(c, allowSwap);
    CountMap current;
    current.emplace(initialCanonicalizer.canonical(empty), 1);
    for (int symbol = 0; symbol < 2; ++symbol) {
        CountMap next;
        for (const auto& [sourceKey, sourceCoefficient] : current) {
            const TransitionResult transition = directTransition(
                unpack(sourceKey, c), c, permutationList);
            Canonicalizer canonicalizer(c, allowSwap);
            const CountMap canonical = reduceRaw(
                transition.raw, canonicalizer, c);
            for (const auto& [target, transitionCoefficient] : canonical) {
                multiplyAdd(next[target], sourceCoefficient,
                            transitionCoefficient);
            }
        }
        current.swap(next);
    }
    return current;
}

void compareCountMaps(const CountMap& expected, const CountMap& actual,
                      const std::string& label) {
    if (expected.size() != actual.size()) {
        throw std::runtime_error(label + " support mismatch");
    }
    for (const auto& [key, coefficient] : expected) {
        const auto found = actual.find(key);
        if (found == actual.end() || found->second != coefficient) {
            throw std::runtime_error(label + " coefficient mismatch");
        }
    }
}

enum class Mode { Direct, Subset, Differential };

struct Options {
    int c = 0;
    int stop = -1;
    bool allowSwap = true;
    bool initializeTwo = true;
    bool initializationCheck = false;
    bool rawProbe = false;
    Mode mode = Mode::Direct;
    std::size_t sourceStart = 0;
    std::size_t sourceProbe = 0;
    std::size_t maxStates = 2'000'000;
    std::size_t maxOperatorStates = 100'000;
    std::uint64_t maxOperatorRecords = 1'000'000;
    bool operatorStateLimitExplicit = false;
    bool operatorRecordLimitExplicit = false;
};

struct SourceStats {
    std::size_t index = 0;
    std::uint64_t validMoves = 0;
    std::uint64_t subsetGenerated = 0;
    std::uint64_t subsetMergedPrefixes = 0;
    std::size_t subsetPeak = 0;
    std::size_t rawSupport = 0;
    std::size_t canonicalSupport = 0;
    std::vector<std::size_t> subsetFrontiers;
    std::vector<std::uint64_t> subsetPlacements;
};

struct LayerStats {
    int symbol = 0;
    std::size_t sourceStates = 0;
    std::size_t processedSources = 0;
    std::size_t targetStates = 0;
    std::uint64_t validMoves = 0;
    std::uint64_t subsetGenerated = 0;
    std::uint64_t subsetMergedPrefixes = 0;
    std::size_t subsetPeak = 0;
    std::uint64_t rawSupport = 0;
    std::uint64_t canonicalSupport = 0;
    double seconds = 0;
    bool closed = true;
};

struct RunResult {
    Count total = 0;
    bool complete = false;
    bool probe = false;
    std::vector<std::size_t> stateCounts;
};

int threadCount() {
#ifdef _OPENMP
    return std::min(32, omp_get_max_threads());
#else
    return 1;
#endif
}

RunResult run(const Options& options) {
    const int stop = options.stop < 0 ? 2 * options.c : options.stop;
    const std::vector<Permutation> permutationList = permutations(options.c);
    CountMap current;
    RunResult result;
    result.stateCounts.push_back(1);
    int startSymbol = 0;
    if (options.c == 5 && options.initializeTwo && stop >= 2) {
        current = initializeTwoSymbols(
            options.c, options.allowSwap, permutationList);
        if (options.initializationCheck) {
            const CountMap reference = initializeTwoSymbolsDirect(
                options.c, options.allowSwap, permutationList);
            compareCountMaps(reference, current, "two-symbol initializer");
            std::cout << "init2_check=1 states=" << current.size() << "\n";
        }
        result.stateCounts.push_back(1);
        result.stateCounts.push_back(current.size());
        startSymbol = 2;
        std::cout << "symbol=2 initialized=1 states=" << current.size() << "\n";
    } else {
        State empty;
        Canonicalizer canonicalizer(options.c, options.allowSwap);
        current.emplace(canonicalizer.canonical(empty), 1);
    }

    for (int symbol = startSymbol; symbol < stop; ++symbol) {
        const auto started = std::chrono::steady_clock::now();
        std::vector<std::pair<Key, Count>> sources(current.begin(), current.end());
        std::sort(sources.begin(), sources.end(),
                  [](const auto& left, const auto& right) {
                      return keyLess(left.first, right.first);
                  });
        const bool finalRequestedTransition = symbol + 1 == stop;
        const bool useRawProbe = options.rawProbe && finalRequestedTransition;
        const std::size_t sourceStart = finalRequestedTransition
                                            ? options.sourceStart
                                            : 0;
        if (sourceStart >= sources.size()) {
            throw std::runtime_error("sourcestart is outside source frontier");
        }
        const std::size_t available = sources.size() - sourceStart;
        const std::size_t processCount =
            finalRequestedTransition && options.sourceProbe != 0
                ? std::min(options.sourceProbe, available)
                : available;

        LayerStats stats;
        stats.symbol = symbol + 1;
        stats.sourceStates = sources.size();
        stats.processedSources = processCount;
        stats.closed = !finalRequestedTransition ||
                       (sourceStart == 0 && options.sourceProbe == 0 &&
                        processCount == sources.size() && !useRawProbe);
        std::vector<SourceStats> sourceStats(processCount);

        const int workers = std::min<int>(threadCount(), processCount);
        std::vector<CountMap> localNext(std::max(1, workers));
        for (CountMap& map : localNext) {
            map.reserve(std::min<std::size_t>(
                options.maxStates,
                current.size() * 4 / std::max(1, workers) + 128));
        }
        std::atomic<bool> failed{false};
        std::atomic<bool> scaleFailed{false};
        std::mutex errorMutex;
        std::string errorMessage;

#ifdef _OPENMP
#pragma omp parallel for schedule(dynamic, 1) num_threads(workers)
#endif
        for (std::int64_t localIndex = 0;
             localIndex < static_cast<std::int64_t>(processCount);
             ++localIndex) {
            if (failed.load(std::memory_order_relaxed)) continue;
#ifdef _OPENMP
            const int worker = omp_get_thread_num();
#else
            const int worker = 0;
#endif
            try {
                const std::size_t sourceIndex =
                    sourceStart + static_cast<std::size_t>(localIndex);
                const auto& [sourceKey, sourceCoefficient] = sources[sourceIndex];
                const State source = unpack(sourceKey, options.c);

                TransitionResult direct;
                TransitionResult subset;
                if (options.mode == Mode::Direct ||
                    options.mode == Mode::Differential) {
                    direct = directTransition(
                        source, options.c, permutationList);
                }
                if (options.mode == Mode::Subset ||
                    options.mode == Mode::Differential) {
                    subset = subsetTransition(
                        source, options.c, options.maxOperatorStates,
                        options.maxOperatorRecords);
                }
                if (options.mode == Mode::Differential) {
                    compareRaw(direct, subset, options.c, symbol + 1);
                }
                const TransitionResult& selected =
                    options.mode == Mode::Direct ? direct : subset;

                SourceStats one;
                one.index = sourceIndex;
                one.validMoves = selected.stats.validMoves;
                one.rawSupport = selected.raw.size();
                if (options.mode != Mode::Direct) {
                    one.subsetGenerated = selected.stats.generatedRecords;
                    one.subsetMergedPrefixes =
                        selected.stats.mergedPrefixPlacements;
                    one.subsetPeak = selected.stats.peakFrontier;
                    one.subsetFrontiers = selected.stats.frontiers;
                    one.subsetPlacements = selected.stats.placements;
                }

                if (!useRawProbe) {
                    Canonicalizer canonicalizer(
                        options.c, options.allowSwap);
                    CountMap canonical = reduceRaw(
                        selected.raw, canonicalizer, options.c);
                    one.canonicalSupport = canonical.size();
                    for (const auto& [target, transitionCoefficient] : canonical) {
                        multiplyAdd(localNext[worker][target], sourceCoefficient,
                                    transitionCoefficient);
                    }
                }
                sourceStats[localIndex] = std::move(one);
            } catch (const ScaleLimit& error) {
                scaleFailed.store(true, std::memory_order_relaxed);
                failed.store(true, std::memory_order_relaxed);
                std::lock_guard<std::mutex> lock(errorMutex);
                if (errorMessage.empty()) errorMessage = error.what();
            } catch (const std::exception& error) {
                failed.store(true, std::memory_order_relaxed);
                std::lock_guard<std::mutex> lock(errorMutex);
                if (errorMessage.empty()) errorMessage = error.what();
            }
        }
        if (failed.load()) {
            if (scaleFailed.load()) throw ScaleLimit(errorMessage);
            throw std::runtime_error(errorMessage);
        }

        CountMap next;
        if (!useRawProbe) {
            next.reserve(std::min<std::size_t>(
                options.maxStates, current.size() * 8 + 128));
            for (const CountMap& local : localNext) {
                for (const auto& [key, coefficient] : local) {
                    multiplyAdd(next[key], coefficient);
                }
            }
            if (next.size() > options.maxStates) {
                throw ScaleLimit("global state limit exceeded");
            }
        }

        for (const SourceStats& one : sourceStats) {
            stats.validMoves += one.validMoves;
            stats.subsetGenerated += one.subsetGenerated;
            stats.subsetMergedPrefixes += one.subsetMergedPrefixes;
            stats.subsetPeak = std::max(stats.subsetPeak, one.subsetPeak);
            stats.rawSupport += one.rawSupport;
            stats.canonicalSupport += one.canonicalSupport;
            if ((finalRequestedTransition && options.sourceProbe != 0) ||
                useRawProbe) {
                std::cout << "source symbol=" << (symbol + 1)
                          << " index=" << one.index
                          << " valid_moves=" << one.validMoves
                          << " raw_support=" << one.rawSupport
                          << " canonical_support=" << one.canonicalSupport
                          << " subset_generated=" << one.subsetGenerated
                          << " subset_merged_prefixes="
                          << one.subsetMergedPrefixes
                          << " subset_peak=" << one.subsetPeak;
                if (!one.subsetFrontiers.empty()) {
                    std::cout << " subset_frontiers=";
                    for (std::size_t i = 0; i < one.subsetFrontiers.size(); ++i) {
                        if (i != 0) std::cout << ',';
                        std::cout << one.subsetFrontiers[i];
                    }
                    std::cout << " subset_placements=";
                    for (std::size_t i = 0; i < one.subsetPlacements.size(); ++i) {
                        if (i != 0) std::cout << ',';
                        std::cout << one.subsetPlacements[i];
                    }
                }
                std::cout << "\n";
            }
        }

        stats.targetStates = next.size();
        stats.seconds = std::chrono::duration<double>(
            std::chrono::steady_clock::now() - started).count();
        std::cout << "symbol=" << (symbol + 1)
                  << " closed=" << (stats.closed ? 1 : 0)
                  << " source_states=" << stats.sourceStates
                  << " processed_sources=" << stats.processedSources
                  << " states=" << stats.targetStates
                  << " valid_moves=" << stats.validMoves
                  << " raw_support=" << stats.rawSupport
                  << " canonical_support=" << stats.canonicalSupport
                  << " subset_generated=" << stats.subsetGenerated
                  << " subset_merged_prefixes="
                  << stats.subsetMergedPrefixes
                  << " subset_peak=" << stats.subsetPeak
                  << " seconds=" << stats.seconds << "\n";

        if (!stats.closed) {
            result.probe = true;
            return result;
        }
        current.swap(next);
        result.stateCounts.push_back(current.size());
    }

    if (stop == 2 * options.c) {
        if (current.size() != 1) {
            throw std::runtime_error("terminal connectivity orbit is not unique");
        }
        result.total = current.begin()->second;
        result.complete = true;
    }
    return result;
}

Count knownValue(int c) {
    if (c == 2) return parseCount("288");
    if (c == 3) return parseCount("28200960");
    if (c == 4) return parseCount("29136487207403520");
    if (c == 5) return parseCount("1903816047972624930994913280000");
    throw std::runtime_error("no known value");
}

Options parseOptions(int argc, char** argv) {
    if (argc < 2) {
        throw std::runtime_error(
            "usage: connectivity_operator C [stop=N] [direct|subset|differential] "
            "[sourcestart=N] [sourceprobe=N] [rawprobe] "
            "[maxstates=N] [maxoperatorstates=N] [maxoperatorrecords=N] "
            "[initcheck] [noinit2] [noswap]");
    }
    Options options;
    options.c = std::atoi(argv[1]);
    for (int i = 2; i < argc; ++i) {
        const std::string argument = argv[i];
        if (argument.rfind("stop=", 0) == 0) {
            options.stop = std::atoi(argument.c_str() + 5);
        } else if (argument.rfind("sourcestart=", 0) == 0) {
            options.sourceStart = static_cast<std::size_t>(
                std::strtoull(argument.c_str() + 12, nullptr, 10));
        } else if (argument.rfind("sourceprobe=", 0) == 0) {
            options.sourceProbe = static_cast<std::size_t>(
                std::strtoull(argument.c_str() + 12, nullptr, 10));
        } else if (argument.rfind("maxstates=", 0) == 0) {
            options.maxStates = static_cast<std::size_t>(
                std::strtoull(argument.c_str() + 10, nullptr, 10));
        } else if (argument.rfind("maxoperatorstates=", 0) == 0) {
            options.maxOperatorStates = static_cast<std::size_t>(
                std::strtoull(
                    argument.c_str() + std::string("maxoperatorstates=").size(),
                    nullptr, 10));
            options.operatorStateLimitExplicit = true;
        } else if (argument.rfind("maxoperatorrecords=", 0) == 0) {
            options.maxOperatorRecords = std::strtoull(
                argument.c_str() + std::string("maxoperatorrecords=").size(),
                nullptr, 10);
            options.operatorRecordLimitExplicit = true;
        } else if (argument == "direct") {
            options.mode = Mode::Direct;
        } else if (argument == "subset") {
            options.mode = Mode::Subset;
        } else if (argument == "differential") {
            options.mode = Mode::Differential;
        } else if (argument == "rawprobe") {
            options.rawProbe = true;
        } else if (argument == "noinit2") {
            options.initializeTwo = false;
        } else if (argument == "initcheck") {
            options.initializationCheck = true;
        } else if (argument == "noswap") {
            options.allowSwap = false;
        } else {
            throw std::runtime_error("unknown argument: " + argument);
        }
    }

    if (options.c < 2 || options.c > MAX_C) {
        throw std::runtime_error("C must be in [2,5]");
    }
    const int stop = options.stop < 0 ? 2 * options.c : options.stop;
    if (stop < 0 || stop > 2 * options.c) {
        throw std::runtime_error("invalid stop symbol");
    }
    if (options.maxStates == 0 || options.maxOperatorStates == 0 ||
        options.maxOperatorRecords == 0) {
        throw std::runtime_error("state and record limits must be positive");
    }
    if (options.sourceProbe == 0 &&
        (options.sourceStart != 0 || options.rawProbe)) {
        throw std::runtime_error("sourcestart/rawprobe requires sourceprobe");
    }
    if (options.initializationCheck &&
        (options.c != 5 || !options.initializeTwo)) {
        throw std::runtime_error("initcheck requires the C=5 optimized initializer");
    }
    if (options.c == 5 && stop > 3) {
        if (options.sourceProbe == 0 ||
            !options.operatorStateLimitExplicit ||
            !options.operatorRecordLimitExplicit ||
            options.mode == Mode::Direct) {
            throw std::runtime_error(
                "C=5 beyond symbol 3 requires subset/differential, sourceprobe, "
                "and explicit operator limits");
        }
    }
    return options;
}

}  // namespace

int main(int argc, char** argv) {
    try {
        const Options options = parseOptions(argc, argv);
        std::cout << "C=" << options.c
                  << " mode="
                  << (options.mode == Mode::Direct
                          ? "direct"
                          : options.mode == Mode::Subset ? "subset"
                                                         : "differential")
                  << " stop=" << (options.stop < 0 ? 2 * options.c : options.stop)
                  << " threads=" << threadCount()
                  << " source_start=" << options.sourceStart
                  << " source_probe=" << options.sourceProbe
                  << " raw_probe=" << (options.rawProbe ? 1 : 0)
                  << " max_operator_states=" << options.maxOperatorStates
                  << " max_operator_records=" << options.maxOperatorRecords
                  << "\n";
        const RunResult result = run(options);
        std::cout << "states:";
        for (std::size_t states : result.stateCounts) std::cout << ' ' << states;
        std::cout << "\n";
        if (result.complete) {
            std::cout << "N(" << options.c << ")="
                      << countToString(result.total) << "\n";
            if (result.total != knownValue(options.c)) {
                std::cerr << "[FAIL] known-value mismatch\n";
                return 1;
            }
            std::cout << "[OK] known-value gate passed\n";
        } else if (result.probe) {
            std::cout << "[PROBE] exact selected-source operator frontier\n";
        } else {
            std::cout << "[BOUNDED] stopped after requested symbol\n";
        }
        return 0;
    } catch (const ScaleLimit& error) {
        std::cerr << "[SCALE-LIMIT] " << error.what() << "\n";
        return 3;
    } catch (const std::exception& error) {
        std::cerr << "[ERROR] " << error.what() << "\n";
        return 2;
    }
}
