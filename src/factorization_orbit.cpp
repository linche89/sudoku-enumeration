// factorization_orbit.cpp -- exact 2xC Sudoku counter via global skeleton
// orbits and ordered 1-factorizations of regular bipartite graphs.
//
// A skeleton sequence is a C x 2C binary matrix B with every row of weight C.
// After symbol relabelling, B is determined by the histogram of its 2C column
// membership patterns.  Row permutations and independent row complements act
// as signed coordinate permutations on that histogram.  We enumerate those
// histogram orbits exactly and attach their full labelled orbit sizes.
//
// For one orbit representative, stack(B) is the number F(Q_B) of ordered
// 1-factorizations of the associated C-regular bipartite graph.  It is counted
// recursively by removing one perfect matching, with a memo shared by every
// outer orbit.  Therefore
//
//   N(C) = sum_[B] |Orb(B)| * F(Q_B)^2.
//
// Exact gates:
//   C=2 -> 288, C=3 -> 28200960,
//   C=4 -> 29136487207403520,
//   C=5 -> 1903816047972624930994913280000.

#include <algorithm>
#include <array>
#include <bit>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <filesystem>
#include <fstream>
#include <functional>
#include <limits>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace {

constexpr int MAX_C = 6;
constexpr int MAX_M = 2 * MAX_C;
constexpr int MAX_PATTERNS = 1 << MAX_C;

int C = 0;
int M = 0;
int FULL = 0;
int NPAT = 0;

using Hist = std::array<uint8_t, MAX_PATTERNS>;

struct HistHash {
    size_t operator()(const Hist& h) const noexcept {
        uint64_t x = 0x9e3779b97f4a7c15ULL;
        for (int i = 0; i < NPAT; ++i) {
            x ^= (uint64_t)h[i] + 0x9e3779b97f4a7c15ULL + (x << 6) + (x >> 2);
        }
        return (size_t)x;
    }
};

bool histLess(const Hist& a, const Hist& b) {
    return std::lexicographical_compare(a.begin(), a.begin() + NPAT,
                                        b.begin(), b.begin() + NPAT);
}

Hist flipCoordinate(const Hist& in, int bit) {
    Hist out{};
    const int mask = 1 << bit;
    for (int p = 0; p < NPAT; ++p) out[p ^ mask] = in[p];
    return out;
}

Hist swapCoordinates(const Hist& in, int a) {
    Hist out{};
    const int ba = 1 << a;
    const int bb = 1 << (a + 1);
    for (int p = 0; p < NPAT; ++p) {
        const int xa = (p & ba) != 0;
        const int xb = (p & bb) != 0;
        int q = p;
        if (xa != xb) q ^= ba | bb;
        out[q] = in[p];
    }
    return out;
}

uint64_t factorial(int n) {
    uint64_t v = 1;
    for (int i = 2; i <= n; ++i) v *= (uint64_t)i;
    return v;
}

uint64_t choose(int n, int k) {
    uint64_t v = 1;
    for (int i = 1; i <= k; ++i) v = v * (uint64_t)(n - k + i) / (uint64_t)i;
    return v;
}

unsigned __int128 ipow(unsigned __int128 x, int e) {
    unsigned __int128 r = 1;
    while (e-- > 0) r *= x;
    return r;
}

std::string u128ToString(unsigned __int128 value) {
    if (value == 0) return "0";
    std::string result;
    while (value != 0) {
        result.push_back((char)('0' + value % 10));
        value /= 10;
    }
    std::reverse(result.begin(), result.end());
    return result;
}

struct OuterClass {
    Hist representative{};
    uint64_t histogramOrbitSize = 0;
    uint64_t labelledMultiplicity = 0;
    unsigned __int128 factorizationCount = 0;
};

struct Vec {
    std::array<uint8_t, 1 << (MAX_C - 1)> counts{};
    uint64_t marginal = 0;
};

struct HistCanon {
    Hist representative{};
    uint32_t mapsToRepresentative = 0;
};

using PatternMap = std::array<uint8_t, MAX_PATTERNS>;

std::vector<PatternMap> coordinatePermutationMaps(int dimensions) {
    std::array<int, MAX_C> order{};
    for (int i = 0; i < dimensions; ++i) order[i] = i;
    std::vector<PatternMap> maps;
    do {
        PatternMap map{};
        const int patterns = 1 << dimensions;
        for (int p = 0; p < patterns; ++p) {
            int q = 0;
            for (int outBit = 0; outBit < dimensions; ++outBit) {
                if ((p >> outBit) & 1) q |= 1 << order[outBit];
            }
            map[p] = (uint8_t)q;
        }
        maps.push_back(map);
    } while (std::next_permutation(order.begin(), order.begin() + dimensions));
    return maps;
}

HistCanon canonicalizeHistogram(const Hist& input, int dimensions,
                                const std::vector<PatternMap>& permutationMaps) {
    const int patterns = 1 << dimensions;
    uint8_t maximumCell = 0;
    for (int p = 0; p < patterns; ++p) maximumCell = std::max(maximumCell, input[p]);

    HistCanon result;
    bool have = false;
    for (int flip = 0; flip < patterns; ++flip) {
        // We use the lexicographically greatest orbit member.  Its first cell
        // must be the largest histogram cell, so all other translations can be
        // skipped exactly.
        if (input[flip] != maximumCell) continue;
        for (const PatternMap& map : permutationMaps) {
            int comparison = 0;
            for (int p = 0; p < patterns; ++p) {
                const uint8_t value = input[flip ^ map[p]];
                if (!have || value > result.representative[p]) {
                    comparison = 1;
                    break;
                }
                if (value < result.representative[p]) {
                    comparison = -1;
                    break;
                }
            }
            if (!have || comparison > 0) {
                for (int p = 0; p < patterns; ++p) {
                    result.representative[p] = input[flip ^ map[p]];
                }
                result.mapsToRepresentative = 1;
                have = true;
            } else if (comparison == 0) {
                ++result.mapsToRepresentative;
            }
        }
    }
    if (!have || result.mapsToRepresentative == 0) {
        throw std::runtime_error("histogram canonicalization failed");
    }
    return result;
}

void generateBalancedSplits(const Hist& parent, int dimensions,
                            const std::vector<PatternMap>& childPermutationMaps,
                            std::unordered_map<Hist, uint32_t, HistHash>& next,
                            uint64_t& rawChildren) {
    const int parentPatterns = 1 << dimensions;
    Hist selected{};
    std::array<int, MAX_PATTERNS + 1> remainingCapacity{};
    for (int p = parentPatterns - 1; p >= 0; --p) {
        remainingCapacity[p] = remainingCapacity[p + 1] + parent[p];
    }

    std::function<void(int, int)> chooseSplit = [&](int p, int remaining) {
        if (p == parentPatterns) {
            if (remaining != 0) return;
            // Flipping the new coordinate exchanges selected and unselected.
            // Generate one orientation of that pair.
            bool selectedIsCanonicalOrientation = true;
            for (int q = 0; q < parentPatterns; ++q) {
                const uint8_t complement = parent[q] - selected[q];
                if (selected[q] < complement) break;
                if (selected[q] > complement) {
                    selectedIsCanonicalOrientation = false;
                    break;
                }
            }
            if (!selectedIsCanonicalOrientation) return;

            Hist child{};
            for (int q = 0; q < parentPatterns; ++q) {
                child[q] = parent[q] - selected[q];
                child[q | parentPatterns] = selected[q];
            }
            ++rawChildren;
            HistCanon canonical = canonicalizeHistogram(child, dimensions + 1,
                                                        childPermutationMaps);
            next.emplace(canonical.representative, canonical.mapsToRepresentative);
            return;
        }
        if (remaining < 0 || remaining > remainingCapacity[p]) return;
        const int upper = std::min<int>(parent[p], remaining);
        for (int take = 0; take <= upper; ++take) {
            selected[p] = (uint8_t)take;
            chooseSplit(p + 1, remaining - take);
        }
        selected[p] = 0;
    };
    chooseSplit(0, M / 2);
}

std::vector<OuterClass> enumerateOuterClassesByAugmentation() {
    Hist initial{};
    initial[0] = (uint8_t)M;
    std::vector<Hist> states{initial};
    std::unordered_map<Hist, uint32_t, HistHash> finalMap;

    for (int dimensions = 0; dimensions < C; ++dimensions) {
        const auto maps = coordinatePermutationMaps(dimensions + 1);
        std::unordered_map<Hist, uint32_t, HistHash> next;
        next.reserve(dimensions + 1 == C ? 100000 : states.size() * 16 + 16);
        uint64_t rawChildren = 0;
        const auto started = std::chrono::steady_clock::now();
        for (const Hist& parent : states) {
            generateBalancedSplits(parent, dimensions, maps, next, rawChildren);
        }
        const double seconds = std::chrono::duration<double>(
            std::chrono::steady_clock::now() - started).count();
        std::fprintf(stderr,
                     "outer augmentation dimension=%d parents=%zu rawChildren=%llu orbits=%zu time=%.3fs\n",
                     dimensions + 1, states.size(),
                     (unsigned long long)rawChildren, next.size(), seconds);
        states.clear();
        states.reserve(next.size());
        for (const auto& entry : next) states.push_back(entry.first);
        if (dimensions + 1 == C) finalMap = std::move(next);
    }

    std::vector<OuterClass> classes;
    classes.reserve(finalMap.size());
    const uint64_t signedCoordinateGroup = (uint64_t)(1u << C) * factorial(C);
    for (const auto& entry : finalMap) {
        const Hist& representative = entry.first;
        const uint32_t stabilizer = entry.second;
        if (stabilizer == 0 || signedCoordinateGroup % stabilizer != 0) {
            throw std::runtime_error("invalid histogram stabilizer");
        }
        const uint64_t orbitSize = signedCoordinateGroup / stabilizer;
        uint64_t symbolLabellings = factorial(M);
        for (int p = 0; p < NPAT; ++p) symbolLabellings /= factorial(representative[p]);
        const unsigned __int128 multiplicity =
            (unsigned __int128)orbitSize * symbolLabellings;
        if (multiplicity > std::numeric_limits<uint64_t>::max()) {
            throw std::overflow_error("outer orbit multiplicity does not fit uint64_t");
        }
        classes.push_back({representative, orbitSize, (uint64_t)multiplicity, 0});
    }
    std::sort(classes.begin(), classes.end(), [](const OuterClass& a, const OuterClass& b) {
        return histLess(a.representative, b.representative);
    });

    unsigned __int128 multiplicitySum = 0;
    for (const OuterClass& oc : classes) multiplicitySum += oc.labelledMultiplicity;
    const unsigned __int128 expected = ipow(choose(M, C), C);
    std::fprintf(stderr, "outer classes=%zu multiplicity sum=%s expected=%s [%s]\n",
                 classes.size(), u128ToString(multiplicitySum).c_str(),
                 u128ToString(expected).c_str(), multiplicitySum == expected ? "OK" : "MISMATCH");
    if (multiplicitySum != expected) throw std::runtime_error("outer orbit multiplicities failed");
    if (C == 6 && classes.size() != 63199) {
        throw std::runtime_error("C=6 outer orbit class count failed");
    }
    return classes;
}

std::vector<Vec> makeCountVectors() {
    const int psmall = 1 << (C - 1);
    std::vector<Vec> result;
    Vec cur;
    std::function<void(int, int)> generate = [&](int pos, int remaining) {
        if (pos == psmall - 1) {
            cur.counts[pos] = (uint8_t)remaining;
            uint64_t marginal = 0;
            for (int b = 0; b < C - 1; ++b) {
                int count = 0;
                for (int p = 0; p < psmall; ++p) {
                    if ((p >> b) & 1) count += cur.counts[p];
                }
                marginal = marginal * (uint64_t)(C + 1) + (uint64_t)count;
            }
            cur.marginal = marginal;
            result.push_back(cur);
            return;
        }
        for (int v = 0; v <= remaining; ++v) {
            cur.counts[pos] = (uint8_t)v;
            generate(pos + 1, remaining - v);
        }
    };
    generate(0, C);
    return result;
}

std::vector<OuterClass> enumerateOuterClasses() {
    if (C == 6) return enumerateOuterClassesByAugmentation();
    const auto vecs = makeCountVectors();
    std::unordered_map<uint64_t, std::vector<int>> byMarginal;
    byMarginal.reserve(vecs.size());
    for (int i = 0; i < (int)vecs.size(); ++i) byMarginal[vecs[i].marginal].push_back(i);

    uint64_t fullMarginal = 0;
    for (int b = 0; b < C - 1; ++b) fullMarginal = fullMarginal * (uint64_t)(C + 1) + (uint64_t)C;

    std::unordered_map<Hist, int, HistHash> orbitOf;
    orbitOf.reserve(C == 5 ? 700000 : 10000);
    std::vector<OuterClass> classes;
    uint64_t rawHistograms = 0;

    const int psmall = 1 << (C - 1);
    for (const Vec& top : vecs) {
        const uint64_t need = fullMarginal - top.marginal;
        auto found = byMarginal.find(need);
        if (found == byMarginal.end()) continue;
        for (int bottomIndex : found->second) {
            const Vec& bottom = vecs[bottomIndex];
            Hist h{};
            for (int p = 0; p < psmall; ++p) {
                h[1 | (p << 1)] = top.counts[p];
                h[p << 1] = bottom.counts[p];
            }
            ++rawHistograms;
            if (orbitOf.find(h) != orbitOf.end()) continue;

            const int orbitId = (int)classes.size();
            std::vector<Hist> queue;
            queue.reserve((size_t)(1 << C) * (size_t)factorial(C));
            orbitOf.emplace(h, orbitId);
            queue.push_back(h);
            Hist representative = h;

            for (size_t head = 0; head < queue.size(); ++head) {
                const Hist cur = queue[head];
                if (histLess(cur, representative)) representative = cur;
                for (int b = 0; b < C; ++b) {
                    Hist next = flipCoordinate(cur, b);
                    if (orbitOf.emplace(next, orbitId).second) queue.push_back(next);
                }
                for (int b = 0; b + 1 < C; ++b) {
                    Hist next = swapCoordinates(cur, b);
                    if (orbitOf.emplace(next, orbitId).second) queue.push_back(next);
                }
            }

            uint64_t symbolLabellings = factorial(M);
            for (int p = 0; p < NPAT; ++p) symbolLabellings /= factorial(representative[p]);
            const unsigned __int128 multiplicity =
                (unsigned __int128)symbolLabellings * (uint64_t)queue.size();
            if (multiplicity > std::numeric_limits<uint64_t>::max()) {
                throw std::overflow_error("outer orbit multiplicity does not fit uint64_t");
            }
            classes.push_back({representative, (uint64_t)queue.size(),
                               (uint64_t)multiplicity, 0});
        }
    }

    unsigned __int128 multiplicitySum = 0;
    for (const OuterClass& oc : classes) multiplicitySum += oc.labelledMultiplicity;
    const unsigned __int128 expected = ipow(choose(M, C), C);
    std::fprintf(stderr,
                 "outer C=%d countVectors=%zu rawHistograms=%llu mappedHistograms=%zu classes=%zu\n",
                 C, vecs.size(), (unsigned long long)rawHistograms, orbitOf.size(), classes.size());
    std::fprintf(stderr, "outer multiplicity sum=%s expected=%s [%s]\n",
                 u128ToString(multiplicitySum).c_str(),
                 u128ToString(expected).c_str(),
                 multiplicitySum == expected ? "OK" : "MISMATCH");
    if (multiplicitySum != expected) throw std::runtime_error("outer orbit multiplicities failed");

    const int expectedClasses = C == 2 ? 2 : C == 3 ? 4 : C == 4 ? 26 : C == 5 ? 355 : C == 6 ? 63199 : -1;
    if (expectedClasses >= 0 && (int)classes.size() != expectedClasses) {
        throw std::runtime_error("outer orbit class count failed");
    }
    return classes;
}

