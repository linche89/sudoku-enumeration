// Bounded native-state / unpaired-graph decision benchmark. No checkpoints.
// Reuse the retained exact graph kernels instead of copying their algorithms.
#define main factorization_orbit_original_main
#include "../../src/factorization_orbit.cpp"
#undef main

#include <atomic>
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <sstream>
#include <thread>
#include <omp.h>
#ifdef _WIN32
#include <psapi.h>
#endif

namespace direct_bench {
using Clock = std::chrono::steady_clock;
using Graph = std::array<uint16_t, MAX_M>;

struct Options {
    int degree = 0;
    uint64_t limit = 0, maxRecords = 0, maxStates = 0;
    double maxSeconds = 0;
    uint64_t maxBytes = 4ULL << 30;
    std::string input, preload, mode = "direct";
    bool verify = false;
    int threads = 0; // explicit opt-in: independent native F4 fill, no graph keys
};

double elapsed(Clock::time_point start) {
    return std::chrono::duration<double>(Clock::now() - start).count();
}

struct Watchdog {
    std::mutex mutex;
    std::condition_variable condition;
    bool done = false;
    std::atomic<uint64_t> peakBytes{0};
    std::thread worker;
    explicit Watchdog(const Options& options) {
        worker = std::thread([this, options] {
            const auto start = Clock::now();
            std::unique_lock<std::mutex> lock(mutex);
            while (!condition.wait_for(lock, std::chrono::milliseconds(20),
                                        [this] { return done; })) {
                if (elapsed(start) > options.maxSeconds) {
                    std::fprintf(stderr, "BOUND maxseconds exceeded; unfinished input has NO exact value\n");
                    std::fflush(nullptr);
                    std::_Exit(124);
                }
#ifdef _WIN32
                using QueryMemory = BOOL (WINAPI*)(HANDLE, PPROCESS_MEMORY_COUNTERS, DWORD);
                const auto query = reinterpret_cast<QueryMemory>(
                    GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "K32GetProcessMemoryInfo"));
                PROCESS_MEMORY_COUNTERS counters{};
                counters.cb = sizeof(counters);
                if (!query || !query(GetCurrentProcess(), &counters, sizeof(counters))) {
                    std::fprintf(stderr, "BOUND cannot query process memory\n");
                    std::fflush(nullptr);
                    std::_Exit(125);
                }
                peakBytes.store(std::max<uint64_t>(peakBytes.load(), counters.PeakWorkingSetSize));
                if (counters.WorkingSetSize > options.maxBytes) {
                    std::fprintf(stderr, "BOUND maxbytes exceeded; unfinished input has NO exact value\n");
                    std::fflush(nullptr);
                    std::_Exit(125);
                }
#endif
            }
        });
    }
    ~Watchdog() {
        { std::lock_guard<std::mutex> lock(mutex); done = true; }
        condition.notify_one();
        worker.join();
    }
};

FactorCount parseCount(const std::string& text) {
    if (text.empty()) throw std::runtime_error("empty exact count");
    FactorCount value = 0;
    const FactorCount largest = ~FactorCount(0);
    for (char ch : text) {
        if (ch < '0' || ch > '9') throw std::runtime_error("invalid exact count");
        if (value > (largest - unsigned(ch - '0')) / 10)
            throw std::runtime_error("exact count exceeds u128");
        value = 10 * value + unsigned(ch - '0');
    }
    return value;
}

struct Input {
    Graph graph{};
    bool hasExpected = false;
    FactorCount expected = 0;
};

