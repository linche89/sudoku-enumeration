#!/usr/bin/env python3
"""
Reference implementation of the exact three-complementary-pair tail transform.

For a C=6 band graph, after three complete complementary row pairs have
been processed, every right column has:
  * one remaining edge in each of three complementary pairs; and
  * exactly three available colors.

The routine `three_pair_tail_value` counts all completions without creating
any lower future-twin frontier.  It splits on one remaining complementary
pair, builds a sparse four-mask kernel for each half, and takes a
complementary-signature inner product.

This file also contains a randomized reachability/self-consistency test for
the supplied G1 and G2 masks.  It does NOT compute F_6(G) by itself; it is the
terminal oracle to attach to a midpoint frontier evaluator.
"""

from __future__ import annotations

import argparse
import itertools
import random
import statistics
from collections import Counter
from dataclasses import dataclass
from typing import Dict, Iterable, Iterator, List, Mapping, Sequence, Tuple

C = 6
ALL_COLORS = (1 << C) - 1

G1_MASKS = (
    0x95A, 0x6A5, 0x4F2, 0xB0D, 0x56C, 0xA93,
    0x9B4, 0x64B, 0xE38, 0x1C7, 0xFC0, 0x03F,
)
G2_MASKS = (
    0x8EA, 0x715, 0x572, 0xA8D, 0x95C, 0x6A3,
    0xDA4, 0x25B, 0xE38, 0x1C7, 0xFC0, 0x03F,
)


def popcount(x: int) -> int:
    return x.bit_count()


def singleton_color(mask: int) -> int:
    if mask == 0 or mask & (mask - 1):
        raise ValueError(f"expected singleton color mask, got {mask:#x}")
    return mask.bit_length() - 1


def row_neighbors(mask: int) -> Tuple[int, ...]:
    ans = tuple(j for j in range(12) if (mask >> j) & 1)
    if len(ans) != C:
        raise ValueError(f"row mask {mask:#x} has degree {len(ans)}, expected {C}")
    return ans


@dataclass(frozen=True)
class HalfColumn:
    """One column as seen from a chosen cut pair."""

    d1: int
    d2: int
    available: int  # exactly three bits at the midpoint


def enumerate_distinct_color_assignments(
    allowed_masks: Sequence[int],
) -> Iterator[Tuple[int, ...]]:
    """
    Enumerate assignments of one allowed color to every column, using each
    of the six colors exactly once.

    The implementation is a small MRV backtracker.  At the three-pair
    midpoint, Bregman-Minc bounds the number of leaves by 36.
    """
    n = len(allowed_masks)
    if n != C:
        raise ValueError(f"expected {C} columns, got {n}")

    assignment = [-1] * n
    remaining = set(range(n))

    def rec(used: int) -> Iterator[Tuple[int, ...]]:
        if not remaining:
            if used == ALL_COLORS:
                yield tuple(assignment)
            return

        # Minimum remaining values; deterministic index tie-break.
        i = min(
            remaining,
            key=lambda idx: (popcount(allowed_masks[idx] & ~used), idx),
        )
        choices = allowed_masks[i] & ~used
        if choices == 0:
            return

        remaining.remove(i)
        x = choices
        while x:
            bit = x & -x
            x ^= bit
            assignment[i] = singleton_color(bit)
            yield from rec(used | bit)
        assignment[i] = -1
        remaining.add(i)

    yield from rec(0)


def half_kernel(columns: Sequence[HalfColumn]) -> Counter[int]:
    """
    Compute Gamma_H as a sparse map from four six-bit row-use masks to counts.

    Signature layout, low to high bits:
        remaining pair 1, side 0
        remaining pair 1, side 1
        remaining pair 2, side 0
        remaining pair 2, side 1
    """
    if len(columns) != C:
        raise ValueError(f"expected {C} columns in a half, got {len(columns)}")
    for col in columns:
        if col.d1 not in (0, 1) or col.d2 not in (0, 1):
            raise ValueError("side bits must be 0 or 1")
        if popcount(col.available) != 3:
            raise ValueError(
                f"midpoint column must have 3 available colors, got "
                f"{popcount(col.available)}"
            )

    allowed = [col.available for col in columns]
    kernel: Counter[int] = Counter()

    for q_assignment in enumerate_distinct_color_assignments(allowed):
        leftover: List[Tuple[int, int]] = []
        for col, q_color in zip(columns, q_assignment):
            rem = col.available & ~(1 << q_color)
            if popcount(rem) != 2:
                raise AssertionError("internal error: expected two leftover colors")
            bits = [i for i in range(C) if (rem >> i) & 1]
            leftover.append((bits[0], bits[1]))

        masks = [0, 0, 0, 0]

        def orient(i: int) -> None:
            if i == C:
                sig = (
                    masks[0]
                    | (masks[1] << 6)
                    | (masks[2] << 12)
                    | (masks[3] << 18)
                )
                kernel[sig] += 1
                return

            col = columns[i]
            a, b = leftover[i]
            row_a = col.d1
            row_b = 2 + col.d2

            for first, second in ((a, b), (b, a)):
                bit_first = 1 << first
                bit_second = 1 << second
                if masks[row_a] & bit_first:
                    continue
                if masks[row_b] & bit_second:
                    continue
                masks[row_a] |= bit_first
                masks[row_b] |= bit_second
                orient(i + 1)
                masks[row_a] ^= bit_first
                masks[row_b] ^= bit_second

        orient(0)

    return kernel


