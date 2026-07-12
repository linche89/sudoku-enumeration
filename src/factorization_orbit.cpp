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
#include <functional>
#include <limits>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace {

constexpr int MAX_C = 5;
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
    uint64_t factorizationCount = 0;
};

struct Vec {
    std::array<uint8_t, 1 << (MAX_C - 1)> counts{};
    uint64_t marginal = 0;
};

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

    const int expectedClasses = C == 2 ? 2 : C == 3 ? 4 : C == 4 ? 26 : C == 5 ? 355 : -1;
    if (expectedClasses >= 0 && (int)classes.size() != expectedClasses) {
        throw std::runtime_error("outer orbit class count failed");
    }
    return classes;
}

struct GraphKey {
    uint64_t lo = 0;
    uint64_t hi = 0;
    bool operator==(const GraphKey&) const = default;
};

struct GraphKeyHash {
    size_t operator()(const GraphKey& k) const noexcept {
        uint64_t x = k.lo ^ std::rotl(k.hi, 23) ^ 0x9e3779b97f4a7c15ULL;
        x ^= x >> 30;
        x *= 0xbf58476d1ce4e5b9ULL;
        x ^= x >> 27;
        x *= 0x94d049bb133111ebULL;
        x ^= x >> 31;
        return (size_t)x;
    }
};

