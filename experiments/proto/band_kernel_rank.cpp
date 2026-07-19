// Bounded exact-rank gate for the symmetry-reduced band kernel.
//
// This decision prototype deliberately reuses the audited state,
// canonicalization, and side-histogram implementation from multiset_q.cpp.
// Its independent checks are: every C<=4 side histogram is compared with the
// brute restricted-bijection builder, every endpoint total is checked, and
// maximal ranks are certified modulo two primes.
#define main multiset_q_embedded_main
#include "../../src/multiset_q.cpp"
#undef main

#include <filesystem>
#include <set>
#include <stdexcept>

namespace {

using Row = std::map<Key, uint64_t>;

std::unordered_map<Key, Key, KeyHash> rawCanon;
uint64_t histogramChecks = 0;

std::string u128String(unsigned __int128 value) {
    if (value == 0) return "0";
    std::string result;
    while (value != 0) {
        result.push_back((char)('0' + value % 10));
        value /= 10;
    }
    std::reverse(result.begin(), result.end());
    return result;
}

std::string i128String(__int128 value) {
    if (value < 0) return "-" + u128String((unsigned __int128)(-value));
    return u128String((unsigned __int128)value);
}

Key canonicalTarget(const PartKey& top, const PartKey& bottom) {
    Key raw{};
    int i = 0, j = 0, k = 0;
    while (i < C && j < C) {
        raw[k++] = top[i] <= bottom[j] ? top[i++] : bottom[j++];
    }
    while (i < C) raw[k++] = top[i++];
    while (j < C) raw[k++] = bottom[j++];
    auto found = rawCanon.find(raw);
    if (found != rawCanon.end()) return found->second;
    const Key key = canonMS2(raw.data());
    rawCanon.emplace(raw, key);
    return key;
}

void checkedSideHistogram(const int* symbols,
                          const std::vector<int>& xm,
                          const std::vector<int>& ym,
                          std::map<PartKey, uint64_t>& fast) {
    buildHistFast(symbols, xm, ym, fast);
    std::map<PartKey, uint64_t> brute;
    buildHistBrute(symbols, xm, ym, brute);
    ++histogramChecks;
    if (fast != brute) {
        throw std::runtime_error("band-kernel side-histogram differential failed");
    }
}

Row transitionRow(const Key& source) {
    std::vector<int> xm(M), ym(M);
    for (int symbol = 0; symbol < M; ++symbol) {
        xm[symbol] = pX(source[symbol]);
        ym[symbol] = pY(source[symbol]);
    }

    Row row;
    for (int skeleton : Askel) {
        int top[8]{}, bottom[8]{};
        int nt = 0, nb = 0;
        for (int symbol = 0; symbol < M; ++symbol) {
            if (skeleton & (1 << symbol)) top[nt++] = symbol;
            else bottom[nb++] = symbol;
        }
        if (nt != C || nb != C) {
            throw std::runtime_error("invalid skeleton partition");
        }

        std::map<PartKey, uint64_t> topHist, bottomHist;
        checkedSideHistogram(top, xm, ym, topHist);
        checkedSideHistogram(bottom, xm, ym, bottomHist);
        for (const auto& [topKey, topWeight] : topHist) {
            for (const auto& [bottomKey, bottomWeight] : bottomHist) {
                row[canonicalTarget(topKey, bottomKey)] +=
                    topWeight * bottomWeight;
            }
        }
    }
    return row;
}

uint64_t modPow(uint64_t base, uint64_t exponent, uint64_t prime) {
    uint64_t result = 1;
    while (exponent != 0) {
        if (exponent & 1) {
            result = (uint64_t)((unsigned __int128)result * base % prime);
        }
        base = (uint64_t)((unsigned __int128)base * base % prime);
        exponent >>= 1;
    }
    return result;
}

size_t rankModulo(const std::vector<std::vector<uint64_t>>& matrix,
                  uint64_t prime) {
    if (matrix.empty() || matrix[0].empty()) return 0;
    const size_t rowCount = matrix.size();
    const size_t columnCount = matrix[0].size();
    std::vector<std::vector<uint64_t>> a = matrix;
    for (auto& row : a) {
        for (uint64_t& value : row) value %= prime;
    }

    size_t rank = 0;
    for (size_t column = 0; column < columnCount && rank < rowCount;
         ++column) {
        size_t pivot = rank;
        while (pivot < rowCount && a[pivot][column] == 0) ++pivot;
        if (pivot == rowCount) continue;
        std::swap(a[rank], a[pivot]);
        const uint64_t inverse = modPow(a[rank][column], prime - 2, prime);
        for (size_t j = column; j < columnCount; ++j) {
            a[rank][j] = (uint64_t)(
                (unsigned __int128)a[rank][j] * inverse % prime);
        }
        for (size_t i = rank + 1; i < rowCount; ++i) {
            const uint64_t factor = a[i][column];
            if (factor == 0) continue;
            for (size_t j = column; j < columnCount; ++j) {
                const uint64_t product = (uint64_t)(
                    (unsigned __int128)factor * a[rank][j] % prime);
                a[i][j] = a[i][j] >= product
                    ? a[i][j] - product
                    : a[i][j] + prime - product;
            }
        }
        ++rank;
    }
    return rank;
}

size_t rankModuloFlat(std::vector<uint32_t> a,
                      size_t dimension,
                      uint32_t prime) {
    size_t rank = 0;
    for (size_t column = 0; column < dimension && rank < dimension;
         ++column) {
        size_t pivot = rank;
        while (pivot < dimension && a[pivot * dimension + column] == 0) {
            ++pivot;
        }
        if (pivot == dimension) continue;
        if (pivot != rank) {
            for (size_t j = column; j < dimension; ++j) {
                std::swap(a[rank * dimension + j],
                          a[pivot * dimension + j]);
            }
        }
        const uint64_t inverse = modPow(
            a[rank * dimension + column], prime - 2, prime);
        for (size_t j = column; j < dimension; ++j) {
            a[rank * dimension + j] = (uint32_t)(
                (unsigned __int128)a[rank * dimension + j] * inverse %
                prime);
        }
        for (size_t i = rank + 1; i < dimension; ++i) {
            const uint32_t factor = a[i * dimension + column];
            if (factor == 0) continue;
            for (size_t j = column; j < dimension; ++j) {
                const uint32_t product = (uint32_t)(
                    (uint64_t)factor * a[rank * dimension + j] % prime);
                uint32_t& value = a[i * dimension + j];
                value = value >= product
                    ? value - product
                    : value + prime - product;
            }
        }
        ++rank;
    }
    return rank;
}

__int128 bareissDeterminant(const std::vector<std::vector<uint64_t>>& matrix) {
    const size_t n = matrix.size();
    if (n == 0 || matrix[0].size() != n) {
        throw std::runtime_error("determinant requires a square matrix");
    }
    std::vector<std::vector<__int128>> a(
        n, std::vector<__int128>(n));
    for (size_t i = 0; i < n; ++i) {
        for (size_t j = 0; j < n; ++j) a[i][j] = matrix[i][j];
    }
    __int128 previous = 1;
    int sign = 1;
    for (size_t k = 0; k + 1 < n; ++k) {
        size_t pivot = k;
        while (pivot < n && a[pivot][k] == 0) ++pivot;
        if (pivot == n) return 0;
        if (pivot != k) {
            std::swap(a[pivot], a[k]);
            sign = -sign;
        }
        const __int128 pivotValue = a[k][k];
        for (size_t i = k + 1; i < n; ++i) {
            for (size_t j = k + 1; j < n; ++j) {
                a[i][j] =
                    (a[i][j] * pivotValue - a[i][k] * a[k][j]) /
                    previous;
            }
        }
        for (size_t i = k + 1; i < n; ++i) a[i][k] = 0;
        previous = pivotValue;
    }
    return sign * a[n - 1][n - 1];
}

struct LayerResult {
    std::vector<Key> targets;
    std::vector<std::vector<uint64_t>> matrix;
    size_t nonzeros = 0;
};

LayerResult buildLayer(const std::vector<Key>& sources, int grade) {
    std::vector<Row> rows;
    rows.reserve(sources.size());
    std::set<Key> targetSet;
    const auto start = std::chrono::steady_clock::now();
    for (size_t i = 0; i < sources.size(); ++i) {
        Row row = transitionRow(sources[i]);
        for (const auto& entry : row) targetSet.insert(entry.first);
        rows.push_back(std::move(row));
    }

    LayerResult result;
    result.targets.assign(targetSet.begin(), targetSet.end());
    std::map<Key, size_t> targetIndex;
    for (size_t i = 0; i < result.targets.size(); ++i) {
        targetIndex.emplace(result.targets[i], i);
    }
    result.matrix.assign(
        sources.size(), std::vector<uint64_t>(result.targets.size(), 0));
    for (size_t i = 0; i < rows.size(); ++i) {
        result.nonzeros += rows[i].size();
        for (const auto& [key, value] : rows[i]) {
            result.matrix[i][targetIndex.at(key)] = value;
        }
    }
    const double seconds = std::chrono::duration<double>(
        std::chrono::steady_clock::now() - start).count();
    std::fprintf(stderr,
        "rank layer C=%d grade=%d sources=%zu targets=%zu nnz=%zu time=%.6fs\n",
        C, grade, sources.size(), result.targets.size(), result.nonzeros,
        seconds);
    return result;
}

int runExactRankGate(int requestedC) {
    if (requestedC < 2 || requestedC > 4) {
        throw std::runtime_error("exact rank gate is bounded to C=2..4");
    }
    C = requestedC;
    M = 2 * C;
    FULLC = (1 << C) - 1;
    initPerms();
    for (int mask = 0; mask < (1 << M); ++mask) {
        if (__builtin_popcount((unsigned)mask) == C) Askel.push_back(mask);
    }

    Key empty{};
    std::vector<Key> sources{canonMS2(empty.data())};
    std::vector<unsigned __int128> weights{1};
    const uint64_t primes[2] = {1000000007ULL, 1000000009ULL};
    std::vector<size_t> dimensions{1};

    for (int grade = 0; grade < C; ++grade) {
        LayerResult layer = buildLayer(sources, grade);
        const size_t rank1 = rankModulo(layer.matrix, primes[0]);
        const size_t rank2 = rankModulo(layer.matrix, primes[1]);
        const size_t maximum = std::min(
            layer.matrix.size(),
            layer.matrix.empty() ? size_t(0) : layer.matrix[0].size());
        if (rank1 != maximum || rank2 != maximum) {
            throw std::runtime_error(
                "small-C band kernel is not maximal rank modulo both primes");
        }

        std::string determinant = "NA";
        if (layer.matrix.size() == layer.targets.size()) {
            determinant = i128String(bareissDeterminant(layer.matrix));
            if (determinant == "0") {
                throw std::runtime_error("square band-kernel determinant is zero");
            }
            std::vector<uint32_t> flat;
            flat.reserve(layer.matrix.size() * layer.targets.size());
            for (const auto& row : layer.matrix) {
                for (uint64_t value : row) flat.push_back((uint32_t)value);
            }
            if (rankModuloFlat(flat, layer.matrix.size(),
                               (uint32_t)primes[0]) != rank1 ||
                rankModuloFlat(flat, layer.matrix.size(),
                               (uint32_t)primes[1]) != rank2) {
                throw std::runtime_error(
                    "flat and rectangular modular-rank implementations disagree");
            }
        }
        std::printf(
            "C=%d grade=%d matrix=%zux%zu nnz=%zu rankQ=%zu rankP1=%zu rankP2=%zu determinant=%s\n",
            C, grade, layer.matrix.size(), layer.targets.size(),
            layer.nonzeros, maximum, rank1, rank2, determinant.c_str());

        std::vector<unsigned __int128> nextWeights(layer.targets.size(), 0);
        for (size_t i = 0; i < weights.size(); ++i) {
            for (size_t j = 0; j < layer.targets.size(); ++j) {
                nextWeights[j] += weights[i] * layer.matrix[i][j];
            }
        }
        weights = std::move(nextWeights);
        sources = std::move(layer.targets);
        dimensions.push_back(sources.size());
    }

    if (weights.size() != 1) {
        throw std::runtime_error("rank gate did not end at the unique full state");
    }
    const char* expected = C == 2 ? "288" :
                           C == 3 ? "28200960" :
                                    "29136487207403520";
    const std::string total = u128String(weights[0]);
    if (total != expected) {
        throw std::runtime_error("rank-gate endpoint total mismatch");
    }
    std::printf("C=%d dimensions=", C);
    for (size_t i = 0; i < dimensions.size(); ++i) {
        std::printf("%s%zu", i == 0 ? "" : ",", dimensions[i]);
    }
    std::printf(" N=%s histDifferentials=%llu rawCanon=%zu [OK]\n",
                total.c_str(),
                (unsigned long long)histogramChecks,
                rawCanon.size());
    return 0;
}

uint64_t mix64(uint64_t value) {
    value += 0x9e3779b97f4a7c15ULL;
    value = (value ^ (value >> 30)) * 0xbf58476d1ce4e5b9ULL;
    value = (value ^ (value >> 27)) * 0x94d049bb133111ebULL;
    return value ^ (value >> 31);
}

uint64_t seededKeyHash(const Key& key, uint64_t seed) {
    uint64_t hash = seed;
    for (uint16_t value : key) {
        hash = mix64(hash ^ value);
    }
    return hash;
}

uint64_t basisFingerprint(const std::vector<Key>& basis) {
    uint64_t hash = 1469598103934665603ULL;
    for (const Key& key : basis) {
        for (uint16_t value : key) {
            hash ^= value;
            hash *= 1099511628211ULL;
        }
    }
    return hash;
}

uint64_t sketchFingerprint(const std::vector<uint32_t>& first,
                           const std::vector<uint32_t>& second) {
    uint64_t hash = 1469598103934665603ULL;
    auto add = [&](const std::vector<uint32_t>& values) {
        for (uint32_t value : values) {
            for (int shift = 0; shift < 32; shift += 8) {
                hash ^= (uint8_t)(value >> shift);
                hash *= 1099511628211ULL;
            }
        }
    };
    add(first);
    add(second);
    return hash;
}

struct TargetSketchInfo {
    uint32_t bucket = 0;
    bool negative = false;
};

struct SketchStats {
    uint64_t sources = 0;
    uint64_t essential = 0;
    uint64_t crossPairs = 0;
    uint64_t targetEntries = 0;
    uint64_t badTargets = 0;
    uint64_t sideHits = 0;
    uint64_t sideMisses = 0;

