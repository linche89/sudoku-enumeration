// Exact box-order joint-histogram decision prototype for the squared 2xC
// objective.  This file is intentionally outside the standard build.
//
// After r complete row pairs, a symbol has used-color masks (S,T) in the two
// copies.  The exact symbol quotient is the joint histogram
//
//     h[S,T] = number of symbols of type (S,T).
//
// A transition assigns one new color a,b to every symbol, with every color
// used twice in each copy.  For a fixed labelled assignment (a_s,b_s), the
// number of shared balanced cuts is 2^kappa, where kappa is the number of
// connected components of the resulting 2-regular bipartite multigraph on
// the two color sets.  The contingency enumerator below groups labelled
// assignments exactly and applies that cycle weight at each leaf.
//
// States are quotiented by independent color permutations and copy swap.  DP
// values are orbit totals.  Therefore the midpoint contraction is
//
//   sum_[h] L([h]) R([complement(h)]) /
//           (orbit_size(h) * (2C)! / product h[S,T]!).
//
// C=2 through C=4 also run an independent labelled-symbol transition
// enumerator and compare every raw target coefficient before canonicalization.
// The C=5 extension is a bounded scale experiment: it disables the labelled
// enumerator, uses an exact occupied-anchor canonicalizer, and can stop after
// a deterministic prefix of source states.

#include <algorithm>
#include <array>
#include <bit>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <functional>
#include <iomanip>
#include <iostream>
#include <limits>
#include <numeric>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace {

constexpr int MAX_C = 5;
constexpr int MAX_MASK = 1 << MAX_C;
constexpr int MAX_HIST = MAX_MASK * MAX_MASK;
constexpr int MAX_ACTIVE_TYPES = 100;  // binom(5,2)^2
constexpr int KEY_WORDS = (4 * MAX_ACTIVE_TYPES + 63) / 64;

using Count = unsigned __int128;
using Hist = std::array<std::uint8_t, MAX_HIST>;

std::string countToString(Count value) {
    if (value == 0) return "0";
    std::string out;
    while (value != 0) {
        out.push_back(static_cast<char>('0' + value % 10));
        value /= 10;
    }
    std::reverse(out.begin(), out.end());
    return out;
}

Count parseCount(const std::string& text) {
    Count value = 0;
    for (char ch : text) {
        if (ch < '0' || ch > '9') throw std::runtime_error("invalid decimal");
        value = value * 10 + static_cast<unsigned>(ch - '0');
    }
    return value;
}

struct Key {
    std::array<std::uint64_t, KEY_WORDS> words{};
    bool operator==(const Key& other) const noexcept { return words == other.words; }
};

std::string keyToHex(const Key& key) {
    std::ostringstream out;
    out << std::hex << std::setfill('0');
    for (std::uint64_t word : key.words) out << std::setw(16) << word;
    return out.str();
}

bool keyLess(const Key& left, const Key& right) noexcept {
    return std::lexicographical_compare(left.words.begin(), left.words.end(),
                                        right.words.begin(), right.words.end());
}

struct KeyHash {
    std::size_t operator()(const Key& key) const noexcept {
        std::uint64_t h = 0x9e3779b97f4a7c15ULL;
        for (std::uint64_t x : key.words) {
            x ^= x >> 30;
            x *= 0xbf58476d1ce4e5b9ULL;
            x ^= x >> 27;
            x *= 0x94d049bb133111ebULL;
            x ^= x >> 31;
            h ^= x + 0x9e3779b97f4a7c15ULL + (h << 6) + (h >> 2);
        }
        return static_cast<std::size_t>(h);
    }
};

using CountMap = std::unordered_map<Key, Count, KeyHash>;

struct ScaleLimit : std::runtime_error {
    using std::runtime_error::runtime_error;
};

class StateSpace {
public:
    StateSpace(int c, bool copySwap) : c_(c), full_((1 << c) - 1), copySwap_(copySwap) {
        if (c < 2 || c > MAX_C) throw std::runtime_error("C must lie in [2,5]");
        for (int layer = 0; layer <= c_; ++layer) {
            positions_[layer].fill(-1);
            for (int mask = 0; mask <= full_; ++mask) {
                if (std::popcount(static_cast<unsigned>(mask)) == layer) {
                    positions_[layer][mask] = static_cast<int>(masks_[layer].size());
                    masks_[layer].push_back(mask);
                }
            }
            const int typeCount = activeTypeCount(layer);
            if (typeCount > MAX_ACTIVE_TYPES) {
                throw std::runtime_error("active key capacity exceeded");
            }
        }

        std::vector<int> p(c_);
        std::iota(p.begin(), p.end(), 0);
        do {
            std::array<std::uint8_t, MAX_MASK> image{};
            for (int mask = 0; mask <= full_; ++mask) {
                int mapped = 0;
                for (int bit = 0; bit < c_; ++bit) {
                    if (mask & (1 << bit)) mapped |= 1 << p[bit];
                }
                image[mask] = static_cast<std::uint8_t>(mapped);
            }
            maskImages_.push_back(image);
        } while (std::next_permutation(p.begin(), p.end()));

        buildTransforms();
    }

    int c() const { return c_; }
    int fullMask() const { return full_; }
    int groupImageCount() const {
        const int base = static_cast<int>(maskImages_.size() * maskImages_.size());
        return copySwap_ ? 2 * base : base;
    }
    const std::vector<int>& masks(int layer) const { return masks_[layer]; }
    int activeTypeCount(int layer) const {
        const int n = static_cast<int>(masks_[layer].size());
        return n * n;
    }

    Key encode(const Hist& hist, int layer) const {
        Key key;
        int pos = 0;
        for (int s : masks_[layer]) {
            for (int t : masks_[layer]) {
                const std::uint8_t value = hist[index(s, t)];
                if (value > 2 * c_) throw std::runtime_error("histogram count exceeds key nibble");
                key.words[pos / 16] |= static_cast<std::uint64_t>(value) << (4 * (pos % 16));
                ++pos;
            }
        }
        return key;
    }

    Hist decode(const Key& key, int layer) const {
        Hist hist{};
        int pos = 0;
        for (int s : masks_[layer]) {
            for (int t : masks_[layer]) {
                hist[index(s, t)] = static_cast<std::uint8_t>(
                    (key.words[pos / 16] >> (4 * (pos % 16))) & 0xFULL);
                ++pos;
            }
        }
        return hist;
    }

