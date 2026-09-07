"""Small read-only research certificates; no production catalogue access.

Run under scripts/run_guarded_step.py --seconds 30 --gib 1.
All counting arithmetic is integer. Larger-C work is formula evaluation only.
"""
from collections import Counter
from functools import lru_cache
from itertools import combinations_with_replacement, permutations, product
from math import comb, factorial
from pathlib import Path
import csv
import hashlib
import time

START = time.monotonic()
VISITS = 0


def tick():
    global VISITS
    VISITS += 1
    if VISITS % 1024 == 0 and time.monotonic() - START > 25:
        raise RuntimeError("25-second cooperative bound; no accepted partial result")


def partitions(n, least=1):
    if n == 0:
        yield ()
    for p in range(least, n + 1):
        for tail in partitions(n - p, p):
            yield (p,) + tail


def coefficient(parts, degree):
    a = [1] + [0] * degree
    for d in parts:
        for j in range(degree, d - 1, -1):
            a[j] += a[j - d]
    return a[degree]


def complete_raw(c):
    numerator = terms = 0
    group = factorial(2 * c)
    for parts in partitions(2 * c):
        tick()
        z = 1
        for d, multiplicity in Counter(parts).items():
            z *= d ** multiplicity * factorial(multiplicity)
        numerator += (group // z) * coefficient(parts, c) ** c
        terms += 1
    assert numerator % group == 0
    return numerator // group, terms


def penultimate_raw(c):
    f = [coefficient((1,) * (2 * (c - 1 - t)) + (2,) * t, c - 1)
         for t in range(c)]
    numerator = 0
    for k in range(c + 1):
        value = comb(c, k)
        if k:
            value *= f[k - 1] ** k
        if k < c:
            value *= f[k] ** (c - k)
        numerator += value
    assert numerator % (1 << c) == 0
    return numerator // (1 << c)


def brute_complete_raw(c):
    count = 0
    for words in combinations_with_replacement(range(1 << c), 2 * c):
        tick()
        if all(sum((v >> b) & 1 for v in words) == c for b in range(c)):
            count += 1
    return count


def brute_penultimate_raw(c):
    inventories = []
    for missing in range(c):
        words = [v for v in range(1 << c) if not (v >> missing) & 1]
        inventories.append([tuple(0 if b == missing else ((u >> b) & 1) + ((v >> b) & 1)
                                  for b in range(c))
                            for u, v in combinations_with_replacement(words, 2)])

    def dfs(i, degrees):
        tick()
        if i == c:
            return int(all(d == c - 1 for d in degrees))
        value = 0
        for contribution in inventories[i]:
            child = tuple(a + b for a, b in zip(degrees, contribution))
            if max(child) <= c - 1:
                value += dfs(i + 1, child)
        return value

    return dfs(0, (0,) * c)


def reduced_latin(c):
    row_options = [[p for p in permutations(range(c)) if p[0] == r] for r in range(c)]

    def dfs(row, columns):
        tick()
        if row == c:
            return 1
        value = 0
        for p in row_options[row]:
            if all(not ((columns[j] >> p[j]) & 1) for j in range(c)):
                value += dfs(row + 1, tuple(columns[j] | (1 << p[j]) for j in range(c)))
        return value

    return dfs(1, tuple(1 << j for j in range(c)))


def family_words(c, t):
    full = (1 << c) - 1
    return tuple(sorted([0] * (c - t) + [full] * (c - t) + [1] * t + [full ^ 1] * t))


def native_incidence(words, c):
    return tuple(sum(1 << (2 * b + ((word >> b) & 1)) for b in range(c)) for word in words)


@lru_cache(maxsize=200000)
def factor(rows):
    tick()
    if not rows:
        return 1
    d = rows[0].bit_count()
    assert all(row.bit_count() == d for row in rows)
    if d <= 1:
        return 1
    if d == 2:
        n = len(rows)
        columns = [[] for _ in range(n)]
        for i, row in enumerate(rows):
            for j in range(n):
                if (row >> j) & 1:
                    columns[j].append(i)
        seen = set()
        components = 0
        for i in range(n):
            if i in seen:
                continue
            components += 1
            stack = [i]
            seen.add(i)
            while stack:
                u = stack.pop()
                for j in range(n):
                    if (rows[u] >> j) & 1:
                        for v in columns[j]:
                            if v not in seen:
                                seen.add(v)
                                stack.append(v)
        return 1 << components
    first = rows[0] & -rows[0]
    child = [rows[0] ^ first]

    def dfs(i, used):
        tick()
        if i == len(rows):
            return factor(tuple(sorted(child)))
        value = 0
        choices = rows[i] & ~used
        while choices:
            bit = choices & -choices
            choices ^= bit
            child.append(rows[i] ^ bit)
            value += dfs(i + 1, used | bit)
            child.pop()
        return value

    return d * dfs(1, first)


def coordinate_orbit_size(words, c):
    images = set()
    for p in permutations(range(c)):
        transformed = [sum(((word >> b) & 1) << p[b] for b in range(c)) for word in words]
        for flip in range(1 << c):
            images.add(tuple(sorted(word ^ flip for word in transformed)))
    return len(images)


def complete_c5_reference_check(latin):
    path = Path('docs/expert/2026-07-26/layer_dp_c5_classes.csv')
    raw = path.read_bytes()
    assert len(raw) < 1000000
    records = list(csv.DictReader(raw.decode('utf-8-sig').splitlines()))
    assert len(records) == 355
    assert sum(int(r['coordinate_orbit_size']) * int(r['labelled_multiplicity']) * int(r['F']) ** 2
               for r in records) == 1903816047972624930994913280000
    matched = {}
    for r in records:
        words = tuple(map(int, r['representative_words'].split()))
        hist = Counter(words)
        if len(hist) == 2 and set(hist.values()) == {5} and all(hist[w ^ 31] == hist[w] for w in hist):
            t = 0
        elif len(hist) == 4 and all(hist[w ^ 31] == hist[w] for w in hist):
            t = min(hist.values())
            pairs = [(u, v) for u in hist for v in hist if u != v and u != (v ^ 31)]
            if not any((u ^ v).bit_count() in (1, 4) for u, v in pairs):
                continue
        else:
            continue
        assert t not in matched
        expected = latin ** 2 // comb(5, t)
        assert int(r['F']) == expected
        orbit = coordinate_orbit_size(words, 5)
        assert int(r['coordinate_orbit_size']) == orbit == (16 if t == 0 else 80)
        ell = factorial(10) // (factorial(5 - t) ** 2 * factorial(t) ** 2)
        assert int(r['labelled_multiplicity']) == ell
        matched[t] = (expected, orbit, ell)
    assert set(matched) == {0, 1, 2}
    print('C5_REFERENCE_FAMILY', sorted(matched.items()), 'sha256=' + hashlib.sha256(raw).hexdigest().upper())
    terminal_pairings = antipodal_states = 0
    for r in records:
        words = tuple(map(int, r['representative_words'].split()))
        hist = Counter(words)
        antipodal_states += int(all(hist[word ^ 31] == multiplicity for word, multiplicity in hist.items()))
        rows = native_incidence(words, 5)
        neighborhoods = [sum(1 << i for i, row in enumerate(rows) if (row >> j) & 1) for j in range(10)]
        types = Counter(neighborhoods)
        assert all(types[v] == types[v ^ 1023] for v in types)
        expected_pairings = 1
        for v, multiplicity in types.items():
            if v < (v ^ 1023):
                expected_pairings *= factorial(multiplicity)
        original_pairs = {}
        for u in range(0, 10, 2):
            pair = (u, u + 1) if neighborhoods[u] < neighborhoods[u + 1] else (u + 1, u)
            original_pairs.setdefault(neighborhoods[pair[0]], []).append(pair)
        found = 0

        def scan_pairings(remaining, pairs):
            nonlocal found
            if not remaining:
                used = Counter()
                mapping = [-1] * 10
                for u, v in pairs:
                    if neighborhoods[u] > neighborhoods[v]:
                        u, v = v, u
                    kind = neighborhoods[u]
                    a, b = original_pairs[kind][used[kind]]
                    used[kind] += 1
                    mapping[u], mapping[v] = a, b
                assert sorted(mapping) == list(range(10))
                assert all(neighborhoods[j] == neighborhoods[mapping[j]] for j in range(10))
                assert {tuple(sorted((mapping[u], mapping[v]))) for u, v in pairs} == {(u, u + 1) for u in range(0, 10, 2)}
                found += 1
                return
            u = remaining[0]
            for v in remaining[1:]:
                if not (neighborhoods[u] & neighborhoods[v]):
                    scan_pairings(tuple(j for j in remaining[1:] if j != v), pairs + ((u, v),))

        scan_pairings(tuple(range(10)), ())
        assert found == expected_pairings
        terminal_pairings += found
    print('TERMINAL_PAIRING_TRANSPORTERS states=%d admissible_pairings=%d antipodal_states=%d PASS' % (len(records), terminal_pairings, antipodal_states))


def circulant_checks():
    cases = subsets = 0
    for c in range(3, 6):
        n = 2 * c
        for d in range(3, c + 1):
            rows = [sum(1 << ((i + j) % n) for j in range(d)) for i in range(n)]
            columns = [sum(1 << i for i in range(n) if (rows[i] >> j) & 1) for j in range(n)]
            assert len(set(rows)) == len(set(columns)) == n
            assert all(not (columns[j] & columns[j + c]) for j in range(c))
            assert all(rows[i] & rows[(i + 1) % n] for i in range(n))
            for mask in range(1, 1 << n):
                if mask.bit_count() >= n - 1:
                    continue
                neighbor = 0
                for j in range(n):
                    if (mask >> j) & 1:
                        neighbor |= columns[j]
                assert neighbor.bit_count() >= mask.bit_count() + 2
                subsets += 1
            cases += 1
    print('CIRCULANT_CORE_CHECK cases=%d right_subsets=%d PASS' % (cases, subsets))


def main():
    for c in range(2, 5):
        direct_complete = brute_complete_raw(c)
        direct_penultimate = brute_penultimate_raw(c)
        assert direct_complete == complete_raw(c)[0]
        assert direct_penultimate == penultimate_raw(c)
        print('RAW_DIFFERENTIAL', c, direct_complete, direct_penultimate, 'PASS')
    assert penultimate_raw(5) == 62185328
    assert penultimate_raw(6) == 4439972139072
    print('C,complete_raw,outer_orbit_lower,penultimate_raw,penultimate_orbit_lower,penultimate_8byte_lower,partition_terms')
    for c in range(2, 10):
        a, terms = complete_raw(c)
        p = penultimate_raw(c)
        g = (1 << c) * factorial(c)
        lower_outer = (a + g - 1) // g
        lower_pen = (p + g - 1) // g
        print(c, a, lower_outer, p, lower_pen, 8 * lower_pen, terms, sep=',')
    for c in range(2, 6):
        reduced = reduced_latin(c)
        latin = reduced * factorial(c) * factorial(c - 1)
        assert latin == {2: 2, 3: 12, 4: 576, 5: 161280}[c]
        for t in range(c // 2 + 1):
            words = family_words(c, t)
            expected = latin ** 2 // comb(c, t)
            measured_orbit = coordinate_orbit_size(words, c)
            expected_orbit = ((1 << (c - 1)) if t == 0 else
                              (1 if c == 2 else c * (1 << (c - 2 if 2 * t == c else c - 1))))
            assert measured_orbit == expected_orbit
            if c <= 4:
                actual = factor(tuple(sorted(native_incidence(words, c))))
                assert actual == expected
                print('FAMILY_GRAPH_CHECK', c, t, actual, measured_orbit, 'PASS')
        print('LATIN_REDUCED_AND_TOTAL', c, reduced, latin)
        if c == 5:
            complete_c5_reference_check(latin)
    for d in range(2, 5):
        # Splice two K_(d,d) after deleting a right vertex in A and a left
        # vertex in B. Join the d exposed left/right endpoints in order.
        rows = [(1 << (d - 1)) - 1 | (1 << (d - 1 + i)) for i in range(d)]
        rows += [((1 << d) - 1) << (d - 1)] * (d - 1)
        lhs = factor(tuple(sorted(rows)))
        latin = reduced_latin(d) * factorial(d) * factorial(d - 1)
        rhs = latin ** 2 // factorial(d)
        assert lhs == rhs
        print('TIGHT_CUT_CHECK', d, lhs, 'PASS')
    circulant_checks()
    print('FACTOR_MEMO', factor.cache_info())
    print('PASS elapsed_seconds=%.6f visits=%d no_production_input=1' % (time.monotonic() - START, VISITS))


if __name__ == '__main__':
    main()

