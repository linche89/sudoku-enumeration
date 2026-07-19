#pragma once

#include <algorithm>
#include <array>
#include <bit>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <cstdio>
#include <deque>
#include <exception>
#include <filesystem>
#include <fstream>
#include <functional>
#include <limits>
#include <map>
#include <memory>
#include <numeric>
#include <optional>
#include <queue>
#include <random>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

#ifdef _OPENMP
#include <omp.h>
#endif

namespace future_twin {

constexpr int MAX_C = 6;
constexpr int MAX_M = 2 * MAX_C;
constexpr uint32_t EMPTY_RECORD = std::numeric_limits<uint32_t>::max();
using Count = unsigned __int128;

enum class OrderMode {
    Minimax,
    Greedy,
    ReverseMinimax,
    ReachableGreedy,
    CanonicalFirst,
    CanonicalLast,
    PairFirstTail,
    PairLastTail,
    PairAdaptiveTail,
};

inline const char* orderName(OrderMode mode) {
    if (mode == OrderMode::Minimax) return "minimax";
    if (mode == OrderMode::Greedy) return "greedy";
    if (mode == OrderMode::ReverseMinimax) return "reverse";
    if (mode == OrderMode::ReachableGreedy) return "reachable";
    if (mode == OrderMode::CanonicalFirst) return "canonical-first";
    if (mode == OrderMode::CanonicalLast) return "canonical-last";
    if (mode == OrderMode::PairFirstTail) return "pair-first-tail";
    if (mode == OrderMode::PairLastTail) return "pair-last-tail";
    return "pair-adaptive-tail";
}

inline bool isPairTailOrder(OrderMode mode) {
    return mode == OrderMode::PairFirstTail ||
           mode == OrderMode::PairLastTail ||
           mode == OrderMode::PairAdaptiveTail;
}

struct State {
    std::array<uint32_t, MAX_M> record{};
    uint8_t keyKind = 0; // 0: strong canonical key, 1: safe labelled fallback
    bool operator==(const State&) const = default;
};

struct StateHash {
    size_t operator()(const State& state) const noexcept {
        uint64_t h = 0x9e3779b97f4a7c15ULL;
        for (uint32_t x : state.record) {
            uint64_t z = x + 0x9e3779b97f4a7c15ULL;
            z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
            z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
            z ^= z >> 31;
            h ^= z + 0x9e3779b97f4a7c15ULL + (h << 6) + (h >> 2);
        }
        h ^= (uint64_t)state.keyKind * 0xd6e8feb86659fd93ULL;
        return (size_t)h;
    }
};

inline bool stateLess(const State& a, const State& b) {
    return a.record < b.record;
}

struct Stats {
    std::array<uint64_t, MAX_M + 1> layerStates{};
    std::vector<int> order;
    uint64_t statesExpanded = 0;
    uint64_t subsetChoices = 0;
    uint64_t groupedLeaves = 0;
    uint64_t canonicalCalls = 0;
    uint64_t canonicalCacheHits = 0;
    uint64_t canonicalCacheClears = 0;
    uint64_t colorPermutationsTried = 0;
    uint64_t canonicalSearchNodes = 0;
    uint64_t canonicalFallbacks = 0;
    uint64_t maxCanonicalSearchNodes = 0;
    uint64_t peakStates = 0;
    uint64_t tailStates = 0;
    uint64_t tailZeroStates = 0;
    uint64_t tailSupportTotal = 0;
    uint64_t tailSupportMax = 0;
    uint64_t tailLocalAssignments = 0;
    uint64_t tailValueMax = 0;
    uint64_t tailKernelLookups = 0;
    uint64_t tailKernelHits = 0;
    uint64_t tailKernelEvictions = 0;
    uint64_t tailColorCanonicalLookups = 0;
    uint64_t tailColorCanonicalHits = 0;
    uint64_t tailColorCanonicalOrbits = 0;
    uint64_t tailColorCanonicalMappings = 0;
    double tailSeconds = 0;
    uint64_t externalRawRecords = 0;
    uint64_t externalReducedRecords = 0;
    uint64_t externalRuns = 0;
    uint64_t externalBytesWritten = 0;
    uint64_t externalReusedLayers = 0;
    uint64_t externalReusedRecords = 0;
    uint64_t externalGenerationResumedParents = 0;
    uint64_t externalTailResumedRecords = 0;
    double externalSeconds = 0;
};

struct Options {
    OrderMode order = OrderMode::Minimax;
    size_t canonicalCacheCap = 500000;
    uint64_t canonicalNodeBudget = 20000;
    bool validateStates = false;
    bool transitionDifferential = false;
    bool progress = false;
    std::string externalDirectory;
    size_t externalBufferRecords = 500000;
    size_t externalMergeFanIn = 32;
    uint64_t externalTailCheckpointRecords = 100000;
    uint64_t externalGenerationCheckpointParents = 100000;
    bool externalReadOnly = false;
    bool externalForceTailRescan = false;
    int externalLayerCount = 0;
    uint64_t progressParentInterval = 100000;
    uint64_t testStopExternalGenerationAfterParents = 0;
    int tailRemainingRows = 6;
    size_t tailKernelCacheRecords = 2000000;
    int tailThreads = 1;
    size_t tailChunkRecords = 240000;
    std::string tailSignatureDirectory;
    std::string tailSignatureReferenceDirectory;
    uint64_t tailSignatureSampleRecords = 0;
    uint64_t tailSignatureValidationRecords = 0;
    uint64_t tailBenchmarkRecords = 0;
    uint64_t tailColorCanonicalSampleKeys = 0;
    uint64_t tailColorSignatureRecords = 0;
    std::string tailKernelInventoryDirectory;
    std::string tailKernelInventoryReferenceDirectory;
    uint64_t tailKernelInventoryRecords = 0;
    std::string tailKernelTableDirectory;
    bool useColorCanonicalTail = false;
    bool testTailSignatureDifferential = false;
};

struct Result {
    Count value = 0;
    Stats stats;
};

class Engine {
    struct Context {
        int c = 0;
        int m = 0;
        uint8_t fullColors = 0;
        std::vector<std::array<uint8_t, MAX_C>> permutations;
        std::vector<std::array<uint8_t, 1 << MAX_C>> maskMaps;
        std::vector<uint16_t> inversePermutation;
        std::array<std::array<std::vector<uint8_t>, MAX_C + 1>, 1 << MAX_C> subsets;
        std::array<uint64_t, MAX_C + 1> factorial{};

        explicit Context(int dimension) : c(dimension), m(2 * dimension) {
            if (c < 2 || c > MAX_C) throw std::invalid_argument("future-twin C out of range");
            fullColors = (uint8_t)((1u << c) - 1u);
            factorial[0] = 1;
            for (int i = 1; i <= c; ++i) factorial[i] = factorial[i - 1] * (uint64_t)i;

            std::array<uint8_t, MAX_C> p{};
            for (int i = 0; i < MAX_C; ++i) p[i] = (uint8_t)i;
            do {
                permutations.push_back(p);
            } while (std::next_permutation(p.begin(), p.begin() + c));

            maskMaps.resize(permutations.size());
            inversePermutation.resize(permutations.size());
            for (size_t pi = 0; pi < permutations.size(); ++pi) {
                for (int mask = 0; mask < (1 << c); ++mask) {
                    uint8_t image = 0;
                    for (int color = 0; color < c; ++color) {
                        if ((mask >> color) & 1) image |= (uint8_t)(1u << permutations[pi][color]);
                    }
                    maskMaps[pi][mask] = image;
                }
            }
            for (size_t pi = 0; pi < permutations.size(); ++pi) {
                std::array<uint8_t, MAX_C> inverse{};
                for (int color = 0; color < MAX_C; ++color) {
                    inverse[color] = (uint8_t)color;
                }
                for (int color = 0; color < c; ++color) {
                    inverse[permutations[pi][color]] = (uint8_t)color;
                }
                const auto iterator = std::find(
                    permutations.begin(), permutations.end(), inverse);
                if (iterator == permutations.end()) {
                    throw std::runtime_error(
                        "future-twin color permutation inverse failed");
                }
                inversePermutation[pi] = (uint16_t)
                    std::distance(permutations.begin(), iterator);
            }
            for (int available = 0; available < (1 << c); ++available) {
                int subset = available;
                for (;;) {
                    subsets[available][std::popcount((unsigned)subset)].push_back((uint8_t)subset);
                    if (subset == 0) break;
                    subset = (subset - 1) & available;
                }
            }
        }
    };

    struct Canonicalizer {
        const Context& ctx;
        Stats* stats;
        size_t cap;
        std::unordered_map<State, State, StateHash> cache;

        Canonicalizer(const Context& context, Stats* counters, size_t capacity)
            : ctx(context), stats(counters), cap(capacity) {
            cache.reserve(std::min<size_t>(cap ? cap : 4096, 200000));
        }

        static uint16_t tauOf(uint32_t record) { return (uint16_t)(record & 0xfffu); }
        static uint8_t colorsOf(uint32_t record) { return (uint8_t)((record >> 12) & 0x3fu); }
        static uint32_t pack(uint16_t tau, uint8_t colors) {
            return (uint32_t)tau | ((uint32_t)colors << 12);
        }

        void beginLayer() { cache.clear(); }

        State transform(const State& input, const std::array<uint8_t, MAX_C>& permutation) const {
            std::array<uint8_t, 1 << MAX_C> maskMap{};
            for (int mask = 0; mask < (1 << ctx.c); ++mask) {
                uint8_t image = 0;
                for (int color = 0; color < ctx.c; ++color) {
                    if ((mask >> color) & 1) image |= (uint8_t)(1u << permutation[color]);
                }
                maskMap[mask] = image;
            }
            State output;
            output.record.fill(EMPTY_RECORD);
            for (int i = 0; i < ctx.m; ++i) {
                output.record[i] = pack(tauOf(input.record[i]), maskMap[colorsOf(input.record[i])]);
            }
            std::sort(output.record.begin(), output.record.begin() + ctx.m);
            return output;
        }

        State transform(const State& input, size_t permutationIndex) const {
            State output;
            output.record.fill(EMPTY_RECORD);
            const auto& maskMap = ctx.maskMaps[permutationIndex];
            for (int i = 0; i < ctx.m; ++i) {
                output.record[i] = pack(tauOf(input.record[i]), maskMap[colorsOf(input.record[i])]);
            }
            std::sort(output.record.begin(), output.record.begin() + ctx.m);
            return output;
        }

        State brute(const State& input) const {
            State best;
            best.record.fill(EMPTY_RECORD);
            bool have = false;
            for (size_t pi = 0; pi < ctx.permutations.size(); ++pi) {
                State candidate = transform(input, pi);
                if (!have || stateLess(candidate, best)) {
                    best = candidate;
                    have = true;
                }
            }
            return best;
        }

        State refined(const State& input) {
            std::vector<uint16_t> taus;
            taus.reserve(ctx.m);
            for (int i = 0; i < ctx.m; ++i) taus.push_back(tauOf(input.record[i]));
            std::sort(taus.begin(), taus.end());
            taus.erase(std::unique(taus.begin(), taus.end()), taus.end());

            std::vector<std::vector<int>> signatures(ctx.c);
            for (int color = 0; color < ctx.c; ++color) {
                signatures[color].assign(taus.size(), 0);
                for (int i = 0; i < ctx.m; ++i) {
                    if ((colorsOf(input.record[i]) >> color) & 1) {
                        const size_t t = (size_t)(std::lower_bound(
                            taus.begin(), taus.end(), tauOf(input.record[i])) - taus.begin());
                        ++signatures[color][t];
                    }
                }
            }

            auto classesFromSignatures = [&](const std::vector<std::vector<int>>& sig) {
                std::vector<std::vector<int>> unique = sig;
                std::sort(unique.begin(), unique.end());
                unique.erase(std::unique(unique.begin(), unique.end()), unique.end());
                std::vector<int> classes(ctx.c);
                for (int color = 0; color < ctx.c; ++color) {
                    classes[color] = (int)(std::lower_bound(
                        unique.begin(), unique.end(), sig[color]) - unique.begin());
                }
                return classes;
            };

            std::vector<int> classes = classesFromSignatures(signatures);
            for (;;) {
                const int classCount = *std::max_element(classes.begin(), classes.end()) + 1;
                std::vector<std::vector<int>> refinedSignatures(ctx.c);
                for (int color = 0; color < ctx.c; ++color) {
                    std::vector<int>& signature = refinedSignatures[color];
                    signature.push_back(classes[color]);
                    for (int group = 0; group < classCount; ++group) {
                        std::vector<std::vector<int>> pairProfiles;
                        for (int other = 0; other < ctx.c; ++other) {
                            if (classes[other] != group) continue;
                            std::vector<int> profile(taus.size(), 0);
                            for (int i = 0; i < ctx.m; ++i) {
                                const uint8_t colors = colorsOf(input.record[i]);
                                if (((colors >> color) & 1) && ((colors >> other) & 1)) {
                                    const size_t t = (size_t)(std::lower_bound(
                                        taus.begin(), taus.end(), tauOf(input.record[i])) - taus.begin());
                                    ++profile[t];
                                }
                            }
                            pairProfiles.push_back(std::move(profile));
                        }
                        std::sort(pairProfiles.begin(), pairProfiles.end());
                        signature.push_back(-1);
                        for (const auto& profile : pairProfiles) {
                            signature.insert(signature.end(), profile.begin(), profile.end());
                            signature.push_back(-2);
                        }
                    }
                }
                std::vector<int> next = classesFromSignatures(refinedSignatures);
                if (next == classes) break;
                classes = std::move(next);
            }

            const int classCount = *std::max_element(classes.begin(), classes.end()) + 1;
            std::vector<std::vector<int>> cells(classCount);
            for (int color = 0; color < ctx.c; ++color) cells[classes[color]].push_back(color);

            std::array<uint8_t, MAX_C> mapping{};
            State best;
            best.record.fill(EMPTY_RECORD);
            bool have = false;
            int targetOffset = 0;
            std::function<void(int, int)> enumerate = [&](int cell, int offset) {
                if (cell == classCount) {
                    if (stats) ++stats->colorPermutationsTried;
                    State candidate = transform(input, mapping);
                    if (!have || stateLess(candidate, best)) {
                        best = candidate;
                        have = true;
                    }
                    return;
                }
                std::vector<uint8_t> targets(cells[cell].size());
                for (size_t i = 0; i < targets.size(); ++i) targets[i] = (uint8_t)(offset + (int)i);
                do {
                    for (size_t i = 0; i < cells[cell].size(); ++i) {
                        mapping[cells[cell][i]] = targets[i];
                    }
                    enumerate(cell + 1, offset + (int)targets.size());
                } while (std::next_permutation(targets.begin(), targets.end()));
            };
            enumerate(0, targetOffset);
            if (!have) throw std::runtime_error("future-twin canonicalization produced no mapping");
            return best;
        }

        State canonical(const State& input) {
            if (stats) ++stats->canonicalCalls;
            auto it = cache.find(input);
            if (it != cache.end()) {
                if (stats) ++stats->canonicalCacheHits;
                return it->second;
            }
            State output = refined(input);
            if (cap != 0) {
                if (cache.size() >= cap) {
                    cache.clear();
                    if (stats) ++stats->canonicalCacheClears;
                }
                cache.emplace(input, output);
            }
            return output;
        }
    };

    struct JointCanonicalizer {
        static constexpr int MAX_FEATURES = MAX_M + MAX_C;

        struct Partition {
            std::array<uint8_t, MAX_FEATURES> featureColor{};
            std::array<uint8_t, MAX_M> columnColor{};
            int featureColors = 0;
            int columnColors = 1;
        };

        const Context& ctx;
        Stats* stats;
        size_t cap;
        uint64_t nodeBudget;
        uint64_t callNodes = 0;
        bool aborted = false;
        int rows = 0;
        std::unordered_map<State, State, StateHash> cache;

        JointCanonicalizer(const Context& context, Stats* counters, size_t capacity,
                           uint64_t budget)
            : ctx(context), stats(counters), cap(capacity), nodeBudget(budget) {
            cache.reserve(std::min<size_t>(cap ? cap : 4096, 200000));
        }

        void beginLayer(int remainingRows) {
            rows = remainingRows;
            cache.clear();
        }

        bool refineFeatures(const std::array<uint16_t, MAX_FEATURES>& adjacency,
                            Partition& partition) const {
            const int featureCount = rows + ctx.c;
            std::array<uint8_t, MAX_FEATURES> next{};
            int nextColors = 0;
            bool changed = false;
            for (int old = 0; old < partition.featureColors; ++old) {
                struct Item {
                    int vertex = 0;
                    std::array<uint8_t, MAX_M> signature{};
                };
                std::array<Item, MAX_FEATURES> items{};
                int count = 0;
                for (int vertex = 0; vertex < featureCount; ++vertex) {
                    if (partition.featureColor[vertex] != old) continue;
                    Item item;
                    item.vertex = vertex;
                    uint16_t bits = adjacency[vertex];
                    while (bits) {
                        const int column = std::countr_zero(bits);
                        bits &= (uint16_t)(bits - 1);
                        ++item.signature[partition.columnColor[column]];
                    }
                    items[count++] = item;
                }
                std::sort(items.begin(), items.begin() + count,
                    [&](const Item& a, const Item& b) {
                        for (int color = 0; color < partition.columnColors; ++color) {
                            if (a.signature[color] != b.signature[color]) {
                                return a.signature[color] < b.signature[color];
                            }
                        }
                        return a.vertex < b.vertex;
                    });
                int assigned = nextColors++;
                for (int i = 0; i < count; ++i) {
                    if (i > 0) {
                        bool different = false;
                        for (int color = 0; color < partition.columnColors; ++color) {
                            if (items[i - 1].signature[color] != items[i].signature[color]) {
                                different = true;
                                break;
                            }
                        }
                        if (different) assigned = nextColors++;
                    }
                    next[items[i].vertex] = (uint8_t)assigned;
                }
            }
            if (nextColors != partition.featureColors) changed = true;
            else {
                for (int vertex = 0; vertex < featureCount; ++vertex) {
                    if (next[vertex] != partition.featureColor[vertex]) changed = true;
                }
            }
            partition.featureColor = next;
            partition.featureColors = nextColors;
            return changed;
        }

        bool refineColumns(const std::array<uint16_t, MAX_FEATURES>& adjacency,
                           Partition& partition) const {
            const int featureCount = rows + ctx.c;
            std::array<uint8_t, MAX_M> next{};
            int nextColors = 0;
            bool changed = false;
            for (int old = 0; old < partition.columnColors; ++old) {
                struct Item {
                    int vertex = 0;
                    std::array<uint8_t, MAX_FEATURES> signature{};
                };
                std::array<Item, MAX_M> items{};
                int count = 0;
                for (int column = 0; column < ctx.m; ++column) {
                    if (partition.columnColor[column] != old) continue;
                    Item item;
                    item.vertex = column;
                    for (int feature = 0; feature < featureCount; ++feature) {
                        if ((adjacency[feature] >> column) & 1u) {
                            ++item.signature[partition.featureColor[feature]];
                        }
                    }
                    items[count++] = item;
                }
                std::sort(items.begin(), items.begin() + count,
                    [&](const Item& a, const Item& b) {
                        for (int color = 0; color < partition.featureColors; ++color) {
                            if (a.signature[color] != b.signature[color]) {
                                return a.signature[color] < b.signature[color];
                            }
                        }
                        return a.vertex < b.vertex;
                    });
                int assigned = nextColors++;
                for (int i = 0; i < count; ++i) {
                    if (i > 0) {
                        bool different = false;
                        for (int color = 0; color < partition.featureColors; ++color) {
                            if (items[i - 1].signature[color] != items[i].signature[color]) {
                                different = true;
                                break;
                            }
                        }
                        if (different) assigned = nextColors++;
                    }
                    next[items[i].vertex] = (uint8_t)assigned;
                }
            }
            if (nextColors != partition.columnColors) changed = true;
            else {
                for (int column = 0; column < ctx.m; ++column) {
                    if (next[column] != partition.columnColor[column]) changed = true;
                }
            }
            partition.columnColor = next;
            partition.columnColors = nextColors;
            return changed;
        }

        void refine(const std::array<uint16_t, MAX_FEATURES>& adjacency,
                    Partition& partition) const {
            for (;;) {
                bool changed = false;
                changed |= refineFeatures(adjacency, partition);
                changed |= refineColumns(adjacency, partition);
                if (!changed) return;
            }
        }

        template<size_t N>
        static void individualize(std::array<uint8_t, N>& colors, int vertexCount,
                                  int& colorCount, int selectedColor, int selectedVertex) {
            for (int vertex = 0; vertex < vertexCount; ++vertex) {
                if (colors[vertex] > selectedColor) ++colors[vertex];
                else if (colors[vertex] == selectedColor) {
                    colors[vertex] = (uint8_t)(vertex == selectedVertex
                        ? selectedColor : selectedColor + 1);
                }
            }
            ++colorCount;
        }

        State encode(const std::array<uint16_t, MAX_FEATURES>& adjacency,
                     const Partition& partition) const {
            const int featureCount = rows + ctx.c;
            for (int row = 0; row < rows; ++row) {
                if (std::popcount(adjacency[row]) != ctx.c) {
                    throw std::runtime_error("future-twin joint input row degree changed row=" +
                        std::to_string(row) + " degree=" +
                        std::to_string(std::popcount(adjacency[row])) +
                        " mask=" + std::to_string(adjacency[row]));
                }
            }
            std::array<int, MAX_FEATURES> featureAt{};
            std::array<int, MAX_M> columnAt{};
            for (int feature = 0; feature < featureCount; ++feature) {
                featureAt[partition.featureColor[feature]] = feature;
            }
            for (int column = 0; column < ctx.m; ++column) {
                columnAt[partition.columnColor[column]] = column;
            }
            for (int row = 0; row < rows; ++row) {
                if (featureAt[row] < 0 || featureAt[row] >= rows) {
                    throw std::runtime_error("future-twin joint row/color partition mixed");
                }
            }
            for (int color = 0; color < ctx.c; ++color) {
                if (featureAt[rows + color] < rows || featureAt[rows + color] >= featureCount) {
                    throw std::runtime_error("future-twin joint color partition mixed");
                }
            }

            State output;
            output.record.fill(EMPTY_RECORD);
            for (int position = 0; position < ctx.m; ++position) {
                const int column = columnAt[position];
                uint16_t tau = 0;
                uint8_t colorMask = 0;
                for (int row = 0; row < rows; ++row) {
                    if ((adjacency[featureAt[row]] >> column) & 1u) {
                        tau |= (uint16_t)(1u << row);
                    }
                }
                for (int color = 0; color < ctx.c; ++color) {
                    if ((adjacency[featureAt[rows + color]] >> column) & 1u) {
                        colorMask |= (uint8_t)(1u << color);
                    }
                }
                output.record[position] = Canonicalizer::pack(tau, colorMask);
            }
            std::sort(output.record.begin(), output.record.begin() + ctx.m);
            for (int row = 0; row < rows; ++row) {
                int degree = 0;
                for (int column = 0; column < ctx.m; ++column) {
                    degree += (Canonicalizer::tauOf(output.record[column]) >> row) & 1u;
                }
                if (degree != ctx.c) {
                    throw std::runtime_error("future-twin joint output row degree changed");
                }
            }
            return output;
        }

        void search(const std::array<uint16_t, MAX_FEATURES>& adjacency,
                    Partition partition, State& best, bool& haveBest) {
            if (aborted) return;
            ++callNodes;
            if (stats) ++stats->canonicalSearchNodes;
            if (nodeBudget != 0 && callNodes > nodeBudget) {
                aborted = true;
                return;
            }
            refine(adjacency, partition);
            const int featureCount = rows + ctx.c;
            if (partition.featureColors == featureCount && partition.columnColors == ctx.m) {
                State candidate = encode(adjacency, partition);
                if (!haveBest || stateLess(candidate, best)) {
                    best = candidate;
                    haveBest = true;
                }
                return;
            }

            bool chooseFeatures = true;
            int selectedColor = -1;
            int selectedSize = std::numeric_limits<int>::max();
            auto consider = [&](const auto& colors, int vertexCount, int colorCount, bool features) {
                std::array<int, MAX_FEATURES> sizes{};
                for (int vertex = 0; vertex < vertexCount; ++vertex) ++sizes[colors[vertex]];
                for (int color = 0; color < colorCount; ++color) {
                    const int size = sizes[color];
                    if (size <= 1) continue;
                    if (size < selectedSize ||
                        (size == selectedSize && features && !chooseFeatures) ||
                        (size == selectedSize && features == chooseFeatures && color < selectedColor)) {
                        selectedSize = size;
                        selectedColor = color;
                        chooseFeatures = features;
                    }
                }
            };
            consider(partition.featureColor, featureCount, partition.featureColors, true);
            consider(partition.columnColor, ctx.m, partition.columnColors, false);

            std::vector<uint32_t> seen;
            const int vertexCount = chooseFeatures ? featureCount : ctx.m;
            for (int vertex = 0; vertex < vertexCount; ++vertex) {
                const int color = chooseFeatures
                    ? partition.featureColor[vertex] : partition.columnColor[vertex];
                if (color != selectedColor) continue;
                uint32_t neighborhood = 0;
                if (chooseFeatures) neighborhood = adjacency[vertex];
                else {
                    for (int feature = 0; feature < featureCount; ++feature) {
                        if ((adjacency[feature] >> vertex) & 1u) neighborhood |= 1u << feature;
                    }
                }
                if (std::find(seen.begin(), seen.end(), neighborhood) != seen.end()) continue;
                seen.push_back(neighborhood);
                Partition child = partition;
                if (chooseFeatures) {
                    individualize(child.featureColor, featureCount, child.featureColors,
                                  selectedColor, vertex);
                } else {
                    individualize(child.columnColor, ctx.m, child.columnColors,
                                  selectedColor, vertex);
                }
                search(adjacency, child, best, haveBest);
                if (aborted) return;
            }
        }

        State uncached(const State& input) {
            const int featureCount = rows + ctx.c;
            std::array<uint16_t, MAX_FEATURES> adjacency{};
            for (int column = 0; column < ctx.m; ++column) {
                const uint16_t tau = Canonicalizer::tauOf(input.record[column]);
                const uint8_t colorMask = Canonicalizer::colorsOf(input.record[column]);
                for (int row = 0; row < rows; ++row) {
                    if ((tau >> row) & 1u) adjacency[row] |= (uint16_t)(1u << column);
                }
                for (int color = 0; color < ctx.c; ++color) {
                    if ((colorMask >> color) & 1u) {
                        adjacency[rows + color] |= (uint16_t)(1u << column);
                    }
                }
            }
            for (int row = 0; row < rows; ++row) {
                if (std::popcount(adjacency[row]) != ctx.c) {
                    std::string detail = "future-twin joint raw row degree changed row=" +
                        std::to_string(row) + " degree=" +
                        std::to_string(std::popcount(adjacency[row])) + " tau=";
                    for (int column = 0; column < ctx.m; ++column) {
                        if (column) detail.push_back(',');
                        detail += std::to_string(Canonicalizer::tauOf(input.record[column]));
                    }
                    throw std::runtime_error(detail);
                }
            }
            Partition partition;
            if (rows == 0) {
                for (int color = 0; color < ctx.c; ++color) partition.featureColor[color] = 0;
                partition.featureColors = 1;
            } else {
                for (int row = 0; row < rows; ++row) partition.featureColor[row] = 0;
                for (int color = 0; color < ctx.c; ++color) partition.featureColor[rows + color] = 1;
                partition.featureColors = 2;
            }
            for (int column = 0; column < ctx.m; ++column) partition.columnColor[column] = 0;
            partition.columnColors = 1;

            State best;
            best.record.fill(EMPTY_RECORD);
            bool haveBest = false;
            callNodes = 0;
            aborted = false;
            search(adjacency, partition, best, haveBest);
            if (stats) {
                stats->maxCanonicalSearchNodes = std::max(
                    stats->maxCanonicalSearchNodes, callNodes);
            }
            if (aborted) {
                if (stats) ++stats->canonicalFallbacks;
                State safe = input;
                safe.keyKind = 1;
                return safe;
            }
            if (!haveBest || featureCount == 0) {
                throw std::runtime_error("future-twin joint canonicalization failed");
            }
            return best;
        }

        State canonical(const State& input) {
            if (stats) ++stats->canonicalCalls;
            auto it = cache.find(input);
            if (it != cache.end()) {
                if (stats) ++stats->canonicalCacheHits;
                return it->second;
            }
            State output = uncached(input);
            if (cap != 0) {
                if (cache.size() >= cap) {
                    cache.clear();
                    if (stats) ++stats->canonicalCacheClears;
                }
                cache.emplace(input, output);
            }
            return output;
        }
    };

    struct Group {
        uint16_t tau = 0;
        uint8_t colors = 0;
        uint8_t multiplicity = 0;
    };

    Context ctx_;

    static uint64_t chooseSmall(int n, int k) {
        if (k < 0 || k > n) return 0;
        k = std::min(k, n - k);
        uint64_t value = 1;
        for (int i = 1; i <= k; ++i) value = value * (uint64_t)(n - k + i) / (uint64_t)i;
        return value;
    }

    std::array<uint16_t, MAX_M> rightNeighborhoods(
        const std::array<uint16_t, MAX_M>& graph) const {
        std::array<uint16_t, MAX_M> neighborhoods{};
        std::array<int, MAX_M> rightDegrees{};
        const uint16_t rightMask = (uint16_t)((1u << ctx_.m) - 1u);
        for (int left = 0; left < ctx_.m; ++left) {
            if ((graph[left] & ~rightMask) != 0 || std::popcount(graph[left]) != ctx_.c) {
                throw std::invalid_argument("future-twin graph has invalid left degree");
            }
            uint16_t bits = graph[left];
            while (bits) {
                const int right = std::countr_zero(bits);
                bits &= (uint16_t)(bits - 1);
                neighborhoods[right] |= (uint16_t)(1u << left);
                ++rightDegrees[right];
            }
        }
        for (int right = 0; right < ctx_.m; ++right) {
            if (rightDegrees[right] != ctx_.c || std::popcount(neighborhoods[right]) != ctx_.c) {
                throw std::invalid_argument("future-twin graph has invalid right degree");
            }
        }
        return neighborhoods;
    }

    long double ambientLog(const std::array<uint16_t, MAX_M>& neighborhoods,
                           uint16_t processed) const {
        std::map<uint16_t, int> multiplicities;
        const uint16_t unprocessed = (uint16_t)(((1u << ctx_.m) - 1u) ^ processed);
        for (int right = 0; right < ctx_.m; ++right) {
            ++multiplicities[(uint16_t)(neighborhoods[right] & unprocessed)];
        }
        long double total = 0;
        for (const auto& [tau, multiplicity] : multiplicities) {
            const int used = ctx_.c - std::popcount(tau);
            const uint64_t choices = chooseSmall(ctx_.c, used);
            const uint64_t tables = chooseSmall((int)choices + multiplicity - 1, multiplicity);
            total += std::log((long double)tables);
        }
        return total;
    }

    std::vector<int> eliminationOrder(const std::array<uint16_t, MAX_M>& neighborhoods,
                                      OrderMode mode) const {
        if (mode == OrderMode::ReachableGreedy) {
            throw std::invalid_argument("reachable order is selected during the DP");
        }
        const int stateCount = 1 << ctx_.m;
        const uint16_t all = (uint16_t)(stateCount - 1);
        std::vector<long double> bound(stateCount);
        for (int processed = 0; processed < stateCount; ++processed) {
            bound[processed] = ambientLog(neighborhoods, (uint16_t)processed);
        }

        if (mode == OrderMode::Greedy) {
            std::vector<int> order;
            uint16_t processed = 0;
            while (processed != all) {
                int bestVertex = -1;
                long double best = std::numeric_limits<long double>::infinity();
                for (int vertex = 0; vertex < ctx_.m; ++vertex) {
                    if ((processed >> vertex) & 1) continue;
                    const long double candidate = bound[processed | (1u << vertex)];
                    if (candidate < best - 1e-18L ||
                        (std::abs(candidate - best) <= 1e-18L && vertex < bestVertex)) {
                        best = candidate;
                        bestVertex = vertex;
                    }
                }
                order.push_back(bestVertex);
                processed |= (uint16_t)(1u << bestVertex);
            }
            return order;
        }

        std::vector<long double> cost(stateCount, std::numeric_limits<long double>::infinity());
        std::vector<int8_t> last(stateCount, -1);
        cost[0] = bound[0];
        for (int processed = 1; processed < stateCount; ++processed) {
            for (int vertex = 0; vertex < ctx_.m; ++vertex) {
                if (!((processed >> vertex) & 1)) continue;
                const int previous = processed ^ (1 << vertex);
                const long double candidate = std::max(cost[previous], bound[processed]);
                if (candidate < cost[processed] - 1e-18L ||
                    (std::abs(candidate - cost[processed]) <= 1e-18L &&
                     (last[processed] < 0 || vertex < last[processed]))) {
                    cost[processed] = candidate;
                    last[processed] = (int8_t)vertex;
                }
            }
        }

        std::vector<int> reverse;
        int processed = all;
        while (processed) {
            const int vertex = last[processed];
            if (vertex < 0) throw std::runtime_error("future-twin order traceback failed");
            reverse.push_back(vertex);
            processed ^= 1 << vertex;
        }
        std::reverse(reverse.begin(), reverse.end());
        if (mode == OrderMode::ReverseMinimax) std::reverse(reverse.begin(), reverse.end());
        return reverse;
    }

    void validateState(const State& state, int processedCount) const {
        std::array<int, MAX_C> colorUse{};
        for (int i = 0; i < ctx_.m; ++i) {
            const uint16_t tau = Canonicalizer::tauOf(state.record[i]);
            const uint8_t colors = Canonicalizer::colorsOf(state.record[i]);
            if (std::popcount((unsigned)colors) != ctx_.c - std::popcount(tau)) {
                throw std::runtime_error("future-twin state size invariant failed");
            }
            for (int color = 0; color < ctx_.c; ++color) {
                colorUse[color] += (colors >> color) & 1;
            }
        }
        for (int color = 0; color < ctx_.c; ++color) {
            if (colorUse[color] != processedCount) {
                throw std::runtime_error("future-twin color marginal invariant failed");
            }
        }
    }

