# Layer-DP final certificate verification

## Status

`experiments/proto/s4_certificate_verify.py` is an independent semantic
verifier for the final layer-DP CSV.  It has passed a complete C=5 gate on all
355 classes, rejects a deliberately falsified orbit size, and independently
constructs the C=6 G1/G2 canonical witnesses and stabilizers.  No complete
C=6 CSV exists yet, so the 63,199-row application of this verifier remains a
future production gate.

The verifier does not read a checkpoint and does not call or link the C++
engine.  Its witness is the `representative_words` field in each CSV row.

## Certificate schema

The accepted header is exactly:

```text
qid,representative_words,coordinate_orbit_size,labelled_multiplicity,F
```

For a C-column band, `representative_words` contains `2C` sorted integers in
`[0,2^C)`.  Bit `b` selects the side of the symbol in box coordinate `b`.
Legality requires exactly C zero and C one occurrences in every coordinate.

## Independent canonical and stabilizer calculation

The coordinate group is `C2 wr S_C`, of order `2^C C!`.  The verifier uses a
definition-level prefix search written in Python:

1. begin with all word occurrences in one ordered partition cell;
2. choose an unused source coordinate and either orientation;
3. split every current cell into its side-0 then side-1 subcells;
4. compare the resulting fixed-width binary prefix signature;
5. retain every signed coordinate choice attaining the global minimum and
   repeat for the next destination coordinate.

The terminal minimum determines the required representative.  The number of
surviving signed coordinate actions is exactly the stabilizer order `s`, so
the verifier derives rather than trusts:

```text
m = 2^C C! / s.
```

For the word multiplicities `c_w`, it independently derives:

```text
ell = (2C)! / product_w c_w!.
```

It then requires the supplied `m` and `ell` to equal these values.  Canonical
representatives must be unique.  Contiguous qids must also follow the exact
little-endian packed-state byte order used for deterministic engine output.

## Bound C=6 anchors

Starting from the audited G1/G2 side-incidence masks, the independent search
obtains:

```text
G1 representative = 0 5 10 19 29 30 39 43 44 48 54 57
G1 stabilizer      = 120
G1 orbit size m    = 384
G1 required F      = 6986348258918400

G2 representative = 0 5 10 23 27 28 35 44 47 48 54 57
G2 stabilizer      = 8
G2 orbit size m    = 5760
G2 required F      = 7053808087203840
```

The final C=6 check looks up these exact canonical representatives and binds
each F value to its row.  Merely finding the two numbers somewhere in the F
column is insufficient.

## Global checks and independent sums

The semantic verifier additionally requires:

```text
class count = 63199                         (at C=6)
sum(m ell)  = binomial(2C,C)^C
N(C)        = sum(m ell F^2)                (Python arbitrary precision)
```

That last calculation is a useful cross-check, but it is not treated as the
only final summation implementation.  Production finalization also runs:

- `experiments/proto/s4_exact_sum.py`; and
- `experiments/proto/s4_exact_sum.ps1`, using .NET `BigInteger` and an
  independent CSV/parser implementation.

Both totals must agree exactly.  The primary and replay CSVs must first be
byte-identical and have the same SHA-256.

## Gates

The independent C=6 anchor construction is cheap:

```powershell
python experiments\proto\s4_certificate_verify.py `
  --self-test-c6-anchors
```

The complete C=5 regression is exercised by `scripts/verify_layer_dp.ps1`.
The future final C=6 invocation is frozen as:

```powershell
python experiments\proto\s4_certificate_verify.py FINAL.csv `
  --c 6 --classes 63199 `
  --expect-n 38296278920738107863746324732012492486187417600000 `
  --quiet
```

This command is a verifier for a completed artifact, not authorization to
construct that artifact.
