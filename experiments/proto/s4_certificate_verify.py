#!/usr/bin/env python3
"""Independent semantic verifier for a layer-DP final class certificate.

The layer-DP engine writes rows of the form

    qid, representative_words, coordinate_orbit_size,
    labelled_multiplicity, F

This verifier treats ``representative_words`` as the certificate witness.  It
does not trust the two supplied weights.  For every row it independently:

* checks that the 2C binary words form a legal balanced complete state;
* recomputes the prefix-order canonical representative under C2 wr S_C;
* counts the stabilizer and derives the coordinate orbit size;
* derives the labelled multiplicity from repeated words;
* checks uniqueness and the engine's deterministic qid ordering.

At C=6 it also constructs the audited G1 and G2 representatives from their
side-incidence masks and binds each known F value to the corresponding row.
The labelled mass and arbitrary-precision weighted square are recomputed as
additional cross-checks, but the separate s4_exact_sum.py remains the primary
external summation implementation.

The canonicalizer below is deliberately definition-level Python.  It explores
the signed-coordinate action by successive prefix partitions; it does not call
or load the C++ layer-DP implementation.
"""

from __future__ import annotations

import argparse
import csv
import hashlib
import math
import os
import struct
import sys
from collections import Counter
from dataclasses import dataclass
from typing import Iterable, Sequence


EXPECTED_HEADER = [
    "qid",
    "representative_words",
    "coordinate_orbit_size",
    "labelled_multiplicity",
    "F",
]

C6_ANCHORS = (
    (
        "G1",
        (0x95A, 0x4F2, 0x56C, 0x9B4, 0xE38, 0xFC0),
        120,
        6986348258918400,
    ),
    (
        "G2",
        (0x8EA, 0x572, 0x95C, 0xDA4, 0xE38, 0xFC0),
        8,
        7053808087203840,
    ),
)


class CertificateError(Exception):
    """A malformed or semantically false certificate."""


@dataclass(frozen=True)
class VerifiedRow:
    qid: int
    words: tuple[int, ...]
    orbit_size: int
    labelled_multiplicity: int
    F: int


def fail(message: str) -> "None":
    raise CertificateError(message)


def _candidate_signature_and_groups(
    words: Sequence[int],
    groups: tuple[tuple[int, ...], ...],
    source_coordinate: int,
    flip: int,
) -> tuple[int, tuple[tuple[int, ...], ...]]:
    """Return the next prefix signature and its ordered row partition.

    In each existing prefix cell, side 0 precedes side 1.  Concatenating those
    binary runs gives the same ordering comparison as the C++ canonical
    specification's fixed-width 2/3 field signature, without sharing its
    implementation.
    """

    signature = 0
    refined: list[tuple[int, ...]] = []
    for group in groups:
        zero: list[int] = []
        one: list[int] = []
        for row_index in group:
            side = ((words[row_index] >> source_coordinate) & 1) ^ flip
            (one if side else zero).append(row_index)
        signature = (signature << len(group)) | ((1 << len(one)) - 1)
        if zero:
            refined.append(tuple(zero))
        if one:
            refined.append(tuple(one))
    return signature, tuple(refined)


