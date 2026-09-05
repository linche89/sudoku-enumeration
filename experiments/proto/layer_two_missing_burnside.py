#!/usr/bin/env python3
"""Exact native L=C-2 orbit counts; no checkpoint or generated-file I/O.

The missing-box multigraph is loopless and 4-regular. Nonidentity signed
box permutations leave at most C-1 independent side-degree coordinates.
See docs/math/two-missing-layer-burnside.md for the counting bijections.
"""

import argparse
from collections import Counter, defaultdict
from functools import lru_cache
from itertools import combinations, product
from math import factorial, isfinite
import sys
from time import perf_counter


EXPECTED = {
    # C: (labelled missing graphs, raw states, Burnside numerator, orbits)
    2: (1, 1, 8, 1),
    3: (1, 1, 48, 1),
    4: (15, 2019, 8832, 23),
    5: (158, 59661280, 62016000, 16150),
    6: (3355, 41602261536160, 41628607626240, 903398603),
}

# Independently checked with a direct 240-mask-orbit generating function.
C6_FIXED_CHECKS = {
    ((), (1, 1, 1, 1, 1, 1)): 25685440,
    ((6,), ()): 318,
    ((), (6,)): 20,
    ((3, 3), ()): 41620,
    ((), (3, 3)): 364,
    ((2, 2, 2), ()): 46345596,
    ((), (2, 2, 2)): 5792,
    ((1, 1), (2, 2)): 784,
    ((2,), (1, 1, 1, 1)): 16890248,
    ((4,), (2,)): 4362,
    ((1,), (5,)): 0,
}


class WorkBound:
    def __init__(self, seconds, max_states):
        self.started = perf_counter()
        self.deadline = self.started + seconds
        self.max_states = max_states

    def check(self):
        if perf_counter() > self.deadline:
            raise TimeoutError("cooperative wall-time limit reached")

    def check_frontier(self, size):
        if size > self.max_states:
            raise RuntimeError(
                f"DP frontier {size} exceeds --max-states {self.max_states}"
            )
        self.check()


def partitions(n, lo=1):
    if not n:
        yield ()
    for d in range(lo, n + 1):
        for part in partitions(n - d, d):
            yield (d,) + part


def zpart(part):
    result = 1
    for degree, multiplicity in Counter(part).items():
        result *= degree ** multiplicity * factorial(multiplicity)
    return result


def missing_graphs(C):
    """All labelled loopless 4-regular multigraphs on C boxes, once each."""
    degrees = [4] * C
    edges = []

    def vertex(i):
        if i == C - 1:
            if degrees[i] == 0:
                yield tuple(edges)
            return

        def row(j, remaining):
            if j == C:
                if remaining == 0:
                    yield from vertex(i + 1)
                return
            for count in range(min(remaining, degrees[j]) + 1):
                degrees[j] -= count
                if count:
                    edges.append((i, j, count))
                yield from row(j + 1, remaining - count)
                if count:
                    edges.pop()
                degrees[j] += count

        yield from row(i + 1, degrees[i])

    yield from vertex(0)


