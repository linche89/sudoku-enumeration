#!/usr/bin/env python3
"""Enumerate every C=6 L4 raw state fixed by either order-three group type.

A stabilizer of order 12 has an element of order three. In C2 wr S6 it is
conjugate to +(3,1,1,1) or +(3,3). The exact fixed-state counts are 280 and
41,620. Thus canonicalizing this finite union covers every stabilizer-12
orbit, without an assumption about connected components.

Output is twelve space-separated true slot masks per line. Existing output
files are never replaced. No production/checkpoint input is read.
"""

import argparse
from itertools import combinations, product
from math import isfinite
from pathlib import Path
import sys
from time import perf_counter


class Bound:
    def __init__(self, seconds, max_states, max_visits):
        self.started = perf_counter()
        self.deadline = self.started + seconds
        self.max_states = max_states
        self.max_visits = max_visits
        self.visits = 0

    def check(self):
        if perf_counter() > self.deadline:
            raise TimeoutError("order-three enumeration reached its time limit")

    def visit(self):
        self.visits += 1
        if self.visits > self.max_visits:
            raise RuntimeError("order-three enumeration exceeded --max-visits")
        if self.visits % 1024 == 0:
            self.check()


def legal_masks():
    return sorted(
        sum(1 << (2 * box + side) for box, side in zip(boxes, sides))
        for boxes in combinations(range(6), 4)
        for sides in product(range(2), repeat=4)
    )