struct GraphKey {
    // M^2 is 144 bits at C=6.  Store the canonical adjacency bitstring in
    // lexicographic (most-significant-word first) order with ample headroom.
    std::array<uint64_t, 3> words{};
    uint8_t keyKind = 0; // 0: strong canonical key / raw cache key, 1: safe weak fallback
    bool operator==(const GraphKey&) const = default;
    bool operator<(const GraphKey& other) const noexcept {
        if (keyKind != other.keyKind) return keyKind < other.keyKind;
        return words < other.words;
    }
};

struct GraphKeyHash {
    size_t operator()(const GraphKey& k) const noexcept {
        uint64_t x = 0x9e3779b97f4a7c15ULL ^ k.keyKind;
        for (uint64_t word : k.words) {
            x ^= word + 0x9e3779b97f4a7c15ULL + (x << 6) + (x >> 2);
            x ^= x >> 30;
            x *= 0xbf58476d1ce4e5b9ULL;
        }
        return (size_t)x;
    }
};

void appendKeyBit(GraphKey& key, int& position, unsigned bit) {
    if (bit) key.words[position / 64] |= (uint64_t)1 << (63 - position % 64);
    ++position;
}

void appendKeyValue(GraphKey& key, int& position, uint16_t value, int width) {
    for (int bit = width - 1; bit >= 0; --bit) appendKeyBit(key, position, (value >> bit) & 1u);
}

GraphKey weakGraphKey(const std::array<uint16_t, MAX_M>& graph) {
    std::array<uint16_t, MAX_M> rows = graph;
    // M is runtime-configured, so use a visibly bounded insertion sort to keep
    // GCC's array-bounds analysis honest for MAX_M=12.
    for (int i = 1; i < MAX_M && i < M; ++i) {
        const uint16_t value = rows[i];
        int j = i;
        while (j > 0 && rows[j - 1] > value) {
            rows[j] = rows[j - 1];
            --j;
        }
        rows[j] = value;
    }
    GraphKey key;
    int position = 0;
    for (int i = 0; i < M; ++i) appendKeyValue(key, position, rows[i], M);
    return key;
}

struct CanonState {
    std::array<uint8_t, MAX_M> rowColor{};
    std::array<uint8_t, MAX_M> colColor{};
    int rowColors = 1;
    int colColors = 1;
};

uint64_t canonSearchNodes = 0;
uint64_t canonComputations = 0;
uint64_t canonCacheHits = 0;
uint64_t canonFallbacks = 0;
uint64_t canonNodeBudget = 20000;
bool verboseProbeProgress = false;
size_t canonCacheCapPerDegree = 0; // zero means unbounded
size_t degree4ParentChunk = 128;
std::array<size_t, MAX_C + 1> requestedCanonCacheCaps{};
uint64_t canonCacheEvictions = 0;

bool refineSide(const std::array<uint16_t, MAX_M>& graph,
                std::array<uint8_t, MAX_M>& ownColor,
                int& ownColors,
                const std::array<uint8_t, MAX_M>& otherColor,
                int otherColors,
                bool rows) {
    std::array<uint8_t, MAX_M> next{};
    int nextColors = 0;
    bool changed = false;
    for (int old = 0; old < ownColors; ++old) {
        struct Item {
            int vertex = 0;
            std::array<uint8_t, MAX_M> signature{};
        };
        std::array<Item, MAX_M> items{};
        int itemCount = 0;
        for (int v = 0; v < M; ++v) {
            if (ownColor[v] != old) continue;
            Item item;
            item.vertex = v;
            if (rows) {
                uint16_t bits = graph[v];
                while (bits) {
                    const int u = std::countr_zero(bits);
                    bits &= (uint16_t)(bits - 1);
                    ++item.signature[otherColor[u]];
                }
            } else {
                for (int u = 0; u < M; ++u) {
                    if ((graph[u] >> v) & 1u) ++item.signature[otherColor[u]];
                }
            }
            items[itemCount++] = item;
        }
        std::sort(items.begin(), items.begin() + itemCount,
                  [&](const Item& a, const Item& b) {
                      for (int i = 0; i < otherColors; ++i) {
                          if (a.signature[i] != b.signature[i]) return a.signature[i] < b.signature[i];
                      }
                      return a.vertex < b.vertex;
                  });
        int groupColor = nextColors++;
        for (int i = 0; i < itemCount; ++i) {
            if (i > 0) {
                bool different = false;
                for (int j = 0; j < otherColors; ++j) {
                    if (items[i - 1].signature[j] != items[i].signature[j]) {
                        different = true;
                        break;
                    }
                }
                if (different) groupColor = nextColors++;
            }
            next[items[i].vertex] = (uint8_t)groupColor;
        }
    }
    if (nextColors != ownColors) changed = true;
    else {
        for (int v = 0; v < M; ++v) {
            if (next[v] != ownColor[v]) {
                changed = true;
                break;
            }
        }
    }
    ownColor = next;
    ownColors = nextColors;
    return changed;
}

void refine(const std::array<uint16_t, MAX_M>& graph, CanonState& state) {
    for (;;) {
        bool changed = false;
        changed |= refineSide(graph, state.rowColor, state.rowColors,
                              state.colColor, state.colColors, true);
        changed |= refineSide(graph, state.colColor, state.colColors,
                              state.rowColor, state.rowColors, false);
        if (!changed) break;
    }
}

void individualize(std::array<uint8_t, MAX_M>& colors, int& colorCount,
                   int selectedColor, int selectedVertex) {
    for (int v = 0; v < M; ++v) {
        if (colors[v] > selectedColor) ++colors[v];
        else if (colors[v] == selectedColor) {
            colors[v] = (uint8_t)(v == selectedVertex ? selectedColor : selectedColor + 1);
        }
    }
    ++colorCount;
}

GraphKey encodeDiscrete(const std::array<uint16_t, MAX_M>& graph,
                        const CanonState& state) {
    std::array<int, MAX_M> rowAt{};
    std::array<int, MAX_M> colAt{};
    for (int v = 0; v < M; ++v) {
        rowAt[state.rowColor[v]] = v;
        colAt[state.colColor[v]] = v;
    }
    GraphKey code;
    int position = 0;
    for (int r = 0; r < M; ++r) {
        const int originalRow = rowAt[r];
        for (int c = 0; c < M; ++c) {
            appendKeyBit(code, position, (graph[originalRow] >> colAt[c]) & 1u);
        }
    }
    return code;
}

void canonicalSearch(const std::array<uint16_t, MAX_M>& graph,
                     CanonState state,
                     GraphKey& best,
                     bool& haveBest,
                     uint64_t& nodesThisCall,
                     bool& aborted) {
    if (aborted) return;
    if (++nodesThisCall > canonNodeBudget) {
        aborted = true;
        return;
    }
    refine(graph, state);
    if (state.rowColors == M && state.colColors == M) {
        const GraphKey code = encodeDiscrete(graph, state);
        if (!haveBest || code < best) {
            best = code;
            haveBest = true;
        }
        return;
    }

    bool chooseRows = true;
    int selectedColor = -1;
    int selectedSize = M + 1;
    auto consider = [&](const std::array<uint8_t, MAX_M>& colors,
                        int colorCount, bool rows) {
        std::array<int, MAX_M> sizes{};
        for (int v = 0; v < M; ++v) ++sizes[colors[v]];
        for (int color = 0; color < colorCount; ++color) {
            const int size = sizes[color];
            if (size <= 1) continue;
            if (size < selectedSize ||
                (size == selectedSize && rows && !chooseRows) ||
                (size == selectedSize && rows == chooseRows && color < selectedColor)) {
                selectedSize = size;
                selectedColor = color;
                chooseRows = rows;
            }
        }
    };
    consider(state.rowColor, state.rowColors, true);
    consider(state.colColor, state.colColors, false);

    const auto& colors = chooseRows ? state.rowColor : state.colColor;
    std::array<uint16_t, MAX_M> seenNeighborhoods{};
    int seenCount = 0;
    for (int vertex = 0; vertex < M; ++vertex) {
        if (colors[vertex] != selectedColor) continue;
        uint16_t neighborhood = 0;
        if (chooseRows) neighborhood = graph[vertex];
        else {
            for (int row = 0; row < M; ++row) {
                if ((graph[row] >> vertex) & 1u) neighborhood |= (uint16_t)(1u << row);
            }
        }
        bool duplicate = false;
        for (int i = 0; i < seenCount; ++i) {
            if (seenNeighborhoods[i] == neighborhood) {
                duplicate = true;
                break;
            }
        }
        if (duplicate) continue;
        seenNeighborhoods[seenCount++] = neighborhood;
        CanonState child = state;
        if (chooseRows) individualize(child.rowColor, child.rowColors, selectedColor, vertex);
        else individualize(child.colColor, child.colColors, selectedColor, vertex);
        canonicalSearch(graph, child, best, haveBest, nodesThisCall, aborted);
        if (aborted) return;
    }
}

using CanonMap = std::unordered_map<GraphKey, GraphKey, GraphKeyHash>;
struct DegreeCanonCache {
    CanonMap map;
    std::deque<GraphKey> fifo;
    size_t cap = 0;
};
std::array<DegreeCanonCache, MAX_C + 1> canonCacheByDegree;

int regularGraphDegree(const std::array<uint16_t, MAX_M>& graph) {
    return std::popcount(graph[0]);
}

// Degree-3 residuals dominate the C=6 layered evaluator.  Keep a cheap,
// label-invariant bucket for them, then prove equivalence with a two-graph
// individualization/refinement search.  A successful isomorphism test can stop
// at its first mapping; only genuinely new classes need a canonical label.
struct Degree3Invariant {
    uint64_t first = 0;
    uint64_t second = 0;
    bool operator==(const Degree3Invariant&) const = default;
};

struct Degree3InvariantHash {
    size_t operator()(const Degree3Invariant& value) const noexcept {
        uint64_t x = value.first ^ std::rotl(value.second, 23);
        x ^= x >> 30;
        x *= 0xbf58476d1ce4e5b9ULL;
        x ^= x >> 27;
        return (size_t)x;
    }
};

struct Degree3IsoGraph {
    std::array<std::array<uint8_t, 3>, 2 * MAX_M> neighbors{};
    std::array<std::array<uint16_t, MAX_M>, 2> sideMasks{};
    std::array<uint32_t, 2 * MAX_M> vertexProfiles{};
    std::array<uint8_t, 2 * MAX_M> structuralColors{};
    uint8_t structuralColorCount = 0;
};

struct Degree3StructuralSignature {
    uint8_t own = 0;
    std::array<uint16_t, MAX_M - 1> sameSide{};
    std::array<uint8_t, 3> opposite{};
    bool operator==(const Degree3StructuralSignature&) const = default;
    bool operator<(const Degree3StructuralSignature& other) const noexcept {
        if (own != other.own) return own < other.own;
        if (sameSide != other.sameSide) return sameSide < other.sameSide;
        return opposite < other.opposite;
    }
};

struct Degree3IsoRepresentative {
    std::array<uint16_t, MAX_M> graph{};
    Degree3IsoGraph prepared{};
    GraphKey key{};
};

std::unordered_map<Degree3Invariant, std::vector<Degree3IsoRepresentative>,
                   Degree3InvariantHash> degree3IsoBuckets;
std::unordered_map<GraphKey, GraphKey, GraphKeyHash> degree3DiscreteCanonMap;
bool useDegree3IsoBatch = false;
bool useDegree3PairIso = false;
bool parallelDegree4ParentEnumeration = false;
bool useDegree4RootedSplit = false;
int colorPivotMaxDegree = 0;
std::array<int, MAX_C + 1> rootedTwoFactorProbes{};
uint64_t degree4RootedCalls = 0;
uint64_t degree4RootedLeaves = 0;
uint64_t degree4RootedNodes = 0;
uint64_t degree3IsoNodeBudget = 1000;
uint64_t degree3IsoChecks = 0;
uint64_t degree3IsoHits = 0;
uint64_t degree3IsoUnknown = 0;
uint64_t degree3IsoNodes = 0;
uint64_t degree3IsoRepresentatives = 0;
uint64_t degree3IsoKnownKeyMisses = 0;
uint64_t degree3DiscreteInputs = 0;
uint64_t degree3DiscreteHits = 0;
uint64_t degree3DiscreteClasses = 0;

void refineDegree3Structure(Degree3IsoGraph& graph);

