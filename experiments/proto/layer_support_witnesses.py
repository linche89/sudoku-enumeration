#!/usr/bin/env python3
"""Construct small C=6 support witnesses; never read or write checkpoints.

Print the actual layer-4 witness, exact raw layer-5 mass, both layer-5
witnesses, and the side-separated layer-4 sector's five earlier candidates.
Membership in a particular saved catalogue requires a separate audit.
"""

import argparse
from collections import Counter
from itertools import permutations
import json
from math import comb, isfinite
from pathlib import Path
from time import perf_counter


GROUP_ORDER = 46080


def penultimate_raw(C):
    """Identity fixed points of the unordered two-word missing-box groups."""
    target = C - 1

    def coefficient(t):
        return sum(
            comb(t, j) * comb(2 * (target - t), target - 2 * j)
            for j in range(t + 1)
            if 0 <= target - 2 * j <= 2 * (target - t)
        )

    coefficients = [coefficient(t) for t in range(C)]
    numerator = sum(
        comb(C, k)
        * (coefficients[k - 1] ** k if k else 1)
        * (coefficients[k] ** (C - k) if k < C else 1)
        for k in range(C + 1)
    )
    if numerator % (1 << C):
        raise AssertionError("nonintegral penultimate raw count")
    return numerator // (1 << C), coefficients, numerator


def l5_disconnected():
    """Used graph: two disjoint copies of K_(6,6) minus a perfect matching."""
    return tuple(sorted(
        sum(1 << (2 * box + side) for box in range(6) if box != missing)
        for missing in range(6)
        for side in range(2)
    ))


def l5_conference():
    """Six complementary word pairs from a directly checked 6x6 matrix."""
    matrix = [[0] * 6 for _ in range(6)]
    for i in range(1, 6):
        matrix[0][i] = matrix[i][0] = 1
    for i in range(1, 6):
        for j in range(1, 6):
            if i != j:
                matrix[i][j] = 1 if (i - j) % 5 in (1, 4) else -1
    if any(
        sum(matrix[i][k] * matrix[k][j] for k in range(6)) != 5 * (i == j)
        for i in range(6)
        for j in range(6)
    ):
        raise AssertionError("conference matrix square is not 5I")
    masks = tuple(sorted(
        sum(
            1 << (2 * box + int(matrix[missing][box] * sign == 1))
            for box in range(6) if box != missing
        )
        for missing in range(6)
        for sign in (1, -1)
    ))
    return masks, matrix


def l4_three_k44():
    """A separately audited PRESENT state, not the missing L4 witness."""
    return tuple(sorted((170, 1360, 2565) * 4))


def l4_missing():
    """The unique absent orbit located by the exhaustive order-three audit."""
    return (294, 294, 554, 554, 1161, 1161, 1360, 1360, 2181, 2181, 2640, 2640)


def validate_state(masks):
    if len(masks) != 12:
        raise AssertionError("a C=6 state needs twelve occurrence masks")
    L = masks[0].bit_count()
    for mask in masks:
        if mask < 0 or mask >= 4096 or mask.bit_count() != L:
            raise AssertionError("mask size or range mismatch")
        if any(((mask >> (2 * box)) & 3) == 3 for box in range(6)):
            raise AssertionError("a mask occupies both sides of a box")
    if any(sum((mask >> slot) & 1 for mask in masks) != L for slot in range(12)):
        raise AssertionError("slot degree mismatch")
    return L


def coordinate_images(masks, check=lambda: None):
    """Every group image, including repeats; full lex order is not native canon."""
    flip_bits = [
        sum(3 << (2 * box) for box in range(6) if (flips >> box) & 1)
        for flips in range(64)
    ]
    for permutation in permutations(range(6)):
        check()
        moved = [
            sum(
                1 << (2 * permutation[box] + side)
                for box in range(6)
                for side in range(2)
                if (mask >> (2 * box + side)) & 1
            )
            for mask in masks
        ]
        swapped = [((mask & 0x555) << 1) | ((mask & 0xAAA) >> 1) for mask in moved]
        for flips in flip_bits:
            yield tuple(sorted(
                mask ^ ((mask ^ other) & flips)
                for mask, other in zip(moved, swapped)
            ))


def witness_info(name, masks, expected_stabilizer, check):
    layer = validate_state(masks)
    images = set(coordinate_images(masks, check))
    if GROUP_ORDER % len(images):
        raise AssertionError("orbit size does not divide the coordinate group")
    stabilizer = GROUP_ORDER // len(images)
    if stabilizer != expected_stabilizer:
        raise AssertionError(f"{name}: unexpected stabilizer {stabilizer}")
    return {
        "name": name,
        "layer": layer,
        "masks": masks,
        "full_lex_representative": min(images),
        "orbit_size": len(images),
        "stabilizer": stabilizer,
    }


def two_regular_graphs():
    """The 130 labelled loopless 2-regular multigraphs on six vertices."""
    degrees = [2] * 6
    edges = []

    def vertex(i):
        if i == 5:
            if degrees[i] == 0:
                yield tuple(edges)
            return

        def row(j, remaining):
            if j == 6:
                if remaining == 0:
                    yield from vertex(i + 1)
                return
            for count in range(min(remaining, degrees[j]) + 1):
                degrees[j] -= count
                edges.extend([(i, j)] * count)
                yield from row(j + 1, remaining - count)
                if count:
                    del edges[-count:]
                degrees[j] += count

        yield from row(i + 1, degrees[i])

    yield from vertex(0)


