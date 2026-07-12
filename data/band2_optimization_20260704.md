# C=5 band-2 optimization workbench, 2026-07-04

## Corrected baseline

The completed `mp_q 5` run reproduced OEIS A291187 n=5:

```text
C=5: N=1903816047972624930994913280000
  expect 1903816047972624930994913280000 [OK]
```

The run log includes a Windows sleep/hibernate interval:

```text
sleep/hibernate = 2026-07-04 01:22:54 +08:00 .. 08:03:38 +08:00
duration        = 24044.185s = 6.679h
```

Corrected active profile:

| band | essential tasks | active time | share | essential/s |
|---:|---:|---:|---:|---:|
| 0 | 1 | 3.0s | 0.0% | |
| 1 | 88 | 95.3s | 0.5% | |
| 2 | 8,458,157 | 15,634.9s | 79.8% | 541.0 |
| 3 | 8,458,157 | 3,865.3s | 19.7% | 2,188.2 |
| 4 | 88 | 4.4s | 0.0% | |

Implications:

- `band-2` is the main wall.
- `band-3` is a real second wall, but `dual` can skip it.
- The apparent late `band-2` slowdown was a sleep/hibernate artifact, not a sideMemo collapse.
- After `dual`, the remaining C=5 baseline is roughly `band0+band1+band2`, about 4.37h active.

## Current hot kernel

For every essential task in `band-2`:

```text
top side multiset -> hTop
bot side multiset -> hBot
for (tp in hTop)
  for (bp in hBot)
    raw target = merge(tp, bp)
    target = canon/cache(raw target)
    wtSum[target] += wt(tp) * wt(bp)
for target in wtSum
  next[target] += coeff * wtSum[target]
```

Known audit results:

| probe | result |
|---|---|
| exact `(topSide, botSide)` reuse | essentially zero in C=5 band-2 sample |
| exact `(topHist, botHist)` reuse | essentially zero |
| plain `(PartKey, PartKey)` reuse | essentially zero |
| C=5 band-2 avg cross/task | about 12.9k in sampled tasks |
| target coverage | 1000 sampled tasks hit 38,791 of 38,801 midpoint targets |
| PartKey orbit compression | C=5 sample about 1.84x, not enough |

## Working hypotheses

1. Small engineering changes can improve constants but are unlikely to give more than 2-3x.
2. `hTop x hBot` enumeration is not the wall by itself; the wall is raw target classification.
3. A useful middle-term win must make `raw target -> orbit/phiIndex` much cheaper, or change the state space.
4. If a fused direct kernel still needs per-raw exact classification, it is unlikely to be a 10x route.

## Experiment log

| date | command | scope | result | decision |
|---|---|---:|---|---|
| 2026-07-04 | `mp_q 5 kernelbench band=2 maxess=20000` | 20,000 band-2 tasks | avgCross 12,732; avgTargets 9,385; side 0.1%, cross/canon/target 88.4%, addMul 11.5% | side hist is not the wall; focus on cross/canon/target |
| 2026-07-04 | `mp_q 5 kernelbench band=2 maxess=100000` | 100,000 band-2 tasks | avgCross 12,907; avgTargets 9,555; side 0.1%, cross/canon/target 87.3%, addMul 12.5% | percentages are stable enough to guide next step |
| 2026-07-05 | `mp_q 3 scalaroracle 20` | full C=3 middle band | `28200960 [OK]`; missingPhi 0; exact denominator division | scalar oracle formula is correct on the first nontrivial odd-C gate |
| 2026-07-05 | `mp_q 5 scalaroracle maxess=20000` | 20,000 band-2 tasks | avgCross 12,863; avgTargets 9,486; side 0.1%, cross/canon/target 94.6%, scalarAdd 5.2%; exact partial quotient | scalar oracle reduces Big/output overhead but does not touch the main cross/canon wall |
| 2026-07-05 | `mp_q 5 scalarindex maxess=20000` | 20,000 band-2 tasks | direct raw->phiIndex cache; wall about 29s in first run; probeFail essentially fixed; scalarAdd about 3-4% | useful constant-factor path, not a breakthrough |
| 2026-07-05 | `mp_q 5 crossfloor band=2 maxess=20000` | 20,000 band-2 tasks | 250.9M raw merges in 0.189s wall for the middle-band portion; rawMergeFloor thread time 3.84s | raw pair enumeration/merge is not the wall; raw->phi/canon classification is the wall |
| 2026-07-05 | `mp_q 5 phiaudit 20 nocheckpoint` | C=5 phi catalogue | `refineTUV` leaves only 260 ambiguous groups / 788 collision states, much better than TUV | promising classifier coverage, but must be extremely cheap to help |
| 2026-07-05 | `mp_q 5 scalarrefine maxess=20000` | 20,000 band-2 tasks | `refineTUV` direct=171.5M fallback=1.9M but wall 64.1s | negative as written: string/refineTUV construction is too expensive |
| 2026-07-05 | `mp_q 5 scalarsig maxess=20000` | 20,000 band-2 tasks | `refineSig` direct=172.9M fallback=3.1M but wall 39.6s | cheaper than refineTUV, still slower than scalarindex |
| 2026-07-05 | `mp_q 3 scalarsigkey 20` | full C=3 middle band | `28200960 [OK]` | fixed-buffer signature path is exact on the first odd-C gate |
| 2026-07-05 | `mp_q 5 scalarsigkey maxess=20000` | 20,000 band-2 tasks | `refineSigKey` direct=154.9M fallback=2.7M; middle wall 33.5s; exact partial quotient | allocation-free signature removes the string penalty but still does not beat scalarindex materially |

