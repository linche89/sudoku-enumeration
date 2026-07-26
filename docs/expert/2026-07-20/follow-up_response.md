# Follow-up: repair the projected reverse-gluing construction while preserving the square                                                                                                                                                           
                                                                                                                                                                                                                                                      
  We independently reproduced and accept Sections 1–5 of your response, including                                                                                                                                                                     
                                                                                                                                                                                                                                                      
  dim V_(6,2) = 54,382,557                                                                                                                                                                                                                            
  dim V_(6,3) = 29,801,801,681                                                                                                                                                                                                                        
                                                                                                                                                                                                                                                      
  and the full-support theorem.                                                                                                                                                                                                                       
                                                                                                                                                                                                                                                      
  However, Section 6 appears inconsistent with the squared Sudoku objective.                                                                                                                                                                          
                                                                                                                                                                                                                                                      
  Under the verified reverse-gluing semantics, the 772-dimensional C=6 two-row vector a contains single-copy partial factorization counts. The exact maps have the form                                                                               
                                                                                                                                                                                                                                                      
  A : Sym^2(V2) -> V4                                                                                                                                                                                                                                 
  J : V4 x V2 -> V6                                                                                                                                                                                                                                   
  F6 = J(A(a,a),a)                                                                                                                                                                                                                                    
  N(6) = sum_q w(q) F6(q)^2                                                                                                                                                                                                                           
       = <F6, W F6>.                                                                                                                                                                                                                                  
                                                                                                                                                                                                                                                      
  Therefore, under a -> lambda a,                                                                                                                                                                                                                     
                                                                                                                                                                                                                                                      
  F6 -> lambda^3 F6                                                                                                                                                                                                                                   
  N(6) -> lambda^6 N(6).                                                                                                                                                                                                                              
                                                                                                                                                                                                                                                      
  But your proposed                                                                                                                                                                                                                                   

  T(u,v)(w) = B(A(u,v),w)                                                                                                                                                                                                                             
  N(6) = T(a,a)(a)                                                                                                                                                                                                                                    
                                                                                                                                                                                                                                                      
  is cubic if A and B are bilinear.                                                                                                                                                                                                                   
                                                                                                                                                                                                                                                      
  Please provide one of the following exact outcomes.                                                                                                                                                                                                 
                                                                                                                                                                                                                                                      
  ## 1. Resolve the discrepancy                                                                                                                                                                                                                       
                                                                                                                                                                                                                                                      
  If our interpretation is wrong, define B completely and prove that it:                                                                                                                                                                              
                                                                                                                                                                                                                                                      
  - preserves sum_q w(q)F(q)^2;                                                                                                                                                                                                                       
  - is genuinely bilinear;                                                                                                                                                                                                                            
  - does not depend on the unknown final vector F6 or on N(6) as a stored constant;                                                                                                                                                                   
  - passes the scaling test above.                                                                                                                                                                                                                    
                                                                                                                                                                                                                                                      
  Otherwise, please retract or correct the claims that the response dimension is 772 and that only                                                                                                                                                    
                                                                                                                                                                                                                                                      
  binom(774,3) = 76,981,524                                                                                                                                                                                                                           
                                                                                                                                                                                                                                                      
  cubic coefficients suffice.                                                                                                                                                                                                                         
                                                                                                                                                                                                                                                      
  ## 2. Give the correct projected object                                                                                                                                                                                                             
                                                                                                                                                                                                                                                      
  A natural corrected formulation seems to be                                                                                                                                                                                                         
                                                                                                                                                                                                                                                      
  R : Sym^3(V2) -> V6                                                                                                                                                                                                                                 
  F6 = R(a,a,a)                                                                                                                                                                                                                                       
  N(6) = <R(a,a,a), W R(a,a,a)>.                                                                                                                                                                                                                      
                                                                                                                                                                                                                                                      
  Equivalently, one may study the Gram operator
                                                                                                                                                                                                                                                      
  H = R^* W R                                                                                                                                                                                                                                         
                                                                                                                                                                                                                                                      
  on Sym^3(V2).                                                                                                                                                                                                                                       
                                                                                                                                                                                                                                                      
  Please determine:                                                                                                                                                                                                                                   
                                                                                                                                                                                                                                                      
  - the exact orbit-total normalization of R, W, and H;                                                                                                                                                                                               
  - whether preserving the square requires paired partial states;                                                                                                                                                                                     
  - the true response dimension or a rigorous bound for it;                                                                                                                                                                                           
  - whether R(a,a,a) or H(z,z) can be evaluated without materializing V4, all source-target pairs, or the known 1.761-billion relative placements;                                                                                                    
  - an explicit arithmetic, memory, and record bound for C=6.                                                                                                                                                                                         
                                                                                                                                                                                                                                                      
  Simply observing that a scalar response exists for the one fixed vector a is tautological and is not a construction.                                                                                                                                
                                                                                                                                                                                                                                                      
  ## 3. Required small-C gates                                                                                                                                                                                                                        
                                                                                                                                                                                                                                                      
  Before any C=6 data generation:                                                                                                                                                                                                                     
                                                                                                                                                                                                                                                      
  ### C=4                                                                                                                                                                                                                                             
                                                                                                                                                                                                                                                      
  Use the verified spaces                                                                                                                                                                                                                             
                                                                                                                                                                                                                                                      
  dim V2 = 23                                                                                                                                                                                                                                         
  dim V4 = 26 complete classes.                                                                                                                                                                                                                       
                                                                                                                                                                                                                                                      
  A correct construction must reproduce every F(q) and N(4), and must scale quartically when the two-row input is scaled.                                                                                                                             
                                                                                                                                                                                                                                                      
  ### C=5                                                                                                                                                                                                                                             
                                                                                                                                                                                                                                                      
  Fuse the verified 2+2+1 construction directly:                                                                                                                                                                                                      
                                                                                                                                                                                                                                                      
  dim V2 = 107                                                                                                                                                                                                                                        
  complete outputs = 355 classes.                                                                                                                                                                                                                     
                                                                                                                                                                                                                                                      
  Without storing a four-row orbit layer, reproduce every verified                                                                                                                                                                                    
                                                                                                                                                                                                                                                      
  (coordinate orbit, labelled multiplicity, F(q))                                                                                                                                                                                                     
                                                                                                                                                                                                                                                      
  triple and then compute sum_q w(q)F(q)^2.                                                                                                                                                                                                           
                                                                                                                                                                                                                                                      
  Report:                                                                                                                                                                                                                                             
                                                                                                                                                                                                                                                      
  - retained response channels;                                                                                                                                                                                                                       
  - nonzero coefficients or emitted records;                                                                                                                                                                                                          
  - contingency leaves and relative placements still traversed;                                                                                                                                                                                       
  - time and peak RSS;                                                                                                                                                                                                                                
  - comparison with the existing 122,166,792 leaves and 95,227,539 cold canonicalizations.                                                                                                                                                            
                                                                                                                                                                                                                                                      
  This gate must demonstrate arithmetic reduction, not only removal of four-row storage.                                                                                                                                                              
                                                                                                                                                                                                                                                      
  ## 4. If no construction exists                                                                                                                                                                                                                     
                                                                                                                                                                                                                                                      
  Please formulate a precise uniform model—such as a monotone/local tensor-network or separator circuit—and prove a lower bound for the actual scalar coefficient                                                                                     
                                                                                                                                                                                                                                                      
  [all variables] B_6^6                                                                                                                                                                                                                               
                                                                                                                                                                                                                                                      
  that cannot be defeated by hard-coding the known value.                                                                                                                                                                                             
                                                                                                                                                                                                                                                      
  No complete C=6 run is authorized; an exact construction, a C=4/C=5 implementation, or a model-specific lower bound is requested.

  ---

  ## Correction and retraction