    Key canonical(const Key& key, int layer) const {
        const int typeCount = activeTypeCount(layer);
        std::array<std::uint8_t, MAX_ACTIVE_TYPES> source{};
        unpackActive(key, typeCount, source);

        std::vector<std::pair<int, std::uint8_t>> nonzero;
        nonzero.reserve(2 * c_);
        for (int pos = 0; pos < typeCount; ++pos) {
            if (source[pos] != 0) nonzero.emplace_back(pos, source[pos]);
        }

        Key best;
        bool first = true;
        for (const auto& [anchorSource, anchorValue] : nonzero) {
            (void)anchorValue;
            for (std::uint16_t transformIndex : anchorTransforms_[layer][anchorSource]) {
                const Transform& transform = transforms_[layer][transformIndex];
                Key candidate;
                for (const auto& [pos, value] : nonzero) {
                    const int destination = transform.destination[pos];
                    candidate.words[destination / 16] |=
                        static_cast<std::uint64_t>(value)
                        << (4 * (destination % 16));
                }
                ++canonicalCandidates_;
                if (first || activeKeyGreater(candidate, best, typeCount)) {
                    best = candidate;
                    first = false;
                }
            }
        }
        ++canonicalCalls_;
        if (first) throw std::runtime_error("cannot canonicalize an empty histogram");
        return best;
    }

    Key canonicalFull(const Key& key, int layer) const {
        const int typeCount = activeTypeCount(layer);
        std::array<std::uint8_t, MAX_ACTIVE_TYPES> source{};
        unpackActive(key, typeCount, source);

        std::array<std::uint8_t, MAX_ACTIVE_TYPES> best{};
        std::array<std::uint8_t, MAX_ACTIVE_TYPES> candidate{};
        bool first = true;
        for (const Transform& transform : transforms_[layer]) {
            candidate.fill(0);
            for (int pos = 0; pos < typeCount; ++pos) {
                candidate[transform.destination[pos]] = source[pos];
            }
            ++fullCanonicalCandidates_;
            if (first || std::lexicographical_compare(
                             best.begin(), best.begin() + typeCount,
                             candidate.begin(), candidate.begin() + typeCount)) {
                best = candidate;
                first = false;
            }
        }
        ++fullCanonicalCalls_;
        if (first) throw std::runtime_error("failed full canonicalization");
        return packActive(best, typeCount);
    }

    Key canonical(const Hist& hist, int layer) const { return canonical(encode(hist, layer), layer); }

    Key transform(const Key& key, int layer, int transformIndex) const {
        const int typeCount = activeTypeCount(layer);
        std::array<std::uint8_t, MAX_ACTIVE_TYPES> source{};
        std::array<std::uint8_t, MAX_ACTIVE_TYPES> target{};
        unpackActive(key, typeCount, source);
        const Transform& transform = transforms_[layer][transformIndex];
        for (int pos = 0; pos < typeCount; ++pos) {
            target[transform.destination[pos]] = source[pos];
        }
        return packActive(target, typeCount);
    }

    std::vector<Key> distinctImages(const Key& key, int layer) const {
        std::unordered_set<Key, KeyHash> seen;
        seen.reserve(transforms_[layer].size());
        for (int i = 0; i < static_cast<int>(transforms_[layer].size()); ++i) {
            seen.insert(transform(key, layer, i));
        }
        std::vector<Key> out;
        out.reserve(seen.size());
        for (const Key& image : seen) out.push_back(image);
        return out;
    }

    std::size_t orbitSize(const Key& key, int layer) const {
        return distinctImages(key, layer).size();
    }

    Key complement(const Key& key, int layer) const {
        const Hist source = decode(key, layer);
        Hist target{};
        for (int s : masks_[layer]) {
            for (int t : masks_[layer]) {
                target[index(full_ ^ s, full_ ^ t)] = source[index(s, t)];
            }
        }
        return canonical(target, c_ - layer);
    }

    void validateHistogram(const Key& key, int layer) const {
        const Hist hist = decode(key, layer);
        int mass = 0;
        for (int s : masks_[layer]) {
            for (int t : masks_[layer]) mass += hist[index(s, t)];
        }
        if (mass != 2 * c_) throw std::runtime_error("histogram mass mismatch");

        for (int color = 0; color < c_; ++color) {
            int first = 0;
            int second = 0;
            for (int s : masks_[layer]) {
                for (int t : masks_[layer]) {
                    const int count = hist[index(s, t)];
                    if (s & (1 << color)) first += count;
                    if (t & (1 << color)) second += count;
                }
            }
            if (first != 2 * layer || second != 2 * layer) {
                throw std::runtime_error("used-color marginal mismatch");
            }
        }
    }

    std::uint64_t symbolAssignments(const Key& key, int layer) const {
        const Hist hist = decode(key, layer);
        std::uint64_t value = factorial(2 * c_);
        for (int s : masks_[layer]) {
            for (int t : masks_[layer]) value /= factorial(hist[index(s, t)]);
        }
        return value;
    }

    static int index(int s, int t) { return s * MAX_MASK + t; }

    struct CanonicalCounters {
        std::uint64_t calls = 0;
        std::uint64_t candidates = 0;
        std::uint64_t fullCalls = 0;
        std::uint64_t fullCandidates = 0;
    };

    CanonicalCounters canonicalCounters() const {
        return {canonicalCalls_, canonicalCandidates_, fullCanonicalCalls_,
                fullCanonicalCandidates_};
    }

private:
    struct Transform {
        std::array<std::uint8_t, MAX_ACTIVE_TYPES> destination{};
    };

    static std::uint64_t factorial(int n) {
        std::uint64_t value = 1;
        for (int i = 2; i <= n; ++i) value *= static_cast<std::uint64_t>(i);
        return value;
    }

    void unpackActive(const Key& key, int count,
                      std::array<std::uint8_t, MAX_ACTIVE_TYPES>& values) const {
        values.fill(0);
        for (int pos = 0; pos < count; ++pos) {
            values[pos] = static_cast<std::uint8_t>(
                (key.words[pos / 16] >> (4 * (pos % 16))) & 0xFULL);
        }
    }

    Key packActive(const std::array<std::uint8_t, MAX_ACTIVE_TYPES>& values,
                   int count) const {
        Key key;
        for (int pos = 0; pos < count; ++pos) {
            key.words[pos / 16] |=
                static_cast<std::uint64_t>(values[pos]) << (4 * (pos % 16));
        }
        return key;
    }

    static bool activeKeyGreater(const Key& left, const Key& right, int count) {
        for (int pos = 0; pos < count; ++pos) {
            const std::uint8_t leftValue = static_cast<std::uint8_t>(
                (left.words[pos / 16] >> (4 * (pos % 16))) & 0xFULL);
            const std::uint8_t rightValue = static_cast<std::uint8_t>(
                (right.words[pos / 16] >> (4 * (pos % 16))) & 0xFULL);
            if (leftValue != rightValue) return leftValue > rightValue;
        }
        return false;
    }

