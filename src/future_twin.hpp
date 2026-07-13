#pragma once

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <functional>
#include <limits>
#include <map>
#include <numeric>
#include <random>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

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
};

inline const char* orderName(OrderMode mode) {
    if (mode == OrderMode::Minimax) return "minimax";
    if (mode == OrderMode::Greedy) return "greedy";
    if (mode == OrderMode::ReverseMinimax) return "reverse";
    if (mode == OrderMode::ReachableGreedy) return "reachable";
    if (mode == OrderMode::CanonicalFirst) return "canonical-first";
    return "canonical-last";
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
};

struct Options {
    OrderMode order = OrderMode::Minimax;
    size_t canonicalCacheCap = 500000;
    uint64_t canonicalNodeBudget = 20000;
    bool validateStates = false;
    bool transitionDifferential = false;
    bool progress = false;
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
            for (size_t pi = 0; pi < permutations.size(); ++pi) {
                for (int mask = 0; mask < (1 << c); ++mask) {
                    uint8_t image = 0;
                    for (int color = 0; color < c; ++color) {
                        if ((mask >> color) & 1) image |= (uint8_t)(1u << permutations[pi][color]);
                    }
                    maskMaps[pi][mask] = image;
                }
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
        using Layer = std::unordered_map<State, Count, StateHash>;
        Layer current, next;
        current.reserve(16);
        current.emplace(canonicalizer.canonical(initial), (Count)1);
        result.stats.layerStates[0] = 1;
        result.stats.peakStates = 1;

        for (int layer = 0; layer < ctx_.m; ++layer) {
            const int remainingRows = ctx_.m - layer;
            const int vertex = options.order == OrderMode::CanonicalFirst ? 0 : remainingRows - 1;
            result.stats.order.push_back(vertex);
            canonicalizer.beginLayer(remainingRows - 1);
            next.clear();
            next.reserve(std::min<size_t>(2000000, current.size() * 8 + 16));
            uint64_t layerParents = 0;
            for (const auto& [state, weight] : current) {
                ++result.stats.statesExpanded;
                ++layerParents;
                if (options.validateStates) validateState(state, layer);
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
                    (layerParents <= 10 || layerParents % 100000 == 0)) {
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
                std::fprintf(stderr,
                    "future joint layer=%d/%d pivot=%d states=%llu leaves=%llu canon=%llu hits=%llu searchNodes=%llu fallbacks=%llu maxSearch=%llu\n",
                    layer + 1, ctx_.m, vertex + 1,
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
        if (current.size() != 1) throw std::runtime_error("future-twin joint terminal is not unique");
        if (options.validateStates) validateState(current.begin()->first, ctx_.m);
        result.value = current.begin()->second;
        return result;
    }

public:
    explicit Engine(int c) : ctx_(c) {}

    Result count(const std::array<uint16_t, MAX_M>& graph, const Options& options = {}) const {
        if (options.order == OrderMode::CanonicalFirst ||
            options.order == OrderMode::CanonicalLast) {
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
        std::fprintf(stderr,
            "future selftest canonical C=2..6 and grouped transitions C=2..4 [OK]\n");
    }
};

} // namespace future_twin
