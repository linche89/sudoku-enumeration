// Bounded canonicalization of explicit native witnesses, never checkpoints.
#define main layer_dp_original_main
#include "layer_dp_gate.cpp"
#undef main
#include <iostream>
#include <map>
#include <set>

int main(int argc, char** argv) try {
    if (argc != 8) throw std::runtime_error(
        "usage: layer_canonical_batch C L input output expectedRows filterStab maxSeconds");
    C = std::stoi(argv[1]); N2C = 2*C;
    const int layer = std::stoi(argv[2]);
    const u64 expectedRows = std::stoull(argv[5]), filter = std::stoull(argv[6]);
    const double bound = std::stod(argv[7]);
    if (C < 2 || C > 6 || layer < 1 || layer > C || !expectedRows ||
        expectedRows > 100000 || !filter || !(bound > 0 && bound <= 180))
        throw std::runtime_error("invalid bounded witness request");
    if (ck_path_exists(argv[4])) throw std::runtime_error("output must be new");
    const double start = now_s();
    auto guard = [&] {
        if (now_s()-start > bound) throw std::runtime_error("time bound exceeded; no accepted output");
    };
    build_group();
    auto less = [](const State& a, const State& b) { return a.m < b.m; };
    std::map<State, u64, decltype(less)> unique(less);
    std::ifstream input(argv[3]);
    if (!input) throw std::runtime_error("cannot read witness input");
    u64 rows = 0;
    std::string line;
    while (std::getline(input, line)) {
        if (auto pos = line.find('#'); pos != std::string::npos) line.resize(pos);
        if (line.find_first_not_of(" \r\n\t") == std::string::npos) continue;
        guard();
        if (++rows > expectedRows) throw std::runtime_error("too many input rows");
        std::istringstream stream(line);
        State state{};
        int degrees[12]{};
        for (int i = 0; i < N2C; ++i) {
            unsigned mask;
            if (!(stream >> mask) || mask >= (1u << N2C) || __builtin_popcount(mask) != layer)
                throw std::runtime_error("invalid true-slot mask");
            for (int b = 0; b < C; ++b) {
                const unsigned value = (mask >> (2*b)) & 3;
                if (value == 3) throw std::runtime_error("both sides in one box");
                if (value) {
                    state.m[i] |= u16((value == 1 ? 2 : 3) << (2*b));
                    ++degrees[2*b + (value == 2)];
                }
            }
        }
        if (std::any_of(degrees, degrees+N2C, [layer](int d) { return d != layer; }))
            throw std::runtime_error("unbalanced witness");
        std::string extra;
        if (stream >> extra) throw std::runtime_error("extra witness field");
        sort_masks(state.m.data(), N2C);
        u64 stabilizer;
        const State key = canonize(state, &stabilizer);
        auto [where, added] = unique.emplace(key, stabilizer);
        if (!added && where->second != stabilizer) throw std::runtime_error("inconsistent stabilizer");
    }
    if (!input.eof() || rows != expectedRows) throw std::runtime_error("wrong complete input size");
    std::map<u64, u64> histogram;
    std::vector<State> selected;
    for (const auto& [key, stabilizer] : unique) {
        ++histogram[stabilizer];
        if (stabilizer != filter) continue;
        guard();
        if (stab_scan(key) != stabilizer) throw std::runtime_error("independent full-group stabilizer mismatch");
        u64 repeated;
        if (!(canonize(key, &repeated) == key) || repeated != stabilizer)
            throw std::runtime_error("canonical idempotence mismatch");
        selected.push_back(key);
    }
    guard();
    // No partial witness file is created before every finite check passes.
    std::ofstream output(argv[4], std::ios::out | std::ios::trunc);
    if (!output) throw std::runtime_error("cannot create new witness file");
    output << "# C=" << C << " L=" << layer << " native canonical representatives; stab=" << filter << '\n';
    for (const State& key : selected) {
        for (int i = 0; i < N2C; ++i) {
            unsigned mask = 0;
            for (int b = 0; b < C; ++b) {
                const int value = fld(key.m[i], b);
                if (value) mask |= 1u << (2*b + (value & 1));
            }
            output << (i ? " " : "") << mask;
        }
        output << '\n';
    }
    output.flush();
    if (!output) throw std::runtime_error("witness output write failed");
    for (auto [stab, count] : histogram)
        std::cout << "stabilizer=" << stab << " orbits=" << count << '\n';
    std::cout << "[OK] rows=" << rows << " distinct_orbits=" << unique.size()
              << " selected=" << selected.size() << " seconds=" << now_s()-start
              << " checkpoint_io=0\n";
    return 0;
} catch (const std::exception& e) {
    std::fprintf(stderr, "layer_canonical_batch: %s\n", e.what());
    return 2;
}
