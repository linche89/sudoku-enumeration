# Exact raw-state response quotient for native reverse gluing

## Result

This construction satisfies route 1 of the requested alternatives.

For a raw four-row native state `U`, define its complete-output response vector

\[
\rho(U)_q := J_{4,1}(q;U,b)
\]

at `C=5`, where `b` is the unique one-row state and `q` ranges over the 355 complete coordinate orbits.  The vector is computed directly from the raw state, without finding a canonical four-row representative, a stabilizer `s_U`, or an orbit size `m_U`.

The implementation interns equal response vectors and fills an exact raw-key-to-response table by orbit expansion.  Consequently every subsequent `2+2` leaf performs one packed-key lookup and one integer accumulation.  A full C=5 run found

```text
four-row coordinate orbits  = 17,120
response quotient classes   = 16,917
merged orbit pairs           = 203
largest response class       = 2 coordinate orbits
```

and reproduced all 355 complete outputs and

```text
N(5) = 1903816047972624930994913280000.
```

The quotient is only slightly smaller than the coordinate-orbit set.  The arithmetic reduction comes from computing a response only once per previously unseen coordinate orbit and replacing per-leaf coordinate canonicalization by exact table lookup.  The method trades memory for arithmetic; it is not a storage reduction.

## Native C=5 response formula

Let `G=G_5=C_2 wr S_5`, `L=|G|=3840`.  A four-row mask occupies four coordinate pairs and therefore misses one pair.  For every pair `p`, exactly two masks of `U` miss `p`; call this multiset

\[
M_p(U)=\{u_{p,0},u_{p,1}\}.
\]

The one-row state supplies singleton masks `e_{p,0}` and `e_{p,1}`.  A native `4+1` contingency pairs the two elements of `M_p(U)` with these two singletons.  There are two choices when the two masks are different and one choice when they are equal.

Let `E(U)` be the Cartesian product of these five local choices.  For `epsilon in E(U)`, let `Q_epsilon` be the resulting raw complete multiset and let `c_z(epsilon)` be its full-mask multiplicities.  Every contingency cell is zero or one, hence the denominator in the native coefficient is one and

\[
K_c(\epsilon)=\prod_z c_z(\epsilon)!.
\]

If `chi(Q)` is the complete-output orbit id, then

\[
\boxed{
\rho(U)_q=
\sum_{\epsilon\in E(U)}
\left(\prod_z c_z(\epsilon)!\right)
\mathbf 1_{\chi(Q_\epsilon)=q}.
}
\]

This has at most `2^5=32` raw completion records.  It uses only the raw four-row masks and the final complete-output orbit index.

Equivariance and invariance of the one-row state give

\[
\rho(gU)=\rho(U)\qquad(g\in G).
\]

Thus equality of these sparse vectors is an exact sufficient response quotient.

## Raw key and no-canonicalization classifier

For each missing pair `p`, delete that coordinate from the two masks in `M_p(U)`.  The remaining side choices are two 4-bit patterns `a_p,b_p`.  Store their unordered triangular rank in one byte.  The five bytes form a 40-bit raw key.

The classifier is built lazily:

```text
response_id(U):
    key = pack_raw(U)
    if raw_to_response contains key:
        return raw_to_response[key]

    signature = exact_rho_from_at_most_32_completions(key)
    id = intern_by_exact_signature(signature)

    for g in G_5:
        raw_to_response[transform_packed_key(key,g)] = id

    return id
```

No minimum over the group, transporter, `s_U`, or `m_U` is computed.  Hashing is only an index: equality of all sparse `(q,coefficient)` pairs is checked exactly, and insertion aborts if one raw key is ever associated with two response ids.

Inductively, a table miss occurs exactly once for every coordinate orbit: all images of every prior miss were inserted when that miss was processed.  Distinct coordinate orbits with equal response signatures are merged by the signature interner.

## Exact native normalization

Let `x,y` be two-row orbit representatives, `H_x=Stab(x)`, and let `D` run over `H_x`-orbits of the distinct coordinate images of `y`.  Write

\[
\omega_D=|H_x\cdot y_D|.
\]