Degree3IsoGraph prepareDegree3IsoGraph(const std::array<uint16_t, MAX_M>& graph) {
    Degree3IsoGraph prepared;
    for (int row = 0; row < M; ++row) {
        prepared.sideMasks[0][row] = graph[row];
        uint16_t bits = graph[row];
        int position = 0;
        while (bits) {
            const int col = std::countr_zero(bits);
            bits &= (uint16_t)(bits - 1);
            prepared.neighbors[row][position++] = (uint8_t)(M + col);
            prepared.sideMasks[1][col] |= (uint16_t)(1u << row);
        }
    }
    for (int col = 0; col < M; ++col) {
        uint16_t bits = prepared.sideMasks[1][col];
        int position = 0;
        while (bits) {
            const int row = std::countr_zero(bits);
            bits &= (uint16_t)(bits - 1);
            prepared.neighbors[M + col][position++] = (uint8_t)row;
        }
    }

    for (int side = 0; side < 2; ++side) {
        for (int vertex = 0; vertex < M; ++vertex) {
            std::array<uint8_t, 4> commonCounts{};
            for (int other = 0; other < M; ++other) {
                if (other == vertex) continue;
                const int common = std::popcount((uint16_t)(
                    prepared.sideMasks[side][vertex] & prepared.sideMasks[side][other]));
                ++commonCounts[common];
            }

            std::array<uint8_t, 3> edgeFourCycles{};
            for (int edge = 0; edge < 3; ++edge) {
                const int opposite = side == 0
                    ? prepared.neighbors[vertex][edge] - M
                    : prepared.neighbors[M + vertex][edge];
                int cycles = 0;
                for (int other = 0; other < M; ++other) {
                    if (other == vertex ||
                        ((prepared.sideMasks[side][other] >> opposite) & 1u) == 0) {
                        continue;
                    }
                    cycles += std::popcount((uint16_t)(prepared.sideMasks[side][vertex] &
                                                       prepared.sideMasks[side][other])) - 1;
                }
                edgeFourCycles[edge] = (uint8_t)cycles;
            }
            std::sort(edgeFourCycles.begin(), edgeFourCycles.end());

            uint32_t profile = (uint32_t)side << 31;
            for (int common = 0; common <= 3; ++common) {
                profile |= (uint32_t)commonCounts[common] << (4 * common);
            }
            for (int edge = 0; edge < 3; ++edge) {
                profile |= (uint32_t)edgeFourCycles[edge] << (16 + 3 * edge);
            }
            prepared.vertexProfiles[side * M + vertex] = profile;
        }
    }
    refineDegree3Structure(prepared);
    return prepared;
}

void refineDegree3Structure(Degree3IsoGraph& graph) {
    std::array<uint32_t, 2 * MAX_M> profiles = graph.vertexProfiles;
    std::sort(profiles.begin(), profiles.begin() + 2 * M);
    std::array<uint32_t, 2 * MAX_M> uniqueProfiles{};
    int uniqueProfileCount = 0;
    for (int i = 0; i < 2 * M; ++i) {
        if (i == 0 || profiles[i] != profiles[i - 1]) {
            uniqueProfiles[uniqueProfileCount++] = profiles[i];
        }
    }
    for (int vertex = 0; vertex < 2 * M; ++vertex) {
        graph.structuralColors[vertex] = (uint8_t)(std::lower_bound(
            uniqueProfiles.begin(), uniqueProfiles.begin() + uniqueProfileCount,
            graph.vertexProfiles[vertex]) - uniqueProfiles.begin());
    }
    graph.structuralColorCount = (uint8_t)uniqueProfileCount;

    for (;;) {
        auto makeSignature = [&](int vertex) {
            Degree3StructuralSignature signature;
            signature.own = graph.structuralColors[vertex];
            signature.sameSide.fill(std::numeric_limits<uint16_t>::max());
            const int side = vertex >= M ? 1 : 0;
            const int local = side == 0 ? vertex : vertex - M;
            int position = 0;
            for (int other = 0; other < M; ++other) {
                if (other == local) continue;
                const int common = std::popcount((uint16_t)(
                    graph.sideMasks[side][local] & graph.sideMasks[side][other]));
                signature.sameSide[position++] = (uint16_t)(
                    (common << 8) | graph.structuralColors[side * M + other]);
            }
            std::sort(signature.sameSide.begin(), signature.sameSide.end());
            for (int edge = 0; edge < 3; ++edge) {
                signature.opposite[edge] = graph.structuralColors[
                    graph.neighbors[vertex][edge]];
            }
            std::sort(signature.opposite.begin(), signature.opposite.end());
            return signature;
        };

        std::array<Degree3StructuralSignature, 2 * MAX_M> signatures{};
        for (int vertex = 0; vertex < 2 * M; ++vertex) {
            signatures[vertex] = makeSignature(vertex);
        }
        std::sort(signatures.begin(), signatures.begin() + 2 * M);
        std::array<Degree3StructuralSignature, 2 * MAX_M> unique{};
        int uniqueCount = 0;
        for (int i = 0; i < 2 * M; ++i) {
            if (i == 0 || !(signatures[i] == signatures[i - 1])) {
                unique[uniqueCount++] = signatures[i];
            }
        }

        std::array<uint8_t, 2 * MAX_M> next{};
        for (int vertex = 0; vertex < 2 * M; ++vertex) {
            const Degree3StructuralSignature signature = makeSignature(vertex);
            next[vertex] = (uint8_t)(std::lower_bound(
                unique.begin(), unique.begin() + uniqueCount, signature) - unique.begin());
        }
        const bool stable = next == graph.structuralColors;
        graph.structuralColors = next;
        graph.structuralColorCount = (uint8_t)uniqueCount;
        if (stable) return;
    }
}

GraphKey degree3DiscreteKey(const Degree3IsoGraph& graph) {
    std::array<int, MAX_M> rowOrder{};
    std::array<int, MAX_M> colOrder{};
    for (int vertex = 0; vertex < M; ++vertex) {
        rowOrder[vertex] = vertex;
        colOrder[vertex] = vertex;
    }
    std::sort(rowOrder.begin(), rowOrder.begin() + M,
              [&](int a, int b) {
                  return graph.structuralColors[a] < graph.structuralColors[b];
              });
    std::sort(colOrder.begin(), colOrder.begin() + M,
              [&](int a, int b) {
                  return graph.structuralColors[M + a] < graph.structuralColors[M + b];
              });
    GraphKey key;
    key.keyKind = 2; // exact discrete structural label; map it to the legacy strong namespace
    int position = 0;
    for (int rowPosition = 0; rowPosition < M; ++rowPosition) {
        const uint16_t row = graph.sideMasks[0][rowOrder[rowPosition]];
        for (int colPosition = 0; colPosition < M; ++colPosition) {
            appendKeyBit(key, position, (row >> colOrder[colPosition]) & 1u);
        }
    }
    return key;
}

Degree3Invariant degree3Invariant(const Degree3IsoGraph& graph) {
    uint64_t first = 0x243f6a8885a308d3ULL;
    uint64_t second = 0x13198a2e03707344ULL;
    auto feed = [&](uint64_t value) {
        value += 0x9e3779b97f4a7c15ULL;
        value = (value ^ (value >> 30)) * 0xbf58476d1ce4e5b9ULL;
        value = (value ^ (value >> 27)) * 0x94d049bb133111ebULL;
        value ^= value >> 31;
        first = std::rotl(first ^ value, 19) * 0x9e3779b185ebca87ULL;
        second = std::rotl(second + value, 31) * 0xc2b2ae3d27d4eb4fULL;
    };

    for (int side = 0; side < 2; ++side) {
        std::array<uint32_t, MAX_M> profiles{};
        for (int vertex = 0; vertex < M; ++vertex) {
            profiles[vertex] = graph.vertexProfiles[side * M + vertex];
        }
        std::sort(profiles.begin(), profiles.begin() + M);
        feed(0x100 + side);
        for (int vertex = 0; vertex < M; ++vertex) feed(profiles[vertex]);

        std::array<uint8_t, MAX_M * (MAX_M - 1) / 2> intersections{};
        int count = 0;
        for (int a = 0; a < M; ++a) {
            for (int b = a + 1; b < M; ++b) {
                intersections[count++] = (uint8_t)std::popcount((uint16_t)(
                    graph.sideMasks[side][a] & graph.sideMasks[side][b]));
            }
        }
        std::sort(intersections.begin(), intersections.begin() + count);
        feed(0x200 + side);
        for (int i = 0; i < count; ++i) feed(intersections[i]);
    }

    struct EdgeFeature {
        uint32_t rowProfile = 0;
        uint32_t colProfile = 0;
        uint8_t fourCycles = 0;
        bool operator<(const EdgeFeature& other) const noexcept {
            if (rowProfile != other.rowProfile) return rowProfile < other.rowProfile;
            if (colProfile != other.colProfile) return colProfile < other.colProfile;
            return fourCycles < other.fourCycles;
        }
    };
    std::array<EdgeFeature, 3 * MAX_M> edges{};
    int edgeCount = 0;
    for (int row = 0; row < M; ++row) {
        for (int edge = 0; edge < 3; ++edge) {
            const int col = graph.neighbors[row][edge] - M;
            int fourCycles = 0;
            for (int other = 0; other < M; ++other) {
                if (other == row || ((graph.sideMasks[0][other] >> col) & 1u) == 0) continue;
                fourCycles += std::popcount((uint16_t)(graph.sideMasks[0][row] &
                                                       graph.sideMasks[0][other])) - 1;
            }
            edges[edgeCount++] = {graph.vertexProfiles[row],
                                  graph.vertexProfiles[M + col],
                                  (uint8_t)fourCycles};
        }
    }
    std::sort(edges.begin(), edges.begin() + edgeCount);
    feed(0x300);
    for (int i = 0; i < edgeCount; ++i) {
        feed(((uint64_t)edges[i].rowProfile << 32) | edges[i].colProfile);
        feed(edges[i].fourCycles);
    }

    std::array<uint8_t, 2 * MAX_M> seen{};
    std::array<uint8_t, 2 * MAX_M> componentSizes{};
    int componentCount = 0;
    for (int start = 0; start < 2 * M; ++start) {
        if (seen[start]) continue;
        std::array<uint8_t, 2 * MAX_M> queue{};
        int begin = 0, end = 0;
        queue[end++] = (uint8_t)start;
        seen[start] = 1;
        while (begin < end) {
            const int vertex = queue[begin++];
            for (int edge = 0; edge < 3; ++edge) {
                const int next = graph.neighbors[vertex][edge];
                if (!seen[next]) {
                    seen[next] = 1;
                    queue[end++] = (uint8_t)next;
                }
            }
        }
        componentSizes[componentCount++] = (uint8_t)end;
    }
    std::sort(componentSizes.begin(), componentSizes.begin() + componentCount);
    feed(0x400);
    for (int i = 0; i < componentCount; ++i) feed(componentSizes[i]);

    feed(0x500);
    feed(graph.structuralColorCount);
    std::array<uint64_t, 2 * MAX_M> coloredVertices{};
    for (int vertex = 0; vertex < 2 * M; ++vertex) {
        coloredVertices[vertex] = ((uint64_t)graph.structuralColors[vertex] << 32) |
                                  graph.vertexProfiles[vertex];
    }
    std::sort(coloredVertices.begin(), coloredVertices.begin() + 2 * M);
    for (int vertex = 0; vertex < 2 * M; ++vertex) feed(coloredVertices[vertex]);

    std::array<uint32_t, 3 * MAX_M> coloredEdges{};
    int coloredEdgeCount = 0;
    for (int row = 0; row < M; ++row) {
        for (int edge = 0; edge < 3; ++edge) {
            const int col = graph.neighbors[row][edge] - M;
            coloredEdges[coloredEdgeCount++] =
                ((uint32_t)graph.structuralColors[row] << 8) |
                graph.structuralColors[M + col];
        }
    }
    std::sort(coloredEdges.begin(), coloredEdges.begin() + coloredEdgeCount);
    for (int i = 0; i < coloredEdgeCount; ++i) feed(coloredEdges[i]);

    for (int side = 0; side < 2; ++side) {
        std::array<uint32_t, MAX_M * (MAX_M - 1) / 2> coloredPairs{};
        int pairCount = 0;
        for (int a = 0; a < M; ++a) {
            for (int b = a + 1; b < M; ++b) {
                const uint8_t firstColor = std::min(
                    graph.structuralColors[side * M + a],
                    graph.structuralColors[side * M + b]);
                const uint8_t secondColor = std::max(
                    graph.structuralColors[side * M + a],
                    graph.structuralColors[side * M + b]);
                const uint8_t common = (uint8_t)std::popcount((uint16_t)(
                    graph.sideMasks[side][a] & graph.sideMasks[side][b]));
                coloredPairs[pairCount++] = ((uint32_t)firstColor << 16) |
                                            ((uint32_t)secondColor << 8) | common;
            }
        }
        std::sort(coloredPairs.begin(), coloredPairs.begin() + pairCount);
        feed(0x510 + side);
        for (int i = 0; i < pairCount; ++i) feed(coloredPairs[i]);
    }
    return {first, second};
}

struct Degree3IsoColorSignature {
    uint8_t own = 0;
    std::array<uint8_t, 3> neighbors{};
    bool operator==(const Degree3IsoColorSignature&) const = default;
    bool operator<(const Degree3IsoColorSignature& other) const noexcept {
        if (own != other.own) return own < other.own;
        return neighbors < other.neighbors;
    }
};

bool refineDegree3Iso(const Degree3IsoGraph& firstGraph,
                      const Degree3IsoGraph& secondGraph,
                      std::array<uint8_t, 2 * MAX_M>& firstColors,
                      std::array<uint8_t, 2 * MAX_M>& secondColors,
                      int& colorCount) {
    for (;;) {
        std::array<Degree3IsoColorSignature, 4 * MAX_M> signatures{};
        auto makeSignature = [](const Degree3IsoGraph& graph,
                                const std::array<uint8_t, 2 * MAX_M>& colors,
                                int vertex) {
            Degree3IsoColorSignature signature;
            signature.own = colors[vertex];
            for (int edge = 0; edge < 3; ++edge) {
                signature.neighbors[edge] = colors[graph.neighbors[vertex][edge]];
            }
            std::sort(signature.neighbors.begin(), signature.neighbors.end());
            return signature;
        };
        for (int vertex = 0; vertex < 2 * M; ++vertex) {
            signatures[vertex] = makeSignature(firstGraph, firstColors, vertex);
            signatures[2 * M + vertex] = makeSignature(secondGraph, secondColors, vertex);
        }
        std::sort(signatures.begin(), signatures.begin() + 4 * M);
        std::array<Degree3IsoColorSignature, 4 * MAX_M> unique{};
        int uniqueCount = 0;
        for (int i = 0; i < 4 * M; ++i) {
            if (i == 0 || !(signatures[i] == signatures[i - 1])) {
                unique[uniqueCount++] = signatures[i];
            }
        }

        std::array<uint8_t, 2 * MAX_M> nextFirst{};
        std::array<uint8_t, 2 * MAX_M> nextSecond{};
        std::array<uint8_t, 4 * MAX_M> firstSizes{};
        std::array<uint8_t, 4 * MAX_M> secondSizes{};
        for (int vertex = 0; vertex < 2 * M; ++vertex) {
            const auto firstSignature = makeSignature(firstGraph, firstColors, vertex);
            const auto secondSignature = makeSignature(secondGraph, secondColors, vertex);
            nextFirst[vertex] = (uint8_t)(std::lower_bound(
                unique.begin(), unique.begin() + uniqueCount, firstSignature) - unique.begin());
            nextSecond[vertex] = (uint8_t)(std::lower_bound(
                unique.begin(), unique.begin() + uniqueCount, secondSignature) - unique.begin());
            ++firstSizes[nextFirst[vertex]];
            ++secondSizes[nextSecond[vertex]];
        }
        for (int color = 0; color < uniqueCount; ++color) {
            if (firstSizes[color] != secondSizes[color]) return false;
        }
        const bool stable = nextFirst == firstColors && nextSecond == secondColors;
        firstColors = nextFirst;
        secondColors = nextSecond;
        colorCount = uniqueCount;
        if (stable) return true;
    }
}