    void buildTransforms() {
        for (int layer = 0; layer <= c_; ++layer) {
            const int n = static_cast<int>(masks_[layer].size());
            const int typeCount = n * n;
            transforms_[layer].reserve(groupImageCount());
            for (const auto& first : maskImages_) {
                for (const auto& second : maskImages_) {
                    Transform direct;
                    for (int i = 0; i < n; ++i) {
                        for (int j = 0; j < n; ++j) {
                            const int mappedI = positions_[layer][first[masks_[layer][i]]];
                            const int mappedJ = positions_[layer][second[masks_[layer][j]]];
                            direct.destination[i * n + j] =
                                static_cast<std::uint8_t>(mappedI * n + mappedJ);
                        }
                    }
                    transforms_[layer].push_back(direct);

                    if (copySwap_) {
                        Transform swapped;
                        for (int i = 0; i < n; ++i) {
                            for (int j = 0; j < n; ++j) {
                                const int mappedI = positions_[layer][first[masks_[layer][j]]];
                                const int mappedJ = positions_[layer][second[masks_[layer][i]]];
                                swapped.destination[i * n + j] =
                                    static_cast<std::uint8_t>(mappedI * n + mappedJ);
                            }
                        }
                        transforms_[layer].push_back(swapped);
                    }
                }
            }
            if (typeCount == 0 || transforms_[layer].empty()) {
                throw std::runtime_error("failed to construct color action");
            }
            if (transforms_[layer].size() > std::numeric_limits<std::uint16_t>::max()) {
                throw std::runtime_error("transform index capacity exceeded");
            }
            anchorTransforms_[layer].resize(typeCount);
            for (int transformIndex = 0;
                 transformIndex < static_cast<int>(transforms_[layer].size());
                 ++transformIndex) {
                const Transform& transform = transforms_[layer][transformIndex];
                int anchorSource = -1;
                for (int pos = 0; pos < typeCount; ++pos) {
                    if (transform.destination[pos] == 0) {
                        anchorSource = pos;
                        break;
                    }
                }
                if (anchorSource < 0) {
                    throw std::runtime_error("transform has no anchor preimage");
                }
                anchorTransforms_[layer][anchorSource].push_back(
                    static_cast<std::uint16_t>(transformIndex));
            }
        }
    }

    int c_;
    int full_;
    bool copySwap_;
    std::array<std::vector<int>, MAX_C + 1> masks_;
    std::array<std::array<int, MAX_MASK>, MAX_C + 1> positions_{};
    std::vector<std::array<std::uint8_t, MAX_MASK>> maskImages_;
    std::array<std::vector<Transform>, MAX_C + 1> transforms_;
    std::array<std::vector<std::vector<std::uint16_t>>, MAX_C + 1> anchorTransforms_;
    mutable std::uint64_t canonicalCalls_ = 0;
    mutable std::uint64_t canonicalCandidates_ = 0;
    mutable std::uint64_t fullCanonicalCalls_ = 0;
    mutable std::uint64_t fullCanonicalCandidates_ = 0;
};

struct TypeInfo {
    int s = 0;
    int t = 0;
    int multiplicity = 0;
    std::vector<std::pair<int, int>> cells;
};

class ContingencyTransition {
public:
    using BatchSink = std::function<void(const CountMap&)>;

    ContingencyTransition(const StateSpace& space, const Hist& source, int layer,
                          std::uint64_t leafLimit, std::uint64_t leafProbe,
                          std::size_t rawBatchSize, BatchSink batchSink,
                          bool countOnly)
        : space_(space), c_(space.c()), layer_(layer), leafLimit_(leafLimit),
          leafProbe_(leafProbe), rawBatchSize_(rawBatchSize),
          batchSink_(std::move(batchSink)), countOnly_(countOnly) {
        residualA_.fill(2);
        residualB_.fill(2);
        for (int i = 0; i <= 2 * c_; ++i) factorial_[i] = factorial(i);

        for (int s : space_.masks(layer_)) {
            for (int t : space_.masks(layer_)) {
                const int multiplicity = source[StateSpace::index(s, t)];
                if (multiplicity == 0) continue;
                TypeInfo type;
                type.s = s;
                type.t = t;
                type.multiplicity = multiplicity;
                for (int a = 0; a < c_; ++a) {
                    if (s & (1 << a)) continue;
                    for (int b = 0; b < c_; ++b) {
                        if ((t & (1 << b)) == 0) type.cells.emplace_back(a, b);
                    }
                }
                types_.push_back(std::move(type));
            }
        }
        std::sort(types_.begin(), types_.end(), [](const TypeInfo& x, const TypeInfo& y) {
            if (x.cells.size() != y.cells.size()) return x.cells.size() < y.cells.size();
            if (x.multiplicity != y.multiplicity) return x.multiplicity > y.multiplicity;
            if (x.s != y.s) return x.s < y.s;
            return x.t < y.t;
        });
        rawTargets_.reserve(1024);
    }

    void run() {
        if (countOnly_) {
            const Count exactLeaves = countTypes(0);
            if (exactLeaves > std::numeric_limits<std::uint64_t>::max()) {
                throw ScaleLimit("contingency leaf count exceeds uint64 capacity");
            }
            leaves_ = static_cast<std::uint64_t>(exactLeaves);
            if (leafLimit_ != 0 && leaves_ > leafLimit_) {
                throw ScaleLimit("contingency leaf limit exceeded by exact count-only DP: " +
                                 std::to_string(leaves_));
            }
            return;
        }
        try {
            recurseTypes(0, 1);
        } catch (const LeafProbeStop&) {
            leafProbeStopped_ = true;
        }
        flushRawTargets();
    }

    const CountMap& rawTargets() const { return rawTargets_; }
    std::uint64_t leaves() const { return leaves_; }
    std::uint64_t rawBatches() const { return rawBatches_; }
    std::uint64_t rawBatchEntries() const { return rawBatchEntries_; }
    bool leafProbeStopped() const { return leafProbeStopped_; }
    std::size_t countMemoStates() const { return countMemo_.size(); }

private:
    struct LeafProbeStop {};

    static std::uint64_t factorial(int n) {
        std::uint64_t value = 1;
        for (int i = 2; i <= n; ++i) value *= static_cast<std::uint64_t>(i);
        return value;
    }

    std::uint64_t countMemoKey(int typeIndex) const {
        std::uint64_t key = static_cast<std::uint64_t>(typeIndex);
        for (int color = 0; color < c_; ++color) {
            key = key * 3 + static_cast<std::uint64_t>(residualA_[color]);
        }
        for (int color = 0; color < c_; ++color) {
            key = key * 3 + static_cast<std::uint64_t>(residualB_[color]);
        }
        return key;
    }