Your interpretation is correct. **Section 6 of my previous response was wrong and should be retracted.**

The project objective is the weighted square

[
N(6)=\sum_q w(q)F_6(q)^2,
]

not a linear functional of the single-copy vector (F_6). 

If (A) and (B) were bilinear and

[
T(u,v)(w)=B(A(u,v),w),
]

then

[
T(\lambda a,\lambda a)(\lambda a)
=\lambda^3T(a,a)(a).
]

But the required quantity satisfies

[
F_6(\lambda a)=\lambda^3F_6(a),\qquad
N_6(\lambda a)=\lambda^6N_6(a).
]

For nonzero (N_6(a)), these cannot agree for all (\lambda). Thus there is no genuinely bilinear scalar map

[
B:V_4\times V_2\longrightarrow\mathbb Q
]

with the properties claimed in the previous answer.

Accordingly, I retract:

* the claim that the relevant response dimension is at most (772);
* the claim that (230{,}347{,}816) projected coefficients specify the required contraction;
* the claim that only
  [
  \binom{774}{3}=76{,}981{,}524
  ]
  cubic coefficients suffice for (N(6)).

The last number is only

[
\dim\operatorname{Sym}^3(V_2);
]

it is the number of cubic **input monomials**, not the number of coefficients needed to specify all (63{,}199) output polynomials, and it certainly is not a sextic representation of the square.

