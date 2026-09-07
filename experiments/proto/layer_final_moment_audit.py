"""Read-only cross-checks against independently constructed moment/Latin oracles.

This supplements, and does NOT replace, s4_certificate_verify.py.  It does not
certify every F value or call the production counting/canonicalization code.
Run with scripts/run_guarded_step.py and explicit time/memory bounds.
"""
from collections import Counter
import argparse
import csv
import hashlib
import io
import json
from math import comb, factorial
from pathlib import Path
import re


HEADER = ['qid', 'representative_words', 'coordinate_orbit_size',
          'labelled_multiplicity', 'F']


def require(condition, message):
    if not condition:
        raise ValueError(message)


def family_type(words, c):
    """Recognize the one-mixing-coordinate family, invariantly, without canon."""
    hist = Counter(words)
    full = (1 << c) - 1
    if any(hist[w ^ full] != count for w, count in hist.items()):
        return None
    if len(hist) == 2 and set(hist.values()) == {c}:
        return 0
    if len(hist) != 4:
        return None
    t = min(hist.values())
    if sorted(hist.values()) != sorted([t, t, c - t, c - t]):
        return None
    if any((u ^ v).bit_count() in (1, c - 1)
           for u in hist for v in hist if u != v and u != (v ^ full)):
        return t
    return None


def family_orbit(c, t):
    if t == 0:
        return 1 << (c - 1)
    if c == 2:
        return 1
    return c * (1 << (c - 2 if 2 * t == c else c - 1))


