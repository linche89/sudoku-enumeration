# C=5 global-skeleton-orbit / 1-factorization reproduction

Date: 2026-07-12 (Asia/Singapore)

## Result

The independent global-skeleton-orbit implementation reproduced the known
2x5 Sudoku count exactly:

\[
N(5)=1,903,816,047,972,624,930,994,913,280,000.
\]

The executable compared the result digit-for-digit with the embedded known
value and reported `[OK]`.

## Method

Source: `src/factorization_orbit.cpp`

1. Represent a complete skeleton sequence by the histogram of the membership
   patterns of its \(2C\) symbols.
2. Quotient those histograms by row permutations and independent row
   complements, using orbit traversal under signed-coordinate generators.
3. Reconstruct the regular bipartite graph \(Q_B\) for one representative of
   every outer orbit.
4. Count its ordered 1-factorizations by recursively deleting perfect
   matchings.
5. Share residual-graph results globally.  Residual graphs use exact
   bipartite individualization/refinement canonicalization, with a safe weak-key
   fallback when the canonical search exceeds its node budget.  The fallback
   can only miss a merge; it cannot merge unequal graphs.
6. Accumulate

   \[
   N(C)=\sum_{[B]} |\operatorname{Orb}(B)|F(Q_B)^2.
   \]

## Validation gates

Before the C=5 run, the same executable reproduced:

- C=2: 288
- C=3: 28,200,960
- C=4: 29,136,487,207,403,520

For C=5, the outer enumeration independently checked:

- raw balanced histograms: 589,392
- signed-row orbits: 355
- sum of labelled orbit multiplicities:
  \(1,016,255,020,032=252^5\)

All checks passed.

## Command and environment

```powershell
g++ -O3 -mpopcnt -std=c++20 -Isrc src/factorization_orbit.cpp -o build/factorization_orbit.exe
build\factorization_orbit.exe 5
```

- Compiler: MinGW-W64 GCC 13.2.0
- CPU: AMD Ryzen 9 9950X3D, 16 cores / 32 logical processors
- The new counter is single-threaded.

## Measured performance

- outer-orbit generation: 0.884108 s
- 355 factorization counts and exact accumulation: 254.996032 s
- total algorithm time: approximately 255.88 s = 4 min 15.88 s
- old quotient-transfer wall time: approximately 4-5 h
- measured wall-clock speedup: approximately 56x-70x
- largest observed working set during polling: at least 490 MB

Final internal counters:

```text
graphMemo=15753
graph calls=5756382
graph memo hits=5732626
perfect matchings enumerated=14191134
canonicalization cache=7955808
canonicalization fallbacks=2
canonical search nodes=141992037
```

## Exact output

```text
C=5 classes=355/355 N=1903816047972624930994913280000
stats countTime=254.996032s outerTime=0.884108s graphMemo=15753 calls=5756382 hits=5732626 misses=15753 PM=14191134 canonCache=7955808 canonComputations=7955808 canonCacheHits=11983705 canonFallbacks=2 canonNodes=141992037
expected=1903816047972624930994913280000 [OK]
```

Raw logs (kept locally as transient run artifacts and intentionally not committed):

- `data/factorization_orbit_c5_20260712.out`
- `data/factorization_orbit_c5_20260712.progress.log`

SHA-256:

```text
0B9318075F2F21AE79FD8781776514F395A1C26B8662D5CCDCA234B4697614C4  src/factorization_orbit.cpp
E3AEF144F928491AE779885D344948BB8939F4B62789D34CE9893F438AA651BF  data/factorization_orbit_c5_20260712.out
A25A5CE8A6D9F67C916A13F2032192D30F2FDFA3DB2F0F980790E3F586F14121  data/factorization_orbit_c5_20260712.progress.log
```
