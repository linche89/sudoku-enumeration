// reverse_glue.cpp -- bounded reverse-gluing decision prototype for 2xC.
//
// This program deliberately stays outside the standard build.  For C <= 4 it
// compares two independent computations of every complete outer class:
//
//   1. a labelled-coordinate ordered-factorization oracle; and
//   2. orbit-level L+(C-L) gluing, with an explicit sum over
//      G = C_2 wr S_C and stabilizer-order normalization.
//
// Symbols are already quotiented: a partial configuration is the sorted list
// of the 2C symbol-neighbourhood masks.  A mask contains one vertex from each
// box used by that symbol.  The contingency-table coefficient for a fixed
// complete labelled graph is
//
//            product_w c_w!
//       --------------------------,
//       product_(u,v) n_(u,v)!
//
// not the raw number of bijections between two chosen symbol representatives.
// For orbit representatives x, y and q, the exact normalization is
//
//                         Stab(q)
//   contribution(q) = -------------- * sum_g Join(x, g y -> q).
//                      Stab(x)Stab(y)
//
// Every division is checked exactly.  The C=2..4 totals are retained as hard
// gates, C=5 closes through restartable pair intervals, and C=6 is restricted
// to an exact two-row inventory or positive-limit frontier probes.  No C=6
// checkpoint is read or written.

#include <algorithm>
#include <array>
#include <bit>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <functional>
#include <numeric>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace {

using U128 = unsigned __int128;

constexpr int MAX_C = 6;
constexpr int MAX_M = 2 * MAX_C;
constexpr int MAX_LEFT_MASKS = 1 << MAX_M;

int C = 0;
int M = 0;
int firstRows = 0;
int secondRows = 0;
bool layerProbeOnly = false;
bool cycleLayerCheck = false;
bool rawLayerCountCheck = false;
bool printClasses = true;
uint64_t pairProbeLimit = 0;
uint64_t pairProbeStart = 0;
uint64_t relativePlacementProbeLimit = 0;
uint64_t contingencyLeafProbeLimit = 0;
bool reduceRawOutputs = false;
bool skipProbeCanonicalization = false;
bool placementsOnlyProbe = false;
bool keyOnlyProbe = false;
uint64_t keyVerifyLimit = 0;
uint64_t keyVerifyCompleted = 0;
uint64_t keyOnlyRequests = 0;
uint64_t keyOnlyGroupImages = 0;
size_t anchoredInputCacheCap = 5000000;
bool canonicalCacheCapExplicit = false;
uint64_t canonicalCacheRecycles = 0;
std::string partialOutputPath;
std::vector<std::string> partialInputPaths;

std::array<uint64_t, MAX_M + 1> factorials{};

std::string decimal(U128 value) {
    if (value == 0) return "0";
    std::string out;
    while (value != 0) {
        out.push_back((char)('0' + value % 10));
        value /= 10;
    }
    std::reverse(out.begin(), out.end());
    return out;
}

U128 power(U128 base, int exponent) {
    U128 value = 1;
    while (exponent-- > 0) value *= base;
    return value;
}

uint64_t choose(int n, int k) {
    uint64_t value = 1;
    for (int i = 1; i <= k; ++i) {
        value = value * (uint64_t)(n - k + i) / (uint64_t)i;
    }
    return value;
}

struct Config {
    std::array<uint16_t, MAX_M> masks{};
    bool operator==(const Config&) const = default;
    bool operator<(const Config& other) const noexcept {
        return masks < other.masks;
    }
};

struct ConfigHash {
    size_t operator()(const Config& config) const noexcept {
        uint64_t hash = 0x9e3779b97f4a7c15ULL;
        for (uint16_t mask : config.masks) {
            hash ^= (uint64_t)mask + 0x9e3779b97f4a7c15ULL
                    + (hash << 6) + (hash >> 2);
        }
        return (size_t)hash;
    }
};

std::string configText(const Config& config) {
    std::string out;
    char cell[16];
    for (int symbol = 0; symbol < M; ++symbol) {
        std::snprintf(cell, sizeof(cell), "%s%03x",
                      symbol == 0 ? "" : ",", config.masks[symbol]);
        out += cell;
    }
    return out;
}

struct GroupElement {
    std::array<uint16_t, MAX_M> bitImage{};
};

std::vector<GroupElement> groupElements;
// A dense |G| x 2^(2C) table is only 7.5 MiB at C=5 but jumps to
// 360 MiB at C=6.  Only 60 two-row and 240 four-row masks can occur at C=6,
// so cache image columns lazily instead.
std::vector<std::vector<uint16_t>> groupMaskImageColumns;
int maskDomain = 0;

std::vector<GroupElement> makeSignedCoordinateGroup() {
    std::array<int, MAX_C> permutation{};
    std::iota(permutation.begin(), permutation.begin() + C, 0);
    std::vector<GroupElement> group;
    do {
        for (int flip = 0; flip < (1 << C); ++flip) {
            GroupElement element;
            for (int box = 0; box < C; ++box) {
                for (int side = 0; side < 2; ++side) {
                    const int oldVertex = 2 * box + side;
                    const int newSide = side ^ ((flip >> box) & 1);
                    const int newVertex = 2 * permutation[box] + newSide;
                    element.bitImage[oldVertex] = (uint16_t)(1u << newVertex);
                }
            }
            group.push_back(element);
        }
    } while (std::next_permutation(permutation.begin(), permutation.begin() + C));
    return group;
}

uint16_t transformMask(uint16_t mask, const GroupElement& element) {
    uint16_t result = 0;
    while (mask != 0) {
        const int bit = std::countr_zero(mask);
        mask &= (uint16_t)(mask - 1);
        result |= element.bitImage[bit];
    }
    return result;
}

void buildGroupMaskImages() {
    maskDomain = 1 << M;
    groupMaskImageColumns.clear();
    groupMaskImageColumns.resize((size_t)maskDomain);
}

const std::vector<uint16_t>& maskImageColumn(uint16_t mask) {
    if (mask >= (uint16_t)maskDomain) {
        throw std::runtime_error("mask outside coordinate domain");
    }
    std::vector<uint16_t>& column = groupMaskImageColumns[mask];
    if (column.empty()) {
        column.resize(groupElements.size());
        for (size_t groupIndex = 0; groupIndex < groupElements.size();
             ++groupIndex) {
            column[groupIndex] = transformMask(mask, groupElements[groupIndex]);
        }
    }
    return column;
}

size_t maskImageCacheBytes() {
    size_t bytes = 0;
    for (const auto& column : groupMaskImageColumns) {
        bytes += column.size() * sizeof(column[0]);
    }
    return bytes;
}

void sortMasks(Config& config) {
    // M is runtime-configured.  The explicit MAX_M bound avoids the false
    // array-bounds warning emitted by MinGW's inlined std::sort here.
    for (int i = 1; i < MAX_M && i < M; ++i) {
        const uint16_t value = config.masks[i];
        int j = i;
        while (j > 0 && config.masks[j - 1] > value) {
            config.masks[j] = config.masks[j - 1];
            --j;
        }
        config.masks[j] = value;
    }
}

Config transform(const Config& input, size_t groupIndex) {
    Config output;
    for (int symbol = 0; symbol < M; ++symbol) {
        output.masks[symbol] = maskImageColumn(input.masks[symbol])[groupIndex];
    }
    sortMasks(output);
    return output;
}

struct CanonicalInfo {
    Config representative{};
    uint32_t stabilizer = 0;
};

std::unordered_map<Config, CanonicalInfo, ConfigHash> canonicalCache;
std::unordered_set<Config, ConfigHash> fullyCachedOrbitRepresentatives;
std::unordered_map<Config, uint32_t, ConfigHash> representativeStabilizers;
std::unordered_map<uint32_t, std::vector<uint16_t>> anchorGroupCache;
std::vector<uint32_t> candidateGroupMarks;
uint32_t candidateGroupMarker = 0;
uint64_t canonicalRequests = 0;
uint64_t canonicalHits = 0;
uint64_t canonicalGroupImages = 0;
uint64_t canonicalComputations = 0;

void recycleBoundedCanonicalCacheIfNeeded() {
    if (anchoredInputCacheCap != 0 &&
        canonicalCache.size() >= anchoredInputCacheCap) {
        canonicalCache.clear();
        fullyCachedOrbitRepresentatives.clear();
        ++canonicalCacheRecycles;
    }
}

const std::vector<uint16_t>& groupsMappingMaskTo(uint16_t inputMask,
                                                uint16_t targetMask) {
    const uint32_t key = (uint32_t)targetMask << 16 | inputMask;
    auto found = anchorGroupCache.find(key);
    if (found != anchorGroupCache.end()) return found->second;
    std::vector<uint16_t> groups;
    const std::vector<uint16_t>& images = maskImageColumn(inputMask);
    for (size_t groupIndex = 0; groupIndex < groupElements.size(); ++groupIndex) {
        if (images[groupIndex] == targetMask) {
            groups.push_back((uint16_t)groupIndex);
        }
    }
    return anchorGroupCache.emplace(key, std::move(groups)).first->second;
}

Config canonicalRepresentativeByFirstMask(const Config& input, int rows) {
    uint16_t target = 0;
    for (int box = 0; box < rows; ++box) {
        target |= (uint16_t)(1u << (2 * box));
    }
    if (candidateGroupMarks.size() != groupElements.size()) {
        candidateGroupMarks.assign(groupElements.size(), 0);
        candidateGroupMarker = 0;
    }
    if (++candidateGroupMarker == 0) {
        std::fill(candidateGroupMarks.begin(), candidateGroupMarks.end(), 0);
        candidateGroupMarker = 1;
    }

    std::vector<uint16_t> candidates;
    for (int symbol = 0; symbol < M;) {
        const uint16_t mask = input.masks[symbol];
        const std::vector<uint16_t>& groups = groupsMappingMaskTo(mask, target);
        for (uint16_t groupIndex : groups) {
            if (candidateGroupMarks[groupIndex] != candidateGroupMarker) {
                candidateGroupMarks[groupIndex] = candidateGroupMarker;
                candidates.push_back(groupIndex);
            }
        }
        do {
            ++symbol;
        } while (symbol < M && input.masks[symbol] == mask);
    }
    if (candidates.empty()) {
        throw std::runtime_error("first-mask canonicalization found no candidate");
    }

    ++keyOnlyRequests;
    keyOnlyGroupImages += candidates.size();
    Config representative;
    bool haveRepresentative = false;
    for (uint16_t groupIndex : candidates) {
        const Config image = transform(input, groupIndex);
        if (!haveRepresentative || image < representative) {
            representative = image;
            haveRepresentative = true;
        }
    }
    return representative;
}

uint32_t checkedRepresentativeStabilizer(const Config& representative) {
    auto cached = representativeStabilizers.find(representative);
    if (cached != representativeStabilizers.end()) return cached->second;
    uint32_t stabilizer = 0;
    Config minimum = representative;
    for (size_t groupIndex = 0; groupIndex < groupElements.size(); ++groupIndex) {
        const Config image = transform(representative, groupIndex);
        ++canonicalGroupImages;
        if (image < minimum) minimum = image;
        if (image == representative) ++stabilizer;
    }
    if (minimum != representative || stabilizer == 0 ||
        groupElements.size() % stabilizer != 0) {
        throw std::runtime_error("anchored canonical representative check failed");
    }
    representativeStabilizers.emplace(representative, stabilizer);
    return stabilizer;
}

