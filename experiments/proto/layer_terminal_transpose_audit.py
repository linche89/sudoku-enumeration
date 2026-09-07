"""Read-only terminal graph-transpose check; no production counting code.

This is a symmetry consistency test, not an independent F reevaluation.
Use scripts/run_guarded_step.py with an explicit time/memory budget.
"""
from collections import defaultdict
import argparse
import csv
import hashlib
import json
from math import comb
from pathlib import Path

from s4_certificate_verify import canonicalize_and_stabilizer


def require(condition, message):
    if not condition:
        raise ValueError(message)


def transpose_words(words, c):
    """Pair complementary symbol neighborhoods, then exchange graph sides."""
    full = (1 << c) - 1
    occurrences = defaultdict(list)
    for i, word in enumerate(words):
        occurrences[word].append(i)
    if any(len(indices) != len(occurrences.get(word ^ full, ()))
           for word, indices in occurrences.items()):
        return None
    pairs = []
    for word in sorted(occurrences):
        if word < (word ^ full):
            pairs.extend(zip(occurrences[word], occurrences[word ^ full]))
    require(len(pairs) == c, 'transpose pairing width')
    output = []
    for old_box in range(c):
        for old_side in range(2):
            word = 0
            for new_box, (a, b) in enumerate(pairs):
                adjacent_a = ((words[a] >> old_box) & 1) == old_side
                adjacent_b = ((words[b] >> old_box) & 1) == old_side
                require(adjacent_a != adjacent_b, 'noncomplementary transpose pair')
                word |= int(adjacent_b) << new_box
            output.append(word)
    return tuple(sorted(output))


def audit(path, c, classes):
    with path.open(newline='', encoding='ascii') as handle:
        records = list(csv.DictReader(handle))
    require(len(records) == classes, 'unexpected class count')
    by_words = {}
    for qid, row in enumerate(records):
        require(int(row['qid']) == qid, 'noncontiguous qid')
        words = tuple(map(int, row['representative_words'].split()))
        require(len(words) == 2*c and all(0 <= w < (1 << c) for w in words),
                'invalid representative')
        require(words not in by_words, 'duplicate representative')
        by_words[words] = row
    maps = {}
    central_raw = 0
    for words, row in by_words.items():
        transposed = transpose_words(words, c)
        if transposed is None:
            continue
        canonical, _ = canonicalize_and_stabilizer(transposed, c)
        require(canonical in by_words, 'transposed orbit missing from complete CSV')
        other = by_words[canonical]
        require(int(row['F']) == int(other['F']), 'transpose F mismatch')
        maps[int(row['qid'])] = int(other['qid'])
        central_raw += int(row['coordinate_orbit_size'])
    require(all(maps.get(b) == a for a, b in maps.items()), 'transpose is not an involution')
    require(central_raw == comb((1 << (c-1)) + c - 1, c), 'antipodal raw mass mismatch')
    fixed = sum(a == b for a, b in maps.items())
    pairs = sorted((a, b) for a, b in maps.items() if a < b)
    require(len(maps) == fixed + 2*len(pairs), 'transpose orbit count mismatch')
    with path.open('rb') as handle:
        digest = hashlib.file_digest(handle, 'sha256').hexdigest().upper()
    return dict(status='PASS', C=c, classes=classes, csv=str(path.resolve()),
                csv_sha256=digest, antipodal_classes=len(maps),
                antipodal_raw_mass=central_raw, self_transpose_classes=fixed,
                exchanged_pairs=pairs, unpaired_graph_classes=classes-len(pairs),
                scope='terminal-only graph isomorphism and F symmetry; not F reevaluation')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--c', required=True, type=int, choices=range(2, 7))
    parser.add_argument('--classes', required=True, type=int)
    parser.add_argument('csv', type=Path)
    args = parser.parse_args()
    print(json.dumps(audit(args.csv, args.c, args.classes), sort_keys=True))


if __name__ == '__main__':
    main()