def canonicalize_and_stabilizer(
    words: Sequence[int], C: int
) -> tuple[tuple[int, ...], int]:
    """Return the specified canonical word tuple and exact stabilizer order.

    A frontier item is one signed partial coordinate permutation.  At every
    destination coordinate only the globally least next prefix signature can
    survive; the later coordinates cannot improve an earlier signature.  At
    depth C, the surviving actions are exactly the coset mapping the input to
    its canonical representative, so their number is the stabilizer order.
    """

    nwords = len(words)
    if nwords != 2 * C:
        fail("canonicalizer received %d words for C=%d" % (nwords, C))

    # (ordered prefix partition, used-source bit mask, destination path)
    frontier: list[
        tuple[tuple[tuple[int, ...], ...], int, tuple[tuple[int, int], ...]]
    ] = [((tuple(range(nwords)),), 0, ())]

    for _destination in range(C):
        best_signature: int | None = None
        next_frontier: list[
            tuple[
                tuple[tuple[int, ...], ...],
                int,
                tuple[tuple[int, int], ...],
            ]
        ] = []
        for groups, used, path in frontier:
            for source in range(C):
                source_bit = 1 << source
                if used & source_bit:
                    continue
                for flip in (0, 1):
                    signature, refined = _candidate_signature_and_groups(
                        words, groups, source, flip
                    )
                    item = (refined, used | source_bit, path + ((source, flip),))
                    if best_signature is None or signature < best_signature:
                        best_signature = signature
                        next_frontier = [item]
                    elif signature == best_signature:
                        next_frontier.append(item)
        if not next_frontier:
            fail("canonical search produced an empty frontier")
        frontier = next_frontier

    first_path = frontier[0][2]
    canonical_words = []
    for word in words:
        transformed = 0
        for destination, (source, flip) in enumerate(first_path):
            side = ((word >> source) & 1) ^ flip
            transformed |= side << destination
        canonical_words.append(transformed)
    canonical_words.sort()
    return tuple(canonical_words), len(frontier)


def state_memcmp_key(words: Sequence[int], C: int) -> bytes:
    """Reproduce the documented little-endian State memcmp qid order."""

    packed = []
    for word in words:
        mask = 0
        for box in range(C):
            mask |= (2 | ((word >> box) & 1)) << (2 * box)
        packed.append(mask)
    packed.extend([0] * (12 - len(packed)))
    return struct.pack("<12H", *packed)


def labelled_multiplicity(words: Sequence[int]) -> int:
    denominator = 1
    for multiplicity in Counter(words).values():
        denominator *= math.factorial(multiplicity)
    return math.factorial(len(words)) // denominator


def words_from_side0(side0: Sequence[int], C: int) -> tuple[int, ...]:
    raw = []
    for symbol in range(2 * C):
        word = 0
        for box, side0_mask in enumerate(side0):
            if not ((side0_mask >> symbol) & 1):
                word |= 1 << box
        raw.append(word)
    return tuple(sorted(raw))


def parse_args(argv: Sequence[str] | None = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Independently verify representative, orbit, and "
        "multiplicity semantics in a layer-DP final CSV certificate."
    )
    parser.add_argument("dump", nargs="?",
                        help="final per-class CSV from layer_dp_gate")
    parser.add_argument("--c", type=int, default=None, metavar="C")
    parser.add_argument("--classes", type=int, default=None)
    parser.add_argument("--expect-n", type=int, default=None, metavar="N")
    parser.add_argument(
        "--quiet",
        action="store_true",
        help="suppress periodic progress while retaining the final summary",
    )
    parser.add_argument(
        "--self-test-c6-anchors",
        action="store_true",
        help="construct and canonicalize the audited G1/G2 witnesses only",
    )
    args = parser.parse_args(argv)
    if args.c is not None and not 2 <= args.c <= 6:
        parser.error("--c must be in 2..6")
    if args.classes is not None and args.classes <= 0:
        parser.error("--classes must be positive")
    if args.self_test_c6_anchors:
        if args.dump is not None:
            parser.error("a dump cannot be combined with --self-test-c6-anchors")
    elif args.dump is None:
        parser.error("dump is required unless --self-test-c6-anchors is used")
    return args


def read_raw_rows(path: str) -> Iterable[tuple[int, list[str]]]:
    try:
        handle = open(path, "r", newline="", encoding="ascii")
    except (OSError, UnicodeError) as exc:
        fail("cannot open %s: %s" % (path, exc))
    with handle:
        reader = csv.reader(handle, strict=True)
        try:
            header = next(reader)
        except StopIteration:
            fail("%s: empty file" % path)
        except csv.Error as exc:
            fail("%s: malformed header: %s" % (path, exc))
        if header != EXPECTED_HEADER:
            fail("%s: unexpected header %r" % (path, header))
        try:
            for lineno, record in enumerate(reader, start=2):
                if not record:
                    fail("%s:%d: blank rows are not permitted" % (path, lineno))
                yield lineno, record
        except csv.Error as exc:
            fail("%s: malformed CSV: %s" % (path, exc))


