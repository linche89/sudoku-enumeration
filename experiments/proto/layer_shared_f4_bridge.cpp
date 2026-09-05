#include "layer_shared_f4_bridge.h"

#define main layer_shared_f4_graph_original_main
#include "../../src/factorization_orbit.cpp"
#undef main

#include <atomic>
#include <mutex>

namespace shared_f4_bridge {
namespace {
std::mutex initializationMutex;
std::atomic<int> initializedDimension{0};
}

void initialize(int boxes) {
    if (boxes < 4 || boxes > 6) throw std::runtime_error("shared F4 bridge requires C=4..6");
    std::lock_guard<std::mutex> lock(initializationMutex);
    const int prior = initializedDimension.load(std::memory_order_acquire);
    if (prior != 0 && prior != boxes)
        throw std::runtime_error("shared F4 bridge cannot change dimension in a live process");
    if (prior != 0) return;
    C = boxes;
    M = 2 * boxes;
    FULL = (1 << M) - 1;
    NPAT = 1 << boxes;
    graphCheckpointReadOnly = true;
    initializedDimension.store(boxes, std::memory_order_release);
}

int dimension() noexcept { return initializedDimension.load(std::memory_order_acquire); }

ClosedValue closed(const std::uint16_t* trueSlotMasks, std::size_t count) {
    const int boxes = dimension();
    if (!boxes || !trueSlotMasks || count != std::size_t(2 * boxes))
        throw std::runtime_error("uninitialized shared F4 bridge or wrong graph size");
    std::array<std::uint16_t, MAX_M> graph{};
    std::array<unsigned, MAX_M> columnDegrees{};
    const unsigned full = (1u << (2 * boxes)) - 1;
    for (std::size_t i = 0; i < count; ++i) {
        const std::uint16_t mask = trueSlotMasks[i];
        if ((unsigned(mask) & ~full) || std::popcount(mask) != 4)
            throw std::runtime_error("shared F4 bridge requires 4-regular row masks");
        graph[i] = mask;
        for (std::size_t j = 0; j < count; ++j) columnDegrees[j] += (mask >> j) & 1u;
    }
    for (std::size_t j = 0; j < count; ++j)
        if (columnDegrees[j] != 4) throw std::runtime_error("shared F4 bridge has a nonregular column");
    // Match the retained caller's row ordering without changing the graph.
    std::sort(graph.begin(), graph.begin() + count);
    const Degree4RootedResult answer = computeDegree4RootedSplit(graph);
    // Independently safe storage bound: at each of 2C left vertices there are
    // at most 4! local edge-color assignments, so F4 <= 24^12 < 2^64.
    if (!answer.value || answer.value > std::numeric_limits<std::uint64_t>::max())
        throw std::runtime_error("closed F4 is zero or exceeds its u64 storage bound");
    return {std::uint64_t(answer.value), answer.leaves, answer.nodes};
}
} // namespace shared_f4_bridge
