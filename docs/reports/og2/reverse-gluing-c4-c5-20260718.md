# Reverse-Gluing C=4/C=5 Decision Gate — 2026-07-18

## Decision

Reverse row-block gluing passed its complete small-C decision ladder.  It now
precedes the unimplemented operator-valued double-permanent proposal in the
C=6 route portfolio.

This is not a C=6 feasibility claim.  The experiment verified the mathematics
through C=5 and identified canonicalization and low-stabilizer four-row
generation as the scaling boundary that a bounded C=6 probe must measure.

## Corrections made before implementation

The external suggestion was directionally useful but four details required
correction:

1. The coordinate group has order `2^C C!`, not `26!`.  Its C=4/5/6 orders
   are 384, 3,840, and 46,080.
2. Stored transporters and stabilizer generators are unnecessary, but exact
   stabilizer *orders* are still required by the orbit normalization.
3. Sixty at C=6 is the number `choose(6,2) 2^2` of masks available to one
   symbol.  It is not the number of two-row configurations or their orbits.
4. The implementation can normalize directly to `F(Q)` for one concrete
   complete representative.  `w(Q)F(Q)` is then a derived value; it is not an
   unavoidable native convention.

The historical Pettersen announcement remains evidence that a related route
was used successfully, not a locally reproducible existence proof of this
implementation or its cost.

## Implementation

The bounded prototype is:

```text
experiments/proto/reverse_glue.cpp
```

It represents a partial configuration by the sorted multiset of symbol
occurrence masks and implements:

- exact contingency-table gluing with the per-fixed-output multinomial;
- direct signed-coordinate group actions;
- checked stabilizer normalization;
- cancellation of repeated `Stab(y)` images;
- a direct `Stab(x)` double-coset quotient;
- unordered 2+2 orbit pairs with doubled off-diagonal contributions;
- an independent labelled-coordinate factorization oracle through C=4;
- C=5 anchored four-row/full canonicalization, followed by a complete group
  scan of every new representative to verify minimality and stabilizer order;
- closed binary pair-interval files with write/read round-trip and exact
  no-gap/no-overlap merge checks.

The standard build does not depend on this prototype.

## C=2..4 gates

Build and run:

```powershell
g++ -O3 -mpopcnt -std=c++20 -Wall -Wextra `
  experiments/proto/reverse_glue.cpp -o build/reverse_glue.exe

