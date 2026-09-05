#ifndef FJ_LAYER_SHARED_F4_CORE_H
#define FJ_LAYER_SHARED_F4_CORE_H

// Include this in the single native-engine translation unit, AFTER the
// retained layer_dp_gate.cpp definitions (State, Layer, C, N2C, canonize).
// The separately linked graph bridge includes no native definitions.
#include "layer_shared_f4_bridge.h"

#define LAYER_PAIRING_FIBER_EMBEDDED
#include "layer_pairing_fiber_bench.cpp"
#undef LAYER_PAIRING_FIBER_EMBEDDED

namespace shared_f4 {
struct Diagnostics {
    u64 pairings = 0;
    u64 fiberSize = 0;
    u64 canonNodes = 0;
    u64 f4Leaves = 0;
    u64 f4Nodes = 0;
    double conversionSeconds = 0;
    double sourceAuditSeconds = 0;
    double fiberSeconds = 0;
    double lookupSeconds = 0;
    double f4Seconds = 0;
};

struct Result {
    u32 alias = UINT32_MAX;
    u64 F4 = 0; // Only the representative's own ID evaluates/stores a value.
    State representativeKey{};
    Diagnostics diagnostics;
};

inline u64 coordinate_group_order() {
    if (C < 4 || C > 6 || N2C != 2*C)
        throw std::runtime_error("shared F4 core requires native C=4..6");
    u64 group = u64(1) << C;
    for (int k = 2; k <= C; ++k) group *= u64(k);
    return group;
}

// Called only after all representative values are closed. This formula never
// inspects the old T array, whose entries may be incomplete production weights.
inline u64 weighted_value(u32 stabilizer, u64 closedF4) {
    const u64 group = coordinate_group_order();
    if (!stabilizer || group % stabilizer || !closedF4)
        throw std::runtime_error("invalid stabilizer or unclosed F4 in native weight fill");
    const u128 weight = u128(group / stabilizer) * closedF4;
    if (weight > UINT64_MAX) throw std::runtime_error("native T4 would overflow u64");
    return u64(weight);
}

inline pairing_fiber::Graph true_slot_graph(const State& native) {
    pairing_fiber::Graph graph{};
    std::array<unsigned,12> columnDegrees{};
    for (int i = 0; i < N2C; ++i) {
        if (native.m[i] >> (2*C)) throw std::runtime_error("native F4 mask exceeds box domain");
        unsigned degree = 0;
        for (int box = 0; box < C; ++box) {
            const int field = fld(native.m[i], box);
            if (field == 1) throw std::runtime_error("invalid native field 1");
            if (field) {
                const int slot = 2*box + field - 2;
                graph[i] |= u16(1u << slot);
                ++columnDegrees[slot]; ++degree;
            }
        }
        if (degree != 4) throw std::runtime_error("shared F4 core input has the wrong row degree");
    }
    for (int i = N2C; i < 12; ++i)
        if (native.m[i]) throw std::runtime_error("nonzero native padding");
    for (int slot = 0; slot < N2C; ++slot)
        if (columnDegrees[slot] != 4) throw std::runtime_error("shared F4 core input is not slot-balanced");
    return graph;
}

class Worker {
    pairing_fiber::Options fiberOptions;
    pairing_fiber::Engine fiberEngine;

public:
    // 2 * 11!! = 20,790 bounds all pairings on both sides at C=6. Smaller
    // positive limits are permitted as fail-closed diagnostic work guards.
    explicit Worker(u64 maxPairingsPerState = 20790) : fiberEngine(fiberOptions) {
        coordinate_group_order();
        if (shared_f4_bridge::dimension() != C)
            throw std::runtime_error("native and graph bridge dimensions differ");
        if (!maxPairingsPerState || maxPairingsPerState > 20790)
            throw std::runtime_error("shared F4 per-state pairing cap must be 1..20790");
        if (g_wlseed) throw std::runtime_error("shared F4 requires the retained unseeded native key convention");
        fiberOptions.degree = 4;
        fiberOptions.maxPairings = maxPairingsPerState;
    }
    Worker(const Worker&) = delete;
    Worker& operator=(const Worker&) = delete;

    Result process(const State& native, const Layer& catalog, u32 currentID) {
        if (currentID >= catalog.size() || currentID >= catalog.stab.size() ||
            !catalog.stab[currentID] || catalog.keys[currentID] != native)
            throw std::runtime_error("shared F4 source ID is absent, a hole, or has a different native key");
        if (catalog.table.empty() || catalog.mask + 1 != catalog.table.size())
            throw std::runtime_error("shared F4 requires an initialized read-only native index");
        Result result;
        auto stamp = pairing_fiber::Clock::now();
        const pairing_fiber::Graph graph = true_slot_graph(native);
        result.diagnostics.conversionSeconds = pairing_fiber::seconds(stamp);
        // Audit every source independently of the catalogue's construction.
        // Regularity is checked first so malformed field/padding encodings
        // cannot reach the retained canonicalizer. Never trust a stored stab.
        stamp = pairing_fiber::Clock::now();
        u64 auditedStabilizer = 0;
        const State auditedKey = canonize(native, &auditedStabilizer);
        result.diagnostics.sourceAuditSeconds = pairing_fiber::seconds(stamp);
        if (auditedKey != native || auditedStabilizer != catalog.stab[currentID])
            throw std::runtime_error("shared F4 source is noncanonical or has an incorrect stored stabilizer");
        fiberEngine.allPairings = 0;
        const pairing_fiber::Result fiber = fiberEngine.run(graph);
        result.representativeKey = fiber.joined.front();
        result.diagnostics.pairings = fiberEngine.allPairings;
        result.diagnostics.fiberSize = fiber.joined.size();
        result.diagnostics.canonNodes = fiber.slots.canonNodes + fiber.symbols.canonNodes;
        result.diagnostics.fiberSeconds = fiber.wall;
        stamp = pairing_fiber::Clock::now();
        result.alias = catalog.find(result.representativeKey);
        result.diagnostics.lookupSeconds = pairing_fiber::seconds(stamp);
        if (result.alias == UINT32_MAX || result.alias >= catalog.size() ||
            result.alias >= catalog.stab.size() || !catalog.stab[result.alias] ||
            catalog.keys[result.alias] != result.representativeKey)
            throw std::runtime_error("exact graph representative missing from complete native catalogue");
        if (result.alias == currentID) {
            stamp = pairing_fiber::Clock::now();
            const auto closed = shared_f4_bridge::closed(graph.data(), std::size_t(N2C));
            result.diagnostics.f4Seconds = pairing_fiber::seconds(stamp);
            result.F4 = closed.value;
            result.diagnostics.f4Leaves = closed.leaves;
            result.diagnostics.f4Nodes = closed.nodes;
        }
        return result;
    }

    Result process(const Layer& catalog, u32 currentID) {
        if (currentID >= catalog.size()) throw std::runtime_error("shared F4 source ID outside catalogue");
        return process(catalog.keys[currentID], catalog, currentID);
    }
};
} // namespace shared_f4

#endif