def complementary_inner_product(
    left: Mapping[int, int],
    right: Mapping[int, int],
) -> int:
    """Sum left[s] * right[bitwise-complement(s)] over 24-bit signatures."""
    full_signature = (1 << 24) - 1
    # Iterate over the smaller dictionary.
    if len(left) > len(right):
        left, right = right, left
    return sum(weight * right.get(full_signature ^ sig, 0)
               for sig, weight in left.items())


def three_pair_tail_value(
    masks: Sequence[int],
    used_color_masks: Sequence[int],
    remaining_pairs: Sequence[int] = (3, 4, 5),
    cut_pair: int | None = None,
) -> Tuple[int, int, int]:
    """
    Return (tail value, left support size, right support size).

    `used_color_masks[j]` is the set of colors already used at right column j.
    Exactly three complete row pairs must have been processed, so each entry
    must contain exactly three colors.
    """
    if len(masks) != 12 or len(used_color_masks) != 12:
        raise ValueError("expected twelve row masks and twelve column color masks")
    if len(remaining_pairs) != 3 or len(set(remaining_pairs)) != 3:
        raise ValueError("remaining_pairs must contain three distinct pair indices")
    if any(popcount(k) != 3 for k in used_color_masks):
        raise ValueError("every midpoint column must have exactly three used colors")

    neighbors = [row_neighbors(m) for m in masks]
    first_side = [set(neighbors[2 * p]) for p in range(6)]

    candidate_cuts = [cut_pair] if cut_pair is not None else list(remaining_pairs)
    best: Tuple[int, int] | None = None  # (matching proxy, pair)
    for q in candidate_cuts:
        if q not in remaining_pairs:
            raise ValueError(f"cut pair {q} is not among remaining pairs")
        proxy = 0
        for side in (0, 1):
            cols = neighbors[2 * q + side]
            allowed = [ALL_COLORS ^ used_color_masks[j] for j in cols]
            proxy += sum(1 for _ in enumerate_distinct_color_assignments(allowed))
        score = (proxy, q)
        if best is None or score < best:
            best = score

    assert best is not None
    q = best[1]
    other = [p for p in remaining_pairs if p != q]

    kernels: List[Counter[int]] = []
    for side in (0, 1):
        cols: List[HalfColumn] = []
        for j in neighbors[2 * q + side]:
            d1 = 0 if j in first_side[other[0]] else 1
            d2 = 0 if j in first_side[other[1]] else 1
            available = ALL_COLORS ^ used_color_masks[j]
            cols.append(HalfColumn(d1=d1, d2=d2, available=available))
        kernels.append(half_kernel(cols))

    value = complementary_inner_product(kernels[0], kernels[1])
    return value, len(kernels[0]), len(kernels[1])


def random_reachable_midpoint(
    masks: Sequence[int],
    rng: random.Random,
    processed_pairs: Sequence[int] = (0, 1, 2),
) -> Tuple[int, ...]:
    """Generate a random valid coloring prefix through three complete pairs."""
    neighbors = [row_neighbors(m) for m in masks]
    used = [0] * 12

    for pair in processed_pairs:
        for row in (2 * pair, 2 * pair + 1):
            cols = neighbors[row]
            allowed = [ALL_COLORS ^ used[j] for j in cols]
            assignments = list(enumerate_distinct_color_assignments(allowed))
            if not assignments:
                raise RuntimeError(
                    "random prefix reached a dead state; retry with another seed"
                )
            chosen = rng.choice(assignments)
            for j, color in zip(cols, chosen):
                used[j] |= 1 << color

    if any(popcount(k) != 3 for k in used):
        raise AssertionError("prefix did not end at a three-pair midpoint")
    return tuple(used)


def run_self_test(
    masks: Sequence[int],
    samples: int,
    seed: int,
) -> None:
    rng = random.Random(seed)
    support_by_cut: Dict[int, List[int]] = {3: [], 4: [], 5: []}
    values: List[int] = []

    completed = 0
    while completed < samples:
        try:
            used = random_reachable_midpoint(masks, rng)
        except RuntimeError:
            continue

        per_cut = {
            q: three_pair_tail_value(
                masks,
                used,
                remaining_pairs=(3, 4, 5),
                cut_pair=q,
            )
            for q in (3, 4, 5)
        }
        value_set = {triple[0] for triple in per_cut.values()}
        if len(value_set) != 1:
            raise AssertionError(
                f"cut-independence failed: {per_cut}"
            )

        values.append(next(iter(value_set)))
        for q, (_, left_size, right_size) in per_cut.items():
            support_by_cut[q].append(left_size + right_size)
        completed += 1

    print(f"samples: {samples}")
    print(f"seed: {seed}")
    for q in (3, 4, 5):
        data = support_by_cut[q]
        print(
            f"cut pair {q + 1}: mean support sum={statistics.mean(data):.3f}, "
            f"max={max(data)}"
        )
    print(
        f"tail value: mean={statistics.mean(values):.3f}, "
        f"max={max(values)}, min={min(values)}"
    )
    print("all three cut choices agreed on every sample")


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--graph", choices=("G1", "G2"), default="G2")
    parser.add_argument("--samples", type=int, default=1000)
    parser.add_argument("--seed", type=int, default=1)
    args = parser.parse_args()

    if args.samples <= 0:
        raise SystemExit("--samples must be positive")

    masks = G1_MASKS if args.graph == "G1" else G2_MASKS
    run_self_test(masks, args.samples, args.seed)


if __name__ == "__main__":
    main()