CanonicalInfo canonicalizeAnchoredC5(const Config& input, int rows) {
    uint16_t target = 0;
    const int targetBoxes = rows == C ? C : C - 1;
    for (int box = 0; box < targetBoxes; ++box) {
        target |= (uint16_t)(1u << (2 * box));
    }

    if (candidateGroupMarks.size() != groupElements.size()) {
        candidateGroupMarks.assign(groupElements.size(), 0);
        candidateGroupMarker = 0;
    }
    if (++candidateGroupMarker == 0) {
        std::fill(candidateGroupMarks.begin(), candidateGroupMarks.end(), 0);
        candidateGroupMarker = 1;
    }

    std::vector<uint16_t> candidates;
    for (int symbol = 0; symbol < M;) {
        const uint16_t mask = input.masks[symbol];
        const std::vector<uint16_t>& groups = groupsMappingMaskTo(mask, target);
        for (uint16_t groupIndex : groups) {
            if (candidateGroupMarks[groupIndex] != candidateGroupMarker) {
                candidateGroupMarks[groupIndex] = candidateGroupMarker;
                candidates.push_back(groupIndex);
            }
        }
        do {
            ++symbol;
        } while (symbol < M && input.masks[symbol] == mask);
    }
    if (candidates.empty()) {
        throw std::runtime_error("anchored canonicalization found no candidate");
    }

    Config representative;
    bool have = false;
    for (uint16_t groupIndex : candidates) {
        const Config image = transform(input, groupIndex);
        ++canonicalGroupImages;
        if (!have || image < representative) {
            representative = image;
            have = true;
        }
    }
    CanonicalInfo result{representative,
                         checkedRepresentativeStabilizer(representative)};
    recycleBoundedCanonicalCacheIfNeeded();
    if (anchoredInputCacheCap != 0) {
        canonicalCache.insert_or_assign(input, result);
    }
    return result;
}

CanonicalInfo canonicalizeImpl(const Config& input, bool cacheWholeOrbit) {
    ++canonicalRequests;
    auto cached = canonicalCache.find(input);
    if (cached != canonicalCache.end() &&
        (!cacheWholeOrbit || fullyCachedOrbitRepresentatives.contains(
             cached->second.representative))) {
        ++canonicalHits;
        return cached->second;
    }

    ++canonicalComputations;
    const int rows = std::popcount(input.masks[0]);
    if (!cacheWholeOrbit && C == 5 && (rows == 4 || rows == 5)) {
        return canonicalizeAnchoredC5(input, rows);
    }

    CanonicalInfo result;
    bool haveRepresentative = false;
    std::vector<Config> images;
    if (cacheWholeOrbit) images.reserve(groupElements.size());
    for (size_t groupIndex = 0; groupIndex < groupElements.size(); ++groupIndex) {
        const Config image = transform(input, groupIndex);
        ++canonicalGroupImages;
        if (cacheWholeOrbit) images.push_back(image);
        if (!haveRepresentative || image < result.representative) {
            result.representative = image;
            haveRepresentative = true;
        }
        if (image == input) ++result.stabilizer;
    }
    if (!haveRepresentative || result.stabilizer == 0 ||
        groupElements.size() % result.stabilizer != 0) {
        throw std::runtime_error("invalid coordinate canonicalization");
    }
    auto [knownStabilizer, insertedStabilizer] =
        representativeStabilizers.emplace(result.representative,
                                           result.stabilizer);
    if (!insertedStabilizer && knownStabilizer->second != result.stabilizer) {
        throw std::runtime_error("inconsistent representative stabilizer");
    }
    if (!cacheWholeOrbit && C >= 5) {
        recycleBoundedCanonicalCacheIfNeeded();
    }
    if (cacheWholeOrbit || anchoredInputCacheCap != 0) {
        canonicalCache.insert_or_assign(input, result);
    }
    if (cacheWholeOrbit) {
        for (const Config& image : images) {
            auto [it, inserted] = canonicalCache.emplace(image, result);
            if (!inserted &&
                (it->second.representative != result.representative ||
                 it->second.stabilizer != result.stabilizer)) {
                throw std::runtime_error("inconsistent cached coordinate orbit");
            }
        }
        fullyCachedOrbitRepresentatives.insert(result.representative);
    }
    return result;
}

CanonicalInfo canonicalize(const Config& input) {
    return canonicalizeImpl(input, false);
}

CanonicalInfo canonicalizeForEnumeration(const Config& input) {
    return canonicalizeImpl(input, true);
}

uint16_t boxSupport(uint16_t mask) {
    uint16_t support = 0;
    for (int box = 0; box < C; ++box) {
        if (mask & (uint16_t)(3u << (2 * box))) support |= (uint16_t)(1u << box);
    }
    return support;
}

bool validOccurrenceMask(uint16_t mask, int rows) {
    if (std::popcount(mask) != rows) return false;
    for (int box = 0; box < C; ++box) {
        if (std::popcount((unsigned)(mask & (uint16_t)(3u << (2 * box)))) > 1) {
            return false;
        }
    }
    return true;
}

using ConfigCallback = std::function<void(const Config&)>;

void enumerateLayer(int rows, const ConfigCallback& callback) {
    std::vector<uint16_t> patterns;
    for (uint16_t mask = 1; mask < (uint16_t)(1u << M); ++mask) {
        if (validOccurrenceMask(mask, rows)) patterns.push_back(mask);
    }

    Config current;
    std::array<uint8_t, MAX_M> degrees{};
    std::function<void(int, int)> recurse = [&](int symbol, int firstPattern) {
        const int remainingAfter = M - symbol - 1;
        for (int patternIndex = firstPattern;
             patternIndex < (int)patterns.size(); ++patternIndex) {
            const uint16_t pattern = patterns[patternIndex];
            bool possible = true;
            for (int vertex = 0; vertex < MAX_M && vertex < M; ++vertex) {
                const int nextDegree = degrees[vertex] + ((pattern >> vertex) & 1u);
                if (nextDegree > rows || nextDegree + remainingAfter < rows) {
                    possible = false;
                    break;
                }
            }
            if (!possible) continue;

            current.masks[symbol] = pattern;
            for (int vertex = 0; vertex < MAX_M && vertex < M; ++vertex) {
                degrees[vertex] += (uint8_t)((pattern >> vertex) & 1u);
            }
            if (symbol + 1 == M) {
                callback(current);
            } else {
                recurse(symbol + 1, patternIndex);
            }
            for (int vertex = 0; vertex < MAX_M && vertex < M; ++vertex) {
                degrees[vertex] -= (uint8_t)((pattern >> vertex) & 1u);
            }
        }
        current.masks[symbol] = 0;
    };
    recurse(0, 0);
}

int componentCount(const Config& config) {
    std::array<int, 2 * MAX_M> parent{};
    for (int vertex = 0; vertex < 2 * M; ++vertex) parent[vertex] = vertex;
    auto root = [&](int vertex) {
        while (parent[vertex] != vertex) {
            parent[vertex] = parent[parent[vertex]];
            vertex = parent[vertex];
        }
        return vertex;
    };

    int components = 2 * M;
    for (int symbol = 0; symbol < M; ++symbol) {
        uint16_t mask = config.masks[symbol];
        while (mask != 0) {
            const int left = std::countr_zero(mask);
            mask &= (uint16_t)(mask - 1);
            int a = root(left);
            int b = root(M + symbol);
            if (a != b) {
                parent[a] = b;
                --components;
            }
        }
    }
    return components;
}

U128 partialFactorizationCount(const Config& config, int rows) {
    if (rows == 1) return 1;
    if (rows == 2) return (U128)1 << componentCount(config);
    throw std::runtime_error("prototype partial factor supports only one or two rows");
}

struct OrbitState {
    Config representative{};
    uint32_t stabilizer = 0;
    uint32_t labelledCoordinateMembers = 0;
    U128 factorizationCount = 0;
};

std::vector<OrbitState> enumerateLayerOrbits(int rows, uint64_t& labelledCount) {
    std::unordered_map<Config, OrbitState, ConfigHash> byRepresentative;
    labelledCount = 0;
    enumerateLayer(rows, [&](const Config& config) {
        ++labelledCount;
        const CanonicalInfo canonical = canonicalizeForEnumeration(config);
        const U128 factor = partialFactorizationCount(config, rows);
        auto [it, inserted] = byRepresentative.emplace(
            canonical.representative,
            OrbitState{canonical.representative, canonical.stabilizer, 0, factor});
        OrbitState& state = it->second;
        ++state.labelledCoordinateMembers;
        if (!inserted && (state.stabilizer != canonical.stabilizer ||
                          state.factorizationCount != factor)) {
            throw std::runtime_error("partial orbit invariant failed");
        }
    });

    std::vector<OrbitState> states;
    states.reserve(byRepresentative.size());
    for (const auto& entry : byRepresentative) {
        OrbitState state = entry.second;
        const size_t expectedOrbit = groupElements.size() / state.stabilizer;
        if (state.labelledCoordinateMembers != expectedOrbit) {
            throw std::runtime_error("partial orbit size failed");
        }
        states.push_back(state);
    }
    std::sort(states.begin(), states.end(),
              [](const OrbitState& a, const OrbitState& b) {
                  return a.representative < b.representative;
              });
    return states;
}

// A two-row configuration is a loopless 2-regular multigraph H on the 2C
// coordinate vertices: every (unlabelled) symbol is one edge.  The fixed box
// pairs form a perfect matching P disjoint from H.  Orbits under C2 wr S_C are
// therefore exactly isomorphism classes of pairs (H,P).  Fixing one canonical
// H for each cycle partition and quotienting admissible P by Aut(H) avoids the
// enormous labelled-coordinate scan at C=6.
struct VertexPermutation {
    std::array<uint8_t, MAX_M> image{};
};

struct Matching {
    std::array<uint8_t, MAX_M> partner{};
};

struct U128Hash {
    size_t operator()(U128 value) const noexcept {
        const uint64_t low = (uint64_t)value;
        const uint64_t high = (uint64_t)(value >> 64);
        uint64_t hash = low ^ (high + 0x9e3779b97f4a7c15ULL
                              + (low << 6) + (low >> 2));
        hash ^= hash >> 30;
        hash *= 0xbf58476d1ce4e5b9ULL;
        hash ^= hash >> 27;
        return (size_t)hash;
    }
};

int pairBitIndex(int first, int second) {
    if (first > second) std::swap(first, second);
    if (first < 0 || second >= M || first == second) {
        throw std::runtime_error("invalid matching edge");
    }
    return first * (2 * M - first - 1) / 2 + second - first - 1;
}

U128 transformedMatchingCode(const Matching& matching,
                             const VertexPermutation& permutation) {
    U128 code = 0;
    for (int vertex = 0; vertex < M; ++vertex) {
        const int partner = matching.partner[vertex];
        if (vertex < partner) {
            const int first = permutation.image[vertex];
            const int second = permutation.image[partner];
            code |= (U128)1 << pairBitIndex(first, second);
        }
    }
    return code;
}