    void add(const SketchStats& other) {
        sources += other.sources;
        essential += other.essential;
        crossPairs += other.crossPairs;
        targetEntries += other.targetEntries;
        badTargets += other.badTargets;
        sideHits += other.sideHits;
        sideMisses += other.sideMisses;
    }
};

void addSignedModulo(uint32_t& destination,
                     uint64_t value,
                     bool negative,
                     uint32_t prime) {
    uint32_t add = (uint32_t)(value % prime);
    if (negative && add != 0) add = prime - add;
    const uint64_t sum = (uint64_t)destination + add;
    destination = (uint32_t)(sum >= prime ? sum - prime : sum);
}

void writeU32(std::ofstream& out, uint32_t value) {
    out.write(reinterpret_cast<const char*>(&value), sizeof(value));
}

void writeU64(std::ofstream& out, uint64_t value) {
    out.write(reinterpret_cast<const char*>(&value), sizeof(value));
}

uint32_t readU32(std::ifstream& in) {
    uint32_t value = 0;
    in.read(reinterpret_cast<char*>(&value), sizeof(value));
    return value;
}

uint64_t readU64(std::ifstream& in) {
    uint64_t value = 0;
    in.read(reinterpret_cast<char*>(&value), sizeof(value));
    return value;
}

void saveSketch(const std::string& path,
                uint32_t dimension,
                uint64_t basisHash,
                uint64_t seed,
                const SketchStats& stats,
                const std::vector<uint32_t>& first,
                const std::vector<uint32_t>& second,
                uint64_t dataHash) {
    namespace fs = std::filesystem;
    const fs::path output(path);
    const fs::path temporary(path + ".tmp");
    if (fs::exists(output) || fs::exists(temporary)) {
        throw std::runtime_error("rank-sketch output path already exists");
    }
    std::ofstream out(temporary, std::ios::binary | std::ios::trunc);
    if (!out) throw std::runtime_error("cannot create rank-sketch output");
    const char magic[8] = {'F','J','B','K','R','S','1','\0'};
    out.write(magic, sizeof(magic));
    writeU32(out, 1);
    writeU32(out, 5);
    writeU32(out, dimension);
    writeU32(out, 38801);
    writeU64(out, basisHash);
    writeU64(out, seed);
    writeU64(out, 1000000007ULL);
    writeU64(out, 1000000009ULL);
    writeU64(out, stats.sources);
    writeU64(out, stats.essential);
    writeU64(out, stats.crossPairs);
    writeU64(out, stats.targetEntries);
    writeU64(out, stats.badTargets);
    writeU64(out, dataHash);
    out.write(reinterpret_cast<const char*>(first.data()),
              (std::streamsize)(first.size() * sizeof(uint32_t)));
    out.write(reinterpret_cast<const char*>(second.data()),
              (std::streamsize)(second.size() * sizeof(uint32_t)));
    out.close();
    if (!out) throw std::runtime_error("rank-sketch output write failed");
    fs::rename(temporary, output);
    const uintmax_t expected = 104 +
        (uintmax_t)(first.size() + second.size()) * sizeof(uint32_t);
    if (fs::file_size(output) != expected) {
        throw std::runtime_error("rank-sketch output size verification failed");
    }
}

int verifySketch(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) throw std::runtime_error("cannot open rank-sketch certificate");
    char magic[8]{};
    in.read(magic, sizeof(magic));
    const char expectedMagic[8] = {'F','J','B','K','R','S','1','\0'};
    if (std::memcmp(magic, expectedMagic, sizeof(magic)) != 0) {
        throw std::runtime_error("rank-sketch certificate magic mismatch");
    }
    const uint32_t version = readU32(in);
    const uint32_t storedC = readU32(in);
    const uint32_t dimension = readU32(in);
    const uint32_t basisCount = readU32(in);
    const uint64_t basisHash = readU64(in);
    const uint64_t seed = readU64(in);
    const uint64_t prime1 = readU64(in);
    const uint64_t prime2 = readU64(in);
    const uint64_t sources = readU64(in);
    const uint64_t essential = readU64(in);
    const uint64_t crossPairs = readU64(in);
    const uint64_t targetEntries = readU64(in);
    const uint64_t badTargets = readU64(in);
    const uint64_t storedHash = readU64(in);
    if (!in || version != 1 || storedC != 5 || basisCount != 38801 ||
        dimension < 16 || dimension > 2048 || sources != dimension ||
        prime1 != 1000000007ULL || prime2 != 1000000009ULL ||
        badTargets != 0) {
        throw std::runtime_error("rank-sketch certificate header is invalid");
    }
    const size_t cells = (size_t)dimension * dimension;
    std::vector<uint32_t> first(cells), second(cells);
    in.read(reinterpret_cast<char*>(first.data()),
            (std::streamsize)(cells * sizeof(uint32_t)));
    in.read(reinterpret_cast<char*>(second.data()),
            (std::streamsize)(cells * sizeof(uint32_t)));
    char trailing = 0;
    if (!in || in.read(&trailing, 1)) {
        throw std::runtime_error("rank-sketch certificate length is invalid");
    }
    const uint64_t computedHash = sketchFingerprint(first, second);
    if (computedHash != storedHash) {
        throw std::runtime_error("rank-sketch certificate data hash mismatch");
    }
    const size_t rank1 = rankModuloFlat(first, dimension, (uint32_t)prime1);
    const size_t rank2 = rankModuloFlat(second, dimension, (uint32_t)prime2);
    std::printf(
        "rankSketchVerify C=5 basis=%u dimension=%u rankP1=%zu rankP2=%zu lowerBound=%zu exactCertificate=%s basisHash=%016llX dataHash=%016llX seed=%llu essential=%llu cross=%llu targetEntries=%llu [OK]\n",
        basisCount, dimension, rank1, rank2, std::max(rank1, rank2),
        (rank1 == dimension || rank2 == dimension) ? "yes" : "no",
        (unsigned long long)basisHash,
        (unsigned long long)computedHash,
        (unsigned long long)seed,
        (unsigned long long)essential,
        (unsigned long long)crossPairs,
        (unsigned long long)targetEntries);
    return 0;
}

