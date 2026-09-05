// CANDIDATE ONLY: interchangeable bridge TU, not part of released builds.
// The production main/core, native keys and chunk semantics are unchanged.
#include "layer_shared_f4_bridge.h"
#include "layer_cpu_f4_incremental_core.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <bit>
#include <limits>
#include <mutex>
#include <stdexcept>

namespace shared_f4_bridge {
namespace {
std::mutex initializationMutex;
std::atomic<int> initializedDimension{0};
struct ExternalProcessGuard {
    unsigned operator()() const noexcept { return 0; }
};
}

void initialize(int boxes) {
    if (boxes < 4 || boxes > 6)
        throw std::runtime_error("shared F4 bridge requires C=4..6");
    std::lock_guard<std::mutex> lock(initializationMutex);
    const int prior = initializedDimension.load(std::memory_order_acquire);
    if (prior != 0 && prior != boxes)
        throw std::runtime_error("shared F4 bridge cannot change dimension in a live process");
    if (prior != 0) return;
    initializedDimension.store(boxes, std::memory_order_release);
}

int dimension() noexcept {
    return initializedDimension.load(std::memory_order_acquire);
}

ClosedValue closed(const std::uint16_t* trueSlotMasks, std::size_t count) {
    const int boxes = dimension();
    if (!boxes || !trueSlotMasks || count != std::size_t(2 * boxes))
        throw std::runtime_error("uninitialized shared F4 bridge or wrong graph size");
    std::array<nodp::U16, 12> graph{};
    std::array<unsigned, 12> columnDegrees{};
    const unsigned full = (1u << (2 * boxes)) - 1;
    std::uint64_t valueBound = 1;
    for (std::size_t i = 0; i < count; ++i) {
        const std::uint16_t mask = trueSlotMasks[i];
        if ((unsigned(mask) & ~full) || std::popcount(mask) != 4)
            throw std::runtime_error("shared F4 bridge requires 4-regular row masks");
        graph[i] = mask;
        for (std::size_t j = 0; j < count; ++j)
            columnDegrees[j] += (mask >> j) & 1u;
        // F4 <= (4!)^(2C); 24^12 fits in u64. Check instead of relying
        // on an implicit implementation-width assumption.
        if (valueBound > std::numeric_limits<std::uint64_t>::max() / 24)
            throw std::runtime_error("shared F4 independent storage bound overflow");
        valueBound *= 24;
    }
    for (std::size_t j = 0; j < count; ++j)
        if (columnDegrees[j] != 4)
            throw std::runtime_error("shared F4 bridge has a nonregular column");
    std::sort(graph.begin(), graph.begin() + count);

    // No experimental 2m-node cutoff and no per-graph clock cutoff. The actual
    // production main's hard process watchdog and closed-chunk transactions
    // remain responsible for time/RSS stops. The search is finite: with the
    // first row fixed, there are at most 6^(2C-1) leaves and
    // sum_{d=0}^{2C-1} 6^d <= 435356467 nodes (C<=6), safely below u64.
    // Returned value stays zero on every nonzero core status.
    ExternalProcessGuard guard;
    const auto answer = incremental_nodp::solve(
        graph.data(), int(count), std::numeric_limits<nodp::U64>::max(), guard);
    if (answer.status != 0 || !answer.value || answer.value % 24 != 0 ||
        answer.value > valueBound)
        throw std::runtime_error("incremental shared F4 did not return a closed valid u64 value");
    return {std::uint64_t(answer.value), std::uint64_t(answer.leaves),
            std::uint64_t(answer.nodes)};
}
} // namespace shared_f4_bridge