    Count countTypes(int typeIndex) {
        if (typeIndex == static_cast<int>(types_.size())) {
            for (int color = 0; color < c_; ++color) {
                if (residualA_[color] != 0 || residualB_[color] != 0) return 0;
            }
            return 1;
        }
        if (!futureFeasible(typeIndex)) return 0;
        const std::uint64_t key = countMemoKey(typeIndex);
        auto found = countMemo_.find(key);
        if (found != countMemo_.end()) return found->second;
        const Count value = allocateCellsCount(typeIndex, 0,
                                               types_[typeIndex].multiplicity);
        countMemo_.emplace(key, value);
        return value;
    }

    Count allocateCellsCount(int typeIndex, int cellIndex, int unitsLeft) {
        const TypeInfo& type = types_[typeIndex];
        if (unitsLeft == 0) return countTypes(typeIndex + 1);
        if (cellIndex == static_cast<int>(type.cells.size())) return 0;

        std::array<bool, MAX_C> rows{};
        std::array<bool, MAX_C> columns{};
        for (int i = cellIndex; i < static_cast<int>(type.cells.size()); ++i) {
            rows[type.cells[i].first] = true;
            columns[type.cells[i].second] = true;
        }
        int rowCapacity = 0;
        int columnCapacity = 0;
        for (int a = 0; a < c_; ++a) if (rows[a]) rowCapacity += residualA_[a];
        for (int b = 0; b < c_; ++b) if (columns[b]) columnCapacity += residualB_[b];
        if (unitsLeft > std::min(rowCapacity, columnCapacity)) return 0;

        const auto [a, b] = type.cells[cellIndex];
        const int maximum = std::min({unitsLeft, residualA_[a], residualB_[b]});
        Count total = 0;
        for (int amount = 0; amount <= maximum; ++amount) {
            residualA_[a] -= amount;
            residualB_[b] -= amount;
            total += allocateCellsCount(typeIndex, cellIndex + 1,
                                        unitsLeft - amount);
            residualB_[b] += amount;
            residualA_[a] += amount;
        }
        return total;
    }

    bool futureFeasible(int typeIndex) const {
        for (int a = 0; a < c_; ++a) {
            int capacity = 0;
            for (int i = typeIndex; i < static_cast<int>(types_.size()); ++i) {
                if ((types_[i].s & (1 << a)) == 0) capacity += types_[i].multiplicity;
            }
            if (capacity < residualA_[a]) return false;
        }
        for (int b = 0; b < c_; ++b) {
            int capacity = 0;
            for (int i = typeIndex; i < static_cast<int>(types_.size()); ++i) {
                if ((types_[i].t & (1 << b)) == 0) capacity += types_[i].multiplicity;
            }
            if (capacity < residualB_[b]) return false;
        }
        return true;
    }

    void recurseTypes(int typeIndex, Count pathWeight) {
        if (typeIndex == static_cast<int>(types_.size())) {
            for (int color = 0; color < c_; ++color) {
                if (residualA_[color] != 0 || residualB_[color] != 0) return;
            }
            ++leaves_;
            if (leafLimit_ != 0 && leaves_ > leafLimit_) {
                throw ScaleLimit(
                    "contingency leaf limit exceeded: leaves=" +
                    std::to_string(leaves_) + " resident_raw_targets=" +
                    std::to_string(rawTargets_.size()) + " flushed_raw_entries=" +
                    std::to_string(rawBatchEntries_));
            }
            const int cycles = cycleComponents();
            const Key target = space_.encode(target_, layer_ + 1);
            rawTargets_[target] += pathWeight << cycles;
            if (rawBatchSize_ != 0 && rawTargets_.size() >= rawBatchSize_) {
                flushRawTargets();
            }
            if (leafProbe_ != 0 && leaves_ >= leafProbe_) {
                throw LeafProbeStop{};
            }
            return;
        }
        if (!futureFeasible(typeIndex)) return;
        const TypeInfo& type = types_[typeIndex];
        allocateCells(typeIndex, 0, type.multiplicity, 1, pathWeight);
    }

    void allocateCells(int typeIndex, int cellIndex, int unitsLeft,
                       std::uint64_t denominator, Count pathWeight) {
        const TypeInfo& type = types_[typeIndex];
        if (unitsLeft == 0) {
            const Count multinomial = factorial_[type.multiplicity] / denominator;
            recurseTypes(typeIndex + 1, pathWeight * multinomial);
            return;
        }
        if (cellIndex == static_cast<int>(type.cells.size())) return;

        std::array<bool, MAX_C> rows{};
        std::array<bool, MAX_C> columns{};
        for (int i = cellIndex; i < static_cast<int>(type.cells.size()); ++i) {
            rows[type.cells[i].first] = true;
            columns[type.cells[i].second] = true;
        }
        int rowCapacity = 0;
        int columnCapacity = 0;
        for (int a = 0; a < c_; ++a) if (rows[a]) rowCapacity += residualA_[a];
        for (int b = 0; b < c_; ++b) if (columns[b]) columnCapacity += residualB_[b];
        if (unitsLeft > std::min(rowCapacity, columnCapacity)) return;

        const auto [a, b] = type.cells[cellIndex];
        const int maximum = std::min({unitsLeft, residualA_[a], residualB_[b]});
        for (int amount = 0; amount <= maximum; ++amount) {
            residualA_[a] -= amount;
            residualB_[b] -= amount;
            target_[StateSpace::index(type.s | (1 << a), type.t | (1 << b))] += amount;
            pairMatrix_[a * MAX_C + b] += amount;

            allocateCells(typeIndex, cellIndex + 1, unitsLeft - amount,
                          denominator * factorial_[amount], pathWeight);

            pairMatrix_[a * MAX_C + b] -= amount;
            target_[StateSpace::index(type.s | (1 << a), type.t | (1 << b))] -= amount;
            residualB_[b] += amount;
            residualA_[a] += amount;
        }
    }

    int cycleComponents() const {
        std::array<int, 2 * MAX_C> parent{};
        for (int i = 0; i < 2 * c_; ++i) parent[i] = i;
        auto root = [&](int x) {
            while (parent[x] != x) {
                parent[x] = parent[parent[x]];
                x = parent[x];
            }
            return x;
        };
        for (int a = 0; a < c_; ++a) {
            for (int b = 0; b < c_; ++b) {
                if (pairMatrix_[a * MAX_C + b] == 0) continue;
                const int ra = root(a);
                const int rb = root(c_ + b);
                if (ra != rb) parent[rb] = ra;
            }
        }
        int components = 0;
        for (int vertex = 0; vertex < 2 * c_; ++vertex) {
            if (root(vertex) == vertex) ++components;
        }
        return components;
    }