int runC5RankSketch(const std::string& basisPath,
                    size_t samples,
                    int cacheLog2,
                    uint64_t seed,
                    const std::string& outputPath) {
    if (samples < 16 || samples > 2048) {
        throw std::runtime_error("C=5 rank sketch requires 16..2048 samples");
    }
    if (cacheLog2 < 20 || cacheLog2 > 28) {
        throw std::runtime_error("C=5 rank sketch requires cachelog=20..28");
    }
    if (basisPath.empty() || outputPath.empty()) {
        throw std::runtime_error("C=5 rank sketch requires basis= and out=");
    }

    C = 5;
    M = 10;
    FULLC = (1 << C) - 1;
    initPerms();
    for (int mask = 0; mask < (1 << M); ++mask) {
        if (__builtin_popcount((unsigned)mask) == C) Askel.push_back(mask);
    }

    int loadedBand = -1;
    std::map<Key, Big> loaded;
    if (!loadCheckpoint(basisPath, loadedBand, loaded) || loadedBand != 1 ||
        loaded.size() != 38801) {
        throw std::runtime_error(
            "rank sketch requires an exact C=5 band-1 checkpoint with 38801 states");
    }
    std::vector<Key> basis;
    basis.reserve(loaded.size());
    for (const auto& [key, weight] : loaded) {
        (void)weight;
        for (int symbol = 0; symbol < M; ++symbol) {
            if (__builtin_popcount((unsigned)pX(key[symbol])) != 2 ||
                __builtin_popcount((unsigned)pY(key[symbol])) != 2) {
                throw std::runtime_error("rank-sketch basis has the wrong grade");
            }
        }
        basis.push_back(key);
    }
    const uint64_t basisHash = basisFingerprint(basis);

    std::unordered_map<Key, TargetSketchInfo, KeyHash> targetInfo;
    targetInfo.reserve(basis.size() * 2);
    for (const Key& source : basis) {
        const Key target = complementKey(source);
        const uint64_t hash = seededKeyHash(target, seed);
        const TargetSketchInfo info{
            (uint32_t)(hash % samples),
            (hash >> 63) != 0};
        auto [it, inserted] = targetInfo.emplace(target, info);
        if (!inserted &&
            (it->second.bucket != info.bucket ||
             it->second.negative != info.negative)) {
            throw std::runtime_error("inconsistent target sketch map");
        }
    }
    if (targetInfo.size() != basis.size()) {
        throw std::runtime_error("complemented target basis is not bijective");
    }

    const size_t cells = samples * samples;
    std::vector<uint32_t> sketch1(cells, 0), sketch2(cells, 0);
    const uint32_t prime1 = 1000000007U;
    const uint32_t prime2 = 1000000009U;
    const bool cacheProfile = samples <= 64;
    gProfile = cacheProfile;
    gCache.init(cacheLog2);
    gCache.resetStats();
    const auto start = std::chrono::steady_clock::now();
    const int threadCount = omp_get_max_threads();
    std::vector<SketchStats> threadStats((size_t)threadCount);
    std::atomic<size_t> done{0};

    #pragma omp parallel
    {
        const int tid = omp_get_thread_num();
        SketchStats& stats = threadStats[(size_t)tid];
        std::unordered_map<Key,
            std::vector<std::pair<PartKey, uint64_t>>, KeyHash> sideMemo;
        std::unordered_map<Key, uint64_t, KeyHash> targetWeights;
        targetWeights.reserve(16384);
        const size_t sideCap = 50000;

        auto getSide = [&](const uint16_t* values)
            -> const std::vector<std::pair<PartKey, uint64_t>>& {
            const Key side = makeSideKey(values);
            auto found = sideMemo.find(side);
            if (found != sideMemo.end()) {
                ++stats.sideHits;
                return found->second;
            }
            ++stats.sideMisses;
            auto histogram = buildSideHistFromKey(side);
            return sideMemo.emplace(side, std::move(histogram)).first->second;
        };

        #pragma omp for schedule(dynamic, 1)
        for (long long sampleIndex = 0;
             sampleIndex < (long long)samples; ++sampleIndex) {
            const size_t basisIndex = (size_t)(
                ((unsigned __int128)(2 * sampleIndex + 1) * basis.size()) /
                (2 * samples));
            const Key& source = basis[basisIndex];
            std::unordered_map<Key, uint32_t, KeyHash> essential;
            essential.reserve(Askel.size());
            TaggedCanon tagged;
            tagged.init(source.data());
            for (int skeleton : Askel) ++essential[tagged.canon(skeleton)];
            ++stats.sources;
            stats.essential += essential.size();

            uint32_t* row1 = sketch1.data() + (size_t)sampleIndex * samples;
            uint32_t* row2 = sketch2.data() + (size_t)sampleIndex * samples;
            for (const auto& [taggedKey, coefficient] : essential) {
                if (sideMemo.size() >= sideCap) sideMemo.clear();
                uint16_t top[8]{}, bottom[8]{};
                int nt = 0, nb = 0;
                for (int symbol = 0; symbol < M; ++symbol) {
                    if (taggedKey[symbol] & TAG) {
                        top[nt++] = taggedKey[symbol] & VALMASK;
                    } else {
                        bottom[nb++] = taggedKey[symbol];
                    }
                }
                if (nt != C || nb != C) {
                    ++stats.badTargets;
                    continue;
                }
                isort(top, C);
                isort(bottom, C);
                const auto& topHist = getSide(top);
                const auto& bottomHist = getSide(bottom);
                stats.crossPairs +=
                    (uint64_t)topHist.size() * bottomHist.size();
                targetWeights.clear();
                for (const auto& [topKey, topWeight] : topHist) {
                    for (const auto& [bottomKey, bottomWeight] : bottomHist) {
                        Key raw{};
                        int i = 0, j = 0, k = 0;
                        while (i < C && j < C) {
                            raw[k++] = topKey[i] <= bottomKey[j]
                                ? topKey[i++] : bottomKey[j++];
                        }
                        while (i < C) raw[k++] = topKey[i++];
                        while (j < C) raw[k++] = bottomKey[j++];
                        targetWeights[gCache.get(raw)] +=
                            topWeight * bottomWeight;
                    }
                }
                stats.targetEntries += targetWeights.size();
                for (const auto& [target, targetWeight] : targetWeights) {
                    auto found = targetInfo.find(target);
                    if (found == targetInfo.end()) {
                        ++stats.badTargets;
                        continue;
                    }
                    const uint64_t value =
                        (uint64_t)coefficient * targetWeight;
                    const TargetSketchInfo info = found->second;
                    addSignedModulo(
                        row1[info.bucket], value, info.negative, prime1);
                    addSignedModulo(
                        row2[info.bucket], value, info.negative, prime2);
                }
            }
            const size_t completed = ++done;
            if ((completed & 15) == 0 || completed == samples) {
                const double seconds = std::chrono::duration<double>(
                    std::chrono::steady_clock::now() - start).count();
                std::fprintf(stderr,
                    "rank sketch sources=%zu/%zu elapsed=%.3fs\n",
                    completed, samples, seconds);
                std::fflush(stderr);
            }
        }
    }

    SketchStats stats;
    for (const SketchStats& thread : threadStats) stats.add(thread);
    const CacheStatsSnapshot cacheStats = gCache.stats();
    gCache.release();
    gProfile = false;
    if (stats.badTargets != 0 || stats.sources != samples) {
        throw std::runtime_error("rank sketch target-basis validation failed");
    }

    const auto rankStart = std::chrono::steady_clock::now();
    const size_t rank1 = rankModuloFlat(sketch1, samples, prime1);
    const size_t rank2 = rankModuloFlat(sketch2, samples, prime2);
    const double rankSeconds = std::chrono::duration<double>(
        std::chrono::steady_clock::now() - rankStart).count();
    const double seconds = std::chrono::duration<double>(
        std::chrono::steady_clock::now() - start).count();
    const uint64_t dataHash = sketchFingerprint(sketch1, sketch2);
    saveSketch(outputPath, (uint32_t)samples, basisHash, seed,
               stats, sketch1, sketch2, dataHash);

    std::printf(
        "C=5 rankSketch basis=38801 samples=%zu seed=%llu rankP1=%zu rankP2=%zu lowerBound=%zu exactCertificate=%s\n",
        samples, (unsigned long long)seed, rank1, rank2,
        std::max(rank1, rank2),
        (rank1 == samples || rank2 == samples) ? "yes" : "no");
    std::printf(
        "rankSketchStats essential=%llu cross=%llu targetEntries=%llu badTargets=%llu sideHits=%llu sideMisses=%llu cacheProfile=%s cacheHits=%llu cacheMisses=%llu cacheInserts=%llu elapsed=%.3fs rankTime=%.3fs basisHash=%016llX dataHash=%016llX out=%s\n",
        (unsigned long long)stats.essential,
        (unsigned long long)stats.crossPairs,
        (unsigned long long)stats.targetEntries,
        (unsigned long long)stats.badTargets,
        (unsigned long long)stats.sideHits,
        (unsigned long long)stats.sideMisses,
        cacheProfile ? "on" : "off",
        cacheStats.hits, cacheStats.misses, cacheStats.inserts,
        seconds, rankSeconds,
        (unsigned long long)basisHash,
        (unsigned long long)dataHash,
        outputPath.c_str());
    return 0;
}

} // namespace

