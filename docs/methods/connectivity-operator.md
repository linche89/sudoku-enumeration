# Connectivity operator and subset-DP decision

`experiments/proto/connectivity_operator.cpp` is a Windows decision prototype
for the exact symbol-synchronous two-copy connectivity recurrence and its
proposed operator-valued double-permanent transition.  It is verified
completely through C=4 and has bounded C=5 measurements.  It is not part of
the standard build and is hard-limited to C<=5.

## Connectivity state

For every band, the state stores the degree of each left and right color
vertex and pairs the endpoints of every unfinished path.  Adding one edge can
extend or join paths.  Closing a cycle deletes that path component and
multiplies the coefficient by two.  The global state is canonicalized under
simultaneous band, left-color, and right-color permutations, with optional
exchange of the two copies.

For a fixed canonical source, the direct one-symbol transition enumerates one
left-color permutation and one right-color permutation.  The raw target keeps
the bands labelled until the transition is complete; only then is it reduced
to the global connectivity orbit.  This direct path is the independent
coefficient oracle for the subset transition.

## Proposed subset transition

The subset implementation processes the C labelled bands in order.  Its key
contains

```text
(partial raw target, used-left-color mask, used-right-color mask).
```

At band `i`, it chooses one unused left color and one unused right color,
updates the exact path state, and merges identical keys.  This is the natural
two-subset realization of the proposed operator-valued double permanent.

It is exact, but it has no internal compression in these coordinates.  Fix a
source state `S` and let `T_k` be a partial target after bands `0..k-1` have
been processed.  In every processed band `i`, the difference between the
degree vectors of `T_k` and `S` is exactly

```text
+1 at the chosen left color a_i,
+1 at the chosen right color b_i,
 0 at every other vertex.
```

Consequently `T_k` alone recovers every pair `(a_i,b_i)` in the assignment
prefix.  Two different legal prefixes cannot have the same partial raw
target, regardless of live-path pairing or cycle deletion.  The used masks
are redundant for identity and cannot create merging.  Thus at every band

```text
number of subset frontier states = number of legal assignment prefixes.
```

This is an injectivity statement for the implemented labelled transition,
not merely an empirical observation.

## Exact gates

The direct and subset transitions agree per source and per labelled raw
target through complete C=2..4.  Both give the known totals and connectivity
layers:

```text
C=2: 1, 1, 3, 1, 1
C=3: 1, 1, 8, 18, 19, 1, 1
C=4: 1, 1, 28, 700, 12856, 9708, 155, 1, 1
```

Disabling copy exchange preserves `N(4)` and changes the C=4 layers to

```text
1, 1, 43, 1290, 25046, 18775, 234, 1, 1.
```

An optimized C=5 two-symbol initializer was also compared key by key and
coefficient by coefficient with the intentionally slow direct construction;
both contain 93 states.

## Bounded C=5 result

The complete third-symbol transition is:

```text
source states                 = 93
target connectivity states    = 83776
valid permutation pairs       = 857244
raw target support            = 857244
sum per-source canonical      = 273518
subset generated records      = 2056501
subset merged prefixes        = 0
maximum single-source frontier= 14400
```

For the next transition, 300 deterministic sources were sampled in three
100-source intervals beginning at indices 0, 40,000, and 80,000.  Across all
300 sources there were 1,442,763 legal placements, 1,442,763 raw targets, and
zero merged subset prefixes in all 1,500 band-prefix checks.  Reducing each
completed target by the full connectivity canonicalizer still retained
1,434,260 per-source targets, or 99.410645% of raw support.

## Boundary

The naive subset double-permanent implementation is rejected as a compression
mechanism.  It replaces a Cartesian permutation-pair loop by an injective
enumeration of the same legal assignments and therefore cannot repair the
83,776-state C=5 wall.

Canonicalizing only the partial target under the full group would be
incorrect because the same action also moves the fixed source and the
processed-band convention.  A different exact route would need, for example,
a source-stabilizer quotient of the joint
`(source, partial target, masks, prefix)` object or a genuinely transformed
operator that sums without materializing labelled partial targets.  No such
compact operator is currently known.  This experiment does not reject every
possible double-permanent transform or the connectivity recurrence itself.

Commands, hashes, bounded measurements, and the decision record are in
`../reports/og2/connectivity-double-permanent-20260719.md`.
