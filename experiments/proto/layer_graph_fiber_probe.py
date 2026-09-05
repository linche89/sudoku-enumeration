#!/usr/bin/env python3
"""Exact graph fibers on bounded native-state samples; no F/N calculation.

A native state is an L-regular bipartite graph plus an admissible slot
pairing. Pairing endpoints must have disjoint symbol neighborhoods.
Automorphism orbits of those pairings are the exact native fiber over a
bipartition-preserving graph class. Allowing whole-bipartition exchange
adds the symbol-side fiber unless the graph is self-transpose.

For UNIFORM native-orbit samples, E[1/fiber] = graph_classes/native_classes.
This identity does not identify a runtime saving. Graph canonicalization,
value evaluation, and lifting/lookup costs are not measured here.
"""
from __future__ import annotations

import argparse
from collections import Counter
from fractions import Fraction
import hashlib
import json
import math
import os
from pathlib import Path
import statistics
import sys
import threading
import time

import networkx as nx


def matchings(masks: tuple[int, ...]) -> list[tuple[tuple[int, int], ...]]:
    n = len(masks)
    adjacency = [sum(1 << j for j in range(n)
                     if i != j and not (masks[i] & masks[j])) for i in range(n)]

    def visit(remaining, edges):
        if not remaining:
            yield tuple(edges)
            return
        first = (remaining & -remaining).bit_length() - 1
        rest = remaining ^ (1 << first)
        choices = adjacency[first] & rest
        while choices:
            bit = choices & -choices
            choices -= bit
            yield from visit(rest ^ bit, edges + [(first, bit.bit_length() - 1)])

    return list(visit((1 << n) - 1, []))


def pairing_orbits(pairings, automorphisms, offset):
    unseen = set(pairings)
    count = 0
    while unseen:
        pairing = next(iter(unseen))
        count += 1
        for automorphism in automorphisms:
            image = tuple(sorted(tuple(sorted((automorphism[i + offset] - offset,
                                               automorphism[j + offset] - offset)))
                                 for i, j in pairing))
            unseen.discard(image)
    return count