def verify(path: str, requested_c: int | None, expected_classes: int | None,
           expected_n: int | None, quiet: bool) -> dict[str, object]:
    rows: list[VerifiedRow] = []
    seen: set[tuple[int, ...]] = set()
    previous_order_key: bytes | None = None
    C = requested_c
    group_order = None
    semantic_hash = hashlib.sha256()

    for lineno, record in read_raw_rows(path):
        if len(record) != 5:
            fail("%s:%d: expected 5 fields, got %d" % (path, lineno, len(record)))
        try:
            qid = int(record[0])
            words = tuple(int(token) for token in record[1].split())
            supplied_m = int(record[2])
            supplied_ell = int(record[3])
            F = int(record[4])
        except ValueError as exc:
            fail("%s:%d: non-integer field (%s)" % (path, lineno, exc))

        if C is None:
            if not words or len(words) % 2:
                fail("%s:%d: representative width is not positive even" %
                     (path, lineno))
            C = len(words) // 2
            if not 2 <= C <= 6:
                fail("%s:%d: inferred C=%d is outside 2..6" % (path, lineno, C))
            group_order = (1 << C) * math.factorial(C)
        assert C is not None
        if group_order is None:
            group_order = (1 << C) * math.factorial(C)

        if qid != len(rows):
            fail("%s:%d: qid %d != contiguous expected %d" %
                 (path, lineno, qid, len(rows)))
        if len(words) != 2 * C:
            fail("%s:%d: representative has %d words, expected %d" %
                 (path, lineno, len(words), 2 * C))
        if tuple(sorted(words)) != words:
            fail("%s:%d: representative words are not sorted" % (path, lineno))
        limit = 1 << C
        if any(word < 0 or word >= limit for word in words):
            fail("%s:%d: representative word outside [0,%d)" %
                 (path, lineno, limit))
        for box in range(C):
            ones = sum((word >> box) & 1 for word in words)
            if ones != C:
                fail("%s:%d: coordinate %d has %d side-1 symbols, expected %d" %
                     (path, lineno, box, ones, C))
        if supplied_m <= 0 or supplied_ell <= 0 or F < 0:
            fail("%s:%d: m/ell must be positive and F nonnegative" %
                 (path, lineno))

        canonical, stabilizer = canonicalize_and_stabilizer(words, C)
        if canonical != words:
            fail("%s:%d: representative is not the specified canonical form; "
                 "expected %s" %
                 (path, lineno, " ".join(map(str, canonical))))
        if group_order % stabilizer:
            fail("%s:%d: stabilizer %d does not divide group order %d" %
                 (path, lineno, stabilizer, group_order))
        derived_m = group_order // stabilizer
        if supplied_m != derived_m:
            fail("%s:%d: coordinate_orbit_size %d != derived %d "
                 "(stabilizer %d)" %
                 (path, lineno, supplied_m, derived_m, stabilizer))
        derived_ell = labelled_multiplicity(words)
        if supplied_ell != derived_ell:
            fail("%s:%d: labelled_multiplicity %d != derived %d" %
                 (path, lineno, supplied_ell, derived_ell))
        if words in seen:
            fail("%s:%d: duplicate canonical representative" % (path, lineno))
        seen.add(words)

        order_key = state_memcmp_key(words, C)
        if previous_order_key is not None and order_key <= previous_order_key:
            fail("%s:%d: representatives are not in strict qid order" %
                 (path, lineno))
        previous_order_key = order_key

        row = VerifiedRow(qid, words, supplied_m, supplied_ell, F)
        rows.append(row)
        semantic_hash.update(
            ("%d|%s|%d|%d|%d\n" %
             (qid, " ".join(map(str, words)), supplied_m, supplied_ell, F))
            .encode("ascii")
        )
        if not quiet and len(rows) % 5000 == 0:
            print("verified rows = %d" % len(rows), flush=True)

    if C is None or not rows:
        fail("%s: no data rows" % path)
    if expected_classes is None and C == 6:
        expected_classes = 63199
    if expected_classes is not None and len(rows) != expected_classes:
        fail("class count %d != expected %d" % (len(rows), expected_classes))

    mass = sum(row.orbit_size * row.labelled_multiplicity for row in rows)
    expected_mass = math.comb(2 * C, C) ** C
    if mass != expected_mass:
        fail("sum(m*ell) %d != binomial(%d,%d)^%d = %d" %
             (mass, 2 * C, C, C, expected_mass))

    anchors: list[tuple[str, int, int]] = []
    if C == 6:
        by_words = {row.words: row for row in rows}
        for name, side0, expected_stabilizer, expected_F in C6_ANCHORS:
            raw = words_from_side0(side0, C)
            key, stabilizer = canonicalize_and_stabilizer(raw, C)
            if stabilizer != expected_stabilizer:
                fail("internal %s witness stabilizer %d != expected %d" %
                     (name, stabilizer, expected_stabilizer))
            row = by_words.get(key)
            if row is None:
                fail("%s canonical representative is absent" % name)
            if row.F != expected_F:
                fail("%s row qid %d has F=%d, expected %d" %
                     (name, row.qid, row.F, expected_F))
            expected_m = group_order // expected_stabilizer
            if row.orbit_size != expected_m:
                fail("%s row qid %d has m=%d, expected %d" %
                     (name, row.qid, row.orbit_size, expected_m))
            anchors.append((name, row.qid, row.F))

    total = sum(
        row.orbit_size * row.labelled_multiplicity * row.F * row.F
        for row in rows
    )
    if expected_n is not None and total != expected_n:
        fail("weighted square %d != expected %d" % (total, expected_n))

    file_hash = hashlib.sha256()
    try:
        with open(path, "rb") as handle:
            for block in iter(lambda: handle.read(8 << 20), b""):
                file_hash.update(block)
    except OSError as exc:
        fail("cannot hash %s: %s" % (path, exc))

    return {
        "path": os.path.abspath(path),
        "C": C,
        "classes": len(rows),
        "mass": mass,
        "weighted_square": total,
        "csv_sha256": file_hash.hexdigest().upper(),
        "semantic_sha256": semantic_hash.hexdigest().upper(),
        "anchors": anchors,
    }