bool nextInput(std::istream& stream, int degree, Input& input) {
    std::string line;
    while (std::getline(stream, line)) {
        const auto comment = line.find('#');
        if (comment != std::string::npos) line.resize(comment);
        if (line.find_first_not_of(" \t\r\n") == std::string::npos) continue;
        input = {};
        std::istringstream row(line);
        std::array<int, MAX_M> columnDegrees{};
        for (int symbol = 0; symbol < M; ++symbol) {
            std::string token;
            if (!(row >> token)) throw std::runtime_error("input needs exactly 2C decimal masks");
            const FactorCount parsed = parseCount(token);
            if (parsed > (unsigned)FULL) throw std::runtime_error("mask outside 2C slots");
            const auto mask = (uint16_t)parsed;
            if (std::popcount(mask) != degree) throw std::runtime_error("wrong native mask degree");
            for (int box = 0; box < C; ++box)
                if (((mask >> (2 * box)) & 3u) == 3u)
                    throw std::runtime_error("symbol uses both slots of one box");
            input.graph[symbol] = mask;
            for (int slot = 0; slot < M; ++slot)
                columnDegrees[slot] += (mask >> slot) & 1u;
        }
        for (int slot = 0; slot < M; ++slot)
            if (columnDegrees[slot] != degree) throw std::runtime_error("unbalanced slot degree");
        std::string token;
        if (row >> token) {
            if (!token.starts_with("expected=")) throw std::runtime_error("unexpected input suffix");
            input.expected = parseCount(token.substr(9));
            input.hasExpected = true;
            if (row >> token) throw std::runtime_error("extra input token");
        }
        std::sort(input.graph.begin(), input.graph.begin() + M);
        return true;
    }
    return false;
}

struct Stats {
    uint64_t rows = 0, matches = 0, weakResiduals = 0, strongResiduals = 0;
    uint64_t closedHits = 0, closedMisses = 0, f4Calls = 0, f4Leaves = 0, f4Nodes = 0;
    uint64_t verified = 0, expectedChecked = 0;
    double construction = 0, sourceCanon = 0, pivot = 0, enumeration = 0;
    double residualCanon = 0, lookup = 0, value = 0, boundPrecount = 0, reference = 0;
};

struct Engine {
    Options options;
    uint64_t records = 0;
    std::unordered_map<GraphKey, FactorCount, GraphKeyHash> values;
    std::unordered_set<GraphKey, GraphKeyHash> residualInventory;
    explicit Engine(Options input) : options(std::move(input)) {}

    void reserveRecords(uint64_t count) {
        if (count > options.maxRecords - records)
            throw std::runtime_error("BOUND maxrecords would be exceeded; unfinished input has NO exact value");
        records += count;
    }

    GraphKey strongKey(const Graph& graph) {
        const auto computed = computeCanonicalGraphKey(graph, weakGraphKey(graph));
        canonSearchNodes += computed.nodes;
        ++canonComputations;
        if (computed.fallback) throw std::runtime_error("canonical budget exhausted; no weak graph quotient accepted");
        return computed.key;
    }

    void insertValue(const GraphKey& key, FactorCount value) {
        const auto found = values.find(key);
        if (found != values.end()) {
            if (found->second != value) throw std::runtime_error("inconsistent exact values in one graph class");
            return;
        }
        if (values.size() >= options.maxStates) throw std::runtime_error("BOUND exact-value cache full");
        values.emplace(key, value);
    }

    FactorCount coldValue(const Graph& graph, int degree, Stats& stats) {
        if (degree <= 1) return 1;
        if (degree == 2) return FactorCount(1) << cycleComponents(graph);
        if (degree == 4) {
            const auto beforeBound = Clock::now();
            const uint16_t first = graph[0] & (uint16_t)(-graph[0]);
            const uint16_t rest = graph[0] ^ first;
            const uint16_t pair = first | (rest & (uint16_t)(-rest));
            const uint64_t leaves = countRootedTwoFactors(graph, pair);
            reserveRecords(leaves);
            stats.boundPrecount += elapsed(beforeBound);
            const auto beforeValue = Clock::now();
            const auto result = computeDegree4RootedSplit(graph);
            stats.value += elapsed(beforeValue);
            if (result.leaves != leaves) throw std::runtime_error("F4 pre-count differs from closed split leaves");
            ++stats.f4Calls;
            stats.f4Leaves += result.leaves;
            stats.f4Nodes += result.nodes;
            return result.value;
        }
        bool complete = true;
        const FactorCount value = gather(graph, degree, stats, false, complete);
        if (!complete) throw std::runtime_error("internal gather unexpectedly incomplete");
        return value;
    }