    template<class Canonical, class Emit>
    void groupedTransitions(const State& state, int vertex, Canonical& canonicalizer,
                            Stats& stats, Emit&& emit, bool compactRows = false) const {
        std::vector<Group> groups;
        for (int i = 0; i < ctx_.m;) {
            int j = i + 1;
            while (j < ctx_.m && state.record[j] == state.record[i]) ++j;
            groups.push_back({Canonicalizer::tauOf(state.record[i]),
                              Canonicalizer::colorsOf(state.record[i]),
                              (uint8_t)(j - i)});
            i = j;
        }

        const uint16_t vertexBit = (uint16_t)(1u << vertex);
        auto compactTau = [&](uint16_t tau) {
            if (!compactRows) return tau;
            const uint16_t lower = (uint16_t)(tau & (vertexBit - 1u));
            const uint16_t upper = (uint16_t)((tau >> (vertex + 1)) << vertex);
            return (uint16_t)(lower | upper);
        };
        std::vector<Group> affected;
        std::array<uint32_t, MAX_M> records{};
        int recordCount = 0;
        uint64_t labelledMultiplicity = 1;
        int affectedColumns = 0;
        for (const Group& group : groups) {
            if (group.tau & vertexBit) {
                affected.push_back(group);
                affectedColumns += group.multiplicity;
                labelledMultiplicity *= ctx_.factorial[group.multiplicity];
            } else {
                for (int copy = 0; copy < group.multiplicity; ++copy) {
                    records[recordCount++] = Canonicalizer::pack(
                        compactTau(group.tau), group.colors);
                }
            }
        }
        if (affectedColumns != ctx_.c) {
            std::string detail = "future-twin transition has wrong affected degree vertex=" +
                std::to_string(vertex) + " got=" + std::to_string(affectedColumns) + " tau=";
            for (int i = 0; i < ctx_.m; ++i) {
                if (i) detail.push_back(',');
                detail += std::to_string(Canonicalizer::tauOf(state.record[i]));
            }
            throw std::runtime_error(detail);
        }

        std::function<void(size_t, uint8_t, int)> search =
            [&](size_t groupIndex, uint8_t remaining, int count) {
                if (groupIndex == affected.size()) {
                    if (remaining != 0 || count != ctx_.m) return;
                    ++stats.groupedLeaves;
                    State raw;
                    raw.record.fill(EMPTY_RECORD);
                    std::copy(records.begin(), records.begin() + count, raw.record.begin());
                    std::sort(raw.record.begin(), raw.record.begin() + ctx_.m);
                    emit(canonicalizer.canonical(raw), labelledMultiplicity);
                    return;
                }
                const Group& group = affected[groupIndex];
                const uint8_t available = (uint8_t)(remaining & ~group.colors & ctx_.fullColors);
                for (uint8_t chosen : ctx_.subsets[available][group.multiplicity]) {
                    ++stats.subsetChoices;
                    int nextCount = count;
                    uint8_t bits = chosen;
                    while (bits) {
                        const uint8_t bit = (uint8_t)(bits & (uint8_t)-bits);
                        bits ^= bit;
                        uint16_t nextTau = (uint16_t)(group.tau ^ vertexBit);
                        nextTau = compactTau(nextTau);
                        records[nextCount++] = Canonicalizer::pack(
                            nextTau,
                            (uint8_t)(group.colors | bit));
                    }
                    search(groupIndex + 1, (uint8_t)(remaining ^ chosen), nextCount);
                }
            };
        search(0, ctx_.fullColors, recordCount);
    }

    using TransitionMap = std::unordered_map<State, uint64_t, StateHash>;
    using WeightedLayer = std::unordered_map<State, Count, StateHash>;

    TransitionMap groupedTransitionMap(const State& state, int vertex,
                                       Canonicalizer& canonicalizer, Stats& stats) const {
        TransitionMap output;
        groupedTransitions(state, vertex, canonicalizer, stats,
            [&](const State& target, uint64_t multiplicity) { output[target] += multiplicity; });
        return output;
    }

    TransitionMap labelledTransitionMap(const State& state, int vertex,
                                        Canonicalizer& canonicalizer) const {
        const uint16_t vertexBit = (uint16_t)(1u << vertex);
        std::array<int, MAX_C> affected{};
        int affectedCount = 0;
        std::array<uint32_t, MAX_M> base{};
        int baseCount = 0;
        for (int i = 0; i < ctx_.m; ++i) {
            if (Canonicalizer::tauOf(state.record[i]) & vertexBit) affected[affectedCount++] = i;
            else base[baseCount++] = state.record[i];
        }
        if (affectedCount != ctx_.c) throw std::runtime_error("future-twin labelled test degree failed");

        TransitionMap output;
        for (const auto& assignment : ctx_.permutations) {
            bool valid = true;
            std::array<uint32_t, MAX_M> records = base;
            int count = baseCount;
            for (int k = 0; k < ctx_.c; ++k) {
                const uint32_t old = state.record[affected[k]];
                const uint8_t bit = (uint8_t)(1u << assignment[k]);
                const uint8_t colors = Canonicalizer::colorsOf(old);
                if (colors & bit) {
                    valid = false;
                    break;
                }
                records[count++] = Canonicalizer::pack(
                    (uint16_t)(Canonicalizer::tauOf(old) ^ vertexBit),
                    (uint8_t)(colors | bit));
            }
            if (!valid) continue;
            State raw;
            raw.record.fill(EMPTY_RECORD);
            std::copy(records.begin(), records.begin() + count, raw.record.begin());
            std::sort(raw.record.begin(), raw.record.begin() + ctx_.m);
            ++output[canonicalizer.refined(raw)];
        }
        return output;
    }

    static std::array<uint16_t, MAX_M> randomBalancedGraph(int c, std::mt19937_64& rng) {
        const int m = 2 * c;
        std::array<uint16_t, MAX_M> graph{};
        std::vector<int> columns(m);
        std::iota(columns.begin(), columns.end(), 0);
        for (int row = 0; row < c; ++row) {
            std::shuffle(columns.begin(), columns.end(), rng);
            for (int i = 0; i < c; ++i) graph[2 * row] |= (uint16_t)(1u << columns[i]);
            graph[2 * row + 1] = (uint16_t)(((1u << m) - 1u) ^ graph[2 * row]);
        }
        return graph;
    }

    struct TailColumn {
        uint8_t firstSide = 0;
        uint8_t secondSide = 0;
        uint8_t available = 0;
    };

    struct TailKernelRecord {
        uint32_t first = 0;
        uint32_t second = 0;

        TailKernelRecord() = default;
        TailKernelRecord(uint32_t signature, uint32_t weight)
            : first(signature), second(weight) {}
        bool operator==(const TailKernelRecord&) const = default;
    };
    static_assert(sizeof(TailKernelRecord) == 8);
    static_assert(std::is_trivially_copyable_v<TailKernelRecord>);

    using TailKernel = std::vector<TailKernelRecord>;

    struct TailKernelSlice {
        const TailKernelRecord* records = nullptr;
        size_t recordCount = 0;

        const TailKernelRecord* begin() const { return records; }
        const TailKernelRecord* end() const {
            return recordCount == 0 ? records : records + recordCount;
        }
        size_t size() const { return recordCount; }
    };

    struct SevenRowKernelCacheEntry {
        std::shared_ptr<const TailKernel> kernel;
    };

    struct SevenRowColorCanonicalHalf {
        uint64_t key = std::numeric_limits<uint64_t>::max();
        uint8_t rowTransform = 0;
        uint16_t colorPermutation = 0;
    };

    struct SevenRowPersistentKernelIndex {
        uint64_t key = 0;
        uint64_t offset = 0;
        uint64_t records = 0;
    };

    struct SevenRowPersistentKernelTable {
        int c = 0;
        int processedRows = 0;
        uint64_t checksum = 0;
        std::vector<SevenRowPersistentKernelIndex> index;
        std::vector<TailKernelRecord> records;
        std::unordered_map<uint64_t, size_t> positions;

        std::optional<TailKernelSlice> find(uint64_t key) const {
            const auto iterator = positions.find(key);
            if (iterator == positions.end()) return std::nullopt;
            const SevenRowPersistentKernelIndex& item =
                index[iterator->second];
            return TailKernelSlice{
                records.data() + (size_t)item.offset,
                (size_t)item.records};
        }
    };

    mutable std::unordered_map<uint64_t, SevenRowKernelCacheEntry>
        sevenRowKernelCache_;
    mutable std::deque<uint64_t> sevenRowKernelCacheOrder_;
    mutable size_t sevenRowKernelCachedRecords_ = 0;
    mutable std::unordered_map<uint64_t, SevenRowColorCanonicalHalf>
        sevenRowColorCanonicalCache_;
    mutable std::shared_ptr<const SevenRowPersistentKernelTable>
        sevenRowPersistentKernelTable_;

    struct TailEvaluation {
        uint64_t value = 0;
        uint64_t localAssignments = 0;
        uint32_t leftSupport = 0;
        uint32_t rightSupport = 0;
        uint32_t kernelLookups = 0;
        uint32_t kernelHits = 0;
        uint32_t kernelEvictions = 0;
        uint32_t colorCanonicalLookups = 0;
        uint32_t colorCanonicalHits = 0;
        uint32_t colorCanonicalOrbits = 0;
        uint32_t colorCanonicalMappings = 0;
    };

    template<class Emit>
    void enumerateTailAssignments(const std::array<uint8_t, MAX_C>& allowed,
                                  Emit&& emit) const {
        std::array<uint8_t, MAX_C> assignment{};
        const uint8_t allColumns = (uint8_t)((1u << ctx_.c) - 1u);
        std::function<void(uint8_t, uint8_t)> search =
            [&](uint8_t assignedColumns, uint8_t usedColors) {
                if (assignedColumns == allColumns) {
                    if (usedColors == ctx_.fullColors) emit(assignment);
                    return;
                }
                int selected = -1;
                int selectedChoices = MAX_C + 1;
                for (int column = 0; column < ctx_.c; ++column) {
                    if ((assignedColumns >> column) & 1u) continue;
                    const int choices = std::popcount(
                        (unsigned)(allowed[column] & (uint8_t)~usedColors));
                    if (choices < selectedChoices) {
                        selected = column;
                        selectedChoices = choices;
                    }
                }
                if (selected < 0 || selectedChoices == 0) return;
                uint8_t choices = (uint8_t)(allowed[selected] &
                                             (uint8_t)~usedColors &
                                             ctx_.fullColors);
                while (choices) {
                    const uint8_t bit = (uint8_t)(choices & (uint8_t)-choices);
                    choices ^= bit;
                    assignment[selected] = bit;
                    search((uint8_t)(assignedColumns | (1u << selected)),
                           (uint8_t)(usedColors | bit));
                }
            };
        search(0, 0);
    }

    uint32_t tailAssignmentCount(const std::array<TailColumn, MAX_C>& columns) const {
        std::array<uint8_t, MAX_C> allowed{};
        for (int i = 0; i < ctx_.c; ++i) allowed[i] = columns[i].available;
        uint32_t count = 0;
        enumerateTailAssignments(allowed, [&](const auto&) { ++count; });
        return count;
    }

    uint32_t tailAssignmentCount(
        const std::array<uint8_t, MAX_C>& allowed) const {
        uint32_t count = 0;
        enumerateTailAssignments(allowed, [&](const auto&) { ++count; });
        return count;
    }

    TailKernel makeTailKernel(const std::array<TailColumn, MAX_C>& columns,
                              uint64_t& localAssignments) const {
        std::array<uint8_t, MAX_C> allowed{};
        for (int i = 0; i < ctx_.c; ++i) {
            const TailColumn& column = columns[i];
            if (column.firstSide > 1 || column.secondSide > 1 ||
                std::popcount((unsigned)column.available) != 3) {
                throw std::runtime_error("future-twin tail column invariant failed");
            }
            allowed[i] = column.available;
        }

        std::vector<uint32_t> raw;
        raw.reserve(512);
        enumerateTailAssignments(allowed, [&](const auto& qAssignment) {
            std::array<std::array<uint8_t, 2>, MAX_C> leftover{};
            for (int column = 0; column < ctx_.c; ++column) {
                uint8_t bits = (uint8_t)(columns[column].available ^
                                         qAssignment[column]);
                if (std::popcount((unsigned)bits) != 2) {
                    throw std::runtime_error("future-twin tail leftover invariant failed");
                }
                const uint8_t first = (uint8_t)(bits & (uint8_t)-bits);
                leftover[column] = {first, (uint8_t)(bits ^ first)};
            }

            std::array<uint8_t, 4> rowColors{};
            std::function<void(int)> orient = [&](int column) {
                if (column == ctx_.c) {
                    uint32_t signature = 0;
                    for (int row = 0; row < 4; ++row) {
                        signature |= (uint32_t)rowColors[row] << (row * ctx_.c);
                    }
                    raw.push_back(signature);
                    return;
                }
                const int firstRow = columns[column].firstSide;
                const int secondRow = 2 + columns[column].secondSide;
                for (int orientation = 0; orientation < 2; ++orientation) {
                    const uint8_t firstColor = leftover[column][orientation];
                    const uint8_t secondColor = leftover[column][1 - orientation];
                    if ((rowColors[firstRow] & firstColor) ||
                        (rowColors[secondRow] & secondColor)) {
                        continue;
                    }
                    rowColors[firstRow] |= firstColor;
                    rowColors[secondRow] |= secondColor;
                    orient(column + 1);
                    rowColors[firstRow] ^= firstColor;
                    rowColors[secondRow] ^= secondColor;
                }
            };
            orient(0);
        });

        localAssignments += raw.size();
        std::sort(raw.begin(), raw.end());
        TailKernel kernel;
        kernel.reserve(raw.size());
        for (size_t i = 0; i < raw.size();) {
            size_t j = i + 1;
            while (j < raw.size() && raw[j] == raw[i]) ++j;
            kernel.emplace_back(raw[i], (uint32_t)(j - i));
            i = j;
        }
        return kernel;
    }

    TailEvaluation threePairTailValue(const State& state) const {
        if (ctx_.c < 3) {
            throw std::runtime_error("future-twin three-pair tail requires C >= 3");
        }
        std::array<uint16_t, 6> rowColumns{};
        for (int column = 0; column < ctx_.m; ++column) {
            const uint16_t tau = Canonicalizer::tauOf(state.record[column]);
            const uint8_t colors = Canonicalizer::colorsOf(state.record[column]);
            if ((tau & ~0x3fu) != 0 || std::popcount((unsigned)tau) != 3 ||
                std::popcount((unsigned)colors) != ctx_.c - 3) {
                throw std::runtime_error("future-twin midpoint tail state invariant failed");
            }
            uint16_t bits = tau;
            while (bits) {
                const int row = std::countr_zero(bits);
                bits &= (uint16_t)(bits - 1);
                rowColumns[row] |= (uint16_t)(1u << column);
            }
        }

        const uint16_t fullColumns = (uint16_t)((1u << ctx_.m) - 1u);
        std::array<std::array<int, 2>, 3> pairs{};
        std::array<bool, 6> usedRows{};
        for (int pair = 0; pair < 3; ++pair) {
            int first = -1;
            for (int row = 0; row < 6; ++row) {
                if (!usedRows[row]) {
                    first = row;
                    break;
                }
            }
            if (first < 0 || std::popcount((unsigned)rowColumns[first]) != ctx_.c) {
                throw std::runtime_error("future-twin tail row degree invariant failed");
            }
            const uint16_t complement = (uint16_t)(fullColumns ^ rowColumns[first]);
            int second = -1;
            for (int row = first + 1; row < 6; ++row) {
                if (!usedRows[row] && rowColumns[row] == complement) {
                    second = row;
                    break;
                }
            }
            if (second < 0) {
                throw std::runtime_error("future-twin tail could not recover complementary row pair");
            }
            usedRows[first] = true;
            usedRows[second] = true;
            pairs[pair] = {first, second};
        }

        auto halfColumns = [&](int cutPair, int side) {
            std::array<TailColumn, MAX_C> output{};
            std::array<int, 2> other{};
            int otherCount = 0;
            for (int pair = 0; pair < 3; ++pair) {
                if (pair != cutPair) other[otherCount++] = pair;
            }
            int count = 0;
            const int cutRow = pairs[cutPair][side];
            for (int column = 0; column < ctx_.m; ++column) {
                if (!((rowColumns[cutRow] >> column) & 1u)) continue;
                TailColumn item;
                for (int index = 0; index < 2; ++index) {
                    const int pair = other[index];
                    const bool firstIncident =
                        ((rowColumns[pairs[pair][0]] >> column) & 1u) != 0;
                    const bool secondIncident =
                        ((rowColumns[pairs[pair][1]] >> column) & 1u) != 0;
                    if (firstIncident == secondIncident) {
                        throw std::runtime_error("future-twin tail pair incidence invariant failed");
                    }
                    if (index == 0) item.firstSide = firstIncident ? 0 : 1;
                    else item.secondSide = firstIncident ? 0 : 1;
                }
                item.available = (uint8_t)(ctx_.fullColors &
                    (uint8_t)~Canonicalizer::colorsOf(state.record[column]));
                if (count >= ctx_.c) {
                    throw std::runtime_error("future-twin tail half has too many columns");
                }
                output[count++] = item;
            }
            if (count != ctx_.c) {
                throw std::runtime_error("future-twin tail half has wrong column count");
            }
            return output;
        };

        int selectedCut = -1;
        uint32_t bestProxy = std::numeric_limits<uint32_t>::max();
        for (int cut = 0; cut < 3; ++cut) {
            const uint32_t proxy = tailAssignmentCount(halfColumns(cut, 0)) +
                                   tailAssignmentCount(halfColumns(cut, 1));
            if (proxy < bestProxy) {
                bestProxy = proxy;
                selectedCut = cut;
            }
        }
        if (selectedCut < 0) {
            throw std::runtime_error("future-twin tail cut selection failed");
        }

        TailEvaluation result;
        const TailKernel left = makeTailKernel(
            halfColumns(selectedCut, 0), result.localAssignments);
        const TailKernel right = makeTailKernel(
            halfColumns(selectedCut, 1), result.localAssignments);
        result.leftSupport = (uint32_t)left.size();
        result.rightSupport = (uint32_t)right.size();
        const uint32_t fullSignature = (1u << (4 * ctx_.c)) - 1u;
        for (const auto& [signature, weight] : left) {
            const uint32_t target = fullSignature ^ signature;
            const auto it = std::lower_bound(
                right.begin(), right.end(), target,
                [](const auto& entry, uint32_t value) { return entry.first < value; });
            if (it != right.end() && it->first == target) {
                result.value += (uint64_t)weight * it->second;
            }
        }
        return result;
    }

    struct SevenRowTailColumn {
        uint8_t firstSide = 0;
        uint8_t secondSide = 0;
        uint8_t pending = 0;
        uint8_t available = 0;
    };

    TailKernel makeSevenRowTailKernel(
        const std::array<SevenRowTailColumn, MAX_C>& columns,
        uint64_t& localAssignments) const {
        std::array<uint8_t, MAX_C> allowed{};
        std::array<int, MAX_C> columnOrder{};
        for (int column = 0; column < ctx_.c; ++column) {
            const SevenRowTailColumn& item = columns[column];
            const int expected = 3 + (item.pending != 0);
            if (item.firstSide > 1 || item.secondSide > 1 ||
                item.pending > 1 ||
                std::popcount((unsigned)item.available) != expected) {
                throw std::runtime_error(
                    "future-twin seven-row tail column invariant failed");
            }
            allowed[column] = item.available;
            columnOrder[column] = column;
        }
        std::stable_sort(
            columnOrder.begin(), columnOrder.begin() + ctx_.c,
            [&](int first, int second) {
                return columns[first].pending > columns[second].pending;
            });

        std::unordered_map<uint32_t, uint32_t> counts;
        counts.reserve(4096);
        enumerateTailAssignments(allowed, [&](const auto& qAssignment) {
            std::array<uint8_t, 5> rowColors{};
            std::function<void(int)> orient = [&](int position) {
                if (position == ctx_.c) {
                    uint32_t signature = 0;
                    for (int row = 0; row < 5; ++row) {
                        signature |= (uint32_t)rowColors[row] << (row * ctx_.c);
                    }
                    uint32_t& count = counts[signature];
                    if (count == std::numeric_limits<uint32_t>::max()) {
                        throw std::overflow_error(
                            "future-twin seven-row kernel count overflow");
                    }
                    ++count;
                    ++localAssignments;
                    return;
                }

                const int column = columnOrder[position];
                const SevenRowTailColumn& item = columns[column];
                uint8_t leftover = (uint8_t)(item.available ^ qAssignment[column]);
                const int rowCount = 2 + (item.pending != 0);
                if (std::popcount((unsigned)leftover) != rowCount) {
                    throw std::runtime_error(
                        "future-twin seven-row leftover invariant failed");
                }
                std::array<int, 3> rows{
                    item.firstSide, 2 + item.secondSide, 4};
                std::array<uint8_t, 3> colors{};
                for (int index = 0; index < rowCount; ++index) {
                    const uint8_t bit = (uint8_t)(leftover & (uint8_t)-leftover);
                    leftover ^= bit;
                    colors[index] = bit;
                }
                do {
                    bool valid = true;
                    for (int index = 0; index < rowCount; ++index) {
                        if (rowColors[rows[index]] & colors[index]) {
                            valid = false;
                            break;
                        }
                    }
                    if (!valid) continue;
                    for (int index = 0; index < rowCount; ++index) {
                        rowColors[rows[index]] |= colors[index];
                    }
                    orient(position + 1);
                    for (int index = 0; index < rowCount; ++index) {
                        rowColors[rows[index]] ^= colors[index];
                    }
                } while (std::next_permutation(
                    colors.begin(), colors.begin() + rowCount));
            };
            orient(0);
        });

        TailKernel kernel;
        kernel.reserve(counts.size());
        for (const auto& entry : counts) {
            kernel.emplace_back(entry.first, entry.second);
        }
        std::sort(kernel.begin(), kernel.end(),
                  [](const auto& first, const auto& second) {
                      return first.first < second.first;
                  });
        return kernel;
    }

    uint64_t packSevenRowTailKernelKey(
        const std::array<SevenRowTailColumn, MAX_C>& columns) const {
        std::array<uint16_t, MAX_C> types{};
        for (int column = 0; column < ctx_.c; ++column) {
            const SevenRowTailColumn& item = columns[column];
            types[column] = (uint16_t)item.available |
                ((uint16_t)item.firstSide << 6) |
                ((uint16_t)item.secondSide << 7) |
                ((uint16_t)item.pending << 8);
        }
        std::sort(types.begin(), types.begin() + ctx_.c);
        uint64_t key = 0;
        for (int column = 0; column < ctx_.c; ++column) {
            key |= (uint64_t)types[column] << (9 * column);
        }
        return key;
    }

    static std::array<uint8_t, 5> sevenRowTransformMap(uint8_t transform) {
        const bool swapPairs = (transform & 1u) != 0;
        const std::array<uint8_t, 2> flip{
            (uint8_t)((transform >> 1) & 1u),
            (uint8_t)((transform >> 2) & 1u)};
        std::array<uint8_t, 5> map{};
        for (int oldPair = 0; oldPair < 2; ++oldPair) {
            const int newPair = swapPairs ? 1 - oldPair : oldPair;
            for (int oldSide = 0; oldSide < 2; ++oldSide) {
                const int newSide = oldSide ^ flip[newPair];
                map[2 * oldPair + oldSide] =
                    (uint8_t)(2 * newPair + newSide);
            }
        }
        map[4] = 4;
        return map;
    }

    std::array<SevenRowTailColumn, MAX_C> transformSevenRowTailColumns(
        const std::array<SevenRowTailColumn, MAX_C>& columns,
        uint8_t transform) const {
        const auto map = sevenRowTransformMap(transform);
        std::array<SevenRowTailColumn, MAX_C> output = columns;
        for (int column = 0; column < ctx_.c; ++column) {
            const int oldFirstRow = columns[column].firstSide;
            const int oldSecondRow = 2 + columns[column].secondSide;
            const int newFirstRow = map[oldFirstRow];
            const int newSecondRow = map[oldSecondRow];
            if (newFirstRow < 2) {
                output[column].firstSide = (uint8_t)newFirstRow;
                output[column].secondSide = (uint8_t)(newSecondRow - 2);
            } else {
                output[column].firstSide = (uint8_t)newSecondRow;
                output[column].secondSide = (uint8_t)(newFirstRow - 2);
            }
        }
        return output;
    }

    struct SevenRowCanonicalHalf {
        uint64_t key = 0;
        uint8_t transform = 0;
        uint8_t transformMask = 0;
        std::array<SevenRowTailColumn, MAX_C> columns{};
    };

    SevenRowCanonicalHalf canonicalSevenRowHalf(
        const std::array<SevenRowTailColumn, MAX_C>& columns) const {
        SevenRowCanonicalHalf best;
        best.key = std::numeric_limits<uint64_t>::max();
        for (uint8_t transform = 0; transform < 8; ++transform) {
            auto candidate = transformSevenRowTailColumns(columns, transform);
            const uint64_t key = packSevenRowTailKernelKey(candidate);
            if (key < best.key) {
                best.key = key;
                best.transform = transform;
                best.transformMask = (uint8_t)(1u << transform);
                best.columns = std::move(candidate);
            } else if (key == best.key) {
                best.transformMask |= (uint8_t)(1u << transform);
            }
        }
        return best;
    }

    struct SevenRowTailSignature {
        uint64_t firstKey = 0;
        uint64_t secondKey = 0;
        uint8_t relativeTransform = 0;

        bool operator==(const SevenRowTailSignature&) const = default;
    };

    struct SevenRowTailSignatureHash {
        size_t operator()(const SevenRowTailSignature& signature) const noexcept {
            auto mix = [](uint64_t value) {
                value = (value ^ (value >> 30)) * 0xbf58476d1ce4e5b9ULL;
                value = (value ^ (value >> 27)) * 0x94d049bb133111ebULL;
                return value ^ (value >> 31);
            };
            uint64_t hash = mix(signature.firstKey + 0x9e3779b97f4a7c15ULL);
            hash ^= mix(signature.secondKey + 0xd6e8feb86659fd93ULL) +
                0x9e3779b97f4a7c15ULL + (hash << 6) + (hash >> 2);
            hash ^= (uint64_t)signature.relativeTransform *
                0xa0761d6478bd642fULL;
            return (size_t)hash;
        }
    };

    struct SevenRowColorTailSignature {
        uint64_t firstKey = 0;
        uint64_t secondKey = 0;
        uint16_t relativeColor = 0;
        uint8_t relativeTransform = 0;

        bool operator==(const SevenRowColorTailSignature&) const = default;
    };

    struct SevenRowColorTailSignatureHash {
        size_t operator()(
            const SevenRowColorTailSignature& signature) const noexcept {
            auto mix = [](uint64_t value) {
                value = (value ^ (value >> 30)) * 0xbf58476d1ce4e5b9ULL;
                value = (value ^ (value >> 27)) * 0x94d049bb133111ebULL;
                return value ^ (value >> 31);
            };
            uint64_t hash = mix(
                signature.firstKey + 0x9e3779b97f4a7c15ULL);
            hash ^= mix(signature.secondKey + 0xd6e8feb86659fd93ULL) +
                0x9e3779b97f4a7c15ULL + (hash << 6) + (hash >> 2);
            hash ^= mix((uint64_t)signature.relativeColor |
                ((uint64_t)signature.relativeTransform << 16));
            return (size_t)hash;
        }
    };

    static bool sevenRowTailSignatureLess(
        const SevenRowTailSignature& first,
        const SevenRowTailSignature& second) {
        if (first.firstKey != second.firstKey) {
            return first.firstKey < second.firstKey;
        }
        if (first.secondKey != second.secondKey) {
            return first.secondKey < second.secondKey;
        }
        return first.relativeTransform < second.relativeTransform;
    }

    static bool sevenRowColorTailSignatureLess(
        const SevenRowColorTailSignature& first,
        const SevenRowColorTailSignature& second) {
        if (first.firstKey != second.firstKey) {
            return first.firstKey < second.firstKey;
        }
        if (first.secondKey != second.secondKey) {
            return first.secondKey < second.secondKey;
        }
        if (first.relativeTransform != second.relativeTransform) {
            return first.relativeTransform < second.relativeTransform;
        }
        return first.relativeColor < second.relativeColor;
    }

    static uint8_t sevenRowTransformForMap(
        const std::array<uint8_t, 5>& wanted) {
        for (uint8_t transform = 0; transform < 8; ++transform) {
            if (sevenRowTransformMap(transform) == wanted) return transform;
        }
        throw std::runtime_error(
            "future-twin seven-row relative transform is outside D8");
    }

    static uint8_t sevenRowRelativeTransform(
        uint8_t leftTransform, uint8_t rightTransform) {
        const auto leftMap = sevenRowTransformMap(leftTransform);
        const auto rightMap = sevenRowTransformMap(rightTransform);
        std::array<uint8_t, 5> relative{};
        for (int originalRow = 0; originalRow < 5; ++originalRow) {
            relative[leftMap[originalRow]] = rightMap[originalRow];
        }
        return sevenRowTransformForMap(relative);
    }

    static uint8_t sevenRowInverseTransform(uint8_t transform) {
        const auto map = sevenRowTransformMap(transform);
        std::array<uint8_t, 5> inverse{};
        for (int row = 0; row < 5; ++row) inverse[map[row]] = (uint8_t)row;
        return sevenRowTransformForMap(inverse);
    }

    static uint8_t sevenRowComposeTransforms(
        uint8_t first, uint8_t second) {
        const auto firstMap = sevenRowTransformMap(first);
        const auto secondMap = sevenRowTransformMap(second);
        std::array<uint8_t, 5> composition{};
        for (int row = 0; row < 5; ++row) {
            composition[row] = secondMap[firstMap[row]];
        }
        return sevenRowTransformForMap(composition);
    }

    uint16_t colorPermutationIndex(
        const std::array<uint8_t, MAX_C>& permutation) const {
        uint64_t rank = 0;
        for (int position = 0; position < ctx_.c; ++position) {
            int smaller = 0;
            for (int later = position + 1; later < ctx_.c; ++later) {
                smaller += permutation[later] < permutation[position];
            }
            rank += (uint64_t)smaller *
                ctx_.factorial[ctx_.c - position - 1];
        }
        if (rank >= ctx_.permutations.size() ||
            ctx_.permutations[(size_t)rank] != permutation) {
            throw std::runtime_error(
                "future-twin color permutation rank failed");
        }
        return (uint16_t)rank;
    }

    uint16_t relativeColorPermutation(
        uint16_t leftPermutation, uint16_t rightPermutation) const {
        if (leftPermutation >= ctx_.permutations.size() ||
            rightPermutation >= ctx_.permutations.size()) {
            throw std::runtime_error(
                "future-twin relative color permutation is invalid");
        }
        const auto& inverseLeft = ctx_.permutations[
            ctx_.inversePermutation[leftPermutation]];
        const auto& right = ctx_.permutations[rightPermutation];
        std::array<uint8_t, MAX_C> relative{};
        for (int color = 0; color < MAX_C; ++color) {
            relative[color] = (uint8_t)color;
        }
        for (int leftColor = 0; leftColor < ctx_.c; ++leftColor) {
            relative[leftColor] = right[inverseLeft[leftColor]];
        }
        return colorPermutationIndex(relative);
    }

    SevenRowTailSignature sevenRowSignatureForHalves(
        const SevenRowCanonicalHalf& left,
        const SevenRowCanonicalHalf& right) const {
        SevenRowTailSignature best;
        best.firstKey = std::numeric_limits<uint64_t>::max();
        best.secondKey = std::numeric_limits<uint64_t>::max();
        best.relativeTransform = std::numeric_limits<uint8_t>::max();
        for (uint8_t leftTransform = 0; leftTransform < 8; ++leftTransform) {
            if (!((left.transformMask >> leftTransform) & 1u)) continue;
            for (uint8_t rightTransform = 0; rightTransform < 8;
                 ++rightTransform) {
                if (!((right.transformMask >> rightTransform) & 1u)) continue;
                SevenRowTailSignature candidate{
                    left.key, right.key,
                    sevenRowRelativeTransform(leftTransform, rightTransform)};
                SevenRowTailSignature swapped{
                    right.key, left.key,
                    sevenRowInverseTransform(candidate.relativeTransform)};
                if (sevenRowTailSignatureLess(swapped, candidate)) {
                    candidate = swapped;
                }
                if (sevenRowTailSignatureLess(candidate, best)) best = candidate;
            }
        }
        if (best.relativeTransform >= 8) {
            throw std::runtime_error(
                "future-twin seven-row signature canonicalization failed");
        }
        return best;
    }

    struct SevenRowKernelView {
        std::shared_ptr<const TailKernel> owner;
        TailKernelSlice kernel;
        uint8_t transform = 0;
        uint16_t colorPermutation = 0;
    };

    SevenRowKernelView getSevenRowTailKernel(
        const SevenRowCanonicalHalf& canonical,
        size_t cacheRecordCap,
        TailEvaluation& evaluation,
        uint16_t colorPermutation = 0) const {
        ++evaluation.kernelLookups;
        const uint64_t key = canonical.key;
        if (sevenRowPersistentKernelTable_) {
            const std::optional<TailKernelSlice> persistent =
                sevenRowPersistentKernelTable_->find(key);
            if (persistent) {
                ++evaluation.kernelHits;
                return {{}, *persistent, canonical.transform,
                        colorPermutation};
            }
        }
        auto evictOne = [&]() {
            if (sevenRowKernelCacheOrder_.empty()) return false;
            const uint64_t victim = sevenRowKernelCacheOrder_.front();
            sevenRowKernelCacheOrder_.pop_front();
            const auto iterator = sevenRowKernelCache_.find(victim);
            if (iterator == sevenRowKernelCache_.end()) return true;
            sevenRowKernelCachedRecords_ -= iterator->second.kernel->size();
            sevenRowKernelCache_.erase(iterator);
            ++evaluation.kernelEvictions;
            return true;
        };
        if (cacheRecordCap == 0) {
            while (evictOne()) {}
        } else {
            while (sevenRowKernelCachedRecords_ > cacheRecordCap && evictOne()) {}
            const auto cached = sevenRowKernelCache_.find(key);
            if (cached != sevenRowKernelCache_.end()) {
                ++evaluation.kernelHits;
                return {cached->second.kernel,
                        TailKernelSlice{
                            cached->second.kernel->data(),
                            cached->second.kernel->size()},
                        canonical.transform, colorPermutation};
            }
        }

        uint64_t localAssignments = 0;
        auto kernel = std::make_shared<const TailKernel>(
            makeSevenRowTailKernel(canonical.columns, localAssignments));
        evaluation.localAssignments += localAssignments;
        if (cacheRecordCap != 0 && kernel->size() <= cacheRecordCap) {
            while (sevenRowKernelCachedRecords_ + kernel->size() > cacheRecordCap &&
                   evictOne()) {}
            if (sevenRowKernelCachedRecords_ + kernel->size() <= cacheRecordCap) {
                sevenRowKernelCachedRecords_ += kernel->size();
                sevenRowKernelCache_.emplace(
                    key, SevenRowKernelCacheEntry{kernel});
                sevenRowKernelCacheOrder_.push_back(key);
            }
        }
        return {kernel, TailKernelSlice{kernel->data(), kernel->size()},
                canonical.transform, colorPermutation};
    }