Detailed 100k sample:

```text
sampled=100000/8458157
wall=172.731s
mergeWall=0.236s
outputStates=38801

hist avgTop=111.63 avgBot=111.71 maxTop=169 maxBot=169
cross=1290661816 avgCross=12906.62 maxCross=28561
targets=955521373 avgTargets=9555.21 maxTargets=17261
targetCompression=1.35

rawCache hit=427208075 miss=863453741 hitRate=33.10%
insert=158070220 probeFail=705383043

threadTime:
  side histogram        7.453s   0.1%
  cross+canon+target 4782.556s  87.3%
  addMul              686.843s  12.5%
```

Notes:

- A second 100k run after removing the obvious `hash | 2` low-bit bias did not improve the cache probe-fail problem, so that was not the main cause.
- A fuller hash finalizer was patched after these measurements, but the user stopped further probing before rerunning it. Treat the cache-probe diagnosis as unresolved until a later controlled rerun.
- The bottleneck conclusion does not depend on that unresolved hash detail: side histogram and merge are tiny; the dominant cost is still the `hTop x hBot` cross loop plus target canonicalization/aggregation.

## Scalar oracle prototype

Implemented `mp_q scalaroracle` for odd C.

For the middle band `L=C-H`, it builds

```text
phi(target) = low[complement(target)] / Q(target)
```

from the shallow midpoint states, scales all `phi` values to a common denominator
`lcm(Q(target))`, and computes the scalar dual contribution directly from the middle
band without materializing the next full DP map as the final result.

Validation:

```text
.\build\mp_q.exe 3 scalaroracle 20
C=3 scalaroracle: N=28200960
  expect 28200960 [OK]
```

C=5 20k sample:

```text
.\build\mp_q.exe 5 scalaroracle maxess=20000

scalaroracle: low=38801 phi=38801 denominatorLCM=104509440000
sampled=20000/8458157
quotient=4380006520697383524630528000
exact=yes

avgCross=12863.18
avgTargets=9485.77
targetCompression=1.36
missingPhi=0
coeffBigMul=0

threadTime:
  side histogram          1.448s   0.1%
  cross+canon+target   1014.940s  94.6%
  scalarAdd              55.997s   5.2%
```

Interpretation:

- Scalar oracle is a useful correctness oracle and strips the final map/Big-output layer.
- It is not the main speedup: `cross+canon+target` becomes an even larger fraction.
- The next prototype must attack the cross loop itself, not the post-cross accumulation.

## Raw classification findings

`crossfloor` showed that enumerating `hTop x hBot` and merging the two sorted `PartKey`s
is extremely cheap by itself:

```text
.\build\mp_q.exe 5 crossfloor band=2 maxess=20000

cross=250897661
avgCross=12544.88
middle-band wall=0.189s
threadTime:
  side histogram  1.299s 25.3%
  rawMergeFloor   3.840s 74.7%
```

Therefore the C=5 wall is not the Cartesian product loop itself.  It is the exact
classification of each raw merged target:

```text
raw target -> orbit / phi index
```

`scalarindex` replaced `raw -> canonical Key -> phi lookup` with a direct
`raw -> phiIndex` cache.  This fixed the cache probe-fail pathology and reduced
post-cross scalar accumulation, but it still has to canonicalize every first-seen raw.
The sample-level improvement is useful but small, not the required 10x.

`refineTUV` is strong as a catalogue invariant:

```text
C=5 phiaudit:
  TUV        groups=33635 collisionStates=9548
  refineSig  groups=37686 collisionStates=1450
  refineTUV  groups=38273 collisionStates=788
```

However, the naive hot-path implementation computes heap-allocated string signatures
per first-seen raw.  Even with high direct coverage, it is slower than `scalarindex`.

The fixed-buffer `SigKey` version removes that allocator cost:

```text
.\build\mp_q.exe 5 scalarsigkey maxess=20000

scalaroracle: refineSigKey groups=37686 direct=37348 ambiguous=338
sampled=20000/8458157
middle-band wall=33.546s
exact=yes

rawPhiIndexCache hit=92377709 miss=157620111 hitRate=36.95%
refine direct=154874033 fallback=2746078

threadTime:
  side histogram          1.510s   0.1%
  cross+canon+target   1023.179s  96.6%
  scalarAdd              34.942s   3.3%
```

This is an important negative result: `std::string` allocation was bad, but the
refinement signature computation itself is still too expensive to create a large win.

Current conclusion:

- Do not use string-valued refinement signatures in the hot loop.
- Fixed-buffer `refineSig` is now tested; it is correct but not materially faster than
  `scalarindex`.
- The next viable classifier must be cheaper than another full `refineColours` pass, or it
  must improve `canonMS2` itself.
- Since raw merge is nearly free, a true algebraic DP that only removes the product loop
  is no longer the first target; the first target is a much cheaper exact raw classifier.

## Next decisions

1. Keep `scalarindex` as the current best scalar prototype and correctness oracle.
2. Do not pursue more heap/string refinement signatures.
3. Treat fixed-buffer refinement signatures as a negative result unless a cheaper signature
   can reuse data already computed by `tryDiscreteCanonMS2` or `canonMS2`.
4. Avoid further growth of `multiset_q.cpp`; new hot-path variants should live in separate
   helper files.
