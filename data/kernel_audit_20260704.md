# Kernel audit, 2026-07-04

Purpose:

- Decide whether the C=5 band-2 wall can be reduced by exact side/hist-pair reuse.
- Test whether simple target invariants can replace `phi(canon(target))` in the dual scalar kernel.

Build:

```powershell
g++ -O3 -mpopcnt -fopenmp -std=c++20 -I src src/multiset_q.cpp -o build/mp_q.exe
```

## Side/hist-pair reuse

Command:

```powershell
.\build\mp_q.exe 4 20 auditband=2 nocheckpoint
.\build\mp_q.exe 5 20 auditband=2 auditmax=200000 nocheckpoint
```

Results:

| C | band | inspected | essential total | unique top side | unique bot side | unique side any | unique side pair | unique hist | unique hist pair | avg cross | max cross |
|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| 4 | 2 | 4156 | 4156 | 3105 | 2339 | 4695 | 4156 | 3343 | 3389 | 10.70 | 256 |
| 5 | 2 | 200000 | 8458157 | 179753 | 175420 | 321043 | 200000 | 320864 | 199995 | 12906.96 | 28561 |

Interpretation:

- Exact `(topSide, botSide)` reuse is essentially zero in the sampled C=5 band-2 stream.
- Exact `(topHist, botHist)` reuse is also essentially zero.
- A simple side-pair or hist-pair cache is not a plausible 10x-100x lever for C=5.
- The next compression layer must identify equivalence under the column/swap group, or replace the side histogram computation with a coarser algebraic kernel.

## Phi coarse-invariant collisions

Command:

```powershell
.\build\mp_q.exe 4 20 phiaudit nocheckpoint
.\build\mp_q.exe 5 20 phiaudit nocheckpoint
```

`phiaudit` constructs the dual scalar weight

```text
phi(k) = wf[complement(k)] / Q(k)
```

from the shallow midpoint states, without entering the expensive middle band.

Results:

| C | target phi states | invariant | groups | collision groups | collision states | max group | max phi values in group |
|---:|---:|---|---:|---:|---:|---:|---:|
| 4 | 141 | multiplicity | 17 | 14 | 138 | 68 | 59 |
| 4 | 141 | T | 64 | 22 | 96 | 14 | 14 |
| 4 | 141 | TUV | 120 | 17 | 35 | 3 | 3 |
| 4 | 141 | aut/Q | 27 | 16 | 130 | 23 | 22 |
| 5 | 38801 | multiplicity | 74 | 63 | 38790 | 27035 | 1533 |
| 5 | 38801 | T | 10104 | 5158 | 33846 | 166 | 149 |
| 5 | 38801 | TUV | 33635 | 4407 | 9548 | 6 | 6 |
| 5 | 38801 | aut/Q | 57 | 40 | 38784 | 24618 | 1265 |

Interpretation:

- None of these coarse invariants is sufficient for exact `phi`.
- `TUV` is much closer than `T`, but still has thousands of collision groups at C=5.
- Any future phi coarsening must use a strictly stronger invariant and must keep this collision test as a gate.

## Current conclusion

The promising path is no longer exact hist-pair caching. The next serious candidate is a group/orbit kernel:

```text
hist pair -> orbit representative plus relative double-coset data
```

or a larger state-space change based on restricted-permanent / rook-polynomial / kjellfp-style profile vectors.

## Target orbit coverage

Command:

```powershell
.\build\mp_q.exe 4 20 targetauditband=2 nocheckpoint
.\build\mp_q.exe 5 20 targetauditband=2 targetauditmax=1000 nocheckpoint
```

Results:

| C | band | inspected | cross pairs | avg cross/task | PartKey pairs | part-pair compression | global targets | avg targets/task | max targets/task | cross/global target |
|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| 4 | 2 | 4156 | 44469 | 10.70 | 9041 | 4.92 | 5 | 2.53 | 5 | 8893.80 |
| 5 | 2 | 100 | 1097628 | 10976.28 | 1097627 | 1.00 | 38640 | 8199.29 | 16086 | 28.41 |
| 5 | 2 | 1000 | 12923324 | 12923.32 | not measured | not measured | 38791 | 9048.84 | 16778 | 333.15 |

Interpretation:

- C=5 target coverage saturates almost immediately: 1000 sampled tasks already hit 38791 of the 38801 midpoint targets.
- Inside one C=5 task, target aggregation is weak: about 12923 cross pairs collapse to about 9049 targets on average.
- A cache keyed only by canonical target orbit is therefore not enough.
- A plain `(PartKey, PartKey) -> target` cache is also not useful at C=5: the first 100 sampled tasks had 1097628 cross pairs and 1097627 distinct PartKey-pair hashes.
- The needed object is an algebraic way to compute the full target-count vector for a side-hist pair without enumerating nearly every `hTop x hBot` pair.

Note: the C=5 run used `cacheLog2=20`, so raw-canon cache statistics from this command are not representative of production. The target-coverage counts are still meaningful.

## PartKey orbit sample

Command:

```powershell
.\build\mp_q.exe 4 20 targetauditband=2 partorbit nocheckpoint
.\build\mp_q.exe 5 20 targetauditband=2 targetauditmax=100 partorbitmax=1000 nocheckpoint
```

Results:

| C | band | task sample | PartKey sample | PartKey orbits under `S_C x S_C x swap` | compression |
|---:|---:|---:|---:|---:|---:|
| 4 | 2 | 4156 | 3200 | 16 | 200.00 |
| 5 | 2 | 100 | 1000 | 544 | 1.84 |

Interpretation:

- C=4 has strong PartKey orbit compression, but C=5 does not show the same behavior in the sampled band-2 stream.
- Plain PartKey orbit canonicalization is therefore not enough for a large C=5 speedup.
- A double-coset/coherent-configuration kernel may still help, but it has to compute grouped target-count vectors algebraically; it cannot just be a memo table over individual PartKey orbits.
