# Constructive result: an exact raw-state response quotient

I found and implemented a construction satisfying **route 1**.

For a raw four-row state (U), the algorithm computes its complete-output response

[
\rho(U)=\bigl(J_{4,1}(q;U,b)\bigr)_q
]

directly, without finding a canonical four-row representative, (s_U), or (m_U). Equal response vectors are interned, and all coordinate images of a newly encountered raw state are inserted into a packed lookup table. Subsequent (2+2) leaves require only one raw-key lookup and one exact integer accumulation.

This uses the requested native, symbol-quotiented convention and returns (F_C(q)) for one concrete representative; (m_q\ell(q)) enters only in the final square. 

The new C=5 measurements are:

[
\boxed{#\rho\text{-classes}=16{,}917}
]

from 17,120 four-row coordinate orbits. Exactly 203 response classes contain two coordinate orbits; no response class contains more than two.

The code reproduces all 355 complete C=5 outputs and

[
\boxed{N(5)=1903816047972624930994913280000}.
]

The 16,917-class quotient and the run measurements below are new results from the attached implementation, not yet part of the authoritative project state.

---

## 1. Exact C=5 response quotient

For a four-row state (U), every occurrence mask occupies four of the five coordinate pairs, so it misses exactly one pair. For each pair (p), precisely two masks of (U) miss (p); call this multiset

[
M_p(U)={u_{p,0},u_{p,1}}.
]

The one-row state (b) supplies the two singleton masks (e_{p,0}) and (e_{p,1}).

For each (p):

* there are two native contingency choices when (u_{p,0}\ne u_{p,1});
* there is one when they are equal.

Let (E(U)) be the Cartesian product of these five local choices. Thus

[
|E(U)|\le 2^5=32.
]

For (\epsilon\in E(U)), let (Q_\epsilon) be the resulting raw complete state and (c_z(\epsilon)) its complete-mask multiplicities. Every (4+1) contingency cell is zero or one, so its native coefficient is

[
K_c(\epsilon)=\prod_z c_z(\epsilon)!.
]

Let (\chi(Q)) be the complete-output coordinate-orbit identifier. Define

[
\boxed{
\rho(U)_q
=========

\sum_{\epsilon\in E(U)}
\left(\prod_z c_z(\epsilon)!\right)
\mathbf 1_{\chi(Q_\epsilon)=q}.
}
\tag{1}
]

This is computed from the raw masks of (U). It does not require a canonical (U), a transporter, a stabilizer, or an intermediate orbit size.

Because the one-row state is fixed by all of (G_5), incidence equivariance gives

[
\boxed{\rho(gU)=\rho(U)\qquad(g\in G_5).}
\tag{2}
]

Equality of the sparse vectors in (1) is therefore an exact sufficient response equivalence.

### Raw encoding

For each missing coordinate (p), delete (p) from the two masks in (M_p(U)). The remaining side selections are two four-bit patterns (a_p,b_p). Their unordered triangular rank lies in ([0,135]) and fits in one byte.

The five bytes give a 40-bit raw key. A coordinate transformation of this key requires only five precomputed pair-code transformations; no masks are sorted and no group minimum is sought.

---

## 2. Lazy quotient construction

The exact classifier is:

```text
response_id(U):
    key = pack_raw(U)

    if raw_to_response contains key:
        return raw_to_response[key]

    signature = exact rho(U) from at most 32 complete outputs
    id = intern signature using exact sparse-vector equality

    for g in G5:
        raw_to_response[transform_packed_key(key, g)] = id

    return id
```

The hash is only an index. Sparse signatures are compared entry by entry, and the implementation aborts if a raw key is ever associated with inconsistent response identifiers.

Inductively, there is exactly one table miss per coordinate orbit: all images of every earlier miss were inserted immediately. Nonconjugate coordinate orbits are nevertheless merged when their exact (\rho)-vectors agree.

For C=5 this produced:

```text
coordinate orbits                         17,120
response quotient classes                16,917
classes containing multiple orbits          203
maximum coordinate orbits per response        2
```

Thus the quotient is genuine, although only slightly smaller than the coordinate-orbit space.

---

## 3. Exact native normalization

Let (x,y) be two-row representatives, (H_x=\operatorname{Stab}(x)), and let (D) range over (H_x)-orbits of distinct coordinate images of (y). Write

[
\omega_D=|H_x\cdot y_D|.
]

Let

[
d_{xy}=2-\delta_{xy}
]

be the unordered (2+2) source-pair factor.

After cancelling the intermediate stabilizer and cancelling (s_y) by enumerating distinct images, the exact fused recurrence is

[
\boxed{
F_5(q)=
\sum_{x\le y}
d_{xy}F_2(x)F_2(y)
\frac{s_q}{s_x}
\sum_D \omega_D
\sum_{n\in\mathcal N_{2,2}(x,y_D)}
K_c(n)\rho(U(n))_q.
}
\tag{3}
]

The cancellation follows directly from:

[
\text{(2+2) coefficient}\propto s_U,
\qquad
\text{(4+1) coefficient}
========================

\frac{s_q}{s_U}\rho(U)_q.
]

No (s_U) or (m_U) survives.

The implementation keeps all accumulation integral by using

[
L=|G_5|=3840.
]

For every response class (r), it accumulates

[
\boxed{
A_r=
\sum_{\substack{x\le y,D,n\\rho(U(n))=r}}
d_{xy}F_2(x)F_2(y)
\frac{L}{s_x}\omega_D K_c(n).
}
\tag{4}
]

Then

[
\boxed{
F_5(q)=\frac{s_q}{L}\sum_r A_r\rho_r(q).
}
\tag{5}
]

Every division in (5) is checked exactly.

Finally,

[
m_q=\frac{L}{s_q},
\qquad
\ell(q)=\frac{10!}{\prod_w c_w!},
]

and the squared objective is

[
\boxed{
N(5)=\sum_q m_q\ell(q)F_5(q)^2.
}
\tag{6}
]

Thus the algorithm preserves the complete (q)-indexed vector until the final square. It is not a one-copy marginal or an orbit-total substitution.

---

## 4. C=5 arithmetic measurement

The authoritative baseline has 3,658,027 relative placements, 122,166,792 contingency leaves, and 95,227,539 cold four-row canonicalizations. 

The new exact run reported:

| Operation or resource                      |                Measured value |
| ------------------------------------------ | ----------------------------: |
| Relative placements                        |                     3,658,027 |
| (2+2) contingency leaves                   |                   122,166,792 |
| Raw quotient lookups                       |                   122,166,792 |
| Raw lookup probes                          |                   175,696,804 |
| Coordinate-orbit misses                    |                        17,120 |
| Response quotient classes                  |                        16,917 |
| Raw four-row keys installed                |                    62,185,328 |
| Packed orbit-image insertion attempts      |                    65,740,800 |
| Raw insertion probes                       |                    94,179,451 |
| (4+1) records used to construct signatures |                   **462,403** |
| Stored nonzero response terms              |                   **375,453** |
| Packed raw table                           |           1,073,741,824 bytes |
| Wall time                                  |            **115.43 seconds** |
| Peak RSS                                   | **1,099,840 KiB = 1.049 GiB** |

The previous immediate fused traversal would emit 3,568,790,481 (4+1) records. The response quotient reduces that to 462,403 records, a factor of about

[
\boxed{7{,}718}.
]

It replaces 95,227,539 cold four-row canonicalization requests by:

* 17,120 exact response computations;
* 65,740,800 simple five-byte orbit transforms;
* one packed-table lookup per contingency leaf.

The 122,166,792 contingency leaves are unchanged. This is therefore an arithmetic reduction in output classification and continuation work, not merely removal of four-row storage.

It is a deliberate memory-for-arithmetic trade: the raw classifier uses one GiB, which is larger than the retained 17,120-state coefficient layer.

---

## 5. Complete C=4 gate

At C=4, the four-row state is already complete, so the response quotient degenerates to a one-hot complete-output identifier.

The attached exact gate constructed the complete (26\times276) matrix and measured:

| C=4 quantity             |       Result |
| ------------------------ | -----------: |
| Two-row orbits           |           23 |
| Symmetric source columns |          276 |
| Complete outputs         |           26 |
| Relative concrete images |       24,576 |
| Contingency leaves       |       10,952 |
| Matrix nonzeros          |        1,068 |
| Exact rational rank      |       **26** |
| Wall time                | 5.45 seconds |
| Peak RSS                 |  303,912 KiB |

It reproduces every (F_4(q)),

[
N(4)=29136487207403520,
]

and verifies

[
F_4(2a)=4F_4(a),
\qquad
N_4(2a)=16N_4(a).
]

Full row rank over (\mathbb Q) follows from a nonzero (26\times26) minor modulo (1{,}000{,}000{,}007).

---

## 6. Independent verification of every C=5 triple

The 355-row C=5 CSV was independently checked by a second program that does not use reverse gluing or the response quotient.

For each complete representative, it builds the corresponding 5-regular (10\times10) bipartite graph and recursively removes perfect matchings. It matched every reported (F_5(q)):

```text
classes                         355
all F5 values match               1
memo states              24,057,191
perfect-matching transitions
                         2,280,793,494
N5           1903816047972624930994913280000
wall time                    11:54.84
peak RSS                   1,398,616 KiB
```

This gives a complete per-class gate rather than only agreement of the final weighted sum.

---

# 7. Exact C=6 response formula

The response construction has a direct C=6 extension for completing four rows by the actual two-row vector without traversing the 772 two-row orbit classes separately.

Temporarily distinguish the twelve occurrences of a raw four-row state (U). Every occurrence mask misses two coordinate pairs. For each pair (p), exactly four occurrences miss (p).

Let (\Sigma(U)) consist of side assignments that choose two of these four occurrences for side 0 and the other two for side 1. Consequently,

[
\boxed{
|\Sigma(U)|=\binom42^6=6^6=46{,}656.
}
\tag{7}
]

For (\sigma\in\Sigma(U)):

* the two selected missing vertices of each occurrence form a concrete two-row mask;
* these masks form a 2-regular graph (Y_\sigma);
* (F_2(Y_\sigma)=2^{\kappa(Y_\sigma)});
* (Q_\sigma) is the completed raw state.

Let (a_u) be the multiplicities of masks in (U), and let (c_z(\sigma)) be the completed-mask multiplicities. Then

[
\boxed{
\rho_6(U)*q=
\frac{1}{\prod_u a_u!}
\sum*{\sigma\in\Sigma(U)}
2^{\kappa(Y_\sigma)}
\left(\prod_z c_z(\sigma)!\right)
\mathbf 1_{\chi(Q_\sigma)=q}.
}
\tag{8}
]

### Native-weight proof

Group the temporarily labelled assignments by their native contingency table (n_{u,v}). A table has

[
\frac{\prod_u a_u!}{\prod_{u,v}n_{u,v}!}
]

labelled realizations. Multiplying this by the per-assignment factor in (8) gives

[
2^{\kappa(Y)}
\frac{\prod_z c_z!}{\prod_{u,v}n_{u,v}!}
========================================

F_2(Y)K_c(n).
]

Thus (8) is exactly the native (4+2) response against the actual two-row vector. It uses neither a four-row stabilizer nor a traversal over the 772 two-row orbit representatives.

With (L=|G_6|=46{,}080), the same accumulation yields

[
F_6(q)=\frac{s_q}{46{,}080}\sum_r A_r\rho_{6,r}(q),
]

followed by

[
N(6)=
\sum_q
\frac{46{,}080}{s_q}\ell(q)F_6(q)^2.
]

No C=6 state generation or full run was performed.

---

## 8. Quantified C=6 projection

This construction eliminates a separate orbit-pair (4+2) join, but it **does not eliminate the initial (2+2) double-coset traversal**.

Let:

* (P_6) be initial relative placements;
* (L_6) be initial contingency leaves;
* (M_6) be encountered four-row coordinate orbits;
* (R_6) be raw four-row keys;
* (Z_6) be stored sparse response terms.

The direct operation bound is

[
\boxed{
O!\left(
P_6+L_6+
M_6(46{,}080+46{,}656)+
Z_6
\right),
}
\tag{9}
]

with one quotient lookup for every (2+2) leaf.

A C=6 raw four-row key needs 96 bits. A half-full open-addressing table with 16-byte slots therefore uses approximately

[
32R_6\ \text{bytes}.
]

The verified 100,000-leaf C=6 prefix already contained 99,423 distinct canonical four-row keys.  On those orbits alone, this implementation would perform

[
99{,}423\cdot46{,}080
=====================

4{,}581{,}411{,}840
]

orbit-image insertion attempts and

[
99{,}423\cdot46{,}656
=====================

4{,}638{,}679{,}488
]

exact two-row response assignments.

The complete-output classifier has at most

[
63{,}199\cdot46{,}080
=====================

2{,}912{,}209{,}920
]

raw coordinate images. At 16 bytes per slot and load factor one half, that is an 86.8-GiB worst-case table bound.

The exact trivial-stabilizer subset already forces 1,761,454,080 initial (2+2) placements.  Applying the three measured C=6 sample leaf counts per placement only as scenarios—not as lower bounds—gives:

|                Scenario | Leaves in the trivial-stabilizer subset | Single-core projection at the measured C=5 rate |
| ----------------------: | --------------------------------------: | ----------------------------------------------: |
|  2,662 leaves/placement |                    (4.689\times10^{12}) |                                       49.8 days |
| 15,360 leaves/placement |                    (2.706\times10^{13}) |                                      287.3 days |
| 27,793 leaves/placement |                    (4.896\times10^{13}) |                                      519.9 days |

Therefore the result is:

[
\boxed{
\text{C=5 arithmetic gate: passed.}
}
]

[
\boxed{
\text{Exact local C=6 (4+2) response: constructed.}
}
]

[
\boxed{
\text{Complete C=6 projection: not scale-passing because the initial
(2+2) incidence remains.}
}
]

It is nevertheless a constructive exact response quotient rather than another obstruction or leaf-by-leaf fused traversal.

## Files

The complete derivation, measurements, source, CSVs, and verifier are bundled here:

[Download the complete constructive bundle](./response_quotient_constructive_bundle.tar.gz)

[Detailed mathematical and implementation report](./response_quotient_constructive_result.md)

### C=4

[C=4 exact gate source](./native_reverse_gluing_c4_response_gate.py)

[Complete (26\times276) matrix](./native_c4_K4_matrix.csv)

[All C=4 output triples](./native_c4_complete_triples.csv)

[C=4 run output](./native_reverse_gluing_c4_response_gate.out)

[C=4 timing and RSS](./native_reverse_gluing_c4_response_gate.time)

### C=5 response quotient

[C=5 response-quotient source](./native_c5_response_quotient.cpp)

[C=5 run output](./native_c5_response_quotient_v3.out)

[C=5 timing and RSS](./native_c5_response_quotient_v3.time)

[All 355 ((q,m_q,\ell(q),F_5(q))) triples](./native_c5_response_quotient_triples.csv)

[All 16,917 exact response signatures](./native_c5_response_quotient_signatures.csv)

### Independent C=5 verification

[Independent factorization verifier source](./native_c5_triples_independent_verify.cpp)

[Independent verifier output](./native_c5_triples_independent_verify.out)

[Independent verifier timing and RSS](./native_c5_triples_independent_verify.time)

Bundle SHA-256:

```text
62320e99fb01c6e4825339b5eeceed4a083286a6c9b133b88c0705e06b20d354
```
