// Exact graph fibers represented by retained native canonical keys.
// No graph catalogue, checkpoint reads/writes, or production F/N accumulation.
// Proof: a native presentation of Q is precisely a perfect matching of its
// slot vertices whose paired neighborhoods are disjoint. Relabelled pairings
// produce the same native key iff they differ by a bipartite automorphism.
// Thus enumerating pairings and deduplicating native keys gives the exact
// fiber. Union with the transpose fiber also quotients whole-part exchange;
// the smallest native key of that union is an exact graph canonical key.
#ifndef LAYER_PAIRING_FIBER_EMBEDDED
#define main layer_dp_pairing_fiber_original_main
#include "layer_dp_gate.cpp"
#undef main
#endif

#include <atomic>
#include <bit>
#include <condition_variable>
#include <functional>
#include <iostream>
#include <map>
#include <mutex>
#include <set>
#include <thread>
#include <unordered_map>
#include <unordered_set>

namespace pairing_fiber {
using Clock = std::chrono::steady_clock;
using Graph = std::array<u16, 12>;
using Fiber = std::vector<State>;
struct Less { bool operator()(const State& a, const State& b) const { return a.m < b.m; } };
struct Hash { size_t operator()(const State& state) const { return size_t(state_hash(state)); } };
double seconds(Clock::time_point start) {
    return std::chrono::duration<double>(Clock::now() - start).count();
}
struct Options {
    int degree = 0;
    u64 limit = 0, maxPairings = 0, invariance = 0;
    double maxSeconds = 0;
    std::string input;
    bool census = false, emitFibers = false, witnessCheck = false;
};
struct Watchdog {
    std::mutex mutex;
    std::condition_variable cv;
    bool done = false;
    std::atomic<u64> peak{0};
    std::thread worker;
    explicit Watchdog(double maxSeconds) {
        worker = std::thread([this, maxSeconds] {
            const auto start = Clock::now();
            std::unique_lock<std::mutex> lock(mutex);
            while (!cv.wait_for(lock, std::chrono::milliseconds(20), [this] { return done; })) {
                PROCESS_MEMORY_COUNTERS memory{}; memory.cb = sizeof(memory);
                if (!GetProcessMemoryInfo(GetCurrentProcess(), &memory, sizeof(memory)) ||
                    memory.WorkingSetSize > (2ULL << 30) || seconds(start) > maxSeconds) {
                    std::fprintf(stderr, "BOUND time/RSS; unfinished fiber is not accepted\n");
                    std::fflush(nullptr); std::_Exit(124);
                }
                peak.store(std::max(peak.load(), u64(memory.PeakWorkingSetSize)));
            }
        });
    }
    ~Watchdog() {
        { std::lock_guard<std::mutex> lock(mutex); done = true; }
        cv.notify_one(); worker.join();
    }
};
u128 decimal(const std::string& token) {
    if (token.empty()) throw std::runtime_error("empty decimal");
    u128 result = 0;
    for (char ch : token) {
        if (ch < '0' || ch > '9' || result > (~u128(0) - unsigned(ch - '0')) / 10)
            throw std::runtime_error("invalid decimal");
        result = 10 * result + unsigned(ch - '0');
    }
    return result;
}
Graph transpose(const Graph& graph) {
    Graph result{};
    for (int i = 0; i < N2C; ++i)
        for (int j = 0; j < N2C; ++j)
            if (graph[i] & (1u << j)) result[j] |= u16(1u << i);
    return result;
}
bool nativePresentation(const Graph& graph) {
    for (int i = 0; i < N2C; ++i)
        for (int b = 0; b < C; ++b)
            if (((graph[i] >> (2 * b)) & 3) == 3) return false;
    return true;
}
State encodeOriginal(const Graph& graph) {
    if (!nativePresentation(graph)) throw std::runtime_error("original slot pairing is not native");
    State state{};
    for (int i = 0; i < N2C; ++i)
        for (int b = 0; b < C; ++b) {
            const int field = (graph[i] >> (2 * b)) & 3;
            if (field) state.m[i] |= u16((field == 1 ? 2 : 3) << (2 * b));
        }
    sort_masks(state.m.data(), N2C);
    return state;
}
struct Input { Graph graph{}; bool hasExpected = false; u128 expected = 0; };
bool nextInput(std::istream& stream, int degree, Input& input) {
    std::string line;
    while (std::getline(stream, line)) {
        const auto comment = line.find('#');
        if (comment != std::string::npos) line.resize(comment);
        if (line.find_first_not_of(" \t\r\n") == std::string::npos) continue;
        input = {}; std::istringstream row(line);
        std::array<int, 12> columnDegree{};
        for (int i = 0; i < N2C; ++i) {
            std::string token;
            if (!(row >> token)) throw std::runtime_error("need exactly 2C slot-mask rows");
            const u128 parsed = decimal(token);
            if (parsed >= (u128(1) << N2C)) throw std::runtime_error("mask exceeds slot domain");
            const u16 mask = u16(parsed);
            if (std::popcount(mask) != degree) throw std::runtime_error("wrong row degree");
            input.graph[i] = mask;
            for (int j = 0; j < N2C; ++j) columnDegree[j] += (mask >> j) & 1;
        }
        for (int j = 0; j < N2C; ++j)
            if (columnDegree[j] != degree) throw std::runtime_error("wrong column degree");
        std::string token;
        if (row >> token) {
            if (!token.starts_with("expected=")) throw std::runtime_error("unexpected input suffix");
            input.expected = decimal(token.substr(9)); input.hasExpected = true;
            if (row >> token) throw std::runtime_error("extra input suffix");
        }
        return true;
    }
    return false;
}
void printKey(const State& state) {
    std::cout << '[';
    for (int i = 0; i < N2C; ++i) std::cout << (i ? "," : "") << state.m[i];
    std::cout << ']';
}
void printFiber(const Fiber& fiber) {
    std::cout << '[';
    for (size_t i = 0; i < fiber.size(); ++i) {
        if (i) std::cout << ',';
        printKey(fiber[i]);
    }
    std::cout << ']';
}
struct Side {
    u64 pairings = 0, canonNodes = 0;
    double canonSeconds = 0;
    Fiber fiber;
};
struct Result {
    Side slots, symbols;
    Fiber joined;
    bool selfTranspose = false;
    double wall = 0;
};
struct Engine {
    const Options& options;
    u64 allPairings = 0;
    explicit Engine(const Options& input) : options(input) {}
    Side side(const Graph& graph) {
        const Graph columns = transpose(graph);
        std::array<u16, 12> compatible{};
        for (int i = 0; i < N2C; ++i)
            for (int j = i + 1; j < N2C; ++j)
                if (!(columns[i] & columns[j])) {
                    compatible[i] |= u16(1u << j);
                    compatible[j] |= u16(1u << i);
                }
        Side result;
        std::array<u8, 12> order{};
        std::function<void(u16, int)> visit = [&](u16 remaining, int boxes) {
            if (!remaining) {
                if (allPairings >= options.maxPairings)
                    throw std::runtime_error("BOUND maxpairings; unfinished fiber is not accepted");
                ++allPairings; ++result.pairings;
                State raw{};
                for (int i = 0; i < N2C; ++i)
                    for (int b = 0; b < C; ++b) {
                        if (graph[i] & (1u << order[2*b])) raw.m[i] |= u16(2u << (2*b));
                        else if (graph[i] & (1u << order[2*b+1])) raw.m[i] |= u16(3u << (2*b));
                    }
                sort_masks(raw.m.data(), N2C);
                const u64 priorNodes = tl_canon_nodes;
                const auto start = Clock::now();
                result.fiber.push_back(canonize(raw));
                result.canonSeconds += seconds(start);
                result.canonNodes += tl_canon_nodes - priorNodes;
                return;
            }
            const int first = std::countr_zero(remaining);
            const u16 rest = remaining ^ u16(1u << first);
            u16 choices = compatible[first] & rest;
            while (choices) {
                const u16 bit = choices & u16(-choices); choices ^= bit;
                order[2*boxes] = u8(first);
                order[2*boxes+1] = u8(std::countr_zero(bit));
                visit(rest ^ bit, boxes + 1);
            }
        };
        visit(u16((1u << N2C) - 1), 0);
        std::sort(result.fiber.begin(), result.fiber.end(), Less{});
        result.fiber.erase(std::unique(result.fiber.begin(), result.fiber.end()), result.fiber.end());
        return result;
    }
    Result run(const Graph& graph) {
        const auto start = Clock::now();
        Result result;
        result.slots = side(graph);
        result.symbols = side(transpose(graph));
        std::set_union(result.slots.fiber.begin(), result.slots.fiber.end(),
                       result.symbols.fiber.begin(), result.symbols.fiber.end(),
                       std::back_inserter(result.joined), Less{});
        if (result.joined.empty()) throw std::runtime_error("graph has no native presentation on either side");
        result.selfTranspose = result.joined.size() < result.slots.fiber.size() + result.symbols.fiber.size();
        if (result.selfTranspose && result.slots.fiber != result.symbols.fiber)
            throw std::runtime_error("overlapping transpose fibers must be identical");
        result.wall = seconds(start);
        return result;
    }
};
struct Group {
    Fiber fiber;
    u64 members = 0;
    bool hasExpected = false;
    u128 expected = 0;
};
using Census = std::unordered_map<State, Group, Hash>;
void addCensus(Census& census, const Fiber& fiber, const Input& input) {
    if (fiber.empty()) throw std::runtime_error("native census has empty slot fiber");
    auto [entry, inserted] = census.try_emplace(fiber.front());
    Group& group = entry->second;
    if (inserted) group.fiber = fiber;
    else if (group.fiber != fiber) throw std::runtime_error("same canonical graph key has inconsistent fiber");
    ++group.members;
    if (input.hasExpected) {
        if (group.hasExpected && group.expected != input.expected)
            throw std::runtime_error("same unpaired graph has inconsistent expected F");
        group.expected = input.expected; group.hasExpected = true;
    }
}
u64 closeCensus(const Census& census, const std::unordered_set<State, Hash>& natives) {
    u64 total = 0;
    for (const auto& [key, group] : census) {
        if (!natives.contains(key)) throw std::runtime_error("fiber minimum missing from complete native input");
        if (group.members != group.fiber.size()) throw std::runtime_error("graph census multiplicity differs from exact fiber size");
        for (const State& member : group.fiber)
            if (!natives.contains(member)) throw std::runtime_error("fiber member missing from complete native input");
        total += group.fiber.size();
    }
    if (total != natives.size()) throw std::runtime_error("fiber sum does not partition complete native input");
    return total;
}
u64 knownLayerCount(int c, int layer) {
    static const u64 counts[6][6] = {
        {}, {}, {0,1,2}, {0,1,5,4}, {0,1,23,54,26}, {0,1,107,16150,17120,355}
    };
    return c <= 5 ? counts[c][layer] : 0;
}
Graph permuteGraph(const Graph& graph, const std::array<int,12>& rows,
                   const std::array<int,12>& columns) {
    Graph result{};
    for (int i = 0; i < N2C; ++i)
        for (int j = 0; j < N2C; ++j)
            if (graph[i] & (1u << j)) result[rows[i]] |= u16(1u << columns[j]);
    return result;
}
void invariantCheck(Engine& engine, const Graph& graph, const Result& reference,
                    std::mt19937_64& random, u64 serial) {
    std::array<int,12> rows{}, columns{};
    for (int i = 0; i < N2C; ++i) rows[i] = columns[i] = i;
    std::shuffle(rows.begin(), rows.begin() + N2C, random);
    std::shuffle(columns.begin(), columns.begin() + N2C, random);
    Graph transformed = permuteGraph(graph, rows, columns);
    if (serial & 1) transformed = transpose(transformed);
    if (engine.run(transformed).joined != reference.joined)
        throw std::runtime_error("arbitrary row/slot permutation or transpose changed graph fiber");
    std::array<int,6> boxes{};
    for (int b = 0; b < C; ++b) boxes[b] = b;
    std::shuffle(boxes.begin(), boxes.begin() + C, random);
    for (int b = 0; b < C; ++b) {
        const int flip = int(random() & 1);
        columns[2*b] = 2*boxes[b] + flip;
        columns[2*b+1] = 2*boxes[b] + (flip ^ 1);
    }
    if (engine.run(permuteGraph(graph, rows, columns)).joined != reference.joined)
        throw std::runtime_error("symbol/box permutation and flips changed graph fiber");
}
Options parse(int argc, char** argv) {
    if (argc < 3) throw std::runtime_error(
        "usage: C L input=PATH limit=N maxpairings=R maxseconds=S [census] [invariance=N] [emitfibers] [witnesscheck]");
    C = std::stoi(argv[1]); N2C = 2*C;
    Options options; options.degree = std::stoi(argv[2]);
    for (int i = 3; i < argc; ++i) {
        const std::string arg = argv[i]; const auto split = arg.find('=');
        const std::string name = arg.substr(0, split);
        const std::string value = split == std::string::npos ? "" : arg.substr(split + 1);
        if (name == "input") options.input = value;
        else if (name == "limit") options.limit = std::stoull(value);
        else if (name == "maxpairings") options.maxPairings = std::stoull(value);
        else if (name == "maxseconds") options.maxSeconds = std::stod(value);
        else if (name == "invariance") options.invariance = std::stoull(value);
        else if (arg == "census") options.census = true;
        else if (arg == "emitfibers") options.emitFibers = true;
        else if (arg == "witnesscheck") options.witnessCheck = true;
        else throw std::runtime_error("unknown fiber benchmark option");
    }
    if (C < 2 || C > 6 || options.degree < 1 || options.degree > C || options.input.empty() ||
        !options.limit || options.limit > 20000 || !options.maxPairings || options.maxPairings > 5000000 ||
        !(options.maxSeconds > 0 && options.maxSeconds <= 180) || options.invariance > options.limit)
        throw std::runtime_error("requires C2..6 L1..C, positive limit<=20000 pairings<=5M seconds<=180");
    if (options.census && (C == 6 || options.limit != knownLayerCount(C, options.degree)))
        throw std::runtime_error("census requires exact complete C2..5 native layer count");
    if (options.witnessCheck && (C != 4 || options.degree != 3))
        throw std::runtime_error("external paired/graph witness has C4 L3");
    return options;
}
int run(int argc, char** argv) {
    const Options options = parse(argc, argv);
    Watchdog guard(options.maxSeconds);
    const auto start = Clock::now();
    g_wlseed = false;
    Engine engine(options);
    if (options.witnessCheck) {
        Graph x{21,21,69,74,82,162,168,168};
        Graph y{21,21,69,82,98,138,168,168};
        if (engine.run(x).joined != engine.run(y).joined)
            throw std::runtime_error("external C4 used-graph witness split");
        std::cout << "{\"external_witness_check\":\"passed\"}\n";
    }
    std::ifstream stream(options.input);
    if (!stream) throw std::runtime_error("cannot open graph input");
    std::mt19937_64 random(20260905);
    std::unordered_set<State, Hash> natives;
    Census rightCensus, fullCensus;
    std::map<size_t,u64> rightDistribution, fullDistribution;
    u64 rows = 0, nativeRows = 0, pairings = 0, nodes = 0, invarianceChecked = 0;
    double canonSeconds = 0, fiberSeconds = 0, inverseSum = 0, inverseSq = 0;
    double inverseRightSum = 0, inverseRightSq = 0;
    Input input;
    while (rows < options.limit && nextInput(stream, options.degree, input)) {
        const Result result = engine.run(input.graph);
        ++rows;
        pairings += result.slots.pairings + result.symbols.pairings;
        nodes += result.slots.canonNodes + result.symbols.canonNodes;
        canonSeconds += result.slots.canonSeconds + result.symbols.canonSeconds;
        fiberSeconds += result.wall;
        ++rightDistribution[result.slots.fiber.size()]; ++fullDistribution[result.joined.size()];
        const double inverse = 1.0 / result.joined.size();
        inverseSum += inverse; inverseSq += inverse * inverse;
        const bool isNative = nativePresentation(input.graph);
        if (isNative) {
            ++nativeRows;
            const double invRight = 1.0 / result.slots.fiber.size();
            inverseRightSum += invRight; inverseRightSq += invRight * invRight;
        }
        if (options.census) {
            if (!isNative) throw std::runtime_error("census row is not a native presentation");
            const State original = canonize(encodeOriginal(input.graph));
            if (!natives.insert(original).second) throw std::runtime_error("duplicate native census row");
            if (!std::binary_search(result.slots.fiber.begin(), result.slots.fiber.end(), original, Less{}))
                throw std::runtime_error("source native key missing from its own pairing fiber");
            addCensus(rightCensus, result.slots.fiber, input);
            addCensus(fullCensus, result.joined, input);
        }
        if (rows <= options.invariance) {
            invariantCheck(engine, input.graph, result, random, rows); ++invarianceChecked;
        }
        std::cout << "{\"sample\":" << rows << ",\"slot_pairings\":" << result.slots.pairings
                  << ",\"symbol_pairings\":" << result.symbols.pairings
                  << ",\"native_fiber_no_transpose\":" << result.slots.fiber.size()
                  << ",\"native_fiber_symbol_side\":" << result.symbols.fiber.size()
                  << ",\"self_transpose\":" << (result.selfTranspose ? "true" : "false")
                  << ",\"native_fiber_with_transpose\":" << result.joined.size()
                  << ",\"native_key\":";
        printKey(result.joined.front());
        std::cout << ",\"canonicalization_s\":" << std::setprecision(12)
                  << result.slots.canonSeconds + result.symbols.canonSeconds
                  << ",\"fiber_wall_s\":" << result.wall;
        if (options.emitFibers) { std::cout << ",\"fiber\":"; printFiber(result.joined); }
        std::cout << "}\n";
    }
    if (!rows) throw std::runtime_error("empty fiber input");
    if (options.census) {
        if (rows != knownLayerCount(C, options.degree) || nextInput(stream, options.degree, input))
            throw std::runtime_error("census input has the wrong complete size");
        const u64 rightMass = closeCensus(rightCensus, natives);
        const u64 fullMass = closeCensus(fullCensus, natives);
        std::cout << "{\"census\":\"passed\",\"native_states\":" << rows
                  << ",\"graph_classes_no_transpose\":" << rightCensus.size()
                  << ",\"graph_classes_with_transpose\":" << fullCensus.size()
                  << ",\"fiber_sum_no_transpose\":" << rightMass
                  << ",\"fiber_sum_with_transpose\":" << fullMass << "}\n";
    }
    const auto se = [](double sum, double square, u64 n) {
        return n > 1 ? std::sqrt(std::max(0.0, (square - sum*sum/n) / (n*(n-1)))) : 0.0;
    };
    std::cout << std::setprecision(12) << "{\"summary\":true,\"rows\":" << rows
              << ",\"native_rows\":" << nativeRows << ",\"pairings\":" << pairings
              << ",\"all_pairings_including_checks\":" << engine.allPairings
              << ",\"canon_nodes\":" << nodes << ",\"canonicalization_s\":" << canonSeconds
              << ",\"fiber_wall_s\":" << fiberSeconds
              << ",\"mean_inverse_fiber_no_transpose\":" << (nativeRows ? inverseRightSum/nativeRows : 0)
              << ",\"se_inverse_fiber_no_transpose\":" << se(inverseRightSum, inverseRightSq, nativeRows)
              << ",\"mean_inverse_fiber_with_transpose\":" << inverseSum/rows
              << ",\"se_inverse_fiber_with_transpose\":" << se(inverseSum, inverseSq, rows)
              << ",\"invariance_checked\":" << invarianceChecked
              << ",\"wall_s\":" << seconds(start) << ",\"sampled_peak_bytes\":" << guard.peak.load()
              << ",\"fiber_distribution_no_transpose\":{";
    bool first = true;
    for (const auto& [size,count] : rightDistribution) {
        std::cout << (first ? "" : ",") << '"' << size << "\":" << count; first = false;
    }
    std::cout << "},\"fiber_distribution_with_transpose\":{"; first = true;
    for (const auto& [size,count] : fullDistribution) {
        std::cout << (first ? "" : ",") << '"' << size << "\":" << count; first = false;
    }
    std::cout << "}}\n[OK] exact bounded pairing fibers; no production count or checkpoint mutation\n";
    return 0;
}
} // namespace pairing_fiber

#ifndef LAYER_PAIRING_FIBER_EMBEDDED
int main(int argc, char** argv) try { return pairing_fiber::run(argc, argv); }
catch (const std::exception& error) { std::fprintf(stderr, "ERROR: %s\n", error.what()); return 1; }
#endif
