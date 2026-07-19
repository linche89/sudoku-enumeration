# Reverse-Gluing C=6 Frontier Probe — 2026-07-18

## Scope and safety

This report records a bounded decision experiment for the reverse `2+2`
row-block route.  It did not generate a complete four-row layer, did not enter
the 63,199-class contraction, and did not read or write any C=6 checkpoint.
Every four-row sample used positive pair, relative-placement, and contingency
leaf limits.  C=6 partial-file output is rejected by the prototype.

The retained source is `experiments/proto/reverse_glue.cpp`.  The dense
`|G| x 2^(2C)` mask-image table was replaced with lazy columns: the exact C=6
two-row layer used 4,884,480 cached-image bytes rather than a 377,487,360-byte
dense table.

## Exact two-row orbit construction

A two-row configuration is a loopless 2-regular multigraph `H` on the `2C`
coordinate vertices.  Its unlabelled symbol occurrences are the edges of
`H`; a two-cycle is represented by a double edge.  The box pairs form a
perfect matching `P` disjoint from `H`.  Therefore coordinate-group orbits are
exactly isomorphism classes of pairs `(H,P)`.

The new generator fixes one `H` for every cycle partition of `2C`, enumerates
the admissible perfect matchings, and quotients them by `Aut(H)`.  Each result
is independently canonicalized under `C2 wr S_C`, including a full stabilizer
scan.  Against the old labelled-coordinate generator it matched every sorted

```text
(representative, stabilizer, coordinate-orbit size, F_2)
```

entry at C=3, C=4, and C=5:

| C | labelled two-row configurations | two-row orbits | result |
|---:|---:|---:|---|
| 3 | 40 | 5 | complete differential passed |
| 4 | 2,019 | 23 | complete differential passed |
| 5 | 165,744 | 107 | complete differential passed |

The C=6 construction then gave:

```text
group order                         = 46080
labelled-coordinate configurations = 20338525
two-row coordinate orbits           = 772
unordered two-row orbit pairs       = 298378
generation time                     = 1.53 s
canonical group images              = 35573760
```

The old DFS was also run in count-only mode, with no canonicalization.  It
independently counted exactly `20,338,525` configurations in about 72 seconds.
This closes the coordinate mass independently of the cycle/matching quotient.

The C=6 two-row stabilizer histogram is:

```text
1:276 2:249 4:129 6:6 8:37 12:9 16:20 24:5 32:15 48:6
64:7 72:3 128:3 144:1 256:1 288:2 384:1 512:1 3072:1
```

In particular, 276 orbits have trivial stabilizer.  Their 38,226 unordered
pairs each have exactly 46,080 double-coset placements, so the complete `2+2`
join has the rigorous lower bound

```text
38226 * 46080 = 1761454080
```

double-coset representatives before counting any pair involving a nontrivial
stabilizer.

Thus the number `60 = choose(6,2) * 2^2` counts masks available to one symbol;
it is not the two-row configuration count or orbit count.

## Bounded four-row samples

Three pair positions were sampled.  A placement-only pass first counted the
exact `Stab(x)\G/Stab(y)` representatives for the selected pair.  A second pass
processed one relative placement with a 100,000-leaf ceiling and skipped
canonicalization, so the reported leaf and raw-key counts are exact for that
placement.  All three calls closed below the ceiling.

| pair ordinal | `(x,y)` | `(stab x,stab y)` | coordinate orbit sizes | double-coset placements | placement mass | contingency leaves | distinct raw keys |
|---:|---|---:|---:|---:|---:|---:|---:|
| 0 | `(0,0)` | `(384,384)` | `(120,120)` | 10 | 120 | 2,662 | 417 |
| 149,189 | `(226,368)` | `(2,16)` | `(23,040,2,880)` | 1,448 | 2,880 | 15,360 | 15,168 |
| 298,377 | `(771,771)` | `(48,48)` | `(960,960)` | 43 | 960 | 27,793 | 10,272 |

Here “placement mass” is the sum of left-stabilizer orbit sizes over all
double cosets, and equals the full coordinate orbit size of `y`, as required.

Separate 2,000-leaf canonicalization probes produced:

| pair ordinal | raw keys in prefix | canonical four-row representatives | cold canonicalizations/s |
|---:|---:|---:|---:|
| 0 | 399 | 24 | 682 |
| 149,189 | 2,000 | 2,000 | 502 |
| 298,377 | 1,553 | 308 | 373 |

The generic middle sample had no raw or canonical duplication in its first
2,000 leaves.  It required 92,160,000 coordinate-group images for those 2,000
cold canonicalizations.  These are prefix measurements, not estimates of the
complete four-row orbit inventory.

## Key-only anchored canonicalization

For an external-sort design, stabilizers need not be computed for every raw
record.  A four-row C=6 mask can always be mapped to the fixed first mask that
uses side zero in the first four boxes.  Enumerating only group elements that
map some input mask to that target gives at most