.\build\reverse_glue.exe 2 summary
.\build\reverse_glue.exe 3 summary
.\build\reverse_glue.exe 4 summary
```

All known totals passed.  The optimized C=4 run reported:

```text
group size                       = 384
labelled two-row configurations = 2019
two-row orbits                  = 23
unordered orbit pairs           = 276
double-coset representatives    = 6666
contingency leaves              = 5031
complete coordinate histograms = 1450
complete classes                = 26
N(4) = 29136487207403520
```

Every one of the 26 factorization values agreed with the independent local
oracle.  A separate process-level comparison also matched the complete
multiset of `(coordinate orbit, labelled multiplicity, F)` triples from
`factorization_orbit`.

## C=5 two-row and four-row layers

The exact two-row inventory is:

```text
possible masks for one symbol   = 40
labelled coordinate configs     = 165744
coordinate orbits               = 107
unordered 2+2 orbit pairs       = 5778
```

One long prototype process was discarded after an abrupt, non-C++-exception
exit: it produced no closed output.  The retained computation divided the
5,778 unordered pairs into 13 disjoint intervals.  Each process wrote a
temporary file, installed it only after the interval closed, reopened it, and
compared every record with the in-memory map.

The interval coverage was:

```text
[0,500)       [500,1000)    [1000,1500)   [1500,2000)
[2000,2500)   [2500,3000)   [3000,3500)   [3500,4000)
[4000,4500)   [4500,5000)   [5000,5250)   [5250,5500)
[5500,5778)
```

The exact merge reported:

```text
partial files               = 13
covered pair interval       = [0,5778)
merged four-row orbits      = 17120
four-row coordinate mass    = 62185328
combined file bytes         = 8895368
```

The sum of the independently timed interval kernels was:

```text
2+2 interval seconds        = 2781.435105
double-coset representatives= 3658027
relative placement mass     = 9128624
contingency leaves          = 122166792
pair-reduced records        = 40876321
cold canonicalizations     = 95227539
maximum leaves in one call  = 1296
```

The 2,781 seconds are deliberately conservative engineering evidence, not an
optimized single-process benchmark: every interval rebuilt its two-row layer,
representative stabilizers, and bounded raw cache.  The block times ranged
from 8.50 seconds to 531.57 seconds.  This skew demonstrates that leading
high-symmetry pairs are not a representative performance sample.

## C=5 4+1 closure

After exact interval merging, the final join reported:

```text
four-row input orbits        = 17120
one-row input orbits         = 1
orbit pairs / group actions  = 17120
contingency leaves           = 462403
pair-reduced records         = 380140
complete classes             = 355
4+1 time                     = 6.204522 s
labelled multiplicity sum    = 1016255020032
N(5) = 1903816047972624930994913280000
```

The final process-level differential parsed all 355 class lines from this
prototype and from:

```powershell
.\build\factorization_orbit.exe 5 pivot rooted4
```

The sorted multisets of

```text
(coordinate orbit size, labelled multiplicity, F(Q))
```

were identical (`355/355`).  Thus the validation is per class and per
factorization count, not only a collision at the final weighted square sum.

## Partial-file evidence

The ignored exact interval files are under:

```text
data/logs/reverse-glue-c5-parts-full-20260718/
```

| interval file | bytes | SHA-256 |
|---|---:|---|
| `part-0000-0499.bin` | 677416 | `CA301B560B450EF33346EEB139FAE4923550228EDA937EFE8AFEF2070210C5EF` |
| `part-0500-0999.bin` | 684816 | `CB560D88ED3AB5CAFC32090D7AA33A0463F292C0525D0F9A26B367B2229AD0C8` |
| `part-1000-1499.bin` | 684816 | `D3659AE2F39BFAC8A04A4F584CAB60DBE645FF4AA129FE2E01C030B174ACA82A` |
| `part-1500-1999.bin` | 684816 | `F42F5CAC77C678B647A3A86C9010F818F99DEE44B3405BF244D9E1E87772C58E` |
| `part-2000-2499.bin` | 684856 | `1E91FFFE3331C40F912174125438D6FFDFDF1999C0C755FDC07F1A222359F525` |
| `part-2500-2999.bin` | 684856 | `831534801215FABC315473F6FF3F246F65A4EF0A497D2E917FFDF87454D0A3AB` |
| `part-3000-3499.bin` | 684856 | `560615C9B871E358C2228580F31DA95E3A4B306A8C9FC9E60AB49F90C5823D2C` |
| `part-3500-3999.bin` | 684816 | `E99FA34721909E332E1DB6525E2FADBD5ADCE530AFCBE1E69B047226149FD3AF` |
| `part-4000-4499.bin` | 684816 | `576A9C401DAFEAEC8AF268024E8119AFFA4AB7324517C8A573BB9683B793B00E` |
| `part-4500-4999.bin` | 684816 | `B51DD4B30EE7213F82B5CE6E9CF4C9EACEBC55DCD8F0799B8B2AC29B0397B77C` |
| `part-5000-5249.bin` | 684816 | `A0284A7AFAFBE70676E9CFAAB9E2F274AFABEBE5E57FDC613E612959B5666236` |
| `part-5250-5499.bin` | 684816 | `B7B7B850F5C79BC4992981667438264E76DAF14CB4855A71BBB68A55CF317C56` |
| `part-5500-5777.bin` | 684856 | `48B70FE65A074CCBF484C928DC635DCCCFBB690CBA531BFAE3B2545813F9A3B4` |

These files are reproducible intermediate evidence, not a C=6 checkpoint and
not tracked by Git.  The retained source SHA-256 is:

```text
2D89D633FAF023994118E9C20EDA8467D2405F9B3F86575AA25D95E0877DCD5F
```

## Repository gate

After the prototype and documentation changes, the complete repository gate
was run from a normal PowerShell session:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts\verify_all.ps1
```

It finished with `ALL REPOSITORY CHECKS PASSED`.  This included the complete
C=2..5 factorization gates, all 71 FJ9 reference classes, and the existing C=6
first-outer-class checkpoint verification in read-only mode.  No C=6
checkpoint was written or modified.

## Scaling decision

What passed:

- the contingency coefficient and stabilizer normalization;
- direct group enumeration without stored transporters;
- double-coset and 2+2 exchange reductions;
- exact closed-interval restartability;
- complete C=5 classwise correctness.

What remains open:

- the exact number and generation method for C=6 two-row configuration
  orbits;
- the stabilizer distribution and double-coset counts for C=6 orbit pairs;
- C=6 four-row raw/canonical duplicate ratios, record rate, and bytes;
- whether the historical “more than 900 million” lookup figure uses the same
  labelled/orbit convention;
- an external canonicalization/sort design that does not materialize all
  coordinate images.

The immediate next experiment is therefore a positive-limit C=6 frontier
probe stratified by stabilizer and pair position.  A full four-row layer, a
63,199-class join, and a full `N(6)` run remain unauthorized.  The
operator-valued double-permanent route becomes the backup decision experiment
if this probe shows an uncontrollable four-row scale.
