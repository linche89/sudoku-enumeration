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
