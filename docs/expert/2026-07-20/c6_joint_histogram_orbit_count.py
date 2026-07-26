#!/usr/bin/env python3
"""Exact Burnside count for paired grade-k mask states of 2xC Sudoku.

A labelled one-copy state is a (2C) x C zero-one matrix with row sum k
and column sum 2k. A paired state is an ordered pair of such matrices on
the same 2C symbol rows. We quotient by

    S_(2C) x S_C x S_C, followed by exchange of the two copies.

The script computes the exact number of orbits without enumerating states.
It reproduces the verified C=2..5 layer dimensions before reporting C=6.
Only the Python standard library is used.
"""

from __future__ import annotations

from collections import Counter, defaultdict
from functools import lru_cache
from itertools import combinations
from math import factorial
from typing import Iterable, Tuple

Partition = Tuple[int, ...]


def partitions(n: int, max_part: int | None = None) -> Iterable[Partition]:
    if n == 0:
        yield ()
        return
    if max_part is None or max_part > n:
        max_part = n
    for first in range(max_part, 0, -1):
        for rest in partitions(n - first, first):
            yield (first,) + rest


def conjugacy_class_size(part: Partition) -> int:
    counts = Counter(part)
    z = 1
    for length, multiplicity in counts.items():
        z *= (length**multiplicity) * factorial(multiplicity)
    return factorial(sum(part)) // z


def representative_permutation(part: Partition) -> tuple[int, ...]:
    """A permutation with the requested cycle type."""
    p = list(range(sum(part)))
    offset = 0
    for length in part:
        for i in range(length):
            p[offset + i] = offset + ((i + 1) % length)
        offset += length
    return tuple(p)


def permute_mask(mask: int, permutation: tuple[int, ...]) -> int:
    out = 0
    for i, image in enumerate(permutation):
        if (mask >> i) & 1:
            out |= 1 << image
    return out


@lru_cache(maxsize=None)
def row_cycle_options(length: int, col_type: Partition, k: int) -> tuple[tuple[int, ...], ...]:
    """Column-degree contributions of one row cycle.

    Pick the first row of the cycle. Invariance determines all later rows by
    repeatedly applying the column permutation. Consistency after `length`
    steps requires the first row to be fixed by alpha**length.
    """
    C = sum(col_type)
    alpha = representative_permutation(col_type)

    alpha_power = tuple(range(C))
    for _ in range(length):
        alpha_power = tuple(alpha[x] for x in alpha_power)

    options: list[tuple[int, ...]] = []
    for chosen in combinations(range(C), k):
        mask = sum(1 << c for c in chosen)
        if permute_mask(mask, alpha_power) != mask:
            continue

        contribution = [0] * C
        current = mask
        for _ in range(length):
            for c in range(C):
                contribution[c] += (current >> c) & 1
            current = permute_mask(current, alpha)
        options.append(tuple(contribution))
    return tuple(options)


@lru_cache(maxsize=None)
def fixed_one_copy(row_type: Partition, col_type: Partition, k: int) -> int:
    """Matrices fixed by a row permutation and a column permutation."""
    C = sum(col_type)
    target = (2 * k,) * C
    dp: dict[tuple[int, ...], int] = {(0,) * C: 1}

    for length in row_type:
        next_dp: dict[tuple[int, ...], int] = defaultdict(int)
        for state, value in dp.items():
            for addition in row_cycle_options(length, col_type, k):
                new_state = tuple(state[c] + addition[c] for c in range(C))
                if all(new_state[c] <= 2 * k for c in range(C)):
                    next_dp[new_state] += value
        dp = next_dp
        if not dp:
            return 0

    return dp.get(target, 0)


def square_cycle_type(part: Partition) -> Partition:
    out: list[int] = []
    for length in part:
        if length & 1:
            out.append(length)
        else:
            out.extend((length // 2, length // 2))
    return tuple(sorted(out, reverse=True))


def orbit_count(C: int, k: int) -> tuple[int, int, int]:
    """Return (full count, no-swap count, normalized swap contribution)."""
    if k == 0 or k == C:
        return (1, 1, 1)
    if not (0 <= k <= C):
        raise ValueError("k must satisfy 0 <= k <= C")

    row_types = tuple(partitions(2 * C))
    col_types = tuple(partitions(C))
    nonswap_numerator = 0
    swap_raw = 0

    for row_type in row_types:
        row_class = conjugacy_class_size(row_type)

        one_copy_sum = sum(
            conjugacy_class_size(col_type) * fixed_one_copy(row_type, col_type, k)
            for col_type in col_types
        )
        nonswap_numerator += row_class * one_copy_sum * one_copy_sum

        squared_row_type = square_cycle_type(row_type)
        squared_sum = sum(
            conjugacy_class_size(col_type) * fixed_one_copy(squared_row_type, col_type, k)
            for col_type in col_types
        )
        swap_raw += row_class * squared_sum

    nonswap_denominator = factorial(2 * C) * factorial(C) ** 2
    if nonswap_numerator % nonswap_denominator:
        raise ArithmeticError("non-swap Burnside sum is not integral")
    nonswap = nonswap_numerator // nonswap_denominator

    # For (sigma, alpha, beta) followed by copy exchange, a fixed pair is
    # determined by one matrix fixed by (sigma**2, alpha*beta). For each
    # product alpha*beta there are C! choices of (alpha,beta).
    swap_denominator = factorial(2 * C) * factorial(C)
    if swap_raw % swap_denominator:
        raise ArithmeticError("swap Burnside sum is not integral")
    swap_contribution = swap_raw // swap_denominator

    if (nonswap + swap_contribution) % 2:
        raise ArithmeticError("full Burnside average is not integral")
    full = (nonswap + swap_contribution) // 2
    return full, nonswap, swap_contribution


def layer_dimensions(C: int) -> list[int]:
    return [orbit_count(C, k)[0] for k in range(C + 1)]


def main() -> None:
    verified = {
        2: [1, 2, 1],
        3: [1, 3, 3, 1],
        4: [1, 5, 141, 5, 1],
        5: [1, 7, 38801, 38801, 7, 1],
    }
    for C, expected in verified.items():
        got = layer_dimensions(C)
        if got != expected:
            raise AssertionError(f"C={C}: expected {expected}, got {got}")
        print(f"C={C}: {got}  [verified match]")

    c6 = layer_dimensions(6)
    print(f"C=6: {c6}")
    print("C=6 k=2 details (full, no-swap, swap contribution):", orbit_count(6, 2))
    print("C=6 k=3 details (full, no-swap, swap contribution):", orbit_count(6, 3))


if __name__ == "__main__":
    main()