std::vector<VertexPermutation> twoFactorAutomorphisms(
    const std::vector<std::vector<uint8_t>>& cycles) {
    std::vector<VertexPermutation> automorphisms;
    VertexPermutation current;

    std::function<void(size_t)> assignGroup = [&](size_t groupStart) {
        if (groupStart == cycles.size()) {
            automorphisms.push_back(current);
            return;
        }
        size_t groupEnd = groupStart + 1;
        while (groupEnd < cycles.size() &&
               cycles[groupEnd].size() == cycles[groupStart].size()) {
            ++groupEnd;
        }
        std::vector<size_t> targets(groupEnd - groupStart);
        std::iota(targets.begin(), targets.end(), groupStart);
        do {
            std::function<void(size_t)> assignCycle = [&](size_t offset) {
                if (groupStart + offset == groupEnd) {
                    assignGroup(groupEnd);
                    return;
                }
                const auto& source = cycles[groupStart + offset];
                const auto& target = cycles[targets[offset]];
                const int length = (int)source.size();
                const int choices = length == 2 ? 2 : 2 * length;
                for (int choice = 0; choice < choices; ++choice) {
                    const int rotation = choice % length;
                    const bool reverse = length != 2 && choice >= length;
                    for (int position = 0; position < length; ++position) {
                        int targetPosition = reverse ? rotation - position
                                                     : rotation + position;
                        targetPosition %= length;
                        if (targetPosition < 0) targetPosition += length;
                        current.image[source[position]] =
                            target[targetPosition];
                    }
                    assignCycle(offset + 1);
                }
            };
            assignCycle(0);
        } while (std::next_permutation(targets.begin(), targets.end()));
    };
    assignGroup(0);

    std::unordered_set<uint64_t> distinct;
    distinct.reserve(automorphisms.size() * 2 + 1);
    for (const VertexPermutation& permutation : automorphisms) {
        uint64_t code = 0;
        for (int vertex = 0; vertex < M; ++vertex) {
            code |= (uint64_t)permutation.image[vertex] << (4 * vertex);
        }
        if (!distinct.insert(code).second) {
            throw std::runtime_error("duplicate two-factor automorphism");
        }
    }
    return automorphisms;
}

Config twoFactorConfigFromMatching(const Config& factor,
                                   const Matching& matching) {
    std::vector<std::pair<int, int>> boxes;
    boxes.reserve(C);
    for (int vertex = 0; vertex < M; ++vertex) {
        if (vertex < matching.partner[vertex]) {
            boxes.emplace_back(vertex, matching.partner[vertex]);
        }
    }
    if ((int)boxes.size() != C) {
        throw std::runtime_error("invalid perfect matching size");
    }
    std::sort(boxes.begin(), boxes.end());
    std::array<uint8_t, MAX_M> coordinate{};
    for (int box = 0; box < C; ++box) {
        coordinate[boxes[box].first] = (uint8_t)(2 * box);
        coordinate[boxes[box].second] = (uint8_t)(2 * box + 1);
    }

    Config result;
    for (int edge = 0; edge < M; ++edge) {
        uint16_t mask = factor.masks[edge];
        const int first = std::countr_zero(mask);
        mask &= (uint16_t)(mask - 1);
        const int second = std::countr_zero(mask);
        result.masks[edge] = (uint16_t)(
            (1u << coordinate[first]) | (1u << coordinate[second]));
    }
    sortMasks(result);
    return result;
}

std::vector<OrbitState> enumerateTwoRowOrbitsByCycles(
    uint64_t& labelledCount) {
    std::vector<std::vector<int>> partitions;
    std::vector<int> partition;
    std::function<void(int, int)> partitionRecurse =
        [&](int remaining, int minimum) {
        if (remaining == 0) {
            partitions.push_back(partition);
            return;
        }
        for (int part = minimum; part <= remaining; ++part) {
            if (remaining - part != 0 && remaining - part < part) continue;
            partition.push_back(part);
            partitionRecurse(remaining - part, part);
            partition.pop_back();
        }
    };
    partitionRecurse(M, 2);

    std::unordered_map<Config, OrbitState, ConfigHash> statesByRepresentative;
    labelledCount = 0;
    for (const std::vector<int>& cycleLengths : partitions) {
        Config factor;
        std::array<uint16_t, MAX_M> adjacency{};
        std::vector<std::vector<uint8_t>> cycles;
        int nextVertex = 0;
        int nextEdge = 0;
        for (int length : cycleLengths) {
            std::vector<uint8_t> cycle;
            for (int position = 0; position < length; ++position) {
                cycle.push_back((uint8_t)(nextVertex + position));
            }
            cycles.push_back(cycle);
            if (length == 2) {
                const int first = nextVertex;
                const int second = nextVertex + 1;
                const uint16_t edge =
                    (uint16_t)((1u << first) | (1u << second));
                factor.masks[nextEdge++] = edge;
                factor.masks[nextEdge++] = edge;
                adjacency[first] |= (uint16_t)(1u << second);
                adjacency[second] |= (uint16_t)(1u << first);
            } else {
                for (int position = 0; position < length; ++position) {
                    const int first = nextVertex + position;
                    const int second = nextVertex + (position + 1) % length;
                    factor.masks[nextEdge++] = (uint16_t)(
                        (1u << first) | (1u << second));
                    adjacency[first] |= (uint16_t)(1u << second);
                    adjacency[second] |= (uint16_t)(1u << first);
                }
            }
            nextVertex += length;
        }
        if (nextVertex != M || nextEdge != M) {
            throw std::runtime_error("invalid two-factor cycle partition");
        }
        sortMasks(factor);

        std::vector<Matching> matchings;
        Matching currentMatching;
        currentMatching.partner.fill(0xff);
        std::function<void(uint16_t)> matchingRecurse = [&](uint16_t used) {
            if (used == (uint16_t)((1u << M) - 1u)) {
                matchings.push_back(currentMatching);
                return;
            }
            const int first = std::countr_zero((uint16_t)~used);
            const uint16_t firstBit = (uint16_t)(1u << first);
            for (int second = first + 1; second < M; ++second) {
                const uint16_t secondBit = (uint16_t)(1u << second);
                if ((used & secondBit) != 0 ||
                    (adjacency[first] & secondBit) != 0) {
                    continue;
                }
                currentMatching.partner[first] = (uint8_t)second;
                currentMatching.partner[second] = (uint8_t)first;
                matchingRecurse((uint16_t)(used | firstBit | secondBit));
                currentMatching.partner[first] = 0xff;
                currentMatching.partner[second] = 0xff;
            }
        };
        matchingRecurse(0);

        const std::vector<VertexPermutation> automorphisms =
            twoFactorAutomorphisms(cycles);
        std::unordered_map<U128, size_t, U128Hash> matchingIndex;
        matchingIndex.reserve(matchings.size() * 2 + 1);
        const VertexPermutation identity = [&] {
            VertexPermutation value;
            for (int vertex = 0; vertex < M; ++vertex) {
                value.image[vertex] = (uint8_t)vertex;
            }
            return value;
        }();
        for (size_t index = 0; index < matchings.size(); ++index) {
            const U128 code = transformedMatchingCode(matchings[index], identity);
            if (!matchingIndex.emplace(code, index).second) {
                throw std::runtime_error("duplicate admissible perfect matching");
            }
        }

        std::vector<uint8_t> visited(matchings.size(), 0);
        uint64_t matchingOrbits = 0;
        for (size_t index = 0; index < matchings.size(); ++index) {
            if (visited[index]) continue;
            ++matchingOrbits;
            for (const VertexPermutation& automorphism : automorphisms) {
                const U128 imageCode =
                    transformedMatchingCode(matchings[index], automorphism);
                auto found = matchingIndex.find(imageCode);
                if (found == matchingIndex.end()) {
                    throw std::runtime_error(
                        "two-factor automorphism left matching domain");
                }
                visited[found->second] = 1;
            }

            const Config coordinateConfig =
                twoFactorConfigFromMatching(factor, matchings[index]);
            const CanonicalInfo canonical = canonicalize(coordinateConfig);
            const U128 factorization =
                (U128)1 << (int)cycleLengths.size();
            if (partialFactorizationCount(coordinateConfig, 2) != factorization) {
                throw std::runtime_error("two-factor component count mismatch");
            }
            const uint32_t coordinateMembers =
                (uint32_t)(groupElements.size() / canonical.stabilizer);
            auto [state, inserted] = statesByRepresentative.emplace(
                canonical.representative,
                OrbitState{canonical.representative, canonical.stabilizer,
                           coordinateMembers, factorization});
            if (!inserted) {
                throw std::runtime_error(
                    "duplicate two-row orbit across matching quotients");
            }
            labelledCount += coordinateMembers;
        }
        std::printf("two-row-cycle-partition parts=");
        for (size_t part = 0; part < cycleLengths.size(); ++part) {
            std::printf("%s%d", part == 0 ? "" : "+", cycleLengths[part]);
        }
        std::printf(" matchings=%zu aut=%zu matching_orbits=%llu\n",
                    matchings.size(), automorphisms.size(),
                    (unsigned long long)matchingOrbits);
    }

    std::vector<OrbitState> states;
    states.reserve(statesByRepresentative.size());
    for (const auto& entry : statesByRepresentative) states.push_back(entry.second);
    std::sort(states.begin(), states.end(),
              [](const OrbitState& first, const OrbitState& second) {
                  return first.representative < second.representative;
              });
    return states;
}

void requireEqualOrbitStates(const std::vector<OrbitState>& first,
                             const std::vector<OrbitState>& second,
                             uint64_t firstLabelled,
                             uint64_t secondLabelled) {
    if (firstLabelled != secondLabelled || first.size() != second.size()) {
        throw std::runtime_error("two-row cycle/raw layer size mismatch");
    }
    for (size_t index = 0; index < first.size(); ++index) {
        const OrbitState& a = first[index];
        const OrbitState& b = second[index];
        if (a.representative != b.representative ||
            a.stabilizer != b.stabilizer ||
            a.labelledCoordinateMembers != b.labelledCoordinateMembers ||
            a.factorizationCount != b.factorizationCount) {
            throw std::runtime_error("two-row cycle/raw orbit mismatch");
        }
    }
}

struct PatternType {
    uint16_t mask = 0;
    uint8_t count = 0;
};

std::vector<PatternType> patternTypes(const Config& config) {
    std::vector<PatternType> result;
    for (int symbol = 0; symbol < M;) {
        int end = symbol + 1;
        while (end < M && config.masks[end] == config.masks[symbol]) ++end;
        result.push_back({config.masks[symbol], (uint8_t)(end - symbol)});
        symbol = end;
    }
    return result;
}

struct GlueStats {
    uint64_t calls = 0;
    uint64_t contingencyLeaves = 0;
    uint64_t incompatibleCalls = 0;
    uint64_t maximumLeavesPerCall = 0;
    uint32_t maximumFirstTypes = 0;
    uint32_t maximumSecondTypes = 0;
    bool probeTruncated = false;
};

using GlueCallback = std::function<void(const Config&, uint64_t)>;

