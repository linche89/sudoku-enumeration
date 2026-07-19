# Fixed-Source Frontier Lower Bound and Expert Follow-up Audit — 2026-07-20

## Scope

This report audits the two raw responses received after the current C=6
mathematical-bottleneck question:

- `docs/expert/2026-07-19/fable_response.md`, SHA-256
  `807D91EC516C1E71025300DBB5D3E96EE09D3AA55879A3E9E989BB7686C192E4`;
- `docs/expert/2026-07-19/gpt5.6_pro_response.md`, SHA-256
  `5FBA3C92678A051039A84D7C9AA1F5014B7E06E6937C98A53D27CFD6170D80A3`.

Raw expert material is not authority.  The retained conclusion below was
independently implemented in
`experiments/proto/source_target_frontier_bound.cpp`.  The program performs a
bounded arithmetic certificate only.  It does not enumerate a C=6 DP layer,
read or write a checkpoint, evaluate an outer class, or accumulate `N(6)`.

## Exact support identity

Fix a fully labelled grade-`k` source

```text
s = ((S_i,T_i))_(i=1..2C),  |S_i|=|T_i|=k,
```

where `S_i` and `T_i` are the columns already used by symbol `i` in the two
copies.  Let `M(S)` be the maps `a:[2C]->[C]` such that `a_i` is not in `S_i`
and every column has exactly two preimages; define `M(T)` similarly.

For fixed `s`, each pair `(a,b)` determines one labelled target by

```text
(S_i,T_i) -> (S_i union {a_i}, T_i union {b_i}).
```

The target determines `(a,b)` by set difference, so two different map pairs
cannot give the same labelled target.  The two fibers of every column form a
perfect matching on the symbols.  The union of the matchings from `a` and `b`
is a disjoint union of alternating even cycles.  A shared balanced cut is a
two-coloring of those cycles and therefore exists with multiplicity
`2^(number of cycles)`.  Every target coefficient is consequently positive,
and the labelled support is exactly

```text
T(s) = |M(S)| * |M(T)|.
```

For a side mask list `S`, form the `2C x 2C` zero-one matrix `B_S` with two
distinguishable slots for each column and

```text
B_S[i,(c,slot)] = 1 iff c is not in S_i.
```

Every balanced map has `2^C` lifts to the distinguishable slots, giving

```text
|M(S)| = permanent(B_S) / 2^C.
```

This is an exact support formula.  It is not an estimate from observed hash
uniqueness.

## Linear-frontier lower bound

Consider a fixed-order linear within-band frontier for one fixed source whose
output semantics distinguish the labelled target.  At a cut of the processed
columns, a complete target recovers its unique assignment prefix.  Different
prefixes therefore have disjoint nonzero completion-and-target support.  The
rows of the prefix/suffix coefficient flattening are linearly independent
over the rationals and over every odd characteristic.

If `H_s` is the stabilizer of the source under the independent column groups
and copy exchange, the same argument after exact orbit reduction gives

```text
frontier width >= number of H_s-orbits of extendable prefixes,
terminal width >= ceil(T(s) / |H_s|).
```

For a trivial-stabilizer source the terminal lower bound is exactly `T(s)`.
This theorem covers a direct target-labelled frontier, an exact signed
Ryser/Glynn frontier that still returns the full target polynomial, and any
other fixed-source linear reordering with the same output semantics.

It deliberately does not lower-bound the rank of the target-only reduced
operator.  A transform that sums different sources directly into one target
orbit before representing `(source,target)` pairs is outside the theorem.

## Retained C=6 witness

The witness is reachable after two legal shared-cut bands.  Its twelve
derived source types are:

```text
16/46 35/16 35/23 46/45 12/24 24/56
16/13 25/16 34/35 45/23 36/25 12/14
```

All types are distinct.  Exhausting `S6 x S6` and copy exchange gives one
ordinary automorphism and no copy-swapping automorphism, so the full source
stabilizer is trivial.  The exact values are:

| quantity | value |
|---|---:|
| `permanent(B_S)` | 4,743,616 |
| `permanent(B_T)` | 4,740,096 |
| `|M(S)|` | 74,119 |
| `|M(T)|` | 74,064 |
| labelled/diagonal-pair terminal support | 5,489,549,616 |

For each copy, all complete balanced maps were independently enumerated and
projected onto every one of the 20 choices of three processed columns.  The
support ranges were:

```text
copy X: 6488 .. 7806
copy Y: 6503 .. 7806
```

The two copies choose their three-column subsets independently.  Every fixed
`3+3` split therefore has flattening rank at least

```text
6488 * 6503 = 42,191,464.
```

Materializing the terminal pair support at only 16 bytes per record would
already require 87,832,793,856 bytes, or 81.801 GiB, before table overhead or
exact coefficients.

## Independent verifier

The retained program derives the source masks from the two cuts and four
assignment vectors rather than accepting the listed masks as input.  It then
uses three independent count paths:

