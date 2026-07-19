# Reduced Band-Kernel Rank Gate — 2026-07-19

## Question and decision threshold

The distinct Problem-B operator is the symmetry-reduced paired band transfer

```text
Kbar_k : Wbar_k -> Wbar_(k+1).
```

It is not the previously rejected 3+3 representation-theoretic channel map.
The decisive square instance first occurs at C=5, where the stack-swap basis
has 38,801 states at both grades two and three.  A very small rank would permit
an exact factorization through a much narrower intermediate space.

The predeclared bounded endpoint was to verify complete C=2--4 matrices, then
raise the C=5 lower bound through at most 1,024 before considering a full-rank
calculation.  Calibrations used 16, 64, and 256 rows; the successful 1,024-row
run subsumes an otherwise redundant 512-row check.  The 1,024 threshold is a
decision against a few-hundred-channel factorization; it is not a claim that
the 38,801-square matrix has full rank.

## Implementation

Commit `94db35380de7281fc770ed137dabcfd9faa1e29c` adds
`experiments/proto/band_kernel_rank.cpp`.  The prototype includes the audited
`multiset_q.cpp` state, canonicalization, tagged-skeleton quotient, and side
histogram code in the same translation unit.  It is not part of the standard
build.

The final audit artifacts are:

```text
source SHA-256 = 96DD2C11ADE0BEB51B10B3EC11F9177551644AD472C4FF3A8A1F8C49C8C46423
binary SHA-256 = 2748D4221D8CB1E56DD787EA9A02B4936C21D24E4C83AB2F1ADD475CC7C85D9D
```

Build command:

```powershell
g++ -O3 -mpopcnt -fopenmp -std=c++20 -I src `
  experiments\proto\band_kernel_rank.cpp `
  -o build\band_kernel_rank.exe
```

## Complete exact C=2--4 gate

For every source and skeleton through C=4, the optimized side histogram was
compared with the independent brute restricted-bijection builder.  The
prototype then materialized each complete reduced transition matrix, propagated
the endpoint vector, computed an exact Bareiss determinant for square layers,
and computed rank modulo both 1,000,000,007 and 1,000,000,009.  Whenever a
modular rank reaches `min(rows,columns)`, the corresponding nonzero integer
minor proves that maximal rank over the rationals exactly.

| C | reduced dimensions | exact rational ranks by layer | endpoint |
|---:|---:|---:|---:|
| 2 | 1, 2, 1 | 1, 1 | 288 |
| 3 | 1, 3, 3, 1 | 1, 3, 1 | 28,200,960 |
| 4 | 1, 5, 141, 5, 1 | 1, 5, 5, 1 | 29,136,487,207,403,520 |

The C=3 middle matrix has nine nonzero entries and exact determinant
2,048,000.  The C=4 `5 x 141` and `141 x 5` matrices each have 677 nonzero
entries.  All 21,280 C=4 side-histogram differentials passed.  The flat
modular eliminator used for the C=5 certificate also agrees with the separate
rectangular eliminator on the C=3 square matrix.

Commands:

```powershell
.\build\band_kernel_rank.exe 2
.\build\band_kernel_rank.exe 3
.\build\band_kernel_rank.exe 4
```

## Exact C=5 basis

The active `mp_q` engine generated only the first two closed bands in a guarded
research run.  It reached seven grade-one states in 2.8 seconds and exactly
38,801 grade-two states in 93.0 seconds.  Building the tagged task inventory
for the next band independently reproduced 8,458,157 essential tasks from
9,777,852 raw `(state,skeleton)` pairs.  Peak observed RSS was 4.653 GiB.

```powershell
& .\scripts\watch_rss.ps1 `
  -Exe .\build\mp_q.exe `
  -Arguments @(
    '5','targetauditband=2','targetauditmax=1','26',
    'ckpt=data\logs\band-kernel-rank-basis-20260719') `
  -LimitGB 8 -MaxMinutes 12 -IntervalSeconds 2 `
  -LogPath data\logs\band-kernel-rank-basis-20260719.rss.csv `
  -StdoutPath data\logs\band-kernel-rank-basis-20260719.out `
  -StderrPath data\logs\band-kernel-rank-basis-20260719.err
```

The ignored research checkpoint is 2,389,448 bytes:

```text
data/logs/band-kernel-rank-basis-20260719_C5_band1.chk
SHA-256 = 2C641031AF6030A68ACD0EB8340A22CB2A55F0CD7E849BBBFA50CDFD13BFA0A2
key fingerprint = ABB7B2B07B110F25
```