---

# 1. Exact orbit-total normalization

Let (\Omega_r) be the labelled (r)-row configuration set used by reverse gluing, and let

[
\mathcal O_r=\Omega_r/\Gamma_r
]

be its coordinate-orbit set. For (O\in\mathcal O_r), write

[
m_O=|O|.
]

Use the orbit-total basis

[
\varepsilon_O=\frac1{m_O}\sum_{x\in O}e_x.
]

Thus a coordinate (u_O) represents total mass (u_O) on (O), and every labelled (x\in O) has coefficient (u_O/m_O).

Let

[
\iota_{r,s}(z;x,y)\in\mathbb Z_{\ge0}
]

be the exact labelled incidence multiplicity for gluing (x\in\Omega_r) and (y\in\Omega_s) to (z\in\Omega_{r+s}). It includes whatever fixed-row-slot embeddings, contingency multiplicities and block symmetries are part of the verified reverse-gluing semantics.

The exact reduced orbit-total bilinear map is then

[
\boxed{
\bigl[\bar G_{r,s}(u,v)\bigr]_Z
===============================

\sum_{O,P}
u_Ov_P,
\frac{
\displaystyle
\sum_{\substack{x\in O,\ y\in P\z\in Z}}
\iota_{r,s}(z;x,y)
}{
m_Om_P
}.
}
\tag{1}
]

There is no factor (m_Z^{-1}) in (1), because the output coordinate is the **sum** over (z\in Z), not the value at one representative.

Define

[
A=\bar G_{2,2}:\operatorname{Sym}^2(V_2)\longrightarrow V_4
]

and

[
J=\bar G_{4,2}:V_4\times V_2\longrightarrow V_6.
]

For the actual two-row orbit-total vector (a),

[
b=A(a,a),\qquad r=J(b,a).
\tag{2}
]

If (Q\in\mathcal O_6) and (F(Q)) is the factorization count at any labelled representative of (Q), then

[
\boxed{r_Q=m_QF(Q).}
\tag{3}
]

Let (w_Q) be the verified labelled multiplicity used in the outer weighted sum. In complete outer-class coordinates one normally has (m_Q=w_Q), but it is useful to keep the two symbols separate until the basis convention is fixed.

The exact weighted form in orbit-total coordinates is

[
\boxed{
W_{QQ}=\frac{w_Q}{m_Q^2}.
}
\tag{4}
]

Consequently,

[
\boxed{
N(6)=r^{\mathsf T}Wr.
}
\tag{5}
]

Under the natural convention (m_Q=w_Q),

[
W_{QQ}=\frac1{w_Q},
\qquad
N(6)=\sum_Q\frac{r_Q^2}{w_Q}.
\tag{6}
]

If the final vector instead stores the per-representative values (F(Q)), then the same identity is

[
N(6)=F^{\mathsf T}\operatorname{diag}(w_Q)F.
]

These are the same normalization in two different bases.

For an unordered (2+2) implementation, the ordered coefficient from (1) must be collected with the usual multiplicity

[
2-\delta_{O,P}.
]

That is the precise location of the block-exchange factor.

---

# 2. The correct cubic map and sextic Gram form

The expression (J(A(u,v),w)) need not be visibly symmetric in all three arguments from its chosen contraction order. Its symmetric polarization is

[
\mathcal R(u,v,w)
=================

\frac13\Bigl(
J(A(u,v),w)
+
J(A(u,w),v)
+
J(A(v,w),u)
\Bigr).
\tag{7}
]

It satisfies

[
\mathcal R(a,a,a)=J(A(a,a),a).
]

Therefore it induces a linear map

[
\boxed{
R:\operatorname{Sym}^3(V_2)\longrightarrow V_6
}
\tag{8}
]

