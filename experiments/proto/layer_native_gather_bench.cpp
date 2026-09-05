// Bounded rooted gather using the existing native canonicalizer unchanged.
// No checkpoint input/output and no production F/N accumulation.
#ifndef FJ_LAYER_NATIVE_GATHER_INCLUDED
#define FJ_LAYER_NATIVE_GATHER_INCLUDED
#ifndef NATIVE_GATHER_ENGINE_INCLUDED
#define main layer_dp_original_main
#include "layer_dp_gate.cpp"
#undef main
#endif

#include <atomic>
#include <bit>
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <unordered_set>

namespace native_gather {
using Clock = std::chrono::steady_clock;
using Graph = std::array<u16, 12>;
struct Hash {
    size_t operator()(const State& s) const {
        u64 h = 0x5344464a434b3031ULL;
        for (int i = 0; i < 3; ++i) {
            u64 word; std::memcpy(&word, s.m.data()+4*i, 8);
            h ^= word; h *= 0xff51afd7ed558ccdULL; h ^= h >> 33;
        }
        return size_t(h);
    }
};
double seconds(Clock::time_point s) {
    return std::chrono::duration<double>(Clock::now()-s).count();
}
struct Options {
    int degree = 0;
    u64 start = 0, limit = 0, maxRecords = 0, maxStates = 0;
    double maxSeconds = 0;
    std::string input, preload;
};
struct Watchdog {
    std::mutex lock;
    std::condition_variable cv;
    bool done = false;
    std::atomic<u64> peak{0};
    std::thread worker;
    explicit Watchdog(double maxSeconds) {
        worker = std::thread([this, maxSeconds] {
            const auto started = Clock::now();
            std::unique_lock<std::mutex> held(lock);
            while (!cv.wait_for(held, std::chrono::milliseconds(20), [this]{return done;})) {
                PROCESS_MEMORY_COUNTERS pm{}; pm.cb = sizeof(pm);
                if (!GetProcessMemoryInfo(GetCurrentProcess(), &pm, sizeof(pm)) ||
                    seconds(started) > maxSeconds || pm.WorkingSetSize > (2ULL << 30)) {
                    std::fprintf(stderr, "BOUND time/RSS; unfinished source has no accepted value\n");
                    std::fflush(nullptr); std::_Exit(124);
                }
                peak.store(std::max(peak.load(), u64(pm.WorkingSetSize)));
            }
        });
    }
    ~Watchdog() {
        { std::lock_guard<std::mutex> held(lock); done = true; }
        cv.notify_all(); worker.join();
    }
};
u128 decimal(const std::string& word) {
    if (word.empty()) throw std::runtime_error("empty integer");
    u128 value = 0;
    for (char c : word) {
        if (c < '0' || c > '9' || value > (~u128(0)-unsigned(c-'0'))/10)
            throw std::runtime_error("bad u128 integer");
        value = 10*value + unsigned(c-'0');
    }
    return value;
}
std::string decimal_string(u128 value) {
    std::string result;
    do { result += char('0'+value%10); value /= 10; } while(value);
    std::reverse(result.begin(), result.end()); return result;
}
struct Input { Graph graph{}; bool expected = false; u128 value = 0; };
bool next(std::istream& stream, int degree, Input& input) {
    std::string line;
    while (std::getline(stream, line)) {
        const auto comment = line.find('#');
        if (comment != std::string::npos) line.resize(comment);
        if (line.find_first_not_of(" \r\n\t") == std::string::npos) continue;
        input = {}; std::istringstream row(line); int columns[12]{};
        for (int i = 0; i < N2C; ++i) {
            std::string token; if (!(row >> token)) throw std::runtime_error("need 2C slot masks");
            const u128 number = decimal(token);
            if (number >= (u128(1) << N2C)) throw std::runtime_error("mask exceeds 2C");
            const u16 mask = u16(number);
            if (std::popcount(mask) != degree) throw std::runtime_error("wrong row degree");
            for (int b = 0; b < C; ++b)
                if (((mask >> (2*b)) & 3) == 3) throw std::runtime_error("both sides of one box");
            for (int j = 0; j < N2C; ++j) columns[j] += (mask >> j)&1;
            input.graph[i] = mask;
        }
        for (int j = 0; j < N2C; ++j)
            if (columns[j] != degree) throw std::runtime_error("wrong column degree");
        std::string token;
        if (row >> token) {
            if (!token.starts_with("expected=")) throw std::runtime_error("unexpected suffix");
            input.expected = true; input.value = decimal(token.substr(9));
            if (row >> token) throw std::runtime_error("extra suffix");
        }
        sort_masks(input.graph.data(), N2C); return true;
    }
    return false;
}
State encode(const Graph& graph) {
    State state{};
    for (int i = 0; i < N2C; ++i)
        for (int b = 0; b < C; ++b) {
            const int v = (graph[i] >> (2*b)) & 3;
            if (v) state.m[i] |= u16((v == 1 ? 2 : 3) << (2*b));
        }
    sort_masks(state.m.data(), N2C); return state;
}
struct Pivot { u16 bit = 0; u64 frequency = UINT64_MAX; };
Pivot pivot(const Graph& graph) {
    std::array<u64, 4096> dp{}; dp[0] = 1;
    const int full = (1 << N2C)-1;
    for (int row = 1; row < N2C; ++row)
        for (int used = 0; used <= full; ++used) {
            if (std::popcount(unsigned(used)) != row-1 || !dp[used]) continue;
            u16 choices = graph[row] & u16(~used);
            while (choices) { const u16 bit = choices & u16(-choices);
                choices ^= bit; dp[used|bit] += dp[used]; }
        }
    Pivot result;
    u16 choices = graph[0];
    while (choices) {
        const u16 bit = choices & u16(-choices); choices ^= bit;
        const u64 frequency = dp[full^bit];
        if (frequency < result.frequency) result = {bit, frequency};
    }
    if (!result.bit || !result.frequency) throw std::runtime_error("no root matching");
    return result;
}
struct Batch {
    const Graph& graph;
    Graph chosen{};
    std::unordered_map<State, u64, Hash> raw;
    u64 records = 0, recordLimit = 0, stateLimit = 0;
    void enumerate(u16 left, u16 used) {
        if (!left) {
            if (++records > recordLimit) throw std::runtime_error("BOUND matching records");
            Graph residual{};
            for (int i = 0; i < N2C; ++i) residual[i] = graph[i]^chosen[i];
            const State key = encode(residual);
            ++raw[key];
            if (raw.size() > stateLimit) throw std::runtime_error("BOUND raw states");
            return;
        }
        int best = -1, count = N2C+1; u16 available = 0, rows = left;
        while (rows) {
            const int i = std::countr_zero(rows); rows &= u16(rows-1);
            const u16 choices = graph[i] & u16(~used);
            const int size = std::popcount(choices);
            if (size < count) { best = i; count = size; available = choices; if (size <= 1) break; }
        }
        if (!count) return;
        while (available) {
            const u16 bit = available & u16(-available); available ^= bit;
            chosen[best] = bit; enumerate(left ^ u16(1u << best), used|bit);
        }
        chosen[best] = 0;
    }
};
int run(int argc, char** argv) {
    if (argc < 3) throw std::runtime_error("usage: C L input=... limit=... maxrecords=... maxstates=... maxseconds=... [preload=...] [start=...]");
    C = std::stoi(argv[1]); N2C = 2*C;
    Options o; o.degree = std::stoi(argv[2]);
    for (int i = 3; i < argc; ++i) {
        const std::string arg = argv[i]; const auto eq = arg.find('=');
        if (eq == std::string::npos) throw std::runtime_error("expected option=value");
        const std::string key = arg.substr(0,eq), value = arg.substr(eq+1);
        if (key == "input") o.input = value;
        else if (key == "preload") o.preload = value;
        else if (key == "limit") o.limit = std::stoull(value);
        else if (key == "start") o.start = std::stoull(value);
        else if (key == "maxrecords") o.maxRecords = std::stoull(value);
        else if (key == "maxstates") o.maxStates = std::stoull(value);
        else if (key == "maxseconds") o.maxSeconds = std::stod(value);
        else throw std::runtime_error("unknown option "+key);
    }
    if (C < 2 || C > 6 || o.degree < 2 || o.degree > C || o.input.empty() ||
        !o.limit || o.limit > 1000 || !o.maxRecords || o.maxRecords > 5000000 ||
        !o.maxStates || o.maxStates > 2000000 || o.maxSeconds <= 0 || o.maxSeconds > 120)
        throw std::runtime_error("invalid domain/bounds: limit<=1000 records<=5m states<=2m seconds<=120");
    Watchdog watchdog(o.maxSeconds); const auto wall = Clock::now();
    g_wlseed = false;
    std::unordered_map<State, u128, Hash> values;
    if (!o.preload.empty()) {
        std::ifstream preload(o.preload); if (!preload) throw std::runtime_error("preload unavailable");
        Input input;
        while (next(preload, o.degree-1, input)) {
            if (!input.expected) throw std::runtime_error("preload requires expected=F");
            const State key = canonize(encode(input.graph));
            const auto [it, added] = values.emplace(key,input.value);
            if (!added && it->second != input.value) throw std::runtime_error("inconsistent preload");
            if (values.size() > o.maxStates) throw std::runtime_error("BOUND preload states");
        }
    }
    std::cout << "preload_native_values=" << values.size() << " seconds=" << seconds(wall) << '\n';
    std::ifstream stream(o.input); if (!stream) throw std::runtime_error("input unavailable");
    Input input;
    for (u64 i = 0; i < o.start; ++i) if (!next(stream,o.degree,input)) throw std::runtime_error("start beyond input");
    std::unordered_set<State,Hash> inventory;
    u64 rows=0, records=0, weak=0, hits=0, misses=0, checked=0, nodes=0;
    double pivotTime=0, enumTime=0, canonTime=0, lookupTime=0;
    while (rows < o.limit && next(stream,o.degree,input)) {
        auto stamp = Clock::now(); const Pivot root = pivot(input.graph); pivotTime += seconds(stamp);
        if (root.frequency > o.maxRecords-records) throw std::runtime_error("BOUND source would exceed maxrecords");
        Batch batch{input.graph, {}, {}, 0, root.frequency, o.maxStates};
        batch.chosen[0] = root.bit;
        stamp = Clock::now(); batch.enumerate(u16(((1u<<N2C)-1)^1u),root.bit); enumTime += seconds(stamp);
        if (batch.records != root.frequency) throw std::runtime_error("root count mismatch");
        records += batch.records; weak += batch.raw.size();
        u128 total = 0; bool complete = !o.preload.empty(); u64 weightedRecords=0;
        for (const auto& [raw,multiplicity] : batch.raw) {
            weightedRecords += multiplicity;
            stamp = Clock::now(); const u64 oldNodes=tl_canon_nodes;
            const State key = canonize(raw); nodes += tl_canon_nodes-oldNodes; canonTime += seconds(stamp);
            inventory.insert(key); if (inventory.size() > o.maxStates) throw std::runtime_error("BOUND native inventory");
            stamp = Clock::now(); const auto found = values.find(key); lookupTime += seconds(stamp);
            if (found == values.end()) { ++misses; complete = false;
                if (!o.preload.empty()) throw std::runtime_error("missing preloaded native predecessor"); }
            else { ++hits; total += u128(multiplicity)*found->second; }
        }
        if (weightedRecords != root.frequency) throw std::runtime_error("raw multiplicity mismatch");
        total *= o.degree; ++rows;
        if (complete && input.expected) {
            if (total != input.value) throw std::runtime_error("exact per-class F mismatch");
            ++checked;
        }
        std::cout << "row=" << rows << " matches=" << batch.records << " raw=" << batch.raw.size()
                  << " F=" << (complete ? decimal_string(total) : "PROBE") << '\n';
    }
    if (!rows) throw std::runtime_error("no source processed");
    std::cout << std::fixed << std::setprecision(9)
              << "summary rows=" << rows << " expected_checked=" << checked << " matches=" << records
              << " weak_residuals=" << weak << " native_inventory=" << inventory.size()
              << " closed_hits=" << hits << " closed_misses=" << misses
              << " pivot_s=" << pivotTime << " enumeration_s=" << enumTime
              << " native_canon_s=" << canonTime << " lookup_s=" << lookupTime
              << " canon_nodes=" << nodes << " wall_s=" << seconds(wall)
              << " sampled_peak_bytes=" << watchdog.peak.load() << '\n'
              << "[OK] bounded native gather; no production checkpoint or full C6 total\n";
    return 0;
}
}
#ifndef NATIVE_GATHER_NO_MAIN
int main(int argc,char** argv) try { return native_gather::run(argc,argv); }
catch(const std::exception& e) { std::fprintf(stderr,"ERROR: %s\n",e.what()); return 1; }
#endif
#endif // FJ_LAYER_NATIVE_GATHER_INCLUDED
