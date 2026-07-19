# Exact global aggregation for the linear C=6 outer sum

This implementation evaluates the linear weighted sum

\[
\sum_{[G]}w([G])F_6(G)
\]

when `w([G])` is the natural orbit-unrolling weight: the number of labeled balanced skeleton matrices represented by the outer class.

It uses the exact identity

\[
\sum_{B}F_C(Q_B)=2^{C^2}
\#\left\{(\sigma_1,\ldots,\sigma_{2C})\in S_C^{2C}:
\sum_jP_{\sigma_j}=2J_C\right\}.
\]

The main program processes row pairs.  Its state is the color-orbit of a balanced histogram

\[
h:\binom{[C]}r\to\mathbb Z_{\ge0},\qquad
\sum_Sh_S=2C,\qquad
\sum_{S\ni a}h_S=2r.
\]

For C=6 the orbit layer sizes are

`1, 1, 24, 132, 24, 1, 1`.

## Build and run

```bash
g++ -std=c++20 -O3 -DNDEBUG -Wall -Wextra -pedantic \
  global_linear_histogram_dp.cpp -o global_linear_histogram_dp
./global_linear_histogram_dp 6
```

Expected final lines:

```text
A_C=70957164389662881792000
N_linear_C=2^(C^2)*A_C=4876139207527966044188061990912000
```

## Exact rank certificates

```bash
g++ -std=c++20 -O3 -DNDEBUG -Wall -Wextra -pedantic \
  global_linear_histogram_rank.cpp -o global_linear_histogram_rank
./global_linear_histogram_rank 6 1000003
```

The middle maps have ranks 24 and 24, the maximum possible, modulo 1,000,003; therefore they have full corresponding rank over Q.

## Independent checks

- `global_linear_aggregation_c6.cpp`: symbol-by-symbol count-matrix orbit DP; different state space, same exact result.
- `direct_skeleton_regression_c3.cpp`: direct sum over all 8,000 labeled balanced C=3 skeletons; result 460,800.
- `global_linear_aggregation_exact_record.txt`: consolidated outputs, rank certificates, resource log, and source hashes.

## Important scope distinction

This solves the **linear** sum as written.  It does not evaluate the two-copy/squared objective

\[
\sum_{[G]}w([G])F_6(G)^2,
\]

because the two colorings must share the same skeleton and the local bit sum no longer collapses to a single balanced-color assignment.
