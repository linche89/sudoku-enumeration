#!/usr/bin/env python3
"""
Enumerate balanced 12-point histograms on {0,1}^r and quotient them by
the signed-coordinate group W_r = (Z/2Z)^r semidirect S_r.

This validates the small residual-skeleton counts used in the proposed
pair-respecting cross-graph schedule.  The brute-force implementation is
intended for r <= 4.
"""

from __future__ import annotations

import argparse
import itertools
from typing import Iterator, Sequence, Tuple


Histogram = Tuple[int, ...]


def balanced_histograms(r: int, total: int = 12) -> Iterator[Histogram]:
    patterns = tuple(itertools.product((0, 1), repeat=r))
    h = [0] * len(patterns)

    def rec(index: int, remaining: int, margins: Tuple[int, ...]):
        if index == len(patterns) - 1:
            value = remaining
            pattern = patterns[index]
            if all(margins[i] == value * pattern[i] for i in range(r)):
                h[index] = value
                yield tuple(h)
            return

        pattern = patterns[index]
        upper = remaining
        for i, bit in enumerate(pattern):
            if bit:
                upper = min(upper, margins[i])

        for value in range(upper + 1):
            next_margins = tuple(
                margins[i] - value * pattern[i] for i in range(r)
            )
            if any(x < 0 or x > remaining - value for x in next_margins):
                continue
            h[index] = value
            yield from rec(index + 1, remaining - value, next_margins)

    yield from rec(0, total, (total // 2,) * r)


def transform(
    h: Histogram,
    r: int,
    complement: Sequence[int],
    permutation: Sequence[int],
) -> Histogram:
    patterns = tuple(itertools.product((0, 1), repeat=r))
    index = {p: i for i, p in enumerate(patterns)}
    out = [0] * len(patterns)
    for pattern, value in zip(patterns, h):
        image = tuple(
            pattern[permutation[i]] ^ complement[i] for i in range(r)
        )
        out[index[image]] = value
    return tuple(out)


def orbit_count(r: int) -> Tuple[int, int]:
    histograms = tuple(balanced_histograms(r))
    unseen = set(histograms)
    count = 0

    complements = tuple(itertools.product((0, 1), repeat=r))
    permutations = tuple(itertools.permutations(range(r)))

    while unseen:
        h = next(iter(unseen))
        orbit = {
            transform(h, r, complement, permutation)
            for complement in complements
            for permutation in permutations
        }
        unseen.difference_update(orbit)
        count += 1

    return len(histograms), count


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--max-r", type=int, default=4)
    args = parser.parse_args()
    if not 1 <= args.max_r <= 4:
        raise SystemExit("--max-r must be between 1 and 4")

    print("r  labeled balanced histograms  W_r orbits")
    for r in range(1, args.max_r + 1):
        labeled, orbits = orbit_count(r)
        print(f"{r:<2} {labeled:<28} {orbits}")


if __name__ == "__main__":
    main()
