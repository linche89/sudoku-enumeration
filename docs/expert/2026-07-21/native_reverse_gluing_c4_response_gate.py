#!/usr/bin/env python3
"""Exact native-basis C=4 reverse-gluing gate.

Builds the 26 x 276 2+2 matrix in the native coordinate-orbit convention,
reproduces every per-representative F_4(q), verifies N(4), and certifies the
exact rational rank by full row rank modulo 1,000,000,007.
"""
from __future__ import annotations

from collections import Counter, defaultdict
from fractions import Fraction
from itertools import permutations, product
from math import factorial
from pathlib import Path
import csv
import time

C = 4
N = 2 * C
P = 1_000_000_007
KNOWN_N4 = 29136487207403520
OUT = Path(__file__).resolve().parent


def group_maps() -> list[tuple[int, ...]]:
    out = []
    for p in permutations(range(C)):
        for flips in product((0, 1), repeat=C):
            m = [0] * N
            for i in range(C):
                for side in (0, 1):
                    m[2 * i + side] = 2 * p[i] + (side ^ flips[i])
            out.append(tuple(m))
    return out


def transform_mask(mask: int, m: tuple[int, ...]) -> int:
    out = 0
    while mask:
        bit = mask & -mask
        i = bit.bit_length() - 1
        out |= 1 << m[i]
        mask -= bit
    return out


def transform_config(cfg: tuple[int, ...], m: tuple[int, ...]) -> tuple[int, ...]:
    return tuple(sorted(transform_mask(x, m) for x in cfg))


def canonical_and_stabilizer(cfg: tuple[int, ...], group: list[tuple[int, ...]]) -> tuple[tuple[int, ...], int]:
    best = None
    stabilizer = 0
    for g in group:
        z = transform_config(cfg, g)
        if best is None or z < best:
            best = z
        if z == cfg:
            stabilizer += 1
    assert best is not None
    return best, stabilizer