enum class Degree3IsoResult : uint8_t { no, yes, unknown };

bool verifyDegree3IsoMapping(const Degree3IsoGraph& firstGraph,
                             const Degree3IsoGraph& secondGraph,
                             const std::array<uint8_t, 2 * MAX_M>& firstColors,
                             const std::array<uint8_t, 2 * MAX_M>& secondColors) {
    std::array<int, 2 * MAX_M> mapped{};
    mapped.fill(-1);
    for (int firstVertex = 0; firstVertex < 2 * M; ++firstVertex) {
        for (int secondVertex = 0; secondVertex < 2 * M; ++secondVertex) {
            if (firstColors[firstVertex] == secondColors[secondVertex]) {
                mapped[firstVertex] = secondVertex;
                break;
            }
        }
        if (mapped[firstVertex] < 0) return false;
    }
    for (int firstVertex = 0; firstVertex < 2 * M; ++firstVertex) {
        const int secondVertex = mapped[firstVertex];
        for (int edge = 0; edge < 3; ++edge) {
            const int wanted = mapped[firstGraph.neighbors[firstVertex][edge]];
            bool found = false;
            for (int otherEdge = 0; otherEdge < 3; ++otherEdge) {
                if (secondGraph.neighbors[secondVertex][otherEdge] == wanted) {
                    found = true;
                    break;
                }
            }
            if (!found) return false;
        }
    }
    return true;
}

Degree3IsoResult searchDegree3Iso(
    const Degree3IsoGraph& firstGraph,
    const Degree3IsoGraph& secondGraph,
    std::array<uint8_t, 2 * MAX_M> firstColors,
    std::array<uint8_t, 2 * MAX_M> secondColors,
    uint64_t& nodes) {
    if (degree3IsoNodeBudget != 0 && nodes >= degree3IsoNodeBudget) {
        return Degree3IsoResult::unknown;
    }
    ++nodes;
    int colorCount = 0;
    if (!refineDegree3Iso(firstGraph, secondGraph,
                          firstColors, secondColors, colorCount)) {
        return Degree3IsoResult::no;
    }

    std::array<uint8_t, 2 * MAX_M> firstSizes{};
    for (int vertex = 0; vertex < 2 * M; ++vertex) ++firstSizes[firstColors[vertex]];
    int selectedColor = -1;
    int selectedSize = 2 * M + 1;
    for (int color = 0; color < colorCount; ++color) {
        if (firstSizes[color] > 1 && firstSizes[color] < selectedSize) {
            selectedColor = color;
            selectedSize = firstSizes[color];
        }
    }
    if (selectedColor < 0) {
        return verifyDegree3IsoMapping(firstGraph, secondGraph,
                                       firstColors, secondColors)
            ? Degree3IsoResult::yes : Degree3IsoResult::no;
    }

    int firstVertex = 0;
    while (firstColors[firstVertex] != selectedColor) ++firstVertex;
    bool sawUnknown = false;
    std::array<std::array<uint8_t, 3>, 2 * MAX_M> triedNeighborhoods{};
    int triedCount = 0;
    for (int secondVertex = 0; secondVertex < 2 * M; ++secondVertex) {
        if (secondColors[secondVertex] != selectedColor) continue;
        bool twin = false;
        for (int i = 0; i < triedCount; ++i) {
            if (triedNeighborhoods[i] == secondGraph.neighbors[secondVertex]) {
                twin = true;
                break;
            }
        }
        if (twin) continue;
        triedNeighborhoods[triedCount++] = secondGraph.neighbors[secondVertex];

        auto childFirst = firstColors;
        auto childSecond = secondColors;
        childFirst[firstVertex] = (uint8_t)colorCount;
        childSecond[secondVertex] = (uint8_t)colorCount;
        const Degree3IsoResult result = searchDegree3Iso(
            firstGraph, secondGraph, childFirst, childSecond, nodes);
        if (result == Degree3IsoResult::yes) return result;
        if (result == Degree3IsoResult::unknown) sawUnknown = true;
    }
    return sawUnknown ? Degree3IsoResult::unknown : Degree3IsoResult::no;
}

Degree3IsoResult areDegree3Isomorphic(const Degree3IsoGraph& firstGraph,
                                      const Degree3IsoGraph& secondGraph,
                                      uint64_t& nodes) {
    if (firstGraph.structuralColorCount != secondGraph.structuralColorCount) {
        return Degree3IsoResult::no;
    }
    std::array<uint8_t, 2 * MAX_M> firstColors = firstGraph.structuralColors;
    std::array<uint8_t, 2 * MAX_M> secondColors = secondGraph.structuralColors;
    std::array<uint8_t, 2 * MAX_M> firstSizes{};
    std::array<uint8_t, 2 * MAX_M> secondSizes{};
    for (int vertex = 0; vertex < 2 * M; ++vertex) {
        ++firstSizes[firstColors[vertex]];
        ++secondSizes[secondColors[vertex]];
    }
    for (int color = 0; color < firstGraph.structuralColorCount; ++color) {
        if (firstSizes[color] != secondSizes[color]) return Degree3IsoResult::no;
    }
    return searchDegree3Iso(firstGraph, secondGraph,
                            firstColors, secondColors, nodes);
}

size_t canonCacheSize() {
    size_t total = 0;
    for (const DegreeCanonCache& cache : canonCacheByDegree) total += cache.map.size();
    return total;
}

void insertCanonCache(DegreeCanonCache& cache, const GraphKey& weak, const GraphKey& strong) {
    if (cache.map.find(weak) != cache.map.end()) return;
    if (cache.cap != 0 && cache.map.size() >= cache.cap) {
        const GraphKey victim = cache.fifo.front();
        cache.fifo.pop_front();
        cache.map.erase(victim);
        ++canonCacheEvictions;
    }
    cache.map.emplace(weak, strong);
    if (cache.cap != 0) cache.fifo.push_back(weak);
}

struct CanonComputation {
    GraphKey key{};
    uint64_t nodes = 0;
    bool fallback = false;
    bool genericCanonicalization = true;
};

CanonComputation computeCanonicalGraphKey(
    const std::array<uint16_t, MAX_M>& graph, const GraphKey& weak) {
    CanonState initial;
    GraphKey best;
    bool haveBest = false;
    uint64_t nodesThisCall = 0;
    bool aborted = false;
    canonicalSearch(graph, initial, best, haveBest, nodesThisCall, aborted);
    CanonComputation result;
    result.nodes = nodesThisCall;
    if (aborted) {
        result.fallback = true;
        result.key = weak;
        result.key.keyKind = 1; // disjoint namespace from strong keys
    } else {
        if (!haveBest) throw std::runtime_error("bipartite graph canonicalization failed");
        result.key = best;
    }
    return result;
}

GraphKey canonicalGraphKey(const std::array<uint16_t, MAX_M>& graph) {
    const GraphKey weak = weakGraphKey(graph);
    DegreeCanonCache& cache = canonCacheByDegree[regularGraphDegree(graph)];
    auto found = cache.map.find(weak);
    if (found != cache.map.end()) {
        ++canonCacheHits;
        return found->second;
    }
    CanonComputation computed = computeCanonicalGraphKey(graph, weak);
    ++canonComputations;
    canonSearchNodes += computed.nodes;
    if (computed.fallback) ++canonFallbacks;
    insertCanonCache(cache, weak, computed.key);
    return computed.key;
}

struct Residual {
    std::array<uint16_t, MAX_M> graph{};
    uint32_t multiplicity = 0;
};

std::vector<GraphKey> canonicalizeResidualKeys(
    const std::vector<Residual>& weakResiduals, size_t* missingUniqueOut = nullptr) {
    if (weakResiduals.empty()) return {};
    const int residualDegree = regularGraphDegree(weakResiduals[0].graph);
    DegreeCanonCache& cache = canonCacheByDegree[residualDegree];
    std::vector<GraphKey> strongKeys(weakResiduals.size());
    std::vector<size_t> pendingOf(weakResiduals.size(), std::numeric_limits<size_t>::max());
    struct Pending {
        GraphKey weak{};
        size_t firstIndex = 0;
    };
    std::unordered_map<GraphKey, size_t, GraphKeyHash> pendingIndex;
    pendingIndex.reserve(weakResiduals.size());
    std::vector<Pending> pending;
    pending.reserve(weakResiduals.size());

    for (size_t i = 0; i < weakResiduals.size(); ++i) {
        const GraphKey weak = weakGraphKey(weakResiduals[i].graph);
        auto found = cache.map.find(weak);
        if (found != cache.map.end()) {
            strongKeys[i] = found->second;
            ++canonCacheHits;
        } else {
            auto [it, inserted] = pendingIndex.emplace(weak, pending.size());
            if (inserted) pending.push_back({weak, i});
            pendingOf[i] = it->second;
        }
    }

    std::vector<CanonComputation> computed(pending.size());
    if (useDegree3IsoBatch && residualDegree == 3) {
        const auto isoBatchStart = std::chrono::steady_clock::now();
        std::vector<Degree3Invariant> invariants(pending.size());
        std::vector<GraphKey> discreteKeys(pending.size());
        std::vector<uint8_t> isDiscrete(pending.size(), 0);
        #pragma omp parallel for schedule(static)
        for (long long j = 0; j < (long long)pending.size(); ++j) {
            const Degree3IsoGraph prepared = prepareDegree3IsoGraph(
                weakResiduals[pending[(size_t)j].firstIndex].graph);
            if (useDegree3PairIso) {
                invariants[(size_t)j] = degree3Invariant(prepared);
            }
            if (prepared.structuralColorCount == 2 * M) {
                isDiscrete[(size_t)j] = 1;
                discreteKeys[(size_t)j] = degree3DiscreteKey(prepared);
            }
        }
        const auto preparedAt = std::chrono::steady_clock::now();

        struct MissingDiscrete {
            GraphKey structuralKey{};
            size_t firstPendingIndex = 0;
        };
        std::unordered_map<GraphKey, size_t, GraphKeyHash> missingDiscreteIndex;
        missingDiscreteIndex.reserve(pending.size());
        std::vector<MissingDiscrete> missingDiscrete;
        std::vector<size_t> missingDiscreteOf(
            pending.size(), std::numeric_limits<size_t>::max());
        std::vector<size_t> unresolved;
        unresolved.reserve(pending.size());
        degree3DiscreteCanonMap.reserve(degree3DiscreteCanonMap.size() + pending.size() / 8);
        for (size_t j = 0; j < pending.size(); ++j) {
            if (!isDiscrete[j]) {
                unresolved.push_back(j);
                continue;
            }
            ++degree3DiscreteInputs;
            auto found = degree3DiscreteCanonMap.find(discreteKeys[j]);
            if (found != degree3DiscreteCanonMap.end()) {
                computed[j].key = found->second;
                computed[j].genericCanonicalization = false;
                ++degree3DiscreteHits;
                continue;
            }
            auto [it, inserted] = missingDiscreteIndex.emplace(
                discreteKeys[j], missingDiscrete.size());
            if (inserted) missingDiscrete.push_back({discreteKeys[j], j});
            else ++degree3DiscreteHits;
            missingDiscreteOf[j] = it->second;
        }
        const auto discreteIndexedAt = std::chrono::steady_clock::now();

        std::vector<CanonComputation> missingDiscreteComputed(missingDiscrete.size());
        #pragma omp parallel for schedule(dynamic, 64)
        for (long long itemNumber = 0;
             itemNumber < (long long)missingDiscrete.size(); ++itemNumber) {
            const size_t pendingIndex = missingDiscrete[(size_t)itemNumber].firstPendingIndex;
            const Pending& item = pending[pendingIndex];
            missingDiscreteComputed[(size_t)itemNumber] = computeCanonicalGraphKey(
                weakResiduals[item.firstIndex].graph, item.weak);
        }
        for (size_t itemNumber = 0; itemNumber < missingDiscrete.size(); ++itemNumber) {
            degree3DiscreteCanonMap.emplace(
                missingDiscrete[itemNumber].structuralKey,
                missingDiscreteComputed[itemNumber].key);
            ++degree3DiscreteClasses;
        }
        for (size_t j = 0; j < pending.size(); ++j) {
            if (missingDiscreteOf[j] == std::numeric_limits<size_t>::max()) continue;
            const size_t itemNumber = missingDiscreteOf[j];
            if (missingDiscrete[itemNumber].firstPendingIndex == j) {
                computed[j] = missingDiscreteComputed[itemNumber];
            } else {
                computed[j].key = missingDiscreteComputed[itemNumber].key;
                computed[j].genericCanonicalization = false;
            }
        }
        const auto discreteCanonicalizedAt = std::chrono::steady_clock::now();

        if (!useDegree3PairIso) {
            const auto fallbackStartedAt = std::chrono::steady_clock::now();
            #pragma omp parallel for schedule(dynamic, 64)
            for (long long unresolvedNumber = 0;
                 unresolvedNumber < (long long)unresolved.size(); ++unresolvedNumber) {
                const size_t pendingIndex = unresolved[(size_t)unresolvedNumber];
                const Pending& item = pending[pendingIndex];
                computed[pendingIndex] = computeCanonicalGraphKey(
                    weakResiduals[item.firstIndex].graph, item.weak);
            }
            if (verboseProbeProgress && C == 6) {
                const auto finishedAt = std::chrono::steady_clock::now();
                std::fprintf(stderr,
                             "probe d3DiscreteTiming pending=%zu unresolved=%zu "
                             "prepare=%.3fs discreteIndex=%.3fs discreteCanon=%.3fs "
                             "genericFallback=%.3fs total=%.3fs\n",
                             pending.size(), unresolved.size(),
                             std::chrono::duration<double>(preparedAt - isoBatchStart).count(),
                             std::chrono::duration<double>(discreteIndexedAt - preparedAt).count(),
                             std::chrono::duration<double>(discreteCanonicalizedAt - discreteIndexedAt).count(),
                             std::chrono::duration<double>(finishedAt - fallbackStartedAt).count(),
                             std::chrono::duration<double>(finishedAt - isoBatchStart).count());
                std::fflush(stderr);
            }
        } else {
        struct IsoGroup {
            Degree3Invariant invariant{};
            std::vector<size_t> pendingIndices;
            std::vector<Degree3IsoRepresentative>* representatives = nullptr;
            uint64_t checks = 0;
            uint64_t hits = 0;
            uint64_t unknown = 0;
            uint64_t nodes = 0;
            uint64_t newRepresentatives = 0;
            uint64_t knownKeyMisses = 0;
        };
        std::unordered_map<Degree3Invariant, size_t, Degree3InvariantHash> groupIndex;
        groupIndex.reserve(unresolved.size());
        std::vector<IsoGroup> groups;
        groups.reserve(unresolved.size());
        for (size_t j : unresolved) {
            auto [it, inserted] = groupIndex.emplace(invariants[j], groups.size());
            if (inserted) {
                IsoGroup group;
                group.invariant = invariants[j];
                groups.push_back(std::move(group));
            }
            groups[it->second].pendingIndices.push_back(j);
        }

        degree3IsoBuckets.reserve(degree3IsoBuckets.size() + groups.size());
        for (IsoGroup& group : groups) {
            auto [it, inserted] = degree3IsoBuckets.try_emplace(group.invariant);
            (void)inserted;
            group.representatives = &it->second;
        }
        const auto groupsBuiltAt = std::chrono::steady_clock::now();

        #pragma omp parallel for schedule(dynamic, 1)
        for (long long groupNumber = 0; groupNumber < (long long)groups.size(); ++groupNumber) {
            IsoGroup& group = groups[(size_t)groupNumber];
            std::vector<Degree3IsoRepresentative>& representatives = *group.representatives;
            for (size_t pendingIndex : group.pendingIndices) {
                const Pending& item = pending[pendingIndex];
                const auto& graph = weakResiduals[item.firstIndex].graph;
                const Degree3IsoGraph prepared = prepareDegree3IsoGraph(graph);
                bool matched = false;
                for (const Degree3IsoRepresentative& representative : representatives) {
                    uint64_t nodes = 0;
                    ++group.checks;
                    const Degree3IsoResult result = areDegree3Isomorphic(
                        prepared, representative.prepared, nodes);
                    group.nodes += nodes;
                    if (result == Degree3IsoResult::yes) {
                        computed[pendingIndex].key = representative.key;
                        computed[pendingIndex].genericCanonicalization = false;
                        ++group.hits;
                        matched = true;
                        break;
                    }
                    if (result == Degree3IsoResult::unknown) ++group.unknown;
                }
                if (matched) continue;

                computed[pendingIndex] = computeCanonicalGraphKey(graph, item.weak);
                bool knownKey = false;
                for (const Degree3IsoRepresentative& representative : representatives) {
                    if (representative.key == computed[pendingIndex].key) {
                        knownKey = true;
                        break;
                    }
                }
                if (knownKey) {
                    ++group.knownKeyMisses;
                }
                if (!knownKey) {
                    representatives.push_back({graph, prepared, computed[pendingIndex].key});
                    ++group.newRepresentatives;
                }
            }
        }
        for (const IsoGroup& group : groups) {
            degree3IsoChecks += group.checks;
            degree3IsoHits += group.hits;
            degree3IsoUnknown += group.unknown;
            degree3IsoNodes += group.nodes;
            degree3IsoRepresentatives += group.newRepresentatives;
            degree3IsoKnownKeyMisses += group.knownKeyMisses;
        }
        if (verboseProbeProgress && C == 6) {
            const auto finishedAt = std::chrono::steady_clock::now();
            std::fprintf(stderr,
                         "probe d3IsoTiming pending=%zu unresolved=%zu groups=%zu "
                         "prepare=%.3fs discreteIndex=%.3fs discreteCanon=%.3fs "
                         "groupBuild=%.3fs iso=%.3fs total=%.3fs\n",
                         pending.size(), unresolved.size(), groups.size(),
                         std::chrono::duration<double>(preparedAt - isoBatchStart).count(),
                         std::chrono::duration<double>(discreteIndexedAt - preparedAt).count(),
                         std::chrono::duration<double>(discreteCanonicalizedAt - discreteIndexedAt).count(),
                         std::chrono::duration<double>(groupsBuiltAt - discreteCanonicalizedAt).count(),
                         std::chrono::duration<double>(finishedAt - groupsBuiltAt).count(),
                         std::chrono::duration<double>(finishedAt - isoBatchStart).count());
            std::fflush(stderr);
        }
        }
    } else {
        #pragma omp parallel for schedule(dynamic, 64)
        for (long long j = 0; j < (long long)pending.size(); ++j) {
            const Pending& item = pending[(size_t)j];
            computed[(size_t)j] = computeCanonicalGraphKey(
                weakResiduals[item.firstIndex].graph, item.weak);
        }
    }
    for (size_t j = 0; j < pending.size(); ++j) {
        if (computed[j].genericCanonicalization) {
            ++canonComputations;
            canonSearchNodes += computed[j].nodes;
            if (computed[j].fallback) ++canonFallbacks;
        }
        insertCanonCache(cache, pending[j].weak, computed[j].key);
    }
    for (size_t i = 0; i < weakResiduals.size(); ++i) {
        if (pendingOf[i] != std::numeric_limits<size_t>::max()) {
            strongKeys[i] = computed[pendingOf[i]].key;
        }
    }
    if (missingUniqueOut) *missingUniqueOut = pending.size();
    return strongKeys;
}