such that, for

[
z=a^{\odot3},
]

we have

[
\boxed{
r=Rz.
}
\tag{9}
]

The correct Gram form is

[
\boxed{
H=R^{\mathsf T}WR
}
\tag{10}
]

and the exact objective is

[
\boxed{
N(6)=z^{\mathsf T}Hz
====================

(a^{\odot3})^{\mathsf T}
R^{\mathsf T}WR
(a^{\odot3}).
}
\tag{11}
]

This passes the homogeneity test:

[
a\mapsto\lambda a
\quad\Longrightarrow\quad
z\mapsto\lambda^3z
\quad\Longrightarrow\quad
N(6)\mapsto\lambda^6N(6).
]

No value of (F_6) or (N(6)) is built into (R), (W), or (H). They are determined entirely by labelled incidence and orbit multiplicities.

## The genuinely bilinear corrected object

There is a correct bilinear form, but its arguments are **paired composite states**, not one (V_4) state and one (V_2) state.

Linearize (J) as

[
\widehat J:V_4\otimes V_2\longrightarrow V_6,
\qquad
\widehat J(u\otimes v)=J(u,v).
]

Define

[
\boxed{
\mathcal B(x,y)
===============

(\widehat Jx)^{\mathsf T}
W
(\widehat Jy),
\qquad
x,y\in V_4\otimes V_2.
}
\tag{12}
]

This is genuinely bilinear. Taking

[
x=A(a,a)\otimes a
]

gives

[
\boxed{
N(6)=\mathcal B(x,x).
}
\tag{13}
]

Under (a\mapsto\lambda a),

[
x\mapsto\lambda^3x,
\qquad
\mathcal B(x,x)\mapsto\lambda^6\mathcal B(x,x).
]

Equation (12), rather than the previous cubic scalar (T(a,a)(a)), is the correct bilinear formulation of the square.

---

# 3. Are paired partial states required?

There are two distinct cases.

### Compute (F_6) first

One may evaluate the single-copy map

[
r=R(a^{\odot3})
]

without paired states, retain its (63{,}199) final coordinates, and then form

[
r^{\mathsf T}Wr.
]

In that contraction order, pairing happens only at the final weighted norm.

### Contract the square before reaching (V_6)

Then a paired object, or something algebraically equivalent to it, is unavoidable. The two copies must be certified to finish in the same outer class (Q). At the labelled level the relevant kernel is

[
\sum_{q}
\iota(q;x_1,x_2,x_3)
\iota(q;y_1,y_2,y_3),
\tag{14}
]

not the product of two independently marginalized sums.

Thus an early contraction must preserve an equality/Gram relation between the two partial outputs. It need not literally store a tuple called a “paired state,” but it must retain a sufficient quotient of exactly that relation. Forgetting it changes

[
\sum_Q w_QF(Q)^2
]

into something resembling

[
\left(\sum_Q c_QF(Q)\right)^2
]

and introduces unwanted cross terms between different (Q).

---

# 4. The true response dimension

Let

[
\rho_6=\operatorname{rank}_{\mathbb Q}R.
\tag{15}
]

Because (W) is diagonal with strictly positive entries,

[
z^{\mathsf T}Hz
===============

\sum_Q\frac{w_Q}{m_Q^2}(Rz)_Q^2.
]

Therefore

[
\ker H=\ker R
]

over (\mathbb Q), and hence

[
\boxed{
\operatorname{rank}H
====================

# \operatorname{rank}R

\rho_6.
}
\tag{16}
]

The rigorous current bounds are only

[
\boxed{
1\le \rho_6\le63{,}199.
}
\tag{17}
]

The upper bound is the complete outer-class dimension. No accepted result currently determines the exact rank of this particular reverse-gluing map.

A direct (Q)-channel realization therefore has exactly (63{,}199) sufficient response channels. The minimum possible number for a uniform exact **linear** response representation is (\rho_6), not (772).

The number (772) is merely

[
\dim V_2.
]

It places no (772)-dimensional upper bound on the image of a cubic map out of (\operatorname{Sym}^3(V_2)).

Likewise,

[
\dim\operatorname{Sym}^3(V_2)
=============================

# \binom{774}{3}

76{,}981{,}524
\tag{18}
]

is the input dimension of (R). A generic explicit matrix for (R) would have