    void flushRawTargets() {
        if (!batchSink_ || rawTargets_.empty()) return;
        ++rawBatches_;
        rawBatchEntries_ += rawTargets_.size();
        batchSink_(rawTargets_);
        rawTargets_.clear();
    }

    const StateSpace& space_;
    int c_;
    int layer_;
    std::uint64_t leafLimit_;
    std::uint64_t leafProbe_;
    std::size_t rawBatchSize_;
    BatchSink batchSink_;
    bool countOnly_;
    std::vector<TypeInfo> types_;
    std::array<int, MAX_C> residualA_{};
    std::array<int, MAX_C> residualB_{};
    std::array<std::uint64_t, 2 * MAX_C + 1> factorial_{};
    Hist target_{};
    std::array<int, MAX_C * MAX_C> pairMatrix_{};
    CountMap rawTargets_;
    std::uint64_t leaves_ = 0;
    std::uint64_t rawBatches_ = 0;
    std::uint64_t rawBatchEntries_ = 0;
    bool leafProbeStopped_ = false;
    std::unordered_map<std::uint64_t, Count> countMemo_;
};

class DirectTransition {
public:
    DirectTransition(const StateSpace& space, const Hist& source, int layer)
        : space_(space), c_(space.c()), layer_(layer) {
        residualA_.fill(2);
        residualB_.fill(2);
        for (int s : space_.masks(layer_)) {
            for (int t : space_.masks(layer_)) {
                for (int i = 0; i < source[StateSpace::index(s, t)]; ++i) {
                    symbols_.emplace_back(s, t);
                }
            }
        }
        std::sort(symbols_.begin(), symbols_.end());
    }

    void run() { recurse(0); }
    const CountMap& rawTargets() const { return rawTargets_; }
    std::uint64_t leaves() const { return leaves_; }

private:
    void recurse(int symbolIndex) {
        if (symbolIndex == static_cast<int>(symbols_.size())) {
            for (int color = 0; color < c_; ++color) {
                if (residualA_[color] != 0 || residualB_[color] != 0) return;
            }
            ++leaves_;
            const Key target = space_.encode(target_, layer_ + 1);
            rawTargets_[target] += static_cast<Count>(1) << cycleComponents();
            return;
        }

        const auto [s, t] = symbols_[symbolIndex];
        for (int a = 0; a < c_; ++a) {
            if ((s & (1 << a)) || residualA_[a] == 0) continue;
            --residualA_[a];
            for (int b = 0; b < c_; ++b) {
                if ((t & (1 << b)) || residualB_[b] == 0) continue;
                --residualB_[b];
                ++target_[StateSpace::index(s | (1 << a), t | (1 << b))];
                ++pairMatrix_[a * MAX_C + b];
                recurse(symbolIndex + 1);
                --pairMatrix_[a * MAX_C + b];
                --target_[StateSpace::index(s | (1 << a), t | (1 << b))];
                ++residualB_[b];
            }
            ++residualA_[a];
        }
    }

    int cycleComponents() const {
        std::array<int, 2 * MAX_C> parent{};
        for (int i = 0; i < 2 * c_; ++i) parent[i] = i;
        auto root = [&](int x) {
            while (parent[x] != x) {
                parent[x] = parent[parent[x]];
                x = parent[x];
            }
            return x;
        };
        for (int a = 0; a < c_; ++a) {
            for (int b = 0; b < c_; ++b) {
                if (pairMatrix_[a * MAX_C + b] == 0) continue;
                const int ra = root(a);
                const int rb = root(c_ + b);
                if (ra != rb) parent[rb] = ra;
            }
        }
        int components = 0;
        for (int vertex = 0; vertex < 2 * c_; ++vertex) {
            if (root(vertex) == vertex) ++components;
        }
        return components;
    }

    const StateSpace& space_;
    int c_;
    int layer_;
    std::vector<std::pair<int, int>> symbols_;
    std::array<int, MAX_C> residualA_{};
    std::array<int, MAX_C> residualB_{};
    Hist target_{};
    std::array<int, MAX_C * MAX_C> pairMatrix_{};
    CountMap rawTargets_;
    std::uint64_t leaves_ = 0;
};

void compareRawTransitions(const CountMap& exact, const CountMap& direct,
                           int c, int layer) {
    if (exact.size() != direct.size()) {
        throw std::runtime_error("direct transition target-count mismatch at C=" +
                                 std::to_string(c) + " layer=" + std::to_string(layer));
    }
    for (const auto& [key, value] : exact) {
        auto found = direct.find(key);
        if (found == direct.end() || found->second != value) {
            throw std::runtime_error("direct transition coefficient mismatch at C=" +
                                     std::to_string(c) + " layer=" + std::to_string(layer));
        }
    }
}

struct LayerStats {
    int layer = 0;
    std::size_t sourceStates = 0;
    std::size_t processedSourceStates = 0;
    std::size_t targetStates = 0;
    std::uint64_t contingencyLeaves = 0;
    std::uint64_t directLeaves = 0;
    std::uint64_t rawTargetOccurrences = 0;
    std::size_t globallyDistinctRawTargets = 0;
    std::uint64_t rawBatches = 0;
    std::uint64_t rawBatchEntries = 0;
    std::uint64_t orbitTransitions = 0;
    std::uint64_t canonicalChecks = 0;
    std::uint64_t canonicalCalls = 0;
    std::uint64_t canonicalCandidates = 0;
    std::uint64_t fullCanonicalCalls = 0;
    std::uint64_t fullCanonicalCandidates = 0;
    std::uint64_t countDpStates = 0;
    std::size_t maxCountDpStatesPerSource = 0;
    double seconds = 0;
    Count layerTotal = 0;
    bool closed = true;
    bool rawDistinctExact = true;
    bool leafProbeStopped = false;
    bool countOnly = false;
};

struct Options {
    int c = 0;
    int stopLayer = -1;
    std::size_t maxStates = 2'000'000;
    std::uint64_t maxLeavesPerSource = 0;
    std::size_t sourceProbe = 0;
    std::size_t sourceStart = 0;
    std::uint64_t leafProbe = 0;
    std::size_t rawBatchSize = 0;
    std::size_t canonicalCheckLimit = 0;
    bool directCheck = true;
    bool copySwap = true;
    bool countLeavesOnly = false;
};

struct RunResult {
    Count fullTotal = 0;
    Count midpointTotal = 0;
    bool completed = false;
    bool sourceProbeStopped = false;
    bool leafProbeStopped = false;
    bool countOnlyStopped = false;
    std::vector<LayerStats> stats;
};