    FactorCount gather(const Graph& graph, int degree, Stats& stats,
                       bool inventoryOnly, bool& complete) {
        if (degree <= 2) return coldValue(graph, degree, stats);
        auto stamp = Clock::now();
        const PivotEdge pivotEdge = leastFrequentRootEdge(graph);
        stats.pivot += elapsed(stamp);
        reserveRecords(pivotEdge.frequency);
        // The raw matching count bounds both materialized residual containers.
        if (pivotEdge.frequency > options.maxStates)
            throw std::runtime_error("BOUND rooted matching batch exceeds maxstates");
        std::array<uint16_t, MAX_M> chosen{};
        chosen[0] = pivotEdge.rightBit;
        std::unordered_map<GraphKey, size_t, GraphKeyHash> index;
        std::vector<Residual> residuals;
        uint64_t matchingCount = 0;
        stamp = Clock::now();
        enumeratePerfectMatchingsQuiet(graph, (uint16_t)(FULL ^ 1u), pivotEdge.rightBit,
                                       chosen, index, residuals, matchingCount);
        stats.enumeration += elapsed(stamp);
        if (matchingCount != pivotEdge.frequency || residualMultiplicitySum(residuals) != matchingCount)
            throw std::runtime_error("rooted matching frequency mismatch");
        stats.matches += matchingCount;
        stats.weakResiduals += residuals.size();
        FactorCount total = 0;
        std::unordered_set<GraphKey, GraphKeyHash> localKeys;
        for (const Residual& residual : residuals) {
            if (degree == 3) {
                total += FactorCount(residual.multiplicity) *
                         (FactorCount(1) << cycleComponents(residual.graph));
                continue;
            }
            stamp = Clock::now();
            const GraphKey key = strongKey(residual.graph);
            stats.residualCanon += elapsed(stamp);
            localKeys.insert(key);
            if (!residualInventory.contains(key)) {
                if (residualInventory.size() >= options.maxStates)
                    throw std::runtime_error("BOUND residual-key inventory full");
                residualInventory.insert(key);
            }
            stamp = Clock::now();
            const auto found = values.find(key);
            stats.lookup += elapsed(stamp);
            if (found != values.end()) {
                ++stats.closedHits;
                total += FactorCount(residual.multiplicity) * found->second;
            } else {
                ++stats.closedMisses;
                if (inventoryOnly) { complete = false; continue; }
                if (!options.preload.empty())
                    throw std::runtime_error("preloaded exact predecessor graph table has a missing class");
                const FactorCount child = coldValue(residual.graph, degree - 1, stats);
                insertValue(key, child);
                total += FactorCount(residual.multiplicity) * child;
            }
        }
        stats.strongResiduals += localKeys.size();
        return FactorCount(degree) * total;
    }

    void preload() {
        if (options.preload.empty()) return;
        std::ifstream stream(options.preload);
        if (!stream) throw std::runtime_error("cannot open preload input");
        Input input;
        uint64_t rows = 0;
        const auto start = Clock::now();
        while (nextInput(stream, options.degree - 1, input)) {
            if (++rows > options.maxStates) throw std::runtime_error("BOUND preload rows exceed maxstates");
            if (!input.hasExpected) throw std::runtime_error("preload requires expected= on every row");
            insertValue(strongKey(input.graph), input.expected);
        }
        std::printf("preload rows=%llu graph_values=%zu seconds=%.9f\n",
                    (unsigned long long)rows, values.size(), elapsed(start));
    }
};