For an unordered source pair use `d_xy=2-delta_xy`.  The exact fused result is

\[
F_5(q)=
\sum_{x\le y}d_{xy}F_2(x)F_2(y)
\frac{s_q}{s_x}
\sum_D\omega_D
\sum_{n\in\mathcal N_{2,2}(x,y_D)}
K_c(n)\rho(U(n))_q.
\]

The intermediate stabilizer cancels because the `2+2` coefficient contains `s_U`, while the invariant-one-row `4+1` coefficient is `s_q rho(U)_q/s_U`.  Enumerating distinct images of `y` cancels `s_y`.

The code uses the common integer scale `L=|G|`.  For every response class `r`, it accumulates

\[
A_r=
\sum_{x\le y,D,n:\rho(U(n))=r}
 d_{xy}F_2(x)F_2(y)
 \frac{L}{s_x}\omega_D K_c(n).
\]

All factors here are integers.  The final per-concrete-representative value is

\[
\boxed{
F_5(q)=\frac{s_q}{L}\sum_r A_r\rho_r(q).
}
\]

The implementation checks every final division exactly.  It then uses

\[
m_q=L/s_q,
\qquad
\ell(q)=10!/\prod_w c_w!,
\]

and computes the required square

\[
\boxed{
N(5)=\sum_q m_q\ell(q)F_5(q)^2.
}
\]

No symbol-labelled orbit total is substituted for `F_5(q)`.

## Measured C=5 work

The complete executable run reported:

```text
relative placements                           3,658,027
2+2 contingency leaves                      122,166,792
raw quotient lookups                         122,166,792
raw lookup probes                            175,696,804
coordinate-orbit misses                           17,120
response quotient classes                         16,917
raw four-row keys                            62,185,328
orbit-image insertion attempts               65,740,800
raw insertion probes                         94,179,451
4+1 signature completion records                462,403
stored nonzero response terms                   375,453
raw table bytes                            1,073,741,824
wall time                                          115.43 s
peak RSS                                      1,099,840 KiB
```

The previous leaf-by-leaf fusion would emit 3,568,790,481 `4+1` completion records.  This quotient implementation emits only 462,403 records while constructing signatures, a factor of approximately 7,718 fewer.  It replaces 95,227,539 cold four-row canonicalization requests by 17,120 exact quotient misses plus 65,740,800 packed orbit-image transforms.  The 122,166,792 contingency leaves themselves are unchanged.

## C=4 gate

At C=4 the response is already complete after `2+2`; the response quotient degenerates to the one-hot complete-output id.  The attached exact gate constructs the full `26 x 276` matrix and reports

```text
two-row orbits                  23
symmetric source columns       276
complete outputs                26
relative concrete images    24,576
contingency leaves           10,952
matrix nonzeros               1,068
rational rank                    26
N(4)             29136487207403520
wall time                       5.45 s
peak RSS                     303,912 KiB
```

It verifies `F_4(2a)=4F_4(a)` and `N_4(2a)=16N_4(a)`.

## Independent C=5 per-class gate

A separate program reads the 355 output triples and recomputes each `F_5(q)` by recursively removing perfect matchings from the corresponding 5-regular bipartite graph.  It shares no response-quotient or reverse-gluing transition code.  It reported

```text
classes                         355
all F5 values match               1
memo states              24,057,191
perfect-matching transitions
                         2,280,793,494
N(5)       1903816047972624930994913280000
wall time                    11:54.84
peak RSS                   1,398,616 KiB
```

## Exact C=6 response extension

The same response idea has a direct raw-state formula for completing four rows by two rows.

Temporarily distinguish the twelve occurrences of a raw four-row state `U`.  Each occurrence mask misses two coordinate pairs.  For every pair `p`, exactly four occurrences miss `p`.  Let `Sigma(U)` be the side assignments that choose two of those four occurrences for side 0 and the other two for side 1.  Therefore

\[
|\Sigma(U)|=\binom42^6=6^6=46,656.
\]