void validateOrbitPartition(const StateSpace& space, const CountMap& states,
                            int layer, bool exhaustive) {
    std::unordered_map<Key, Key, KeyHash> owner;
    std::size_t checked = 0;
    const std::size_t maximum = exhaustive ? states.size() : std::min<std::size_t>(states.size(), 32);
    for (const auto& [key, value] : states) {
        (void)value;
        if (checked++ >= maximum) break;
        if (!(space.canonical(key, layer) == key)) {
            throw std::runtime_error("stored state is not canonical");
        }
        if (!(space.canonicalFull(key, layer) == key)) {
            throw std::runtime_error("anchored/full canonical differential failure");
        }
        space.validateHistogram(key, layer);
        const std::vector<Key> images = space.distinctImages(key, layer);
        for (const Key& image : images) {
            if (!(space.canonical(image, layer) == key)) {
                throw std::runtime_error("canonical invariance failure");
            }
            auto [it, inserted] = owner.emplace(image, key);
            if (!inserted && !(it->second == key)) {
                throw std::runtime_error("canonical separation failure");
            }
        }
        const Key complemented = space.complement(key, layer);
        if (!(space.complement(complemented, space.c() - layer) == key)) {
            throw std::runtime_error("canonical complement involution failure");
        }
    }
}

Count midpointContraction(const StateSpace& space, const CountMap& left,
                          const CountMap& right, int leftLayer, bool verboseTerms) {
    Count total = 0;
    std::size_t matched = 0;
    for (const auto& [key, leftTotal] : left) {
        const Key complement = space.complement(key, leftLayer);
        auto found = right.find(complement);
        if (found == right.end()) continue;
        const std::size_t orbit = space.orbitSize(key, leftLayer);
        const std::uint64_t labelledAssignments = space.symbolAssignments(key, leftLayer);
        const Count denominator = static_cast<Count>(orbit) * labelledAssignments;
        const Count numerator = leftTotal * found->second;
        if (denominator == 0 || numerator % denominator != 0) {
            throw std::runtime_error("nonintegral orbit-normalized midpoint term");
        }
        const Count term = numerator / denominator;
        total += term;
        ++matched;
        if (verboseTerms) {
            std::cout << "midpoint_term orbit=" << orbit
                      << " labelled=" << labelledAssignments
                      << " left=" << countToString(leftTotal)
                      << " right=" << countToString(found->second)
                      << " term=" << countToString(term) << "\n";
        }
    }
    std::cout << "midpoint matched_orbits=" << matched
              << " value=" << countToString(total) << "\n";
    return total;
}