[
63{,}199\cdot76{,}981{,}524
===========================

4{,}865{,}155{,}335{,}276
\tag{19}
]

entries.

Thus the previous statement that “(76{,}981{,}524) cubic coefficients suffice” omitted the entire output index (Q).

---

# 5. Exact C=6 size bounds for the corrected objects

These are algebraic representation sizes, not lower bounds against every possible structured circuit.

| Object                      |                                Exact size |
| --------------------------- | ----------------------------------------: |
| (V_2)                       |                                     (772) |
| (\operatorname{Sym}^3(V_2)) |                          (76{,}981{,}524) |
| (V_6)                       |                                (63{,}199) |
| Generic dense (R)           |       (4{,}865{,}155{,}335{,}276) entries |
| Symmetric explicit (H)      | (2{,}963{,}077{,}557{,}172{,}050) entries |

At eight bytes per coefficient, dense (R) would require about (35.4) TiB; at sixteen bytes, about (70.8) TiB.

An explicit symmetric (H) would require approximately (21.1) PiB at eight bytes per entry or (42.1) PiB at sixteen bytes.

By contrast, retaining the final (63{,}199)-coordinate vector uses only about (0.96) MiB at sixteen bytes per coordinate, before big-integer overhead. Thus **the final square is not the memory problem**. The unresolved problem is applying (R).

## What can be avoided algebraically

The composition

[
R=J\circ(A\otimes I)
]

can be streamed. One may accumulate directly into the final (63{,}199) coordinates without storing a (V_4) layer or a permanent ((\text{source triple},Q)) table:

[
r_Q
===

\sum_{O_1,O_2,O_3}
a_{O_1}a_{O_2}a_{O_3}
\sum_U
J_{Q;U,O_3}A_{U;O_1,O_2}.
\tag{20}
]

This is an exact removal of intermediate **storage**.

It is not, by itself, an arithmetic reduction. Evaluating the inner contraction through the currently verified incidence enumerator still traverses the same relative placements and contingency leaves.

The authoritative project state establishes that the current C=6 (2+2) route has 772 two-row orbits and that the 276 trivial-stabilizer orbits alone force (1{,}761{,}454{,}080) double-coset placements. It also reports near-injective canonical output behavior in bounded samples. 

Consequently, for the currently implemented double-coset route:

[
\boxed{
\text{placement visits before the final }2\text{-row join}
\ge1{,}761{,}454{,}080.
}
\tag{21}
]

This is an exact lower bound for that route, not for an unknown global transform.

If one emitted one sixteen-byte record per such placement, even this subset would occupy about (26.25) GiB. Streaming removes that storage, but not the visits.

Using the measured anchored rate of about (8{,}195) keys per second gives an optimistic projection of roughly (59.7) hours merely for this placement subset. That time is a heuristic extrapolation, not a theorem; it also ignores the thousands of contingency leaves observed in sampled placements, so it is optimistic.

Therefore:

[
\boxed{
\text{No scale-passing method for applying }R
\text{ or }H\text{ follows from the corrected formulation.}
}
]

One can avoid materializing (V_4), but no known exact construction avoids the verified placement arithmetic.

---

# 6. Exact uniform separator lower bound

A fixed numerical input (a) admits no meaningful unrestricted rank lower bound, because a nonuniform circuit could hard-code the answer. The appropriate uniform object is the polarized six-linear form

[
\Phi(u_1,u_2,u_3;v_1,v_2,v_3)
=============================

\left\langle
\mathcal R(u_1,u_2,u_3),
W\mathcal R(v_1,v_2,v_3)
\right\rangle.
\tag{22}
]

Specializing all six arguments to (a) gives (N(6)).

Consider a linear-separator or tensor-network model in which an edge separates the first three inputs from the last three and carries an (m)-dimensional exact vector space. Equivalently, suppose

