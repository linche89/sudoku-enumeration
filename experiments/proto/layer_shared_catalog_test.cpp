// Bounded readonly loader regression; never accepts a C6 binary catalogue.
#define main layer_dp_original_main_for_shared_catalog_test
#include "layer_dp_gate.cpp"
#undef main
#include "layer_shared_catalog.h"
#include <iostream>

int main(int argc, char** argv) try {
    if (argc != 4 && argc != 5)
        throw std::runtime_error("usage: C L disposable-small-source [sampletext]");
    const bool sampleText = argc == 5 && std::string(argv[4]) == "sampletext";
    if (argc == 5 && !sampleText) throw std::runtime_error("unknown test mode");
    C = std::stoi(argv[1]); N2C = 2*C;
    const int degree = std::stoi(argv[2]);
    if (C < 2 || C > (sampleText ? 6 : 5))
        throw std::runtime_error("small binary catalogue or explicit sample tests only");
    g_wlseed = false; g_forceWide = false; g_rehearsalDenom = 0;
    u64 oldNonzero = 0;
    if (!sampleText) {
        CkptHeader header{};
        if (!ck_read_header(argv[3], header) || header.nEntries > 200000)
            throw std::runtime_error("invalid/big test fixture");
        CkptImage before;
        if (!ck_read_file(argv[3], before)) throw std::runtime_error("fixture read failed");
        for (u64 value : before.T) oldNonzero += value != 0;
    }
    shared_catalog::Options options;
    options.path = argv[3]; options.expectedLayer = degree;
    options.threads = 8; options.checkpointReadonly = true;
    options.maxEntries = 200000; options.maxResidentBytes = 2ULL << 30;
    options.maxSeconds = 30;
    auto result = sampleText ? shared_catalog::load_text(options) : shared_catalog::load(options);
    u64 hits = 0, zeros = 0;
    for (u32 i = 0; i < result.layer.size(); ++i) {
        zeros += result.layer.T[i] == 0;
        if (result.layer.is_hole(i)) continue;
        if (result.layer.find(result.layer.keys[i]) != i)
            throw std::runtime_error("native key ID changed");
        ++hits;
    }
    if ((!sampleText && !oldNonzero) || zeros != result.layer.size())
        throw std::runtime_error("historical T not entirely erased");
    shared_catalog::verify_source_unchanged(result);
    unsigned refused = 0;
    auto requireRefusal = [&](const shared_catalog::Options& bad) {
        try {
            auto rejected = sampleText ? shared_catalog::load_text(bad) : shared_catalog::load(bad);
            (void)rejected;
        } catch (const std::exception&) { ++refused; return; }
        throw std::runtime_error("invalid bounded-loader options were accepted");
    };
    auto bad = options; bad.checkpointReadonly = false; requireRefusal(bad);
    bad = options; bad.maxEntries = 1; requireRefusal(bad);
    bad = options; bad.maxResidentBytes = 1; requireRefusal(bad);
    bad = options; bad.maxSeconds = std::numeric_limits<double>::quiet_NaN(); requireRefusal(bad);
    bad = options; bad.expectedLayer = degree == 4 ? 3 : 4; requireRefusal(bad);
    std::cout << "[OK] old_nonzero_T=" << oldNonzero << " returned_zero_T=" << zeros
              << " exact_key_ID_hits=" << hits << " source_sha256=" << result.sourceSha256
              << " source_header_hash=" << result.source.headerHash
              << " source_payload_hash=" << result.sourcePayloadHash
              << " mass=" << result.storedMass << " wall_s=" << result.wallSeconds
              << " domain=" << result.domain
              << " negative_bounds_refused=" << refused
              << " readonly=yes\n";
    return 0;
} catch (const std::exception& error) {
    std::fprintf(stderr, "ERROR: %s\n", error.what()); return 1;
}
