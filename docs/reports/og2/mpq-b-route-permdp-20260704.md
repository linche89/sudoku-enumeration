# B-route permanent DP prototype, 2026-07-04

Goal:

- Test whether a coloured restricted-permanent formulation can reproduce the existing side histogram exactly.
- Decide whether it is worth replacing `buildHistFast` directly, or only using this formulation as a foundation for a direct top/bot kernel.

## Model

For one side histogram, group symbols by their current `(xmask, ymask)` pair:

```text
g = (Xg, Yg), multiplicity ng
```

Then `buildHistFast` is equivalent to a coloured restricted permanent:

```text
choose a perfect matching between X columns and Y columns;
colour each edge (a,b) by group g;
use each group g exactly ng times;
edge (a,b,g) is legal iff a notin Xg and b notin Yg;
edge contributes token (Xg | 1<<a, Yg | 1<<b);
sorted tokens form the PartKey.
```

The prototype `buildHistPermDP` computes this with a DP over:

```text
X column index, used-Y mask, group-count mixed-radix code, partial sorted PartKey
```

It is a local side-hist replacement candidate and a foundation for a future direct middle-band kernel.

## Correctness checks

Commands:

```powershell
.\build\mp_q.exe 3 permdptest 2000
.\build\mp_q.exe 4 permdptest 20000
.\build\mp_q.exe 5 permdptest 5000
```

Results:

```text
permdptest C=3: tests=2000 mismatches=0 sumBad=0 empties=0  [OK]
permdptest C=4: tests=20000 mismatches=0 sumBad=0 empties=0  [OK]
permdptest C=5: tests=5000 mismatches=0 sumBad=0 empties=0  [OK]
```

Each test checks:

- `buildHistPermDP == buildHistFast`
- `buildHistPermDP == buildHistBrute`
- total histogram mass equals `validXbijections * validYbijections`

## Timing

Commands:

```powershell
.\build\mp_q.exe 4 histbench 20000
.\build\mp_q.exe 5 histbench 10000
```

Results:

| C | tests | `buildHistFast` | `buildHistPermDP` | ratio perm/fast | avg keys |
|---:|---:|---:|---:|---:|---:|
| 4 | 20000 | 0.021940s | 0.075791s | 3.45 | 6.65 |
| 5 | 10000 | 0.046462s | 0.191841s | 4.13 | 28.71 |

Interpretation:

- `buildHistPermDP` is exact, but slower as a drop-in side histogram replacement.
- Do not enable it in the current engine.
- Its value is structural: it expresses side histograms as a coloured restricted permanent, which can be extended toward a direct top/bot kernel that avoids materializing `hTop`, `hBot`, and their Cartesian product.

## Next experiment

Build a direct scalar/vector kernel from the permanent formulation:

```text
top side DP x bot side DP -> target-count vector or dual scalar
```

The key question is whether the two side DPs can be fused so that the output is target-weighted without enumerating almost every `(PartKey_top, PartKey_bot)` pair.