[
\Phi(z,z')
==========

L(z)^{\mathsf T}C,M(z')
]

for linear maps

[
L,M:\operatorname{Sym}^3(V_2)\longrightarrow\mathbb Q^m.
]

Then

[
H=L^{\mathsf T}CM
]

and therefore

[
\boxed{
m\ge\operatorname{rank}H=\rho_6.
}
\tag{23}
]

This is an exact communication/bond-dimension lower bound for a uniform polarized circuit. It cannot be defeated by storing (N(6)), because the circuit must work for arbitrary symbolic inputs on both sides.

Its present numerical weakness is that (\rho_6) has not been measured. The direct outer-class realization proves only that (m=63{,}199) is sufficient.

A stronger numerical separator result therefore requires an exact rank measurement of (R), rather than the previously claimed (772)-channel argument.

---

# 7. Required small-(C) gates for the corrected map

I have not run a new C=4/C=5 implementation here, so I cannot truthfully supply time or RSS measurements. The following are the exact smallest tests for the corrected object.

## C=4

Here

[
\dim V_2=23,\qquad
\dim\operatorname{Sym}^2(V_2)=\binom{24}{2}=276,
\qquad
\dim V_4=26.
]

The complete map is the small matrix

[
A_4:\operatorname{Sym}^2(V_2)\longrightarrow V_4
]

with only

[
26\cdot276=7{,}176
]

possible entries.

The exact gate is

[
f_4=A_4(a^{\odot2}),
\qquad
N(4)=f_4^{\mathsf T}W_4f_4.
]

It must reproduce every one of the 26 values (F(q)) and the verified (N(4)). Under (a\mapsto\lambda a),

[
f_4\mapsto\lambda^2f_4,\qquad
N(4)\mapsto\lambda^4N(4).
]

The smallest numerical falsification is therefore (\lambda=2):

[
f_4(2a)=4f_4(a),\qquad
N_4(2a)=16N_4(a).
]

The exact rank of the (26\times276) matrix also gives the minimum uniform linear response dimension at C=4.

## C=5

Let (b) be the actual one-row vector. Compose directly

[
R_{5,b}(u\odot v)
=================

J_{4,1}(A(u,v),b).
\tag{24}
]

Since

[
\dim V_2=107,
\qquad
\dim\operatorname{Sym}^2(V_2)
=============================

# \binom{108}{2}

5{,}778,
]

and there are 355 complete outputs, the complete projected matrix has at most

[
355\cdot5{,}778
===============

2{,}051{,}190
\tag{25}
]

entries, approximately (31.3) MiB at sixteen bytes per dense entry.

It can be generated without storing the 17,120-orbit four-row layer:

1. Process one unordered two-row orbit pair.
2. Enumerate its exact relative placements and contingency leaves.
3. Immediately perform the (4+1) completion.
4. Canonicalize the complete output (q), not the intermediate four-row state.
5. Accumulate the coefficient for that source pair and (q).
6. Emit at most 355 coefficients for the source pair.

This is an exact projected construction. However, using the current leaf generator it would still traverse the verified

[
122{,}166{,}792
]

contingency leaves and the same relative placements. The existing implementation made (95{,}227{,}539) cold four-row canonicalizations. 

Therefore merely implementing (24) would demonstrate removal of (V_4) storage, but **not yet the required arithmetic reduction**. The decisive measurements are exactly those in your gate:

* number of final-(q) canonicalizations;
* number of emitted nonzero source-pair/output coefficients;
* whether multiple contingency leaves are combined before canonicalization;
* time and peak RSS;
* exact rank of the resulting (355\times5{,}778) matrix.

If the leaf count remains (122{,}166{,}792), the projected object has not solved the measured bottleneck, even though it stores only 355 response channels at a time.

---

# Final corrected status

The correct statements are:

[
\boxed{
F_6=R(a^{\odot3}),\qquad
N(6)=F_6^{\mathsf T}\operatorname{diag}(w)F_6
}
]

in per-class coordinates, or

[
\boxed{
r=R(a^{\odot3}),\qquad
N(6)=r^{\mathsf T}\operatorname{diag}(1/w),r
}
]

in the natural final orbit-total coordinates.

The correct direct-square object is a paired Gram form on

[
V_4\otimes V_2,
]

or equivalently

[
H=R^{\mathsf T}WR
]

on (\operatorname{Sym}^3(V_2)).

The minimum uniform linear response dimension is

[
\rho_6=\operatorname{rank}R,
]

with only

[
1\le\rho_6\le63{,}199
]

currently proved. Neither (772) nor (76{,}981{,}524) is the response dimension.

Finally, direct projection can eliminate the stored (V_4) layer, but no presently established construction eliminates the known relative-placement and contingency arithmetic. Thus Section 6 did not provide a C=6 route; it confused a cubic single-copy response with the required sextic weighted square.