    struct SevenRowCutCandidate {
        std::array<std::array<SevenRowTailColumn, MAX_C>, 2> halves{};
        std::array<SevenRowCanonicalHalf, 2> canonical{};
    };

    std::array<SevenRowCutCandidate, 3> sevenRowTailCandidates(
        const State& state) const {
        if (ctx_.c < 4) {
            throw std::runtime_error(
                "future-twin seven-row tail requires C >= 4");
        }
        constexpr int remainingRows = 7;
        const int processedRows = ctx_.m - remainingRows;
        std::array<uint16_t, remainingRows> rowColumns{};
        for (int column = 0; column < ctx_.m; ++column) {
            const uint16_t tau = Canonicalizer::tauOf(state.record[column]);
            const uint8_t colors = Canonicalizer::colorsOf(state.record[column]);
            if ((tau & ~0x7fu) != 0 ||
                std::popcount((unsigned)tau) !=
                    ctx_.c - std::popcount((unsigned)colors) ||
                (std::popcount((unsigned)tau) != 3 &&
                 std::popcount((unsigned)tau) != 4)) {
                throw std::runtime_error(
                    "future-twin seven-row tail state invariant failed");
            }
            uint16_t bits = tau;
            while (bits) {
                const int row = std::countr_zero(bits);
                bits &= (uint16_t)(bits - 1);
                rowColumns[row] |= (uint16_t)(1u << column);
            }
        }

        const int pending = pendingMateVertex(
            state, processedRows, remainingRows);
        const uint16_t fullColumns = (uint16_t)((1u << ctx_.m) - 1u);
        std::array<std::array<int, 2>, 3> pairs{};
        std::array<bool, remainingRows> usedRows{};
        usedRows[pending] = true;
        for (int pair = 0; pair < 3; ++pair) {
            int first = -1;
            for (int row = 0; row < remainingRows; ++row) {
                if (!usedRows[row]) {
                    first = row;
                    break;
                }
            }
            if (first < 0 ||
                std::popcount((unsigned)rowColumns[first]) != ctx_.c) {
                throw std::runtime_error(
                    "future-twin seven-row degree invariant failed");
            }
            const uint16_t complement =
                (uint16_t)(fullColumns ^ rowColumns[first]);
            int second = -1;
            for (int row = first + 1; row < remainingRows; ++row) {
                if (!usedRows[row] && rowColumns[row] == complement) {
                    second = row;
                    break;
                }
            }
            if (second < 0) {
                throw std::runtime_error(
                    "future-twin seven-row tail could not recover row pair");
            }
            usedRows[first] = true;
            usedRows[second] = true;
            pairs[pair] = {first, second};
        }

        auto halfColumns = [&](int cutPair, int side) {
            std::array<SevenRowTailColumn, MAX_C> output{};
            std::array<int, 2> other{};
            int otherCount = 0;
            for (int pair = 0; pair < 3; ++pair) {
                if (pair != cutPair) other[otherCount++] = pair;
            }
            int count = 0;
            const int cutRow = pairs[cutPair][side];
            for (int column = 0; column < ctx_.m; ++column) {
                if (!((rowColumns[cutRow] >> column) & 1u)) continue;
                SevenRowTailColumn item;
                for (int index = 0; index < 2; ++index) {
                    const int pair = other[index];
                    const bool firstIncident =
                        ((rowColumns[pairs[pair][0]] >> column) & 1u) != 0;
                    const bool secondIncident =
                        ((rowColumns[pairs[pair][1]] >> column) & 1u) != 0;
                    if (firstIncident == secondIncident) {
                        throw std::runtime_error(
                            "future-twin seven-row pair incidence invariant failed");
                    }
                    if (index == 0) item.firstSide = firstIncident ? 0 : 1;
                    else item.secondSide = firstIncident ? 0 : 1;
                }
                item.pending = (uint8_t)(
                    (rowColumns[pending] >> column) & 1u);
                item.available = (uint8_t)(ctx_.fullColors &
                    (uint8_t)~Canonicalizer::colorsOf(state.record[column]));
                if (std::popcount((unsigned)item.available) !=
                    3 + (item.pending != 0)) {
                    throw std::runtime_error(
                        "future-twin seven-row availability invariant failed");
                }
                if (count >= ctx_.c) {
                    throw std::runtime_error(
                        "future-twin seven-row half has too many columns");
                }
                output[count++] = item;
            }
            if (count != ctx_.c) {
                throw std::runtime_error(
                    "future-twin seven-row half has wrong column count");
            }
            return output;
        };

        std::array<SevenRowCutCandidate, 3> candidates{};
        for (int cut = 0; cut < 3; ++cut) {
            for (int side = 0; side < 2; ++side) {
                candidates[cut].halves[side] = halfColumns(cut, side);
                candidates[cut].canonical[side] =
                    canonicalSevenRowHalf(candidates[cut].halves[side]);
            }
        }
        return candidates;
    }

    SevenRowTailSignature sevenRowTailSignature(
        const State& state, int* selectedCut = nullptr) const {
        const auto candidates = sevenRowTailCandidates(state);
        SevenRowTailSignature best;
        best.firstKey = std::numeric_limits<uint64_t>::max();
        best.secondKey = std::numeric_limits<uint64_t>::max();
        best.relativeTransform = std::numeric_limits<uint8_t>::max();
        int bestCut = -1;
        for (int cut = 0; cut < 3; ++cut) {
            const SevenRowTailSignature candidate = sevenRowSignatureForHalves(
                candidates[cut].canonical[0], candidates[cut].canonical[1]);
            if (sevenRowTailSignatureLess(candidate, best)) {
                best = candidate;
                bestCut = cut;
            }
        }
        if (bestCut < 0) {
            throw std::runtime_error(
                "future-twin seven-row full signature failed");
        }
        if (selectedCut != nullptr) *selectedCut = bestCut;
        return best;
    }

    uint64_t combineSevenRowTailKernels(
        const TailKernelSlice& left, const TailKernelSlice& right,
        uint8_t relativeTransform,
        uint16_t relativeColor = 0) const {
        const auto relativeMap = sevenRowTransformMap(relativeTransform);
        if (relativeColor >= ctx_.maskMaps.size()) {
            throw std::runtime_error(
                "future-twin tail kernel color transform is invalid");
        }
        const auto& colorMap = ctx_.maskMaps[relativeColor];
        uint64_t value = 0;
        for (const auto& [signature, weight] : left) {
            std::array<uint8_t, 5> leftMasks{};
            for (int row = 0; row < 5; ++row) {
                leftMasks[row] = (uint8_t)(
                    (signature >> (row * ctx_.c)) & ctx_.fullColors);
            }
            uint32_t target = 0;
            for (int leftRow = 0; leftRow < 5; ++leftRow) {
                const uint8_t complement = (uint8_t)(
                    ctx_.fullColors ^ colorMap[leftMasks[leftRow]]);
                target |= (uint32_t)complement <<
                    (relativeMap[leftRow] * ctx_.c);
            }
            const auto iterator = std::lower_bound(
                right.begin(), right.end(), target,
                [](const auto& entry, uint32_t wanted) {
                    return entry.first < wanted;
                });
            if (iterator != right.end() && iterator->first == target) {
                value += (uint64_t)weight * iterator->second;
            }
        }
        return value;
    }

    uint64_t combineSevenRowTailKernelsSmallSide(
        const TailKernelSlice& left, const TailKernelSlice& right,
        uint8_t relativeTransform,
        uint16_t relativeColor = 0) const {
        if (relativeColor >= ctx_.inversePermutation.size()) {
            throw std::runtime_error(
                "future-twin tail kernel color transform is invalid");
        }
        if (left.size() <= right.size()) {
            return combineSevenRowTailKernels(
                left, right,
                relativeTransform, relativeColor);
        }
        return combineSevenRowTailKernels(
            right, left,
            sevenRowInverseTransform(relativeTransform),
            ctx_.inversePermutation[relativeColor]);
    }

    uint64_t combineSevenRowTailKernelViews(
        const SevenRowKernelView& left,
        const SevenRowKernelView& right) const {
        return combineSevenRowTailKernelsSmallSide(
            left.kernel, right.kernel,
            sevenRowRelativeTransform(left.transform, right.transform),
            relativeColorPermutation(
                left.colorPermutation, right.colorPermutation));
    }

    SevenRowCanonicalHalf sevenRowCanonicalHalfFromKey(uint64_t key) const {
        SevenRowCanonicalHalf canonical;
        canonical.key = key;
        canonical.transform = 0;
        canonical.transformMask = 1;
        for (int column = 0; column < ctx_.c; ++column) {
            const uint16_t type = (uint16_t)((key >> (9 * column)) & 0x1ffu);
            SevenRowTailColumn& item = canonical.columns[column];
            item.available = (uint8_t)(type & 0x3fu);
            item.firstSide = (uint8_t)((type >> 6) & 1u);
            item.secondSide = (uint8_t)((type >> 7) & 1u);
            item.pending = (uint8_t)((type >> 8) & 1u);
        }
        if (packSevenRowTailKernelKey(canonical.columns) != key) {
            throw std::runtime_error(
                "future-twin seven-row signature key decode failed");
        }
        return canonical;
    }

    SevenRowColorCanonicalHalf canonicalSevenRowHalfWithColors(
        uint64_t key) const {
        const SevenRowCanonicalHalf decoded =
            sevenRowCanonicalHalfFromKey(key);
        SevenRowColorCanonicalHalf best;
        for (uint8_t rowTransform = 0; rowTransform < 8; ++rowTransform) {
            const auto rowColumns = transformSevenRowTailColumns(
                decoded.columns, rowTransform);
            for (uint16_t permutation = 0;
                 permutation < (uint16_t)ctx_.maskMaps.size();
                 ++permutation) {
                auto candidate = rowColumns;
                const auto& maskMap = ctx_.maskMaps[permutation];
                for (int column = 0; column < ctx_.c; ++column) {
                    candidate[column].available =
                        maskMap[candidate[column].available];
                }
                const uint64_t candidateKey =
                    packSevenRowTailKernelKey(candidate);
                if (candidateKey < best.key) {
                    best.key = candidateKey;
                    best.rowTransform = rowTransform;
                    best.colorPermutation = permutation;
                }
            }
        }
        return best;
    }

    SevenRowColorCanonicalHalf canonicalSevenRowHalfWithColorRefinement(
        uint64_t key) const {
        const SevenRowCanonicalHalf decoded =
            sevenRowCanonicalHalfFromKey(key);
        SevenRowColorCanonicalHalf best;
        using ColorProfile = std::array<uint8_t, 8>;
        for (uint8_t rowTransform = 0; rowTransform < 8; ++rowTransform) {
            const auto rowColumns = transformSevenRowTailColumns(
                decoded.columns, rowTransform);
            std::array<ColorProfile, MAX_C> profiles{};
            for (int column = 0; column < ctx_.c; ++column) {
                const SevenRowTailColumn& item = rowColumns[column];
                const int rowType = item.firstSide |
                    (item.secondSide << 1) | (item.pending << 2);
                for (int color = 0; color < ctx_.c; ++color) {
                    profiles[color][rowType] +=
                        (item.available >> color) & 1u;
                }
            }
            std::array<uint8_t, MAX_C> orderedColors{};
            for (int color = 0; color < MAX_C; ++color) {
                orderedColors[color] = (uint8_t)color;
            }
            std::sort(orderedColors.begin(),
                      orderedColors.begin() + ctx_.c,
                [&](uint8_t first, uint8_t second) {
                    if (profiles[first] != profiles[second]) {
                        return profiles[first] < profiles[second];
                    }
                    return first < second;
                });
            std::vector<std::pair<int, int>> cells;
            for (int first = 0; first < ctx_.c;) {
                int last = first + 1;
                while (last < ctx_.c &&
                       profiles[orderedColors[first]] ==
                           profiles[orderedColors[last]]) {
                    ++last;
                }
                cells.emplace_back(first, last);
                first = last;
            }
            std::array<uint8_t, MAX_C> colorMap{};
            for (int color = 0; color < MAX_C; ++color) {
                colorMap[color] = (uint8_t)color;
            }
            std::function<void(size_t)> enumerateCells =
                [&](size_t cellIndex) {
                    if (cellIndex == cells.size()) {
                        auto candidate = rowColumns;
                        for (int column = 0; column < ctx_.c; ++column) {
                            uint8_t image = 0;
                            const uint8_t available =
                                candidate[column].available;
                            for (int color = 0; color < ctx_.c; ++color) {
                                if ((available >> color) & 1u) {
                                    image |= (uint8_t)(1u << colorMap[color]);
                                }
                            }
                            candidate[column].available = image;
                        }
                        const uint64_t candidateKey =
                            packSevenRowTailKernelKey(candidate);
                        if (candidateKey < best.key) {
                            best.key = candidateKey;
                            best.rowTransform = rowTransform;
                            best.colorPermutation =
                                colorPermutationIndex(colorMap);
                        }
                        return;
                    }
                    const auto [first, last] = cells[cellIndex];
                    std::sort(orderedColors.begin() + first,
                              orderedColors.begin() + last);
                    do {
                        for (int target = first; target < last; ++target) {
                            colorMap[orderedColors[target]] =
                                (uint8_t)target;
                        }
                        enumerateCells(cellIndex + 1);
                    } while (std::next_permutation(
                        orderedColors.begin() + first,
                        orderedColors.begin() + last));
                };
            enumerateCells(0);
        }
        return best;
    }

    SevenRowColorCanonicalHalf getSevenRowColorCanonicalHalf(
        uint64_t rawKey, TailEvaluation& evaluation) const {
        ++evaluation.colorCanonicalLookups;
        const auto cached = sevenRowColorCanonicalCache_.find(rawKey);
        if (cached != sevenRowColorCanonicalCache_.end()) {
            ++evaluation.colorCanonicalHits;
            return cached->second;
        }
        const SevenRowColorCanonicalHalf canonical =
            canonicalSevenRowHalfWithColorRefinement(rawKey);
        ++evaluation.colorCanonicalOrbits;
        const auto [inserted, created] =
            sevenRowColorCanonicalCache_.emplace(rawKey, canonical);
        if (!created || inserted->second.key != canonical.key) {
            throw std::runtime_error(
                "future-twin color-canonical mapping insertion failed");
        }
        ++evaluation.colorCanonicalMappings;
        return inserted->second;
    }

    std::array<uint64_t, 6> sevenRowColorCanonicalKernelKeys(
        const State& state) const {
        const auto candidates = sevenRowTailCandidates(state);
        std::array<uint64_t, 6> keys{};
        size_t index = 0;
        for (int cut = 0; cut < 3; ++cut) {
            for (int side = 0; side < 2; ++side) {
                keys[index++] = canonicalSevenRowHalfWithColorRefinement(
                    candidates[cut].canonical[side].key).key;
            }
        }
        return keys;
    }

    SevenRowColorTailSignature sevenRowColorSignatureForHalves(
        const SevenRowCanonicalHalf& leftRaw,
        const SevenRowCanonicalHalf& rightRaw,
        TailEvaluation& evaluation) const {
        const SevenRowColorCanonicalHalf left =
            getSevenRowColorCanonicalHalf(leftRaw.key, evaluation);
        const SevenRowColorCanonicalHalf right =
            getSevenRowColorCanonicalHalf(rightRaw.key, evaluation);
        SevenRowColorTailSignature candidate{
            left.key,
            right.key,
            relativeColorPermutation(
                left.colorPermutation, right.colorPermutation),
            sevenRowRelativeTransform(
                sevenRowComposeTransforms(
                    leftRaw.transform, left.rowTransform),
                sevenRowComposeTransforms(
                    rightRaw.transform, right.rowTransform))};
        SevenRowColorTailSignature swapped{
            candidate.secondKey,
            candidate.firstKey,
            ctx_.inversePermutation[candidate.relativeColor],
            sevenRowInverseTransform(candidate.relativeTransform)};
        if (sevenRowColorTailSignatureLess(swapped, candidate)) {
            candidate = swapped;
        }
        return candidate;
    }

    SevenRowColorTailSignature sevenRowColorTailSignature(
        const State& state, TailEvaluation* counters = nullptr) const {
        TailEvaluation localCounters;
        TailEvaluation& evaluation = counters == nullptr
            ? localCounters : *counters;
        const auto candidates = sevenRowTailCandidates(state);
        SevenRowColorTailSignature best;
        best.firstKey = std::numeric_limits<uint64_t>::max();
        best.secondKey = std::numeric_limits<uint64_t>::max();
        best.relativeColor = std::numeric_limits<uint16_t>::max();
        best.relativeTransform = std::numeric_limits<uint8_t>::max();
        for (int cut = 0; cut < 3; ++cut) {
            const SevenRowColorTailSignature candidate =
                sevenRowColorSignatureForHalves(
                    candidates[cut].canonical[0],
                    candidates[cut].canonical[1], evaluation);
            if (sevenRowColorTailSignatureLess(candidate, best)) {
                best = candidate;
            }
        }
        if (best.relativeTransform >= 8 ||
            best.relativeColor >= ctx_.permutations.size()) {
            throw std::runtime_error(
                "future-twin seven-row color signature failed");
        }
        return best;
    }

    TailEvaluation sevenRowTailValueFromColorSignature(
        const SevenRowColorTailSignature& signature,
        size_t cacheRecordCap) const {
        TailEvaluation result;
        const SevenRowCanonicalHalf first =
            sevenRowCanonicalHalfFromKey(signature.firstKey);
        const SevenRowCanonicalHalf second =
            sevenRowCanonicalHalfFromKey(signature.secondKey);
        const SevenRowKernelView left = getSevenRowTailKernel(
            first, cacheRecordCap, result);
        const SevenRowKernelView right = getSevenRowTailKernel(
            second, cacheRecordCap, result);
        result.leftSupport = (uint32_t)left.kernel.size();
        result.rightSupport = (uint32_t)right.kernel.size();
        result.value = combineSevenRowTailKernelsSmallSide(
            left.kernel, right.kernel,
            signature.relativeTransform, signature.relativeColor);
        return result;
    }

    TailEvaluation sevenRowTailValueFromSignature(
        const SevenRowTailSignature& signature,
        size_t cacheRecordCap) const {
        TailEvaluation result;
        const SevenRowCanonicalHalf first =
            sevenRowCanonicalHalfFromKey(signature.firstKey);
        const SevenRowCanonicalHalf second =
            sevenRowCanonicalHalfFromKey(signature.secondKey);
        const SevenRowKernelView left = getSevenRowTailKernel(
            first, cacheRecordCap, result);
        const SevenRowKernelView right = getSevenRowTailKernel(
            second, cacheRecordCap, result);
        result.leftSupport = (uint32_t)left.kernel.size();
        result.rightSupport = (uint32_t)right.kernel.size();
        result.value = combineSevenRowTailKernelsSmallSide(
            left.kernel, right.kernel, signature.relativeTransform);
        return result;
    }

    TailEvaluation sevenRowTailValue(
        const State& state, size_t cacheRecordCap,
        bool useColorCanonical = false) const {
        const auto candidates = sevenRowTailCandidates(state);
        TailEvaluation result;

        auto halfProxy = [&](const auto& columns) {
            std::array<uint8_t, MAX_C> allowed{};
            uint64_t proxy = 1;
            for (int column = 0; column < ctx_.c; ++column) {
                allowed[column] = columns[column].available;
                proxy *= columns[column].pending ? 6u : 2u;
            }
            return proxy * tailAssignmentCount(allowed);
        };

        struct CutWork {
            std::array<bool, 2> cached{};
            int cachedCount = 0;
            uint64_t workProxy = 0;
        };
        std::array<CutWork, 3> work{};
        std::array<std::array<SevenRowColorCanonicalHalf, 2>, 3>
            colorCanonical{};
        int selectedCut = -1;
        int bestCachedCount = -1;
        uint64_t bestWorkProxy = std::numeric_limits<uint64_t>::max();
        for (int cut = 0; cut < 3; ++cut) {
            const SevenRowCutCandidate& candidate = candidates[cut];
            CutWork& cutWork = work[cut];
            for (int side = 0; side < 2; ++side) {
                uint64_t kernelKey = candidate.canonical[side].key;
                if (useColorCanonical) {
                    ++result.colorCanonicalLookups;
                    colorCanonical[cut][side] =
                        canonicalSevenRowHalfWithColorRefinement(
                            candidate.canonical[side].key);
                    ++result.colorCanonicalOrbits;
                    kernelKey = colorCanonical[cut][side].key;
                }
                if (sevenRowPersistentKernelTable_) {
                    const std::optional<TailKernelSlice> persistent =
                        sevenRowPersistentKernelTable_->find(kernelKey);
                    if (persistent) {
                        cutWork.cached[side] = true;
                        ++cutWork.cachedCount;
                        cutWork.workProxy += persistent->size();
                        continue;
                    }
                }
                if (cacheRecordCap != 0) {
                    const auto iterator = sevenRowKernelCache_.find(
                        kernelKey);
                    if (iterator != sevenRowKernelCache_.end()) {
                        cutWork.cached[side] = true;
                        ++cutWork.cachedCount;
                        cutWork.workProxy += iterator->second.kernel->size();
                        continue;
                    }
                }
                cutWork.workProxy += halfProxy(candidate.halves[side]);
            }
            if (cutWork.cachedCount > bestCachedCount ||
                (cutWork.cachedCount == bestCachedCount &&
                 cutWork.workProxy < bestWorkProxy)) {
                bestCachedCount = cutWork.cachedCount;
                bestWorkProxy = cutWork.workProxy;
                selectedCut = cut;
            }
        }
        if (selectedCut < 0) {
            throw std::runtime_error(
                "future-twin seven-row cut selection failed");
        }

        const SevenRowCutCandidate& selected = candidates[selectedCut];
        const CutWork& selectedWork = work[selectedCut];
        std::array<SevenRowCanonicalHalf, 2> kernelCanonical{};
        std::array<uint16_t, 2> colorPermutation{};
        for (int side = 0; side < 2; ++side) {
            if (useColorCanonical) {
                const SevenRowColorCanonicalHalf& color =
                    colorCanonical[selectedCut][side];
                kernelCanonical[side] =
                    sevenRowCanonicalHalfFromKey(color.key);
                kernelCanonical[side].transform =
                    sevenRowComposeTransforms(
                        selected.canonical[side].transform,
                        color.rowTransform);
                colorPermutation[side] = color.colorPermutation;
            } else {
                kernelCanonical[side] = selected.canonical[side];
            }
        }
        std::array<SevenRowKernelView, 2> views{};
        const int firstSide = selectedWork.cached[0] ? 0 :
                              selectedWork.cached[1] ? 1 : 0;
        views[firstSide] = getSevenRowTailKernel(
            kernelCanonical[firstSide], cacheRecordCap, result,
            colorPermutation[firstSide]);
        views[1 - firstSide] = getSevenRowTailKernel(
            kernelCanonical[1 - firstSide], cacheRecordCap, result,
            colorPermutation[1 - firstSide]);
        const SevenRowKernelView& left = views[0];
        const SevenRowKernelView& right = views[1];
        result.leftSupport = (uint32_t)left.kernel.size();
        result.rightSupport = (uint32_t)right.kernel.size();
        result.value = combineSevenRowTailKernelViews(left, right);
        return result;
    }

    int pendingMateVertex(const State& state, int processedRows,
                          int remainingRows) const {
        if ((processedRows & 1) == 0) {
            throw std::runtime_error("future-twin mate requested at an even boundary");
        }
        const int lowerColorCount = processedRows / 2;
        for (int row = 0; row < remainingRows; ++row) {
            int degree = 0;
            bool allLower = true;
            for (int column = 0; column < ctx_.m; ++column) {
                const uint32_t record = state.record[column];
                if (!((Canonicalizer::tauOf(record) >> row) & 1u)) continue;
                ++degree;
                if (std::popcount((unsigned)Canonicalizer::colorsOf(record)) !=
                    lowerColorCount) {
                    allLower = false;
                }
            }
            if (degree == ctx_.c && allLower) return row;
        }
        throw std::runtime_error("future-twin could not identify pending complementary mate");
    }

    uint64_t groupedTransitionLeafCount(const State& state, int vertex) const {
        const uint16_t vertexBit = (uint16_t)(1u << vertex);
        std::array<uint64_t, 1 << MAX_C> current{};
        std::array<uint64_t, 1 << MAX_C> next{};
        current[0] = 1;
        for (int first = 0; first < ctx_.m;) {
            int last = first + 1;
            while (last < ctx_.m &&
                   state.record[last] == state.record[first]) {
                ++last;
            }
            const uint16_t tau =
                Canonicalizer::tauOf(state.record[first]);
            if (tau & vertexBit) {
                const uint8_t colors =
                    Canonicalizer::colorsOf(state.record[first]);
                const int multiplicity = last - first;
                next.fill(0);
                for (int used = 0; used <= ctx_.fullColors; ++used) {
                    if (current[(size_t)used] == 0) continue;
                    const uint8_t available = (uint8_t)(
                        ctx_.fullColors & ~used & ~colors);
                    for (uint8_t chosen :
                         ctx_.subsets[available][multiplicity]) {
                        next[(size_t)(used | chosen)] +=
                            current[(size_t)used];
                    }
                }
                current = next;
            }
            first = last;
        }
        return current[ctx_.fullColors];
    }

    int adaptivePairVertex(const State& state, int remainingRows) const {
        int bestVertex = -1;
        uint64_t bestLeaves = std::numeric_limits<uint64_t>::max();
        for (int vertex = 0; vertex < remainingRows; ++vertex) {
            const uint64_t leaves =
                groupedTransitionLeafCount(state, vertex);
            if (leaves < bestLeaves ||
                (leaves == bestLeaves && vertex > bestVertex)) {
                bestLeaves = leaves;
                bestVertex = vertex;
            }
        }
        if (bestVertex < 0) {
            throw std::runtime_error(
                "future-twin adaptive pair selection failed");
        }
        return bestVertex;
    }

    struct ExternalKey {
        std::array<uint16_t, MAX_M> rank{};
        uint8_t keyKind = 0;
    };

    struct ExternalRecord {
        ExternalKey key;
        Count weight = 0;
    };

    struct ExternalFileInfo {
        std::filesystem::path path;
        uint64_t records = 0;
        Count mass = 0;
        int processedRows = 0;
    };

    static bool externalKeyLess(const ExternalKey& first, const ExternalKey& second) {
        if (first.rank != second.rank) return first.rank < second.rank;
        return first.keyKind < second.keyKind;
    }

    static bool externalKeyEqual(const ExternalKey& first, const ExternalKey& second) {
        return first.keyKind == second.keyKind && first.rank == second.rank;
    }

    class ExternalCodec {
        const Context& ctx;
        int rows;
        std::vector<uint32_t> recordByRank;
        std::vector<int32_t> rankByRecord;

    public:
        ExternalCodec(const Context& context, int remainingRows)
            : ctx(context), rows(remainingRows), rankByRecord(1u << 18, -1) {
            const int featureCount = rows + ctx.c;
            std::function<void(int, int, uint32_t)> enumerate =
                [&](int next, int needed, uint32_t mask) {
                    if (needed == 0) {
                        const uint16_t tau = (uint16_t)(mask & ((1u << rows) - 1u));
                        const uint8_t colors = (uint8_t)(mask >> rows);
                        const uint32_t record = Canonicalizer::pack(tau, colors);
                        if (record >= rankByRecord.size()) {
                            throw std::runtime_error("future external record index overflow");
                        }
                        rankByRecord[record] = (int32_t)recordByRank.size();
                        recordByRank.push_back(record);
                        return;
                    }
                    for (int feature = next; feature <= featureCount - needed; ++feature) {
                        enumerate(feature + 1, needed - 1, mask | (1u << feature));
                    }
                };
            enumerate(0, ctx.c, 0);
            if (recordByRank.size() > std::numeric_limits<uint16_t>::max()) {
                throw std::runtime_error("future external codec rank overflow");
            }
        }

        ExternalKey encode(const State& state) const {
            ExternalKey key;
            key.rank.fill(std::numeric_limits<uint16_t>::max());
            for (int column = 0; column < ctx.m; ++column) {
                const uint32_t record = state.record[column];
                if (record >= rankByRecord.size() || rankByRecord[record] < 0) {
                    throw std::runtime_error("future external state record is not rankable");
                }
                key.rank[column] = (uint16_t)rankByRecord[record];
            }
            std::sort(key.rank.begin(), key.rank.begin() + ctx.m);
            key.keyKind = state.keyKind;
            return key;
        }

        State decode(const ExternalKey& key) const {
            State state;
            state.record.fill(EMPTY_RECORD);
            for (int column = 0; column < ctx.m; ++column) {
                const uint16_t rank = key.rank[column];
                if (rank >= recordByRank.size()) {
                    throw std::runtime_error("future external state rank is invalid");
                }
                state.record[column] = recordByRank[rank];
            }
            std::sort(state.record.begin(), state.record.begin() + ctx.m);
            state.keyKind = key.keyKind;
            return state;
        }
    };

    static void checkedAdd(Count& destination, Count value, const char* detail) {
        const Count maximum = ~(Count)0;
        if (destination > maximum - value) throw std::overflow_error(detail);
        destination += value;
    }

    static void writeExternalHeader(std::ostream& output, int c, int processedRows,
                                    uint64_t records, Count mass) {
        std::array<char, 40> bytes{};
        const char magic[8] = {'F', 'J', 'F', 'T', 'L', '0', '1', '\0'};
        std::memcpy(bytes.data(), magic, sizeof(magic));
        bytes[8] = (char)c;
        bytes[9] = (char)processedRows;
        const uint64_t low = (uint64_t)mass;
        const uint64_t high = (uint64_t)(mass >> 64);
        std::memcpy(bytes.data() + 16, &records, sizeof(records));
        std::memcpy(bytes.data() + 24, &low, sizeof(low));
        std::memcpy(bytes.data() + 32, &high, sizeof(high));
        output.write(bytes.data(), (std::streamsize)bytes.size());
        if (!output) throw std::runtime_error("future external header write failed");
    }

    static ExternalFileInfo readExternalHeader(std::istream& input,
                                               const std::filesystem::path& path,
                                               int expectedC,
                                               int expectedProcessedRows) {
        std::array<char, 40> bytes{};
        input.read(bytes.data(), (std::streamsize)bytes.size());
        const char magic[8] = {'F', 'J', 'F', 'T', 'L', '0', '1', '\0'};
        if (!input || std::memcmp(bytes.data(), magic, sizeof(magic)) != 0 ||
            (uint8_t)bytes[8] != expectedC ||
            (uint8_t)bytes[9] != expectedProcessedRows) {
            throw std::runtime_error("future external header validation failed: " + path.string());
        }
        ExternalFileInfo info;
        info.path = path;
        info.processedRows = expectedProcessedRows;
        uint64_t low = 0;
        uint64_t high = 0;
        std::memcpy(&info.records, bytes.data() + 16, sizeof(info.records));
        std::memcpy(&low, bytes.data() + 24, sizeof(low));
        std::memcpy(&high, bytes.data() + 32, sizeof(high));
        info.mass = (Count)low | ((Count)high << 64);
        return info;
    }

    static void writeExternalRecord(std::ostream& output, const ExternalRecord& record) {
        std::array<char, 41> bytes{};
        for (int i = 0; i < MAX_M; ++i) {
            std::memcpy(bytes.data() + 2 * i, &record.key.rank[i], sizeof(uint16_t));
        }
        bytes[24] = (char)record.key.keyKind;
        const uint64_t low = (uint64_t)record.weight;
        const uint64_t high = (uint64_t)(record.weight >> 64);
        std::memcpy(bytes.data() + 25, &low, sizeof(low));
        std::memcpy(bytes.data() + 33, &high, sizeof(high));
        output.write(bytes.data(), (std::streamsize)bytes.size());
        if (!output) throw std::runtime_error("future external record write failed");
    }

    static bool readExternalRecord(std::istream& input, ExternalRecord& record) {
        std::array<char, 41> bytes{};
        input.read(bytes.data(), (std::streamsize)bytes.size());
        if (!input) return false;
        for (int i = 0; i < MAX_M; ++i) {
            std::memcpy(&record.key.rank[i], bytes.data() + 2 * i, sizeof(uint16_t));
        }
        record.key.keyKind = (uint8_t)bytes[24];
        uint64_t low = 0;
        uint64_t high = 0;
        std::memcpy(&low, bytes.data() + 25, sizeof(low));
        std::memcpy(&high, bytes.data() + 33, sizeof(high));
        record.weight = (Count)low | ((Count)high << 64);
        return true;
    }

    class ExternalReader {
        std::ifstream input;
        uint64_t recordsRead = 0;
        Count massRead = 0;
        ExternalKey previous{};
        bool havePrevious = false;
        bool finalAuditDone = false;

    public:
        ExternalFileInfo info;

        ExternalReader(const std::filesystem::path& path, int c, int processedRows)
            : input(path, std::ios::binary),
              info(readExternalHeader(input, path, c, processedRows)) {
            if (!input) throw std::runtime_error("future external file open failed: " + path.string());
            const uintmax_t expectedBytes = 40u + 41u * (uintmax_t)info.records;
            if (std::filesystem::file_size(path) != expectedBytes) {
                throw std::runtime_error(
                    "future external file size audit failed: " + path.string());
            }
        }