GraphKey weakGraphKey(const std::array<uint16_t, MAX_M>& graph) {
    std::array<uint16_t, MAX_M> rows = graph;
    std::sort(rows.begin(), rows.begin() + M);
    unsigned __int128 packed = 0;
    for (int i = 0; i < M; ++i) packed = (packed << M) | rows[i];
    return {(uint64_t)packed, (uint64_t)(packed >> 64)};
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
constexpr uint64_t CANON_NODE_BUDGET = 20000;

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

unsigned __int128 encodeDiscrete(const std::array<uint16_t, MAX_M>& graph,
                                 const CanonState& state) {
    std::array<int, MAX_M> rowAt{};
    std::array<int, MAX_M> colAt{};
    for (int v = 0; v < M; ++v) {
        rowAt[state.rowColor[v]] = v;
        colAt[state.colColor[v]] = v;
    }
    unsigned __int128 code = 0;
    for (int r = 0; r < M; ++r) {
        const int originalRow = rowAt[r];
        for (int c = 0; c < M; ++c) {
            code <<= 1;
            code |= (graph[originalRow] >> colAt[c]) & 1u;
        }
    }
    return code;
}

void canonicalSearch(const std::array<uint16_t, MAX_M>& graph,
                     CanonState state,
                     unsigned __int128& best,
                     bool& haveBest,
                     uint64_t& nodesThisCall,
                     bool& aborted) {
    if (aborted) return;
    if (++nodesThisCall > CANON_NODE_BUDGET) {
        aborted = true;
        return;
    }
    ++canonSearchNodes;
    refine(graph, state);
    if (state.rowColors == M && state.colColors == M) {
        const unsigned __int128 code = encodeDiscrete(graph, state);
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

std::unordered_map<GraphKey, GraphKey, GraphKeyHash> canonCache;

GraphKey canonicalGraphKey(const std::array<uint16_t, MAX_M>& graph) {
    const GraphKey weak = weakGraphKey(graph);
    auto found = canonCache.find(weak);
    if (found != canonCache.end()) {
        ++canonCacheHits;
        return found->second;
    }
    ++canonComputations;
    CanonState initial;
    unsigned __int128 best = 0;
    bool haveBest = false;
    uint64_t nodesThisCall = 0;
    bool aborted = false;
    canonicalSearch(graph, initial, best, haveBest, nodesThisCall, aborted);
    GraphKey result;
    if (aborted) {
        ++canonFallbacks;
        result = weak;
        result.hi |= (uint64_t)1 << 63; // disjoint namespace from strong keys
    } else {
        if (!haveBest) throw std::runtime_error("bipartite graph canonicalization failed");
        result = {(uint64_t)best, (uint64_t)(best >> 64)};
    }
    canonCache.emplace(weak, result);
    return result;
}

struct Residual {
    std::array<uint16_t, MAX_M> graph{};
    uint32_t multiplicity = 0;
};

std::unordered_map<GraphKey, uint64_t, GraphKeyHash> graphMemo;
uint64_t graphCalls = 0;
uint64_t graphMemoHits = 0;
uint64_t graphMemoMisses = 0;
uint64_t perfectMatchings = 0;

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

uint64_t countFactorizations(const std::array<uint16_t, MAX_M>& graph, int degree);

void enumeratePerfectMatchings(
    const std::array<uint16_t, MAX_M>& graph,
    uint16_t remainingLeft,
    uint16_t usedRight,
    std::array<uint16_t, MAX_M>& chosen,
    std::unordered_map<GraphKey, size_t, GraphKeyHash>& residualIndex,
    std::vector<Residual>& residuals) {
    if (remainingLeft == 0) {
        ++perfectMatchings;
        std::array<uint16_t, MAX_M> residual{};
        for (int i = 0; i < M; ++i) residual[i] = graph[i] ^ chosen[i];
        const GraphKey key = canonicalGraphKey(residual);
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

uint64_t countFactorizations(const std::array<uint16_t, MAX_M>& input, int degree) {
    ++graphCalls;
    if (degree == 1) return 1;
    if (degree == 2) return (uint64_t)1 << cycleComponents(input);

    std::array<uint16_t, MAX_M> graph = input;
    std::sort(graph.begin(), graph.begin() + M);
    const GraphKey key = canonicalGraphKey(graph);
    auto memoIt = graphMemo.find(key);
    if (memoIt != graphMemo.end()) {
        ++graphMemoHits;
        return memoIt->second;
    }
    ++graphMemoMisses;

    std::unordered_map<GraphKey, size_t, GraphKeyHash> residualIndex;
    std::vector<Residual> residuals;
    residualIndex.reserve(256);
    residuals.reserve(256);
    std::array<uint16_t, MAX_M> chosen{};
    enumeratePerfectMatchings(graph, (uint16_t)((1u << M) - 1u), 0,
                              chosen, residualIndex, residuals);

    unsigned __int128 total = 0;
    for (const Residual& residual : residuals) {
        total += (unsigned __int128)residual.multiplicity *
                 countFactorizations(residual.graph, degree - 1);
    }
    if (total > std::numeric_limits<uint64_t>::max()) {
        throw std::overflow_error("factorization count does not fit uint64_t");
    }
    const uint64_t value = (uint64_t)total;
    graphMemo.emplace(key, value);
    return value;
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
    int limit = -1;
    for (int i = 2; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "outer") outerOnly = true;
        else if (arg.rfind("limit=", 0) == 0) limit = std::atoi(arg.c_str() + 6);
    }

    try {
        const auto outerStart = std::chrono::steady_clock::now();
        std::vector<OuterClass> classes = enumerateOuterClasses();
        const double outerSeconds = std::chrono::duration<double>(
            std::chrono::steady_clock::now() - outerStart).count();
        std::fprintf(stderr, "outer generation time=%.3fs\n", outerSeconds);
        if (outerOnly) return 0;

        graphMemo.reserve(C == 5 ? 1000000 : 10000);
        canonCache.reserve(C == 5 ? 1000000 : 10000);
        unsigned __int128 answer = 0;
        const int totalClasses = limit < 0 ? (int)classes.size() : std::min(limit, (int)classes.size());
        const auto countStart = std::chrono::steady_clock::now();
        for (int i = 0; i < totalClasses; ++i) {
            OuterClass& oc = classes[i];
            const auto graph = graphFromHistogram(oc.representative);
            const auto classStart = std::chrono::steady_clock::now();
            oc.factorizationCount = countFactorizations(graph, C);
            const double classSeconds = std::chrono::duration<double>(
                std::chrono::steady_clock::now() - classStart).count();
            answer += (unsigned __int128)oc.labelledMultiplicity *
                      oc.factorizationCount * oc.factorizationCount;
            const double elapsed = std::chrono::duration<double>(
                std::chrono::steady_clock::now() - countStart).count();
            const double eta = (i + 1) > 0 ? elapsed * (totalClasses - i - 1) / (i + 1) : 0.0;
            std::fprintf(stderr,
                         "class %d/%d orbit=%llu mult=%llu F=%llu class=%.3fs elapsed=%.3fs ETA=%.3fs "
                         "memo=%zu hit=%llu miss=%llu PM=%llu canonCache=%zu canon=%llu fallback=%llu nodes=%llu\n",
                         i + 1, totalClasses,
                         (unsigned long long)oc.histogramOrbitSize,
                         (unsigned long long)oc.labelledMultiplicity,
                         (unsigned long long)oc.factorizationCount,
                         classSeconds, elapsed, eta, graphMemo.size(),
                         (unsigned long long)graphMemoHits,
                         (unsigned long long)graphMemoMisses,
                         (unsigned long long)perfectMatchings,
                         canonCache.size(),
                         (unsigned long long)canonComputations,
                         (unsigned long long)canonFallbacks,
                         (unsigned long long)canonSearchNodes);
            std::fflush(stderr);
        }

        const double countSeconds = std::chrono::duration<double>(
            std::chrono::steady_clock::now() - countStart).count();
        std::printf("C=%d classes=%d/%zu N=%s\n", C, totalClasses, classes.size(),
                    u128ToString(answer).c_str());
        std::printf("stats countTime=%.6fs outerTime=%.6fs graphMemo=%zu calls=%llu hits=%llu misses=%llu PM=%llu "
                    "canonCache=%zu canonComputations=%llu canonCacheHits=%llu canonFallbacks=%llu canonNodes=%llu\n",
                    countSeconds, outerSeconds, graphMemo.size(),
                    (unsigned long long)graphCalls,
                    (unsigned long long)graphMemoHits,
                    (unsigned long long)graphMemoMisses,
                    (unsigned long long)perfectMatchings,
                    canonCache.size(),
                    (unsigned long long)canonComputations,
                    (unsigned long long)canonCacheHits,
                    (unsigned long long)canonFallbacks,
                    (unsigned long long)canonSearchNodes);
        if (limit < 0) {
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