void glue(const Config& first, const Config& second, int outputRows,
          GlueStats& stats, const GlueCallback& callback) {
    ++stats.calls;
    const uint64_t leavesBefore = stats.contingencyLeaves;
    const std::vector<PatternType> a = patternTypes(first);
    const std::vector<PatternType> b = patternTypes(second);
    stats.maximumFirstTypes = std::max<uint32_t>(
        stats.maximumFirstTypes, (uint32_t)a.size());
    stats.maximumSecondTypes = std::max<uint32_t>(
        stats.maximumSecondTypes, (uint32_t)b.size());
    std::array<uint8_t, MAX_M> remainingB{};
    for (int v = 0; v < (int)b.size(); ++v) remainingB[v] = b[v].count;
    std::array<uint8_t, MAX_LEFT_MASKS> outputCounts{};

    bool anyCompatibility = true;
    for (const PatternType& left : a) {
        int capacity = 0;
        for (const PatternType& right : b) {
            if ((boxSupport(left.mask) & boxSupport(right.mask)) == 0) {
                capacity += right.count;
            }
        }
        if (capacity < left.count) {
            anyCompatibility = false;
            break;
        }
    }
    if (!anyCompatibility) {
        ++stats.incompatibleCalls;
        return;
    }

    std::function<void(int, uint64_t)> rows;
    rows = [&](int u, uint64_t denominator) {
        if (stats.probeTruncated) return;
        if (u == (int)a.size()) {
            for (int v = 0; v < (int)b.size(); ++v) {
                if (remainingB[v] != 0) return;
            }
            uint64_t numerator = 1;
            Config output;
            int symbol = 0;
            for (int mask = 0; mask < (1 << M); ++mask) {
                const int count = outputCounts[mask];
                numerator *= factorials[count];
                for (int copy = 0; copy < count; ++copy) {
                    if (symbol >= M ||
                        !validOccurrenceMask((uint16_t)mask, outputRows)) {
                        throw std::runtime_error("invalid complete gluing output");
                    }
                    output.masks[symbol++] = (uint16_t)mask;
                }
            }
            if (symbol != M || denominator == 0 || numerator % denominator != 0) {
                throw std::runtime_error("invalid contingency coefficient");
            }
            if (contingencyLeafProbeLimit != 0 &&
                stats.contingencyLeaves >= contingencyLeafProbeLimit) {
                stats.probeTruncated = true;
                return;
            }
            ++stats.contingencyLeaves;
            callback(output, numerator / denominator);
            return;
        }

        std::function<void(int, int, uint64_t)> columns;
        columns = [&](int v, int stillNeeded, uint64_t rowDenominator) {
            if (stats.probeTruncated) return;
            if (v == (int)b.size()) {
                if (stillNeeded == 0) rows(u + 1, rowDenominator);
                return;
            }

            int laterCapacity = 0;
            for (int w = v + 1; w < (int)b.size(); ++w) {
                if ((boxSupport(a[u].mask) & boxSupport(b[w].mask)) == 0) {
                    laterCapacity += remainingB[w];
                }
            }
            const bool compatible =
                (boxSupport(a[u].mask) & boxSupport(b[v].mask)) == 0;
            const int maximum = compatible
                ? std::min<int>(stillNeeded, remainingB[v]) : 0;
            const int minimum = std::max(0, stillNeeded - laterCapacity);
            if (minimum > maximum) return;

            for (int take = minimum; take <= maximum; ++take) {
                const uint16_t united = (uint16_t)(a[u].mask | b[v].mask);
                remainingB[v] -= (uint8_t)take;
                if (take != 0) outputCounts[united] += (uint8_t)take;
                columns(v + 1, stillNeeded - take,
                        rowDenominator * factorials[take]);
                if (take != 0) outputCounts[united] -= (uint8_t)take;
                remainingB[v] += (uint8_t)take;
                if (stats.probeTruncated) return;
            }
        };
        columns(0, a[u].count, denominator);
    };
    rows(0, 1);
    stats.maximumLeavesPerCall = std::max(
        stats.maximumLeavesPerCall, stats.contingencyLeaves - leavesBefore);
}

struct OrbitGlueResult {
    std::unordered_map<Config, U128, ConfigHash> factorByClass;
    GlueStats glueStats{};
    uint64_t orbitPairs = 0;
    uint64_t orbitPairsStarted = 0;
    uint64_t groupApplications = 0;
    uint64_t relativePlacementMass = 0;
    uint64_t rawReducedRecords = 0;
    uint64_t pairReducedRecords = 0;
    uint64_t probePartialCanonicalRecords = 0;
    bool complete = true;
};

OrbitGlueResult orbitGlue(const std::vector<OrbitState>& first,
                          const std::vector<OrbitState>& second,
                          int outputRows,
                          bool symmetricPairs,
                          uint64_t pairStart,
                          uint64_t pairLimit = 0,
                          const char* progressLabel = nullptr) {
    OrbitGlueResult result;
    if (pairStart != 0) result.complete = false;
    const auto started = std::chrono::steady_clock::now();
    std::vector<std::vector<Config>> secondOrbitImages(second.size());
    std::vector<std::unordered_map<Config, uint16_t, ConfigHash>>
        secondOrbitImageIndex(second.size());
    auto ensureSecondOrbit = [&](size_t yIndex) {
        std::vector<Config>& images = secondOrbitImages[yIndex];
        if (!images.empty()) return;
        images.reserve(groupElements.size());
        for (size_t groupIndex = 0; groupIndex < groupElements.size(); ++groupIndex) {
            images.push_back(transform(second[yIndex].representative, groupIndex));
        }
        std::sort(images.begin(), images.end());
        images.erase(std::unique(images.begin(), images.end()), images.end());
        if (images.size() !=
            groupElements.size() / second[yIndex].stabilizer) {
            throw std::runtime_error("relative orbit image count failed");
        }
        auto& index = secondOrbitImageIndex[yIndex];
        index.reserve(images.size() * 5 / 4 + 1);
        for (size_t imageIndex = 0; imageIndex < images.size(); ++imageIndex) {
            index.emplace(images[imageIndex], (uint16_t)imageIndex);
        }
    };
    if (symmetricPairs && first.size() != second.size()) {
        throw std::runtime_error("symmetric gluing requires equal orbit layers");
    }
    const size_t totalPairs = symmetricPairs
        ? first.size() * (first.size() + 1) / 2
        : first.size() * second.size();
    const uint64_t progressInterval = totalPairs > 10000 ? 1000 : 100;
    uint64_t pairOrdinal = 0;
    for (size_t xIndex = 0; xIndex < first.size(); ++xIndex) {
        const OrbitState& x = first[xIndex];
        std::vector<uint16_t> stabilizerElements;
        stabilizerElements.reserve(x.stabilizer);
        for (size_t groupIndex = 0; groupIndex < groupElements.size(); ++groupIndex) {
            if (transform(x.representative, groupIndex) == x.representative) {
                stabilizerElements.push_back((uint16_t)groupIndex);
            }
        }
        if (stabilizerElements.size() != x.stabilizer) {
            throw std::runtime_error("left stabilizer enumeration failed");
        }
        const size_t firstY = symmetricPairs ? xIndex : 0;
        for (size_t yIndex = firstY; yIndex < second.size(); ++yIndex) {
            const OrbitState& y = second[yIndex];
            const uint64_t currentPairOrdinal = pairOrdinal++;
            if (currentPairOrdinal < pairStart) continue;
            if (pairLimit != 0 && result.orbitPairs >= pairLimit) {
                result.complete = false;
                return result;
            }
            ++result.orbitPairsStarted;
            if (C == 6) {
                std::printf("C=6 pair-start ordinal=%llu x=%zu y=%zu "
                            "x_stab=%u y_stab=%u x_orbit=%u y_orbit=%u\n",
                            (unsigned long long)currentPairOrdinal,
                            xIndex, yIndex, x.stabilizer, y.stabilizer,
                            x.labelledCoordinateMembers,
                            y.labelledCoordinateMembers);
            }
            std::unordered_map<Config, U128, ConfigHash> groupSum;
            std::unordered_map<Config, U128, ConfigHash> rawGroupSum;
            auto flushRawGroupSum = [&] {
                if (rawGroupSum.empty()) return;
                result.rawReducedRecords += rawGroupSum.size();
                if (skipProbeCanonicalization) {
                    rawGroupSum.clear();
                    return;
                }
                for (const auto& raw : rawGroupSum) {
                    if (keyOnlyProbe) {
                        const Config representative =
                            canonicalRepresentativeByFirstMask(raw.first,
                                                               outputRows);
                        if (keyVerifyCompleted < keyVerifyLimit) {
                            const CanonicalInfo full = canonicalize(raw.first);
                            if (representative != full.representative) {
                                throw std::runtime_error(
                                    "key-only/full canonical mismatch");
                            }
                            ++keyVerifyCompleted;
                        }
                        groupSum[representative] += raw.second;
                    } else {
                        const CanonicalInfo q = canonicalize(raw.first);
                        groupSum[q.representative] += raw.second;
                    }
                }
                rawGroupSum.clear();
            };
            ensureSecondOrbit(yIndex);
            const std::vector<Config>& yImages = secondOrbitImages[yIndex];
            const auto& yImageIndex = secondOrbitImageIndex[yIndex];
            std::vector<uint8_t> visited(yImages.size(), 0);
            for (size_t imageIndex = 0; imageIndex < yImages.size(); ++imageIndex) {
                if (visited[imageIndex]) continue;
                if (relativePlacementProbeLimit != 0 &&
                    result.groupApplications >= relativePlacementProbeLimit) {
                    result.probePartialCanonicalRecords += groupSum.size();
                    result.complete = false;
                    return result;
                }
                uint32_t placementMultiplicity = 0;
                for (uint16_t groupIndex : stabilizerElements) {
                    const Config image = transform(yImages[imageIndex], groupIndex);
                    auto foundImage = yImageIndex.find(image);
                    if (foundImage == yImageIndex.end()) {
                        throw std::runtime_error("left stabilizer left relative orbit");
                    }
                    if (!visited[foundImage->second]) {
                        visited[foundImage->second] = 1;
                        ++placementMultiplicity;
                    }
                }
                if (placementMultiplicity == 0) {
                    throw std::runtime_error("empty relative double coset");
                }
                ++result.groupApplications;
                result.relativePlacementMass += placementMultiplicity;
                if (placementsOnlyProbe) continue;
                glue(x.representative, yImages[imageIndex], outputRows,
                     result.glueStats,
                     [&](const Config& output, uint64_t coefficient) {
                    const U128 value =
                        (U128)coefficient * x.factorizationCount
                                           * y.factorizationCount
                                           * placementMultiplicity;
                    if (reduceRawOutputs) {
                        rawGroupSum[output] += value;
                    } else {
                        const CanonicalInfo q = canonicalize(output);
                        groupSum[q.representative] += value;
                    }
                });
                // C=6 probes flush per relative placement.  This keeps the
                // raw map bounded and measures canonicalization even when an
                // inner safety limit stops the pair before exact closure.
                if (reduceRawOutputs && C == 6) flushRawGroupSum();
                if (result.glueStats.probeTruncated) {
                    result.probePartialCanonicalRecords += groupSum.size();
                    result.complete = false;
                    return result;
                }
            }
            if (reduceRawOutputs) {
                flushRawGroupSum();
            }
            result.pairReducedRecords += groupSum.size();
            if (C == 6 && keyOnlyProbe) {
                ++result.orbitPairs;
                continue;
            }
            for (const auto& entry : groupSum) {
                const CanonicalInfo q = canonicalize(entry.first);
                const U128 numerator = entry.second * q.stabilizer;
                // Each distinct coordinate image of y represents Stab(y)
                // group elements.  That factor cancels the Stab(y) in the
                // full group-sum normalization, leaving only Stab(x).
                const uint64_t denominator = x.stabilizer;
                if (denominator == 0 || numerator % denominator != 0) {
                    throw std::runtime_error(
                        "nonintegral orbit/stabilizer gluing normalization");
                }
                const uint64_t pairMultiplicity =
                    symmetricPairs && xIndex != yIndex ? 2 : 1;
                result.factorByClass[entry.first] +=
                    (numerator / denominator) * pairMultiplicity;
            }
            ++result.orbitPairs;
            if (progressLabel != nullptr &&
                result.orbitPairs % progressInterval == 0) {
                const double seconds = std::chrono::duration<double>(
                    std::chrono::steady_clock::now() - started).count();
                std::fprintf(stderr,
                    "reverse-glue progress stage=%s pairs=%llu/%zu "
                    "group_applications=%llu contingencies=%llu classes=%zu "
                    "time=%.3fs\n",
                    progressLabel,
                    (unsigned long long)result.orbitPairs,
                    totalPairs,
                    (unsigned long long)result.groupApplications,
                    (unsigned long long)result.glueStats.contingencyLeaves,
                    result.factorByClass.size(), seconds);
                std::fflush(stderr);
            }
        }
    }
    return result;
}