Options parseOptions(int argc, char** argv) {
    if (argc < 3) throw std::runtime_error(
        "usage: layer_direct_bench C L input=PATH limit=N maxseconds=S maxrecords=R maxstates=K "
        "[mode=direct|gather|gatheronly] [preload=PATH] [verify] [maxbytes=B] [threads=1..24]");
    C = std::stoi(argv[1]);
    Options options;
    options.degree = std::stoi(argv[2]);
    if (C < 2 || C > 6 || options.degree < 1 || options.degree > C || options.degree > 5)
        throw std::runtime_error("requires C=2..6, L=1..min(C,5); no full C6-class evaluator");
    M = 2 * C; FULL = (1 << M) - 1; NPAT = 1 << C;
    for (int i = 3; i < argc; ++i) {
        const std::string arg = argv[i];
        const auto equals = arg.find('=');
        const std::string name = arg.substr(0, equals);
        const std::string value = equals == std::string::npos ? "" : arg.substr(equals + 1);
        if (name == "input") options.input = value;
        else if (name == "preload") options.preload = value;
        else if (name == "limit") options.limit = std::stoull(value);
        else if (name == "maxrecords") options.maxRecords = std::stoull(value);
        else if (name == "maxstates") options.maxStates = std::stoull(value);
        else if (name == "maxseconds") options.maxSeconds = std::stod(value);
        else if (name == "maxbytes") options.maxBytes = std::stoull(value);
        else if (name == "mode") options.mode = value;
        else if (name == "threads") {
            options.threads = std::stoi(value);
            if (options.threads <= 0) throw std::runtime_error("threads must be positive");
        }
        else if (arg == "verify") options.verify = true;
        else throw std::runtime_error("unknown benchmark option: " + arg);
    }
    if (options.input.empty() || options.limit == 0 || options.limit > 1000000 ||
        options.maxRecords == 0 || options.maxRecords > 1000000000ULL ||
        options.maxStates == 0 || options.maxStates > 1000000 ||
        !(options.maxSeconds > 0 && options.maxSeconds <= 600) ||
        options.maxBytes < (64ULL << 20) || options.maxBytes > (4ULL << 30))
        throw std::runtime_error("mandatory positive bounds: limit/states<=1M, records<=1B, seconds<=600, bytes=64MiB..4GiB");
    if (options.mode != "direct" && options.mode != "gather" && options.mode != "gatheronly")
        throw std::runtime_error("unknown benchmark mode");
    if (!options.preload.empty() && (options.mode == "direct" || options.degree < 2))
        throw std::runtime_error("preload is only for predecessor-table gather");
    if (options.verify && C == 6)
        throw std::runtime_error("independent plain recurrence verification is restricted to C<=5");
    if (options.threads < 0 || options.threads > 24 ||
        (options.threads && (options.degree != 4 || options.mode != "direct" ||
                             options.verify || !options.preload.empty())))
        throw std::runtime_error("threads=1..24 is only for native direct F4 batch without verify/preload");
    return options;
}

int parallelDirect(const Options& options, Watchdog& guard) {
    const auto totalStart = Clock::now();
    std::ifstream stream(options.input);
    if (!stream) throw std::runtime_error("cannot open native input");
    std::vector<Input> inputs;
    while (inputs.size() < options.limit) {
        Input input;
        if (!nextInput(stream, 4, input)) break;
        if (inputs.size() >= options.maxStates) throw std::runtime_error("BOUND batch input exceeds maxstates");
        inputs.push_back(input);
    }
    if (inputs.empty()) throw std::runtime_error("empty bounded input");
    const double constructionSeconds = elapsed(totalStart);
    std::vector<uint64_t> leaves(inputs.size());
    std::vector<Degree4RootedResult> results(inputs.size());
    std::vector<double> preCountCpu(inputs.size()), valueCpu(inputs.size());
    omp_set_dynamic(0);
    omp_set_num_threads(options.threads);
    const auto preflightStart = Clock::now();
    // This phase computes exact work reservations only. No F4 value is computed
    // until the aggregate emitted-leaf count fits the owner's hard record cap.
    #pragma omp parallel for schedule(dynamic, 1)
    for (long long i = 0; i < (long long)inputs.size(); ++i) {
        const auto stamp = Clock::now();
        const Graph& graph = inputs[(size_t)i].graph;
        const uint16_t first = graph[0] & (uint16_t)(-graph[0]);
        const uint16_t rest = graph[0] ^ first;
        const uint16_t pair = first | (rest & (uint16_t)(-rest));
        leaves[(size_t)i] = countRootedTwoFactors(graph, pair);
        preCountCpu[(size_t)i] = elapsed(stamp);
    }
    const double preflightWall = elapsed(preflightStart);
    uint64_t records = 0;
    for (uint64_t count : leaves) {
        if (count > options.maxRecords - records)
            throw std::runtime_error("BOUND batch leaf reservation exceeds maxrecords; no F4 values computed");
        records += count;
    }
    const auto kernelStart = Clock::now();
    #pragma omp parallel for schedule(dynamic, 1)
    for (long long i = 0; i < (long long)inputs.size(); ++i) {
        const auto stamp = Clock::now();
        results[(size_t)i] = computeDegree4RootedSplit(inputs[(size_t)i].graph);
        valueCpu[(size_t)i] = elapsed(stamp);
    }
    const double kernelWall = elapsed(kernelStart);
    uint64_t expectedChecked = 0, nodes = 0;
    FactorCount exactSum = 0;
    double preflightCpuSum = 0, kernelCpuSum = 0;
    for (size_t i = 0; i < inputs.size(); ++i) {
        if (results[i].leaves != leaves[i]) throw std::runtime_error("parallel F4 leaf reservation mismatch");
        if (inputs[i].hasExpected) {
            if (results[i].value != inputs[i].expected)
                throw std::runtime_error("parallel F4 differs from exact native T/m");
            ++expectedChecked;
        }
        exactSum += results[i].value;
        nodes += results[i].nodes;
        preflightCpuSum += preCountCpu[i];
        kernelCpuSum += valueCpu[i];
    }
    std::printf("benchmark C=%d L=4 mode=parallel-direct threads=%d bounded=yes "
                "graph_canonicalizations=0 checkpoint_reads=0 checkpoint_writes=0\n", C, options.threads);
    std::printf("row,F,f4_leaves,f4_nodes\n");
    for (size_t i = 0; i < inputs.size(); ++i)
        std::printf("%zu,%s,%llu,%llu\n", i + 1, u128ToString(results[i].value).c_str(),
                    (unsigned long long)results[i].leaves, (unsigned long long)results[i].nodes);
    std::printf("summary rows=%zu expected_checked=%llu reference_checked=0 records=%llu "
                "closed_misses=0 threads=%d exact_value_sum=%s f4_nodes=%llu "
                "construction_s=%.9f preflight_wall_s=%.9f kernel_wall_s=%.9f "
                "preflight_thread_elapsed_sum_s=%.9f kernel_thread_elapsed_sum_s=%.9f "
                "wall_s=%.9f sampled_peak_bytes=%llu\n", inputs.size(),
                (unsigned long long)expectedChecked, (unsigned long long)records, options.threads,
                u128ToString(exactSum).c_str(), (unsigned long long)nodes,
                constructionSeconds, preflightWall, kernelWall, preflightCpuSum, kernelCpuSum,
                elapsed(totalStart), (unsigned long long)guard.peakBytes.load());
    std::printf("[OK] bounded parallel native F4 fill; no full C6 total claimed\n");
    return 0;
}
} // namespace direct_bench