RunResult run(const Options& options) {
    const StateSpace space(options.c, options.copySwap);
    const int stopLayer = options.stopLayer < 0 ? options.c : options.stopLayer;
    if (stopLayer < 0 || stopLayer > options.c) throw std::runtime_error("invalid stop layer");

    Hist startHist{};
    startHist[StateSpace::index(0, 0)] = static_cast<std::uint8_t>(2 * options.c);
    const Key start = space.canonical(startHist, 0);
    CountMap current;
    current.emplace(start, 1);

    const int leftLayer = options.c / 2;
    const int rightDepth = options.c - leftLayer;
    CountMap savedLeft;
    CountMap savedRight;
    if (leftLayer == 0) savedLeft = current;
    if (rightDepth == 0) savedRight = current;

    RunResult result;
    std::cout << "C=" << options.c
              << " group_images=" << space.groupImageCount()
              << " direct_check=" << (options.directCheck ? 1 : 0)
              << " canonical_check_limit=" << options.canonicalCheckLimit
              << " source_probe=" << options.sourceProbe
              << " source_start=" << options.sourceStart
              << " leaf_probe=" << options.leafProbe
              << " raw_batch=" << options.rawBatchSize
              << " count_leaves_only=" << (options.countLeavesOnly ? 1 : 0)
              << " stop_layer=" << stopLayer
              << " max_states=" << options.maxStates << "\n";
    std::cout << "layer=0 states=1 total=1\n";

    for (int layer = 0; layer < stopLayer; ++layer) {
        const auto started = std::chrono::steady_clock::now();
        CountMap next;
        next.reserve(std::min<std::size_t>(options.maxStates, current.size() * 8 + 128));
        std::unordered_map<Key, Key, KeyHash> canonicalCache;
        if (options.rawBatchSize == 0) {
            canonicalCache.reserve(current.size() * 64 + 128);
        }

        LayerStats stats;
        stats.layer = layer + 1;
        stats.sourceStates = current.size();

        std::vector<std::pair<Key, Count>> orderedSources(current.begin(), current.end());
        std::sort(orderedSources.begin(), orderedSources.end(),
                  [](const auto& left, const auto& right) {
                      return keyLess(left.first, right.first);
                  });
        const std::size_t sourceStart = layer + 1 == stopLayer
                                            ? options.sourceStart
                                            : 0;
        if (sourceStart >= orderedSources.size()) {
            throw std::runtime_error("sourcestart is outside the source layer");
        }
        const std::size_t availableSources = orderedSources.size() - sourceStart;
        const std::size_t processCount = options.sourceProbe == 0
                                             ? availableSources
                                             : std::min(options.sourceProbe,
                                                        availableSources);
        stats.countOnly = options.countLeavesOnly && layer + 1 == stopLayer;
        stats.closed = sourceStart == 0 && processCount == orderedSources.size() &&
                       !stats.countOnly;
        stats.rawDistinctExact = options.rawBatchSize == 0;
        const StateSpace::CanonicalCounters canonicalBefore = space.canonicalCounters();

        for (std::size_t localSourceIndex = 0;
             localSourceIndex < processCount;
             ++localSourceIndex) {
            const std::size_t sourceIndex = sourceStart + localSourceIndex;
            const auto& [sourceKey, sourceTotal] = orderedSources[sourceIndex];
            const Hist source = space.decode(sourceKey, layer);
            if (stats.countOnly || options.sourceProbe != 0 || sourceStart != 0) {
                std::cout << "source_begin target_layer=" << (layer + 1)
                          << " ordered_index=" << sourceIndex
                          << " key=" << keyToHex(sourceKey)
                          << " coefficient=" << countToString(sourceTotal) << "\n";
            }
            CountMap orbitTransitions;
            orbitTransitions.reserve(options.rawBatchSize == 0
                                         ? 1024
                                         : std::min<std::size_t>(options.maxStates, 100'000));

            auto reduceRawTargets = [&](const CountMap& rawTargets) {
                stats.rawTargetOccurrences += rawTargets.size();
                for (const auto& [rawTarget, multiplicity] : rawTargets) {
                    Key canonicalTarget;
                    bool shouldCheck = false;
                    if (options.rawBatchSize == 0) {
                        auto [cacheIt, inserted] =
                            canonicalCache.emplace(rawTarget, Key{});
                        if (inserted) {
                            cacheIt->second = space.canonical(rawTarget, layer + 1);
                            shouldCheck = true;
                        }
                        canonicalTarget = cacheIt->second;
                    } else {
                        canonicalTarget = space.canonical(rawTarget, layer + 1);
                        shouldCheck = true;
                    }
                    if (shouldCheck &&
                        stats.canonicalChecks < options.canonicalCheckLimit) {
                        if (!(space.canonicalFull(rawTarget, layer + 1) ==
                              canonicalTarget)) {
                            throw std::runtime_error(
                                "anchored/full raw-target canonical mismatch at C=" +
                                std::to_string(options.c) + " layer=" +
                                std::to_string(layer + 1));
                        }
                        ++stats.canonicalChecks;
                    }
                    orbitTransitions[canonicalTarget] += multiplicity;
                }
                if (orbitTransitions.size() > options.maxStates) {
                    throw ScaleLimit("per-source orbit-transition limit exceeded at layer " +
                                     std::to_string(layer + 1));
                }
            };

            ContingencyTransition::BatchSink batchSink;
            if (options.rawBatchSize != 0) batchSink = reduceRawTargets;
            const std::uint64_t layerLeafProbe =
                layer + 1 == stopLayer ? options.leafProbe : 0;
            ContingencyTransition transition(space, source, layer,
                                             options.maxLeavesPerSource,
                                             layerLeafProbe,
                                             options.rawBatchSize,
                                             std::move(batchSink),
                                             stats.countOnly);
            transition.run();
            stats.contingencyLeaves += transition.leaves();
            stats.countDpStates += transition.countMemoStates();
            stats.maxCountDpStatesPerSource = std::max(
                stats.maxCountDpStatesPerSource, transition.countMemoStates());
            stats.rawBatches += transition.rawBatches();
            stats.rawBatchEntries += transition.rawBatchEntries();
            if (options.rawBatchSize == 0 && !stats.countOnly) {
                reduceRawTargets(transition.rawTargets());
            }

            if (options.directCheck && !stats.countOnly) {
                if (transition.leafProbeStopped()) {
                    throw std::runtime_error("leafprobe requires nodirect");
                }
                DirectTransition direct(space, source, layer);
                direct.run();
                compareRawTransitions(transition.rawTargets(), direct.rawTargets(),
                                      options.c, layer);
                stats.directLeaves += direct.leaves();
            }

            stats.orbitTransitions += orbitTransitions.size();
            for (const auto& [target, multiplicity] : orbitTransitions) {
                next[target] += sourceTotal * multiplicity;
            }
            ++stats.processedSourceStates;
            if (stats.countOnly || options.sourceProbe != 0 || sourceStart != 0) {
                std::cout << "source_end target_layer=" << (layer + 1)
                          << " ordered_index=" << sourceIndex
                          << " leaves=" << transition.leaves()
                          << " count_dp_states=" << transition.countMemoStates()
                          << " orbit_support=" << orbitTransitions.size()
                          << " leaf_probe_stopped="
                          << (transition.leafProbeStopped() ? 1 : 0) << "\n";
            }
            if (next.size() > options.maxStates) {
                throw ScaleLimit("state limit exceeded at layer " +
                                 std::to_string(layer + 1));
            }
            if (transition.leafProbeStopped()) {
                stats.closed = false;
                stats.leafProbeStopped = true;
                break;
            }
        }

        if (stats.countOnly) stats.closed = false;

        stats.globallyDistinctRawTargets = canonicalCache.size();
        stats.targetStates = next.size();
        for (const auto& [key, value] : next) {
            (void)key;
            stats.layerTotal += value;
        }
        stats.seconds = std::chrono::duration<double>(
            std::chrono::steady_clock::now() - started).count();
        const StateSpace::CanonicalCounters canonicalAfter = space.canonicalCounters();
        stats.canonicalCalls = canonicalAfter.calls - canonicalBefore.calls;
        stats.canonicalCandidates =
            canonicalAfter.candidates - canonicalBefore.candidates;
        stats.fullCanonicalCalls =
            canonicalAfter.fullCalls - canonicalBefore.fullCalls;
        stats.fullCanonicalCandidates =
            canonicalAfter.fullCandidates - canonicalBefore.fullCandidates;
        result.stats.push_back(stats);

        current.swap(next);
        if (!stats.closed) {
            result.leafProbeStopped = stats.leafProbeStopped;
            result.countOnlyStopped = stats.countOnly;
            result.sourceProbeStopped =
                !stats.leafProbeStopped && !stats.countOnly;
            std::cout << "layer=" << (layer + 1)
                      << " closed=0"
                      << " leaf_probe_stopped=" << (stats.leafProbeStopped ? 1 : 0)
                      << " count_only=" << (stats.countOnly ? 1 : 0)
                      << " source_start=" << sourceStart
                      << " processed_sources=" << stats.processedSourceStates
                      << " source_states=" << stats.sourceStates
                      << " partial_target_states=" << stats.targetStates
                      << " contingency_leaves=" << stats.contingencyLeaves
                      << " raw_target_occurrences=" << stats.rawTargetOccurrences
                      << " raw_batches=" << stats.rawBatches
                      << " raw_batch_entries=" << stats.rawBatchEntries
                      << " raw_distinct_exact=" << (stats.rawDistinctExact ? 1 : 0)
                      << " global_raw_targets=" << stats.globallyDistinctRawTargets
                      << " orbit_transitions=" << stats.orbitTransitions
                      << " canonical_checks=" << stats.canonicalChecks
                      << " canonical_calls=" << stats.canonicalCalls
                      << " canonical_candidates=" << stats.canonicalCandidates
                      << " full_canonical_calls=" << stats.fullCanonicalCalls
                      << " full_canonical_candidates="
                      << stats.fullCanonicalCandidates
                      << " count_dp_states=" << stats.countDpStates
                      << " max_count_dp_states_per_source="
                      << stats.maxCountDpStatesPerSource
                      << " partial_total=" << countToString(stats.layerTotal)
                      << " seconds=" << stats.seconds << "\n";
            return result;
        }
        validateOrbitPartition(space, current, layer + 1, options.c <= 3);
        if (layer + 1 == leftLayer) savedLeft = current;
        if (layer + 1 == rightDepth) savedRight = current;

        std::cout << "layer=" << (layer + 1)
                  << " closed=1"
                  << " source_states=" << stats.sourceStates
                  << " states=" << stats.targetStates
                  << " contingency_leaves=" << stats.contingencyLeaves
                  << " direct_leaves=" << stats.directLeaves
                  << " raw_target_occurrences=" << stats.rawTargetOccurrences
                  << " raw_batches=" << stats.rawBatches
                  << " raw_batch_entries=" << stats.rawBatchEntries
                  << " raw_distinct_exact=" << (stats.rawDistinctExact ? 1 : 0)
                  << " global_raw_targets=" << stats.globallyDistinctRawTargets
                  << " orbit_transitions=" << stats.orbitTransitions
                  << " canonical_checks=" << stats.canonicalChecks
                  << " canonical_calls=" << stats.canonicalCalls
                  << " canonical_candidates=" << stats.canonicalCandidates
                  << " full_canonical_calls=" << stats.fullCanonicalCalls
                  << " full_canonical_candidates="
                  << stats.fullCanonicalCandidates
                  << " count_dp_states=" << stats.countDpStates
                  << " max_count_dp_states_per_source="
                  << stats.maxCountDpStatesPerSource
                  << " total=" << countToString(stats.layerTotal)
                  << " seconds=" << stats.seconds << "\n";
    }

    if (stopLayer == options.c) {
        if (current.size() != 1) throw std::runtime_error("terminal orbit is not unique");
        result.fullTotal = current.begin()->second;
        result.completed = true;
        if (savedLeft.empty() || savedRight.empty()) {
            throw std::runtime_error("midpoint layers were not retained");
        }
        result.midpointTotal = midpointContraction(
            space, savedLeft, savedRight, leftLayer, options.c == 2);
        if (result.midpointTotal != result.fullTotal) {
            throw std::runtime_error("midpoint and sequential totals differ");
        }
        std::cout << "N(" << options.c << ")=" << countToString(result.fullTotal) << "\n";
    }
    return result;
}

Options parseOptions(int argc, char** argv) {
    if (argc < 2) throw std::runtime_error(
        "usage: joint_histogram C [stop=N] [maxstates=N] [maxleaves=N] "
        "[sourcestart=N] [sourceprobe=N] [leafprobe=N] [rawbatch=N] "
        "[canoncheck=N] [countleaves] [nodirect] [noswap]");
    Options options;
    options.c = std::atoi(argv[1]);
    options.directCheck = options.c <= 4;
    options.canonicalCheckLimit = options.c <= 4
                                      ? std::numeric_limits<std::size_t>::max()
                                      : 0;
    for (int i = 2; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg.rfind("stop=", 0) == 0) {
            options.stopLayer = std::atoi(arg.c_str() + 5);
        } else if (arg.rfind("maxstates=", 0) == 0) {
            options.maxStates = static_cast<std::size_t>(
                std::strtoull(arg.c_str() + 10, nullptr, 10));
        } else if (arg.rfind("maxleaves=", 0) == 0) {
            options.maxLeavesPerSource = std::strtoull(arg.c_str() + 10, nullptr, 10);
        } else if (arg.rfind("sourcestart=", 0) == 0) {
            options.sourceStart = static_cast<std::size_t>(
                std::strtoull(arg.c_str() + 12, nullptr, 10));
        } else if (arg.rfind("sourceprobe=", 0) == 0) {
            options.sourceProbe = static_cast<std::size_t>(
                std::strtoull(arg.c_str() + 12, nullptr, 10));
            if (options.sourceProbe == 0) {
                throw std::runtime_error("sourceprobe must be positive");
            }
        } else if (arg.rfind("leafprobe=", 0) == 0) {
            options.leafProbe = std::strtoull(arg.c_str() + 10, nullptr, 10);
            if (options.leafProbe == 0) {
                throw std::runtime_error("leafprobe must be positive");
            }
        } else if (arg.rfind("rawbatch=", 0) == 0) {
            options.rawBatchSize = static_cast<std::size_t>(
                std::strtoull(arg.c_str() + 9, nullptr, 10));
            if (options.rawBatchSize == 0) {
                throw std::runtime_error("rawbatch must be positive");
            }
        } else if (arg.rfind("canoncheck=", 0) == 0) {
            options.canonicalCheckLimit = static_cast<std::size_t>(
                std::strtoull(arg.c_str() + 11, nullptr, 10));
        } else if (arg == "nodirect") {
            options.directCheck = false;
        } else if (arg == "noswap") {
            options.copySwap = false;
        } else if (arg == "countleaves") {
            options.countLeavesOnly = true;
        } else {
            throw std::runtime_error("unknown argument: " + arg);
        }
    }
    if (options.maxStates == 0) throw std::runtime_error("maxstates must be positive");
    if (options.rawBatchSize != 0 && options.directCheck) {
        throw std::runtime_error("rawbatch requires nodirect");
    }
    if (options.leafProbe != 0 && options.directCheck) {
        throw std::runtime_error("leafprobe requires nodirect");
    }
    if (options.countLeavesOnly && options.leafProbe != 0) {
        throw std::runtime_error("countleaves and leafprobe are mutually exclusive");
    }
    if (options.countLeavesOnly && options.rawBatchSize != 0) {
        throw std::runtime_error("countleaves does not use rawbatch");
    }
    return options;
}

Count knownValue(int c) {
    if (c == 2) return parseCount("288");
    if (c == 3) return parseCount("28200960");
    if (c == 4) return parseCount("29136487207403520");
    if (c == 5) return parseCount("1903816047972624930994913280000");
    throw std::runtime_error("no known value");
}

}  // namespace