struct GlueMeasurement {
    OrbitGlueResult result{};
    double seconds = 0.0;
    uint64_t canonicalRequests = 0;
    uint64_t canonicalHits = 0;
    uint64_t canonicalGroupImages = 0;
    uint64_t canonicalComputations = 0;
    int outputRows = 0;
};

GlueMeasurement measureOrbitGlue(const std::vector<OrbitState>& first,
                                 const std::vector<OrbitState>& second,
                                 int outputRows,
                                 bool symmetricPairs,
                                 uint64_t pairStart,
                                 uint64_t pairLimit,
                                 const char* progressLabel) {
    const uint64_t requestsBefore = canonicalRequests;
    const uint64_t hitsBefore = canonicalHits;
    const uint64_t imagesBefore = canonicalGroupImages;
    const uint64_t computationsBefore = canonicalComputations;
    const auto started = std::chrono::steady_clock::now();
    GlueMeasurement measurement;
    measurement.outputRows = outputRows;
    measurement.result = orbitGlue(
        first, second, outputRows, symmetricPairs,
        pairStart, pairLimit, progressLabel);
    measurement.seconds = std::chrono::duration<double>(
        std::chrono::steady_clock::now() - started).count();
    measurement.canonicalRequests = canonicalRequests - requestsBefore;
    measurement.canonicalHits = canonicalHits - hitsBefore;
    measurement.canonicalGroupImages = canonicalGroupImages - imagesBefore;
    measurement.canonicalComputations =
        canonicalComputations - computationsBefore;
    return measurement;
}

void printGlueMeasurement(const char* label, const GlueMeasurement& measurement) {
    const OrbitGlueResult& glued = measurement.result;
    const double duplicateRatio = glued.pairReducedRecords == 0 ? 0.0 :
        (double)glued.glueStats.contingencyLeaves /
        (double)glued.pairReducedRecords;
    const double rawDuplicateRatio = glued.rawReducedRecords == 0 ? 0.0 :
        (double)glued.glueStats.contingencyLeaves /
        (double)glued.rawReducedRecords;
    const double groupRate = measurement.seconds == 0.0 ? 0.0 :
        (double)glued.groupApplications / measurement.seconds;
    const double outputRate = measurement.seconds == 0.0 ? 0.0 :
        (double)glued.glueStats.contingencyLeaves / measurement.seconds;
    const uint64_t coldCanonicalizations = measurement.canonicalComputations;
    const double coldRate = measurement.seconds == 0.0 ? 0.0 :
        (double)coldCanonicalizations / measurement.seconds;
    uint64_t discoveredCoordinateMass = 0;
    uint64_t discoveredOrbits = 0;
    for (const auto& entry : representativeStabilizers) {
        if (std::popcount(entry.first.masks[0]) == measurement.outputRows) {
            ++discoveredOrbits;
            discoveredCoordinateMass += groupElements.size() / entry.second;
        }
    }
    std::printf(
        "glue stage=%s complete=%s orbit_pairs=%llu pairs_started=%llu "
        "group_applications=%llu "
        "relative_mass=%llu calls=%llu contingencies=%llu incompatible_calls=%llu "
        "raw_reduced=%llu pair_reduced=%llu partial_canonical=%llu classes=%zu "
        "raw_per_raw_reduced=%.3f raw_per_pair_reduced=%.3f "
        "max_contingencies_per_call=%llu max_types=%u,%u time=%.6fs "
        "group_applications_per_second=%.3f output_records_per_second=%.3f\n",
        label, glued.complete ? "yes" : "no",
        (unsigned long long)glued.orbitPairs,
        (unsigned long long)glued.orbitPairsStarted,
        (unsigned long long)glued.groupApplications,
        (unsigned long long)glued.relativePlacementMass,
        (unsigned long long)glued.glueStats.calls,
        (unsigned long long)glued.glueStats.contingencyLeaves,
        (unsigned long long)glued.glueStats.incompatibleCalls,
        (unsigned long long)glued.rawReducedRecords,
        (unsigned long long)glued.pairReducedRecords,
        (unsigned long long)glued.probePartialCanonicalRecords,
        glued.factorByClass.size(), rawDuplicateRatio, duplicateRatio,
        (unsigned long long)glued.glueStats.maximumLeavesPerCall,
        glued.glueStats.maximumFirstTypes,
        glued.glueStats.maximumSecondTypes,
        measurement.seconds, groupRate, outputRate);
    std::printf("probe stage=%s placement_limit=%llu leaf_limit=%llu "
                "leaf_truncated=%s skip_canonical=%s placements_only=%s "
                "key_only=%s key_requests=%llu key_group_images=%llu "
                "key_verified=%llu key_per_second=%.3f\n",
                label,
                (unsigned long long)relativePlacementProbeLimit,
                (unsigned long long)contingencyLeafProbeLimit,
                glued.glueStats.probeTruncated ? "yes" : "no",
                skipProbeCanonicalization ? "yes" : "no",
                placementsOnlyProbe ? "yes" : "no",
                keyOnlyProbe ? "yes" : "no",
                (unsigned long long)keyOnlyRequests,
                (unsigned long long)keyOnlyGroupImages,
                (unsigned long long)keyVerifyCompleted,
                measurement.seconds == 0.0 ? 0.0 :
                    (double)keyOnlyRequests / measurement.seconds);
    std::printf(
        "canonical stage=%s requests=%llu hits=%llu cold=%llu "
        "group_images=%llu cold_per_second=%.3f cache=%zu cache_cap=%zu "
        "cache_recycles=%llu discovered_orbits=%llu "
        "discovered_coordinate_mass=%llu\n",
        label,
        (unsigned long long)measurement.canonicalRequests,
        (unsigned long long)measurement.canonicalHits,
        (unsigned long long)coldCanonicalizations,
        (unsigned long long)measurement.canonicalGroupImages,
        coldRate, canonicalCache.size(), anchoredInputCacheCap,
        (unsigned long long)canonicalCacheRecycles,
        (unsigned long long)discoveredOrbits,
        (unsigned long long)discoveredCoordinateMass);
}

struct Graph {
    std::array<uint16_t, MAX_M> adjacency{};
    uint8_t degree = 0;
    bool operator==(const Graph&) const = default;
};

struct GraphHash {
    size_t operator()(const Graph& graph) const noexcept {
        uint64_t hash = graph.degree;
        for (uint16_t row : graph.adjacency) {
            hash ^= (uint64_t)row + 0x9e3779b97f4a7c15ULL
                    + (hash << 6) + (hash >> 2);
        }
        return (size_t)hash;
    }
};

Graph graphFromConfig(const Config& config, int degree) {
    Graph graph;
    graph.degree = (uint8_t)degree;
    for (int symbol = 0; symbol < M; ++symbol) {
        uint16_t mask = config.masks[symbol];
        while (mask != 0) {
            const int left = std::countr_zero(mask);
            mask &= (uint16_t)(mask - 1);
            graph.adjacency[left] |= (uint16_t)(1u << symbol);
        }
    }
    for (int left = 0; left < M; ++left) {
        if (std::popcount(graph.adjacency[left]) != degree) {
            throw std::runtime_error("nonregular graph from configuration");
        }
    }
    return graph;
}

int graphComponents(const Graph& graph) {
    std::array<int, 2 * MAX_M> parent{};
    for (int vertex = 0; vertex < 2 * M; ++vertex) parent[vertex] = vertex;
    auto root = [&](int vertex) {
        while (parent[vertex] != vertex) {
            parent[vertex] = parent[parent[vertex]];
            vertex = parent[vertex];
        }
        return vertex;
    };
    int components = 2 * M;
    for (int left = 0; left < M; ++left) {
        uint16_t bits = graph.adjacency[left];
        while (bits != 0) {
            const int right = std::countr_zero(bits);
            bits &= (uint16_t)(bits - 1);
            int a = root(left);
            int b = root(M + right);
            if (a != b) {
                parent[a] = b;
                --components;
            }
        }
    }
    return components;
}

std::unordered_map<Graph, U128, GraphHash> factorMemo;
uint64_t factorCalls = 0;
uint64_t factorMemoHits = 0;
uint64_t factorMatchingLeaves = 0;

U128 factorGraph(const Graph& graph) {
    ++factorCalls;
    if (graph.degree <= 1) return 1;
    if (graph.degree == 2) return (U128)1 << graphComponents(graph);
    auto found = factorMemo.find(graph);
    if (found != factorMemo.end()) {
        ++factorMemoHits;
        return found->second;
    }

    U128 total = 0;
    std::array<uint16_t, MAX_M> chosen{};
    std::function<void(int, uint16_t)> match = [&](int left, uint16_t used) {
        if (left == M) {
            ++factorMatchingLeaves;
            Graph child = graph;
            --child.degree;
            for (int row = 0; row < M; ++row) {
                child.adjacency[row] ^= chosen[row];
            }
            total += factorGraph(child);
            return;
        }
        uint16_t available = graph.adjacency[left] & (uint16_t)~used;
        while (available != 0) {
            const uint16_t bit = available & (uint16_t)(-available);
            available ^= bit;
            chosen[left] = bit;
            match(left + 1, (uint16_t)(used | bit));
        }
    };
    match(0, 0);
    factorMemo.emplace(graph, total);
    return total;
}

struct CompleteClass {
    Config representative{};
    uint32_t stabilizer = 0;
    uint32_t labelledCoordinateMembers = 0;
};