def fiber(masks: tuple[int, ...]) -> dict[str, int | bool]:
    n = len(masks)
    columns = tuple(sum(1 << i for i, mask in enumerate(masks) if mask >> j & 1)
                    for j in range(n))
    graph = nx.Graph()
    graph.add_nodes_from((i, {"side": i // n}) for i in range(2 * n))
    graph.add_edges_from((i, n + j) for i, mask in enumerate(masks)
                         for j in range(n) if mask >> j & 1)
    matcher = nx.algorithms.isomorphism.GraphMatcher(
        graph, graph, node_match=lambda a, b: a["side"] == b["side"])
    automorphisms = []
    for automorphism in matcher.isomorphisms_iter():
        automorphisms.append(automorphism)
        if len(automorphisms) > 100000:
            raise RuntimeError("automorphism bound exceeded; no fiber accepted")
    slot_pairings, symbol_pairings = matchings(columns), matchings(masks)
    b_slot = pairing_orbits(slot_pairings, automorphisms, n)
    b_symbol = pairing_orbits(symbol_pairings, automorphisms, 0)
    if not b_slot:
        raise RuntimeError("valid native graph has no admissible slot pairing")
    self_transpose = False
    if b_symbol:
        self_transpose = nx.algorithms.isomorphism.GraphMatcher(
            graph, graph, node_match=lambda a, b: a["side"] != b["side"]
        ).is_isomorphic()
    if self_transpose and b_slot != b_symbol:
        raise RuntimeError("self-transpose graph has unequal oriented fibers")
    return {
        "automorphisms_preserving_parts": len(automorphisms),
        "slot_pairings": len(slot_pairings),
        "symbol_pairings": len(symbol_pairings),
        "native_fiber_no_transpose": b_slot,
        "native_fiber_symbol_side": b_symbol,
        "self_transpose": self_transpose,
        "native_fiber_with_transpose": b_slot if self_transpose else b_slot + b_symbol,
    }


def parse_rows(path: Path, c: int, layer: int, limit: int):
    data = path.read_bytes()
    if len(data) > 8 << 20:
        raise RuntimeError("input text exceeds 8 MiB bound")
    rows, comments = [], []
    for line in data.decode("ascii").splitlines():
        if line.startswith("#"):
            comments.append(line)
            continue
        tokens = line.split()
        if not tokens:
            continue
        if len(tokens) not in (2 * c, 2 * c + 1):
            raise RuntimeError("input row needs exactly 2C masks and optional expected=F")
        if len(tokens) > 2 * c and not tokens[-1].startswith("expected="):
            raise RuntimeError("unexpected input suffix")
        masks = tuple(sorted(int(token) for token in tokens[:2 * c]))
        for mask in masks:
            if mask < 0 or mask >> (2 * c) or mask.bit_count() != layer:
                raise RuntimeError("invalid mask degree/range")
            if any((mask >> (2 * b)) & 3 == 3 for b in range(c)):
                raise RuntimeError("mask occupies both sides of a box")
        if any(sum(mask >> j & 1 for mask in masks) != layer for j in range(2 * c)):
            raise RuntimeError("unbalanced slot degree")
        rows.append(masks)
        if len(rows) == limit:
            break
    if not rows:
        raise RuntimeError("empty input")
    return data, comments, rows


def summarize(values: list[int]):
    reciprocal = [1.0 / value for value in values]
    mean = statistics.mean(reciprocal)
    se = statistics.stdev(reciprocal) / math.sqrt(len(values)) if len(values) > 1 else None
    interval = None
    if se is not None and mean > 1.96 * se:
        interval = [1.0 / min(1.0, mean + 1.96 * se), 1.0 / (mean - 1.96 * se)]
    return {
        "mean_fiber": statistics.mean(values),
        "mean_reciprocal_fiber": mean,
        "standard_error_reciprocal": se,
        "inverse_mean_reciprocal": 1.0 / mean,
        "inverse_normal_approx_95pct_interval": interval,
        "fiber_histogram": dict(sorted(Counter(values).items())),
    }


def self_test(path: Path):
    assert fiber((1, 2, 4, 8))["native_fiber_with_transpose"] == 1
    assert fiber((5, 5, 10, 10))["native_fiber_with_transpose"] == 1
    assert fiber((5, 6, 9, 10))["native_fiber_with_transpose"] == 1
    _, _, rows = parse_rows(path, 4, 3, 1024)
    if len(rows) != 54 or len(set(rows)) != 54:
        raise RuntimeError("self-test needs the complete 54-state C4,L3 native fixture")
    results = [fiber(row) for row in rows]
    graphs = sum((Fraction(1, result["native_fiber_with_transpose"])
                  for result in results), Fraction())
    oriented = sum((Fraction(1, result["native_fiber_no_transpose"])
                    for result in results), Fraction())
    if graphs != 33 or oriented.denominator != 1:
        raise RuntimeError("C4 complete graph-class count differential failed")
    print(json.dumps({"self_test": "passed", "native_states": 54,
                      "graph_classes_no_transpose": int(oriented),
                      "graph_classes_with_transpose": int(graphs)}), flush=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("input", type=Path)
    parser.add_argument("--c", type=int, default=6)
    parser.add_argument("--layer", type=int, default=4)
    parser.add_argument("--limit", type=int, required=True)
    parser.add_argument("--max-seconds", type=float, required=True)
    parser.add_argument("--uniform-orbit-sample", action="store_true")
    parser.add_argument("--self-test-c4", type=Path)
    args = parser.parse_args()
    if not 2 <= args.c <= 6 or not 1 <= args.layer <= args.c or not 1 <= args.limit <= 1024:
        parser.error("require C=2..6, L=1..C, sample limit=1..1024")
    if not 0 < args.max_seconds <= 180:
        parser.error("require max-seconds in (0,180]")
    started = time.monotonic()
    stopped = threading.Event()

    def watch():
        while not stopped.wait(0.1):
            if time.monotonic() - started > args.max_seconds:
                print("BOUND time exceeded; no complete sample estimate accepted", file=sys.stderr, flush=True)
                os._exit(124)

    worker = threading.Thread(target=watch, daemon=True)
    worker.start()
    try:
        if args.self_test_c4:
            self_test(args.self_test_c4)
        data, comments, rows = parse_rows(args.input, args.c, args.layer, args.limit)
        if args.uniform_orbit_sample and not any(
                "sampling=uniform-record-with-replacement-reject-holes" in line for line in comments):
            raise RuntimeError("uniform sample requires the audited sampler provenance comment")
        results = []
        for i, row in enumerate(rows):
            result = fiber(row)
            results.append(result)
            print(json.dumps({"sample": i + 1, **result}), flush=True)
        result = {
            "complete": True, "c": args.c, "layer": args.layer, "samples": len(results),
            "networkx_version": nx.__version__, "python_version": sys.version.split()[0],
            "input_sha256": hashlib.sha256(data).hexdigest().upper(),
            "provenance": comments,
            "sampling_assumption": "audited uniform native-orbit record sample" if args.uniform_orbit_sample
                                   else "unspecified; summaries are descriptive only",
            "no_transpose": summarize([r["native_fiber_no_transpose"] for r in results]),
            "with_transpose": summarize([r["native_fiber_with_transpose"] for r in results]),
            "automorphism_histogram": dict(sorted(Counter(
                r["automorphisms_preserving_parts"] for r in results).items())),
            "self_transpose_graphs": sum(r["self_transpose"] for r in results),
            "seconds": time.monotonic() - started,
            "performance_claim": "none; graph classification, F4 and native lifting costs not measured",
        }
        if args.c == 6 and args.layer == 4 and args.uniform_orbit_sample:
            result["one_missing_orbit_reciprocal_mean_bias_bound"] = 1 / 903398603
        print("SUMMARY " + json.dumps(result, sort_keys=True), flush=True)
    finally:
        stopped.set()
        worker.join()


if __name__ == "__main__":
    main()
