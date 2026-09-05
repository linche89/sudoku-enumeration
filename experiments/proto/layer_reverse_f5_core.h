// Exact reverse L5 gather, with a separately typed membership-only diagnostic.
// Include after layer_dp_gate.cpp in ONE native-engine translation unit.
// This core never reads/writes files and never accesses an old checkpoint T.
// process() requires the supplied immutable Layer.T to contain CLOSED F4,
// NOT native weighted T4. The caller must validate/convert that input first.
#ifndef FJ_LAYER_REVERSE_F5_CORE_H
#define FJ_LAYER_REVERSE_F5_CORE_H

#include <type_traits>

#define NATIVE_GATHER_ENGINE_INCLUDED
#define NATIVE_GATHER_NO_MAIN
#include "layer_native_gather_bench.cpp"
#undef NATIVE_GATHER_NO_MAIN
#undef NATIVE_GATHER_ENGINE_INCLUDED

namespace reverse_f5 {

struct Limits {
    u64 maxMatchings = 100000;
    u64 maxWeakStates = 50000;
};
struct Diagnostics {
    u64 labelledMatchings = 0, weakResiduals = 0, lookupHits = 0;
    u64 canonicalizationNodes = 0, lookupChecksum = 0;
    double conversionSeconds = 0, pivotSeconds = 0, enumerationSeconds = 0;
    double canonicalizationSeconds = 0, lookupSeconds = 0, totalSeconds = 0;
};
struct ClosedValue {
    u64 F5 = 0;
    Diagnostics diagnostics;
};
struct LookupDiagnostic {
    static constexpr const char* tag = "LOOKUP_ONLY_NO_F5";
    Diagnostics diagnostics;
    // Deliberately no F5/value member. This path never reads Layer.T.
};

inline native_gather::Graph true_slot_graph(const State& native) {
    native_gather::Graph graph{}; unsigned columnDegrees[12]{};
    for (int i = 0; i < N2C; ++i) {
        if (native.m[i] >> N2C) throw std::runtime_error("L5 source mask exceeds box domain");
        unsigned degree = 0;
        for (int box = 0; box < C; ++box) {
            const unsigned field = fld(native.m[i], box);
            if (field == 1) throw std::runtime_error("L5 source has invalid native field1");
            if (field) {
                const int slot = 2*box + int(field)-2;
                graph[i] |= u16(1u << slot); ++columnDegrees[slot]; ++degree;
            }
        }
        if (degree != 5) throw std::runtime_error("reverse F5 source has wrong row degree");
    }
    for (int i = N2C; i < 12; ++i)
        if (native.m[i]) throw std::runtime_error("nonzero L5 source padding");
    for (int slot = 0; slot < N2C; ++slot)
        if (columnDegrees[slot] != 5) throw std::runtime_error("L5 source is not balanced");
    sort_masks(graph.data(), N2C); // Same root-row convention as the proven bench.
    return graph;
}

class Worker {
    Limits limits;
    template<bool Calculate>
    auto run(const State& source, const Layer& catalogue) const {
        using Answer = std::conditional_t<Calculate, ClosedValue, LookupDiagnostic>;
        if (C < 5 || C > 6 || N2C != 2*C || g_wlseed || g_forceWide || g_rehearsalDenom)
            throw std::runtime_error("reverse F5 requires the retained C5/C6 native convention");
        if (!catalogue.size() || catalogue.table.empty() ||
            catalogue.mask + 1 != catalogue.table.size() ||
            catalogue.keys.size() < catalogue.size() || catalogue.stab.size() < catalogue.size())
            throw std::runtime_error("reverse F5 requires an immutable complete native L4 index");
        if constexpr (Calculate) {
            if (catalogue.T.size() < catalogue.size())
                throw std::runtime_error("closed F4 array is unavailable");
        }
        Answer answer;
        auto& stats = answer.diagnostics;
        const auto began = native_gather::Clock::now();
        auto stamp = native_gather::Clock::now();
        const auto graph = true_slot_graph(source);
        stats.conversionSeconds = native_gather::seconds(stamp);
        stamp = native_gather::Clock::now();
        const auto root = native_gather::pivot(graph);
        stats.pivotSeconds = native_gather::seconds(stamp);
        if (root.frequency > limits.maxMatchings)
            throw std::runtime_error("reverse F5 source exceeds the rooted matching bound");
        native_gather::Batch batch{graph, {}, {}, 0, root.frequency, limits.maxWeakStates};
        batch.chosen[0] = root.bit;
        stamp = native_gather::Clock::now();
        batch.enumerate(u16(((1u << N2C)-1)^1u), root.bit);
        stats.enumerationSeconds = native_gather::seconds(stamp);
        if (batch.records != root.frequency)
            throw std::runtime_error("reverse F5 matching enumeration disagrees with subset DP");
        stats.labelledMatchings = batch.records; stats.weakResiduals = batch.raw.size();
        u128 total = 0;
        u64 countedMultiplicity = 0;
        for (const auto& [raw, multiplicity] : batch.raw) {
            countedMultiplicity += multiplicity;
            stamp = native_gather::Clock::now();
            const u64 beforeNodes = tl_canon_nodes;
            const State key = canonize(raw);
            stats.canonicalizationNodes += tl_canon_nodes-beforeNodes;
            stats.canonicalizationSeconds += native_gather::seconds(stamp);
            stamp = native_gather::Clock::now();
            const u32 id = catalogue.find(key);
            if (id == UINT32_MAX || id >= catalogue.size() || !catalogue.stab[id])
                throw std::runtime_error("reverse F5 predecessor is absent from native L4 catalogue");
            ++stats.lookupHits;
            stats.lookupChecksum ^= sample_mix64(u64(id) ^ catalogue.keys[id].m[0] ^
                                                  (u64(catalogue.stab[id]) << 32));
            if constexpr (Calculate) {
                const u64 F4 = catalogue.T[id];
                if (!F4) throw std::runtime_error("reverse F5 predecessor has no closed F4 value");
                const u128 term = u128(multiplicity)*F4;
                if (total > ~u128(0)-term) throw std::runtime_error("reverse F5 u128 sum overflow");
                total += term;
            }
            stats.lookupSeconds += native_gather::seconds(stamp);
        }
        if (countedMultiplicity != stats.labelledMatchings)
            throw std::runtime_error("reverse F5 raw residual multiplicity mismatch");
        if constexpr (Calculate) {
            // Color the fixed edge in one of the five ordered row colors.
            // No orbit, symbol-factorial or pairing weight occurs here.
            if (!total || total > u128(UINT64_MAX)/5)
                throw std::runtime_error("closed F5 is zero or exceeds u64 storage");
            answer.F5 = u64(5*total);
        }
        stats.totalSeconds = native_gather::seconds(began);
        return answer;
    }
public:
    explicit Worker(Limits bounds = {}) : limits(bounds) {
        if (!limits.maxMatchings || limits.maxMatchings > 1000000 ||
            !limits.maxWeakStates || limits.maxWeakStates > 1000000)
            throw std::runtime_error("reverse F5 requires positive per-source bounds<=one million");
    }
    ClosedValue process(const State& nativeL5, const Layer& closedF4) const {
        return run<true>(nativeL5, closedF4);
    }
    LookupDiagnostic lookup_only(const State& nativeL5, const Layer& keyCatalogue) const {
        return run<false>(nativeL5, keyCatalogue);
    }
};

} // namespace reverse_f5
#endif
