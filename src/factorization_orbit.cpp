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
    DegreeCanonCache& cache = canonCacheByDegree[regularGraphDegree(weakResiduals[0].graph)];
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
    #pragma omp parallel for schedule(dynamic, 64)
    for (long long j = 0; j < (long long)pending.size(); ++j) {
        const Pending& item = pending[(size_t)j];
        computed[(size_t)j] = computeCanonicalGraphKey(
            weakResiduals[item.firstIndex].graph, item.weak);
    }
    for (size_t j = 0; j < pending.size(); ++j) {
        ++canonComputations;
        canonSearchNodes += computed[j].nodes;
        if (computed[j].fallback) ++canonFallbacks;
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
std::array<uint64_t, MAX_C + 1> graphCallsByDegree{};
std::array<uint64_t, MAX_C + 1> graphMissesByDegree{};
std::array<uint64_t, MAX_C + 1> perfectMatchingsByDegree{};
std::string graphCheckpointPath;
int graphCheckpointInterval = 0;
size_t graphCheckpointParentInterval = 0;
size_t parentsSinceGraphCheckpoint = 0;
uint64_t completedDegree5 = 0;

void saveGraphCheckpoint() {
    if (graphCheckpointPath.empty()) return;
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

FactorCount computeDegree3Direct(const std::array<uint16_t, MAX_M>& graph) {
    std::unordered_map<GraphKey, size_t, GraphKeyHash> residualIndex;
    std::vector<Residual> residuals;
    residualIndex.reserve(512);
    residuals.reserve(512);
    std::array<uint16_t, MAX_M> chosen{};
    enumeratePerfectMatchings(graph, (uint16_t)((1u << M) - 1u), 0,
                              chosen, residualIndex, residuals);
    FactorCount total = 0;
    for (const Residual& residual : residuals) {
        total += (FactorCount)residual.multiplicity << cycleComponents(residual.graph);
    }
    return total;
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

    for (size_t chunkStart = 0; chunkStart < missingParents.size();
         chunkStart += degree4ParentChunk) {
        const size_t chunkEnd = std::min(chunkStart + degree4ParentChunk,
                                         missingParents.size());
        const size_t parentCount = chunkEnd - chunkStart;
        std::vector<size_t> offsets(parentCount + 1, 0);
        std::vector<Residual> flat;
        flat.reserve(parentCount * 2800);

        for (size_t local = 0; local < parentCount; ++local) {
            const size_t parent = missingParents[chunkStart + local];
            std::unordered_map<GraphKey, size_t, GraphKeyHash> residualIndex;
            std::vector<Residual> residuals;
            residualIndex.reserve(4096);
            residuals.reserve(4096);
            std::array<uint16_t, MAX_M> chosen{};
            enumeratePerfectMatchings(degree4Residuals[parent].graph,
                                      (uint16_t)((1u << M) - 1u), 0,
                                      chosen, residualIndex, residuals);
            flat.insert(flat.end(), residuals.begin(), residuals.end());
            offsets[local + 1] = flat.size();
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
            values[parent] = value;
            graphMemo.emplace(degree4Keys[parent], value);
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
                         "probe layeredF4 parents=%zu/%zu flatD3=%zu strongD3=%zu graphMemo=%zu cache=%zu\n",
                         chunkEnd, missingParents.size(), flat.size(), degree3Values.size(),
                         graphMemo.size(), canonCacheSize());
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

    std::unordered_map<GraphKey, size_t, GraphKeyHash> residualIndex;
    std::vector<Residual> residuals;
    residualIndex.reserve(256);
    residuals.reserve(256);
    std::array<uint16_t, MAX_M> chosen{};
    enumeratePerfectMatchings(graph, (uint16_t)((1u << M) - 1u), 0,
                              chosen, residualIndex, residuals);
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
                    "canonCache=%zu canonCacheEvictions=%llu canonComputations=%llu canonCacheHits=%llu canonFallbacks=%llu canonNodes=%llu\n",
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
                    (unsigned long long)canonSearchNodes);
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