std::vector<CompleteClass> enumerateCompleteClasses(uint64_t& labelledCount) {
    std::unordered_map<Config, CompleteClass, ConfigHash> classes;
    labelledCount = 0;
    enumerateLayer(C, [&](const Config& config) {
        ++labelledCount;
        const CanonicalInfo canonical = canonicalizeForEnumeration(config);
        auto [it, inserted] = classes.emplace(
            canonical.representative,
            CompleteClass{canonical.representative, canonical.stabilizer, 0});
        CompleteClass& complete = it->second;
        ++complete.labelledCoordinateMembers;
        if (!inserted && complete.stabilizer != canonical.stabilizer) {
            throw std::runtime_error("complete orbit invariant failed");
        }
    });

    std::vector<CompleteClass> result;
    result.reserve(classes.size());
    for (const auto& entry : classes) {
        CompleteClass complete = entry.second;
        const size_t expectedOrbit = groupElements.size() / complete.stabilizer;
        if (complete.labelledCoordinateMembers != expectedOrbit) {
            throw std::runtime_error("complete coordinate orbit size failed");
        }
        result.push_back(complete);
    }
    std::sort(result.begin(), result.end(),
              [](const CompleteClass& a, const CompleteClass& b) {
                  return a.representative < b.representative;
              });
    return result;
}

uint64_t symbolLabellings(const Config& config) {
    uint64_t result = factorials[M];
    for (int symbol = 0; symbol < M;) {
        int end = symbol + 1;
        while (end < M && config.masks[end] == config.masks[symbol]) ++end;
        result /= factorials[end - symbol];
        symbol = end;
    }
    return result;
}

int expectedClassCount(int c) {
    if (c == 2) return 2;
    if (c == 3) return 4;
    if (c == 4) return 26;
    if (c == 5) return 355;
    return -1;
}

std::string expectedTotal(int c) {
    if (c == 2) return "288";
    if (c == 3) return "28200960";
    if (c == 4) return "29136487207403520";
    if (c == 5) return "1903816047972624930994913280000";
    return {};
}

std::vector<OrbitState> orbitStatesFromGlue(
    const std::unordered_map<Config, U128, ConfigHash>& values) {
    std::vector<OrbitState> states;
    states.reserve(values.size());
    for (const auto& entry : values) {
        const CanonicalInfo canonical = canonicalize(entry.first);
        if (canonical.representative != entry.first || entry.second == 0) {
            throw std::runtime_error("invalid intermediate glued orbit");
        }
        states.push_back({entry.first, canonical.stabilizer,
                          (uint32_t)(groupElements.size() / canonical.stabilizer),
                          entry.second});
    }
    std::sort(states.begin(), states.end(),
              [](const OrbitState& a, const OrbitState& b) {
                  return a.representative < b.representative;
              });
    return states;
}

struct PartialLayerFile {
    uint64_t pairStart = 0;
    uint64_t pairCount = 0;
    uint64_t totalPairs = 0;
    std::unordered_map<Config, U128, ConfigHash> values;
};

void writePartialLayerFile(
    const std::string& path, uint64_t pairStart, uint64_t pairCount,
    uint64_t totalPairs,
    const std::unordered_map<Config, U128, ConfigHash>& values) {
    if (path.empty()) return;
    if (std::filesystem::exists(path)) {
        throw std::runtime_error("partial output already exists: " + path);
    }
    const std::string temporary = path + ".tmp";
    if (std::filesystem::exists(temporary)) {
        throw std::runtime_error("partial temporary output already exists: " + temporary);
    }
    const std::filesystem::path parent = std::filesystem::path(path).parent_path();
    if (!parent.empty()) std::filesystem::create_directories(parent);

    std::vector<std::pair<Config, U128>> records(values.begin(), values.end());
    std::sort(records.begin(), records.end(),
              [](const auto& a, const auto& b) { return a.first < b.first; });
    std::ofstream output(temporary, std::ios::binary | std::ios::trunc);
    if (!output) throw std::runtime_error("cannot create partial output: " + temporary);
    const char magic[8] = {'R','J','G','L','U','0','1','\0'};
    const uint32_t version = 1;
    const uint32_t storedC = (uint32_t)C;
    const uint32_t outputRows = 4;
    const uint32_t storedM = (uint32_t)M;
    const uint64_t recordCount = records.size();
    output.write(magic, sizeof(magic));
    output.write((const char*)&version, sizeof(version));
    output.write((const char*)&storedC, sizeof(storedC));
    output.write((const char*)&outputRows, sizeof(outputRows));
    output.write((const char*)&storedM, sizeof(storedM));
    output.write((const char*)&pairStart, sizeof(pairStart));
    output.write((const char*)&pairCount, sizeof(pairCount));
    output.write((const char*)&totalPairs, sizeof(totalPairs));
    output.write((const char*)&recordCount, sizeof(recordCount));
    for (const auto& record : records) {
        output.write((const char*)record.first.masks.data(),
                     sizeof(record.first.masks));
        const uint64_t low = (uint64_t)record.second;
        const uint64_t high = (uint64_t)(record.second >> 64);
        output.write((const char*)&low, sizeof(low));
        output.write((const char*)&high, sizeof(high));
    }
    output.close();
    if (!output) throw std::runtime_error("failed writing partial output: " + temporary);
    std::filesystem::rename(temporary, path);
    std::printf("partial-output path=%s start=%llu count=%llu total=%llu "
                "records=%llu bytes=%llu\n",
                path.c_str(), (unsigned long long)pairStart,
                (unsigned long long)pairCount,
                (unsigned long long)totalPairs,
                (unsigned long long)recordCount,
                (unsigned long long)std::filesystem::file_size(path));
    std::fflush(stdout);
}

PartialLayerFile readPartialLayerFile(const std::string& path) {
    std::ifstream input(path, std::ios::binary);
    if (!input) throw std::runtime_error("cannot open partial input: " + path);
    char magic[8]{};
    uint32_t version = 0;
    uint32_t storedC = 0;
    uint32_t outputRows = 0;
    uint32_t storedM = 0;
    uint64_t recordCount = 0;
    PartialLayerFile file;
    input.read(magic, sizeof(magic));
    input.read((char*)&version, sizeof(version));
    input.read((char*)&storedC, sizeof(storedC));
    input.read((char*)&outputRows, sizeof(outputRows));
    input.read((char*)&storedM, sizeof(storedM));
    input.read((char*)&file.pairStart, sizeof(file.pairStart));
    input.read((char*)&file.pairCount, sizeof(file.pairCount));
    input.read((char*)&file.totalPairs, sizeof(file.totalPairs));
    input.read((char*)&recordCount, sizeof(recordCount));
    if (!input || std::memcmp(magic, "RJGLU01", 7) != 0 || version != 1 ||
        storedC != (uint32_t)C || outputRows != 4 || storedM != (uint32_t)M ||
        recordCount > 1000000) {
        throw std::runtime_error("invalid partial input header: " + path);
    }
    file.values.reserve((size_t)(recordCount * 5 / 4 + 1));
    Config previous;
    bool havePrevious = false;
    for (uint64_t recordIndex = 0; recordIndex < recordCount; ++recordIndex) {
        Config config;
        uint64_t low = 0;
        uint64_t high = 0;
        input.read((char*)config.masks.data(), sizeof(config.masks));
        input.read((char*)&low, sizeof(low));
        input.read((char*)&high, sizeof(high));
        if (!input || (havePrevious && !(previous < config))) {
            throw std::runtime_error("unsorted/truncated partial input: " + path);
        }
        for (int symbol = 0; symbol < M; ++symbol) {
            if (!validOccurrenceMask(config.masks[symbol], 4)) {
                throw std::runtime_error("invalid partial record mask: " + path);
            }
        }
        for (int symbol = M; symbol < MAX_M; ++symbol) {
            if (config.masks[symbol] != 0) {
                throw std::runtime_error("invalid partial record padding: " + path);
            }
        }
        const U128 value = (U128)low | ((U128)high << 64);
        if (value == 0 || !file.values.emplace(config, value).second) {
            throw std::runtime_error("invalid/duplicate partial value: " + path);
        }
        previous = config;
        havePrevious = true;
    }
    char extra = 0;
    if (input.read(&extra, 1)) {
        throw std::runtime_error("trailing bytes in partial input: " + path);
    }
    if (!input.eof()) throw std::runtime_error("partial input read failure: " + path);
    std::printf("partial-input path=%s start=%llu count=%llu total=%llu "
                "records=%llu bytes=%llu\n",
                path.c_str(), (unsigned long long)file.pairStart,
                (unsigned long long)file.pairCount,
                (unsigned long long)file.totalPairs,
                (unsigned long long)recordCount,
                (unsigned long long)std::filesystem::file_size(path));
    return file;
}

std::unordered_map<Config, U128, ConfigHash> mergePartialLayerFiles(
    uint64_t expectedTotalPairs) {
    std::vector<PartialLayerFile> files;
    files.reserve(partialInputPaths.size());
    for (const std::string& path : partialInputPaths) {
        files.push_back(readPartialLayerFile(path));
    }
    std::sort(files.begin(), files.end(), [](const PartialLayerFile& a,
                                             const PartialLayerFile& b) {
        return a.pairStart < b.pairStart;
    });
    uint64_t nextPair = 0;
    std::unordered_map<Config, U128, ConfigHash> merged;
    merged.reserve(25000);
    for (const PartialLayerFile& file : files) {
        if (file.totalPairs != expectedTotalPairs || file.pairStart != nextPair ||
            file.pairCount == 0 || file.pairCount > expectedTotalPairs - nextPair) {
            throw std::runtime_error("partial inputs have a gap, overlap, or identity mismatch");
        }
        for (const auto& entry : file.values) merged[entry.first] += entry.second;
        nextPair += file.pairCount;
    }
    if (nextPair != expectedTotalPairs) {
        throw std::runtime_error("partial inputs do not cover the complete pair interval");
    }
    std::printf("partial-merge files=%zu pairs=%llu records=%zu [COVERAGE OK]\n",
                files.size(), (unsigned long long)nextPair, merged.size());
    return merged;
}

void printLayerStabilizerHistogram(const std::vector<OrbitState>& states) {
    std::unordered_map<uint32_t, uint64_t> counts;
    for (const OrbitState& state : states) ++counts[state.stabilizer];
    std::vector<std::pair<uint32_t, uint64_t>> ordered(counts.begin(),
                                                       counts.end());
    std::sort(ordered.begin(), ordered.end());
    std::printf("layer-stabilizers");
    for (const auto& entry : ordered) {
        std::printf(" %u:%llu", entry.first,
                    (unsigned long long)entry.second);
    }
    std::printf("\n");
    if (C == 6) {
        const uint64_t trivial = counts[1];
        const uint64_t trivialPairs = trivial * (trivial + 1) / 2;
        const uint64_t placementLowerBound =
            trivialPairs * (uint64_t)groupElements.size();
        std::printf("layer-trivial-stabilizer orbits=%llu pairs=%llu "
                    "double_coset_lower_bound=%llu\n",
                    (unsigned long long)trivial,
                    (unsigned long long)trivialPairs,
                    (unsigned long long)placementLowerBound);
    }
}