def audit_records(handle, c, classes, linear, latin):
    reader = csv.reader(handle, strict=True)
    require(next(reader, None) == HEADER, 'unexpected CSV header')
    count = 0
    moments = [0, 0, 0]
    family = {}
    for record in reader:
        require(len(record) == 5, 'expected five nonempty CSV fields')
        qid, m, ell, value = map(int, (record[0], record[2], record[3], record[4]))
        words = tuple(map(int, record[1].split()))
        require(qid == count, 'noncontiguous qid')
        require(len(words) == 2 * c and words == tuple(sorted(words)),
                'invalid representative width/order')
        require(all(0 <= w < (1 << c) for w in words), 'word out of range')
        require(all(sum((w >> b) & 1 for w in words) == c for b in range(c)),
                'unbalanced representative')
        require(m > 0 and ell > 0 and value > 0, 'nonpositive coefficient')
        require(value % factorial(c) == 0, 'F is not divisible by C!')
        weight = m * ell
        moments[0] += weight
        moments[1] += weight * value
        moments[2] += weight * value * value
        t = family_type(words, c)
        if t is not None:
            require(t not in family, 'duplicate Latin-family class')
            expected, remainder = divmod(latin * latin, comb(c, t))
            require(remainder == 0, 'nonintegral Latin-family oracle')
            require(value == expected, 'Latin-family F mismatch')
            require(m == family_orbit(c, t), 'Latin-family orbit mismatch')
            expected_ell = factorial(2 * c) // (factorial(t)**2 * factorial(c-t)**2)
            require(ell == expected_ell, 'Latin-family labelled multiplicity mismatch')
            family[t] = dict(t=t, qid=qid, words=words, m=m, ell=ell, F=value)
        count += 1
    require(count == classes, 'class count mismatch')
    require(moments[0] == comb(2 * c, c)**c, 'zero-moment mass mismatch')
    require(moments[1] == linear, 'independent first-moment mismatch')
    require(set(family) == set(range(c // 2 + 1)), 'missing Latin-family class')
    return dict(status='PASS', C=c, classes=count, moments_0_1_2=moments,
                positive_F_divisible_by_factorial_C=count,
                latin_family=[family[t] for t in sorted(family)],
                scope='additional moments and Latin anchors; not full numerical F recomputation')


def one(pattern, text):
    matches = re.findall(pattern, text, re.MULTILINE)
    require(len(matches) == 1, 'oracle field must occur exactly once: ' + pattern)
    return matches[0]


def read_oracles(c, linear_path, latin_path):
    linear_raw = linear_path.read_bytes()
    latin_raw = latin_path.read_bytes()
    a = linear_raw.decode('ascii')
    b = latin_raw.decode('ascii')
    require(int(one(r'^C=(\d+)\s*$', a)) == c, 'wrong linear-oracle C')
    arrays = int(one(r'^A_C=(\d+)\s*$', a))
    linear = int(one(r'^N_linear_C=2\^\(C\^2\)\*A_C=(\d+)\s*$', a))
    require(linear == arrays * (1 << (c*c)), 'linear-oracle orientation factor mismatch')
    if c == 6:
        reduced = int(one(r'^REDUCED_LATIN_ORDER6 (\d+)\s*$', b))
        latin = int(one(r'^LABELLED_LATIN_ORDER6 (\d+)\s*$', b))
        multiplier = int(one(r'^LABELLED_MULTIPLIER (\d+) = 6! \* 5!\s*$', b))
        require(multiplier == factorial(c)*factorial(c-1), 'Latin multiplier mismatch')
    else:
        reduced, latin = map(int, one(r'^LATIN_REDUCED_AND_TOTAL %d (\d+) (\d+)\s*$' % c, b))
    require(latin == reduced * factorial(c) * factorial(c-1), 'Latin normalization mismatch')
    provenance = dict(
        linear_log=str(linear_path.resolve()),
        linear_log_sha256=hashlib.sha256(linear_raw).hexdigest().upper(),
        latin_log=str(latin_path.resolve()),
        latin_log_sha256=hashlib.sha256(latin_raw).hexdigest().upper(),
        permutation_arrays=arrays, independent_linear=linear,
        reduced_latin=reduced, labelled_latin=latin)
    return linear, latin, provenance


def self_test():
    def fixture(rows, header=HEADER):
        handle = io.StringIO(newline='')
        writer = csv.writer(handle)
        writer.writerow(header)
        writer.writerows(rows)
        handle.seek(0)
        return handle

    good = [[0, '0 0 3 3', 2, 6, 4], [1, '0 1 2 3', 1, 24, 2]]
    result = audit_records(fixture(good), 2, 2, 96, 2)
    require(result['moments_0_1_2'] == [36, 96, 288], 'C2 fixture failed')
    bad = []
    for col, value in [(0, 7), (1, '0 0 3 4'), (2, 3), (3, 7), (4, 0), (4, 3), (4, 6)]:
        changed = [list(row) for row in good]
        changed[0][col] = value
        bad.append((changed, HEADER, 2, 96, 2))
    bad.extend([(good[:1], HEADER, 2, 96, 2), (good, HEADER[:-1], 2, 96, 2),
                (good, HEADER, 3, 96, 2), (good, HEADER, 2, 97, 2),
                (good, HEADER, 2, 96, 4)])
    rejected = 0
    for rows, header, classes, linear, latin in bad:
        try:
            audit_records(fixture(rows, header), 2, classes, linear, latin)
        except (ValueError, csv.Error):
            rejected += 1
        else:
            raise AssertionError('accepted a corrupted fixture')
    print('MOMENT AUDIT SELF-TEST PASS accepted=1 rejected=%d' % rejected)


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--self-test', action='store_true')
    p.add_argument('--c', type=int, choices=range(2, 7))
    p.add_argument('--classes', type=int)
    p.add_argument('--linear-log', type=Path)
    p.add_argument('--latin-log', type=Path)
    p.add_argument('csv', nargs='*', type=Path)
    args = p.parse_args()
    if args.self_test:
        require(not args.csv, 'self-test takes no CSV')
        self_test()
        return
    require(args.c and args.classes and args.linear_log and args.latin_log and args.csv,
            'C, class count, both independent oracle logs, and CSV paths required')
    linear, latin, provenance = read_oracles(args.c, args.linear_log, args.latin_log)
    for path in args.csv:
        with path.open(newline='', encoding='ascii') as handle:
            result = audit_records(handle, args.c, args.classes, linear, latin)
        with path.open('rb') as handle:
            digest = hashlib.file_digest(handle, 'sha256').hexdigest().upper()
        result.update(csv=str(path.resolve()), csv_sha256=digest, oracles=provenance)
        print(json.dumps(result, sort_keys=True))


if __name__ == '__main__':
    main()