int main(int argc, char** argv) {
    try {
        if (argc > 2 && std::string(argv[1]) == "verify") {
            return verifySketch(argv[2]);
        }
        const int requestedC = argc > 1 ? std::atoi(argv[1]) : 3;
        if (requestedC == 5 && argc > 2 &&
            std::string(argv[2]) == "sketch") {
            std::string basisPath, outputPath;
            size_t samples = 0;
            int cacheLog2 = 0;
            uint64_t seed = 0x6a09e667f3bcc909ULL;
            for (int i = 3; i < argc; ++i) {
                const std::string argument = argv[i];
                if (argument.rfind("basis=", 0) == 0) {
                    basisPath = argument.substr(6);
                } else if (argument.rfind("samples=", 0) == 0) {
                    samples = (size_t)std::strtoull(
                        argument.c_str() + 8, nullptr, 10);
                } else if (argument.rfind("cachelog=", 0) == 0) {
                    cacheLog2 = std::atoi(argument.c_str() + 9);
                } else if (argument.rfind("seed=", 0) == 0) {
                    seed = std::strtoull(
                        argument.c_str() + 5, nullptr, 0);
                } else if (argument.rfind("out=", 0) == 0) {
                    outputPath = argument.substr(4);
                } else {
                    throw std::runtime_error(
                        "unknown C=5 rank-sketch argument");
                }
            }
            return runC5RankSketch(
                basisPath, samples, cacheLog2, seed, outputPath);
        }
        return runExactRankGate(requestedC);
    } catch (const std::exception& error) {
        std::fprintf(stderr, "fatal: %s\n", error.what());
        return 2;
    }
}