PARTITION_OPTIONS = {
    m: [(part, factorial(m) // zpart(part)) for part in partitions(m)]
    for m in range(1, 5)
}


@lru_cache(None)
def side_coefficient(degrees, target):
    coefficients = [1] + [0] * target
    for degree in degrees:
        for k in range(target, degree - 1, -1):
            coefficients[k] += coefficients[k - degree]
    return coefficients[target]


def raw_native(C, graphs, bound):
    """Identity fixed points by the multiset cycle index, integer arithmetic."""
    total = 0
    terms = 0
    for edges in graphs:
        bound.check()
        numerator = 0
        denominator = 1
        for _, _, multiplicity in edges:
            denominator *= factorial(multiplicity)

        for choices in product(*(PARTITION_OPTIONS[m] for _, _, m in edges)):
            ways = 1
            degrees = [[] for _ in range(C)]
            for (i, j, _), (part, coefficient) in zip(edges, choices):
                ways *= coefficient
                for box in range(C):
                    if box != i and box != j:
                        degrees[box].extend(part)
            for box in range(C):
                ways *= side_coefficient(tuple(sorted(degrees[box])), C - 2)
            numerator += ways
            terms += 1

        if numerator % denominator:
            raise AssertionError("nonintegral missing-graph decoration count")
        total += numerator // denominator
    return total, terms


def signed_conjugacy(C):
    """Signed cycle type, representative, positive-cycle roots and class size."""
    order = 2 ** C * factorial(C)
    for positive_size in range(C + 1):
        for plus in partitions(positive_size):
            for minus in partitions(C - positive_size):
                permutation = list(range(C))
                flips = [0] * C
                roots = []
                offset = 0
                for sign, part in ((0, plus), (1, minus)):
                    for length in part:
                        for j in range(length):
                            permutation[offset + j] = offset + (j + 1) % length
                        flips[offset + length - 1] = sign
                        if not sign:
                            roots.append(offset)
                        offset += length
                centralizer = (
                    2 ** (len(plus) + len(minus)) * zpart(plus) * zpart(minus)
                )
                yield plus, minus, permutation, flips, roots, order // centralizer


def mask_action(C, permutation, flips):
    bitmap = [
        2 * permutation[box] + (side ^ flips[box])
        for box in range(C)
        for side in range(2)
    ]

    def apply(mask):
        result = 0
        while mask:
            bit = mask & -mask
            mask -= bit
            result |= 1 << bitmap[bit.bit_length() - 1]
        return result

    return apply


def fixed_native(C, graphs, permutation, flips, roots, bound):
    L = C - 2
    dimension = len(roots)
    target = (L,) * dimension
    pairs = list(combinations(range(C), 2))
    pair_action = {
        edge: tuple(sorted((permutation[edge[0]], permutation[edge[1]])))
        for edge in pairs
    }
    seen = set()
    edge_orbits = []
    for edge in pairs:
        if edge in seen:
            continue
        orbit = []
        current = edge
        while current not in seen:
            orbit.append(current)
            seen.add(current)
            current = pair_action[current]
        edge_orbits.append(tuple(orbit))

    apply = mask_action(C, permutation, flips)

    def options(edge_orbit, multiplicity):
        edge = edge_orbit[0]
        orbit_length = len(edge_orbit)
        occupied = [box for box in range(C) if box not in edge]
        masks = [
            sum(1 << (2 * box + side) for box, side in zip(occupied, sides))
            for sides in product(range(2), repeat=L)
        ]
        seen_masks = set()
        word_orbits = []
        for mask in masks:
            if mask in seen_masks:
                continue
            orbit = []
            current = mask
            while current not in seen_masks:
                seen_masks.add(current)
                orbit.append(current)
                for _ in range(orbit_length):
                    current = apply(current)
            if len(orbit) <= multiplicity:
                contribution = [0] * dimension
                for current in orbit:
                    for _ in range(orbit_length):
                        for j, box in enumerate(roots):
                            contribution[j] += (current >> (2 * box + 1)) & 1
                        current = apply(current)
                word_orbits.append((len(orbit), tuple(contribution)))

        result = defaultdict(int)

        def choose(i, remaining, contribution):
            if remaining == 0:
                result[contribution] += 1
                return
            if i == len(word_orbits):
                return
            length, values = word_orbits[i]
            for count in range(remaining // length + 1):
                updated = tuple(x + count * y for x, y in zip(contribution, values))
                if all(x <= L for x in updated):
                    choose(i + 1, remaining - count * length, updated)

        choose(0, multiplicity, (0,) * dimension)
        bound.check_frontier(len(result))
        return result

    option_cache = {}
    answer = 0
    fixed_graphs = 0
    peak = 0
    for edges in graphs:
        bound.check()
        multiplicities = {(i, j): m for i, j, m in edges}
        blocks = []
        for orbit in edge_orbits:
            count = multiplicities.get(orbit[0], 0)
            if any(multiplicities.get(edge, 0) != count for edge in orbit[1:]):
                break
            if count:
                blocks.append((orbit, count))
        else:
            fixed_graphs += 1
            factors = []
            for key in blocks:
                if key not in option_cache:
                    option_cache[key] = options(*key)
                factors.append(option_cache[key])
            if any(not factor for factor in factors):
                continue
            factors.sort(key=len)
            dp = {(0,) * dimension: 1}
            for factor in factors:
                updated = defaultdict(int)
                for index, (values, weight) in enumerate(dp.items()):
                    if index % 128 == 0:
                        bound.check()
                    for other, other_weight in factor.items():
                        combined = tuple(x + y for x, y in zip(values, other))
                        if all(x <= L for x in combined):
                            updated[combined] += weight * other_weight
                dp = updated
                bound.check_frontier(len(dp))
                peak = max(peak, len(dp))
            answer += dp.get(target, 0)
    return answer, fixed_graphs, peak


def direct_c4_checks(bound):
    """Independent small-case referee: enumerate raw masks, without H."""
    masks = sorted(
        (1 << (2 * i + a)) | (1 << (2 * j + b))
        for i, j in combinations(range(4), 2)
        for a, b in product(range(2), repeat=2)
    )
    slots = [[i for i in range(8) if (mask >> i) & 1] for mask in masks]
    degrees = [0] * 8
    state = []
    states = []
    visits = 0

    def choose(first):
        nonlocal visits
        visits += 1
        if visits % 1024 == 0:
            bound.check()
        if len(state) == 8:
            if degrees == [2] * 8:
                states.append(tuple(state))
            return
        for index in range(first, len(masks)):
            if any(degrees[slot] == 2 for slot in slots[index]):
                continue
            for slot in slots[index]:
                degrees[slot] += 1
            state.append(masks[index])
            choose(index)
            state.pop()
            for slot in slots[index]:
                degrees[slot] -= 1

    choose(0)
    if len(states) != 2019:
        raise AssertionError(f"direct C=4 raw state count: {len(states)}")
    result = {}
    for plus, minus, permutation, flips, _, _ in signed_conjugacy(4):
        bound.check()
        apply = mask_action(4, permutation, flips)
        result[(plus, minus)] = sum(
            tuple(sorted(apply(mask) for mask in state)) == state
            for state in states
        )
    return result


def run(args):
    bound = WorkBound(args.time_limit, args.max_states)
    referee = direct_c4_checks(bound) if args.self_test else None
    if referee is not None:
        print("SELFTEST direct_C4_raw=2019 signed_types=20", flush=True)
    for C in range(2, args.max_c + 1):
        started = perf_counter()
        graphs = list(missing_graphs(C))
        raw, identity_terms = raw_native(C, graphs, bound)
        total = class_count = class_weight_sum = peak = checked_terms = 0
        for plus, minus, permutation, flips, roots, weight in signed_conjugacy(C):
            if plus == (1,) * C and not minus:
                value, graph_count, local_peak = raw, len(graphs), 0
            else:
                value, graph_count, local_peak = fixed_native(
                    C, graphs, permutation, flips, roots, bound
                )
            key = (plus, minus)
            if C == 4 and referee is not None and value != referee[key]:
                raise AssertionError(f"direct C=4 fixed-point mismatch: {key}")
            if C == 6 and key in C6_FIXED_CHECKS:
                if value != C6_FIXED_CHECKS[key]:
                    raise AssertionError(f"independent C=6 fixed-point mismatch: {key}")
                checked_terms += 1
            total += weight * value
            class_count += 1
            class_weight_sum += weight
            peak = max(peak, local_peak)
            if args.terms:
                print(
                    f"TERM C={C} plus={plus} minus={minus} class_size={weight} "
                    f"fixed={value} fixed_H={graph_count} peak={local_peak}",
                    flush=True,
                )
        order = 2 ** C * factorial(C)
        if class_weight_sum != order or total % order:
            raise AssertionError("Burnside class weights or integrality mismatch")
        orbits = total // order
        actual = (len(graphs), raw, total, orbits)
        if actual != EXPECTED[C]:
            raise AssertionError(f"C={C} inventory mismatch: {actual}")
        if C == 6 and checked_terms != len(C6_FIXED_CHECKS):
            raise AssertionError("not all independent C=6 fixed-point checks ran")
        print(
            f"COMPLETE C={C} L={C-2} H={len(graphs)} signed_types={class_count} "
            f"identity_terms={identity_terms} raw={raw} numerator={total} "
            f"orbits={orbits} peak_dp={peak} checked_C6_terms={checked_terms} "
            f"seconds={perf_counter()-started:.6f}",
            flush=True,
        )
    print(
        f"PASS max_C={args.max_c} elapsed={perf_counter()-bound.started:.6f}s "
        "no_checkpoint_io=1",
        flush=True,
    )


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--max-c", type=int, choices=range(2, 7), default=6)
    parser.add_argument("--time-limit", type=float, default=180.0, metavar="SECONDS")
    parser.add_argument("--max-states", type=int, default=5000)
    parser.add_argument("--terms", action="store_true", help="print all fixed-point terms")
    parser.add_argument(
        "--self-test", action="store_true", help="independently enumerate C=4 raw masks"
    )
    args = parser.parse_args()
    if not isfinite(args.time_limit) or args.time_limit <= 0:
        parser.error("--time-limit must be finite and positive")
    if args.max_states <= 0:
        parser.error("--max-states must be positive")
    if args.self_test and args.max_c < 4:
        parser.error("--self-test needs --max-c >= 4 for the differential")
    try:
        run(args)
    except (TimeoutError, RuntimeError, AssertionError) as error:
        print(f"STOP: {error}; no partial orbit total is accepted", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