def action(mask, permutation):
    result = 0
    while mask:
        bit = mask & -mask
        mask -= bit
        slot = bit.bit_length() - 1
        result |= 1 << (2 * permutation[slot // 2] + slot % 2)
    return result


def mask_orbits(permutation):
    seen = set()
    orbits = []
    for mask in legal_masks():
        if mask in seen:
            continue
        orbit = []
        current = mask
        while current not in seen:
            orbit.append(current)
            seen.add(current)
            current = action(current, permutation)
        if current != mask or len(orbit) not in (1, 3):
            raise AssertionError("unexpected order-three mask orbit")
        orbits.append(tuple(orbit))
    if len(seen) != 240:
        raise AssertionError("legal mask inventory mismatch")
    return orbits


def enumerate_fixed(two_cycles, bound):
    permutation = (1, 2, 0, 4, 5, 3) if two_cycles else (1, 2, 0, 3, 4, 5)
    roots = (0, 1, 6, 7) if two_cycles else (0, 1, 6, 7, 8, 9, 10, 11)
    orbits = mask_orbits(permutation)
    fixed = [orbit[0] for orbit in orbits if len(orbit) == 1]
    long_orbits = [orbit for orbit in orbits if len(orbit) == 3]
    expected_orbit_inventory = (0, 80) if two_cycles else (12, 76)
    if (len(fixed), len(long_orbits)) != expected_orbit_inventory:
        raise AssertionError("wrong singleton/three-cycle mask inventory")
    contributions = [
        tuple(sum((mask >> slot) & 1 for mask in orbit) for slot in roots)
        for orbit in long_orbits
    ]

    # A singleton mask occupies all first three boxes on one common side,
    # and precisely one of the six fixed slots. Its multiplicities therefore
    # form a 2x6 nonnegative contingency table with known margins.
    singleton = [[None] * 6 for _ in range(2)]
    if not two_cycles:
        for mask in fixed:
            side = 0 if mask & 1 else 1
            slots = [slot for slot in range(6, 12) if (mask >> slot) & 1]
            if len(slots) != 1 or singleton[side][slots[0] - 6] is not None:
                raise AssertionError("singleton mask is not a 2x6 table cell")
            singleton[side][slots[0] - 6] = mask
        if any(mask is None for row in singleton for mask in row):
            raise AssertionError("singleton table is incomplete")

    selected = []
    states = []

    def accept(extra=()):
        state = tuple(sorted(
            [mask for index in selected for mask in long_orbits[index]] + list(extra)
        ))
        if len(state) != 12:
            raise AssertionError("fixed state has wrong symbol multiplicity")
        states.append(state)
        if len(states) > bound.max_states:
            raise RuntimeError("fixed-state output exceeds --max-states")

    def fill_singletons(values):
        remaining = tuple(4 - value for value in values)
        if two_cycles:
            if len(selected) == 4 and not any(remaining):
                accept()
            return

        rows, columns = remaining[:2], remaining[2:]
        if sum(rows) != sum(columns) or sum(rows) + 3 * len(selected) != 12:
            return
        suffix = [0] * 7
        for j in range(5, -1, -1):
            suffix[j] = suffix[j + 1] + columns[j]
        first_row = [0] * 6

        def table(j, left):
            if left < 0 or left > suffix[j]:
                return
            if j == 6:
                extra = []
                for k, count in enumerate(first_row):
                    extra.extend([singleton[0][k]] * count)
                    extra.extend([singleton[1][k]] * (columns[k] - count))
                accept(extra)
                return
            for count in range(min(left, columns[j]) + 1):
                first_row[j] = count
                table(j + 1, left - count)

        table(0, rows[0])

    def choose(first, values):
        bound.visit()
        fill_singletons(values)
        if len(selected) == 4:
            return
        for index in range(first, len(long_orbits)):
            updated = tuple(x + y for x, y in zip(values, contributions[index]))
            if any(x > 4 for x in updated):
                continue
            selected.append(index)
            choose(index, updated)
            selected.pop()

    choose(0, (0,) * len(roots))
    expected = 41620 if two_cycles else 280
    if len(states) != expected or len(set(states)) != expected:
        raise AssertionError(f"fixed count/uniqueness mismatch: {len(states)} != {expected}")
    for index, state in enumerate(states):
        if index % 1024 == 0:
            bound.check()
        if any(sum((mask >> slot) & 1 for mask in state) != 4 for slot in range(12)):
            raise AssertionError("fixed-state slot degrees do not balance")
        if tuple(sorted(action(mask, permutation) for mask in state)) != state:
            raise AssertionError("emitted state is not fixed by its group element")
    return sorted(states)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, help="exclusive-create a twelve-mask-per-line file")
    parser.add_argument("--time-limit", type=float, default=180.0)
    parser.add_argument("--max-states", type=int, default=50000)
    parser.add_argument("--max-visits", type=int, default=5000000)
    args = parser.parse_args()
    if not isfinite(args.time_limit) or args.time_limit <= 0:
        parser.error("--time-limit must be finite and positive")
    if args.max_states <= 0 or args.max_visits <= 0:
        parser.error("--max-states and --max-visits must be positive")
    if args.output is not None:
        if args.output.exists():
            parser.error("output already exists; it will not be replaced")
        if not args.output.parent.is_dir():
            parser.error("output parent directory must already exist")
    bound = Bound(args.time_limit, args.max_states, args.max_visits)
    first = enumerate_fixed(False, bound)
    print(f"FIXED plus=(3,1,1,1) masks=240 singleton_orbits=12 triple_orbits=76 states={len(first)}", flush=True)
    second = enumerate_fixed(True, bound)
    print(f"FIXED plus=(3,3) masks=240 singleton_orbits=0 triple_orbits=80 states={len(second)}", flush=True)
    combined = first + second
    if len(combined) > args.max_states:
        raise RuntimeError("combined output exceeds --max-states")
    distinct = len(set(combined))
    if args.output is not None:
        bound.check()
        # The complete counts, legality, fixed-point and uniqueness checks
        # have passed before a new output file is opened.
        with args.output.open("x", encoding="ascii", newline="\n") as stream:
            for state in combined:
                stream.write(" ".join(map(str, state)) + "\n")
    print(
        f"PASS records={len(combined)} distinct_raw={distinct} visits={bound.visits} "
        f"elapsed={perf_counter()-bound.started:.6f}s checkpoint_io=0 "
        f"output={args.output if args.output is not None else 'none'}",
        flush=True,
    )


if __name__ == "__main__":
    try:
        main()
    except (TimeoutError, RuntimeError, AssertionError) as error:
        print(f"STOP: {error}; no partial fixed-state inventory is accepted", file=sys.stderr)
        raise SystemExit(1)
