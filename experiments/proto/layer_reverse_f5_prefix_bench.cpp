// Isolated compatible-prefix reverse benchmark. No released source changes.
// Full C6 remains membership-only and requires explicit ack-large-c6.
// C5 exact mode checks every final native value against independent fixtures.
#define main layer_native_original_main_for_reverse_f5_prefix_bench
#include "layer_dp_gate.cpp"
#undef main
#include "layer_shared_catalog.h"
#include "layer_two_missing_prefix_canon.h"
// Scope the substitution AFTER the native engine and loader are included.
// Unsupported/seeded/one-missing inputs retain their native canonicalizer.
#define canonize two_missing_prefix::canonicalize
#include "layer_reverse_f5_core.h"
#undef canonize

namespace reverse_f5_prefix_bench {
struct Options {
    std::string catalogue, input, preload, mode;
    u64 limit = 0, maxRecords = 0, maxRssGiB = 0;
    int threads = 0;
    double maxSeconds = 0;
    bool readonly = false, repair = false, acknowledgeLarge = false;
};

int run(int argc, char** argv) {
    if (argc < 2) throw std::runtime_error("usage: C catalog=... input=... mode=lookup|exact limit=... maxrecords=... threads=... maxseconds=... maxrssgib=... checkpointreadonly [repair-l4-support] [preload=...]");
    C = std::stoi(argv[1]); N2C = 2*C;
    Options options;
    for (int i = 2; i < argc; ++i) {
        const std::string argument = argv[i];
        if (argument == "ack-large-c6") { options.acknowledgeLarge = true; continue; }
        if (argument == "checkpointreadonly") { options.readonly = true; continue; }
        if (argument == "repair-l4-support") { options.repair = true; continue; }
        const auto at = argument.find('=');
        if (at == std::string::npos) throw std::runtime_error("expected option=value");
        const auto name = argument.substr(0, at), value = argument.substr(at+1);
        if (name == "catalog") options.catalogue = value;
        else if (name == "input") options.input = value;
        else if (name == "preload") options.preload = value;
        else if (name == "mode") options.mode = value;
        else if (name == "limit") options.limit = std::stoull(value);
        else if (name == "maxrecords") options.maxRecords = std::stoull(value);
        else if (name == "maxrssgib") options.maxRssGiB = std::stoull(value);
        else if (name == "threads") options.threads = std::stoi(value);
        else if (name == "maxseconds") options.maxSeconds = std::stod(value);
        else throw std::runtime_error("unknown option " + name);
    }
    if (C < 5 || C > 6 || !options.readonly || options.catalogue.empty() ||
        options.input.empty() || !options.limit || options.limit > 1024 ||
        !options.maxRecords || options.maxRecords > 5000000 ||
        options.threads < 1 || options.threads > 24 ||
        !options.maxRssGiB || options.maxRssGiB > 55 ||
        !std::isfinite(options.maxSeconds) || options.maxSeconds <= 0 || options.maxSeconds > 600 ||
        (options.mode != "lookup" && options.mode != "exact") ||
        (C == 6 && (options.mode != "lookup" || !options.preload.empty() || !options.repair)) ||
        (C == 5 && (options.repair || options.acknowledgeLarge || options.threads != 1 || options.maxRssGiB > 1 || options.maxSeconds > 120)) ||
        (C == 6 && !options.acknowledgeLarge) ||
        (options.mode == "exact" && (options.preload.empty() || options.limit != 355)))
        throw std::runtime_error("require bounded readonly C5/C6; C5 <=1thread/1GiB/120s; C6 LOOKUP ONLY with ack-large-c6; C5 exact all355");
    g_wlseed = false; g_forceWide = false; g_rehearsalDenom = 0;
    const auto began = native_gather::Clock::now();
    shared_catalog::Options loading;
    loading.path = options.catalogue; loading.threads = options.threads;
    loading.expectedLayer = 4; loading.checkpointReadonly = true;
    loading.repairC6 = options.repair;
    loading.maxEntries = C == 6 ? 903398621ULL : 200000ULL;
    loading.maxResidentBytes = options.maxRssGiB << 30;
    loading.maxSeconds = options.maxSeconds;
    shared_catalog::detail::LoadBudget wholeRunBudget(loading);

    // Preflight every selected rooted matching count BEFORE a giant load.
    std::ifstream input(options.input);
    if (!input) throw std::runtime_error("L5 sample/reference text unavailable");
    std::vector<native_gather::Input> rows;
    std::unordered_set<State, native_gather::Hash> exactSources;
    native_gather::Input row; u64 expectedRecords = 0;
    while (rows.size() < options.limit && native_gather::next(input, 5, row)) {
        if (options.mode == "exact" && !row.expected)
            throw std::runtime_error("C5 exact reference requires expected=F5");
        if (options.mode == "exact" &&
            !exactSources.insert(canonize(native_gather::encode(row.graph))).second)
            throw std::runtime_error("C5 final reference repeats a native orbit");
        expectedRecords += native_gather::pivot(row.graph).frequency;
        if (expectedRecords > options.maxRecords)
            throw std::runtime_error("selected sources exceed total rooted matching budget");
        rows.push_back(row);
    }
    if (rows.empty() || (options.mode == "exact" &&
        (rows.size() != 355 || native_gather::next(input, 5, row))))
        throw std::runtime_error("empty samples or incomplete/overlong C5 final reference");

    auto catalogue = shared_catalog::load(loading);
    Layer& layer = catalogue.layer;
    if (options.mode == "exact") {
        // A validated old checkpoint is not a factorization cache: the loader
        // erased T, and the production value path must reject those zeros.
        bool zeroRefused = false;
        try {
            reverse_f5::Worker{}.process(native_gather::encode(rows.front().graph), layer);
        } catch (const std::exception& exception) {
            zeroRefused = std::string(exception.what()).find("no closed F4 value") != std::string::npos;
        }
        if (!zeroRefused) throw std::runtime_error("zero/unclosed predecessor value was not refused");
        std::ifstream preload(options.preload);
        if (!preload) throw std::runtime_error("closed C5 F4 text unavailable");
        u64 loaded = 0;
        while (native_gather::next(preload, 4, row)) {
            if (!row.expected || !row.value || row.value > UINT64_MAX)
                throw std::runtime_error("preload requires positive closed u64 expected=F4");
            const State key = canonize(native_gather::encode(row.graph));
            const u32 id = layer.find(key);
            if (id == UINT32_MAX || layer.T[id])
                throw std::runtime_error("duplicate/missing C5 F4 reference key");
            layer.T[id] = u64(row.value); ++loaded;
        }
        if (loaded != layer.real_size()) throw std::runtime_error("C5 F4 reference is incomplete");
        std::cout << "closed_C5_F4_preload=" << loaded << " zero_predecessor_refused=yes\n";
    }
    std::cout << "canonicalizer=compatible_max_missing_edge native_key_semantics=UNCHANGED "
              << "source_sha256=" << catalogue.sourceSha256
              << " source_T=discarded layer_live=" << layer.real_size()
              << " read_s=" << catalogue.readSeconds << " sha256_s=" << catalogue.sha256Seconds
              << " zero_s=" << catalogue.zeroSeconds << " index_s=" << catalogue.indexSeconds
              << " mode=" << options.mode << " threads=" << options.threads << '\n' << std::flush;
    const auto querying = native_gather::Clock::now();
    std::atomic<bool> failed{false}; std::mutex errorMutex; std::string error;
    u64 records = 0, weak = 0, hits = 0, checksum = 0, checked = 0, nodes = 0;
    double conversion = 0, pivot = 0, enumeration = 0, canon = 0, lookup = 0, cpuWall = 0;
#pragma omp parallel num_threads(options.threads) \
    reduction(+:records,weak,hits,checked,nodes,conversion,pivot,enumeration,canon,lookup,cpuWall) reduction(^:checksum)
    {
        reverse_f5::Worker worker;
#pragma omp for schedule(dynamic,1)
        for (int64_t i = 0; i < int64_t(rows.size()); ++i) {
            if (failed.load(std::memory_order_relaxed)) continue;
            try {
                const State source = native_gather::encode(rows[size_t(i)].graph);
                reverse_f5::Diagnostics stats;
                if (options.mode == "exact") {
                    const auto result = worker.process(source, layer);
                    if (u128(result.F5) != rows[size_t(i)].value)
                        throw std::runtime_error("closed F5 differs from exact complete C5 reference");
                    ++checked; stats = result.diagnostics;
                } else stats = worker.lookup_only(source, layer).diagnostics;
                records += stats.labelledMatchings; weak += stats.weakResiduals;
                hits += stats.lookupHits; checksum ^= stats.lookupChecksum;
                nodes += stats.canonicalizationNodes;
                conversion += stats.conversionSeconds; pivot += stats.pivotSeconds;
                enumeration += stats.enumerationSeconds; canon += stats.canonicalizationSeconds;
                lookup += stats.lookupSeconds; cpuWall += stats.totalSeconds;
            } catch (const std::exception& exception) {
                failed.store(true, std::memory_order_relaxed);
                std::lock_guard<std::mutex> held(errorMutex);
                if (error.empty()) error = exception.what();
            }
        }
    }
    if (failed.load()) throw std::runtime_error("parallel reverse gate rejected: " + error);
    if (records != expectedRecords || hits != weak ||
        (options.mode == "exact" && checked != 355))
        throw std::runtime_error("parallel reverse gate counts disagree");
    const double querySeconds = native_gather::seconds(querying);
    shared_catalog::verify_source_unchanged(catalogue);
    PROCESS_MEMORY_COUNTERS memory{}; memory.cb = sizeof(memory);
    if (!GetProcessMemoryInfo(GetCurrentProcess(), &memory, sizeof(memory)))
        throw std::runtime_error("cannot inspect final benchmark peak RSS");
    const u64 peak = std::max(wholeRunBudget.peak.load(), u64(memory.PeakWorkingSetSize));
    std::cout << std::fixed << std::setprecision(9)
              << "summary sources=" << rows.size() << " threads=" << options.threads
              << " mode=" << options.mode << " labelled_matchings=" << records
              << " weak_residuals=" << weak << " hits=" << hits << " misses=0"
              << " checked=" << checked << " lookup_checksum=" << checksum
              << " canon_nodes=" << nodes << " conversion_cpu_s=" << conversion
              << " pivot_cpu_s=" << pivot << " enumeration_cpu_s=" << enumeration
              << " canonicalization_cpu_s=" << canon << " lookup_cpu_s=" << lookup
              << " worker_total_cpu_s=" << cpuWall << " query_wall_s=" << querySeconds
              << " total_wall_s=" << native_gather::seconds(began)
              << " peak_rss_bytes=" << peak << '\n'
              << (options.mode == "exact" ? "[OK] COMPLETE_C5_ALL355_EXACT_F5" :
                  "[OK] LOOKUP_ONLY_NO_F5 production_T_values_read=no")
              << " source_checkpointreadonly=yes file_writes=none N6=NOT_COMPUTED\n";
    return 0;
}
} // namespace reverse_f5_prefix_bench
int main(int argc, char** argv) try { return reverse_f5_prefix_bench::run(argc, argv); }
catch (const std::exception& exception) {
    std::fprintf(stderr, "ERROR: %s\n", exception.what()); return 1;
}
