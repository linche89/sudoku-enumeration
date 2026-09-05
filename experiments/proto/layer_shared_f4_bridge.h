#ifndef FJ_LAYER_SHARED_F4_BRIDGE_H
#define FJ_LAYER_SHARED_F4_BRIDGE_H

#include <cstddef>
#include <cstdint>

// The separate translation unit prevents the native layer implementation's C
// and the graph engine's C from colliding. Initialize once before workers start;
// subsequently all graph-kernel workspaces used here are thread-local.
namespace shared_f4_bridge {
struct ClosedValue {
    std::uint64_t value = 0;
    std::uint64_t leaves = 0;
    std::uint64_t nodes = 0;
};

void initialize(int boxes);
int dimension() noexcept;

// Inputs are row adjacency masks of a simple 4-regular bipartite graph, with
// 2C vertices on each side. This function never loads/saves a checkpoint, uses
// graph memo values, or returns a partial accumulator. Failure throws.
ClosedValue closed(const std::uint16_t* trueSlotMasks, std::size_t count);
} // namespace shared_f4_bridge

#endif