        bool next(ExternalRecord& record) {
            if (recordsRead == info.records) {
                if (!finalAuditDone) {
                    if (massRead != info.mass) {
                        throw std::runtime_error(
                            "future external input mass audit failed: " + info.path.string());
                    }
                    finalAuditDone = true;
                }
                return false;
            }
            if (!readExternalRecord(input, record)) {
                throw std::runtime_error("future external file was truncated: " + info.path.string());
            }
            if (record.weight == 0) {
                throw std::runtime_error(
                    "future external file contains zero weight: " + info.path.string());
            }
            if (havePrevious && !externalKeyLess(previous, record.key)) {
                throw std::runtime_error(
                    "future external file is not strictly sorted: " + info.path.string());
            }
            previous = record.key;
            havePrevious = true;
            checkedAdd(massRead, record.weight, "future external input mass overflow");
            ++recordsRead;
            return true;
        }
    };

    using ExternalManifest = std::map<std::string, std::string>;

    static ExternalManifest readExternalManifest(
        const std::filesystem::path& path) {
        std::ifstream input(path);
        if (!input) {
            throw std::runtime_error(
                "future external manifest open failed: " + path.string());
        }
        ExternalManifest values;
        std::string line;
        while (std::getline(input, line)) {
            const size_t equals = line.find('=');
            if (equals == std::string::npos || equals == 0) {
                throw std::runtime_error(
                    "future external malformed manifest: " + path.string());
            }
            const std::string key = line.substr(0, equals);
            const std::string value = line.substr(equals + 1);
            if (!values.emplace(key, value).second) {
                throw std::runtime_error(
                    "future external duplicate manifest key: " + path.string());
            }
        }
        if (!input.eof()) {
            throw std::runtime_error(
                "future external manifest read failed: " + path.string());
        }
        return values;
    }

    static const std::string& externalManifestValue(
        const ExternalManifest& values, const std::string& key,
        const std::filesystem::path& path) {
        const auto iterator = values.find(key);
        if (iterator == values.end()) {
            throw std::runtime_error(
                "future external manifest is missing " + key + ": " + path.string());
        }
        return iterator->second;
    }

    static uint64_t externalManifestU64(
        const ExternalManifest& values, const std::string& key,
        const std::filesystem::path& path) {
        const std::string& text = externalManifestValue(values, key, path);
        if (text.empty() ||
            !std::all_of(text.begin(), text.end(), [](unsigned char ch) {
                return ch >= '0' && ch <= '9';
            })) {
            throw std::runtime_error(
                "future external manifest has invalid " + key + ": " + path.string());
        }
        size_t consumed = 0;
        uint64_t value = 0;
        try {
            value = std::stoull(text, &consumed);
        } catch (const std::exception&) {
            throw std::runtime_error(
                "future external manifest has overflowing " + key + ": " + path.string());
        }
        if (consumed != text.size()) {
            throw std::runtime_error(
                "future external manifest has invalid " + key + ": " + path.string());
        }
        return value;
    }

    static Count externalManifestCount(
        const ExternalManifest& values, const std::string& lowKey,
        const std::string& highKey, const std::filesystem::path& path) {
        const uint64_t low = externalManifestU64(values, lowKey, path);
        const uint64_t high = externalManifestU64(values, highKey, path);
        return (Count)low | ((Count)high << 64);
    }

    static void writeExternalLayerManifest(
        const ExternalFileInfo& info, int c,
        const std::filesystem::path& manifestPath) {
        const std::filesystem::path temporaryPath = manifestPath.string() + ".tmp";
        std::ofstream manifest(temporaryPath, std::ios::trunc);
        if (!manifest) throw std::runtime_error("future external manifest create failed");
        manifest << "format=FJFTL01\n"
                 << "C=" << c << "\n"
                 << "processed_rows=" << info.processedRows << "\n"
                 << "records=" << info.records << "\n"
                 << "mass_low=" << (uint64_t)info.mass << "\n"
                 << "mass_high=" << (uint64_t)(info.mass >> 64) << "\n"
                 << "record_bytes=41\n"
                 << "file_bytes=" << (40u + 41u * (uintmax_t)info.records) << "\n";
        manifest.flush();
        manifest.close();
        if (!manifest) throw std::runtime_error("future external manifest close failed");
        std::filesystem::rename(temporaryPath, manifestPath);
    }

    std::optional<ExternalFileInfo> loadExternalLayer(
        const std::filesystem::path& rootDirectory,
        int processedRows) const {
        char layerName[48];
        std::snprintf(layerName, sizeof(layerName), "layer-%02d.bin", processedRows);
        const std::filesystem::path layerPath = rootDirectory / layerName;
        const std::filesystem::path manifestPath =
            rootDirectory / (std::string(layerName) + ".manifest");
        const bool haveLayer = std::filesystem::exists(layerPath);
        const bool haveManifest = std::filesystem::exists(manifestPath);
        if (!haveLayer && !haveManifest) return std::nullopt;
        if (haveLayer != haveManifest) {
            throw std::runtime_error(
                "future external layer commit is incomplete: " + layerPath.string());
        }

        ExternalReader reader(layerPath, ctx_.c, processedRows);
        ExternalFileInfo info = reader.info;
        const ExternalManifest values = readExternalManifest(manifestPath);
        if (externalManifestValue(values, "format", manifestPath) != "FJFTL01" ||
            externalManifestU64(values, "C", manifestPath) != (uint64_t)ctx_.c ||
            externalManifestU64(values, "processed_rows", manifestPath) !=
                (uint64_t)processedRows ||
            externalManifestU64(values, "records", manifestPath) != info.records ||
            externalManifestCount(values, "mass_low", "mass_high", manifestPath) !=
                info.mass ||
            externalManifestU64(values, "record_bytes", manifestPath) != 41 ||
            externalManifestU64(values, "file_bytes", manifestPath) !=
                40u + 41u * (uintmax_t)info.records) {
            throw std::runtime_error(
                "future external layer manifest audit failed: " + manifestPath.string());
        }
        return info;
    }

    void analyzeExternalSevenRowSignatures(
        const ExternalFileInfo& layer, const Options& options) const {
        if (options.tailSignatureDirectory.empty()) return;
        if (ctx_.m - layer.processedRows != 7) {
            throw std::runtime_error(
                "future tail signature analysis requires a seven-row layer");
        }
        if (options.tailSignatureSampleRecords == 0) {
            throw std::runtime_error(
                "future tail signature analysis requires a positive sample size");
        }
        constexpr uint64_t maximumSampleRecords = 10000000;
        if (options.tailSignatureSampleRecords > maximumSampleRecords) {
            throw std::runtime_error(
                "future tail signature sample exceeds the 10000000-record memory bound");
        }
        constexpr uint64_t maximumColorSampleKeys = 100000;
        if (options.tailColorCanonicalSampleKeys > maximumColorSampleKeys) {
            throw std::runtime_error(
                "future tail color-canonical sample exceeds the 100000-key time bound");
        }
        constexpr uint64_t maximumColorSignatureRecords = 100000;
        if (options.tailColorSignatureRecords >
            maximumColorSignatureRecords) {
            throw std::runtime_error(
                "future tail color signature sample exceeds the 100000-record time bound");
        }
        const uint64_t sampleRecords = std::min<uint64_t>(
            options.tailSignatureSampleRecords, layer.records);
        const uint64_t validationRecords = std::min<uint64_t>(
            options.tailSignatureValidationRecords, sampleRecords);
        const uint64_t benchmarkRecords = std::min<uint64_t>(
            options.tailBenchmarkRecords, sampleRecords);
        const uint64_t colorSignatureRecords = std::min<uint64_t>(
            options.tailColorSignatureRecords, sampleRecords);
        const uint64_t directEvaluationRecords = std::max(
            validationRecords, benchmarkRecords);
        const std::filesystem::path outputDirectory =
            options.tailSignatureDirectory;
        const std::string referenceIdentity =
            options.tailSignatureReferenceDirectory.empty()
                ? "none"
                : std::filesystem::absolute(
                    options.tailSignatureReferenceDirectory)
                    .lexically_normal().string();
        const std::filesystem::path committedManifest =
            outputDirectory / "COMMITTED.manifest";
        if (std::filesystem::exists(committedManifest)) {
            const ExternalManifest values = readExternalManifest(committedManifest);
            if (externalManifestValue(values, "format", committedManifest) !=
                    "FJFTSIG01" ||
                externalManifestU64(values, "C", committedManifest) !=
                    (uint64_t)ctx_.c ||
                externalManifestU64(values, "processed_rows", committedManifest) !=
                    (uint64_t)layer.processedRows ||
                externalManifestU64(values, "layer_records", committedManifest) !=
                    layer.records ||
                externalManifestCount(
                    values, "layer_mass_low", "layer_mass_high",
                    committedManifest) != layer.mass ||
                externalManifestU64(values, "sample_records", committedManifest) !=
                    sampleRecords ||
                externalManifestU64(
                    values, "validation_records", committedManifest) !=
                    validationRecords ||
                externalManifestU64(
                    values, "benchmark_records", committedManifest) !=
                    benchmarkRecords ||
                externalManifestU64(
                    values, "color_sample_keys_requested", committedManifest) !=
                    options.tailColorCanonicalSampleKeys ||
                externalManifestU64(
                    values, "color_signature_records", committedManifest) !=
                    colorSignatureRecords ||
                externalManifestU64(
                    values, "color_tail_enabled", committedManifest) !=
                    (uint64_t)options.useColorCanonicalTail ||
                externalManifestValue(
                    values, "reference_directory", committedManifest) !=
                    referenceIdentity) {
                throw std::runtime_error(
                    "future tail signature committed analysis identity mismatch: " +
                    committedManifest.string());
            }
            if (options.progress) {
                std::fprintf(stderr,
                    "future tail signature reuse samples=%llu report=%s\n",
                    (unsigned long long)sampleRecords,
                    committedManifest.string().c_str());
                std::fflush(stderr);
            }
            return;
        }
        if (std::filesystem::exists(outputDirectory)) {
            if (!std::filesystem::is_directory(outputDirectory) ||
                std::filesystem::directory_iterator(outputDirectory) !=
                    std::filesystem::directory_iterator()) {
                throw std::runtime_error(
                    "future tail signature output is not an empty resumable directory: " +
                    outputDirectory.string());
            }
        } else if (!std::filesystem::create_directories(outputDirectory)) {
            throw std::runtime_error(
                "future tail signature output directory create failed: " +
                outputDirectory.string());
        }

        struct SignatureFrequency {
            uint64_t records = 0;
        };
        std::unordered_map<SevenRowTailSignature, SignatureFrequency,
                           SevenRowTailSignatureHash> signatureFrequencies;
        std::unordered_map<SevenRowColorTailSignature, SignatureFrequency,
                           SevenRowColorTailSignatureHash>
            colorSignatureFrequencies;
        std::unordered_map<uint64_t, uint64_t> halfFrequencies;
        signatureFrequencies.reserve((size_t)sampleRecords);
        colorSignatureFrequencies.reserve((size_t)colorSignatureRecords);
        halfFrequencies.reserve((size_t)std::min<uint64_t>(
            2 * sampleRecords, maximumSampleRecords));
        std::array<uint64_t, 3> cutHistogram{};
        std::array<uint64_t, 8> relativeHistogram{};
        std::array<uint64_t, 2> keyKindHistogram{};
        Count sampleMass = 0;
        uint64_t selectedRecords = 0;
        uint64_t validatedRecords = 0;
        uint64_t directEvaluatedRecords = 0;
        uint64_t validationNanoseconds = 0;
        uint64_t validationLocalAssignments = 0;
        uint64_t validationKernelLookups = 0;
        uint64_t validationKernelHits = 0;
        uint64_t validationKernelEvictions = 0;
        uint64_t validationColorLookups = 0;
        uint64_t validationColorHits = 0;
        uint64_t validationColorOrbits = 0;
        uint64_t validationColorMappings = 0;
        uint64_t colorSignatureEvaluatedRecords = 0;
        uint64_t colorSignatureValidatedRecords = 0;
        uint64_t colorSignatureNanoseconds = 0;
        uint64_t colorSignatureLookups = 0;
        uint64_t colorSignatureHits = 0;
        uint64_t colorSignatureOrbits = 0;
        uint64_t colorSignatureMappings = 0;
        std::unique_ptr<Engine> validationReferenceEngine;
        if (validationRecords != 0) {
            validationReferenceEngine =
                std::make_unique<Engine>(ctx_.c);
        }
        std::unique_ptr<Engine> colorSignatureEngine;
        if (colorSignatureRecords != 0) {
            colorSignatureEngine = std::make_unique<Engine>(ctx_.c);
        }
        const auto started = std::chrono::steady_clock::now();
        ExternalReader reader(layer.path, ctx_.c, layer.processedRows);
        ExternalCodec codec(ctx_, ctx_.m - layer.processedRows);
        ExternalRecord record;
        uint64_t sourceIndex = 0;
        while (reader.next(record)) {
            const bool selected =
                ((Count)(sourceIndex + 1) * sampleRecords / layer.records) !=
                ((Count)sourceIndex * sampleRecords / layer.records);
            ++sourceIndex;
            if (!selected) {
                if (options.progress && sourceIndex % 25000000 == 0) {
                    std::fprintf(stderr,
                        "future tail signature scan source=%llu/%llu samples=%llu unique=%llu halfUnique=%llu\n",
                        (unsigned long long)sourceIndex,
                        (unsigned long long)layer.records,
                        (unsigned long long)selectedRecords,
                        (unsigned long long)signatureFrequencies.size(),
                        (unsigned long long)halfFrequencies.size());
                    std::fflush(stderr);
                }
                continue;
            }
            const State state = codec.decode(record.key);
            validateState(state, layer.processedRows);
            int selectedCut = -1;
            const SevenRowTailSignature signature =
                sevenRowTailSignature(state, &selectedCut);
            ++signatureFrequencies[signature].records;
            ++halfFrequencies[signature.firstKey];
            ++halfFrequencies[signature.secondKey];
            ++cutHistogram[(size_t)selectedCut];
            ++relativeHistogram[(size_t)signature.relativeTransform];
            if (record.key.keyKind > 1) {
                throw std::runtime_error(
                    "future tail signature encountered an invalid key kind");
            }
            ++keyKindHistogram[(size_t)record.key.keyKind];
            checkedAdd(sampleMass, record.weight,
                       "future tail signature sample mass overflow");
            ++selectedRecords;

            std::optional<uint64_t> directValue;
            if (directEvaluatedRecords < directEvaluationRecords) {
                const auto validationStarted =
                    std::chrono::steady_clock::now();
                const TailEvaluation direct = sevenRowTailValue(
                    state, options.tailKernelCacheRecords,
                    options.useColorCanonicalTail);
                validationNanoseconds += (uint64_t)
                    std::chrono::duration_cast<std::chrono::nanoseconds>(
                        std::chrono::steady_clock::now() -
                            validationStarted).count();
                validationLocalAssignments += direct.localAssignments;
                validationKernelLookups += direct.kernelLookups;
                validationKernelHits += direct.kernelHits;
                validationKernelEvictions += direct.kernelEvictions;
                validationColorLookups += direct.colorCanonicalLookups;
                validationColorHits += direct.colorCanonicalHits;
                validationColorOrbits += direct.colorCanonicalOrbits;
                validationColorMappings += direct.colorCanonicalMappings;
                directValue = direct.value;
                if (validatedRecords < validationRecords) {
                    const TailEvaluation reconstructed =
                        validationReferenceEngine->sevenRowTailValueFromSignature(
                            signature, std::min<size_t>(
                                options.tailKernelCacheRecords, 2000000));
                    if (direct.value != reconstructed.value) {
                        throw std::runtime_error(
                            "future tail signature value differential failed");
                    }
                    ++validatedRecords;
                }
                ++directEvaluatedRecords;
            }
            if (colorSignatureEvaluatedRecords < colorSignatureRecords) {
                const auto colorStarted =
                    std::chrono::steady_clock::now();
                TailEvaluation colorCounters;
                const SevenRowColorTailSignature colorSignature =
                    colorSignatureEngine->sevenRowColorTailSignature(
                        state, &colorCounters);
                colorSignatureNanoseconds += (uint64_t)
                    std::chrono::duration_cast<std::chrono::nanoseconds>(
                        std::chrono::steady_clock::now() -
                            colorStarted).count();
                colorSignatureLookups +=
                    colorCounters.colorCanonicalLookups;
                colorSignatureHits += colorCounters.colorCanonicalHits;
                colorSignatureOrbits += colorCounters.colorCanonicalOrbits;
                colorSignatureMappings +=
                    colorCounters.colorCanonicalMappings;
                ++colorSignatureFrequencies[colorSignature].records;
                if (colorSignatureValidatedRecords < validationRecords &&
                    colorSignatureValidatedRecords <
                        colorSignatureRecords) {
                    if (!directValue.has_value()) {
                        throw std::runtime_error(
                            "future tail color signature validation lacks a direct value");
                    }
                    const TailEvaluation reconstructed =
                        validationReferenceEngine->
                            sevenRowTailValueFromColorSignature(
                                colorSignature, std::min<size_t>(
                                    options.tailKernelCacheRecords,
                                    2000000));
                    if (*directValue != reconstructed.value) {
                        throw std::runtime_error(
                            "future tail color signature value differential failed");
                    }
                    ++colorSignatureValidatedRecords;
                }
                ++colorSignatureEvaluatedRecords;
            }
            if (options.progress && sourceIndex % 25000000 == 0) {
                std::fprintf(stderr,
                    "future tail signature scan source=%llu/%llu samples=%llu unique=%llu halfUnique=%llu\n",
                    (unsigned long long)sourceIndex,
                    (unsigned long long)layer.records,
                    (unsigned long long)selectedRecords,
                    (unsigned long long)signatureFrequencies.size(),
                    (unsigned long long)halfFrequencies.size());
                std::fflush(stderr);
            }
        }
        if (sourceIndex != layer.records || selectedRecords != sampleRecords ||
            validatedRecords != validationRecords ||
            directEvaluatedRecords != directEvaluationRecords ||
            colorSignatureEvaluatedRecords != colorSignatureRecords ||
            colorSignatureValidatedRecords !=
                std::min(validationRecords, colorSignatureRecords)) {
            throw std::runtime_error(
                "future tail signature sample cardinality audit failed");
        }

        uint64_t signatureSingletons = 0;
        uint64_t signatureRepeatedCoverage = 0;
        uint64_t signatureCollisionPairs = 0;
        uint64_t signatureMaxFrequency = 0;
        struct TopSignature {
            SevenRowTailSignature signature;
            uint64_t records = 0;
        };
        auto signatureBetter = [](const TopSignature& first,
                                  const TopSignature& second) {
            if (first.records != second.records) {
                return first.records > second.records;
            }
            return sevenRowTailSignatureLess(
                first.signature, second.signature);
        };
        std::vector<TopSignature> topSignatures;
        constexpr size_t topCount = 50;
        for (const auto& [signature, frequency] : signatureFrequencies) {
            const uint64_t records = frequency.records;
            signatureSingletons += records == 1;
            if (records > 1) signatureRepeatedCoverage += records;
            signatureCollisionPairs += records * (records - 1) / 2;
            signatureMaxFrequency = std::max(signatureMaxFrequency, records);
            TopSignature item{signature, records};
            if (topSignatures.size() < topCount) {
                topSignatures.push_back(item);
            } else {
                size_t worst = 0;
                for (size_t index = 1; index < topSignatures.size(); ++index) {
                    if (signatureBetter(topSignatures[worst],
                                        topSignatures[index])) {
                        worst = index;
                    }
                }
                if (signatureBetter(item, topSignatures[worst])) {
                    topSignatures[worst] = item;
                }
            }
        }
        std::sort(topSignatures.begin(), topSignatures.end(), signatureBetter);

        const uint64_t colorSignatureUnique =
            colorSignatureFrequencies.size();
        uint64_t colorSignatureSingletons = 0;
        uint64_t colorSignatureRepeatedCoverage = 0;
        uint64_t colorSignatureCollisionPairs = 0;
        uint64_t colorSignatureMaxFrequency = 0;
        for (const auto& [signature, frequency] :
             colorSignatureFrequencies) {
            (void)signature;
            const uint64_t records = frequency.records;
            colorSignatureSingletons += records == 1;
            if (records > 1) {
                colorSignatureRepeatedCoverage += records;
            }
            colorSignatureCollisionPairs +=
                records * (records - 1) / 2;
            colorSignatureMaxFrequency = std::max(
                colorSignatureMaxFrequency, records);
        }

        uint64_t halfSingletons = 0;
        uint64_t halfRepeatedCoverage = 0;
        uint64_t halfCollisionPairs = 0;
        uint64_t halfMaxFrequency = 0;
        std::array<uint64_t, 10> halfFrequencyHistogram{};
        struct TopHalf {
            uint64_t key = 0;
            uint64_t records = 0;
        };
        auto halfBetter = [](const TopHalf& first, const TopHalf& second) {
            if (first.records != second.records) {
                return first.records > second.records;
            }
            return first.key < second.key;
        };
        std::vector<TopHalf> topHalves;
        std::vector<TopHalf> sortedHalves;
        sortedHalves.reserve(halfFrequencies.size());
        for (const auto& [key, records] : halfFrequencies) {
            halfSingletons += records == 1;
            if (records > 1) halfRepeatedCoverage += records;
            halfCollisionPairs += records * (records - 1) / 2;
            halfMaxFrequency = std::max(halfMaxFrequency, records);
            const size_t frequencyBin = records <= 5 ? (size_t)(records - 1) :
                records <= 10 ? 5 : records <= 20 ? 6 :
                records <= 50 ? 7 : records <= 100 ? 8 : 9;
            ++halfFrequencyHistogram[frequencyBin];
            TopHalf item{key, records};
            sortedHalves.push_back(item);
            if (topHalves.size() < topCount) {
                topHalves.push_back(item);
            } else {
                size_t worst = 0;
                for (size_t index = 1; index < topHalves.size(); ++index) {
                    if (halfBetter(topHalves[worst], topHalves[index])) {
                        worst = index;
                    }
                }
                if (halfBetter(item, topHalves[worst])) topHalves[worst] = item;
            }
        }
        std::sort(topHalves.begin(), topHalves.end(), halfBetter);
        std::sort(sortedHalves.begin(), sortedHalves.end(),
            [](const TopHalf& first, const TopHalf& second) {
                return first.key < second.key;
            });

        struct ColorCanonicalFrequency {
            uint64_t rawKeys = 0;
            uint64_t occurrences = 0;
        };
        const uint64_t colorSampleKeys = std::min<uint64_t>(
            options.tailColorCanonicalSampleKeys, sortedHalves.size());
        std::unordered_map<uint64_t, ColorCanonicalFrequency>
            colorCanonicalFrequencies;
        colorCanonicalFrequencies.reserve((size_t)colorSampleKeys);
        uint64_t colorSampleOccurrences = 0;
        uint64_t selectedColorKeys = 0;
        for (uint64_t index = 0; index < sortedHalves.size(); ++index) {
            const bool selected = colorSampleKeys != 0 &&
                ((Count)(index + 1) * colorSampleKeys / sortedHalves.size()) !=
                ((Count)index * colorSampleKeys / sortedHalves.size());
            if (!selected) continue;
            const TopHalf& item = sortedHalves[(size_t)index];
            const SevenRowColorCanonicalHalf canonical =
                canonicalSevenRowHalfWithColorRefinement(item.key);
            ColorCanonicalFrequency& frequency =
                colorCanonicalFrequencies[canonical.key];
            ++frequency.rawKeys;
            frequency.occurrences += item.records;
            colorSampleOccurrences += item.records;
            if (selectedColorKeys < 20) {
                const SevenRowColorCanonicalHalf repeated =
                    canonicalSevenRowHalfWithColorRefinement(canonical.key);
                const SevenRowColorCanonicalHalf brute =
                    canonicalSevenRowHalfWithColors(item.key);
                const SevenRowColorCanonicalHalf bruteImage =
                    canonicalSevenRowHalfWithColors(canonical.key);
                if (repeated.key != canonical.key ||
                    bruteImage.key != brute.key) {
                    throw std::runtime_error(
                        "future tail color-canonical refinement differential failed");
                }
            }
            ++selectedColorKeys;
        }
        if (selectedColorKeys != colorSampleKeys) {
            throw std::runtime_error(
                "future tail color-canonical sample cardinality audit failed");
        }
        uint64_t colorSingletons = 0;
        uint64_t colorDoubletons = 0;
        uint64_t colorMaxRawKeys = 0;
        uint64_t colorCollisionPairs = 0;
        for (const auto& [key, frequency] : colorCanonicalFrequencies) {
            (void)key;
            colorSingletons += frequency.rawKeys == 1;
            colorDoubletons += frequency.rawKeys == 2;
            colorMaxRawKeys = std::max(
                colorMaxRawKeys, frequency.rawKeys);
            colorCollisionPairs +=
                frequency.rawKeys * (frequency.rawKeys - 1) / 2;
        }
        const uint64_t colorCanonicalUnique =
            colorCanonicalFrequencies.size();
        const uint64_t colorChaoAdditional = colorDoubletons == 0
            ? 0
            : (uint64_t)(((Count)colorSingletons * colorSingletons +
                           2 * colorDoubletons - 1) /
                          (2 * colorDoubletons));

        const std::filesystem::path topSignatureTemporary =
            outputDirectory / "top-signatures.tsv.tmp";
        const std::filesystem::path topSignatureFinal =
            outputDirectory / "top-signatures.tsv";
        {
            std::ofstream output(topSignatureTemporary, std::ios::trunc);
            if (!output) {
                throw std::runtime_error(
                    "future tail signature top-signature report create failed");
            }
            output << "rank\trecords\tfirst_key_hex\tsecond_key_hex\trelative_transform\n";
            char firstText[32];
            char secondText[32];
            for (size_t index = 0; index < topSignatures.size(); ++index) {
                const TopSignature& item = topSignatures[index];
                std::snprintf(firstText, sizeof(firstText), "%016llx",
                    (unsigned long long)item.signature.firstKey);
                std::snprintf(secondText, sizeof(secondText), "%016llx",
                    (unsigned long long)item.signature.secondKey);
                output << index + 1 << '\t' << item.records << '\t'
                       << firstText << '\t' << secondText << '\t'
                       << (unsigned)item.signature.relativeTransform << '\n';
            }
            output.flush();
            output.close();
            if (!output) {
                throw std::runtime_error(
                    "future tail signature top-signature report close failed");
            }
        }
        std::filesystem::rename(topSignatureTemporary, topSignatureFinal);

        const std::filesystem::path topHalfTemporary =
            outputDirectory / "top-halves.tsv.tmp";
        const std::filesystem::path topHalfFinal =
            outputDirectory / "top-halves.tsv";
        {
            std::ofstream output(topHalfTemporary, std::ios::trunc);
            if (!output) {
                throw std::runtime_error(
                    "future tail signature top-half report create failed");
            }
            output << "rank\trecords\thalf_key_hex\n";
            char keyText[32];
            for (size_t index = 0; index < topHalves.size(); ++index) {
                const TopHalf& item = topHalves[index];
                std::snprintf(keyText, sizeof(keyText), "%016llx",
                    (unsigned long long)item.key);
                output << index + 1 << '\t' << item.records << '\t'
                       << keyText << '\n';
            }
            output.flush();
            output.close();
            if (!output) {
                throw std::runtime_error(
                    "future tail signature top-half report close failed");
            }
        }
        std::filesystem::rename(topHalfTemporary, topHalfFinal);

        auto writeHalfFrequencyFile = [&](const std::filesystem::path& path) {
            std::ofstream output(path, std::ios::binary | std::ios::trunc);
            if (!output) {
                throw std::runtime_error(
                    "future tail half-frequency file create failed: " +
                    path.string());
            }
            std::array<char, 32> header{};
            const char magic[8] = {'F', 'J', 'F', 'T', 'H', 'F', '0', '1'};
            std::memcpy(header.data(), magic, sizeof(magic));
            header[8] = (char)ctx_.c;
            const uint64_t unique = sortedHalves.size();
            std::memcpy(header.data() + 16, &sampleRecords,
                        sizeof(sampleRecords));
            std::memcpy(header.data() + 24, &unique, sizeof(unique));
            output.write(header.data(), (std::streamsize)header.size());
            std::array<char, 16> bytes{};
            for (const TopHalf& item : sortedHalves) {
                std::memcpy(bytes.data(), &item.key, sizeof(item.key));
                std::memcpy(bytes.data() + 8, &item.records,
                            sizeof(item.records));
                output.write(bytes.data(), (std::streamsize)bytes.size());
            }
            output.flush();
            output.close();
            if (!output) {
                throw std::runtime_error(
                    "future tail half-frequency file close failed: " +
                    path.string());
            }
        };
        auto readHalfFrequencyFile = [&](const std::filesystem::path& path,
                                         uint64_t& sourceSamples) {
            std::ifstream input(path, std::ios::binary);
            if (!input) {
                throw std::runtime_error(
                    "future tail half-frequency reference open failed: " +
                    path.string());
            }
            std::array<char, 32> header{};
            input.read(header.data(), (std::streamsize)header.size());
            const char magic[8] = {'F', 'J', 'F', 'T', 'H', 'F', '0', '1'};
            uint64_t unique = 0;
            std::memcpy(&sourceSamples, header.data() + 16,
                        sizeof(sourceSamples));
            std::memcpy(&unique, header.data() + 24, sizeof(unique));
            if (!input || std::memcmp(header.data(), magic, sizeof(magic)) != 0 ||
                (uint8_t)header[8] != ctx_.c ||
                std::filesystem::file_size(path) !=
                    32u + 16u * (uintmax_t)unique) {
                throw std::runtime_error(
                    "future tail half-frequency reference header audit failed: " +
                    path.string());
            }
            std::vector<TopHalf> entries;
            entries.reserve((size_t)unique);
            std::array<char, 16> bytes{};
            uint64_t previousKey = 0;
            bool havePrevious = false;
            for (uint64_t index = 0; index < unique; ++index) {
                input.read(bytes.data(), (std::streamsize)bytes.size());
                TopHalf item;
                std::memcpy(&item.key, bytes.data(), sizeof(item.key));
                std::memcpy(&item.records, bytes.data() + 8,
                            sizeof(item.records));
                if (!input || item.records == 0 ||
                    (havePrevious && item.key <= previousKey)) {
                    throw std::runtime_error(
                        "future tail half-frequency reference record audit failed: " +
                        path.string());
                }
                previousKey = item.key;
                havePrevious = true;
                entries.push_back(item);
            }
            char trailing = 0;
            if (input.read(&trailing, 1)) {
                throw std::runtime_error(
                    "future tail half-frequency reference has trailing data: " +
                    path.string());
            }
            return entries;
        };

        const std::filesystem::path halfFrequencyTemporary =
            outputDirectory / "half-frequencies.bin.tmp";
        const std::filesystem::path halfFrequencyFinal =
            outputDirectory / "half-frequencies.bin";
        writeHalfFrequencyFile(halfFrequencyTemporary);
        std::filesystem::rename(halfFrequencyTemporary, halfFrequencyFinal);

        uint64_t referenceSamples = 0;
        uint64_t referenceHalfOccurrences = 0;
        uint64_t referenceHalfUnique = 0;
        uint64_t referenceIntersectionUnique = 0;
        uint64_t referenceCurrentCoveredOccurrences = 0;
        uint64_t referenceCoveredOccurrences = 0;
        uint64_t referenceMinimumCoveredOccurrences = 0;
        if (!options.tailSignatureReferenceDirectory.empty()) {
            const std::filesystem::path referencePath =
                std::filesystem::path(
                    options.tailSignatureReferenceDirectory) /
                "half-frequencies.bin";
            const std::vector<TopHalf> reference =
                readHalfFrequencyFile(referencePath, referenceSamples);
            referenceHalfUnique = reference.size();
            for (const TopHalf& item : reference) {
                referenceHalfOccurrences += item.records;
            }
            size_t currentIndex = 0;
            size_t referenceIndex = 0;
            while (currentIndex < sortedHalves.size() &&
                   referenceIndex < reference.size()) {
                const TopHalf& current = sortedHalves[currentIndex];
                const TopHalf& prior = reference[referenceIndex];
                if (current.key < prior.key) {
                    ++currentIndex;
                } else if (prior.key < current.key) {
                    ++referenceIndex;
                } else {
                    ++referenceIntersectionUnique;
                    referenceCurrentCoveredOccurrences += current.records;
                    referenceCoveredOccurrences += prior.records;
                    referenceMinimumCoveredOccurrences +=
                        std::min(current.records, prior.records);
                    ++currentIndex;
                    ++referenceIndex;
                }
            }
        }

        const uint64_t signatureUnique = signatureFrequencies.size();
        const uint64_t halfOccurrences = 2 * sampleRecords;
        const uint64_t halfUnique = halfFrequencies.size();
        const uint64_t halfChaoAdditional = halfFrequencyHistogram[1] == 0
            ? 0
            : (uint64_t)(((Count)halfFrequencyHistogram[0] *
                           halfFrequencyHistogram[0] +
                           2 * halfFrequencyHistogram[1] - 1) /
                          (2 * halfFrequencyHistogram[1]));
        const auto elapsedMilliseconds = (uint64_t)
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - started).count();
        const std::filesystem::path manifestTemporary =
            committedManifest.string() + ".tmp";
        {
            std::ofstream output(manifestTemporary, std::ios::trunc);
            if (!output) {
                throw std::runtime_error(
                    "future tail signature manifest create failed");
            }
            output << "format=FJFTSIG01\n"
                   << "C=" << ctx_.c << "\n"
                   << "processed_rows=" << layer.processedRows << "\n"
                   << "layer_records=" << layer.records << "\n"
                   << "layer_mass_low=" << (uint64_t)layer.mass << "\n"
                   << "layer_mass_high=" << (uint64_t)(layer.mass >> 64) << "\n"
                   << "source_file_bytes=" << std::filesystem::file_size(layer.path) << "\n"
                   << "reference_directory=" << referenceIdentity << "\n"
                   << "sample_records=" << sampleRecords << "\n"
                   << "sample_mass_low=" << (uint64_t)sampleMass << "\n"
                   << "sample_mass_high=" << (uint64_t)(sampleMass >> 64) << "\n"
                   << "validation_records=" << validationRecords << "\n"
                   << "benchmark_records=" << benchmarkRecords << "\n"
                   << "direct_evaluation_records="
                   << directEvaluationRecords << "\n"
                   << "color_tail_enabled="
                   << (options.useColorCanonicalTail ? 1 : 0) << "\n"
                   << "validation_nanoseconds="
                   << validationNanoseconds << "\n"
                   << "validation_local_assignments="
                   << validationLocalAssignments << "\n"
                   << "validation_kernel_lookups="
                   << validationKernelLookups << "\n"
                   << "validation_kernel_hits="
                   << validationKernelHits << "\n"
                   << "validation_kernel_evictions="
                   << validationKernelEvictions << "\n"
                   << "validation_color_lookups="
                   << validationColorLookups << "\n"
                   << "validation_color_hits="
                   << validationColorHits << "\n"
                   << "validation_color_orbits="
                   << validationColorOrbits << "\n"
                   << "validation_color_mappings="
                   << validationColorMappings << "\n"
                   << "color_signature_records="
                   << colorSignatureRecords << "\n"
                   << "color_signature_validated_records="
                   << colorSignatureValidatedRecords << "\n"
                   << "color_signature_nanoseconds="
                   << colorSignatureNanoseconds << "\n"
                   << "color_signature_lookups="
                   << colorSignatureLookups << "\n"
                   << "color_signature_hits="
                   << colorSignatureHits << "\n"
                   << "color_signature_orbits="
                   << colorSignatureOrbits << "\n"
                   << "color_signature_mappings="
                   << colorSignatureMappings << "\n"
                   << "color_signature_unique="
                   << colorSignatureUnique << "\n"
                   << "color_signature_duplicate_records="
                   << colorSignatureRecords - colorSignatureUnique << "\n"
                   << "color_signature_duplicate_ppm="
                   << (colorSignatureRecords == 0 ? 0 :
                       (colorSignatureRecords - colorSignatureUnique) *
                           1000000 / colorSignatureRecords)
                   << "\n"
                   << "color_signature_singletons="
                   << colorSignatureSingletons << "\n"
                   << "color_signature_repeated_coverage="
                   << colorSignatureRepeatedCoverage << "\n"
                   << "color_signature_collision_pairs="
                   << colorSignatureCollisionPairs << "\n"
                   << "color_signature_max_frequency="
                   << colorSignatureMaxFrequency << "\n"
                   << "color_sample_keys_requested="
                   << options.tailColorCanonicalSampleKeys << "\n"
                   << "color_sample_keys=" << colorSampleKeys << "\n"
                   << "color_sample_occurrences="
                   << colorSampleOccurrences << "\n"
                   << "color_canonical_unique="
                   << colorCanonicalUnique << "\n"
                   << "color_canonical_duplicate_keys="
                   << colorSampleKeys - colorCanonicalUnique << "\n"
                   << "color_canonical_duplicate_ppm="
                   << (colorSampleKeys == 0 ? 0 :
                       (colorSampleKeys - colorCanonicalUnique) *
                           1000000 / colorSampleKeys)
                   << "\n"
                   << "color_canonical_singletons="
                   << colorSingletons << "\n"
                   << "color_canonical_doubletons="
                   << colorDoubletons << "\n"
                   << "color_canonical_collision_pairs="
                   << colorCollisionPairs << "\n"
                   << "color_canonical_max_raw_keys="
                   << colorMaxRawKeys << "\n"
                   << "color_canonical_chao1_estimated_unique="
                   << colorCanonicalUnique + colorChaoAdditional << "\n"
                   << "signature_unique=" << signatureUnique << "\n"
                   << "signature_duplicate_records="
                   << sampleRecords - signatureUnique << "\n"
                   << "signature_duplicate_ppm="
                   << (sampleRecords == 0 ? 0 :
                       (sampleRecords - signatureUnique) * 1000000 / sampleRecords)
                   << "\n"
                   << "signature_singletons=" << signatureSingletons << "\n"
                   << "signature_repeated_coverage="
                   << signatureRepeatedCoverage << "\n"
                   << "signature_collision_pairs="
                   << signatureCollisionPairs << "\n"
                   << "signature_max_frequency="
                   << signatureMaxFrequency << "\n"
                   << "half_occurrences=" << halfOccurrences << "\n"
                   << "half_unique=" << halfUnique << "\n"
                   << "half_duplicate_occurrences="
                   << halfOccurrences - halfUnique << "\n"
                   << "half_duplicate_ppm="
                   << (halfOccurrences == 0 ? 0 :
                       (halfOccurrences - halfUnique) * 1000000 / halfOccurrences)
                   << "\n"
                   << "half_singletons=" << halfSingletons << "\n"
                   << "half_repeated_coverage=" << halfRepeatedCoverage << "\n"
                   << "half_collision_pairs=" << halfCollisionPairs << "\n"
                   << "half_max_frequency=" << halfMaxFrequency << "\n"
                   << "half_frequency_keys_1=" << halfFrequencyHistogram[0] << "\n"
                   << "half_frequency_keys_2=" << halfFrequencyHistogram[1] << "\n"
                   << "half_frequency_keys_3=" << halfFrequencyHistogram[2] << "\n"
                   << "half_frequency_keys_4=" << halfFrequencyHistogram[3] << "\n"
                   << "half_frequency_keys_5=" << halfFrequencyHistogram[4] << "\n"
                   << "half_frequency_keys_6_10=" << halfFrequencyHistogram[5] << "\n"
                   << "half_frequency_keys_11_20=" << halfFrequencyHistogram[6] << "\n"
                   << "half_frequency_keys_21_50=" << halfFrequencyHistogram[7] << "\n"
                   << "half_frequency_keys_51_100=" << halfFrequencyHistogram[8] << "\n"
                   << "half_frequency_keys_101_plus=" << halfFrequencyHistogram[9] << "\n"
                   << "half_good_turing_unseen_ppm="
                   << (halfOccurrences == 0 ? 0 :
                       halfFrequencyHistogram[0] * 1000000 / halfOccurrences)
                   << "\n"
                   << "half_chao1_estimated_unique="
                   << halfUnique + halfChaoAdditional << "\n"
                   << "half_frequency_format=FJFTHF01\n"
                   << "half_frequency_record_bytes=16\n"
                   << "half_frequency_file_bytes="
                   << std::filesystem::file_size(halfFrequencyFinal) << "\n"
                   << "reference_samples=" << referenceSamples << "\n"
                   << "reference_half_occurrences="
                   << referenceHalfOccurrences << "\n"
                   << "reference_half_unique=" << referenceHalfUnique << "\n"
                   << "reference_intersection_unique="
                   << referenceIntersectionUnique << "\n"
                   << "reference_intersection_current_unique_ppm="
                   << (halfUnique == 0 ? 0 :
                       referenceIntersectionUnique * 1000000 / halfUnique)
                   << "\n"
                   << "reference_current_covered_occurrences="
                   << referenceCurrentCoveredOccurrences << "\n"
                   << "reference_current_covered_ppm="
                   << (halfOccurrences == 0 ? 0 :
                       referenceCurrentCoveredOccurrences * 1000000 /
                           halfOccurrences)
                   << "\n"
                   << "reference_covered_occurrences="
                   << referenceCoveredOccurrences << "\n"
                   << "reference_covered_ppm="
                   << (referenceHalfOccurrences == 0 ? 0 :
                       referenceCoveredOccurrences * 1000000 /
                           referenceHalfOccurrences)
                   << "\n"
                   << "reference_minimum_covered_occurrences="
                   << referenceMinimumCoveredOccurrences << "\n"
                   << "key_kind_strong=" << keyKindHistogram[0] << "\n"
                   << "key_kind_fallback=" << keyKindHistogram[1] << "\n";
            for (size_t cut = 0; cut < cutHistogram.size(); ++cut) {
                output << "selected_cut_" << cut << '=' << cutHistogram[cut] << "\n";
            }
            for (size_t transform = 0; transform < relativeHistogram.size();
                 ++transform) {
                output << "relative_transform_" << transform << '='
                       << relativeHistogram[transform] << "\n";
            }
            output << "elapsed_milliseconds=" << elapsedMilliseconds << "\n";
            output.flush();
            output.close();
            if (!output) {
                throw std::runtime_error(
                    "future tail signature manifest close failed");
            }
        }
        std::filesystem::rename(manifestTemporary, committedManifest);
        std::fprintf(stderr,
            "future tail signature complete samples=%llu unique=%llu duplicatePpm=%llu halfUnique=%llu halfDuplicatePpm=%llu colorSignatures=%llu colorSignatureUnique=%llu colorSignatureDuplicatePpm=%llu colorSamples=%llu colorUnique=%llu referenceIntersection=%llu referenceCurrentCoveragePpm=%llu validated=%llu elapsed=%.3fs report=%s\n",
            (unsigned long long)sampleRecords,
            (unsigned long long)signatureUnique,
            (unsigned long long)((sampleRecords - signatureUnique) *
                                 1000000 / sampleRecords),
            (unsigned long long)halfUnique,
            (unsigned long long)((halfOccurrences - halfUnique) *
                                 1000000 / halfOccurrences),
            (unsigned long long)colorSignatureRecords,
            (unsigned long long)colorSignatureUnique,
            (unsigned long long)(colorSignatureRecords == 0 ? 0 :
                (colorSignatureRecords - colorSignatureUnique) *
                    1000000 / colorSignatureRecords),
            (unsigned long long)colorSampleKeys,
            (unsigned long long)colorCanonicalUnique,
            (unsigned long long)referenceIntersectionUnique,
            (unsigned long long)(halfOccurrences == 0 ? 0 :
                referenceCurrentCoveredOccurrences * 1000000 /
                    halfOccurrences),
            (unsigned long long)validationRecords,
            elapsedMilliseconds / 1000.0,
            committedManifest.string().c_str());
        std::fflush(stderr);
    }