For `sigma in Sigma(U)`, the two selected missing vertices of every occurrence form a concrete two-row mask.  The resulting graph `Y_sigma` is 2-regular; let `kappa(Y_sigma)` be its number of cycle components.  Let `Q_sigma` be the completed raw state, let `a_u` be the multiplicities in `U`, and let `c_z(sigma)` be the final multiplicities.  Then the exact native response is

\[
\boxed{
\rho_6(U)_q=
\frac{1}{\prod_u a_u!}
\sum_{\sigma\in\Sigma(U)}
2^{\kappa(Y_\sigma)}
\left(\prod_z c_z(\sigma)!\right)
\mathbf 1_{\chi(Q_\sigma)=q}.
}
\]

To prove the normalization, group labelled assignments by their native contingency table `n_(u,v)`.  A table has `prod_u a_u!/prod_(u,v)n_(u,v)!` labelled realizations.  Multiplication by the displayed per-assignment factor gives exactly

\[
2^{\kappa(Y)}
\frac{\prod_z c_z!}{\prod_{u,v}n_{u,v}!}
=F_2(Y)K_c(n).
\]

Thus the formula sums the actual two-row vector directly and avoids a separate `4+2` traversal over the 772 two-row orbits.  It also requires no four-row stabilizer.

The same scaled accumulation gives

\[
F_6(q)=\frac{s_q}{46,080}\sum_r A_r\rho_{6,r}(q),
\qquad
N(6)=\sum_q\frac{46,080}{s_q}\ell(q)F_6(q)^2.
\]

## C=6 projection

This construction does **not** remove the initial `2+2` double-coset wall.  Let

- `P_6` be the number of initial relative placements,
- `L_6` the number of initial contingency leaves,
- `M_6` the number of four-row coordinate orbits encountered,
- `R_6` the number of raw four-row keys,
- `Z_6` the retained sparse response terms.

The direct projected operation count is

\[
O\bigl(P_6+L_6+M_6(46,080+46,656)+Z_6\bigr),
\]

with one quotient lookup per contingency leaf.  A packed C=6 raw-key table needs about `32 R_6` bytes at load factor one half because a key occupies 96 bits.

The known 100,000-leaf prefix already contains 99,423 distinct coordinate orbits.  On those orbits alone this implementation would execute

```text
99,423 * 46,080 = 4,581,411,840 orbit-image insertion attempts
99,423 * 46,656 = 4,638,679,488 exact two-row response assignments
```

before processing later four-row orbits.

A lazy complete-output table has at most

\[
63,199\cdot46,080=2,912,209,920
\]

raw keys; a 16-byte key/value slot at load factor one half gives an 86.8 GiB worst-case table bound.

The exact trivial-stabilizer subset already contains 1,761,454,080 initial placements.  Using the three measured sample leaf counts per placement only as scenarios—not as lower bounds—gives

```text
2,662 leaves/placement   ->  4.689e12 leaves
15,360 leaves/placement  ->  2.706e13 leaves
27,793 leaves/placement  ->  4.896e13 leaves
```

At the measured C=5 rate of about 1.09 million leaves/second, those scenarios correspond to roughly 49.8, 287.3, and 519.9 single-core days for that trivial-stabilizer subset alone.

Therefore the response quotient is a real C=5 arithmetic reduction and supplies an exact local C=6 `4+2` response formula, but its C=6 projection is not scale-passing because the initial `2+2` incidence and raw-key inventories remain too large.

## Reproduction

C=4:

```bash
python native_reverse_gluing_c4_response_gate.py
```

C=5 main construction:

```bash
g++ -O3 -march=native -std=c++20 -DNDEBUG \
  native_c5_response_quotient.cpp -o native_c5_response_quotient
/usr/bin/time -v ./native_c5_response_quotient \
  native_c5_response_quotient_triples.csv \
  native_c5_response_quotient_signatures.csv
```

Independent C=5 output verification:

```bash
g++ -O3 -march=native -std=c++20 -DNDEBUG \
  native_c5_triples_independent_verify.cpp \
  -o native_c5_triples_independent_verify
/usr/bin/time -v ./native_c5_triples_independent_verify \
  native_c5_response_quotient_triples.csv
```