```text
12 * 4! * 2! * 2^2 = 2304
```

candidate images per raw record, rather than 46,080.  The prototype now has a
probe-only key path that performs this exact minimization without computing a
stabilizer.  The first 100 middle-sample keys were individually compared with
a full 46,080-image canonicalization and all matched.

For the exact middle placement, 15,168 raw keys became 15,132 anchored
canonical keys at about 7,094 keys/s.  A seven-placement prefix stopped at the
global 100,000-leaf bound and reported:

```text
contingency leaves       = 100000
raw keys                 = 99808
canonical keys           = 99423
candidate group images   = 228750336
average candidates/key   = 2291.904
measured key rate        = 8194.729 / s
```

Thus delayed key-only canonicalization is about an order of magnitude faster
than immediately computing a full stabilizer, but the general sample still
has only about 0.4% canonical duplication.  This is the correct starting
point for a bulk/external implementation; it does not rescue the naive
pairwise join by itself.

## Decision

The reverse-gluing identity remains exact, and the C=6 two-row layer is now
known exactly.  However, the proposed cheap orbit-pair implementation does not
pass its C=6 scale gate:

- 772 two-row orbits give 298,378 unordered orbit pairs, not a very small
  orbit inventory;
- an unreduced 46,080-element group sum for every pair would already require
  13,749,258,240 group placements before any contingency work;
- double-coset reduction is essential, but the middle sampled pair still has
  1,448 placements, and trivial-stabilizer pairs alone force at least
  1,761,454,080 placements globally;
- one general sampled placement emitted 15,360 contingency leaves and 15,168
  distinct raw outputs, so the kernel is not a microsecond-scale scalar
  operation at C=6;
- on-site canonicalization with a stabilizer is only hundreds of cold outputs
  per second; anchored key-only canonicalization is much faster, but a
  100,000-leaf generic prefix remains 99.4% distinct after canonicalization.

This does not disprove Pettersen's historical reverse-gluing route.  It rules
out the particular simplification “brute group sum per orbit pair plus
immediate canonicalization.”  A viable historical-style implementation would
need a bulk four-row generator or lookup, delayed/external reduction, and a
canonical key substantially cheaper than a fresh 46,080-image scan.  The
reported “more than 900 million” historical lookup size is now qualitatively
consistent with the observed state explosion, but remains only a rough
comparison because its convention and files are unavailable.

Under the agreed route policy, the operator-valued double-permanent subset DP
therefore becomes the next global bounded decision experiment.  Reverse
gluing remains an open bulk/external-engineering route, not the next naive
pairwise computation.

## Reproduction commands

Build and small-C differential gates:

```powershell
g++ -O3 -mpopcnt -std=c++20 -Wall -Wextra `
  experiments\proto\reverse_glue.cpp -o build\reverse_glue.exe
.\build\reverse_glue.exe 3 cyclelayercheck summary
.\build\reverse_glue.exe 4 cyclelayercheck summary
.\build\reverse_glue.exe 5 layerprobe cyclelayercheck summary
```

Exact C=6 two-row layer and independent raw count:

```powershell
.\build\reverse_glue.exe 6 layerprobe layercountcheck summary
```

One bounded canonicalization sample:

```powershell
.\build\reverse_glue.exe 6 summary rawreduce `
  pairprobe=1 pairstart=149189 placementprobe=1 leafprobe=2000 `
  canoncachecap=5000
```

Anchored key-only sample with 100 full-key differential checks:

```powershell
.\build\reverse_glue.exe 6 summary rawreduce keyonlyprobe keyverify=100 `
  pairprobe=1 pairstart=149189 placementprobe=1 leafprobe=100000 `
  canoncachecap=5000
```

Exact placement count without gluing, followed by one exact contingency call
without canonicalization:

```powershell
.\build\reverse_glue.exe 6 summary rawreduce placementsonly `
  pairprobe=1 pairstart=149189 placementprobe=100000 leafprobe=1 `
  canoncachecap=5000

.\build\reverse_glue.exe 6 summary rawreduce nocanonicalprobe `
  pairprobe=1 pairstart=149189 placementprobe=1 leafprobe=100000 `
  canoncachecap=5000
```

The current source also re-read all 13 retained C=5 closed interval files,
verified exact `[0,5778)` coverage, reproduced 17,120 four-row orbits and all
355 complete classes, and returned the known exact `N(5)`.

The retained source SHA-256 is:

```text
111FC9F474BAE5472E8F93DA657186D2A389DE51904F2F7E7BCE3429EF390D40
```

The complete repository gate was then run:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts\verify_all.ps1
```

It finished in 135.4 seconds with `ALL REPOSITORY CHECKS PASSED`, including
complete C=2..5 factorization gates, all 71 FJ9 reference classes, and the
existing C=6 first-outer-class checkpoint verification in read-only mode.