    void analyzeExternalSevenRowKernelInventory(
        const ExternalFileInfo& layer, const Options& options) const {
        if (options.tailKernelInventoryDirectory.empty()) return;
        if (ctx_.m - layer.processedRows != 7) {
            throw std::runtime_error(
                "future tail kernel inventory requires a seven-row layer");
        }
        if (options.tailKernelInventoryRecords == 0) {
            throw std::runtime_error(
                "future tail kernel inventory requires a positive record count");
        }
        const uint64_t sampleRecords = std::min<uint64_t>(
            options.tailKernelInventoryRecords, layer.records);
        if (sampleRecords > std::numeric_limits<uint64_t>::max() / 6) {
            throw std::runtime_error(
                "future tail kernel inventory occurrence count overflow");
        }
        const uint64_t expectedOccurrences = 6 * sampleRecords;
        const std::filesystem::path outputDirectory =
            options.tailKernelInventoryDirectory;
        const std::filesystem::path committedManifest =
            outputDirectory / "COMMITTED.manifest";
        const std::filesystem::path inventoryFile =
            outputDirectory / "half-kernel-inventory.bin";
        const std::string referenceIdentity =
            options.tailKernelInventoryReferenceDirectory.empty()
                ? "none"
                : std::filesystem::absolute(
                    options.tailKernelInventoryReferenceDirectory)
                    .lexically_normal().string();

        if (std::filesystem::exists(committedManifest)) {
            const ExternalManifest values =
                readExternalManifest(committedManifest);
            if (externalManifestValue(values, "format", committedManifest) !=
                    "FJFTKI01" ||
                externalManifestU64(values, "C", committedManifest) !=
                    (uint64_t)ctx_.c ||
                externalManifestU64(
                    values, "processed_rows", committedManifest) !=
                    (uint64_t)layer.processedRows ||
                externalManifestU64(
                    values, "layer_records", committedManifest) !=
                    layer.records ||
                externalManifestCount(
                    values, "layer_mass_low", "layer_mass_high",
                    committedManifest) != layer.mass ||
                externalManifestU64(
                    values, "sample_records", committedManifest) !=
                    sampleRecords ||
                externalManifestValue(
                    values, "reference_directory", committedManifest) !=
                    referenceIdentity ||
                !std::filesystem::exists(inventoryFile) ||
                externalManifestU64(
                    values, "inventory_file_bytes", committedManifest) !=
                    std::filesystem::file_size(inventoryFile)) {
                throw std::runtime_error(
                    "future tail kernel inventory committed identity mismatch: " +
                    committedManifest.string());
            }
            if (options.progress) {
                std::fprintf(stderr,
                    "future tail kernel inventory reuse records=%llu report=%s\n",
                    (unsigned long long)sampleRecords,
                    committedManifest.string().c_str());
                std::fflush(stderr);
            }
            return;
        }
        if (std::filesystem::exists(outputDirectory)) {
            if (!std::filesystem::is_directory(outputDirectory) ||
                std::filesystem::directory_iterator(outputDirectory) !=
                    std::filesystem::directory_iterator()) {
                throw std::runtime_error(
                    "future tail kernel inventory output is not empty: " +
                    outputDirectory.string());
            }
        } else if (!std::filesystem::create_directories(outputDirectory)) {
            throw std::runtime_error(
                "future tail kernel inventory output create failed: " +
                outputDirectory.string());
        }

        const int inventoryThreads = std::max(1, options.tailThreads);
#ifndef _OPENMP
        if (inventoryThreads != 1) {
            throw std::runtime_error(
                "future tail kernel inventory threads require OpenMP");
        }
#endif
        std::vector<std::unordered_map<uint64_t, uint64_t>>
            threadFrequencies((size_t)inventoryThreads);
        for (auto& frequencies : threadFrequencies) {
            frequencies.reserve(100000);
        }
        std::vector<State> states;
        states.reserve(std::max<size_t>(1, options.tailChunkRecords));
        ExternalReader reader(layer.path, ctx_.c, layer.processedRows);
        ExternalCodec codec(ctx_, ctx_.m - layer.processedRows);
        ExternalRecord record;
        uint64_t sourceIndex = 0;
        uint64_t selectedRecords = 0;
        uint64_t nextProgress = 25000000;
        const auto started = std::chrono::steady_clock::now();
        while (true) {
            states.clear();
            while (states.size() <
                       std::max<size_t>(1, options.tailChunkRecords) &&
                   reader.next(record)) {
                const bool selected =
                    ((Count)(sourceIndex + 1) * sampleRecords /
                         layer.records) !=
                    ((Count)sourceIndex * sampleRecords / layer.records);
                ++sourceIndex;
                if (!selected) continue;
                states.push_back(codec.decode(record.key));
                ++selectedRecords;
            }
            if (states.empty()) break;
            std::exception_ptr parallelError;
#ifdef _OPENMP
#pragma omp parallel for num_threads(inventoryThreads) schedule(static)
#endif
            for (int64_t index = 0;
                 index < (int64_t)states.size(); ++index) {
                try {
#ifdef _OPENMP
                    const int thread = omp_get_thread_num();
#else
                    const int thread = 0;
#endif
                    const auto keys = sevenRowColorCanonicalKernelKeys(
                        states[(size_t)index]);
                    auto& frequencies = threadFrequencies[(size_t)thread];
                    for (uint64_t key : keys) {
                        uint64_t& frequency = frequencies[key];
                        if (frequency ==
                            std::numeric_limits<uint64_t>::max()) {
                            throw std::runtime_error(
                                "future tail kernel inventory frequency overflow");
                        }
                        ++frequency;
                    }
                } catch (...) {
#ifdef _OPENMP
#pragma omp critical(future_tail_inventory_exception)
#endif
                    {
                        if (!parallelError) {
                            parallelError = std::current_exception();
                        }
                    }
                }
            }
            if (parallelError) std::rethrow_exception(parallelError);
            if (options.progress && sourceIndex >= nextProgress) {
                uint64_t threadUnique = 0;
                for (const auto& frequencies : threadFrequencies) {
                    threadUnique += frequencies.size();
                }
                std::fprintf(stderr,
                    "future tail kernel inventory source=%llu/%llu selected=%llu threadUnique=%llu\n",
                    (unsigned long long)sourceIndex,
                    (unsigned long long)layer.records,
                    (unsigned long long)selectedRecords,
                    (unsigned long long)threadUnique);
                std::fflush(stderr);
                while (nextProgress <= sourceIndex &&
                       nextProgress <=
                           std::numeric_limits<uint64_t>::max() - 25000000) {
                    nextProgress += 25000000;
                }
            }
        }
        if (sourceIndex != layer.records ||
            selectedRecords != sampleRecords) {
            throw std::runtime_error(
                "future tail kernel inventory sample cardinality failed");
        }

        size_t threadEntryTotal = 0;
        for (const auto& frequencies : threadFrequencies) {
            threadEntryTotal += frequencies.size();
        }
        std::unordered_map<uint64_t, uint64_t> merged;
        merged.reserve(threadEntryTotal);
        for (const auto& frequencies : threadFrequencies) {
            for (const auto& [key, frequency] : frequencies) {
                uint64_t& total = merged[key];
                if (total >
                    std::numeric_limits<uint64_t>::max() - frequency) {
                    throw std::runtime_error(
                        "future tail kernel inventory merge overflow");
                }
                total += frequency;
            }
        }

        struct InventoryRecord {
            uint64_t key = 0;
            uint64_t occurrences = 0;
            uint64_t support = 0;
            uint64_t localAssignments = 0;

            bool operator==(const InventoryRecord&) const = default;
        };
        std::vector<InventoryRecord> inventory;
        inventory.reserve(merged.size());
        uint64_t occurrenceAudit = 0;
        for (const auto& [key, occurrences] : merged) {
            if (occurrenceAudit >
                std::numeric_limits<uint64_t>::max() - occurrences) {
                throw std::runtime_error(
                    "future tail kernel inventory occurrence audit overflow");
            }
            occurrenceAudit += occurrences;
            inventory.push_back(InventoryRecord{key, occurrences, 0, 0});
        }
        if (occurrenceAudit != expectedOccurrences) {
            throw std::runtime_error(
                "future tail kernel inventory occurrence audit failed");
        }
        std::sort(inventory.begin(), inventory.end(),
            [](const InventoryRecord& first, const InventoryRecord& second) {
                return first.key < second.key;
            });

        std::exception_ptr kernelError;
#ifdef _OPENMP
#pragma omp parallel for num_threads(inventoryThreads) schedule(static)
#endif
        for (int64_t index = 0;
             index < (int64_t)inventory.size(); ++index) {
            try {
                const SevenRowCanonicalHalf canonical =
                    sevenRowCanonicalHalfFromKey(
                        inventory[(size_t)index].key);
                uint64_t localAssignments = 0;
                const TailKernel kernel = makeSevenRowTailKernel(
                    canonical.columns, localAssignments);
                inventory[(size_t)index].support = kernel.size();
                inventory[(size_t)index].localAssignments =
                    localAssignments;
            } catch (...) {
#ifdef _OPENMP
#pragma omp critical(future_tail_inventory_kernel_exception)
#endif
                {
                    if (!kernelError) kernelError = std::current_exception();
                }
            }
        }
        if (kernelError) std::rethrow_exception(kernelError);

        auto writeInventory = [&](const std::filesystem::path& path) {
            std::ofstream output(path, std::ios::binary | std::ios::trunc);
            if (!output) {
                throw std::runtime_error(
                    "future tail kernel inventory file create failed: " +
                    path.string());
            }
            std::array<char, 32> header{};
            const char magic[8] =
                {'F', 'J', 'F', 'T', 'K', 'I', '0', '1'};
            std::memcpy(header.data(), magic, sizeof(magic));
            header[8] = (char)ctx_.c;
            header[9] = (char)layer.processedRows;
            const uint64_t unique = inventory.size();
            std::memcpy(header.data() + 16, &sampleRecords,
                        sizeof(sampleRecords));
            std::memcpy(header.data() + 24, &unique, sizeof(unique));
            output.write(header.data(), (std::streamsize)header.size());
            std::array<char, 32> bytes{};
            for (const InventoryRecord& item : inventory) {
                std::memcpy(bytes.data(), &item.key, sizeof(item.key));
                std::memcpy(bytes.data() + 8, &item.occurrences,
                            sizeof(item.occurrences));
                std::memcpy(bytes.data() + 16, &item.support,
                            sizeof(item.support));
                std::memcpy(bytes.data() + 24, &item.localAssignments,
                            sizeof(item.localAssignments));
                output.write(bytes.data(), (std::streamsize)bytes.size());
            }
            output.flush();
            output.close();
            if (!output) {
                throw std::runtime_error(
                    "future tail kernel inventory file close failed: " +
                    path.string());
            }
        };
        auto readInventory = [&](const std::filesystem::path& path,
                                 uint64_t& sourceSamples) {
            std::ifstream input(path, std::ios::binary);
            if (!input) {
                throw std::runtime_error(
                    "future tail kernel inventory file open failed: " +
                    path.string());
            }
            std::array<char, 32> header{};
            input.read(header.data(), (std::streamsize)header.size());
            const char magic[8] =
                {'F', 'J', 'F', 'T', 'K', 'I', '0', '1'};
            uint64_t unique = 0;
            std::memcpy(&sourceSamples, header.data() + 16,
                        sizeof(sourceSamples));
            std::memcpy(&unique, header.data() + 24, sizeof(unique));
            if (!input ||
                std::memcmp(header.data(), magic, sizeof(magic)) != 0 ||
                (uint8_t)header[8] != ctx_.c ||
                (uint8_t)header[9] != layer.processedRows ||
                std::filesystem::file_size(path) !=
                    32u + 32u * (uintmax_t)unique) {
                throw std::runtime_error(
                    "future tail kernel inventory header audit failed: " +
                    path.string());
            }
            std::vector<InventoryRecord> result;
            result.reserve((size_t)unique);
            std::array<char, 32> bytes{};
            uint64_t previousKey = 0;
            bool havePrevious = false;
            for (uint64_t index = 0; index < unique; ++index) {
                input.read(bytes.data(), (std::streamsize)bytes.size());
                InventoryRecord item;
                std::memcpy(&item.key, bytes.data(), sizeof(item.key));
                std::memcpy(&item.occurrences, bytes.data() + 8,
                            sizeof(item.occurrences));
                std::memcpy(&item.support, bytes.data() + 16,
                            sizeof(item.support));
                std::memcpy(&item.localAssignments, bytes.data() + 24,
                            sizeof(item.localAssignments));
                if (!input || item.occurrences == 0 ||
                    (havePrevious && item.key <= previousKey)) {
                    throw std::runtime_error(
                        "future tail kernel inventory record audit failed: " +
                        path.string());
                }
                previousKey = item.key;
                havePrevious = true;
                result.push_back(item);
            }
            char trailing = 0;
            if (input.read(&trailing, 1)) {
                throw std::runtime_error(
                    "future tail kernel inventory has trailing data: " +
                    path.string());
            }
            return result;
        };

        const std::filesystem::path inventoryTemporary =
            inventoryFile.string() + ".tmp";
        writeInventory(inventoryTemporary);
        uint64_t roundTripSamples = 0;
        const std::vector<InventoryRecord> roundTrip =
            readInventory(inventoryTemporary, roundTripSamples);
        if (roundTripSamples != sampleRecords || roundTrip != inventory) {
            throw std::runtime_error(
                "future tail kernel inventory round-trip failed");
        }
        std::filesystem::rename(inventoryTemporary, inventoryFile);

        uint64_t singletonKeys = 0;
        uint64_t maximumFrequency = 0;
        uint64_t totalKernelRecords = 0;
        uint64_t maximumSupport = 0;
        uint64_t zeroSupportKeys = 0;
        uint64_t totalLocalAssignments = 0;
        Count collisionPairs = 0;
        std::array<uint64_t, 9> frequencyHistogram{};
        std::array<uint64_t, 8> supportHistogram{};
        for (const InventoryRecord& item : inventory) {
            singletonKeys += item.occurrences == 1;
            maximumFrequency = std::max(
                maximumFrequency, item.occurrences);
            collisionPairs += (Count)item.occurrences *
                (item.occurrences - 1) / 2;
            const size_t frequencyBin = item.occurrences == 1 ? 0 :
                item.occurrences == 2 ? 1 :
                item.occurrences <= 10 ? 2 :
                item.occurrences <= 100 ? 3 :
                item.occurrences <= 1000 ? 4 :
                item.occurrences <= 10000 ? 5 :
                item.occurrences <= 100000 ? 6 :
                item.occurrences <= 1000000 ? 7 : 8;
            ++frequencyHistogram[frequencyBin];
            if (totalKernelRecords >
                std::numeric_limits<uint64_t>::max() - item.support ||
                totalLocalAssignments >
                std::numeric_limits<uint64_t>::max() -
                    item.localAssignments) {
                throw std::runtime_error(
                    "future tail kernel inventory kernel metric overflow");
            }
            totalKernelRecords += item.support;
            totalLocalAssignments += item.localAssignments;
            maximumSupport = std::max(maximumSupport, item.support);
            zeroSupportKeys += item.support == 0;
            const size_t supportBin = item.support <= 100 ? 0 :
                item.support <= 500 ? 1 :
                item.support <= 1000 ? 2 :
                item.support <= 2000 ? 3 :
                item.support <= 4000 ? 4 :
                item.support <= 8000 ? 5 :
                item.support <= 16000 ? 6 : 7;
            ++supportHistogram[supportBin];
        }

        std::vector<size_t> topIndices(inventory.size());
        std::iota(topIndices.begin(), topIndices.end(), 0);
        const size_t topCount = std::min<size_t>(50, topIndices.size());
        std::partial_sort(topIndices.begin(),
                          topIndices.begin() + topCount,
                          topIndices.end(),
            [&](size_t first, size_t second) {
                if (inventory[first].occurrences !=
                    inventory[second].occurrences) {
                    return inventory[first].occurrences >
                        inventory[second].occurrences;
                }
                return inventory[first].key < inventory[second].key;
            });
        topIndices.resize(topCount);
        const std::filesystem::path topTemporary =
            outputDirectory / "top-kernels.tsv.tmp";
        const std::filesystem::path topFinal =
            outputDirectory / "top-kernels.tsv";
        {
            std::ofstream output(topTemporary, std::ios::trunc);
            if (!output) {
                throw std::runtime_error(
                    "future tail kernel inventory top report create failed");
            }
            output << "rank\toccurrences\tkey_hex\tsupport\tlocal_assignments\n";
            char keyText[32];
            for (size_t rank = 0; rank < topIndices.size(); ++rank) {
                const InventoryRecord& item =
                    inventory[topIndices[rank]];
                std::snprintf(keyText, sizeof(keyText), "%016llx",
                    (unsigned long long)item.key);
                output << rank + 1 << '\t' << item.occurrences << '\t'
                       << keyText << '\t' << item.support << '\t'
                       << item.localAssignments << '\n';
            }
            output.flush();
            output.close();
            if (!output) {
                throw std::runtime_error(
                    "future tail kernel inventory top report close failed");
            }
        }
        std::filesystem::rename(topTemporary, topFinal);

        uint64_t referenceSamples = 0;
        uint64_t referenceOccurrences = 0;
        uint64_t referenceUnique = 0;
        uint64_t intersectionUnique = 0;
        uint64_t currentCoveredOccurrences = 0;
        uint64_t referenceCoveredOccurrences = 0;
        uint64_t minimumCoveredOccurrences = 0;
        uint64_t intersectionKernelRecords = 0;
        uint64_t referenceKernelRecords = 0;
        if (!options.tailKernelInventoryReferenceDirectory.empty()) {
            const std::filesystem::path referenceDirectory =
                options.tailKernelInventoryReferenceDirectory;
            if (!std::filesystem::exists(
                    referenceDirectory / "COMMITTED.manifest")) {
                throw std::runtime_error(
                    "future tail kernel inventory reference is not committed: " +
                    referenceDirectory.string());
            }
            const std::vector<InventoryRecord> reference = readInventory(
                referenceDirectory / "half-kernel-inventory.bin",
                referenceSamples);
            referenceUnique = reference.size();
            for (const InventoryRecord& item : reference) {
                referenceOccurrences += item.occurrences;
                referenceKernelRecords += item.support;
            }
            size_t currentIndex = 0;
            size_t referenceIndex = 0;
            while (currentIndex < inventory.size() &&
                   referenceIndex < reference.size()) {
                const InventoryRecord& current = inventory[currentIndex];
                const InventoryRecord& prior = reference[referenceIndex];
                if (current.key < prior.key) {
                    ++currentIndex;
                } else if (prior.key < current.key) {
                    ++referenceIndex;
                } else {
                    if (current.support != prior.support ||
                        current.localAssignments != prior.localAssignments) {
                        throw std::runtime_error(
                            "future tail kernel inventory shared-key differential failed");
                    }
                    ++intersectionUnique;
                    currentCoveredOccurrences += current.occurrences;
                    referenceCoveredOccurrences += prior.occurrences;
                    minimumCoveredOccurrences += std::min(
                        current.occurrences, prior.occurrences);
                    intersectionKernelRecords += current.support;
                    ++currentIndex;
                    ++referenceIndex;
                }
            }
        }

        const uint64_t elapsedMilliseconds = (uint64_t)
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - started).count();
        const std::filesystem::path manifestTemporary =
            committedManifest.string() + ".tmp";
        {
            std::ofstream output(manifestTemporary, std::ios::trunc);
            if (!output) {
                throw std::runtime_error(
                    "future tail kernel inventory manifest create failed");
            }
            output << "format=FJFTKI01\n"
                   << "C=" << ctx_.c << "\n"
                   << "processed_rows=" << layer.processedRows << "\n"
                   << "layer_records=" << layer.records << "\n"
                   << "layer_mass_low=" << (uint64_t)layer.mass << "\n"
                   << "layer_mass_high=" << (uint64_t)(layer.mass >> 64)
                   << "\n"
                   << "source_file_bytes="
                   << std::filesystem::file_size(layer.path) << "\n"
                   << "sample_records=" << sampleRecords << "\n"
                   << "half_occurrences=" << expectedOccurrences << "\n"
                   << "canonical_unique=" << inventory.size() << "\n"
                   << "canonical_singletons=" << singletonKeys << "\n"
                   << "canonical_max_frequency=" << maximumFrequency << "\n"
                   << "canonical_collision_pairs_low="
                   << (uint64_t)collisionPairs << "\n"
                   << "canonical_collision_pairs_high="
                   << (uint64_t)(collisionPairs >> 64) << "\n"
                   << "frequency_keys_1=" << frequencyHistogram[0] << "\n"
                   << "frequency_keys_2=" << frequencyHistogram[1] << "\n"
                   << "frequency_keys_3_10=" << frequencyHistogram[2] << "\n"
                   << "frequency_keys_11_100=" << frequencyHistogram[3] << "\n"
                   << "frequency_keys_101_1000=" << frequencyHistogram[4] << "\n"
                   << "frequency_keys_1001_10000=" << frequencyHistogram[5] << "\n"
                   << "frequency_keys_10001_100000=" << frequencyHistogram[6] << "\n"
                   << "frequency_keys_100001_1000000=" << frequencyHistogram[7] << "\n"
                   << "frequency_keys_1000001_plus=" << frequencyHistogram[8] << "\n"
                   << "kernel_records=" << totalKernelRecords << "\n"
                   << "kernel_payload_bytes="
                   << totalKernelRecords * 8 << "\n"
                   << "kernel_max_support=" << maximumSupport << "\n"
                   << "kernel_zero_support_keys=" << zeroSupportKeys << "\n"
                   << "kernel_local_assignments="
                   << totalLocalAssignments << "\n"
                   << "support_keys_0_100=" << supportHistogram[0] << "\n"
                   << "support_keys_101_500=" << supportHistogram[1] << "\n"
                   << "support_keys_501_1000=" << supportHistogram[2] << "\n"
                   << "support_keys_1001_2000=" << supportHistogram[3] << "\n"
                   << "support_keys_2001_4000=" << supportHistogram[4] << "\n"
                   << "support_keys_4001_8000=" << supportHistogram[5] << "\n"
                   << "support_keys_8001_16000=" << supportHistogram[6] << "\n"
                   << "support_keys_16001_plus=" << supportHistogram[7] << "\n"
                   << "inventory_format=FJFTKI01\n"
                   << "inventory_record_bytes=32\n"
                   << "inventory_file_bytes="
                   << std::filesystem::file_size(inventoryFile) << "\n"
                   << "threads=" << inventoryThreads << "\n"
                   << "reference_directory=" << referenceIdentity << "\n"
                   << "reference_samples=" << referenceSamples << "\n"
                   << "reference_half_occurrences="
                   << referenceOccurrences << "\n"
                   << "reference_canonical_unique=" << referenceUnique << "\n"
                   << "reference_kernel_records="
                   << referenceKernelRecords << "\n"
                   << "intersection_unique=" << intersectionUnique << "\n"
                   << "intersection_current_unique_ppm="
                   << (inventory.empty() ? 0 :
                       intersectionUnique * 1000000 / inventory.size())
                   << "\n"
                   << "intersection_reference_unique_ppm="
                   << (referenceUnique == 0 ? 0 :
                       intersectionUnique * 1000000 / referenceUnique)
                   << "\n"
                   << "intersection_current_occurrences="
                   << currentCoveredOccurrences << "\n"
                   << "intersection_current_occurrences_ppm="
                   << (expectedOccurrences == 0 ? 0 :
                       currentCoveredOccurrences * 1000000 /
                           expectedOccurrences)
                   << "\n"
                   << "intersection_reference_occurrences="
                   << referenceCoveredOccurrences << "\n"
                   << "intersection_reference_occurrences_ppm="
                   << (referenceOccurrences == 0 ? 0 :
                       referenceCoveredOccurrences * 1000000 /
                           referenceOccurrences)
                   << "\n"
                   << "intersection_minimum_occurrences="
                   << minimumCoveredOccurrences << "\n"
                   << "intersection_kernel_records="
                   << intersectionKernelRecords << "\n"
                   << "union_unique="
                   << inventory.size() + referenceUnique -
                          intersectionUnique
                   << "\n"
                   << "union_kernel_records="
                   << totalKernelRecords + referenceKernelRecords -
                          intersectionKernelRecords
                   << "\n"
                   << "elapsed_milliseconds=" << elapsedMilliseconds
                   << "\n";
            output.flush();
            output.close();
            if (!output) {
                throw std::runtime_error(
                    "future tail kernel inventory manifest close failed");
            }
        }
        std::filesystem::rename(manifestTemporary, committedManifest);
        std::fprintf(stderr,
            "future tail kernel inventory complete records=%llu occurrences=%llu unique=%llu kernelRecords=%llu maxSupport=%llu intersection=%llu elapsed=%.3fs report=%s\n",
            (unsigned long long)sampleRecords,
            (unsigned long long)expectedOccurrences,
            (unsigned long long)inventory.size(),
            (unsigned long long)totalKernelRecords,
            (unsigned long long)maximumSupport,
            (unsigned long long)intersectionUnique,
            elapsedMilliseconds / 1000.0,
            committedManifest.string().c_str());
        std::fflush(stderr);
    }

    static uint64_t updateSevenRowKernelTableChecksum(
        uint64_t checksum, const void* data, size_t bytes) {
        const auto* input = static_cast<const unsigned char*>(data);
        for (size_t index = 0; index < bytes; ++index) {
            checksum ^= input[index];
            checksum *= 1099511628211ULL;
        }
        return checksum;
    }

    std::shared_ptr<SevenRowPersistentKernelTable>
    loadSevenRowPersistentKernelTable(
        const std::filesystem::path& directory,
        int expectedProcessedRows,
        bool progress) const {
        const std::filesystem::path manifestPath =
            directory / "COMMITTED.manifest";
        const std::filesystem::path tablePath =
            directory / "kernel-table.bin";
        if (!std::filesystem::exists(manifestPath) ||
            !std::filesystem::exists(tablePath)) {
            throw std::runtime_error(
                "future tail kernel table is not committed: " +
                directory.string());
        }
        const ExternalManifest manifest = readExternalManifest(manifestPath);
        if (externalManifestValue(manifest, "format", manifestPath) !=
                "FJFTKT01" ||
            externalManifestU64(manifest, "C", manifestPath) !=
                (uint64_t)ctx_.c ||
            externalManifestU64(
                manifest, "processed_rows", manifestPath) !=
                (uint64_t)expectedProcessedRows ||
            externalManifestU64(
                manifest, "table_file_bytes", manifestPath) !=
                std::filesystem::file_size(tablePath)) {
            throw std::runtime_error(
                "future tail kernel table manifest audit failed: " +
                manifestPath.string());
        }

        std::ifstream input(tablePath, std::ios::binary);
        if (!input) {
            throw std::runtime_error(
                "future tail kernel table open failed: " +
                tablePath.string());
        }
        std::array<char, 64> header{};
        input.read(header.data(), (std::streamsize)header.size());
        const char magic[8] = {'F', 'J', 'F', 'T', 'K', 'T', '0', '1'};
        uint64_t keyCount = 0;
        uint64_t recordCount = 0;
        uint64_t expectedChecksum = 0;
        uint64_t expectedFileBytes = 0;
        uint64_t indexRecordBytes = 0;
        uint64_t kernelRecordBytes = 0;
        std::memcpy(&keyCount, header.data() + 16, sizeof(keyCount));
        std::memcpy(&recordCount, header.data() + 24,
                    sizeof(recordCount));
        std::memcpy(&expectedChecksum, header.data() + 32,
                    sizeof(expectedChecksum));
        std::memcpy(&expectedFileBytes, header.data() + 40,
                    sizeof(expectedFileBytes));
        std::memcpy(&indexRecordBytes, header.data() + 48,
                    sizeof(indexRecordBytes));
        std::memcpy(&kernelRecordBytes, header.data() + 56,
                    sizeof(kernelRecordBytes));
        if (!input ||
            std::memcmp(header.data(), magic, sizeof(magic)) != 0 ||
            (uint8_t)header[8] != ctx_.c ||
            (uint8_t)header[9] != expectedProcessedRows ||
            indexRecordBytes != 24 || kernelRecordBytes != 8 ||
            keyCount > (std::numeric_limits<uint64_t>::max() - 64) / 24 ||
            recordCount >
                (std::numeric_limits<uint64_t>::max() - 64 -
                 24 * keyCount) / 8 ||
            expectedFileBytes != 64 + 24 * keyCount + 8 * recordCount ||
            expectedFileBytes != std::filesystem::file_size(tablePath) ||
            keyCount != externalManifestU64(
                manifest, "canonical_unique", manifestPath) ||
            recordCount != externalManifestU64(
                manifest, "kernel_records", manifestPath) ||
            expectedChecksum != externalManifestU64(
                manifest, "table_checksum", manifestPath)) {
            throw std::runtime_error(
                "future tail kernel table header audit failed: " +
                tablePath.string());
        }
        if (keyCount > std::numeric_limits<size_t>::max() ||
            recordCount > std::numeric_limits<size_t>::max()) {
            throw std::runtime_error(
                "future tail kernel table exceeds addressable memory");
        }

        auto table = std::make_shared<SevenRowPersistentKernelTable>();
        table->c = ctx_.c;
        table->processedRows = expectedProcessedRows;
        table->checksum = expectedChecksum;
        table->index.resize((size_t)keyCount);
        uint64_t checksum = 14695981039346656037ULL;
        std::array<char, 24> indexBytes{};
        uint64_t previousKey = 0;
        uint64_t nextOffset = 0;
        bool havePrevious = false;
        for (size_t index = 0; index < table->index.size(); ++index) {
            input.read(indexBytes.data(),
                       (std::streamsize)indexBytes.size());
            checksum = updateSevenRowKernelTableChecksum(
                checksum, indexBytes.data(), indexBytes.size());
            SevenRowPersistentKernelIndex& item = table->index[index];
            std::memcpy(&item.key, indexBytes.data(), sizeof(item.key));
            std::memcpy(&item.offset, indexBytes.data() + 8,
                        sizeof(item.offset));
            std::memcpy(&item.records, indexBytes.data() + 16,
                        sizeof(item.records));
            if (!input || (havePrevious && item.key <= previousKey) ||
                item.offset != nextOffset ||
                item.records > recordCount - nextOffset) {
                throw std::runtime_error(
                    "future tail kernel table index audit failed: " +
                    tablePath.string());
            }
            previousKey = item.key;
            havePrevious = true;
            nextOffset += item.records;
        }
        if (nextOffset != recordCount) {
            throw std::runtime_error(
                "future tail kernel table index coverage failed: " +
                tablePath.string());
        }
        table->records.resize((size_t)recordCount);
        if (recordCount != 0) {
            input.read(
                reinterpret_cast<char*>(table->records.data()),
                (std::streamsize)(recordCount * sizeof(TailKernelRecord)));
            checksum = updateSevenRowKernelTableChecksum(
                checksum, table->records.data(),
                table->records.size() * sizeof(TailKernelRecord));
        }
        char trailing = 0;
        if (!input || input.read(&trailing, 1) ||
            checksum != expectedChecksum) {
            throw std::runtime_error(
                "future tail kernel table payload audit failed: " +
                tablePath.string());
        }
        for (const SevenRowPersistentKernelIndex& item : table->index) {
            uint32_t previousSignature = 0;
            bool haveSignature = false;
            const size_t begin = (size_t)item.offset;
            const size_t end = begin + (size_t)item.records;
            for (size_t position = begin; position < end; ++position) {
                const TailKernelRecord& record = table->records[position];
                if (record.second == 0 ||
                    (haveSignature && record.first <= previousSignature)) {
                    throw std::runtime_error(
                        "future tail kernel table kernel ordering failed: " +
                        tablePath.string());
                }
                previousSignature = record.first;
                haveSignature = true;
            }
        }
        table->positions.reserve(table->index.size() * 2 + 1);
        for (size_t index = 0; index < table->index.size(); ++index) {
            if (!table->positions.emplace(
                    table->index[index].key, index).second) {
                throw std::runtime_error(
                    "future tail kernel table duplicate key");
            }
        }
        if (progress) {
            std::fprintf(stderr,
                "future tail kernel table loaded keys=%llu records=%llu bytes=%llu checksum=%llu path=%s\n",
                (unsigned long long)keyCount,
                (unsigned long long)recordCount,
                (unsigned long long)expectedFileBytes,
                (unsigned long long)expectedChecksum,
                tablePath.string().c_str());
            std::fflush(stderr);
        }
        return table;
    }

    void prepareSevenRowPersistentKernelTable(
        const ExternalFileInfo& layer, const Options& options) const {
        if (options.tailKernelTableDirectory.empty()) {
            sevenRowPersistentKernelTable_.reset();
            return;
        }
        if (ctx_.m - layer.processedRows != 7) {
            throw std::runtime_error(
                "future tail kernel table requires a seven-row layer");
        }

        struct SourceRecord {
            uint64_t key = 0;
            uint64_t support = 0;
            uint64_t localAssignments = 0;
        };
        auto readInventory = [&](const std::filesystem::path& directory) {
            const std::filesystem::path manifestPath =
                directory / "COMMITTED.manifest";
            const std::filesystem::path inventoryPath =
                directory / "half-kernel-inventory.bin";
            if (!std::filesystem::exists(manifestPath) ||
                !std::filesystem::exists(inventoryPath)) {
                throw std::runtime_error(
                    "future tail kernel table inventory is not committed: " +
                    directory.string());
            }
            const ExternalManifest manifest =
                readExternalManifest(manifestPath);
            if (externalManifestValue(manifest, "format", manifestPath) !=
                    "FJFTKI01" ||
                externalManifestU64(manifest, "C", manifestPath) !=
                    (uint64_t)ctx_.c ||
                externalManifestU64(
                    manifest, "processed_rows", manifestPath) !=
                    (uint64_t)layer.processedRows ||
                externalManifestU64(
                    manifest, "inventory_file_bytes", manifestPath) !=
                    std::filesystem::file_size(inventoryPath)) {
                throw std::runtime_error(
                    "future tail kernel table inventory manifest audit failed: " +
                    manifestPath.string());
            }
            std::ifstream input(inventoryPath, std::ios::binary);
            std::array<char, 32> header{};
            input.read(header.data(), (std::streamsize)header.size());
            const char magic[8] =
                {'F', 'J', 'F', 'T', 'K', 'I', '0', '1'};
            uint64_t unique = 0;
            std::memcpy(&unique, header.data() + 24, sizeof(unique));
            if (!input ||
                std::memcmp(header.data(), magic, sizeof(magic)) != 0 ||
                (uint8_t)header[8] != ctx_.c ||
                (uint8_t)header[9] != layer.processedRows ||
                std::filesystem::file_size(inventoryPath) !=
                    32 + 32 * (uintmax_t)unique) {
                throw std::runtime_error(
                    "future tail kernel table inventory header audit failed: " +
                    inventoryPath.string());
            }
            std::vector<SourceRecord> result;
            result.reserve((size_t)unique);
            std::array<char, 32> bytes{};
            uint64_t previousKey = 0;
            bool havePrevious = false;
            for (uint64_t index = 0; index < unique; ++index) {
                input.read(bytes.data(), (std::streamsize)bytes.size());
                uint64_t occurrences = 0;
                SourceRecord item;
                std::memcpy(&item.key, bytes.data(), sizeof(item.key));
                std::memcpy(&occurrences, bytes.data() + 8,
                            sizeof(occurrences));
                std::memcpy(&item.support, bytes.data() + 16,
                            sizeof(item.support));
                std::memcpy(&item.localAssignments, bytes.data() + 24,
                            sizeof(item.localAssignments));
                if (!input || occurrences == 0 ||
                    (havePrevious && item.key <= previousKey)) {
                    throw std::runtime_error(
                        "future tail kernel table inventory record audit failed: " +
                        inventoryPath.string());
                }
                previousKey = item.key;
                havePrevious = true;
                result.push_back(item);
            }
            char trailing = 0;
            if (input.read(&trailing, 1)) {
                throw std::runtime_error(
                    "future tail kernel table inventory has trailing data: " +
                    inventoryPath.string());
            }
            return result;
        };

        const std::filesystem::path tableDirectory =
            options.tailKernelTableDirectory;
        if (options.tailKernelInventoryDirectory.empty()) {
            sevenRowPersistentKernelTable_ =
                loadSevenRowPersistentKernelTable(
                    tableDirectory, layer.processedRows,
                    options.progress);
            return;
        }

        const std::filesystem::path currentDirectory =
            options.tailKernelInventoryDirectory;
        const std::string currentIdentity = std::filesystem::absolute(
            currentDirectory).lexically_normal().string();
        const std::string referenceIdentity =
            options.tailKernelInventoryReferenceDirectory.empty()
                ? "none"
                : std::filesystem::absolute(
                    options.tailKernelInventoryReferenceDirectory)
                    .lexically_normal().string();
        const std::vector<SourceRecord> current =
            readInventory(currentDirectory);
        std::vector<SourceRecord> reference;
        if (!options.tailKernelInventoryReferenceDirectory.empty()) {
            reference = readInventory(
                options.tailKernelInventoryReferenceDirectory);
        }
        std::vector<SourceRecord> sources;
        sources.reserve(current.size() + reference.size());
        size_t currentIndex = 0;
        size_t referenceIndex = 0;
        while (currentIndex < current.size() ||
               referenceIndex < reference.size()) {
            if (referenceIndex == reference.size() ||
                (currentIndex < current.size() &&
                 current[currentIndex].key <
                    reference[referenceIndex].key)) {
                sources.push_back(current[currentIndex++]);
            } else if (currentIndex == current.size() ||
                       reference[referenceIndex].key <
                           current[currentIndex].key) {
                sources.push_back(reference[referenceIndex++]);
            } else {
                const SourceRecord& first = current[currentIndex++];
                const SourceRecord& second = reference[referenceIndex++];
                if (first.support != second.support ||
                    first.localAssignments != second.localAssignments) {
                    throw std::runtime_error(
                        "future tail kernel table shared inventory differential failed");
                }
                sources.push_back(first);
            }
        }

        const std::filesystem::path committedManifest =
            tableDirectory / "COMMITTED.manifest";
        const std::filesystem::path tablePath =
            tableDirectory / "kernel-table.bin";
        if (std::filesystem::exists(committedManifest)) {
            const ExternalManifest manifest =
                readExternalManifest(committedManifest);
            if (externalManifestValue(
                    manifest, "inventory_directory", committedManifest) !=
                    currentIdentity ||
                externalManifestValue(
                    manifest, "reference_inventory_directory",
                    committedManifest) != referenceIdentity) {
                throw std::runtime_error(
                    "future tail kernel table committed source identity mismatch: " +
                    committedManifest.string());
            }
            auto table = loadSevenRowPersistentKernelTable(
                tableDirectory, layer.processedRows, options.progress);
            if (table->index.size() != sources.size()) {
                throw std::runtime_error(
                    "future tail kernel table committed key count mismatch");
            }
            for (size_t index = 0; index < sources.size(); ++index) {
                if (table->index[index].key != sources[index].key ||
                    table->index[index].records != sources[index].support) {
                    throw std::runtime_error(
                        "future tail kernel table committed inventory mismatch");
                }
            }
            sevenRowPersistentKernelTable_ = std::move(table);
            return;
        }
        if (std::filesystem::exists(tableDirectory)) {
            if (!std::filesystem::is_directory(tableDirectory) ||
                std::filesystem::directory_iterator(tableDirectory) !=
                    std::filesystem::directory_iterator()) {
                throw std::runtime_error(
                    "future tail kernel table output is not empty: " +
                    tableDirectory.string());
            }
        } else if (!std::filesystem::create_directories(tableDirectory)) {
            throw std::runtime_error(
                "future tail kernel table output create failed: " +
                tableDirectory.string());
        }

        const auto started = std::chrono::steady_clock::now();
        uint64_t totalRecords = 0;
        uint64_t totalLocalAssignments = 0;
        for (const SourceRecord& source : sources) {
            if (source.support >
                    std::numeric_limits<uint64_t>::max() - totalRecords ||
                source.localAssignments >
                    std::numeric_limits<uint64_t>::max() -
                        totalLocalAssignments) {
                throw std::runtime_error(
                    "future tail kernel table metric overflow");
            }
            totalRecords += source.support;
            totalLocalAssignments += source.localAssignments;
        }
        if (totalRecords > std::numeric_limits<size_t>::max()) {
            throw std::runtime_error(
                "future tail kernel table exceeds addressable memory");
        }
        auto table = std::make_shared<SevenRowPersistentKernelTable>();
        table->c = ctx_.c;
        table->processedRows = layer.processedRows;
        table->index.resize(sources.size());
        table->records.resize((size_t)totalRecords);
        uint64_t offset = 0;
        for (size_t index = 0; index < sources.size(); ++index) {
            table->index[index] = SevenRowPersistentKernelIndex{
                sources[index].key, offset, sources[index].support};
            offset += sources[index].support;
        }

        const int buildThreads = std::max(1, options.tailThreads);
#ifndef _OPENMP
        if (buildThreads != 1) {
            throw std::runtime_error(
                "future tail kernel table threads require OpenMP");
        }
#endif
        std::exception_ptr buildError;
#ifdef _OPENMP
#pragma omp parallel for num_threads(buildThreads) schedule(static)
#endif
        for (int64_t index = 0;
             index < (int64_t)sources.size(); ++index) {
            try {
                const SourceRecord& source = sources[(size_t)index];
                const SevenRowCanonicalHalf canonical =
                    sevenRowCanonicalHalfFromKey(source.key);
                uint64_t localAssignments = 0;
                const TailKernel kernel = makeSevenRowTailKernel(
                    canonical.columns, localAssignments);
                if (kernel.size() != source.support ||
                    localAssignments != source.localAssignments) {
                    throw std::runtime_error(
                        "future tail kernel table generated kernel differential failed");
                }
                std::copy(
                    kernel.begin(), kernel.end(),
                    table->records.begin() +
                        (size_t)table->index[(size_t)index].offset);
            } catch (...) {
#ifdef _OPENMP
#pragma omp critical(future_tail_kernel_table_exception)
#endif
                {
                    if (!buildError) buildError = std::current_exception();
                }
            }
        }
        if (buildError) std::rethrow_exception(buildError);

        uint64_t checksum = 14695981039346656037ULL;
        std::array<char, 24> indexBytes{};
        for (const SevenRowPersistentKernelIndex& item : table->index) {
            indexBytes.fill(0);
            std::memcpy(indexBytes.data(), &item.key, sizeof(item.key));
            std::memcpy(indexBytes.data() + 8, &item.offset,
                        sizeof(item.offset));
            std::memcpy(indexBytes.data() + 16, &item.records,
                        sizeof(item.records));
            checksum = updateSevenRowKernelTableChecksum(
                checksum, indexBytes.data(), indexBytes.size());
        }
        checksum = updateSevenRowKernelTableChecksum(
            checksum, table->records.data(),
            table->records.size() * sizeof(TailKernelRecord));
        table->checksum = checksum;
        const uint64_t fileBytes =
            64 + 24 * (uint64_t)table->index.size() +
            8 * (uint64_t)table->records.size();
        const std::filesystem::path temporaryTable =
            tablePath.string() + ".tmp";
        {
            std::ofstream output(
                temporaryTable, std::ios::binary | std::ios::trunc);
            if (!output) {
                throw std::runtime_error(
                    "future tail kernel table create failed: " +
                    temporaryTable.string());
            }
            std::array<char, 64> header{};
            const char magic[8] =
                {'F', 'J', 'F', 'T', 'K', 'T', '0', '1'};
            std::memcpy(header.data(), magic, sizeof(magic));
            header[8] = (char)ctx_.c;
            header[9] = (char)layer.processedRows;
            const uint64_t keyCount = table->index.size();
            const uint64_t recordCount = table->records.size();
            const uint64_t indexRecordBytes = 24;
            const uint64_t kernelRecordBytes = 8;
            std::memcpy(header.data() + 16, &keyCount,
                        sizeof(keyCount));
            std::memcpy(header.data() + 24, &recordCount,
                        sizeof(recordCount));
            std::memcpy(header.data() + 32, &checksum,
                        sizeof(checksum));
            std::memcpy(header.data() + 40, &fileBytes,
                        sizeof(fileBytes));
            std::memcpy(header.data() + 48, &indexRecordBytes,
                        sizeof(indexRecordBytes));
            std::memcpy(header.data() + 56, &kernelRecordBytes,
                        sizeof(kernelRecordBytes));
            output.write(header.data(), (std::streamsize)header.size());
            for (const SevenRowPersistentKernelIndex& item : table->index) {
                indexBytes.fill(0);
                std::memcpy(indexBytes.data(), &item.key,
                            sizeof(item.key));
                std::memcpy(indexBytes.data() + 8, &item.offset,
                            sizeof(item.offset));
                std::memcpy(indexBytes.data() + 16, &item.records,
                            sizeof(item.records));
                output.write(indexBytes.data(),
                             (std::streamsize)indexBytes.size());
            }
            if (!table->records.empty()) {
                output.write(
                    reinterpret_cast<const char*>(
                        table->records.data()),
                    (std::streamsize)(table->records.size() *
                                      sizeof(TailKernelRecord)));
            }
            output.flush();
            output.close();
            if (!output) {
                throw std::runtime_error(
                    "future tail kernel table close failed: " +
                    temporaryTable.string());
            }
        }
        if (std::filesystem::file_size(temporaryTable) != fileBytes) {
            throw std::runtime_error(
                "future tail kernel table file size round-trip failed");
        }
        {
            std::ifstream input(temporaryTable, std::ios::binary);
            std::array<char, 64> header{};
            input.read(header.data(), (std::streamsize)header.size());
            uint64_t roundTripChecksum = 14695981039346656037ULL;
            for (const SevenRowPersistentKernelIndex& expected :
                 table->index) {
                input.read(indexBytes.data(),
                           (std::streamsize)indexBytes.size());
                SevenRowPersistentKernelIndex actual;
                std::memcpy(&actual.key, indexBytes.data(),
                            sizeof(actual.key));
                std::memcpy(&actual.offset, indexBytes.data() + 8,
                            sizeof(actual.offset));
                std::memcpy(&actual.records, indexBytes.data() + 16,
                            sizeof(actual.records));
                if (!input || actual.key != expected.key ||
                    actual.offset != expected.offset ||
                    actual.records != expected.records) {
                    throw std::runtime_error(
                        "future tail kernel table index round-trip failed");
                }
                roundTripChecksum = updateSevenRowKernelTableChecksum(
                    roundTripChecksum, indexBytes.data(),
                    indexBytes.size());
            }
            constexpr size_t roundTripChunkRecords = 1u << 20;
            std::vector<TailKernelRecord> buffer(roundTripChunkRecords);
            size_t position = 0;
            while (position < table->records.size()) {
                const size_t count = std::min(
                    roundTripChunkRecords,
                    table->records.size() - position);
                input.read(reinterpret_cast<char*>(buffer.data()),
                           (std::streamsize)(count *
                                             sizeof(TailKernelRecord)));
                if (!input || std::memcmp(
                        buffer.data(), table->records.data() + position,
                        count * sizeof(TailKernelRecord)) != 0) {
                    throw std::runtime_error(
                        "future tail kernel table payload round-trip failed");
                }
                roundTripChecksum = updateSevenRowKernelTableChecksum(
                    roundTripChecksum, buffer.data(),
                    count * sizeof(TailKernelRecord));
                position += count;
            }
            char trailing = 0;
            if (input.read(&trailing, 1) ||
                roundTripChecksum != checksum) {
                throw std::runtime_error(
                    "future tail kernel table checksum round-trip failed");
            }
        }
        std::filesystem::rename(temporaryTable, tablePath);

        const auto elapsedMilliseconds = (uint64_t)
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - started).count();
        const std::filesystem::path temporaryManifest =
            committedManifest.string() + ".tmp";
        {
            std::ofstream output(temporaryManifest, std::ios::trunc);
            if (!output) {
                throw std::runtime_error(
                    "future tail kernel table manifest create failed");
            }
            output << "format=FJFTKT01\n"
                   << "C=" << ctx_.c << "\n"
                   << "processed_rows=" << layer.processedRows << "\n"
                   << "inventory_directory=" << currentIdentity << "\n"
                   << "reference_inventory_directory="
                   << referenceIdentity << "\n"
                   << "canonical_unique=" << table->index.size() << "\n"
                   << "kernel_records=" << table->records.size() << "\n"
                   << "kernel_payload_bytes="
                   << table->records.size() * sizeof(TailKernelRecord)
                   << "\n"
                   << "table_format=FJFTKT01\n"
                   << "table_index_record_bytes=24\n"
                   << "table_kernel_record_bytes=8\n"
                   << "table_file_bytes=" << fileBytes << "\n"
                   << "table_checksum=" << checksum << "\n"
                   << "kernel_local_assignments="
                   << totalLocalAssignments << "\n"
                   << "threads=" << buildThreads << "\n"
                   << "elapsed_milliseconds=" << elapsedMilliseconds
                   << "\n";
            output.flush();
            output.close();
            if (!output) {
                throw std::runtime_error(
                    "future tail kernel table manifest close failed");
            }
        }
        std::filesystem::rename(temporaryManifest, committedManifest);
        table->positions.reserve(table->index.size() * 2 + 1);
        for (size_t index = 0; index < table->index.size(); ++index) {
            table->positions.emplace(table->index[index].key, index);
        }
        sevenRowPersistentKernelTable_ = std::move(table);
        if (options.progress) {
            std::fprintf(stderr,
                "future tail kernel table complete keys=%llu records=%llu bytes=%llu checksum=%llu elapsed=%.3fs report=%s\n",
                (unsigned long long)sources.size(),
                (unsigned long long)totalRecords,
                (unsigned long long)fileBytes,
                (unsigned long long)checksum,
                elapsedMilliseconds / 1000.0,
                committedManifest.string().c_str());
            std::fflush(stderr);
        }
    }

    struct ExternalGenerationCheckpoint {
        uint64_t processedParents = 0;
        uint64_t rawRecords = 0;
        Count rawMass = 0;
        std::vector<ExternalFileInfo> runs;
    };

    std::optional<ExternalGenerationCheckpoint>
    loadExternalGenerationCheckpoint(
        const std::filesystem::path& workDirectory,
        const ExternalFileInfo& parent,
        int childProcessedRows) const {
        if (!std::filesystem::exists(workDirectory)) return std::nullopt;
        const std::string prefix = "generation-checkpoint-";
        const std::string suffix = ".manifest";
        uint64_t bestProcessed = 0;
        std::filesystem::path bestPath;
        for (const auto& entry :
             std::filesystem::directory_iterator(workDirectory)) {
            if (!entry.is_regular_file()) continue;
            const std::string name = entry.path().filename().string();
            if (name.size() <= prefix.size() + suffix.size() ||
                name.rfind(prefix, 0) != 0 ||
                name.substr(name.size() - suffix.size()) != suffix) {
                continue;
            }
            const std::string digits = name.substr(
                prefix.size(), name.size() - prefix.size() - suffix.size());
            if (digits.empty() ||
                !std::all_of(digits.begin(), digits.end(), [](unsigned char ch) {
                    return ch >= '0' && ch <= '9';
                })) {
                throw std::runtime_error(
                    "future external generation checkpoint name is invalid: " +
                    entry.path().string());
            }
            const uint64_t processed = std::stoull(digits);
            if (bestPath.empty() || processed > bestProcessed) {
                bestProcessed = processed;
                bestPath = entry.path();
            }
        }
        if (bestPath.empty()) return std::nullopt;

        const ExternalManifest values = readExternalManifest(bestPath);
        ExternalGenerationCheckpoint checkpoint;
        checkpoint.processedParents = externalManifestU64(
            values, "processed_parents", bestPath);
        checkpoint.rawRecords = externalManifestU64(
            values, "raw_records", bestPath);
        checkpoint.rawMass = externalManifestCount(
            values, "raw_mass_low", "raw_mass_high", bestPath);
        const uint64_t runCount = externalManifestU64(
            values, "run_count", bestPath);
        const uint64_t expectedReducedRecords = externalManifestU64(
            values, "run_reduced_records", bestPath);
        if (externalManifestValue(values, "format", bestPath) != "FJFGEN01" ||
            externalManifestU64(values, "C", bestPath) != (uint64_t)ctx_.c ||
            externalManifestValue(values, "parent_file", bestPath) !=
                parent.path.filename().string() ||
            externalManifestU64(values, "parent_processed_rows", bestPath) !=
                (uint64_t)parent.processedRows ||
            externalManifestU64(values, "child_processed_rows", bestPath) !=
                (uint64_t)childProcessedRows ||
            externalManifestU64(values, "parent_records", bestPath) !=
                parent.records ||
            externalManifestCount(
                values, "parent_mass_low", "parent_mass_high", bestPath) !=
                parent.mass ||
            checkpoint.processedParents != bestProcessed ||
            checkpoint.processedParents > parent.records) {
            throw std::runtime_error(
                "future external generation checkpoint audit failed: " +
                bestPath.string());
        }

        checkpoint.runs.reserve((size_t)runCount);
        Count runMass = 0;
        uint64_t reducedRecords = 0;
        for (uint64_t index = 0; index < runCount; ++index) {
            char name[64];
            std::snprintf(name, sizeof(name), "run-%08llu.bin",
                          (unsigned long long)index);
            const std::filesystem::path path = workDirectory / name;
            ExternalReader reader(path, ctx_.c, childProcessedRows);
            checkedAdd(runMass, reader.info.mass,
                       "future external generation checkpoint mass overflow");
            if (reducedRecords >
                std::numeric_limits<uint64_t>::max() - reader.info.records) {
                throw std::overflow_error(
                    "future external generation checkpoint record overflow");
            }
            reducedRecords += reader.info.records;
            checkpoint.runs.push_back(reader.info);
        }
        if (runMass != checkpoint.rawMass ||
            reducedRecords != expectedReducedRecords) {
            throw std::runtime_error(
                "future external generation run-prefix audit failed: " +
                bestPath.string());
        }

        std::vector<std::filesystem::path> uncommitted;
        for (const auto& entry :
             std::filesystem::directory_iterator(workDirectory)) {
            if (!entry.is_regular_file()) continue;
            const std::string name = entry.path().filename().string();
            bool remove = name.rfind("merge-", 0) == 0 ||
                          name.ends_with(".tmp");
            if (name.rfind("run-", 0) == 0 && name.ends_with(".bin")) {
                const std::string digits = name.substr(4, name.size() - 8);
                if (digits.empty() ||
                    !std::all_of(digits.begin(), digits.end(),
                        [](unsigned char ch) {
                            return ch >= '0' && ch <= '9';
                        })) {
                    throw std::runtime_error(
                        "future external generation run name is invalid: " +
                        entry.path().string());
                }
                remove = std::stoull(digits) >= runCount;
            }
            if (remove) uncommitted.push_back(entry.path());
        }
        for (const auto& path : uncommitted) {
            if (!std::filesystem::remove(path)) {
                throw std::runtime_error(
                    "future external generation cleanup failed: " +
                    path.string());
            }
        }
        return checkpoint;
    }

    void writeExternalGenerationCheckpoint(
        const std::filesystem::path& workDirectory,
        const ExternalFileInfo& parent,
        int childProcessedRows,
        uint64_t processedParents,
        uint64_t rawRecords,
        Count rawMass,
        const std::vector<ExternalFileInfo>& runs) const {
        char name[96];
        std::snprintf(name, sizeof(name),
                      "generation-checkpoint-%012llu.manifest",
                      (unsigned long long)processedParents);
        const std::filesystem::path finalPath = workDirectory / name;
        if (std::filesystem::exists(finalPath)) return;
        uint64_t reducedRecords = 0;
        Count runMass = 0;
        for (const auto& run : runs) {
            if (reducedRecords >
                std::numeric_limits<uint64_t>::max() - run.records) {
                throw std::overflow_error(
                    "future external generation checkpoint record overflow");
            }
            reducedRecords += run.records;
            checkedAdd(runMass, run.mass,
                       "future external generation checkpoint mass overflow");
        }
        if (runMass != rawMass) {
            throw std::runtime_error(
                "future external generation checkpoint mass mismatch");
        }
        const std::filesystem::path temporaryPath = finalPath.string() + ".tmp";
        std::ofstream output(temporaryPath, std::ios::trunc);
        if (!output) {
            throw std::runtime_error(
                "future external generation checkpoint create failed: " +
                temporaryPath.string());
        }
        output << "format=FJFGEN01\n"
               << "C=" << ctx_.c << "\n"
               << "parent_file=" << parent.path.filename().string() << "\n"
               << "parent_processed_rows=" << parent.processedRows << "\n"
               << "child_processed_rows=" << childProcessedRows << "\n"
               << "parent_records=" << parent.records << "\n"
               << "parent_mass_low=" << (uint64_t)parent.mass << "\n"
               << "parent_mass_high=" << (uint64_t)(parent.mass >> 64) << "\n"
               << "processed_parents=" << processedParents << "\n"
               << "raw_records=" << rawRecords << "\n"
               << "raw_mass_low=" << (uint64_t)rawMass << "\n"
               << "raw_mass_high=" << (uint64_t)(rawMass >> 64) << "\n"
               << "run_count=" << runs.size() << "\n"
               << "run_reduced_records=" << reducedRecords << "\n";
        output.flush();
        output.close();
        if (!output) {
            throw std::runtime_error(
                "future external generation checkpoint close failed: " +
                temporaryPath.string());
        }
        std::filesystem::rename(temporaryPath, finalPath);
    }

    ExternalFileInfo flushExternalBuffer(
        std::vector<ExternalRecord>& buffer,
        const std::filesystem::path& workDirectory,
        int processedRows,
        uint64_t runIndex,
        Stats& stats) const {
        if (buffer.empty()) {
            throw std::runtime_error("future external attempted to flush an empty buffer");
        }
        std::sort(buffer.begin(), buffer.end(), [](const auto& first, const auto& second) {
            return externalKeyLess(first.key, second.key);
        });

        char name[64];
        std::snprintf(name, sizeof(name), "run-%08llu.bin",
                      (unsigned long long)runIndex);
        const std::filesystem::path finalPath = workDirectory / name;
        const std::filesystem::path temporaryPath = finalPath.string() + ".tmp";
        std::ofstream output(temporaryPath, std::ios::binary | std::ios::trunc);
        if (!output) {
            throw std::runtime_error("future external run create failed: " + temporaryPath.string());
        }
        writeExternalHeader(output, ctx_.c, processedRows, 0, 0);

        ExternalFileInfo info;
        info.path = finalPath;
        info.processedRows = processedRows;
        for (size_t i = 0; i < buffer.size();) {
            size_t j = i + 1;
            Count weight = buffer[i].weight;
            while (j < buffer.size() && externalKeyEqual(buffer[i].key, buffer[j].key)) {
                checkedAdd(weight, buffer[j].weight,
                           "future external local reduction overflow");
                ++j;
            }
            writeExternalRecord(output, {buffer[i].key, weight});
            ++info.records;
            checkedAdd(info.mass, weight, "future external local mass overflow");
            i = j;
        }
        output.seekp(0);
        writeExternalHeader(output, ctx_.c, processedRows, info.records, info.mass);
        output.flush();
        output.close();
        if (!output) throw std::runtime_error("future external run close failed");
        std::filesystem::rename(temporaryPath, finalPath);
        ++stats.externalRuns;
        stats.externalBytesWritten += 40 + 41 * info.records;
        buffer.clear();
        return info;
    }

    ExternalFileInfo mergeExternalGroup(
        const std::vector<ExternalFileInfo>& inputs,
        const std::filesystem::path& outputPath,
        int processedRows,
        Stats& stats) const {
        if (inputs.empty()) {
            throw std::runtime_error("future external merge received no inputs");
        }
        struct HeapItem {
            ExternalRecord record;
            size_t reader = 0;
        };
        struct HeapGreater {
            bool operator()(const HeapItem& first, const HeapItem& second) const {
                return externalKeyLess(second.record.key, first.record.key);
            }
        };

        std::vector<std::unique_ptr<ExternalReader>> readers;
        readers.reserve(inputs.size());
        Count expectedMass = 0;
        std::priority_queue<HeapItem, std::vector<HeapItem>, HeapGreater> heap;
        for (size_t i = 0; i < inputs.size(); ++i) {
            readers.push_back(std::make_unique<ExternalReader>(
                inputs[i].path, ctx_.c, processedRows));
            checkedAdd(expectedMass, readers.back()->info.mass,
                       "future external merge input mass overflow");
            ExternalRecord record;
            if (readers.back()->next(record)) heap.push({record, i});
        }

        const std::filesystem::path temporaryPath = outputPath.string() + ".tmp";
        std::ofstream output(temporaryPath, std::ios::binary | std::ios::trunc);
        if (!output) {
            throw std::runtime_error("future external merge create failed: " + temporaryPath.string());
        }
        writeExternalHeader(output, ctx_.c, processedRows, 0, 0);

        ExternalFileInfo result;
        result.path = outputPath;
        result.processedRows = processedRows;
        while (!heap.empty()) {
            const ExternalKey key = heap.top().record.key;
            Count weight = 0;
            while (!heap.empty() && externalKeyEqual(heap.top().record.key, key)) {
                HeapItem item = heap.top();
                heap.pop();
                checkedAdd(weight, item.record.weight,
                           "future external merge reduction overflow");
                ExternalRecord next;
                if (readers[item.reader]->next(next)) {
                    heap.push({next, item.reader});
                }
            }
            writeExternalRecord(output, {key, weight});
            ++result.records;
            checkedAdd(result.mass, weight, "future external merge mass overflow");
        }
        if (result.mass != expectedMass) {
            throw std::runtime_error("future external merge mass audit failed");
        }
        output.seekp(0);
        writeExternalHeader(output, ctx_.c, processedRows, result.records, result.mass);
        output.flush();
        output.close();
        if (!output) throw std::runtime_error("future external merge close failed");
        std::filesystem::rename(temporaryPath, outputPath);
        stats.externalBytesWritten += 40 + 41 * result.records;
        return result;
    }

    ExternalFileInfo commitExternalRuns(
        std::vector<ExternalFileInfo> runs,
        const std::filesystem::path& rootDirectory,
        const std::filesystem::path& workDirectory,
        int processedRows,
        size_t mergeFanIn,
        Stats& stats) const {
        if (runs.empty()) throw std::runtime_error("future external layer produced no runs");
        mergeFanIn = std::max<size_t>(2, mergeFanIn);
        std::vector<std::filesystem::path> originalRunPaths;
        originalRunPaths.reserve(runs.size());
        for (const auto& run : runs) originalRunPaths.push_back(run.path);
        int pass = 0;
        while (runs.size() > 1) {
            std::vector<ExternalFileInfo> next;
            for (size_t begin = 0; begin < runs.size(); begin += mergeFanIn) {
                const size_t end = std::min(runs.size(), begin + mergeFanIn);
                std::vector<ExternalFileInfo> group(
                    runs.begin() + (ptrdiff_t)begin, runs.begin() + (ptrdiff_t)end);
                char name[80];
                std::snprintf(name, sizeof(name), "merge-%02d-%06llu.bin", pass,
                              (unsigned long long)(begin / mergeFanIn));
                const std::filesystem::path mergedPath = workDirectory / name;
                ExternalFileInfo merged = mergeExternalGroup(
                    group, mergedPath, processedRows, stats);
                if (pass != 0) {
                    for (const auto& input : group) {
                        if (!std::filesystem::remove(input.path)) {
                            throw std::runtime_error(
                                "future external could not remove merged input: " +
                                input.path.string());
                        }
                    }
                }
                next.push_back(std::move(merged));
            }
            runs = std::move(next);
            ++pass;
        }

        char layerName[48];
        std::snprintf(layerName, sizeof(layerName), "layer-%02d.bin", processedRows);
        const std::filesystem::path committedPath = rootDirectory / layerName;
        std::filesystem::rename(runs[0].path, committedPath);
        ExternalFileInfo committed = runs[0];
        committed.path = committedPath;

        const std::filesystem::path manifestPath =
            rootDirectory / (std::string(layerName) + ".manifest");
        writeExternalLayerManifest(committed, ctx_.c, manifestPath);
        stats.externalReducedRecords += committed.records;
        if (pass != 0) {
            for (const auto& path : originalRunPaths) {
                if (!std::filesystem::remove(path)) {
                    throw std::runtime_error(
                        "future external could not remove committed source run: " +
                        path.string());
                }
            }
        }
        std::vector<std::filesystem::path> checkpointManifests;
        for (const auto& entry :
             std::filesystem::directory_iterator(workDirectory)) {
            if (!entry.is_regular_file()) continue;
            const std::string name = entry.path().filename().string();
            if (name.rfind("generation-checkpoint-", 0) == 0 &&
                name.ends_with(".manifest")) {
                checkpointManifests.push_back(entry.path());
            }
        }
        for (const auto& path : checkpointManifests) {
            if (!std::filesystem::remove(path)) {
                throw std::runtime_error(
                    "future external could not remove generation checkpoint: " +
                    path.string());
            }
        }
        if (!std::filesystem::remove(workDirectory)) {
            throw std::runtime_error("future external work directory was not empty after commit");
        }
        return committed;
    }

    void accumulateTailEvaluation(const TailEvaluation& tail, Count weight,
                                  Stats& stats, Count& total) const {
        ++stats.tailStates;
        stats.tailZeroStates += tail.value == 0;
        stats.tailSupportTotal += (uint64_t)tail.leftSupport + tail.rightSupport;
        stats.tailSupportMax = std::max<uint64_t>(
            stats.tailSupportMax, (uint64_t)tail.leftSupport + tail.rightSupport);
        stats.tailLocalAssignments += tail.localAssignments;
        stats.tailValueMax = std::max<uint64_t>(stats.tailValueMax, tail.value);
        stats.tailKernelLookups += tail.kernelLookups;
        stats.tailKernelHits += tail.kernelHits;
        stats.tailKernelEvictions += tail.kernelEvictions;
        stats.tailColorCanonicalLookups += tail.colorCanonicalLookups;
        stats.tailColorCanonicalHits += tail.colorCanonicalHits;
        stats.tailColorCanonicalOrbits += tail.colorCanonicalOrbits;
        stats.tailColorCanonicalMappings += tail.colorCanonicalMappings;
        if (tail.value == 0) return;
        const Count maximum = ~(Count)0;
        if (weight > maximum / (Count)tail.value) {
            throw std::overflow_error("future-twin tail multiplication overflow");
        }
        const Count contribution = weight * (Count)tail.value;
        checkedAdd(total, contribution, "future-twin tail addition overflow");
    }

    void accumulateTailState(const State& state, Count weight,
                             int remainingRows,
                             size_t tailKernelCacheRecords,
                             bool useColorCanonical,
                             bool signatureDifferential,
                             Stats& stats, Count& total) const {
        const TailEvaluation tail = remainingRows == 6
            ? threePairTailValue(state)
            : sevenRowTailValue(
                state, tailKernelCacheRecords, useColorCanonical);
        if (signatureDifferential && remainingRows == 7) {
            const SevenRowTailSignature signature =
                sevenRowTailSignature(state);
            const TailEvaluation reconstructed =
                sevenRowTailValueFromSignature(
                    signature, tailKernelCacheRecords);
            if (tail.value != reconstructed.value) {
                throw std::runtime_error(
                    "future-twin seven-row signature differential failed");
            }
        }
        accumulateTailEvaluation(tail, weight, stats, total);
    }

    template<class NextParent>
    ExternalFileInfo generateExternalLayer(
        uint64_t parentCount,
        NextParent&& nextParent,
        int layer,
        JointCanonicalizer& canonicalizer,
        const Options& options,
        Stats& stats,
        const std::filesystem::path& rootDirectory,
        const ExternalFileInfo* resumableParent) const {
        const int remainingRows = ctx_.m - layer;
        const bool mateLayer = (layer & 1) != 0;
        const bool firstChoice = options.order == OrderMode::PairFirstTail;
        const bool adaptiveChoice =
            options.order == OrderMode::PairAdaptiveTail;
        const int fixedVertex = firstChoice ? 0 : remainingRows - 1;
        if (resumableParent != nullptr &&
            (resumableParent->processedRows != layer ||
             resumableParent->records != parentCount)) {
            throw std::runtime_error(
                "future external resumable parent identity mismatch");
        }
        stats.order.push_back(mateLayer ? -1 :
                              adaptiveChoice ? -2 : fixedVertex);
        canonicalizer.beginLayer(remainingRows - 1);
        ExternalCodec targetCodec(ctx_, remainingRows - 1);

        char workName[48];
        std::snprintf(workName, sizeof(workName), "work-layer-%02d", layer + 1);
        std::filesystem::path workDirectory;
        std::optional<ExternalGenerationCheckpoint> generationCheckpoint;
        std::filesystem::path firstAvailable;
        for (unsigned retry = 0; retry < 10000; ++retry) {
            std::filesystem::path candidate;
            if (retry == 0) {
                candidate = rootDirectory / workName;
            } else {
                char retryName[64];
                std::snprintf(retryName, sizeof(retryName),
                              "work-layer-%02d-retry-%03u",
                              layer + 1, retry);
                candidate = rootDirectory / retryName;
            }
            if (!std::filesystem::exists(candidate)) {
                if (firstAvailable.empty()) firstAvailable = candidate;
                break;
            }
            if (!std::filesystem::is_directory(candidate)) {
                throw std::runtime_error(
                    "future external work path is not a directory: " +
                    candidate.string());
            }
            if (resumableParent != nullptr) {
                auto loaded = loadExternalGenerationCheckpoint(
                    candidate, *resumableParent, layer + 1);
                if (loaded &&
                    (!generationCheckpoint ||
                     loaded->processedParents >
                         generationCheckpoint->processedParents)) {
                    generationCheckpoint = std::move(loaded);
                    workDirectory = candidate;
                }
            }
        }
        if (!generationCheckpoint) {
            if (firstAvailable.empty()) {
                throw std::runtime_error(
                    "future external exhausted retry work directories");
            }
            workDirectory = firstAvailable;
            if (!std::filesystem::create_directory(workDirectory)) {
                throw std::runtime_error(
                    "future external work directory create failed: " +
                    workDirectory.string());
            }
        }

        std::vector<ExternalRecord> buffer;
        const size_t bufferLimit = std::max<size_t>(1, options.externalBufferRecords);
        buffer.reserve(bufferLimit);
        std::vector<ExternalFileInfo> runs;
        uint64_t runIndex = 0;
        uint64_t rawRecords = 0;
        Count rawMass = 0;
        uint64_t processedParents = 0;
        if (generationCheckpoint) {
            processedParents = generationCheckpoint->processedParents;
            rawRecords = generationCheckpoint->rawRecords;
            rawMass = generationCheckpoint->rawMass;
            runs = std::move(generationCheckpoint->runs);
            runIndex = runs.size();
            stats.externalGenerationResumedParents += processedParents;
        }
        auto flush = [&]() {
            if (buffer.empty()) return;
            runs.push_back(flushExternalBuffer(
                buffer, workDirectory, layer + 1, runIndex++, stats));
        };

        State state;
        Count weight = 0;
        for (uint64_t skipped = 0; skipped < processedParents; ++skipped) {
            if (!nextParent(state, weight)) {
                throw std::runtime_error(
                    "future external generation checkpoint exceeds parent input");
            }
        }
        if (generationCheckpoint && options.progress) {
            std::fprintf(stderr,
                "future external generation resume layer=%d parents=%llu/%llu raw=%llu runs=%llu\n",
                layer + 1,
                (unsigned long long)processedParents,
                (unsigned long long)parentCount,
                (unsigned long long)rawRecords,
                (unsigned long long)runs.size());
            std::fflush(stderr);
        }
        while (nextParent(state, weight)) {
            ++processedParents;
            ++stats.statesExpanded;
            if (options.validateStates) validateState(state, layer);
            const int vertex = mateLayer
                ? pendingMateVertex(state, layer, remainingRows)
                : adaptiveChoice
                    ? adaptivePairVertex(state, remainingRows)
                    : fixedVertex;
            groupedTransitions(state, vertex, canonicalizer, stats,
                [&](const State& target, uint64_t multiplicity) {
                    const Count maximum = ~(Count)0;
                    if (weight > maximum / (Count)multiplicity) {
                        throw std::overflow_error(
                            "future external transition multiplication overflow");
                    }
                    const Count contribution = weight * (Count)multiplicity;
                    buffer.push_back({targetCodec.encode(target), contribution});
                    ++rawRecords;
                    checkedAdd(rawMass, contribution,
                               "future external emitted mass overflow");
                    if (buffer.size() >= bufferLimit) flush();
                }, true);
            const uint64_t checkpointInterval =
                options.externalGenerationCheckpointParents;
            if (resumableParent != nullptr && checkpointInterval != 0 &&
                (processedParents % checkpointInterval == 0 ||
                 processedParents == parentCount)) {
                flush();
                writeExternalGenerationCheckpoint(
                    workDirectory, *resumableParent, layer + 1,
                    processedParents, rawRecords, rawMass, runs);
                if (options.testStopExternalGenerationAfterParents != 0 &&
                    processedParents >=
                        options.testStopExternalGenerationAfterParents) {
                    throw std::runtime_error(
                        "future external generation intentional test stop");
                }
            }
            if (options.progress &&
                (processedParents <= 10 ||
                 (options.progressParentInterval != 0 &&
                  processedParents % options.progressParentInterval == 0))) {
                std::fprintf(stderr,
                    "future external work layer=%d parents=%llu/%llu raw=%llu runs=%llu canon=%llu fallbacks=%llu\n",
                    layer + 1,
                    (unsigned long long)processedParents,
                    (unsigned long long)parentCount,
                    (unsigned long long)rawRecords,
                    (unsigned long long)(runs.size() + !buffer.empty()),
                    (unsigned long long)stats.canonicalCalls,
                    (unsigned long long)stats.canonicalFallbacks);
                std::fflush(stderr);
            }
        }
        if (processedParents != parentCount) {
            throw std::runtime_error("future external parent count audit failed");
        }
        flush();
        stats.externalRawRecords += rawRecords;
        ExternalFileInfo committed = commitExternalRuns(
            std::move(runs), rootDirectory, workDirectory, layer + 1,
            options.externalMergeFanIn, stats);
        if (committed.mass != rawMass) {
            throw std::runtime_error("future external committed mass audit failed");
        }
        stats.layerStates[layer + 1] = committed.records;
        stats.peakStates = std::max<uint64_t>(stats.peakStates, committed.records);
        if (options.progress) {
            const std::string pivotText = mateLayer
                ? "mate" : adaptiveChoice
                    ? "adaptive" : std::to_string(fixedVertex + 1);
            std::fprintf(stderr,
                "future external layer=%d pivot=%s parents=%llu raw=%llu states=%llu runs=%llu bytesWritten=%llu\n",
                layer + 1, pivotText.c_str(),
                (unsigned long long)parentCount,
                (unsigned long long)rawRecords,
                (unsigned long long)committed.records,
                (unsigned long long)runIndex,
                (unsigned long long)stats.externalBytesWritten);
            std::fflush(stderr);
        }
        return committed;
    }

    struct ExternalTailCheckpoint {
        uint64_t processedRecords = 0;
        Count partial = 0;
        uint64_t zeroStates = 0;
        uint64_t supportTotal = 0;
        uint64_t supportMax = 0;
        uint64_t localAssignments = 0;
        uint64_t valueMax = 0;
        uint64_t kernelLookups = 0;
        uint64_t kernelHits = 0;
        uint64_t kernelEvictions = 0;
    };

    std::optional<ExternalTailCheckpoint> loadExternalTailCheckpoint(
        const ExternalFileInfo& layer) const {
        const std::filesystem::path rootDirectory = layer.path.parent_path();
        const std::string prefix = "tail-checkpoint-";
        const std::string suffix = ".manifest";
        uint64_t bestProcessed = 0;
        std::filesystem::path bestPath;
        for (const auto& entry : std::filesystem::directory_iterator(rootDirectory)) {
            if (!entry.is_regular_file()) continue;
            const std::string name = entry.path().filename().string();
            if (name.size() <= prefix.size() + suffix.size() ||
                name.rfind(prefix, 0) != 0 ||
                name.substr(name.size() - suffix.size()) != suffix) {
                continue;
            }
            const std::string digits = name.substr(
                prefix.size(), name.size() - prefix.size() - suffix.size());
            if (digits.empty() ||
                !std::all_of(digits.begin(), digits.end(), [](unsigned char ch) {
                    return ch >= '0' && ch <= '9';
                })) {
                continue;
            }
            uint64_t processed = 0;
            try {
                processed = std::stoull(digits);
            } catch (const std::exception&) {
                continue;
            }
            if (bestPath.empty() || processed > bestProcessed) {
                bestProcessed = processed;
                bestPath = entry.path();
            }
        }
        if (bestPath.empty()) return std::nullopt;

        const ExternalManifest values = readExternalManifest(bestPath);
        ExternalTailCheckpoint checkpoint;
        checkpoint.processedRecords = externalManifestU64(
            values, "processed_records", bestPath);
        if (externalManifestValue(values, "format", bestPath) != "FJFTAIL01" ||
            externalManifestU64(values, "C", bestPath) != (uint64_t)ctx_.c ||
            externalManifestU64(values, "processed_rows", bestPath) !=
                (uint64_t)layer.processedRows ||
            externalManifestU64(values, "layer_records", bestPath) != layer.records ||
            externalManifestCount(values, "layer_mass_low", "layer_mass_high", bestPath) !=
                layer.mass ||
            checkpoint.processedRecords != bestProcessed ||
            checkpoint.processedRecords > layer.records) {
            throw std::runtime_error(
                "future external tail checkpoint audit failed: " + bestPath.string());
        }
        checkpoint.partial = externalManifestCount(
            values, "partial_low", "partial_high", bestPath);
        checkpoint.zeroStates = externalManifestU64(values, "tail_zero", bestPath);
        checkpoint.supportTotal = externalManifestU64(
            values, "tail_support_total", bestPath);
        checkpoint.supportMax = externalManifestU64(
            values, "tail_support_max", bestPath);
        checkpoint.localAssignments = externalManifestU64(
            values, "tail_local_assignments", bestPath);
        checkpoint.valueMax = externalManifestU64(
            values, "tail_value_max", bestPath);
        checkpoint.kernelLookups = externalManifestU64(
            values, "tail_kernel_lookups", bestPath);
        checkpoint.kernelHits = externalManifestU64(
            values, "tail_kernel_hits", bestPath);
        checkpoint.kernelEvictions = externalManifestU64(
            values, "tail_kernel_evictions", bestPath);
        return checkpoint;
    }

    void writeExternalTailCheckpoint(
        const ExternalFileInfo& layer,
        uint64_t processedRecords,
        Count partial,
        const Stats& stats) const {
        char name[80];
        std::snprintf(name, sizeof(name),
                      "tail-checkpoint-%012llu.manifest",
                      (unsigned long long)processedRecords);
        const std::filesystem::path finalPath = layer.path.parent_path() / name;
        if (std::filesystem::exists(finalPath)) return;
        const std::filesystem::path temporaryPath = finalPath.string() + ".tmp";
        std::ofstream output(temporaryPath, std::ios::trunc);
        if (!output) {
            throw std::runtime_error(
                "future external tail checkpoint create failed: " +
                temporaryPath.string());
        }
        output << "format=FJFTAIL01\n"
               << "C=" << ctx_.c << "\n"
               << "processed_rows=" << layer.processedRows << "\n"
               << "layer_records=" << layer.records << "\n"
               << "layer_mass_low=" << (uint64_t)layer.mass << "\n"
               << "layer_mass_high=" << (uint64_t)(layer.mass >> 64) << "\n"
               << "processed_records=" << processedRecords << "\n"
               << "partial_low=" << (uint64_t)partial << "\n"
               << "partial_high=" << (uint64_t)(partial >> 64) << "\n"
               << "tail_zero=" << stats.tailZeroStates << "\n"
               << "tail_support_total=" << stats.tailSupportTotal << "\n"
               << "tail_support_max=" << stats.tailSupportMax << "\n"
               << "tail_local_assignments=" << stats.tailLocalAssignments << "\n"
               << "tail_value_max=" << stats.tailValueMax << "\n"
               << "tail_kernel_lookups=" << stats.tailKernelLookups << "\n"
               << "tail_kernel_hits=" << stats.tailKernelHits << "\n"
               << "tail_kernel_evictions=" << stats.tailKernelEvictions << "\n";
        output.flush();
        output.close();
        if (!output) {
            throw std::runtime_error(
                "future external tail checkpoint close failed: " +
                temporaryPath.string());
        }
        std::filesystem::rename(temporaryPath, finalPath);
    }

    void scanExternalTail(const ExternalFileInfo& layer,
                          const Options& options,
                          Result& result) const {
        const auto tailStart = std::chrono::steady_clock::now();
        ExternalReader reader(layer.path, ctx_.c, layer.processedRows);
        ExternalCodec codec(ctx_, ctx_.m - layer.processedRows);
        ExternalRecord record;
        Count total = 0;
        uint64_t parents = 0;
        if (!options.externalForceTailRescan) {
            if (const auto checkpoint = loadExternalTailCheckpoint(layer)) {
                parents = checkpoint->processedRecords;
                result.stats.externalTailResumedRecords = parents;
                total = checkpoint->partial;
                result.stats.tailStates = parents;
                result.stats.tailZeroStates = checkpoint->zeroStates;
                result.stats.tailSupportTotal = checkpoint->supportTotal;
                result.stats.tailSupportMax = checkpoint->supportMax;
                result.stats.tailLocalAssignments = checkpoint->localAssignments;
                result.stats.tailValueMax = checkpoint->valueMax;
                result.stats.tailKernelLookups = checkpoint->kernelLookups;
                result.stats.tailKernelHits = checkpoint->kernelHits;
                result.stats.tailKernelEvictions = checkpoint->kernelEvictions;
                for (uint64_t skipped = 0; skipped < parents; ++skipped) {
                    if (!reader.next(record)) {
                        throw std::runtime_error(
                            "future external tail checkpoint exceeds input");
                    }
                }
                if (options.progress) {
                    std::fprintf(stderr,
                        "future external tail resume parents=%llu/%llu partialLow=%llu\n",
                        (unsigned long long)parents,
                        (unsigned long long)reader.info.records,
                        (unsigned long long)(uint64_t)total);
                    std::fflush(stderr);
                }
            }
        }
        const int tailThreads = std::max(1, options.tailThreads);
#ifndef _OPENMP
        if (tailThreads != 1) {
            throw std::runtime_error(
                "future external parallel tail requires an OpenMP build");
        }
#endif
        const int remainingRows = ctx_.m - layer.processedRows;
        auto commitEvaluation = [&](const TailEvaluation& evaluation,
                                    Count weight) {
            accumulateTailEvaluation(
                evaluation, weight, result.stats, total);
            ++parents;
            const uint64_t checkpointInterval =
                options.externalTailCheckpointRecords;
            if (!options.externalReadOnly && checkpointInterval != 0 &&
                (parents % checkpointInterval == 0 ||
                 parents == reader.info.records)) {
                writeExternalTailCheckpoint(
                    layer, parents, total, result.stats);
            }
            if (options.progress && (parents <= 10 || parents % 100000 == 0)) {
                std::fprintf(stderr,
                    "future external tail parents=%llu/%llu zero=%llu maxSupport=%llu maxValue=%llu kernelHits=%llu/%llu colorHits=%llu/%llu colorOrbits=%llu\n",
                    (unsigned long long)parents,
                    (unsigned long long)reader.info.records,
                    (unsigned long long)result.stats.tailZeroStates,
                    (unsigned long long)result.stats.tailSupportMax,
                    (unsigned long long)result.stats.tailValueMax,
                    (unsigned long long)result.stats.tailKernelHits,
                    (unsigned long long)result.stats.tailKernelLookups,
                    (unsigned long long)result.stats.tailColorCanonicalHits,
                    (unsigned long long)result.stats.tailColorCanonicalLookups,
                    (unsigned long long)result.stats.tailColorCanonicalOrbits);
                std::fflush(stderr);
            }
        };

        if (tailThreads == 1) {
            while (reader.next(record)) {
                const State state = codec.decode(record.key);
                if (options.validateStates) {
                    validateState(state, layer.processedRows);
                }
                const TailEvaluation evaluation = remainingRows == 6
                    ? threePairTailValue(state)
                    : sevenRowTailValue(
                        state, options.tailKernelCacheRecords,
                        options.useColorCanonicalTail);
                commitEvaluation(evaluation, record.weight);
            }
        } else {
#ifdef _OPENMP
            std::vector<std::unique_ptr<Engine>> workers;
            std::vector<size_t> workerCacheCaps;
            workers.reserve((size_t)tailThreads);
            workerCacheCaps.reserve((size_t)tailThreads);
            const size_t cacheBase =
                options.tailKernelCacheRecords / (size_t)tailThreads;
            const size_t cacheRemainder =
                options.tailKernelCacheRecords % (size_t)tailThreads;
            for (int thread = 0; thread < tailThreads; ++thread) {
                workers.push_back(std::make_unique<Engine>(ctx_.c));
                workers.back()->sevenRowPersistentKernelTable_ =
                    sevenRowPersistentKernelTable_;
                workerCacheCaps.push_back(
                    cacheBase + ((size_t)thread < cacheRemainder ? 1 : 0));
            }

            const size_t tailChunkRecords =
                std::max<size_t>(1, options.tailChunkRecords);
            std::vector<State> states;
            std::vector<Count> weights;
            std::vector<TailEvaluation> evaluations;
            states.reserve(tailChunkRecords);
            weights.reserve(tailChunkRecords);
            evaluations.reserve(tailChunkRecords);
            while (true) {
                states.clear();
                weights.clear();
                while (states.size() < tailChunkRecords && reader.next(record)) {
                    State state = codec.decode(record.key);
                    if (options.validateStates) {
                        validateState(state, layer.processedRows);
                    }
                    states.push_back(std::move(state));
                    weights.push_back(record.weight);
                }
                if (states.empty()) break;
                evaluations.resize(states.size());
                std::exception_ptr parallelError;
#pragma omp parallel for num_threads(tailThreads) schedule(static)
                for (int64_t index = 0;
                     index < (int64_t)states.size(); ++index) {
                    try {
                        const int thread = omp_get_thread_num();
                        evaluations[(size_t)index] = remainingRows == 6
                            ? workers[(size_t)thread]->threePairTailValue(
                                states[(size_t)index])
                            : workers[(size_t)thread]->sevenRowTailValue(
                                states[(size_t)index],
                                workerCacheCaps[(size_t)thread],
                                options.useColorCanonicalTail);
                    } catch (...) {
#pragma omp critical(future_tail_exception)
                        {
                            if (!parallelError) {
                                parallelError = std::current_exception();
                            }
                        }
                    }
                }
                if (parallelError) std::rethrow_exception(parallelError);
                for (size_t index = 0; index < evaluations.size(); ++index) {
                    commitEvaluation(evaluations[index], weights[index]);
                }
            }
#endif
        }
        if (parents != reader.info.records) {
            throw std::runtime_error("future external tail record count audit failed");
        }
        result.stats.tailSeconds = std::chrono::duration<double>(
            std::chrono::steady_clock::now() - tailStart).count();
        result.value = total;
    }

    std::string externalGraphSignature(
        const std::array<uint16_t, MAX_M>& graph) const {
        std::string signature;
        for (int row = 0; row < ctx_.m; ++row) {
            if (row != 0) signature.push_back(',');
            signature += std::to_string(graph[row]);
        }
        return signature;
    }

    void validateExternalIdentity(
        const ExternalManifest& values,
        const std::filesystem::path& manifestPath,
        int startLayer,
        int prefixLayers,
        const Options& options,
        const std::string& graphSignature) const {
        if (externalManifestValue(values, "format", manifestPath) != "FJFTL01" ||
            externalManifestU64(values, "C", manifestPath) != (uint64_t)ctx_.c ||
            externalManifestU64(values, "start_layer", manifestPath) !=
                (uint64_t)startLayer ||
            externalManifestU64(values, "target_layer", manifestPath) !=
                (uint64_t)prefixLayers ||
            externalManifestU64(values, "tail_rows", manifestPath) !=
                (uint64_t)options.tailRemainingRows ||
            externalManifestValue(values, "order", manifestPath) !=
                orderName(options.order) ||
            externalManifestValue(values, "graph", manifestPath) != graphSignature) {
            throw std::runtime_error(
                "future external job identity audit failed: " + manifestPath.string());
        }
    }

    void appendExternalOrder(int layer, const Options& options, Stats& stats) const {
        const int remainingRows = ctx_.m - layer;
        const bool mateLayer = (layer & 1) != 0;
        const bool firstChoice = options.order == OrderMode::PairFirstTail;
        const bool adaptiveChoice =
            options.order == OrderMode::PairAdaptiveTail;
        const int fixedVertex = firstChoice ? 0 : remainingRows - 1;
        stats.order.push_back(mateLayer ? -1 :
                              adaptiveChoice ? -2 : fixedVertex);
    }

    void countJointExternal(
        WeightedLayer& current,
        int startLayer,
        int prefixLayers,
        JointCanonicalizer& canonicalizer,
        const std::array<uint16_t, MAX_M>& graph,
        const Options& options,
        Result& result) const {
        const auto externalStart = std::chrono::steady_clock::now();
        const std::filesystem::path rootDirectory = options.externalDirectory;
        const std::string graphSignature = externalGraphSignature(graph);
        const std::filesystem::path runningManifest = rootDirectory / "RUNNING.manifest";
        const std::filesystem::path committedManifest =
            rootDirectory / "COMMITTED.manifest";

        if (options.externalReadOnly &&
            !std::filesystem::exists(committedManifest)) {
            throw std::runtime_error(
                "future external read-only verification requires a committed job: " +
                rootDirectory.string());
        }

        if (std::filesystem::exists(committedManifest)) {
            const ExternalManifest values = readExternalManifest(committedManifest);
            validateExternalIdentity(values, committedManifest, startLayer,
                                     prefixLayers, options, graphSignature);
            const std::optional<ExternalFileInfo> completedLayer =
                loadExternalLayer(rootDirectory, prefixLayers);
            if (!completedLayer) {
                throw std::runtime_error(
                    "future external completed job is missing its final layer");
            }
            result.stats.externalReusedLayers = 1;
            result.stats.externalReusedRecords = completedLayer->records;
            result.stats.layerStates[prefixLayers] = completedLayer->records;
            result.stats.peakStates = std::max<uint64_t>(
                result.stats.peakStates, completedLayer->records);
            for (int layer = startLayer; layer < prefixLayers; ++layer) {
                appendExternalOrder(layer, options, result.stats);
            }
            WeightedLayer empty;
            current.swap(empty);
            analyzeExternalSevenRowKernelInventory(
                *completedLayer, options);
            prepareSevenRowPersistentKernelTable(
                *completedLayer, options);
            analyzeExternalSevenRowSignatures(
                *completedLayer, options);
            scanExternalTail(*completedLayer, options, result);
            const Count expectedResult = externalManifestCount(
                values, "result_low", "result_high", committedManifest);
            if (result.value != expectedResult) {
                throw std::runtime_error(
                    "future external completed result audit failed");
            }
            result.stats.externalSeconds = std::chrono::duration<double>(
                std::chrono::steady_clock::now() - externalStart).count();
            return;
        }

        if (std::filesystem::exists(rootDirectory)) {
            if (!std::filesystem::is_directory(rootDirectory) ||
                !std::filesystem::exists(runningManifest)) {
                throw std::runtime_error(
                    "future external directory is not a resumable job: " +
                    rootDirectory.string());
            }
            const ExternalManifest values = readExternalManifest(runningManifest);
            validateExternalIdentity(values, runningManifest, startLayer,
                                     prefixLayers, options, graphSignature);
        } else {
            if (!std::filesystem::create_directories(rootDirectory)) {
                throw std::runtime_error(
                    "future external directory create failed: " + rootDirectory.string());
            }
            const std::filesystem::path temporaryManifest =
                runningManifest.string() + ".tmp";
            std::ofstream running(temporaryManifest, std::ios::trunc);
            if (!running) throw std::runtime_error("future external running manifest failed");
            running << "format=FJFTL01\n"
                    << "C=" << ctx_.c << "\n"
                    << "start_layer=" << startLayer << "\n"
                    << "target_layer=" << prefixLayers << "\n"
                    << "tail_rows=" << options.tailRemainingRows << "\n"
                    << "order=" << orderName(options.order) << "\n"
                    << "graph=" << graphSignature << "\n"
                    << "buffer_records=" << options.externalBufferRecords << "\n"
                    << "merge_fan_in=" << options.externalMergeFanIn << "\n";
            running.flush();
            running.close();
            if (!running) throw std::runtime_error("future external running manifest failed");
            std::filesystem::rename(temporaryManifest, runningManifest);
        }

        std::vector<std::optional<ExternalFileInfo>> reusableLayers;
        bool missingLayer = false;
        for (int layer = startLayer; layer < prefixLayers; ++layer) {
            std::optional<ExternalFileInfo> reusable =
                loadExternalLayer(rootDirectory, layer + 1);
            if (!reusable) {
                missingLayer = true;
            } else if (missingLayer) {
                throw std::runtime_error(
                    "future external committed layers are not contiguous");
            }
            reusableLayers.push_back(std::move(reusable));
        }

        ExternalFileInfo previous;
        bool havePrevious = false;
        for (int layer = startLayer; layer < prefixLayers; ++layer) {
            const std::optional<ExternalFileInfo>& reusable =
                reusableLayers[(size_t)(layer - startLayer)];
            if (reusable) {
                appendExternalOrder(layer, options, result.stats);
                previous = *reusable;
                havePrevious = true;
                result.stats.layerStates[layer + 1] = previous.records;
                result.stats.peakStates = std::max<uint64_t>(
                    result.stats.peakStates, previous.records);
                ++result.stats.externalReusedLayers;
                result.stats.externalReusedRecords += previous.records;
                if (layer == startLayer) {
                    WeightedLayer empty;
                    current.swap(empty);
                }
                if (options.progress) {
                    std::fprintf(stderr,
                        "future external reuse layer=%d states=%llu massLow=%llu\n",
                        layer + 1,
                        (unsigned long long)previous.records,
                        (unsigned long long)(uint64_t)previous.mass);
                    std::fflush(stderr);
                }
                continue;
            }
            if (!havePrevious) {
                auto iterator = current.begin();
                auto nextParent = [&](State& state, Count& weight) {
                    if (iterator == current.end()) return false;
                    state = iterator->first;
                    weight = iterator->second;
                    ++iterator;
                    return true;
                };
                previous = generateExternalLayer(
                    current.size(), nextParent, layer, canonicalizer,
                    options, result.stats, rootDirectory, nullptr);
                WeightedLayer empty;
                current.swap(empty);
            } else {
                ExternalReader reader(previous.path, ctx_.c, previous.processedRows);
                ExternalCodec codec(ctx_, ctx_.m - previous.processedRows);
                auto nextParent = [&](State& state, Count& weight) {
                    ExternalRecord record;
                    if (!reader.next(record)) return false;
                    state = codec.decode(record.key);
                    weight = record.weight;
                    return true;
                };
                previous = generateExternalLayer(
                    reader.info.records, nextParent, layer, canonicalizer,
                    options, result.stats, rootDirectory, &reader.info);
            }
            havePrevious = true;
        }
        if (!havePrevious || previous.processedRows != prefixLayers) {
            throw std::runtime_error("future external did not reach the tail boundary");
        }
        scanExternalTail(previous, options, result);
        result.stats.externalSeconds = std::chrono::duration<double>(
            std::chrono::steady_clock::now() - externalStart).count();

        const std::filesystem::path committedTemporary =
            rootDirectory / "COMMITTED.manifest.tmp";
        {
            std::ofstream committed(committedTemporary, std::ios::trunc);
            if (!committed) throw std::runtime_error("future external commit manifest failed");
            committed << "format=FJFTL01\n"
                      << "C=" << ctx_.c << "\n"
                      << "start_layer=" << startLayer << "\n"
                      << "target_layer=" << prefixLayers << "\n"
                      << "tail_rows=" << options.tailRemainingRows << "\n"
                      << "order=" << orderName(options.order) << "\n"
                      << "graph=" << graphSignature << "\n"
                      << "processed_rows=" << previous.processedRows << "\n"
                      << "records=" << previous.records << "\n"
                      << "result_low=" << (uint64_t)result.value << "\n"
                      << "result_high=" << (uint64_t)(result.value >> 64) << "\n";
            committed.flush();
            committed.close();
            if (!committed) throw std::runtime_error("future external commit manifest failed");
        }
        std::filesystem::rename(committedTemporary, committedManifest);
        if (!std::filesystem::remove(runningManifest)) {
            throw std::runtime_error("future external running manifest removal failed");
        }
    }

    Result countJoint(const std::array<uint16_t, MAX_M>& graph,
                      const Options& options) const {
        Result result;
        const std::array<uint16_t, MAX_M> neighborhoods = rightNeighborhoods(graph);
        State initial;
        initial.record.fill(EMPTY_RECORD);
        for (int right = 0; right < ctx_.m; ++right) {
            initial.record[right] = Canonicalizer::pack(neighborhoods[right], 0);
        }
        std::sort(initial.record.begin(), initial.record.begin() + ctx_.m);

        JointCanonicalizer canonicalizer(
            ctx_, &result.stats, options.canonicalCacheCap, options.canonicalNodeBudget);
        canonicalizer.beginLayer(ctx_.m);
        WeightedLayer current, next;
        current.reserve(16);
        current.emplace(canonicalizer.canonical(initial), (Count)1);
        result.stats.layerStates[0] = 1;
        result.stats.peakStates = 1;

        const bool pairOrder = isPairTailOrder(options.order);
        const bool supportedTail =
            (options.tailRemainingRows == 6 && ctx_.c >= 3) ||
            (options.tailRemainingRows == 7 && ctx_.c >= 4);
        if (pairOrder && !supportedTail) {
            throw std::runtime_error(
                "future-twin pair tail requires tailRemainingRows=6 (C>=3) "
                "or 7 (C>=4)");
        }
        const bool useTail = pairOrder && supportedTail;
        const int prefixLayers = useTail
            ? ctx_.m - options.tailRemainingRows : ctx_.m;
        const int defaultExternalLayers =
            options.tailRemainingRows == 7 ? 1 : 2;
        const int externalLayers = options.externalDirectory.empty() ? 0 :
            options.externalLayerCount > 0
                ? options.externalLayerCount : defaultExternalLayers;
        if (externalLayers > prefixLayers) {
            throw std::runtime_error(
                "future external layer count exceeds the prefix length");
        }
        const int inMemoryLayers = useTail && !options.externalDirectory.empty()
            ? std::max(0, prefixLayers - externalLayers) : prefixLayers;
        for (int layer = 0; layer < inMemoryLayers; ++layer) {
            const int remainingRows = ctx_.m - layer;
            const bool mateLayer = pairOrder && ((layer & 1) != 0);
            const bool firstChoice = options.order == OrderMode::CanonicalFirst ||
                                     options.order == OrderMode::PairFirstTail;
            const bool adaptiveChoice =
                options.order == OrderMode::PairAdaptiveTail;
            const int fixedVertex = firstChoice ? 0 : remainingRows - 1;
            result.stats.order.push_back(mateLayer ? -1 :
                                         adaptiveChoice ? -2 : fixedVertex);
            canonicalizer.beginLayer(remainingRows - 1);
            next.clear();
            next.reserve(std::min<size_t>(2000000, current.size() * 8 + 16));
            uint64_t layerParents = 0;
            for (const auto& [state, weight] : current) {
                ++result.stats.statesExpanded;
                ++layerParents;
                if (options.validateStates) validateState(state, layer);
                const int vertex = mateLayer
                    ? pendingMateVertex(state, layer, remainingRows)
                    : adaptiveChoice
                        ? adaptivePairVertex(state, remainingRows)
                        : fixedVertex;
                groupedTransitions(state, vertex, canonicalizer, result.stats,
                    [&](const State& target, uint64_t multiplicity) {
                        const Count maximum = ~(Count)0;
                        if (weight > maximum / (Count)multiplicity) {
                            throw std::overflow_error("future-twin joint multiplication overflow");
                        }
                        const Count contribution = weight * (Count)multiplicity;
                        Count& destination = next[target];
                        if (destination > maximum - contribution) {
                            throw std::overflow_error("future-twin joint addition overflow");
                        }
                        destination += contribution;
                    }, true);
                if (options.progress &&
                    (layerParents <= 10 ||
                     (options.progressParentInterval != 0 &&
                      layerParents % options.progressParentInterval == 0))) {
                    std::fprintf(stderr,
                        "future joint work layer=%d parents=%llu/%llu next=%llu canon=%llu fallbacks=%llu maxSearch=%llu\n",
                        layer + 1,
                        (unsigned long long)layerParents,
                        (unsigned long long)current.size(),
                        (unsigned long long)next.size(),
                        (unsigned long long)result.stats.canonicalCalls,
                        (unsigned long long)result.stats.canonicalFallbacks,
                        (unsigned long long)result.stats.maxCanonicalSearchNodes);
                    std::fflush(stderr);
                }
            }
            current.swap(next);
            result.stats.layerStates[layer + 1] = current.size();
            result.stats.peakStates = std::max<uint64_t>(result.stats.peakStates, current.size());
            if (options.progress) {
                const std::string pivotText = mateLayer
                    ? "mate" : adaptiveChoice
                        ? "adaptive" : std::to_string(fixedVertex + 1);
                std::fprintf(stderr,
                    "future joint layer=%d/%d pivot=%s states=%llu leaves=%llu canon=%llu hits=%llu searchNodes=%llu fallbacks=%llu maxSearch=%llu\n",
                    layer + 1, prefixLayers, pivotText.c_str(),
                    (unsigned long long)current.size(),
                    (unsigned long long)result.stats.groupedLeaves,
                    (unsigned long long)result.stats.canonicalCalls,
                    (unsigned long long)result.stats.canonicalCacheHits,
                    (unsigned long long)result.stats.canonicalSearchNodes,
                    (unsigned long long)result.stats.canonicalFallbacks,
                    (unsigned long long)result.stats.maxCanonicalSearchNodes);
                std::fflush(stderr);
            }
            if (current.empty()) throw std::runtime_error("future-twin joint DP reached an empty layer");
        }

        if (useTail && inMemoryLayers < prefixLayers) {
            countJointExternal(current, inMemoryLayers, prefixLayers,
                               canonicalizer, graph, options, result);
            return result;
        }

        if (useTail) {
            const auto tailStart = std::chrono::steady_clock::now();
            Count total = 0;
            uint64_t tailParents = 0;
            for (const auto& [state, weight] : current) {
                ++tailParents;
                if (options.validateStates) validateState(state, prefixLayers);
                accumulateTailState(
                    state, weight, ctx_.m - prefixLayers,
                    options.tailKernelCacheRecords,
                    options.useColorCanonicalTail,
                    options.testTailSignatureDifferential,
                    result.stats, total);
                if (options.progress &&
                    (tailParents <= 10 || tailParents % 100000 == 0)) {
                    std::fprintf(stderr,
                        "future tail work parents=%llu/%llu zero=%llu maxSupport=%llu maxValue=%llu\n",
                        (unsigned long long)tailParents,
                        (unsigned long long)current.size(),
                        (unsigned long long)result.stats.tailZeroStates,
                        (unsigned long long)result.stats.tailSupportMax,
                        (unsigned long long)result.stats.tailValueMax);
                    std::fflush(stderr);
                }
            }
            result.stats.tailSeconds = std::chrono::duration<double>(
                std::chrono::steady_clock::now() - tailStart).count();
            result.value = total;
            if (options.progress) {
                const double meanSupport = result.stats.tailStates == 0 ? 0.0 :
                    (double)result.stats.tailSupportTotal / result.stats.tailStates;
                std::fprintf(stderr,
                    "future tail states=%llu zero=%llu meanSupport=%.3f maxSupport=%llu localAssignments=%llu maxValue=%llu kernelHits=%llu/%llu kernelEvictions=%llu time=%.3fs\n",
                    (unsigned long long)result.stats.tailStates,
                    (unsigned long long)result.stats.tailZeroStates,
                    meanSupport,
                    (unsigned long long)result.stats.tailSupportMax,
                    (unsigned long long)result.stats.tailLocalAssignments,
                    (unsigned long long)result.stats.tailValueMax,
                    (unsigned long long)result.stats.tailKernelHits,
                    (unsigned long long)result.stats.tailKernelLookups,
                    (unsigned long long)result.stats.tailKernelEvictions,
                    result.stats.tailSeconds);
                std::fflush(stderr);
            }
            return result;
        }

        if (current.size() != 1) throw std::runtime_error("future-twin joint terminal is not unique");
        if (options.validateStates) validateState(current.begin()->first, ctx_.m);
        result.value = current.begin()->second;
        return result;
    }