int main(int argc, char** argv) {
    try {
        using namespace direct_bench;
        const Options options = parseOptions(argc, argv);
        Watchdog guard(options);
        if (options.threads) return parallelDirect(options, guard);
        canonNodeBudget = 10000000;
        for (auto& cache : canonCacheByDegree) cache.cap = 1;
        colorPivotMaxDegree = MAX_C;
        useDegree4RootedSplit = false;
        graphCheckpointReadOnly = true;
        Engine engine(options);
        engine.preload();
        std::ifstream stream(options.input);
        if (!stream) throw std::runtime_error("cannot open native input");
        const auto start = Clock::now();
        Stats stats;
        std::printf("benchmark C=%d L=%d mode=%s bounded=yes checkpoint_reads=0 checkpoint_writes=0\n",
                    C, options.degree, options.mode.c_str());
        std::printf("row,F,construction_s,source_canon_s,pivot_s,enumeration_s,residual_canon_s,lookup_s,value_s,bound_precount_s,matches,f4_leaves\n");
        while (stats.rows < options.limit) {
            Input input;
            const auto constructionStart = Clock::now();
            if (!nextInput(stream, options.degree, input)) break;
            Stats local;
            local.construction = elapsed(constructionStart);
            auto stamp = Clock::now();
            const GraphKey key = engine.strongKey(input.graph);
            local.sourceCanon = elapsed(stamp);
            bool complete = true;
            const FactorCount value = options.mode == "direct"
                ? engine.coldValue(input.graph, options.degree, local)
                : engine.gather(input.graph, options.degree, local,
                                options.mode == "gatheronly", complete);
            if (complete) engine.insertValue(key, value);
            if (input.hasExpected && complete) {
                if (value != input.expected) throw std::runtime_error("expected native T/m differs from graph value");
                ++stats.expectedChecked;
            }
            if (options.verify && complete) {
                // Bound the opaque reference's emitted matching records before
                // calling it. With one rooted edge fixed, degree d has at most
                // d^(M-1) matching leaves, each followed by degree d-1 work.
                // This intentionally conservative guard costs no enumeration.
                FactorCount referenceBound = 0;
                FactorCount referenceStates = 1;
                for (int degree = 3; degree <= options.degree; ++degree) {
                    FactorCount matchingBound = 1;
                    for (int row = 1; row < M; ++row) matchingBound *= degree;
                    referenceBound = matchingBound * (1 + referenceBound);
                    if (degree >= 4) referenceStates = 1 + matchingBound * referenceStates;
                }
                if (referenceBound > options.maxRecords - engine.records)
                    throw std::runtime_error("BOUND conservative plain-reference record budget is too large");
                if (referenceStates > options.maxStates)
                    throw std::runtime_error("BOUND conservative plain-reference memo budget is too large");
                stamp = Clock::now();
                graphMemo.clear();
                // This independent small-C comparison uses the retained plain
                // matching recurrence, deliberately disabling rooted F4.
                const uint64_t priorMatchings = perfectMatchings;
                const FactorCount reference = countFactorizations(input.graph, options.degree);
                engine.reserveRecords(perfectMatchings - priorMatchings);
                local.reference += elapsed(stamp);
                if (reference != value) throw std::runtime_error("plain graph recurrence differs from benchmark value");
                if (graphMemo.size() > options.maxStates) throw std::runtime_error("BOUND reference memo too large");
                ++stats.verified;
                graphMemo.clear();
            }
            ++stats.rows;
#define ADD(field) stats.field += local.field
            ADD(construction); ADD(sourceCanon); ADD(pivot); ADD(enumeration);
            ADD(residualCanon); ADD(lookup); ADD(value); ADD(boundPrecount); ADD(reference);
            ADD(matches); ADD(weakResiduals); ADD(strongResiduals); ADD(closedHits); ADD(closedMisses);
            ADD(f4Calls); ADD(f4Leaves); ADD(f4Nodes);
#undef ADD
            std::printf("%llu,%s,%.9f,%.9f,%.9f,%.9f,%.9f,%.9f,%.9f,%.9f,%llu,%llu\n",
                (unsigned long long)stats.rows, complete ? u128ToString(value).c_str() : "PROBE",
                local.construction, local.sourceCanon, local.pivot, local.enumeration,
                local.residualCanon, local.lookup, local.value, local.boundPrecount,
                (unsigned long long)local.matches, (unsigned long long)local.f4Leaves);
            std::fflush(stdout);
        }
        std::printf("summary rows=%llu expected_checked=%llu reference_checked=%llu records=%llu "
                    "graph_values=%zu residual_inventory=%zu matches=%llu weak_residuals=%llu strong_residuals=%llu "
                    "closed_hits=%llu closed_misses=%llu f4_calls=%llu f4_leaves=%llu f4_nodes=%llu "
                    "construction_s=%.9f source_canon_s=%.9f pivot_s=%.9f enumeration_s=%.9f "
                    "residual_canon_s=%.9f lookup_s=%.9f value_s=%.9f bound_precount_s=%.9f "
                    "reference_s=%.9f wall_s=%.9f sampled_peak_bytes=%llu\n",
            (unsigned long long)stats.rows, (unsigned long long)stats.expectedChecked,
            (unsigned long long)stats.verified, (unsigned long long)engine.records,
            engine.values.size(), engine.residualInventory.size(),
            (unsigned long long)stats.matches, (unsigned long long)stats.weakResiduals,
            (unsigned long long)stats.strongResiduals, (unsigned long long)stats.closedHits,
            (unsigned long long)stats.closedMisses, (unsigned long long)stats.f4Calls,
            (unsigned long long)stats.f4Leaves, (unsigned long long)stats.f4Nodes,
            stats.construction, stats.sourceCanon, stats.pivot, stats.enumeration,
            stats.residualCanon, stats.lookup, stats.value, stats.boundPrecount, stats.reference,
            elapsed(start), (unsigned long long)guard.peakBytes.load());
        if (stats.rows == 0) throw std::runtime_error("empty bounded input");
        std::printf("[OK] bounded native graph evaluation; no full C6 total claimed\n");
        return 0;
    } catch (const std::exception& error) {
        std::fprintf(stderr, "layer_direct_bench: %s\n", error.what());
        return 2;
    }
}