def main(argv: Sequence[str] | None = None) -> int:
    args = parse_args(argv)
    if args.self_test_c6_anchors:
        for name, side0, expected_stabilizer, expected_F in C6_ANCHORS:
            raw = words_from_side0(side0, 6)
            key, stabilizer = canonicalize_and_stabilizer(raw, 6)
            if stabilizer != expected_stabilizer:
                print("CERTIFICATE FAIL: %s stabilizer %d != %d" %
                      (name, stabilizer, expected_stabilizer), file=sys.stderr)
                return 2
            orbit = (1 << 6) * math.factorial(6) // stabilizer
            print("%s representative = %s; stab=%d; m=%d; expected F=%d" %
                  (name, " ".join(map(str, key)), stabilizer, orbit, expected_F))
        print("C6 ANCHOR SELF-TEST PASS")
        return 0
    try:
        summary = verify(
            args.dump, args.c, args.classes, args.expect_n, args.quiet
        )
    except CertificateError as exc:
        print("CERTIFICATE FAIL: %s" % exc, file=sys.stderr)
        return 2

    print("certificate = %s" % summary["path"])
    print("C = %d" % summary["C"])
    print("classes = %d  OK" % summary["classes"])
    print("sum(m*ell) = %d  OK" % summary["mass"])
    for name, qid, F in summary["anchors"]:
        print("%s = qid %d, F=%d  BOUND" % (name, qid, F))
    print("weighted_square = %d" % summary["weighted_square"])
    print("csv_sha256 = %s" % summary["csv_sha256"])
    print("semantic_sha256 = %s" % summary["semantic_sha256"])
    print("CERTIFICATE PASS")
    return 0


if __name__ == "__main__":
    sys.exit(main())
