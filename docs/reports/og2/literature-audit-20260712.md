# Historical audit of the 2x5 and 2x6 counts

Date: 2026-07-12 (Asia/Singapore)

This report corrects the earlier project assumption that the exact C=6 value
had never been published.  It records primary historical sources separately
from the project's independent calculations.

## C=5 primary announcement

Kjell Fredrik Pettersen posted the C=5 total on 2005-10-20:

```text
N(5) = 1903816047972624930994913280000
```

The post also gives the factorization

```text
10! * 4! * 5! * 2^18 * 3 * 5 * 131 * 35603 * 9932999
```

and states that the 5-by-10 band configurations were grouped into 355
equivalence classes.  A preceding post reports approximately ten hours for
the original computation.

Primary source:

- https://forum.enjoysudoku.com/su-doku-s-maths-t44-420.html#p12191

The later Jarvis-hosted Burnside page gives 524640665777288616345600 for the
identity symmetry after quotienting symbol relabeling.  Multiplying by 10!
gives the same labeled total exactly.

- https://www.afjarvis.org.uk/sudoku/sud25gp.html

## C=6 primary announcement

Pettersen's 2006 thread explicitly describes the outer computation as

```text
sum over classes C of E(C) * B(C)^2,
```

where E(C) is the class size and B(C) is the band-completion count.  It reports
63,199 band-configuration classes and initially estimates four to five weeks
of runtime.

On 2006-11-14 Pettersen announced:

```text
N(6) = 38296278920738107863746324732012492486187417600000
```

with factorization presented as

```text
(12! * 6! * 2^5 * 5!) * 2^24 * 3^4 * 5 * 4255797680395107022515709.
```

The thread says the intermediate values also exactly reconfirmed the number
of 6x2 Sudoku bands.  It does not expose a source-code download in the page
content inspected during this audit.

Primary source:

- https://forum.enjoysudoku.com/6x2-counting-t4835.html#p37761

## Consequences for current claims

1. The current factorization/orbit identity is a graph-theoretic
   reformulation and new implementation of Pettersen's historical outer
   class-square sum, not a priority claim for that decomposition.
2. The repository independently reproduces C=5 with a different inner
   one-factorization engine and an algorithmically separate transfer-kernel
   cross-check.
3. The current code independently reproduces the C=6 outer class count 63,199
   and has closed only its first outer class.  It has not verified the full
   historical C=6 decimal.
4. Future C=6 work should be described as an open, independently reproducible
   verification of Pettersen's announced value.
5. Archived notes that call C=6 unknown are retained only as history and do
   not override `STATUS.md`.