def side_separated_l4(check=lambda: None):
    """Complete two-(6+6)-component sector, with its five stabilizer-12 keys."""
    graphs = list(two_regular_graphs())
    if len(graphs) != 130:
        raise AssertionError("two-regular missing graph inventory mismatch")
    index = {graph: i for i, graph in enumerate(graphs)}
    transforms = []
    for permutation in permutations(range(6)):
        check()
        transforms.append([
            index[tuple(sorted(
                tuple(sorted((permutation[a], permutation[b])))
                for a, b in graph
            ))]
            for graph in graphs
        ])
    remaining = {(i, j) for i in range(130) for j in range(i, 130)}
    histogram = Counter()
    candidates = []
    while remaining:
        check()
        i, j = min(remaining)
        images = {tuple(sorted((transform[i], transform[j]))) for transform in transforms}
        remaining.difference_update(images)
        stabilizer = sum(
            int(transform[i] == i and transform[j] == j)
            + int(transform[i] == j and transform[j] == i)
            for transform in transforms
        )
        histogram[stabilizer] += 1
        if stabilizer == 12:
            masks = tuple(sorted(
                sum(1 << (2 * box + side) for box in range(6) if box not in edge)
                for side, graph in ((0, graphs[i]), (1, graphs[j]))
                for edge in graph
            ))
            candidates.append(masks)
    if sum(histogram.values()) != 47 or len(candidates) != 5:
        raise AssertionError("side-separated native sector inventory mismatch")
    return dict(sorted(histogram.items())), candidates


def check_fixture(path, witnesses):
    """Check witness geometry and support arithmetic, not source-file hashes."""
    with path.open(encoding="utf-8") as stream:
        fixture = json.load(stream)
    if fixture["schema_version"] != 1 or fixture["C"] != 6:
        raise AssertionError("unsupported support fixture schema")
    if fixture["coordinate_group_order"] != GROUP_ORDER:
        raise AssertionError("fixture coordinate group mismatch")
    expected = {witness["name"]: witness for witness in witnesses}
    if len(fixture["witnesses"]) != len(expected):
        raise AssertionError("fixture witness count mismatch")
    seen = set()
    additions = Counter()
    masses = Counter()
    for record in fixture["witnesses"]:
        name = record["id"]
        if name in seen or name not in expected:
            raise AssertionError("duplicate or unknown fixture witness")
        seen.add(name)
        actual = expected[name]
        if (
            record["C"] != 6
            or record["L"] != actual["layer"]
            or tuple(record["slot_masks"]) != actual["masks"]
            or record["stabilizer"] != actual["stabilizer"]
            or record["orbit_mass"] != actual["orbit_size"]
        ):
            raise AssertionError(f"fixture witness mismatch: {name}")
        source = record["source_id"]
        additions[source] += 1
        masses[source] += record["orbit_mass"]
    for source in fixture["sources"]:
        name = source["id"]
        if source["real_states"] + additions[name] != source["complete_support_states"]:
            raise AssertionError(f"fixture support count does not close: {name}")
        if source["raw_orbit_mass"] + masses[name] != source["complete_raw_orbit_mass"]:
            raise AssertionError(f"fixture orbit mass does not close: {name}")
        if source["weights_closed"] or source["semantic_full_key_audit"]:
            raise AssertionError("fixture overclaims the source audit")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--time-limit", type=float, default=30.0)
    parser.add_argument("--fixture", type=Path, help="verify a retained witness JSON fixture")
    args = parser.parse_args()
    if not isfinite(args.time_limit) or args.time_limit <= 0:
        parser.error("--time-limit must be finite and positive")
    started = perf_counter()

    def check():
        if perf_counter() - started > args.time_limit:
            raise TimeoutError("witness generation exceeded its time limit")

    raw, coefficients, numerator = penultimate_raw(6)
    if raw != 4439972139072:
        raise AssertionError("complete raw L5 mass mismatch")
    first = witness_info("l5_disconnected", l5_disconnected(), 1440, check)
    conference, matrix = l5_conference()
    second = witness_info("l5_conference", conference, 240, check)
    if first["orbit_size"] + second["orbit_size"] != 224:
        raise AssertionError("L5 witness orbit mass mismatch")
    missing = witness_info("l4_missing", l4_missing(), 12, check)
    if missing["orbit_size"] != 3840:
        raise AssertionError("L4 witness orbit mass mismatch")
    if args.fixture is not None:
        check_fixture(args.fixture, [missing, first, second])
    histogram, masks = side_separated_l4(check)
    candidates = [
        witness_info(f"l4_side_separated_{i}", state, 12, check)
        for i, state in enumerate(masks, 1)
    ]
    result = {
        "l4_missing_witness": missing,
        "l5_raw_mass": raw,
        "l5_identity_coefficients": coefficients,
        "l5_identity_numerator_before_64": numerator,
        "l5_witnesses": [first, second],
        "conference_matrix": matrix,
        "l4_sector_orbits": 47,
        "l4_sector_stabilizer_histogram": histogram,
        "l4_stabilizer_12_candidates": candidates,
        "elapsed_seconds": perf_counter() - started,
        "checkpoint_io": False,
        "membership_tested_by_this_script": False,
        "fixture_verified": args.fixture is not None,
    }
    print(json.dumps(result, indent=2))


if __name__ == "__main__":
    main()