1. subset DP for both `12 x 12` permanents;
2. Ryser inclusion-exclusion for the same permanents;
3. a raw DFS over balanced maps, followed by all 20 three-column projections.

It also exhausts all `720^2 * 2` column/copy transformations.  Build and run:

```powershell
g++ -O3 -std=c++20 -Wall -Wextra `
  experiments\proto\source_target_frontier_bound.cpp `
  -o build\source_target_frontier_bound.exe

.\build\source_target_frontier_bound.exe
```

Source identity and local build evidence:

```text
source = 6F069C5B7520AAA63847CEBE481C0D57BD60949D8E9C80E5FB4BD151A383AD61
first audit PE image = CC9AA0DD336A858D2B24DBA1F94E79A4C5E2346814F9666F0A7AD204C4D1477A
fresh verification PE image = D005110AC79F0D34EA3CF323816D83F70661A9275DFAB9FBBB5827BC20D5095D
```

The source hash is the retained identity.  MinGW PE link timestamps make the
executable hash rebuild-dependent, so binary hashes are recorded as local
evidence rather than an acceptance criterion.

Five local runs averaged 0.1072 seconds, with a range of 0.1038--0.1166
seconds.  The one-second RSS watcher did not sample before the process exited.
The complete output ended in:

```text
terminalSupport=5489549616 recordBytes16=87832793856 recordGiB16=81.801
scalarResources C5=21252/63504 C6=263844/853776 gradeZeroC6=248314429440000
orbitalDimensionLower=1761454080 [OK]
```

After the documentation update, the complete repository gate also passed:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts\verify_all.ps1
```

This reproduced all exact C=2..5 totals, both complete C=5 future-twin modes,
the FJ9 71-class/reference-file gates, and the read-only C=6 G1 checkpoint
lookup.  The checkpoint SHA-256 remained
`FE8B68DE6C15592848D7CF69BF43928C000F0B2EE59FC263A094BCA2D376A865`.

## Coherent-configuration consequence

The independently verified reverse-gluing inventory has 276 two-row orbits
with trivial stabilizer in the coordinate group of order 46,080.  Their
38,226 unordered orbit pairs expose

```text
38226 * 46080 = 1,761,454,080
```

independent relative-placement coordinates before any pair with a nontrivial
stabilizer is included.  In representation language, a pair of regular
`G`-sets has an equivariant Hom space of dimension `|G|`; Fourier blocks only
change basis because `sum d_lambda^2=|G|`.  Materializing the full orbital or
coherent-configuration semantics therefore cannot remove the known
1.761-billion work factor.

This does not rule out a much smaller subspace containing only the specific
downstream weighted response.  Such a common response subspace must be
constructed and bounded; ordinary Burnside or a full orbital algebra is not
that construction.

## Audit of the other proposed exits

The separate PSD/Kraus identity

```text
V_(k+1) = sum_A M_A V_k M_A^T
```

is exact, but its factor rank is the rank of the same history-to-boundary map
already audited in the 3+3 route.  That map is full rank 630/630 at C=4 and
full row rank 8001/8001 at C=5.  A group-invariant matrix belongs to the
centralizer/orbital space, not to the one-point single-copy orbit space.  Thus
storing its factor in a single-copy orbit basis is not a valid reduction, and
no new C=6 cost bound was supplied.

The proposed graph-invariant regression is at most a conjecture screen.  A
read-only complete C=5 inspection found 354 connected classes and only one
disconnected class, so component multiplicativity has negligible coverage.
Grouping all classes by `(perfect-matching count, component count)` produced
56 groups with different exact `F` values.  For example, connected classes
335 and 345 both have 5,864 perfect matchings, but their factorization counts
are 1,244,006,400 and 1,253,683,200.  No precise stronger “cycle type” or proof
was supplied.

The retained read-only diagnostic commands were:

```powershell
.\build\factorization_orbit.exe 5 inspect limit=355
.\build\factorization_orbit.exe 5 pivot rooted4
```

The cycle-factor/matching identity was already used by the joint-histogram,
connectivity, and reverse-gluing implementations.  Replacing a path pairing
by an explicit shared cut changes coordinates but does not by itself compress
the target payload.  The new lower bound now rejects the proposed per-source
signed-permanent version whenever it retains that payload.

## Decision

The fixed-source, target-distinguishing B2/E3 frontier is rejected as a C=6
implementation mechanism.  Its near-injectivity is now a theorem with a
reachable exact witness rather than a C=5 sampling diagnosis.  Increasing
state limits, adding external sort, or applying signed inclusion-exclusion to
the same output polynomial cannot change the certified support or flattening
rank.

No positive C=6 route follows.  The surviving high-upside questions are
narrower:

1. apply the actual multi-source vector directly to target-orbit coefficients
   without materializing source-target pairs; or
2. prove that the downstream responses of many reverse-gluing regular blocks
   lie in a small common Fourier/communication subspace.

Neither object currently has an exact construction or a bounded C=5
implementation.  No complete C=6 layer or 63,199-class run is authorized.