public:
    explicit Engine(int c) : ctx_(c) {}

    Result count(const std::array<uint16_t, MAX_M>& graph, const Options& options = {}) const {
        sevenRowPersistentKernelTable_.reset();
        if (options.order == OrderMode::CanonicalFirst ||
            options.order == OrderMode::CanonicalLast ||
            isPairTailOrder(options.order)) {
            return countJoint(graph, options);
        }
        Result result;
        const std::array<uint16_t, MAX_M> neighborhoods = rightNeighborhoods(graph);
        if (options.order != OrderMode::ReachableGreedy) {
            result.stats.order = eliminationOrder(neighborhoods, options.order);
        }

        State initial;
        initial.record.fill(EMPTY_RECORD);
        for (int right = 0; right < ctx_.m; ++right) {
            initial.record[right] = Canonicalizer::pack(neighborhoods[right], 0);
        }
        std::sort(initial.record.begin(), initial.record.begin() + ctx_.m);

        Canonicalizer canonicalizer(ctx_, &result.stats, options.canonicalCacheCap);
        using Layer = std::unordered_map<State, Count, StateHash>;
        Layer current;
        current.reserve(16);
        current.emplace(canonicalizer.canonical(initial), (Count)1);
        result.stats.layerStates[0] = current.size();
        result.stats.peakStates = current.size();

        uint16_t processed = 0;
        auto expandLayer = [&](int layer, int vertex, Layer& output) {
            canonicalizer.beginLayer();
            output.clear();
            output.reserve(std::min<size_t>(2000000, current.size() * 8 + 16));
            for (const auto& [state, weight] : current) {
                ++result.stats.statesExpanded;
                if (options.validateStates) validateState(state, layer);
                if (options.transitionDifferential) {
                    Stats scratch;
                    TransitionMap grouped = groupedTransitionMap(state, vertex, canonicalizer, scratch);
                    TransitionMap labelled = labelledTransitionMap(state, vertex, canonicalizer);
                    if (grouped != labelled) {
                        throw std::runtime_error("future-twin grouped transition differential failed");
                    }
                }
                groupedTransitions(state, vertex, canonicalizer, result.stats,
                    [&](const State& target, uint64_t multiplicity) {
                        const Count maximum = ~(Count)0;
                        if (weight > maximum / (Count)multiplicity) {
                            throw std::overflow_error("future-twin transition multiplication overflow");
                        }
                        const Count contribution = weight * (Count)multiplicity;
                        Count& destination = output[target];
                        if (destination > maximum - contribution) {
                            throw std::overflow_error("future-twin transition addition overflow");
                        }
                        destination += contribution;
                    });
            }
        };

        for (int layer = 0; layer < ctx_.m; ++layer) {
            int vertex = -1;
            Layer next;
            if (options.order == OrderMode::ReachableGreedy) {
                size_t bestSize = std::numeric_limits<size_t>::max();
                Layer candidate;
                for (int trial = 0; trial < ctx_.m; ++trial) {
                    if ((processed >> trial) & 1) continue;
                    expandLayer(layer, trial, candidate);
                    if (options.progress) {
                        std::fprintf(stderr,
                            "future order-search layer=%d vertex=%d candidateStates=%llu\n",
                            layer + 1, trial + 1, (unsigned long long)candidate.size());
                        std::fflush(stderr);
                    }
                    if (candidate.size() < bestSize ||
                        (candidate.size() == bestSize && (vertex < 0 || trial < vertex))) {
                        bestSize = candidate.size();
                        vertex = trial;
                        next = std::move(candidate);
                        candidate.clear();
                    }
                }
                if (vertex < 0) throw std::runtime_error("future-twin reachable order failed");
                result.stats.order.push_back(vertex);
            } else {
                vertex = result.stats.order[layer];
                expandLayer(layer, vertex, next);
            }
            processed |= (uint16_t)(1u << vertex);
            current.swap(next);
            result.stats.layerStates[layer + 1] = current.size();
            result.stats.peakStates = std::max<uint64_t>(result.stats.peakStates, current.size());
            if (options.progress) {
                std::fprintf(stderr,
                    "future layer=%d/%d vertex=%d states=%llu leaves=%llu canon=%llu cacheHits=%llu perms=%llu\n",
                    layer + 1, ctx_.m, vertex + 1,
                    (unsigned long long)current.size(),
                    (unsigned long long)result.stats.groupedLeaves,
                    (unsigned long long)result.stats.canonicalCalls,
                    (unsigned long long)result.stats.canonicalCacheHits,
                    (unsigned long long)result.stats.colorPermutationsTried);
                std::fflush(stderr);
            }
            if (current.empty()) throw std::runtime_error("future-twin DP reached an empty layer");
        }
        if (current.size() != 1) throw std::runtime_error("future-twin terminal layer is not unique");
        if (options.validateStates) validateState(current.begin()->first, ctx_.m);
        result.value = current.begin()->second;
        return result;
    }

    void canonicalSelfTest(uint64_t seed, int tests) const {
        std::mt19937_64 rng(seed);
        Stats stats;
        Canonicalizer canonicalizer(ctx_, &stats, 0);
        for (int test = 0; test < tests; ++test) {
            State state;
            state.record.fill(EMPTY_RECORD);
            for (int i = 0; i < ctx_.m; ++i) {
                const uint16_t tau = (uint16_t)(rng() & ((1u << ctx_.m) - 1u));
                const uint8_t colors = (uint8_t)(rng() & ctx_.fullColors);
                state.record[i] = Canonicalizer::pack(tau, colors);
            }
            std::sort(state.record.begin(), state.record.begin() + ctx_.m);
            const State fast = canonicalizer.refined(state);
            const State brute = canonicalizer.brute(state);
            if (canonicalizer.brute(fast) != brute) {
                throw std::runtime_error("future-twin canonical separation test failed");
            }
            const size_t pi = (size_t)(rng() % ctx_.permutations.size());
            const State relabelled = canonicalizer.transform(state, pi);
            if (canonicalizer.refined(relabelled) != fast) {
                throw std::runtime_error("future-twin canonical invariance test failed");
            }

            State changed = state;
            const int index = (int)(rng() % ctx_.m);
            changed.record[index] ^= 1u;
            std::sort(changed.record.begin(), changed.record.begin() + ctx_.m);
            const bool fastEqual = canonicalizer.refined(changed) == fast;
            const bool bruteEqual = canonicalizer.brute(changed) == brute;
            if (fastEqual != bruteEqual) {
                throw std::runtime_error("future-twin canonical equivalence test failed");
            }
        }
    }

    static void runSelfTests() {
        for (int c = 2; c <= MAX_C; ++c) {
            Engine engine(c);
            engine.canonicalSelfTest(0x9e3779b97f4a7c15ULL + (uint64_t)c, 100);
        }
        std::mt19937_64 rng(0x465554555245ULL);
        for (int c = 2; c <= 4; ++c) {
            Engine engine(c);
            for (int sample = 0; sample < 3; ++sample) {
                const auto graph = randomBalancedGraph(c, rng);
                Options options;
                options.order = sample & 1 ? OrderMode::Greedy : OrderMode::Minimax;
                options.canonicalCacheCap = 10000;
                options.validateStates = true;
                options.transitionDifferential = true;
                Result result = engine.count(graph, options);
                if (result.value == 0) throw std::runtime_error("future-twin self-test returned zero");
            }
        }
        for (int c = 3; c <= 4; ++c) {
            Engine engine(c);
            for (int sample = 0; sample < 3; ++sample) {
                const auto graph = randomBalancedGraph(c, rng);
                Options referenceOptions;
                referenceOptions.order = OrderMode::CanonicalLast;
                referenceOptions.canonicalCacheCap = 10000;
                referenceOptions.validateStates = true;
                const Result reference = engine.count(graph, referenceOptions);

                Options pairOptions = referenceOptions;
                pairOptions.order = OrderMode::PairFirstTail;
                const Result pairFirst = engine.count(graph, pairOptions);
                pairOptions.order = OrderMode::PairLastTail;
                const Result pairLast = engine.count(graph, pairOptions);
                pairOptions.order = OrderMode::PairAdaptiveTail;
                const Result pairAdaptive = engine.count(graph, pairOptions);
                if (pairFirst.value != reference.value ||
                    pairLast.value != reference.value ||
                    pairAdaptive.value != reference.value) {
                    throw std::runtime_error(
                        "future-twin three-pair tail differential failed");
                }
                if (c >= 4) {
                    pairOptions.tailRemainingRows = 7;
                    pairOptions.testTailSignatureDifferential = true;
                    pairOptions.order = OrderMode::PairFirstTail;
                    const Result sevenFirst = engine.count(graph, pairOptions);
                    pairOptions.order = OrderMode::PairLastTail;
                    const Result sevenLast = engine.count(graph, pairOptions);
                    pairOptions.order = OrderMode::PairAdaptiveTail;
                    const Result sevenAdaptive = engine.count(
                        graph, pairOptions);
                    pairOptions.useColorCanonicalTail = true;
                    const Result sevenColorCanonical = engine.count(
                        graph, pairOptions);
                    if (sevenFirst.value != reference.value ||
                        sevenLast.value != reference.value ||
                        sevenAdaptive.value != reference.value ||
                        sevenColorCanonical.value != reference.value) {
                        throw std::runtime_error(
                            "future-twin seven-row tail differential failed");
                    }
                }
            }
        }

        {
            Engine engine(5);
            const auto graph = randomBalancedGraph(5, rng);
            Options referenceOptions;
            referenceOptions.order = OrderMode::CanonicalLast;
            referenceOptions.canonicalCacheCap = 10000;
            referenceOptions.validateStates = true;
            const Result reference = engine.count(graph, referenceOptions);
            Options sevenOptions = referenceOptions;
            sevenOptions.order = OrderMode::PairLastTail;
            sevenOptions.tailRemainingRows = 7;
            sevenOptions.testTailSignatureDifferential = true;
            const Result seven = engine.count(graph, sevenOptions);
            sevenOptions.order = OrderMode::PairAdaptiveTail;
            const Result sevenAdaptive = engine.count(graph, sevenOptions);
            sevenOptions.useColorCanonicalTail = true;
            const Result sevenColorCanonical = engine.count(
                graph, sevenOptions);
            if (seven.value != reference.value ||
                sevenAdaptive.value != reference.value ||
                sevenColorCanonical.value != reference.value) {
                throw std::runtime_error(
                    "future-twin C=5 seven-row tail differential failed");
            }
        }

        {
            Engine engine(4);
            const auto graph = randomBalancedGraph(4, rng);
            Options referenceOptions;
            referenceOptions.order = OrderMode::CanonicalLast;
            referenceOptions.canonicalCacheCap = 10000;
            referenceOptions.validateStates = true;
            const Result reference = engine.count(graph, referenceOptions);

            std::filesystem::path temporaryRoot =
                std::filesystem::temp_directory_path() /
                ("future-twin-selftest-" + std::to_string(
                    (uint64_t)std::chrono::steady_clock::now()
                        .time_since_epoch().count()));
            for (unsigned suffix = 1; std::filesystem::exists(temporaryRoot); ++suffix) {
                temporaryRoot = std::filesystem::temp_directory_path() /
                    ("future-twin-selftest-" + std::to_string(
                        (uint64_t)std::chrono::steady_clock::now()
                            .time_since_epoch().count()) +
                     "-" + std::to_string(suffix));
            }
            struct TemporaryDirectoryGuard {
                std::filesystem::path path;
                ~TemporaryDirectoryGuard() {
                    std::error_code error;
                    std::filesystem::remove_all(path, error);
                }
            } guard{temporaryRoot};

            Options externalOptions = referenceOptions;
            externalOptions.order = OrderMode::PairLastTail;
            externalOptions.externalDirectory = temporaryRoot.string();
            externalOptions.externalBufferRecords = 7;
            externalOptions.externalMergeFanIn = 3;
            externalOptions.externalTailCheckpointRecords = 1;
            externalOptions.externalGenerationCheckpointParents = 1;
#ifdef _OPENMP
            externalOptions.tailThreads = 2;
            externalOptions.tailChunkRecords = 3;
#endif
            externalOptions.testStopExternalGenerationAfterParents = 1;
            bool generationStopped = false;
            try {
                (void)engine.count(graph, externalOptions);
            } catch (const std::runtime_error& error) {
                if (std::string(error.what()) !=
                    "future external generation intentional test stop") {
                    throw;
                }
                generationStopped = true;
            }
            if (!generationStopped) {
                throw std::runtime_error(
                    "future-twin external generation stop fixture failed");
            }
            externalOptions.testStopExternalGenerationAfterParents = 0;
            const Result first = engine.count(graph, externalOptions);
            if (first.value != reference.value ||
                first.stats.externalRawRecords == 0 ||
                first.stats.externalReducedRecords == 0 ||
                first.stats.tailStates < 2 ||
                first.stats.externalGenerationResumedParents != 1) {
                throw std::runtime_error(
                    "future-twin external sort-reduce differential failed: runs=" +
                    std::to_string(first.stats.externalRuns) +
                    " raw=" + std::to_string(first.stats.externalRawRecords) +
                    " reduced=" + std::to_string(
                        first.stats.externalReducedRecords) +
                    " tail=" + std::to_string(first.stats.tailStates) +
                    " resumed=" + std::to_string(
                        first.stats.externalGenerationResumedParents));
            }

            const std::filesystem::path committed =
                temporaryRoot / "COMMITTED.manifest";
            const std::filesystem::path running =
                temporaryRoot / "RUNNING.manifest";
            std::filesystem::copy_file(committed, running);
            const uint64_t resumePoint = first.stats.tailStates / 2;
            for (const auto& entry : std::filesystem::directory_iterator(temporaryRoot)) {
                const std::string name = entry.path().filename().string();
                if (name.rfind("tail-checkpoint-", 0) != 0 ||
                    name.size() < 10 ||
                    name.substr(name.size() - 9) != ".manifest") {
                    continue;
                }
                const ExternalManifest values = readExternalManifest(entry.path());
                if (externalManifestU64(
                        values, "processed_records", entry.path()) > resumePoint &&
                    !std::filesystem::remove(entry.path())) {
                    throw std::runtime_error(
                        "future-twin external tail fixture cleanup failed");
                }
            }
            if (!std::filesystem::remove(committed) ||
                !std::filesystem::remove(temporaryRoot / "layer-02.bin") ||
                !std::filesystem::remove(
                    temporaryRoot / "layer-02.bin.manifest")) {
                throw std::runtime_error(
                    "future-twin external resume fixture setup failed");
            }

            externalOptions.externalBufferRecords = 5;
            externalOptions.externalMergeFanIn = 5;
            const Result resumed = engine.count(graph, externalOptions);
            if (resumed.value != reference.value ||
                resumed.stats.externalReusedLayers != 1 ||
                resumed.stats.externalReusedRecords == 0 ||
                resumed.stats.externalTailResumedRecords != resumePoint) {
                throw std::runtime_error(
                    "future-twin external partial resume differential failed");
            }
            externalOptions.externalReadOnly = true;
            const Result completed = engine.count(graph, externalOptions);
            if (completed.value != reference.value ||
                completed.stats.externalReusedLayers != 1 ||
                completed.stats.statesExpanded != 0 ||
                completed.stats.externalTailResumedRecords !=
                    completed.stats.tailStates) {
                throw std::runtime_error(
                    "future-twin external completed resume differential failed");
            }

            Options signatureOptions = referenceOptions;
            signatureOptions.order = OrderMode::PairLastTail;
            signatureOptions.tailRemainingRows = 7;
            signatureOptions.externalDirectory =
                (temporaryRoot / "seven-row-source").string();
            signatureOptions.externalBufferRecords = 7;
            signatureOptions.externalMergeFanIn = 3;
            signatureOptions.externalTailCheckpointRecords = 1;
            const Result signatureSource = engine.count(graph, signatureOptions);
            if (signatureSource.value != reference.value ||
                signatureSource.stats.tailStates == 0) {
                throw std::runtime_error(
                    "future-twin seven-row external signature source failed");
            }
            signatureOptions.externalReadOnly = true;
            signatureOptions.tailSignatureDirectory =
                (temporaryRoot / "signature-analysis").string();
            signatureOptions.tailSignatureSampleRecords =
                signatureSource.stats.tailStates;
            signatureOptions.tailSignatureValidationRecords =
                std::min<uint64_t>(signatureSource.stats.tailStates, 20);
            signatureOptions.tailColorCanonicalSampleKeys = 2;
            signatureOptions.tailColorSignatureRecords =
                std::min<uint64_t>(signatureSource.stats.tailStates, 20);
            signatureOptions.tailKernelInventoryDirectory =
                (temporaryRoot / "kernel-inventory").string();
            signatureOptions.tailKernelInventoryRecords =
                signatureSource.stats.tailStates;
            signatureOptions.tailKernelTableDirectory =
                (temporaryRoot / "kernel-table").string();
            signatureOptions.useColorCanonicalTail = true;
            const Result analyzed = engine.count(graph, signatureOptions);
            const std::filesystem::path signatureManifest =
                temporaryRoot / "signature-analysis" / "COMMITTED.manifest";
            if (analyzed.value != reference.value ||
                !std::filesystem::exists(signatureManifest)) {
                throw std::runtime_error(
                    "future-twin external signature analysis differential failed");
            }
            const ExternalManifest signatureValues =
                readExternalManifest(signatureManifest);
            if (externalManifestValue(
                    signatureValues, "format", signatureManifest) !=
                    "FJFTSIG01" ||
                externalManifestU64(
                    signatureValues, "sample_records", signatureManifest) !=
                    signatureSource.stats.tailStates ||
                externalManifestU64(
                    signatureValues, "validation_records", signatureManifest) !=
                    signatureOptions.tailSignatureValidationRecords ||
                externalManifestU64(
                    signatureValues, "signature_unique", signatureManifest) == 0 ||
                externalManifestU64(
                    signatureValues, "half_unique", signatureManifest) == 0 ||
                externalManifestU64(
                    signatureValues, "color_sample_keys", signatureManifest) == 0 ||
                externalManifestU64(
                    signatureValues, "color_canonical_unique", signatureManifest) == 0) {
                throw std::runtime_error(
                    "future-twin external signature analysis manifest audit failed");
            }
            const std::filesystem::path inventoryManifest =
                temporaryRoot / "kernel-inventory" / "COMMITTED.manifest";
            const ExternalManifest inventoryValues =
                readExternalManifest(inventoryManifest);
            if (externalManifestValue(
                    inventoryValues, "format", inventoryManifest) !=
                    "FJFTKI01" ||
                externalManifestU64(
                    inventoryValues, "sample_records", inventoryManifest) !=
                    signatureSource.stats.tailStates ||
                externalManifestU64(
                    inventoryValues, "half_occurrences", inventoryManifest) !=
                    6 * signatureSource.stats.tailStates ||
                externalManifestU64(
                    inventoryValues, "canonical_unique", inventoryManifest) == 0 ||
                externalManifestU64(
                    inventoryValues, "kernel_records", inventoryManifest) == 0 ||
                !std::filesystem::exists(
                    temporaryRoot / "kernel-inventory" /
                        "half-kernel-inventory.bin")) {
                throw std::runtime_error(
                    "future-twin external kernel inventory manifest audit failed");
            }
            const std::filesystem::path tableManifest =
                temporaryRoot / "kernel-table" / "COMMITTED.manifest";
            const ExternalManifest tableValues =
                readExternalManifest(tableManifest);
            if (externalManifestValue(
                    tableValues, "format", tableManifest) !=
                    "FJFTKT01" ||
                externalManifestU64(
                    tableValues, "canonical_unique", tableManifest) !=
                    externalManifestU64(
                        inventoryValues, "canonical_unique",
                        inventoryManifest) ||
                externalManifestU64(
                    tableValues, "kernel_records", tableManifest) !=
                    externalManifestU64(
                        inventoryValues, "kernel_records",
                        inventoryManifest) ||
                !std::filesystem::exists(
                    temporaryRoot / "kernel-table" /
                        "kernel-table.bin")) {
                throw std::runtime_error(
                    "future-twin external kernel table manifest audit failed");
            }
            const Result reusedAnalysis = engine.count(graph, signatureOptions);
            if (reusedAnalysis.value != reference.value) {
                throw std::runtime_error(
                    "future-twin external signature analysis reuse failed");
            }
            Options tableScan = signatureOptions;
            tableScan.tailSignatureDirectory.clear();
            tableScan.tailSignatureReferenceDirectory.clear();
            tableScan.tailSignatureSampleRecords = 0;
            tableScan.tailSignatureValidationRecords = 0;
            tableScan.tailColorCanonicalSampleKeys = 0;
            tableScan.tailColorSignatureRecords = 0;
            tableScan.tailKernelInventoryDirectory.clear();
            tableScan.tailKernelInventoryReferenceDirectory.clear();
            tableScan.tailKernelInventoryRecords = 0;
            tableScan.externalForceTailRescan = true;
            tableScan.tailKernelCacheRecords = 0;
            const Result tableRescan = engine.count(graph, tableScan);
            if (tableRescan.value != reference.value ||
                tableRescan.stats.tailStates !=
                    signatureSource.stats.tailStates ||
                tableRescan.stats.tailLocalAssignments != 0 ||
                tableRescan.stats.tailKernelLookups == 0 ||
                tableRescan.stats.tailKernelHits !=
                    tableRescan.stats.tailKernelLookups ||
                tableRescan.stats.tailKernelEvictions != 0) {
                throw std::runtime_error(
                    "future-twin persistent kernel table rescan differential failed");
            }
        }
        std::fprintf(stderr,
            "future selftest canonical C=2..6, grouped transitions C=2..4, "
            "three-pair tail C=3..4, seven-row tail C=4..5, "
            "and external generation/tail/signature/inventory/table resume "
            "C=4 [OK]\n");
    }
};

} // namespace future_twin