It is separate from, and did not access, the active C=6 graph-memo checkpoint.

## C=5 rank certificate

Let `A` be the integer `38801 x 38801` middle matrix.  For a sketch dimension
`m`, the prototype selects the deterministic source rows

```text
floor((2*i+1)*38801/(2*m)),  i=0,...,m-1,
```

so no two selected sources share a sketch row.  It constructs the complete
transition row for each selected source.  The target basis is generated in
advance by complementing every grade-two key; all 38,801 complements are
distinct.  A deterministic signed CountSketch `R` maps target columns to `m`
columns, producing

```text
S = E * A * R,
```

where `E` selects the `m` complete source rows.  Therefore, for either prime
`p`,

```text
rank_Fp(S) <= rank_Fp(A) <= rank_Q(A).
```

A full-rank `m x m` sketch is consequently an exact lower-bound certificate;
hash collisions can only lower the observed rank.  The fixed target hash seed
was `0x6a09e667f3bcc909`.

| m | rank mod 1,000,000,007 | rank mod 1,000,000,009 | exact cross pairs | canonical task-target entries | time | peak RSS |
|---:|---:|---:|---:|---:|---:|---:|
| 16 | 16 | 16 | 43,128,058 | 32,079,316 | 11.299 s | 0.262 GiB |
| 64 | 64 | 64 | 182,163,004 | 135,478,636 | 29.293 s | 0.539 GiB |
| 256 | 256 | 256 | 722,562,784 | 530,189,838 | 101.637 s | 2.032 GiB |
| 1,024 | 1,024 | 1,024 | 2,827,377,889 | 2,084,272,587 | 406.503 s | 4.521 GiB |

Every run had `badTargets=0`: every emitted canonical target belonged to the
precomputed exact complement basis.  The 1,024 elimination itself took 3.397
seconds.  Thus the verified conclusion is

```text
rank_Q(Kbar_2 at C=5) >= 1024.
```

The guarded command was:

```powershell
& .\scripts\watch_rss.ps1 `
  -Exe .\build\band_kernel_rank.exe `
  -Arguments @(
    '5','sketch',
    'basis=data\logs\band-kernel-rank-basis-20260719_C5_band1.chk',
    'samples=1024','cachelog=26',
    'seed=0x6a09e667f3bcc909',
    'out=data\logs\band-kernel-rank-sketch-c5-m1024-20260719.bin') `
  -LimitGB 7 -MaxMinutes 15 -IntervalSeconds 5 `
  -LogPath data\logs\band-kernel-rank-sketch-c5-m1024-20260719.rss.csv `
  -StdoutPath data\logs\band-kernel-rank-sketch-c5-m1024-20260719.out `
  -StderrPath data\logs\band-kernel-rank-sketch-c5-m1024-20260719.err
```

The installed 8,388,712-byte certificate has:

```text
SHA-256          = BFCD398F3EBB692048713E734A6AF862B2150679BE6DF2BF140B0188BE90F68A
embedded dataHash = F8620CF513430C4B
```

It was reopened from disk, its magic/header/length and embedded data hash were
checked, and both modular ranks were recomputed:

```powershell
.\build\band_kernel_rank.exe verify `
  data\logs\band-kernel-rank-sketch-c5-m1024-20260719.bin
```

The verifier again returned ranks 1,024 and 1,024 with `[OK]`.

## Decision

The ordinary few-hundred-dimensional B1 proposal fails its predeclared scale
gate.  The C=3 square map is already full rank, C=4 obtains its low ranks only
from the adjacent five-dimensional bottlenecks, and the first unbottlenecked
C=5 map has rational rank at least 1,024.  In particular it cannot factor
exactly through the 126-dimensional C=5 matching-cycle kernel or any other
space of dimension at most 1,023.

This does **not** establish full rank 38,801, nor does it exclude a still
meaningful factorization of rank 1,024 or several thousand.  Larger sketches
would only move the lower bound unless paired with a new structural upper
bound, so extending `m` after the declared 1,024 threshold is not the next
priority.

The next bounded Problem-B experiment is the distinct B2/E3 question: build a
fused joint within-band column frontier, verify every C=3/C=4 transition
against these explicit matrices, and measure whether a C=5 source can be
contracted without a nearly injective state per assignment prefix.  It must
have explicit state/record/time/RSS limits and is a falsification experiment,
not a C=6 run.
