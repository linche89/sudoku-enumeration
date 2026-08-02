#!/usr/bin/env python3
"""S4 external exact summation for the layer-DP band count.

Reads the per-class dump CSV written by ``layer_dp_gate --dump`` (header
``qid,representative_words,coordinate_orbit_size,labelled_multiplicity,F``,
where ``representative_words`` is a quoted space-separated list of 2C
integers) and computes, in exact arbitrary-precision integer arithmetic,

    N = sum over classes of  m * ell * F**2

with m = coordinate_orbit_size and ell = labelled_multiplicity.

Integrity gates, all evaluated BEFORE the total is printed:

  1. the row count equals the expected class count (``--classes``);
  2. every row carries the same even word count 2C, and the labelled mass
     satisfies  sum(m * ell) == binomial(2C, C) ** C  exactly;
  3. every value given via ``--expect-f`` appears in the F column (at C=6
     the two known class values are 6986348258918400 and 7053808087203840).

If ``--expect-n`` is given, the computed N is compared against it and the
script prints ``PASS``/``FAIL`` accordingly.

``--rehearsal`` instead requires the visibly watermarked last column
``F_rehearsal`` and labels the result as a mechanical checksum, never N(C).
The optional class count still gates against truncation, while labelled mass
is required to be no larger than the complete mass (a sparse chain need not
reach every final class).

Exit codes:
  0  all integrity gates (and the optional N comparison) passed
  1  usage or I/O error (bad CSV structure, unreadable file, bad flags)
  2  integrity gate failure
  3  N does not equal ``--expect-n``

Example (C=5 regression):

  python s4_exact_sum.py c5dump.csv --classes 355 \\
      --expect-n 1903816047972624930994913280000
"""

import argparse
import csv
import math
import re
import sys

EXPECTED_HEADER = ["qid", "representative_words", "coordinate_orbit_size",
                   "labelled_multiplicity"]
F_COLUMN_RE = re.compile(r"F\d*")  # accept F, F5, F6, ... as the last column


def fail(code, msg):
    print("FAIL: " + msg, file=sys.stderr)
    sys.exit(code)


def parse_args(argv):
    p = argparse.ArgumentParser(
        description="Exact external summation N = sum m*ell*F^2 over a "
                    "layer_dp_gate per-class dump CSV, with integrity gates.")
    p.add_argument("dump", help="per-class dump CSV (from --dump)")
    p.add_argument("--classes", type=int, required=False,
                   help="expected number of data rows (complete classes)")
    p.add_argument("--expect-f", default=None, metavar="F1,F2,...",
                   help="comma-separated F values that must appear in the "
                        "F column")
    p.add_argument("--expect-n", type=int, default=None, metavar="N",
                   help="expected exact value of N; PASS/FAIL is printed")
    p.add_argument("--rehearsal", action="store_true",
                   help="require F_rehearsal input and print a non-N "
                        "mechanical checksum")
    args = p.parse_args(argv)
    if args.classes is not None and args.classes <= 0:
        p.error("--classes must be positive")
    if args.classes is None and not args.rehearsal:
        p.error("--classes is required outside --rehearsal mode")
    if args.expect_f is not None:
        try:
            args.expect_f = [int(t) for t in args.expect_f.split(",") if t]
        except ValueError:
            p.error("--expect-f must be a comma-separated list of integers")
        if not args.expect_f:
            p.error("--expect-f given but empty")
    return args