std::vector<Residual> canonicalizeResidualBatch(std::vector<Residual> weakResiduals) {
    if (weakResiduals.empty()) return {};
    size_t missingUnique = 0;
    std::vector<GraphKey> strongKeys = canonicalizeResidualKeys(weakResiduals, &missingUnique);

    std::unordered_map<GraphKey, size_t, GraphKeyHash> strongIndex;
    strongIndex.reserve(weakResiduals.size());
    std::vector<Residual> result;
    result.reserve(weakResiduals.size());
    for (size_t i = 0; i < weakResiduals.size(); ++i) {
        auto [it, inserted] = strongIndex.emplace(strongKeys[i], result.size());
        if (inserted) result.push_back(weakResiduals[i]);
        else result[it->second].multiplicity += weakResiduals[i].multiplicity;
    }
    if (verboseProbeProgress && C == 6 && weakResiduals.size() >= 1000) {
        std::fprintf(stderr,
                     "probe canonBatch weak=%zu missing=%zu strong=%zu cache=%zu fallback=%llu nodes=%llu\n",
                     weakResiduals.size(), missingUnique, result.size(), canonCacheSize(),
                     (unsigned long long)canonFallbacks,
                     (unsigned long long)canonSearchNodes);
        std::fflush(stderr);
    }
    return result;
}

using FactorCount = unsigned __int128;

std::unordered_map<GraphKey, FactorCount, GraphKeyHash> graphMemo;
uint64_t graphCalls = 0;
uint64_t graphMemoHits = 0;
uint64_t graphMemoMisses = 0;
uint64_t perfectMatchings = 0;
uint64_t pivotCalls = 0;
uint64_t pivotAllMatchings = 0;
uint64_t pivotSelectedMatchings = 0;
std::array<uint64_t, MAX_C + 1> graphCallsByDegree{};
std::array<uint64_t, MAX_C + 1> graphMissesByDegree{};
std::array<uint64_t, MAX_C + 1> perfectMatchingsByDegree{};
std::string graphCheckpointPath;
bool graphCheckpointReadOnly = false;
int graphCheckpointInterval = 0;
size_t graphCheckpointParentInterval = 0;
size_t parentsSinceGraphCheckpoint = 0;
uint64_t completedDegree5 = 0;