def generate_two_row_configs() -> set[tuple[int, ...]]:
    configs: set[tuple[int, ...]] = set()
    for p in permutations(range(N)):
        if all(p[i] // 2 != i // 2 for i in range(N)):
            configs.add(tuple(sorted((1 << i) | (1 << p[i]) for i in range(N))))
    return configs


def component_count(cfg: tuple[int, ...]) -> int:
    adj = [[] for _ in range(N)]
    for edge_id, mask in enumerate(cfg):
        vertices = [i for i in range(N) if (mask >> i) & 1]
        assert len(vertices) == 2
        a, b = vertices
        adj[a].append((b, edge_id))
        adj[b].append((a, edge_id))
    seen = [False] * N
    components = 0
    for start in range(N):
        if seen[start]:
            continue
        components += 1
        stack = [start]
        seen[start] = True
        while stack:
            u = stack.pop()
            for v, _ in adj[u]:
                if not seen[v]:
                    seen[v] = True
                    stack.append(v)
    return components


def box_support(mask: int) -> int:
    out = 0
    for i in range(C):
        if mask & (3 << (2 * i)):
            out |= 1 << i
    return out


def enumerate_contingencies(x: tuple[int, ...], y: tuple[int, ...]) -> tuple[dict[tuple[int, ...], int], int]:
    xs = sorted(Counter(x).items())
    ys = sorted(Counter(y).items())
    rem = [count for _, count in ys]
    allowed = [[not (box_support(u) & box_support(v)) for v, _ in ys] for u, _ in xs]
    union_counts: Counter[int] = Counter()
    result: defaultdict[tuple[int, ...], int] = defaultdict(int)
    leaf_count = 0

    def distributions(i: int, j: int, need: int, current: list[int]):
        if j == len(ys):
            if need == 0:
                yield tuple(current)
            return
        maximum = min(need, rem[j]) if allowed[i][j] else 0
        for value in range(maximum + 1):
            current.append(value)
            yield from distributions(i, j + 1, need - value, current)
            current.pop()

    def recurse(i: int, denominator: int) -> None:
        nonlocal leaf_count
        if i == len(xs):
            if any(rem):
                return
            leaf_count += 1
            raw: list[int] = []
            numerator = 1
            for mask, count in union_counts.items():
                raw.extend([mask] * count)
                numerator *= factorial(count)
            result[tuple(sorted(raw))] += numerator // denominator
            return
        u, count = xs[i]
        for dist in distributions(i, 0, count, []):
            changed = []
            factor = 1
            for j, value in enumerate(dist):
                if value:
                    rem[j] -= value
                    w = u | ys[j][0]
                    union_counts[w] += value
                    changed.append((j, w, value))
                    factor *= factorial(value)
            recurse(i + 1, denominator * factor)
            for j, w, value in changed:
                rem[j] += value
                union_counts[w] -= value
                if not union_counts[w]:
                    del union_counts[w]

    recurse(0, 1)
    return dict(result), leaf_count


def modular_rank(matrix: list[list[int]], prime: int) -> tuple[int, list[int]]:
    a = [[v % prime for v in row] for row in matrix]
    rows = len(a)
    cols = len(a[0]) if rows else 0
    rank = 0
    pivots: list[int] = []
    for col in range(cols):
        pivot = next((r for r in range(rank, rows) if a[r][col]), None)
        if pivot is None:
            continue
        a[rank], a[pivot] = a[pivot], a[rank]
        inv = pow(a[rank][col], prime - 2, prime)
        a[rank] = [(v * inv) % prime for v in a[rank]]
        for r in range(rows):
            if r == rank or not a[r][col]:
                continue
            factor = a[r][col]
            a[r] = [(a[r][c] - factor * a[rank][c]) % prime for c in range(cols)]
        pivots.append(col)
        rank += 1
        if rank == rows:
            break
    return rank, pivots


def complete_word(mask: int) -> int:
    word = 0
    for pair in range(C):
        bits = (mask >> (2 * pair)) & 3
        assert bits in (1, 2)
        if bits == 2:
            word |= 1 << pair
    return word


def main() -> None:
    t0 = time.perf_counter()
    group = group_maps()
    assert len(group) == (1 << C) * factorial(C)
    configs = generate_two_row_configs()

    reps: list[tuple[int, ...]] = []
    stabilizers: dict[tuple[int, ...], int] = {}
    for cfg in configs:
        can, stab = canonical_and_stabilizer(cfg, group)
        if can == cfg:
            reps.append(cfg)
            stabilizers[cfg] = stab
    reps.sort()
    assert len(reps) == 23

    images = {r: sorted({transform_config(r, g) for g in group}) for r in reps}
    complete_cache: dict[tuple[int, ...], tuple[tuple[int, ...], int]] = {}
    pair_values: dict[tuple[int, int], dict[tuple[int, ...], int]] = {}
    relative_images = 0
    contingency_leaves = 0

    for i, x in enumerate(reps):
        sx = stabilizers[x]
        for j in range(i, len(reps)):
            y = reps[j]
            sums: defaultdict[tuple[int, ...], int] = defaultdict(int)
            for y_image in images[y]:
                relative_images += 1
                contingency_map, leaves = enumerate_contingencies(x, y_image)
                contingency_leaves += leaves
                for raw, coefficient in contingency_map.items():
                    if raw not in complete_cache:
                        complete_cache[raw] = canonical_and_stabilizer(raw, group)
                    q, _ = complete_cache[raw]
                    sums[q] += coefficient
            values: dict[tuple[int, ...], int] = {}
            for q, raw_sum in sums.items():
                if q not in complete_cache:
                    complete_cache[q] = canonical_and_stabilizer(q, group)
                sq = complete_cache[q][1]
                value = Fraction(sq * raw_sum, sx)  # s_y cancelled by distinct-image enumeration
                assert value.denominator == 1
                values[q] = value.numerator
            pair_values[(i, j)] = values

    q_reps = sorted({can for can, _ in complete_cache.values()})
    assert len(q_reps) == 26
    q_index = {q: i for i, q in enumerate(q_reps)}
    columns = [(i, j) for i in range(len(reps)) for j in range(i, len(reps))]
    assert len(columns) == 276
    matrix = [[0] * len(columns) for _ in q_reps]
    for column, pair in enumerate(columns):
        for q, value in pair_values[pair].items():
            matrix[q_index[q]][column] = value

    f2 = [1 << component_count(r) for r in reps]
    f4: list[int] = []
    for row in range(len(q_reps)):
        total = 0
        for column, (i, j) in enumerate(columns):
            total += (1 if i == j else 2) * f2[i] * f2[j] * matrix[row][column]
        f4.append(total)

    triples = []
    for q, value in zip(q_reps, f4):
        _, sq = canonical_and_stabilizer(q, group)
        m = len(group) // sq
        counts = Counter(q)
        labelled = factorial(N)
        for count in counts.values():
            labelled //= factorial(count)
        words = tuple(sorted(complete_word(mask) for mask in q))
        triples.append((words, m, labelled, value))

    n4 = sum(m * labelled * value * value for _, m, labelled, value in triples)
    assert n4 == KNOWN_N4
    rank, pivots = modular_rank(matrix, P)
    assert rank == 26  # a nonzero 26x26 minor modulo P proves rational rank 26
    assert len(pivots) == 26

    matrix_path = OUT / "native_c4_K4_matrix.csv"
    with matrix_path.open("w", newline="") as f:
        writer = csv.writer(f)
        writer.writerow(["q_index"] + [f"x{i}_y{j}" for i, j in columns])
        for i, row in enumerate(matrix):
            writer.writerow([i] + row)

    triples_path = OUT / "native_c4_complete_triples.csv"
    with triples_path.open("w", newline="") as f:
        writer = csv.writer(f)
        writer.writerow(["q_index", "canonical_side_words", "m_q", "ell_q", "F4"])
        for i, (words, m, labelled, value) in enumerate(triples):
            writer.writerow([i, " ".join(f"{word:0{C}b}" for word in words), m, labelled, value])

    print(f"two_row_orbits={len(reps)}")
    print(f"symmetric_columns={len(columns)}")
    print(f"complete_outputs={len(q_reps)}")
    print(f"matrix_nonzeros={sum(v != 0 for row in matrix for v in row)}")
    print(f"relative_images={relative_images}")
    print(f"contingency_leaves={contingency_leaves}")
    print(f"cold_complete_canonicalizations={len(complete_cache)}")
    print(f"rank_mod_{P}={rank}")
    print(f"rational_rank={rank}")
    print("pivot_columns=" + ",".join(map(str, pivots)))
    print("F4=" + ",".join(map(str, f4)))
    print(f"N4={n4}")
    f4_2a = []
    for row in range(len(q_reps)):
        total = 0
        for column, (i, j) in enumerate(columns):
            total += (1 if i == j else 2) * (2 * f2[i]) * (2 * f2[j]) * matrix[row][column]
        f4_2a.append(total)
    n4_2a = sum(m * labelled * value * value for (_, m, labelled, _), value in zip(triples, f4_2a))
    assert f4_2a == [4 * value for value in f4]
    assert n4_2a == 16 * n4
    print(f"scale_F4_2a_ok={f4_2a == [4 * value for value in f4]}")
    print(f"scale_N4_2a={n4_2a}")
    print(f"matrix_csv={matrix_path}")
    print(f"triples_csv={triples_path}")
    print(f"seconds={time.perf_counter()-t0:.6f}")


if __name__ == "__main__":
    main()