def read_dump(path, rehearsal=False):
    """Return (rows, C) where rows is a list of (m, ell, F) ints."""
    rows = []
    words_per_row = None
    try:
        f = open(path, newline="", encoding="ascii")
    except OSError as e:
        fail(1, "cannot open %s: %s" % (path, e))
    with f:
        reader = csv.reader(f)
        header = next(reader, None)
        if header is None:
            fail(1, "%s: empty file" % path)
        final_column_ok = False
        if len(header) == 5:
            final_column_ok = (
                header[4] == "F_rehearsal" if rehearsal else
                F_COLUMN_RE.fullmatch(header[4]) is not None)
        if (len(header) != 5 or header[:4] != EXPECTED_HEADER
                or not final_column_ok):
            fail(1, "%s: unexpected header %r" % (path, header))
        for lineno, rec in enumerate(reader, start=2):
            if not rec:
                continue  # tolerate a trailing blank line
            if len(rec) != 5:
                fail(1, "%s:%d: expected 5 fields, got %d"
                     % (path, lineno, len(rec)))
            try:
                qid = int(rec[0])
                words = [int(w) for w in rec[1].split()]
                m, ell, F = int(rec[2]), int(rec[3]), int(rec[4])
            except ValueError as e:
                fail(1, "%s:%d: non-integer field (%s)" % (path, lineno, e))
            if words_per_row is None:
                words_per_row = len(words)
                if words_per_row == 0 or words_per_row % 2:
                    fail(1, "%s:%d: word count %d is not a positive even "
                         "number" % (path, lineno, words_per_row))
            elif len(words) != words_per_row:
                fail(1, "%s:%d: word count %d differs from first row's %d"
                     % (path, lineno, len(words), words_per_row))
            if m <= 0 or ell <= 0 or F < 0:
                fail(1, "%s:%d: non-positive m/ell or negative F"
                     % (path, lineno))
            if qid != len(rows):
                fail(1, "%s:%d: qid %d is not the expected contiguous %d"
                     % (path, lineno, qid, len(rows)))
            rows.append((m, ell, F))
    if not rows:
        fail(1, "%s: no data rows" % path)
    return rows, words_per_row // 2


def main(argv=None):
    args = parse_args(argv)
    rows, C = read_dump(args.dump, args.rehearsal)

    # --- integrity gates (all BEFORE N is printed) -----------------------
    print("dump = %s" % args.dump)
    print("C = %d  (2C = %d words per representative)" % (C, 2 * C))
    if args.rehearsal:
        print("mode = BOUNDED REHEARSAL (watermarked; result is NOT N(%d))" % C)

    if args.classes is not None:
        if len(rows) != args.classes:
            fail(2, "class count %d != expected %d"
                 % (len(rows), args.classes))
        print("classes = %d  OK (expected %d)"
              % (len(rows), args.classes))
    else:
        print("classes = %d  OK (contiguous qids)" % len(rows))

    mass = sum(m * ell for m, ell, _ in rows)
    expected_mass = math.comb(2 * C, C) ** C
    if args.rehearsal:
        if mass > expected_mass:
            fail(2, "captured sum(m*ell) = %d exceeds complete mass %d"
                 % (mass, expected_mass))
        print("captured sum(m*ell) = %d <= complete mass %d  OK"
              % (mass, expected_mass))
    else:
        if mass != expected_mass:
            fail(2, "sum(m*ell) = %d != binomial(%d,%d)^%d = %d"
                 % (mass, 2 * C, C, C, expected_mass))
        print("sum(m*ell) = %d  OK (== binomial(%d,%d)^%d)"
              % (mass, 2 * C, C, C))

    if args.expect_f is not None:
        present = {F for _, _, F in rows}
        missing = [F for F in args.expect_f if F not in present]
        if missing:
            fail(2, "expected F value(s) absent from dump: %s"
                 % ", ".join(map(str, missing)))
        print("expected F values present: %s  OK"
              % ", ".join(map(str, args.expect_f)))

    # --- exact total -----------------------------------------------------
    N = sum(m * ell * F * F for m, ell, F in rows)
    if args.rehearsal:
        print("rehearsal_checksum(%d) = %d  (NOT N(%d))" % (C, N, C))
    else:
        print("N(%d) = %d" % (C, N))

    if args.expect_n is not None:
        if N == args.expect_n:
            label = "rehearsal checksum" if args.rehearsal else "N"
            print("%s check vs expected %d: PASS" % (label, args.expect_n))
        else:
            label = "rehearsal checksum" if args.rehearsal else "N"
            print("%s check vs expected %d: FAIL" % (label, args.expect_n))
            sys.exit(3)
    sys.exit(0)


if __name__ == "__main__":
    main()