int runC6Probe(const std::vector<OrbitState>& twoRows,
               uint64_t twoRowLabelled, double layerSeconds) {
    const uint64_t totalPairs =
        twoRows.size() * (twoRows.size() + 1) / 2;
    std::printf("reverse-glue C=6 probe=2+2 group=%zu\n",
                groupElements.size());
    std::printf("layers rows=2 labelled=%llu orbits=%zu generation=%.6fs "
                "unordered_pairs=%llu mask_cache_bytes=%zu\n",
                (unsigned long long)twoRowLabelled, twoRows.size(),
                layerSeconds, (unsigned long long)totalPairs,
                maskImageCacheBytes());
    printLayerStabilizerHistogram(twoRows);
    if (pairProbeStart >= totalPairs) {
        throw std::runtime_error("C=6 pairstart is outside the orbit-pair range");
    }

    GlueMeasurement measurement = measureOrbitGlue(
        twoRows, twoRows, 4, true, pairProbeStart, pairProbeLimit,
        "C6-2+2");
    printGlueMeasurement("C6-2+2", measurement);
    std::printf("C=6 bounded-probe start=%llu requested_pairs=%llu "
                "completed_pairs=%llu started_pairs=%llu "
                "placements=%llu leaves=%llu (not an exact layer)\n",
                (unsigned long long)pairProbeStart,
                (unsigned long long)pairProbeLimit,
                (unsigned long long)measurement.result.orbitPairs,
                (unsigned long long)measurement.result.orbitPairsStarted,
                (unsigned long long)measurement.result.groupApplications,
                (unsigned long long)measurement.result.glueStats.contingencyLeaves);
    return 0;
}

int runC5Pipeline(const std::vector<OrbitState>& twoRows,
                  uint64_t twoRowLabelled, double layerSeconds) {
    std::printf("reverse-glue C=5 pipeline=2+2->4,4+1->5 group=%zu\n",
                groupElements.size());
    std::printf("layers rows=2 labelled=%llu orbits=%zu generation=%.6fs\n",
                (unsigned long long)twoRowLabelled, twoRows.size(), layerSeconds);

    const uint64_t totalFourRowPairs =
        twoRows.size() * (twoRows.size() + 1) / 2;
    std::unordered_map<Config, U128, ConfigHash> fourRowValues;
    if (!partialInputPaths.empty()) {
        fourRowValues = mergePartialLayerFiles(totalFourRowPairs);
    } else {
        GlueMeasurement fourMeasurement = measureOrbitGlue(
            twoRows, twoRows, 4, true,
            pairProbeStart, pairProbeLimit, "C5-2+2");
        printGlueMeasurement("C5-2+2", fourMeasurement);
        if (!partialOutputPath.empty()) {
            writePartialLayerFile(
                partialOutputPath, pairProbeStart,
                fourMeasurement.result.orbitPairs, totalFourRowPairs,
                fourMeasurement.result.factorByClass);
            const PartialLayerFile roundTrip =
                readPartialLayerFile(partialOutputPath);
            if (roundTrip.pairStart != pairProbeStart ||
                roundTrip.pairCount != fourMeasurement.result.orbitPairs ||
                roundTrip.totalPairs != totalFourRowPairs ||
                roundTrip.values != fourMeasurement.result.factorByClass) {
                throw std::runtime_error("partial output round-trip failed");
            }
            std::printf("partial-roundtrip path=%s [OK]\n",
                        partialOutputPath.c_str());
        }
        if (pairProbeLimit != 0) {
            std::printf("pair-probe stopped_after=%llu requested=%llu "
                        "start=%llu partial_classes=%zu (not an exact layer)\n",
                        (unsigned long long)fourMeasurement.result.orbitPairs,
                        (unsigned long long)pairProbeLimit,
                        (unsigned long long)pairProbeStart,
                        fourMeasurement.result.factorByClass.size());
            return 0;
        }
        if (!fourMeasurement.result.complete) {
            throw std::runtime_error("unexpected incomplete C=5 four-row layer");
        }
        fourRowValues = std::move(fourMeasurement.result.factorByClass);
    }

    std::vector<OrbitState> fourRows = orbitStatesFromGlue(fourRowValues);
    uint64_t fourRowCoordinateMass = 0;
    for (const OrbitState& state : fourRows) {
        fourRowCoordinateMass += state.labelledCoordinateMembers;
    }
    std::printf("layers rows=4 orbits=%zu labelled_coordinate=%llu\n",
                fourRows.size(),
                (unsigned long long)fourRowCoordinateMass);

    uint64_t oneRowLabelled = 0;
    std::vector<OrbitState> oneRow = enumerateLayerOrbits(1, oneRowLabelled);
    std::printf("layers rows=1 labelled=%llu orbits=%zu\n",
                (unsigned long long)oneRowLabelled, oneRow.size());

    GlueMeasurement fullMeasurement = measureOrbitGlue(
        fourRows, oneRow, 5, false, 0, 0, "C5-4+1");
    printGlueMeasurement("C5-4+1", fullMeasurement);
    if (!fullMeasurement.result.complete ||
        (int)fullMeasurement.result.factorByClass.size() != expectedClassCount(5)) {
        throw std::runtime_error("C=5 complete gluing class count failed");
    }

    std::vector<std::pair<Config, U128>> classes(
        fullMeasurement.result.factorByClass.begin(),
        fullMeasurement.result.factorByClass.end());
    std::sort(classes.begin(), classes.end(),
              [](const auto& a, const auto& b) { return a.first < b.first; });

    U128 total = 0;
    U128 multiplicitySum = 0;
    int classIndex = 0;
    for (const auto& entry : classes) {
        ++classIndex;
        const CanonicalInfo canonical = canonicalize(entry.first);
        if (canonical.representative != entry.first) {
            throw std::runtime_error("noncanonical complete C=5 class");
        }
        const uint64_t coordinateOrbit =
            groupElements.size() / canonical.stabilizer;
        const uint64_t multiplicity =
            coordinateOrbit * symbolLabellings(entry.first);
        multiplicitySum += multiplicity;
        total += (U128)multiplicity * entry.second * entry.second;
        if (printClasses) {
            std::printf(
                "class=%03d stab=%u orbit=%llu mult=%llu F=%s wF=%s "
                "key=%s [GLUED]\n",
                classIndex, canonical.stabilizer,
                (unsigned long long)coordinateOrbit,
                (unsigned long long)multiplicity,
                decimal(entry.second).c_str(),
                decimal((U128)multiplicity * entry.second).c_str(),
                configText(entry.first).c_str());
        }
    }

    const U128 expectedMultiplicity = power(choose(M, C), C);
    if (multiplicitySum != expectedMultiplicity) {
        throw std::runtime_error("C=5 complete multiplicity sum failed");
    }
    if (decimal(total) != expectedTotal(5)) {
        throw std::runtime_error("C=5 known total failed");
    }
    std::printf("multiplicity_sum=%s expected=%s [OK]\n",
                decimal(multiplicitySum).c_str(),
                decimal(expectedMultiplicity).c_str());
    std::printf("N(5)=%s expected=%s [OK]\n",
                decimal(total).c_str(), expectedTotal(5).c_str());
    return 0;
}