void saveGraphCheckpoint() {
    if (graphCheckpointPath.empty() || graphCheckpointReadOnly) return;
    const std::string temporary = graphCheckpointPath + ".tmp";
    std::ofstream out(temporary, std::ios::binary | std::ios::trunc);
    if (!out) throw std::runtime_error("cannot create graph checkpoint");
    const char magic[8] = {'F','J','F','A','C','0','1','\0'};
    const uint32_t storedC = (uint32_t)C;
    const uint64_t count = (uint64_t)graphMemo.size();
    out.write(magic, sizeof(magic));
    out.write((const char*)&storedC, sizeof(storedC));
    out.write((const char*)&count, sizeof(count));
    for (const auto& entry : graphMemo) {
        out.write((const char*)entry.first.words.data(),
                  (std::streamsize)(entry.first.words.size() * sizeof(uint64_t)));
        out.write((const char*)&entry.first.keyKind, sizeof(entry.first.keyKind));
        const uint64_t low = (uint64_t)entry.second;
        const uint64_t high = (uint64_t)(entry.second >> 64);
        out.write((const char*)&low, sizeof(low));
        out.write((const char*)&high, sizeof(high));
    }
    out.close();
    if (!out) throw std::runtime_error("failed while writing graph checkpoint");
#ifdef _WIN32
    if (!MoveFileExA(temporary.c_str(), graphCheckpointPath.c_str(),
                     MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
        throw std::runtime_error("cannot install graph checkpoint, Windows error " +
                                 std::to_string(GetLastError()));
    }
#else
    std::error_code error;
    std::filesystem::rename(temporary, graphCheckpointPath, error);
    if (error) throw std::runtime_error("cannot install graph checkpoint: " + error.message());
#endif
    if (verboseProbeProgress) {
        std::fprintf(stderr, "probe checkpoint saved path=%s entries=%zu\n",
                     graphCheckpointPath.c_str(), graphMemo.size());
        std::fflush(stderr);
    }
}

void loadGraphCheckpoint() {
    if (graphCheckpointPath.empty() || !std::filesystem::exists(graphCheckpointPath)) return;
    std::ifstream in(graphCheckpointPath, std::ios::binary);
    if (!in) throw std::runtime_error("cannot open graph checkpoint");
    char magic[8]{};
    uint32_t storedC = 0;
    uint64_t count = 0;
    in.read(magic, sizeof(magic));
    in.read((char*)&storedC, sizeof(storedC));
    in.read((char*)&count, sizeof(count));
    if (std::memcmp(magic, "FJFAC01", 7) != 0 || storedC != (uint32_t)C) {
        throw std::runtime_error("incompatible graph checkpoint");
    }
    graphMemo.reserve((size_t)(count * 5 / 4 + 1024));
    for (uint64_t i = 0; i < count; ++i) {
        GraphKey key;
        uint64_t low = 0, high = 0;
        in.read((char*)key.words.data(),
                (std::streamsize)(key.words.size() * sizeof(uint64_t)));
        in.read((char*)&key.keyKind, sizeof(key.keyKind));
        in.read((char*)&low, sizeof(low));
        in.read((char*)&high, sizeof(high));
        graphMemo.emplace(key, (FactorCount)low | ((FactorCount)high << 64));
    }
    if (!in) throw std::runtime_error("truncated graph checkpoint");
    std::fprintf(stderr, "graph checkpoint loaded path=%s entries=%zu\n",
                 graphCheckpointPath.c_str(), graphMemo.size());
}

int cycleComponents(const std::array<uint16_t, MAX_M>& graph) {
    std::array<int, 2 * MAX_M> parent{};
    for (int i = 0; i < 2 * M; ++i) parent[i] = i;
    auto root = [&](int x) {
        while (parent[x] != x) {
            parent[x] = parent[parent[x]];
            x = parent[x];
        }
        return x;
    };
    int components = 2 * M;
    for (int left = 0; left < M; ++left) {
        uint16_t bits = graph[left];
        while (bits) {
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

FactorCount countFactorizations(const std::array<uint16_t, MAX_M>& graph, int degree);

struct PivotEdge {
    uint16_t rightBit = 0;
    uint64_t frequency = 0;
    uint64_t allMatchings = 0;
};

// F_d(G) = d * sum_{M contains e} F_{d-1}(G-M).  Frequencies at one
// left vertex sum to pm(G), so its least-frequent incident edge is a safe pivot.
PivotEdge leastFrequentRootEdge(const std::array<uint16_t, MAX_M>& graph) {
    std::array<uint64_t, 1 << MAX_M> dp{};
    dp[0] = 1;
    for (int row = 1; row < M; ++row) {
        for (int used = 0; used <= FULL; ++used) {
            if (std::popcount((unsigned)used) != row - 1 || dp[used] == 0) continue;
            uint16_t available = graph[row] & (uint16_t)~used;
            while (available) {
                const uint16_t bit = available & (uint16_t)(-available);
                available ^= bit;
                dp[used | bit] += dp[used];
            }
        }
    }

    PivotEdge result;
    result.frequency = std::numeric_limits<uint64_t>::max();
    uint16_t incident = graph[0];
    while (incident) {
        const uint16_t bit = incident & (uint16_t)(-incident);
        incident ^= bit;
        const uint64_t frequency = dp[FULL ^ bit];
        result.allMatchings += frequency;
        if (frequency < result.frequency) {
            result.rightBit = bit;
            result.frequency = frequency;
        }
    }
    if (result.rightBit == 0 || result.frequency == 0) {
        throw std::runtime_error("regular bipartite graph has no usable pivot edge");
    }
    return result;
}

void recordPivot(const PivotEdge& pivot) {
    ++pivotCalls;
    pivotAllMatchings += pivot.allMatchings;
    pivotSelectedMatchings += pivot.frequency;
}

uint64_t residualMultiplicitySum(const std::vector<Residual>& residuals) {
    uint64_t total = 0;
    for (const Residual& residual : residuals) total += residual.multiplicity;
    return total;
}

struct RootedTwoFactorPairCount {
    uint16_t edgePair = 0;
    uint64_t count = 0;
};

struct RootedTwoFactorCounts {
    std::array<RootedTwoFactorPairCount, MAX_C * (MAX_C - 1) / 2> pairs{};
    int pairCount = 0;
    uint64_t minimum = std::numeric_limits<uint64_t>::max();
    uint64_t maximum = 0;
    uint64_t total = 0;
};

uint64_t countRootedTwoFactors(const std::array<uint16_t, MAX_M>& graph,
                               uint16_t rootPair) {
    std::array<int, MAX_M> powerOfThree{};
    powerOfThree[0] = 1;
    for (int col = 1; col < M; ++col) powerOfThree[col] = 3 * powerOfThree[col - 1];
    const int stateCount = 3 * powerOfThree[M - 1];
    const int target = stateCount - 1;
    std::vector<uint64_t> current((size_t)stateCount, 0);
    std::vector<uint64_t> next((size_t)stateCount, 0);
    std::vector<int> active;
    std::vector<int> nextActive;
    active.reserve(80000);
    nextActive.reserve(80000);

    int rootState = 0;
    uint16_t rootBits = rootPair;
    while (rootBits) {
        const int col = std::countr_zero(rootBits);
        rootBits &= (uint16_t)(rootBits - 1);
        rootState += powerOfThree[col];
    }
    current[rootState] = 1;
    active.push_back(rootState);

    for (int row = 1; row < M; ++row) {
        for (int state : active) {
            const uint64_t ways = current[state];
            uint16_t firstChoices = graph[row];
            while (firstChoices) {
                const uint16_t firstBit = firstChoices & (uint16_t)(-firstChoices);
                firstChoices ^= firstBit;
                const int firstCol = std::countr_zero(firstBit);
                if ((state / powerOfThree[firstCol]) % 3 >= 2) continue;
                uint16_t secondChoices = firstChoices;
                while (secondChoices) {
                    const uint16_t secondBit = secondChoices & (uint16_t)(-secondChoices);
                    secondChoices ^= secondBit;
                    const int secondCol = std::countr_zero(secondBit);
                    if ((state / powerOfThree[secondCol]) % 3 >= 2) continue;
                    const int child = state + powerOfThree[firstCol] + powerOfThree[secondCol];
                    if (next[child] == 0) nextActive.push_back(child);
                    next[child] += ways;
                }
            }
        }
        for (int state : active) current[state] = 0;
        current.swap(next);
        active.swap(nextActive);
        nextActive.clear();
    }
    return current[target];
}

RootedTwoFactorCounts countRootedTwoFactorPairs(
    const std::array<uint16_t, MAX_M>& graph) {
    RootedTwoFactorCounts result;
    uint16_t firstChoices = graph[0];
    while (firstChoices) {
        const uint16_t firstBit = firstChoices & (uint16_t)(-firstChoices);
        firstChoices ^= firstBit;
        uint16_t secondChoices = firstChoices;
        while (secondChoices) {
            const uint16_t secondBit = secondChoices & (uint16_t)(-secondChoices);
            secondChoices ^= secondBit;
            const uint16_t pair = firstBit | secondBit;
            const uint64_t count = countRootedTwoFactors(graph, pair);
            result.pairs[result.pairCount++] = {pair, count};
            result.minimum = std::min(result.minimum, count);
            result.maximum = std::max(result.maximum, count);
            result.total += count;
        }
    }
    return result;
}

void probeRootedTwoFactors(const std::array<uint16_t, MAX_M>& graph,
                           int degree) {
    if (rootedTwoFactorProbes[degree] <= 0) return;
    const auto start = std::chrono::steady_clock::now();
    const RootedTwoFactorCounts counts = countRootedTwoFactorPairs(graph);
    const double seconds = std::chrono::duration<double>(
        std::chrono::steady_clock::now() - start).count();
    std::fprintf(stderr,
                 "rooted2 probe degree=%d pairs=%d min=%llu max=%llu "
                 "total=%llu time=%.6fs values=",
                 degree, counts.pairCount,
                 (unsigned long long)counts.minimum,
                 (unsigned long long)counts.maximum,
                 (unsigned long long)counts.total, seconds);
    for (int i = 0; i < counts.pairCount; ++i) {
        const uint16_t pair = counts.pairs[i].edgePair;
        const int first = std::countr_zero(pair);
        const int second = std::countr_zero((uint16_t)(pair & (pair - 1)));
        std::fprintf(stderr, "%s%d-%d:%llu", i == 0 ? "" : ",",
                     first, second,
                     (unsigned long long)counts.pairs[i].count);
    }
    std::fprintf(stderr, "\n");
    std::fflush(stderr);
    --rootedTwoFactorProbes[degree];
}

struct RollbackDsu {
    struct Change {
        uint8_t child;
        uint8_t root;
        uint8_t rootSize;
    };

    std::array<uint8_t, 2 * MAX_M> parent{};
    std::array<uint8_t, 2 * MAX_M> size{};
    std::array<Change, 2 * MAX_M> history{};
    int historySize = 0;
    int components = 0;

    explicit RollbackDsu(int vertices) : components(vertices) {
        for (int vertex = 0; vertex < vertices; ++vertex) {
            parent[vertex] = (uint8_t)vertex;
            size[vertex] = 1;
        }
    }

    int root(int vertex) const {
        while (parent[vertex] != vertex) vertex = parent[vertex];
        return vertex;
    }

    void addEdge(int left, int right) {
        int a = root(left);
        int b = root(right);
        if (a == b) return;
        if (size[a] < size[b]) std::swap(a, b);
        history[historySize++] = {(uint8_t)b, (uint8_t)a, size[a]};
        parent[b] = (uint8_t)a;
        size[a] = (uint8_t)(size[a] + size[b]);
        --components;
    }

    void rollback(int snapshot) {
        while (historySize > snapshot) {
            const Change change = history[--historySize];
            parent[change.child] = change.child;
            size[change.root] = change.rootSize;
            ++components;
        }
    }
};

struct Degree4RootedResult {
    FactorCount value = 0;
    uint64_t leaves = 0;
    uint64_t nodes = 0;
};

void addFactorEdges(RollbackDsu& dsu, int left, uint16_t rightBits) {
    while (rightBits) {
        const int right = std::countr_zero(rightBits);
        rightBits &= (uint16_t)(rightBits - 1);
        dsu.addEdge(left, M + right);
    }
}

struct RootedSplitWorkspace {
    std::vector<uint32_t> marks;
    std::array<std::vector<int>, MAX_M> layers;
    uint32_t marker = 0;

    uint32_t nextMarker() {
        if (++marker == 0) {
            std::fill(marks.begin(), marks.end(), 0);
            marker = 1;
        }
        return marker;
    }
};

thread_local RootedSplitWorkspace rootedSplitWorkspace;

void enumerateRootedDegree4Split(
    const std::array<uint16_t, MAX_M>& graph,
    const std::array<int, MAX_M>& powerOfThree,
    const std::vector<uint32_t>& reachable,
    uint32_t marker,
    int row,
    int state,
    RollbackDsu& selected,
    RollbackDsu& complement,
    Degree4RootedResult& result) {
    ++result.nodes;
    if (row == 0) {
        result.value += (FactorCount)1 << (selected.components + complement.components);
        ++result.leaves;
        return;
    }

    uint16_t firstChoices = graph[row];
    while (firstChoices) {
        const uint16_t first = firstChoices & (uint16_t)(-firstChoices);
        firstChoices ^= first;
        const int firstCol = std::countr_zero(first);
        if ((state / powerOfThree[firstCol]) % 3 == 0) continue;
        uint16_t secondChoices = firstChoices;
        while (secondChoices) {
            const uint16_t second = secondChoices & (uint16_t)(-secondChoices);
            secondChoices ^= second;
            const int secondCol = std::countr_zero(second);
            if ((state / powerOfThree[secondCol]) % 3 == 0) continue;
            const uint16_t pair = first | second;
            const int predecessor = state - powerOfThree[firstCol]
                                          - powerOfThree[secondCol];
            if (reachable[predecessor] != marker) continue;
            const int selectedSnapshot = selected.historySize;
            const int complementSnapshot = complement.historySize;
            addFactorEdges(selected, row, pair);
            addFactorEdges(complement, row, graph[row] ^ pair);
            enumerateRootedDegree4Split(graph, powerOfThree, reachable, marker,
                                        row - 1, predecessor,
                                        selected, complement, result);
            selected.rollback(selectedSnapshot);
            complement.rollback(complementSnapshot);
        }
    }
}

Degree4RootedResult computeDegree4RootedSplit(
    const std::array<uint16_t, MAX_M>& graph) {
    // F_4(G) = 6 * sum_H 2^(c(H) + c(G-H)), with H containing rootPair.
    const uint16_t first = graph[0] & (uint16_t)(-graph[0]);
    const uint16_t rest = graph[0] ^ first;
    const uint16_t second = rest & (uint16_t)(-rest);
    const uint16_t rootPair = first | second;
    if (std::popcount((unsigned)graph[0]) != 4 || second == 0) {
        throw std::runtime_error("rooted degree-4 split requires a 4-regular graph");
    }

    std::array<int, MAX_M> powerOfThree{};
    powerOfThree[0] = 1;
    for (int col = 1; col < M; ++col) {
        powerOfThree[col] = 3 * powerOfThree[col - 1];
    }
    const int stateCount = 3 * powerOfThree[M - 1];
    if ((int)rootedSplitWorkspace.marks.size() != stateCount) {
        rootedSplitWorkspace.marks.assign((size_t)stateCount, 0);
        rootedSplitWorkspace.marker = 0;
    }
    for (int row = 0; row < M; ++row) rootedSplitWorkspace.layers[row].clear();
    // The ternary digit sum fixes the layer, so one stamp covers every layer.
    const uint32_t marker = rootedSplitWorkspace.nextMarker();

    int rootState = 0;
    uint16_t rootBits = rootPair;
    while (rootBits) {
        const int col = std::countr_zero(rootBits);
        rootBits &= (uint16_t)(rootBits - 1);
        rootState += powerOfThree[col];
    }
    rootedSplitWorkspace.layers[0].push_back(rootState);
    rootedSplitWorkspace.marks[rootState] = marker;
    for (int row = 1; row < M; ++row) {
        const std::vector<int>& current = rootedSplitWorkspace.layers[row - 1];
        std::vector<int>& next = rootedSplitWorkspace.layers[row];
        for (int state : current) {
            uint16_t firstChoices = graph[row];
            while (firstChoices) {
                const uint16_t firstBit = firstChoices & (uint16_t)(-firstChoices);
                firstChoices ^= firstBit;
                const int firstCol = std::countr_zero(firstBit);
                if ((state / powerOfThree[firstCol]) % 3 >= 2) continue;
                uint16_t secondChoices = firstChoices;
                while (secondChoices) {
                    const uint16_t secondBit = secondChoices & (uint16_t)(-secondChoices);
                    secondChoices ^= secondBit;
                    const int secondCol = std::countr_zero(secondBit);
                    if ((state / powerOfThree[secondCol]) % 3 >= 2) continue;
                    const int child = state + powerOfThree[firstCol]
                                            + powerOfThree[secondCol];
                    if (rootedSplitWorkspace.marks[child] != marker) {
                        rootedSplitWorkspace.marks[child] = marker;
                        next.push_back(child);
                    }
                }
            }
        }
    }

    RollbackDsu selected(2 * M);
    RollbackDsu complement(2 * M);
    addFactorEdges(selected, 0, rootPair);
    addFactorEdges(complement, 0, graph[0] ^ rootPair);
    Degree4RootedResult result;
    const int target = stateCount - 1;
    if (rootedSplitWorkspace.marks[target] == marker) {
        enumerateRootedDegree4Split(graph, powerOfThree,
                                    rootedSplitWorkspace.marks, marker,
                                    M - 1, target,
                                    selected, complement, result);
    }
    result.value *= 6;
    return result;
}

void enumeratePerfectMatchings(
    const std::array<uint16_t, MAX_M>& graph,
    uint16_t remainingLeft,
    uint16_t usedRight,
    std::array<uint16_t, MAX_M>& chosen,
    std::unordered_map<GraphKey, size_t, GraphKeyHash>& residualIndex,
    std::vector<Residual>& residuals) {
    if (remainingLeft == 0) {
        ++perfectMatchings;
        ++perfectMatchingsByDegree[regularGraphDegree(graph)];
        if (verboseProbeProgress && C == 6 && perfectMatchings % 50000 == 0) {
            std::fprintf(stderr,
                         "probe progress PM=%llu [d3=%llu d4=%llu d5=%llu d6=%llu] "
                         "miss=[%llu,%llu,%llu,%llu] graphMemo=%zu canonCache=%zu "
                         "cacheByDegree=[%zu,%zu,%zu] evict=%llu canon=%llu fallback=%llu nodes=%llu\n",
                         (unsigned long long)perfectMatchings,
                         (unsigned long long)perfectMatchingsByDegree[3],
                         (unsigned long long)perfectMatchingsByDegree[4],
                         (unsigned long long)perfectMatchingsByDegree[5],
                         (unsigned long long)perfectMatchingsByDegree[6],
                         (unsigned long long)graphMissesByDegree[3],
                         (unsigned long long)graphMissesByDegree[4],
                         (unsigned long long)graphMissesByDegree[5],
                         (unsigned long long)graphMissesByDegree[6],
                         graphMemo.size(), canonCacheSize(),
                         canonCacheByDegree[3].map.size(),
                         canonCacheByDegree[4].map.size(),
                         canonCacheByDegree[5].map.size(),
                         (unsigned long long)canonCacheEvictions,
                         (unsigned long long)canonComputations,
                         (unsigned long long)canonFallbacks,
                         (unsigned long long)canonSearchNodes);
            std::fflush(stderr);
        }
        std::array<uint16_t, MAX_M> residual{};
        for (int i = 0; i < M; ++i) residual[i] = graph[i] ^ chosen[i];
        // First collapse exact duplicates modulo left-vertex order only.  The
        // more expensive two-sided canonicalization is performed once per
        // distinct weak residual, in a parallel batch after enumeration.
        const GraphKey key = weakGraphKey(residual);
        auto [it, inserted] = residualIndex.emplace(key, residuals.size());
        if (inserted) residuals.push_back({residual, 1});
        else ++residuals[it->second].multiplicity;
        return;
    }

    int bestLeft = -1;
    uint16_t bestAvailable = 0;
    int bestCount = M + 1;
    uint16_t leftBits = remainingLeft;
    while (leftBits) {
        const int left = std::countr_zero(leftBits);
        leftBits &= (uint16_t)(leftBits - 1);
        const uint16_t available = graph[left] & (uint16_t)~usedRight;
        const int count = std::popcount(available);
        if (count < bestCount) {
            bestCount = count;
            bestLeft = left;
            bestAvailable = available;
            if (count <= 1) break;
        }
    }
    if (bestCount == 0) return;

    uint16_t available = bestAvailable;
    while (available) {
        const uint16_t bit = available & (uint16_t)(-available);
        available ^= bit;
        chosen[bestLeft] = bit;
        enumeratePerfectMatchings(graph,
                                  remainingLeft ^ (uint16_t)(1u << bestLeft),
                                  usedRight | bit,
                                  chosen,
                                  residualIndex,
                                  residuals);
    }
    chosen[bestLeft] = 0;
}

void enumeratePerfectMatchingsQuiet(
    const std::array<uint16_t, MAX_M>& graph,
    uint16_t remainingLeft,
    uint16_t usedRight,
    std::array<uint16_t, MAX_M>& chosen,
    std::unordered_map<GraphKey, size_t, GraphKeyHash>& residualIndex,
    std::vector<Residual>& residuals,
    uint64_t& matchingCount) {
    if (remainingLeft == 0) {
        ++matchingCount;
        std::array<uint16_t, MAX_M> residual{};
        for (int i = 0; i < M; ++i) residual[i] = graph[i] ^ chosen[i];
        const GraphKey key = weakGraphKey(residual);
        auto [it, inserted] = residualIndex.emplace(key, residuals.size());
        if (inserted) residuals.push_back({residual, 1});
        else ++residuals[it->second].multiplicity;
        return;
    }

    int bestLeft = -1;
    uint16_t bestAvailable = 0;
    int bestCount = M + 1;
    uint16_t leftBits = remainingLeft;
    while (leftBits) {
        const int left = std::countr_zero(leftBits);
        leftBits &= (uint16_t)(leftBits - 1);
        const uint16_t available = graph[left] & (uint16_t)~usedRight;
        const int count = std::popcount(available);
        if (count < bestCount) {
            bestCount = count;
            bestLeft = left;
            bestAvailable = available;
            if (count <= 1) break;
        }
    }
    if (bestCount == 0) return;

    uint16_t available = bestAvailable;
    while (available) {
        const uint16_t bit = available & (uint16_t)(-available);
        available ^= bit;
        chosen[bestLeft] = bit;
        enumeratePerfectMatchingsQuiet(graph,
                                       remainingLeft ^ (uint16_t)(1u << bestLeft),
                                       usedRight | bit,
                                       chosen,
                                       residualIndex,
                                       residuals,
                                       matchingCount);
    }
    chosen[bestLeft] = 0;
}

FactorCount computeDegree3Direct(const std::array<uint16_t, MAX_M>& graph) {
    std::unordered_map<GraphKey, size_t, GraphKeyHash> residualIndex;
    std::vector<Residual> residuals;
    residualIndex.reserve(512);
    residuals.reserve(512);
    std::array<uint16_t, MAX_M> chosen{};
    int colorMultiplier = 1;
    if (colorPivotMaxDegree >= 3) {
        const PivotEdge pivot = leastFrequentRootEdge(graph);
        recordPivot(pivot);
        chosen[0] = pivot.rightBit;
        enumeratePerfectMatchings(graph, (uint16_t)(FULL ^ 1u), pivot.rightBit,
                                  chosen, residualIndex, residuals);
        if (residualMultiplicitySum(residuals) != pivot.frequency) {
            throw std::runtime_error("degree-3 pivot frequency mismatch");
        }
        colorMultiplier = 3;
    } else {
        enumeratePerfectMatchings(graph, (uint16_t)FULL, 0,
                                  chosen, residualIndex, residuals);
    }
    FactorCount total = 0;
    for (const Residual& residual : residuals) {
        total += (FactorCount)residual.multiplicity << cycleComponents(residual.graph);
    }
    return (FactorCount)colorMultiplier * total;
}

std::vector<FactorCount> evaluateDegree4Layered(const std::vector<Residual>& degree4Residuals) {
    std::vector<FactorCount> values(degree4Residuals.size());
    std::vector<GraphKey> degree4Keys(degree4Residuals.size());
    std::vector<size_t> missingParents;
    missingParents.reserve(degree4Residuals.size());

    for (size_t i = 0; i < degree4Residuals.size(); ++i) {
        ++graphCalls;
        ++graphCallsByDegree[4];
        degree4Keys[i] = canonicalGraphKey(degree4Residuals[i].graph);
        auto found = graphMemo.find(degree4Keys[i]);
        if (found != graphMemo.end()) {
            ++graphMemoHits;
            values[i] = found->second;
        } else {
            ++graphMemoMisses;
            ++graphMissesByDegree[4];
            missingParents.push_back(i);
        }
    }

    for (size_t parent : missingParents) {
        if (rootedTwoFactorProbes[4] <= 0) break;
        probeRootedTwoFactors(degree4Residuals[parent].graph, 4);
    }

    if (useDegree4RootedSplit) {
        for (size_t chunkStart = 0; chunkStart < missingParents.size();
             chunkStart += degree4ParentChunk) {
            const size_t chunkEnd = std::min(chunkStart + degree4ParentChunk,
                                             missingParents.size());
            const size_t parentCount = chunkEnd - chunkStart;
            std::vector<Degree4RootedResult> results(parentCount);
            #pragma omp parallel for if(parallelDegree4ParentEnumeration) schedule(dynamic, 1)
            for (long long local = 0; local < (long long)parentCount; ++local) {
                const size_t parent = missingParents[chunkStart + (size_t)local];
                results[(size_t)local] = computeDegree4RootedSplit(
                    degree4Residuals[parent].graph);
            }
            for (size_t local = 0; local < parentCount; ++local) {
                const size_t parent = missingParents[chunkStart + local];
                values[parent] = results[local].value;
                graphMemo.emplace(degree4Keys[parent], values[parent]);
                ++degree4RootedCalls;
                degree4RootedLeaves += results[local].leaves;
                degree4RootedNodes += results[local].nodes;
            }
            if (graphCheckpointParentInterval > 0) {
                parentsSinceGraphCheckpoint += parentCount;
                if (parentsSinceGraphCheckpoint >= graphCheckpointParentInterval) {
                    saveGraphCheckpoint();
                    parentsSinceGraphCheckpoint = 0;
                }
            }
        }
        return values;
    }

    for (size_t chunkStart = 0; chunkStart < missingParents.size();
         chunkStart += degree4ParentChunk) {
        const size_t chunkEnd = std::min(chunkStart + degree4ParentChunk,
                                         missingParents.size());
        const size_t parentCount = chunkEnd - chunkStart;
        std::vector<size_t> offsets(parentCount + 1, 0);
        std::vector<Residual> flat;
        flat.reserve(parentCount * 2800);
        const bool pivotDegree4 = colorPivotMaxDegree >= 4;
        std::vector<PivotEdge> parentPivots(pivotDegree4 ? parentCount : 0);

        if (parallelDegree4ParentEnumeration) {
            std::vector<std::vector<Residual>> parentResiduals(parentCount);
            std::vector<uint64_t> parentMatchingCounts(parentCount, 0);
            #pragma omp parallel for schedule(dynamic, 1)
            for (long long local = 0; local < (long long)parentCount; ++local) {
                const size_t parent = missingParents[chunkStart + (size_t)local];
                std::unordered_map<GraphKey, size_t, GraphKeyHash> residualIndex;
                residualIndex.reserve(4096);
                std::vector<Residual>& residuals = parentResiduals[(size_t)local];
                residuals.reserve(4096);
                std::array<uint16_t, MAX_M> chosen{};
                if (pivotDegree4) {
                    const PivotEdge pivot = leastFrequentRootEdge(
                        degree4Residuals[parent].graph);
                    parentPivots[(size_t)local] = pivot;
                    chosen[0] = pivot.rightBit;
                    enumeratePerfectMatchingsQuiet(
                        degree4Residuals[parent].graph,
                        (uint16_t)(FULL ^ 1u), pivot.rightBit,
                        chosen, residualIndex, residuals,
                        parentMatchingCounts[(size_t)local]);
                } else {
                    enumeratePerfectMatchingsQuiet(
                        degree4Residuals[parent].graph,
                        (uint16_t)FULL, 0,
                        chosen, residualIndex, residuals,
                        parentMatchingCounts[(size_t)local]);
                }
            }
            size_t totalResiduals = 0;
            uint64_t chunkMatchingCount = 0;
            for (size_t local = 0; local < parentCount; ++local) {
                totalResiduals += parentResiduals[local].size();
                chunkMatchingCount += parentMatchingCounts[local];
                if (pivotDegree4) {
                    if (parentMatchingCounts[local] != parentPivots[local].frequency) {
                        throw std::runtime_error("degree-4 parallel pivot frequency mismatch");
                    }
                    recordPivot(parentPivots[local]);
                }
            }
            flat.reserve(totalResiduals);
            for (size_t local = 0; local < parentCount; ++local) {
                flat.insert(flat.end(), parentResiduals[local].begin(),
                            parentResiduals[local].end());
                offsets[local + 1] = flat.size();
            }
            perfectMatchings += chunkMatchingCount;
            perfectMatchingsByDegree[4] += chunkMatchingCount;
        } else {
            for (size_t local = 0; local < parentCount; ++local) {
                const size_t parent = missingParents[chunkStart + local];
                std::unordered_map<GraphKey, size_t, GraphKeyHash> residualIndex;
                std::vector<Residual> residuals;
                residualIndex.reserve(4096);
                residuals.reserve(4096);
                std::array<uint16_t, MAX_M> chosen{};
                if (pivotDegree4) {
                    const PivotEdge pivot = leastFrequentRootEdge(
                        degree4Residuals[parent].graph);
                    parentPivots[local] = pivot;
                    recordPivot(pivot);
                    chosen[0] = pivot.rightBit;
                    enumeratePerfectMatchings(degree4Residuals[parent].graph,
                                              (uint16_t)(FULL ^ 1u), pivot.rightBit,
                                              chosen, residualIndex, residuals);
                    if (residualMultiplicitySum(residuals) != pivot.frequency) {
                        throw std::runtime_error("degree-4 pivot frequency mismatch");
                    }
                } else {
                    enumeratePerfectMatchings(degree4Residuals[parent].graph,
                                              (uint16_t)FULL, 0,
                                              chosen, residualIndex, residuals);
                }
                flat.insert(flat.end(), residuals.begin(), residuals.end());
                offsets[local + 1] = flat.size();
            }
        }

        std::vector<GraphKey> degree3Keys = canonicalizeResidualKeys(flat);
        std::unordered_map<GraphKey, FactorCount, GraphKeyHash> degree3Values;
        degree3Values.reserve(degree3Keys.size());
        for (size_t i = 0; i < flat.size(); ++i) {
            auto [localValue, inserted] = degree3Values.emplace(degree3Keys[i], 0);
            if (!inserted) continue;
            ++graphCalls;
            ++graphCallsByDegree[3];
            auto found = graphMemo.find(degree3Keys[i]);
            if (found != graphMemo.end()) {
                ++graphMemoHits;
                localValue->second = found->second;
            } else {
                ++graphMemoMisses;
                ++graphMissesByDegree[3];
                localValue->second = computeDegree3Direct(flat[i].graph);
                graphMemo.emplace(degree3Keys[i], localValue->second);
            }
        }

        for (size_t local = 0; local < parentCount; ++local) {
            FactorCount value = 0;
            for (size_t i = offsets[local]; i < offsets[local + 1]; ++i) {
                value += (FactorCount)flat[i].multiplicity * degree3Values.find(degree3Keys[i])->second;
            }
            const size_t parent = missingParents[chunkStart + local];
            values[parent] = pivotDegree4 ? (FactorCount)4 * value : value;
            graphMemo.emplace(degree4Keys[parent], values[parent]);
        }

        if (graphCheckpointParentInterval > 0) {
            parentsSinceGraphCheckpoint += parentCount;
            if (parentsSinceGraphCheckpoint >= graphCheckpointParentInterval) {
                saveGraphCheckpoint();
                parentsSinceGraphCheckpoint = 0;
            }
        }

        if (verboseProbeProgress) {
            std::fprintf(stderr,
                         "probe layeredF4 parents=%zu/%zu flatD3=%zu strongD3=%zu graphMemo=%zu cache=%zu "
                         "d3Discrete=[input=%llu hit=%llu class=%llu] "
                         "d3Iso=[rep=%llu hit=%llu check=%llu unknown=%llu nodes=%llu]\n",
                         chunkEnd, missingParents.size(), flat.size(), degree3Values.size(),
                         graphMemo.size(), canonCacheSize(),
                         (unsigned long long)degree3DiscreteInputs,
                         (unsigned long long)degree3DiscreteHits,
                         (unsigned long long)degree3DiscreteClasses,
                         (unsigned long long)degree3IsoRepresentatives,
                         (unsigned long long)degree3IsoHits,
                         (unsigned long long)degree3IsoChecks,
                         (unsigned long long)degree3IsoUnknown,
                         (unsigned long long)degree3IsoNodes);
            std::fflush(stderr);
        }
    }
    return values;
}

FactorCount countFactorizations(const std::array<uint16_t, MAX_M>& input, int degree) {
    const auto degreeStart = degree == 5 ? std::chrono::steady_clock::now()
                                         : std::chrono::steady_clock::time_point{};
    ++graphCalls;
    ++graphCallsByDegree[degree];
    if (degree == 1) return 1;
    if (degree == 2) return (FactorCount)1 << cycleComponents(input);

    std::array<uint16_t, MAX_M> graph = input;
    std::sort(graph.begin(), graph.begin() + M);
    const GraphKey key = canonicalGraphKey(graph);
    auto memoIt = graphMemo.find(key);
    if (memoIt != graphMemo.end()) {
        ++graphMemoHits;
        return memoIt->second;
    }
    ++graphMemoMisses;
    ++graphMissesByDegree[degree];

    probeRootedTwoFactors(graph, degree);

    if (degree == 4 && useDegree4RootedSplit) {
        const Degree4RootedResult result = computeDegree4RootedSplit(graph);
        ++degree4RootedCalls;
        degree4RootedLeaves += result.leaves;
        degree4RootedNodes += result.nodes;
        graphMemo.emplace(key, result.value);
        return result.value;
    }

    std::unordered_map<GraphKey, size_t, GraphKeyHash> residualIndex;
    std::vector<Residual> residuals;
    residualIndex.reserve(256);
    residuals.reserve(256);
    std::array<uint16_t, MAX_M> chosen{};
    int colorMultiplier = 1;
    if (colorPivotMaxDegree >= degree) {
        const PivotEdge pivot = leastFrequentRootEdge(graph);
        recordPivot(pivot);
        chosen[0] = pivot.rightBit;
        enumeratePerfectMatchings(graph, (uint16_t)(FULL ^ 1u), pivot.rightBit,
                                  chosen, residualIndex, residuals);
        if (residualMultiplicitySum(residuals) != pivot.frequency) {
            throw std::runtime_error("factorization pivot frequency mismatch");
        }
        colorMultiplier = degree;
    } else {
        enumeratePerfectMatchings(graph, (uint16_t)FULL, 0,
                                  chosen, residualIndex, residuals);
    }
    // At degree 3 the residuals are 2-regular, and F_2 is available directly
    // as 2^(number of cycle components).  Canonicalizing those residuals is
    // strictly wasted work; weak-key grouping is already exact for summation.
    if (degree > 3) residuals = canonicalizeResidualBatch(std::move(residuals));

    FactorCount total = 0;
    if (C == 6 && degree == 5) {
        std::vector<FactorCount> childValues = evaluateDegree4Layered(residuals);
        for (size_t i = 0; i < residuals.size(); ++i) {
            total += (FactorCount)residuals[i].multiplicity * childValues[i];
        }
    } else {
        for (const Residual& residual : residuals) {
            total += (FactorCount)residual.multiplicity *
                     countFactorizations(residual.graph, degree - 1);
        }
    }
    total *= (FactorCount)colorMultiplier;
    graphMemo.emplace(key, total);
    if (degree == 5) {
        ++completedDegree5;
        if (verboseProbeProgress) {
            const double seconds = std::chrono::duration<double>(
                std::chrono::steady_clock::now() - degreeStart).count();
            const std::string value = u128ToString(total);
            std::fprintf(stderr,
                         "probe completedF5=%llu F=%s time=%.3fs graphMemo=%zu\n",
                         (unsigned long long)completedDegree5, value.c_str(), seconds,
                         graphMemo.size());
            std::fflush(stderr);
        }
        if (graphCheckpointInterval > 0 &&
            completedDegree5 % (uint64_t)graphCheckpointInterval == 0) {
            saveGraphCheckpoint();
        }
    }
    return total;
}

std::array<uint16_t, MAX_M> graphFromHistogram(const Hist& h) {
    std::array<uint16_t, MAX_M> graph{};
    int symbol = 0;
    for (int pattern = 0; pattern < NPAT; ++pattern) {
        for (int copy = 0; copy < h[pattern]; ++copy) {
            for (int row = 0; row < C; ++row) {
                if ((pattern >> row) & 1) graph[2 * row] |= (uint16_t)(1u << symbol);
            }
            ++symbol;
        }
    }
    if (symbol != M) throw std::runtime_error("histogram does not contain 2C symbols");
    for (int row = 0; row < C; ++row) {
        graph[2 * row + 1] = (uint16_t)(FULL ^ graph[2 * row]);
        if (std::popcount(graph[2 * row]) != C) throw std::runtime_error("unbalanced skeleton row");
    }
    return graph;
}

uint64_t perfectMatchingCount(const std::array<uint16_t, MAX_M>& graph) {
    std::array<uint64_t, 1 << MAX_M> dp{};
    dp[0] = 1;
    const int all = (1 << M) - 1;
    for (int used = 0; used <= all; ++used) {
        const int row = std::popcount((unsigned)used);
        if (row >= M || dp[used] == 0) continue;
        uint16_t available = graph[row] & (uint16_t)~used;
        while (available) {
            const uint16_t bit = available & (uint16_t)(-available);
            available ^= bit;
            dp[used | bit] += dp[used];
        }
    }
    return dp[all];
}

int graphComponentCount(const std::array<uint16_t, MAX_M>& graph) {
    std::array<int, 2 * MAX_M> parent{};
    for (int i = 0; i < 2 * M; ++i) parent[i] = i;
    auto root = [&](int x) {
        while (parent[x] != x) {
            parent[x] = parent[parent[x]];
            x = parent[x];
        }
        return x;
    };
    int components = 2 * M;
    for (int left = 0; left < M; ++left) {
        uint16_t bits = graph[left];
        while (bits) {
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

std::string expectedValue(int c) {
    if (c == 2) return "288";
    if (c == 3) return "28200960";
    if (c == 4) return "29136487207403520";
    if (c == 5) return "1903816047972624930994913280000";
    return {};
}

} // namespace

int main(int argc, char** argv) {
    C = argc > 1 ? std::atoi(argv[1]) : 4;
    if (C < 2 || C > MAX_C) {
        std::fprintf(stderr, "supported range is 2 <= C <= %d\n", MAX_C);
        return 2;
    }
    M = 2 * C;
    FULL = (1 << M) - 1;
    NPAT = 1 << C;

    bool outerOnly = false;
    bool inspectOnly = false;
    int limit = -1;
    int startClass = 0;
    for (int i = 2; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "outer") outerOnly = true;
        else if (arg == "inspect") inspectOnly = true;
        else if (arg == "progress") verboseProbeProgress = true;
        else if (arg == "d3iso") useDegree3IsoBatch = true;
        else if (arg == "d3pairs") {
            useDegree3IsoBatch = true;
            useDegree3PairIso = true;
        }
        else if (arg == "parallelparents") parallelDegree4ParentEnumeration = true;
        else if (arg == "rooted4") useDegree4RootedSplit = true;
        else if (arg == "pivot") colorPivotMaxDegree = MAX_C;
        else if (arg == "pivotinner") colorPivotMaxDegree = 5;
        else if (arg == "rooted2probe") rootedTwoFactorProbes[5] = 1;
        else if (arg.rfind("rooted2probes=", 0) == 0) {
            rootedTwoFactorProbes[5] = std::max(0, std::atoi(arg.c_str() + 14));
        }
        else if (arg == "rooted2probe4") rootedTwoFactorProbes[4] = 1;
        else if (arg.rfind("rooted2probes4=", 0) == 0) {
            rootedTwoFactorProbes[4] = std::max(0, std::atoi(arg.c_str() + 15));
        }
        else if (arg.rfind("limit=", 0) == 0) limit = std::atoi(arg.c_str() + 6);
        else if (arg.rfind("start=", 0) == 0) startClass = std::atoi(arg.c_str() + 6);
        else if (arg.rfind("canonbudget=", 0) == 0) {
            canonNodeBudget = std::strtoull(arg.c_str() + 12, nullptr, 10);
        }
        else if (arg.rfind("canoncachecap=", 0) == 0) {
            canonCacheCapPerDegree = (size_t)std::strtoull(arg.c_str() + 14, nullptr, 10);
        }
        else if (arg.rfind("canoncachecap3=", 0) == 0) {
            requestedCanonCacheCaps[3] = (size_t)std::strtoull(arg.c_str() + 15, nullptr, 10);
        }
        else if (arg.rfind("canoncachecap4=", 0) == 0) {
            requestedCanonCacheCaps[4] = (size_t)std::strtoull(arg.c_str() + 15, nullptr, 10);
        }
        else if (arg.rfind("canoncachecap5=", 0) == 0) {
            requestedCanonCacheCaps[5] = (size_t)std::strtoull(arg.c_str() + 15, nullptr, 10);
        }
        else if (arg.rfind("checkpoint=", 0) == 0) {
            graphCheckpointPath = arg.substr(11);
        }
        else if (arg == "checkpointreadonly") graphCheckpointReadOnly = true;
        else if (arg.rfind("checkpointinterval=", 0) == 0) {
            graphCheckpointInterval = std::max(0, std::atoi(arg.c_str() + 19));
        }
        else if (arg.rfind("checkpointparents=", 0) == 0) {
            graphCheckpointParentInterval = (size_t)std::strtoull(
                arg.c_str() + 18, nullptr, 10);
        }
        else if (arg.rfind("parentchunk=", 0) == 0) {
            degree4ParentChunk = std::max<size_t>(
                1, (size_t)std::strtoull(arg.c_str() + 12, nullptr, 10));
        }
        else if (arg.rfind("d3isobudget=", 0) == 0) {
            degree3IsoNodeBudget = std::strtoull(arg.c_str() + 12, nullptr, 10);
        }
    }

    try {
        const auto outerStart = std::chrono::steady_clock::now();
        std::vector<OuterClass> classes = enumerateOuterClasses();
        const double outerSeconds = std::chrono::duration<double>(
            std::chrono::steady_clock::now() - outerStart).count();
        std::fprintf(stderr, "outer generation time=%.3fs\n", outerSeconds);
        if (outerOnly) return 0;
        if (inspectOnly) {
            if (limit <= 0) limit = 1;
            startClass = std::clamp(startClass, 0, (int)classes.size());
            const int endClass = std::min(startClass + limit, (int)classes.size());
            for (int i = startClass; i < endClass; ++i) {
                const auto graph = graphFromHistogram(classes[i].representative);
                std::printf("inspect class=%d/%zu orbit=%llu mult=%llu topPM=%llu components=%d\n",
                            i + 1, classes.size(),
                            (unsigned long long)classes[i].histogramOrbitSize,
                            (unsigned long long)classes[i].labelledMultiplicity,
                            (unsigned long long)perfectMatchingCount(graph),
                            graphComponentCount(graph));
            }
            return 0;
        }
        if (C == 6 && limit <= 0) {
            throw std::runtime_error(
                "full C=6 counting is intentionally disabled; use a positive limit= for a bounded probe");
        }

        graphMemo.reserve(C >= 5 ? 1000000 : 10000);
        for (int degree = 3; degree <= C; ++degree) {
            DegreeCanonCache& cache = canonCacheByDegree[degree];
            cache.cap = requestedCanonCacheCaps[degree] != 0
                ? requestedCanonCacheCaps[degree] : canonCacheCapPerDegree;
            const size_t reserveCount = cache.cap != 0
                ? std::min<size_t>(cache.cap, 500000)
                : (C >= 5 ? 500000 : 5000);
            cache.map.reserve(reserveCount);
        }
        loadGraphCheckpoint();
        unsigned __int128 answer = 0;
        startClass = std::clamp(startClass, 0, (int)classes.size());
        const int endClass = limit < 0 ? (int)classes.size()
                                       : std::min(startClass + limit, (int)classes.size());
        const int totalClasses = endClass - startClass;
        const auto countStart = std::chrono::steady_clock::now();
        for (int i = startClass; i < endClass; ++i) {
            OuterClass& oc = classes[i];
            const auto graph = graphFromHistogram(oc.representative);
            const auto classStart = std::chrono::steady_clock::now();
            oc.factorizationCount = countFactorizations(graph, C);
            const double classSeconds = std::chrono::duration<double>(
                std::chrono::steady_clock::now() - classStart).count();
            if (C <= 5) {
                answer += (unsigned __int128)oc.labelledMultiplicity *
                          oc.factorizationCount * oc.factorizationCount;
            }
            const double elapsed = std::chrono::duration<double>(
                std::chrono::steady_clock::now() - countStart).count();
            const int completed = i - startClass + 1;
            const double eta = elapsed * (endClass - i - 1) / completed;
            const std::string factorizationString = u128ToString(oc.factorizationCount);
            std::fprintf(stderr,
                         "class %d/%zu sample=%d/%d orbit=%llu mult=%llu F=%s class=%.3fs elapsed=%.3fs ETA=%.3fs "
                         "memo=%zu hit=%llu miss=%llu PM=%llu canonCache=%zu evict=%llu canon=%llu fallback=%llu nodes=%llu\n",
                         i + 1, classes.size(), completed, totalClasses,
                         (unsigned long long)oc.histogramOrbitSize,
                         (unsigned long long)oc.labelledMultiplicity,
                         factorizationString.c_str(),
                         classSeconds, elapsed, eta, graphMemo.size(),
                         (unsigned long long)graphMemoHits,
                         (unsigned long long)graphMemoMisses,
                         (unsigned long long)perfectMatchings,
                         canonCacheSize(),
                         (unsigned long long)canonCacheEvictions,
                         (unsigned long long)canonComputations,
                         (unsigned long long)canonFallbacks,
                         (unsigned long long)canonSearchNodes);
            std::fflush(stderr);
        }
        if (!graphCheckpointPath.empty()) saveGraphCheckpoint();

        const double countSeconds = std::chrono::duration<double>(
            std::chrono::steady_clock::now() - countStart).count();
        if (C <= 5) {
            std::printf("C=%d classes=%d/%zu N=%s\n", C, totalClasses, classes.size(),
                        u128ToString(answer).c_str());
        } else {
            std::printf("C=%d bounded probe start=%d classes=%d/%zu (no full-N accumulation)\n",
                        C, startClass, totalClasses, classes.size());
        }
        std::printf("stats countTime=%.6fs outerTime=%.6fs graphMemo=%zu calls=%llu hits=%llu misses=%llu PM=%llu "
                    "canonCache=%zu canonCacheEvictions=%llu canonComputations=%llu canonCacheHits=%llu canonFallbacks=%llu canonNodes=%llu "
                    "pivotCalls=%llu pivotAll=%llu pivotSelected=%llu "
                    "rooted4Calls=%llu rooted4Leaves=%llu rooted4Nodes=%llu "
                    "d3DiscreteInputs=%llu d3DiscreteHits=%llu d3DiscreteClasses=%llu "
                    "d3IsoRep=%llu d3IsoHits=%llu d3IsoChecks=%llu d3IsoUnknown=%llu d3IsoNodes=%llu d3IsoKnownKeyMisses=%llu\n",
                    countSeconds, outerSeconds, graphMemo.size(),
                    (unsigned long long)graphCalls,
                    (unsigned long long)graphMemoHits,
                    (unsigned long long)graphMemoMisses,
                    (unsigned long long)perfectMatchings,
                    canonCacheSize(),
                    (unsigned long long)canonCacheEvictions,
                    (unsigned long long)canonComputations,
                    (unsigned long long)canonCacheHits,
                    (unsigned long long)canonFallbacks,
                    (unsigned long long)canonSearchNodes,
                    (unsigned long long)pivotCalls,
                    (unsigned long long)pivotAllMatchings,
                    (unsigned long long)pivotSelectedMatchings,
                    (unsigned long long)degree4RootedCalls,
                    (unsigned long long)degree4RootedLeaves,
                    (unsigned long long)degree4RootedNodes,
                    (unsigned long long)degree3DiscreteInputs,
                    (unsigned long long)degree3DiscreteHits,
                    (unsigned long long)degree3DiscreteClasses,
                    (unsigned long long)degree3IsoRepresentatives,
                    (unsigned long long)degree3IsoHits,
                    (unsigned long long)degree3IsoChecks,
                    (unsigned long long)degree3IsoUnknown,
                    (unsigned long long)degree3IsoNodes,
                    (unsigned long long)degree3IsoKnownKeyMisses);
        if (limit < 0 && startClass == 0) {
            const std::string expected = expectedValue(C);
            if (!expected.empty()) {
                const std::string actual = u128ToString(answer);
                std::printf("expected=%s [%s]\n", expected.c_str(), actual == expected ? "OK" : "MISMATCH");
                if (actual != expected) return 1;
            }
        }
    } catch (const std::exception& e) {
        std::fprintf(stderr, "fatal: %s\n", e.what());
        return 1;
    }
    return 0;
}
