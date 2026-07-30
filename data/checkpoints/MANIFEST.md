# Checkpoint Manifest

## factorization_orbit_c6_graphmemo.bin

```text
format magic: FJFAC01
size: 217954462 bytes
entries: 5315962
sha256: FE8B68DE6C15592848D7CF69BF43928C000F0B2EE59FC263A094BCA2D376A865
compatible implementation: 0713d27 and descendants
```

Coverage:

- C=6 outer class 1 is complete:
  `F6(G1) = 6986348258918400`.
- Outer class 2 contains its first 50 newly evaluated degree-5 values.
- Every stored value is closed and exact; no partial accumulator is trusted.

Local safety backup created before repository reorganization:

```text
E:\Code\sudoku_FJ_backup\factorization_orbit_c6_graphmemo_20260712_pre_reorg.bin
```

Both copies had the SHA-256 value above when the reorganization began.

Verification must pass `checkpointreadonly`; ordinary runs save atomically via
a temporary file and replace operation.

## layer_dp_c6_layer3_20260731.snap

```text
format magic: LDPCAN01
format version: 2
size: 443695556 bytes
stored entries: 12324873
closed states: 12324872
parallel insertion holes: 1
orbit mass: 566455903200
sha256: 1D882DB7B0B18490E981539F7C690FA90227ECEC243817F5B538BEC8DAC865B7
compatible implementation: 78835bf descendants with unchanged LDPCAN01 key semantics
```

The bounded S0 run completed both transitions exactly:

```text
1->2 emissions = 59245120
layer-2 states = 772
layer-2 orbit mass = 20338525
2->3 emissions = 2605194602
layer-3 states = 12324872
layer-3 orbit mass = 566455903200
```

It ran under a 2 GiB / 15 minute external guard, took 521.3 seconds, and
peaked at 661.1 MiB.  A read-only load/round-trip reproduced the stored state
count and orbit mass.

External physical backup:

```text
D:\sudoku_FJ_checkpoint_backups\layer_dp_c6_layer3_20260731.snap
```

Source and backup had the SHA-256 value above after copying.