int run() {
    factorials[0] = 1;
    for (int i = 1; i <= MAX_M; ++i) factorials[i] = factorials[i - 1] * i;
    groupElements = makeSignedCoordinateGroup();
    const uint64_t expectedGroup = (uint64_t)(1u << C) * factorials[C];
    if (groupElements.size() != expectedGroup) {
        throw std::runtime_error("signed coordinate group size failed");
    }
    buildGroupMaskImages();

    const auto started = std::chrono::steady_clock::now();
    uint64_t firstLabelled = 0;
    uint64_t secondLabelled = 0;
    auto enumerateRequestedLayer = [&](int rows, uint64_t& labelled) {
        if (C == 6 && rows == 2) {
            return enumerateTwoRowOrbitsByCycles(labelled);
        }
        std::vector<OrbitState> states = enumerateLayerOrbits(rows, labelled);
        if (cycleLayerCheck && rows == 2) {
            uint64_t cycleLabelled = 0;
            const std::vector<OrbitState> cycleStates =
                enumerateTwoRowOrbitsByCycles(cycleLabelled);
            requireEqualOrbitStates(states, cycleStates,
                                    labelled, cycleLabelled);
            std::printf("two-row cycle/raw differential C=%d orbits=%zu "
                        "labelled=%llu [OK]\n",
                        C, states.size(), (unsigned long long)labelled);
        }
        return states;
    };
    std::vector<OrbitState> first =
        enumerateRequestedLayer(firstRows, firstLabelled);
    std::vector<OrbitState> second;
    if (firstRows == secondRows) {
        second = first;
        secondLabelled = firstLabelled;
    } else {
        second = enumerateRequestedLayer(secondRows, secondLabelled);
    }
    const double layerSeconds = std::chrono::duration<double>(
        std::chrono::steady_clock::now() - started).count();

    if (rawLayerCountCheck) {
        uint64_t rawCount = 0;
        enumerateLayer(2, [&](const Config&) { ++rawCount; });
        uint64_t expected = 0;
        if (firstRows == 2) expected = firstLabelled;
        else if (secondRows == 2) expected = secondLabelled;
        else throw std::runtime_error("layercountcheck requires a two-row layer");
        if (rawCount != expected) {
            throw std::runtime_error("two-row independent raw count mismatch");
        }
        std::printf("two-row raw-count labelled=%llu [OK]\n",
                    (unsigned long long)rawCount);
    }

    if (layerProbeOnly) {
        std::printf("reverse-glue layer-probe C=%d group=%zu mask_table_bytes=%zu\n",
                    C, groupElements.size(),
                    maskImageCacheBytes());
        std::printf("layers rows=%d labelled=%llu orbits=%zu generation=%.6fs "
                    "canonical_requests=%llu hits=%llu group_images=%llu cache=%zu\n",
                    firstRows, (unsigned long long)firstLabelled, first.size(),
                    layerSeconds, (unsigned long long)canonicalRequests,
                    (unsigned long long)canonicalHits,
                    (unsigned long long)canonicalGroupImages,
                    canonicalCache.size());
        printLayerStabilizerHistogram(first);
        return 0;
    }

    if (C == 6) {
        return runC6Probe(first, firstLabelled, layerSeconds);
    }

    if (C == 5) {
        return runC5Pipeline(first, firstLabelled, layerSeconds);
    }

    // Keep complete outputs cold here.  Pre-enumerating them would make every
    // join canonicalization a cache hit and give a misleading C=6 throughput
    // projection.
    const uint64_t canonicalRequestsBeforeGlue = canonicalRequests;
    const uint64_t canonicalHitsBeforeGlue = canonicalHits;
    const uint64_t canonicalImagesBeforeGlue = canonicalGroupImages;
    const auto glueStarted = std::chrono::steady_clock::now();
    OrbitGlueResult glued = orbitGlue(
        first, second, C, firstRows == secondRows, 0);
    const double glueSeconds = std::chrono::duration<double>(
        std::chrono::steady_clock::now() - glueStarted).count();
    const uint64_t glueCanonicalRequests =
        canonicalRequests - canonicalRequestsBeforeGlue;
    const uint64_t glueCanonicalHits = canonicalHits - canonicalHitsBeforeGlue;
    const uint64_t glueCanonicalImages =
        canonicalGroupImages - canonicalImagesBeforeGlue;

    uint64_t completeLabelled = 0;
    const std::vector<CompleteClass> complete =
        enumerateCompleteClasses(completeLabelled);
    if ((int)complete.size() != expectedClassCount(C)) {
        throw std::runtime_error("complete class count failed");
    }

    if (glued.factorByClass.size() != complete.size()) {
        throw std::runtime_error("gluing did not cover every complete class");
    }

    U128 total = 0;
    U128 multiplicitySum = 0;
    std::printf("reverse-glue C=%d split=%d+%d group=%zu\n",
                C, firstRows, secondRows, groupElements.size());
    std::printf("layers first_labelled=%llu first_orbits=%zu "
                "second_labelled=%llu second_orbits=%zu generation=%.6fs\n",
                (unsigned long long)firstLabelled, first.size(),
                (unsigned long long)secondLabelled, second.size(), layerSeconds);
    std::printf("complete labelled_coordinate=%llu classes=%zu\n",
                (unsigned long long)completeLabelled, complete.size());

    int classIndex = 0;
    for (const CompleteClass& outer : complete) {
        ++classIndex;
        auto found = glued.factorByClass.find(outer.representative);
        if (found == glued.factorByClass.end()) {
            throw std::runtime_error("missing glued complete class");
        }
        const U128 oracle = factorGraph(graphFromConfig(outer.representative, C));
        if (found->second != oracle) {
            throw std::runtime_error("per-class gluing/oracle mismatch at " +
                                     configText(outer.representative));
        }
        const uint64_t coordinateOrbit = groupElements.size() / outer.stabilizer;
        const uint64_t multiplicity =
            coordinateOrbit * symbolLabellings(outer.representative);
        multiplicitySum += multiplicity;
        total += (U128)multiplicity * oracle * oracle;
        if (printClasses) {
            std::printf("class=%02d stab=%u orbit=%llu mult=%llu F=%s wF=%s key=%s [OK]\n",
                        classIndex, outer.stabilizer,
                        (unsigned long long)coordinateOrbit,
                        (unsigned long long)multiplicity,
                        decimal(oracle).c_str(),
                        decimal((U128)multiplicity * oracle).c_str(),
                        configText(outer.representative).c_str());
        }
    }

    const U128 expectedMultiplicity = power(choose(M, C), C);
    if (multiplicitySum != expectedMultiplicity) {
        throw std::runtime_error("complete labelled multiplicity sum failed");
    }
    if (decimal(total) != expectedTotal(C)) {
        throw std::runtime_error("known total failed");
    }

    const double duplicateRatio = glued.pairReducedRecords == 0 ? 0.0 :
        (double)glued.glueStats.contingencyLeaves /
        (double)glued.pairReducedRecords;
    const double groupApplicationRate = glueSeconds == 0.0 ? 0.0 :
        (double)glued.groupApplications / glueSeconds;
    const double outputRecordRate = glueSeconds == 0.0 ? 0.0 :
        (double)glued.glueStats.contingencyLeaves / glueSeconds;
    const double canonicalRequestRate = glueSeconds == 0.0 ? 0.0 :
        (double)glueCanonicalRequests / glueSeconds;
    const uint64_t coldCanonicalizations = groupElements.empty() ? 0 :
        glueCanonicalImages / groupElements.size();
    const double coldCanonicalRate = glueSeconds == 0.0 ? 0.0 :
        (double)coldCanonicalizations / glueSeconds;
    std::printf("glue orbit_pairs=%llu group_applications=%llu calls=%llu "
                "contingencies=%llu incompatible_calls=%llu pair_reduced=%llu "
                "raw_per_pair_reduced=%.3f max_contingencies_per_call=%llu "
                "max_types=%u,%u time=%.6fs group_applications_per_second=%.3f "
                "output_records_per_second=%.3f\n",
                (unsigned long long)glued.orbitPairs,
                (unsigned long long)glued.groupApplications,
                (unsigned long long)glued.glueStats.calls,
                (unsigned long long)glued.glueStats.contingencyLeaves,
                (unsigned long long)glued.glueStats.incompatibleCalls,
                (unsigned long long)glued.pairReducedRecords,
                duplicateRatio,
                (unsigned long long)glued.glueStats.maximumLeavesPerCall,
                glued.glueStats.maximumFirstTypes,
                glued.glueStats.maximumSecondTypes,
                glueSeconds, groupApplicationRate, outputRecordRate);
    std::printf("canonical glue_requests=%llu glue_hits=%llu cold=%llu "
                "group_images=%llu requests_per_second=%.3f "
                "cold_per_second=%.3f total_cache=%zu\n",
                (unsigned long long)glueCanonicalRequests,
                (unsigned long long)glueCanonicalHits,
                (unsigned long long)coldCanonicalizations,
                (unsigned long long)glueCanonicalImages,
                canonicalRequestRate, coldCanonicalRate, canonicalCache.size());
    std::printf("oracle calls=%llu memo_hits=%llu matching_leaves=%llu memo=%zu\n",
                (unsigned long long)factorCalls,
                (unsigned long long)factorMemoHits,
                (unsigned long long)factorMatchingLeaves,
                factorMemo.size());
    std::printf("multiplicity_sum=%s expected=%s [OK]\n",
                decimal(multiplicitySum).c_str(),
                decimal(expectedMultiplicity).c_str());
    std::printf("N(%d)=%s expected=%s [OK]\n",
                C, decimal(total).c_str(), expectedTotal(C).c_str());
    return 0;
}

} // namespace

int main(int argc, char** argv) {
    try {
        C = argc >= 2 ? std::atoi(argv[1]) : 4;
        if (C < 2 || C > 6) {
            throw std::runtime_error("prototype gate supports C=2..6 only");
        }
        // C=6 is probe-only: it must be run with layerprobe or pairprobe=N.
        // The parser below rejects a full join and applies independent caps to
        // pairs, relative placements, contingency leaves, and canonical cache.
        M = 2 * C;
        firstRows = C >= 5 ? 2 : C / 2;
        secondRows = C >= 5 ? 2 : C - firstRows;
        for (int arg = 2; arg < argc; ++arg) {
            const std::string option = argv[arg];
            if (option == "layerprobe") layerProbeOnly = true;
            else if (option == "cyclelayercheck") cycleLayerCheck = true;
            else if (option == "layercountcheck") rawLayerCountCheck = true;
            else if (option == "summary") printClasses = false;
            else if (option == "rawreduce") reduceRawOutputs = true;
            else if (option == "nocanonicalprobe") {
                skipProbeCanonicalization = true;
            }
            else if (option == "placementsonly") placementsOnlyProbe = true;
            else if (option == "keyonlyprobe") keyOnlyProbe = true;
            else if (option.rfind("canoncachecap=", 0) == 0) {
                anchoredInputCacheCap = std::stoull(option.substr(14));
                canonicalCacheCapExplicit = true;
            }
            else if (option.rfind("partialout=", 0) == 0) {
                partialOutputPath = option.substr(11);
                if (partialOutputPath.empty()) {
                    throw std::runtime_error("partialout requires a path");
                }
            }
            else if (option.rfind("partialin=", 0) == 0) {
                const std::string path = option.substr(10);
                if (path.empty()) {
                    throw std::runtime_error("partialin requires a path");
                }
                partialInputPaths.push_back(path);
            }
            else if (option.rfind("pairprobe=", 0) == 0) {
                pairProbeLimit = std::stoull(option.substr(10));
                if (pairProbeLimit == 0) {
                    throw std::runtime_error("pairprobe requires a positive limit");
                }
            }
            else if (option.rfind("pairstart=", 0) == 0) {
                pairProbeStart = std::stoull(option.substr(10));
            }
            else if (option.rfind("placementprobe=", 0) == 0) {
                relativePlacementProbeLimit = std::stoull(option.substr(15));
                if (relativePlacementProbeLimit == 0) {
                    throw std::runtime_error(
                        "placementprobe requires a positive limit");
                }
            }
            else if (option.rfind("leafprobe=", 0) == 0) {
                contingencyLeafProbeLimit = std::stoull(option.substr(10));
                if (contingencyLeafProbeLimit == 0) {
                    throw std::runtime_error("leafprobe requires a positive limit");
                }
            }
            else if (option.rfind("keyverify=", 0) == 0) {
                keyVerifyLimit = std::stoull(option.substr(10));
                if (keyVerifyLimit == 0 || keyVerifyLimit > 1000) {
                    throw std::runtime_error("keyverify must be in 1..1000");
                }
            }
            else throw std::runtime_error("unknown option: " + option);
        }
        if (pairProbeStart != 0 && pairProbeLimit == 0) {
            throw std::runtime_error("pairstart requires pairprobe");
        }
        if (!partialOutputPath.empty() && pairProbeLimit == 0) {
            throw std::runtime_error("partialout requires pairprobe");
        }
        if (!partialInputPaths.empty() &&
            (pairProbeLimit != 0 || pairProbeStart != 0 ||
             !partialOutputPath.empty() || layerProbeOnly)) {
            throw std::runtime_error(
                "partialin is exclusive with probes and partialout");
        }
        if (C == 6) {
            if (!partialInputPaths.empty() || !partialOutputPath.empty()) {
                throw std::runtime_error(
                    "C=6 probe cannot read or write partial layer files");
            }
            if (cycleLayerCheck) {
                throw std::runtime_error(
                    "C=6 cannot run the labelled-coordinate cycle cross-check");
            }
            if (layerProbeOnly) {
                if (pairProbeLimit != 0 || pairProbeStart != 0 ||
                    relativePlacementProbeLimit != 0 ||
                    contingencyLeafProbeLimit != 0 ||
                    skipProbeCanonicalization || placementsOnlyProbe ||
                    keyOnlyProbe || keyVerifyLimit != 0) {
                    throw std::runtime_error(
                        "C=6 layerprobe is exclusive with pair probes");
                }
            } else {
                if (rawLayerCountCheck) {
                    throw std::runtime_error(
                        "C=6 layercountcheck requires layerprobe");
                }
                if (pairProbeLimit == 0 ||
                    relativePlacementProbeLimit == 0 ||
                    contingencyLeafProbeLimit == 0 || !reduceRawOutputs) {
                    throw std::runtime_error(
                        "C=6 requires positive pairprobe, placementprobe, "
                        "leafprobe, and rawreduce");
                }
                if (contingencyLeafProbeLimit > 100000) {
                    throw std::runtime_error(
                        "C=6 leafprobe safety maximum is 100000");
                }
                if (pairProbeLimit > 16 ||
                    relativePlacementProbeLimit > 100000) {
                    throw std::runtime_error(
                        "C=6 safety maxima are pairprobe=16 and "
                        "placementprobe=100000");
                }
                if ((skipProbeCanonicalization && keyOnlyProbe) ||
                    (placementsOnlyProbe && keyOnlyProbe) ||
                    (keyVerifyLimit != 0 && !keyOnlyProbe)) {
                    throw std::runtime_error(
                        "invalid C=6 key-only probe option combination");
                }
                if (!canonicalCacheCapExplicit || anchoredInputCacheCap == 0 ||
                    anchoredInputCacheCap > 100000) {
                    throw std::runtime_error(
                        "C=6 requires explicit canoncachecap in 1..100000");
                }
            }
        } else if (relativePlacementProbeLimit != 0 ||
                   contingencyLeafProbeLimit != 0 ||
                   skipProbeCanonicalization || placementsOnlyProbe ||
                   keyOnlyProbe || keyVerifyLimit != 0) {
            throw std::runtime_error(
                "placementprobe and leafprobe are C=6-only safety bounds");
        }
        if (secondRows > 2) {
            throw std::runtime_error("prototype partial factor supports at most two rows");
        }
        return run();
    } catch (const std::exception& error) {
        std::fprintf(stderr, "reverse_glue error: %s\n", error.what());
        return 1;
    }
}
