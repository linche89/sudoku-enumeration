# C=5 controlled inner-engine ablation

Date: 2026-07-12 (Asia/Singapore)

## Purpose

This run isolates the two mathematical shortcuts used by the C=5 manuscript:
the least-frequency rooted edge pivot and the direct rooted degree-four
two-factor split.  It replaces comparisons between different historical code
versions with a same-binary, same-machine ablation.

## Provenance

- source commit: `d86cd88383da915e33caf8e361cfb193134953dc`
- source: `src/factorization_orbit.cpp`
- source SHA-256:
  `A19A951E613E5305A1E8A4E34A9CF5CC52FF76FF7B512819D5D7103E67691EA8`
- compiler: MinGW-w64 GCC 13.2.0
- platform: Windows 11 Pro build 26100
- CPU: AMD Ryzen 9 9950X3D, 16 cores / 32 logical processors

The standard `scripts/build_og2.ps1` build was run once.  Four clean processes
then used the resulting executable sequentially:

```powershell
.\build\factorization_orbit.exe 5
.\build\factorization_orbit.exe 5 pivot
.\build\factorization_orbit.exe 5 rooted4
.\build\factorization_orbit.exe 5 pivot rooted4
```

Every process reproduced

```text
1903816047972624930994913280000 [OK]
```

and independently passed the outer mass check.

## Results

The timings below are factor-stage times.  The common outer generation took
0.77--0.88 seconds and is excluded from the speedup column.

| inner method | factor time | speedup | perfect matchings | graph calls | canonicalization computations |
|---|---:|---:|---:|---:|---:|
| plain recurrence | 53.348169 s | 1.00x | 14,191,134 | 5,820,174 | 7,933,335 |
| edge pivot only | 36.927065 s | 1.44x | 3,256,904 | 1,999,603 | 2,471,437 |
| degree-four split only | 11.480672 s | 4.65x | 2,181,504 | 357,320 | 1,426,850 |
| edge pivot + degree-four split | 8.577403 s | 6.22x | 427,548 | 119,959 | 320,221 |

The combined run made 14,122 degree-four calls and visited 40,488,962 rooted
two-factor leaves.  Those leaves are cheap cycle-weight evaluations and do not
construct or canonicalize cubic residual graphs.

## Interpretation

The direct degree-four identity is the dominant improvement: it removes the
entire cubic residual layer.  The edge pivot composes with it by reducing the
top-level matching list from 2,181,504 to 427,548, a factor of 5.10.  The
combined controlled speedup relative to the current plain recurrence is 6.22x.

The earlier 254.996-second first prototype remains useful development history,
but it is not used as a controlled algorithmic baseline because later versions
also changed data structures and canonicalization.

## Raw logs

Transient ignored logs are under `data/logs/paper-ablation-20260712/`.
Their SHA-256 digests are:

```text
F4E74F73DDC1561FF368CFA52CBF84C12E4D31E6CDC1D93855FF41F6DBEE929C  plain.out
797E01150CA75F1755FDCA5CE99C0361D37A1248F29EDF0934BD328976D00B43  plain.err
849ECA2C6FBD2A462AFCEC9B21858E7D078135A8623C07E861F9D9CED875B6A1  pivot.out
B7B28C14EEF0E33E73F08585E49436E97F2E1A96AAF055899C3258852583BA03  pivot.err
CFCD19B5B02290596AD1F0B60A891C835F25E046DFBA85D16AE08C33328A85AA  rooted4.out
CCFA1BB752B353650462C7F7AC29D04180120F8EF95F78E04E31A9D2CAD07AB7  rooted4.err
6C493A3F5633765F53EE3D7C305E419A694C51FAB94875CA4863B1700DF55703  pivot-rooted4.out
C1F7BB5E15636E0B73EED28AE8AE7DB379AE764D343E8AD6DC38A4081863E37A  pivot-rooted4.err
```
