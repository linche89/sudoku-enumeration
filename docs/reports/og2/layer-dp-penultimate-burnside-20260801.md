# Exact C=6 penultimate-layer Burnside count

Date: 2026-08-01

## Outcome

An independent exact counter closes the previously provisional C=6 layer-5
state-count question:

```text
M5 = 96,452,755 exactly.
```

The existing fixed capacity of 250,000,000 is 2.59194 times the real-state
count.  It leaves 153,547,245 slots, or 159.19% of `M5`, for parallel
insertion holes and operational margin.  The cap is retained rather than
reduced.

This does not generate layer 4 or layer 5, compute `T_5`, or run a complete
C=6 count.

## Construction

At layer `C-1`, exactly two native occurrence masks miss each coordinate
pair.  The counter represents those masks as an unordered pair of binary
side-choice words.  For each signed box permutation in `C2 wr S_C`, it:

1. propagates a local pair around each box-permutation cycle;
2. retains choices that close exactly;
3. combines cycle contributions with a degree-vector DP;
4. applies Burnside's lemma over signed conjugacy classes.

The reachability step is exact: every balanced mask multiset gives a regular
bipartite incidence graph, and regular bipartite graphs decompose into the
row perfect matchings required by the layer DP.

The audited derivation is in
`../../math/penultimate-layer-burnside.md`.

## Exact output

Command:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass `
  -File scripts\build_layer_dp.ps1
.\build\layer_penultimate_burnside.exe
```

Output:

```text
penultimate C=2 layer=1 signed_classes=5 group=8
  burnside_sum=8 M_1=1 expected=1 [OK]
penultimate C=3 layer=2 signed_classes=10 group=48
  burnside_sum=240 M_2=5 expected=5 [OK]
penultimate C=4 layer=3 signed_classes=20 group=384
  burnside_sum=20736 M_3=54 expected=54 [OK]
penultimate C=5 layer=4 signed_classes=36 group=3840
  burnside_sum=65740800 M_4=17120 expected=17120 [OK]
penultimate C=6 layer=5 signed_classes=65 group=46080
  burnside_sum=4444542950400 M_5=96452755
PENULTIMATE-LAYER BURNSIDE GATES PASSED
```

The C=6 computation took 2.070 seconds on the reference workstation.

## Verification

The C=2..5 values were independently reproduced by the existing forward
layer-DP implementation.  In particular, a fresh C=5 staged run constructed
all 17,120 four-row states.

The counter was added to `scripts/verify_layer_dp.ps1`.  The production
engine was also given a penultimate-layer boundary check for C=2..6, so its
ordinary C=3..5 forward gates execute the same acceptance path that a future
C=6 layer-5 snapshot must pass.

The complete repository gate passed after these changes:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts\verify_all.ps1
```

```text
exit code: 0
wall time: 197.9 seconds
ALL REPOSITORY CHECKS PASSED
```

Functional commits:

```text
189d605 layer-dp: count penultimate layer exactly
34c298f layer-dp: enforce penultimate state anchor
```

## Decision

The layer-5 real-state capacity gate is closed exactly.  A completed C=6
layer-5 snapshot now has an independent mandatory key-count anchor:

```text
real_size == 96,452,755.
```

The next bounded step is the staged allocation/checkpoint/restart rehearsal.
It must measure parallel insertion holes and actual RAM/disk behavior; the
exact orbit count does not by itself certify those operational effects.