int main(int argc, char** argv) {
    try {
        const Options options = parseOptions(argc, argv);
        const RunResult result = run(options);
        if (result.completed) {
            if (result.fullTotal != knownValue(options.c)) {
                std::cerr << "[FAIL] expected=" << countToString(knownValue(options.c))
                          << " got=" << countToString(result.fullTotal) << "\n";
                return 1;
            }
            std::cout << "[OK] exact sequential, midpoint, and known-value gates passed\n";
        } else if (result.countOnlyStopped) {
            std::cout << "[PROBE] exact contingency-leaf count completed without "
                         "materializing targets\n";
        } else if (result.leafProbeStopped) {
            std::cout << "[PROBE] stopped after deterministic leaf prefix; "
                         "reported target data are partial\n";
        } else if (result.sourceProbeStopped) {
            std::cout << "[PROBE] stopped after deterministic source prefix; "
                         "reported target data are partial\n";
        } else {
            std::cout << "[BOUNDED] stopped after requested layer\n";
        }
        return 0;
    } catch (const ScaleLimit& e) {
        std::cerr << "[SCALE-LIMIT] " << e.what() << "\n";
        return 3;
    } catch (const std::exception& e) {
        std::cerr << "[ERROR] " << e.what() << "\n";
        return 2;
    }
}
