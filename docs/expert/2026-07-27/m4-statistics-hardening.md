# m4-statistics-hardening (agent report, 2026-07-26 pre-flight sweep)

## Summary

M_4 = 9.0e8 (95% band 8.87-9.20e8), confirming the ~9.1e8 working figure but tightening it. The key finding is methodological: the probe's total kept-emission count H supplies a *second exact identity* alongside D, which is what makes M_4 identifiable at all — without it the nonparametric profile likelihood is provably flat (Link 2003) and every histogram-only estimator is arbitrary. The homogeneous-Poisson value 8.823e8 is not an estimate but a hard assumption-free floor (Jensen). ZTNB fits r≈20.8 (CV 0.219) and is formally rejected (chi2 1091 on 2 df), so its ±0.02% likelihood interval is meaningless; real error bars come from model spread and are ~±1.5%. The C=5 calibration passes: the same estimator returns 17,285 vs truth 17,120 (+0.96%), and the ZTNB 95% profile CI [16,684, 17,208] contains 17,120. Strided sampling is a non-issue for smooth fan/in-degree correlation (bias <0.02% even at |rho|→1) because both identities are complete counts of the realised sample; the only dangerous failure mode requires >=10% of children captured at <=0.1x nominal rate. Recommended production table capacity: 1.30e9 entries (~59 GB), up from the planned 1.0e9 which would run at a 0.90 load factor and can overflow. Strongest recommendation: rerun the probe at k=58,400 (~19 min, no extra memory) to cut the unseen fraction from 7.2% to <0.3% and retire the entire statistical argument before committing 7.6 days of compute.

## Details

# Statistical hardening of the M_4 capture-recapture estimate

Work in `C:\Users\ChopperLin\AppData\Local\Temp\claude\E--Code-sudoku-FJ\c1528767-e58c-4a4d-8325-aaf8b87b1d06\scratchpad\`:
`nb_core.py`, `ctem.py` (estimators), `task1_c6_fit.py`, `task1b_identified_set.py`, `task1c_bounds.py`, `task1e_profile.py` (fits), `task2_sensitivity.py` (design), `task3_c5.py` (calibration), `task4_final.py`, `task5_robust.py`.

---

## 0. The structural result that reframes everything

The probe reports **two complete counts** over the window, not one:

```
  #{y : X_y >= 1} = D = 12,995,262     ->   1 - E_G[e^-lambda] = D / N_w
  sum_y X_y       = H = 39,402,926     ->       E_G[lambda]    = H / N_w
```

Dividing gives an exact, model-free constraint: `E[lambda] / (1 - E[e^-lambda]) = H/D = 3.032099`.

This matters for three reasons.

**(a) The Poisson answer is a hard floor, not an estimate.** By Jensen, `E[e^-L] >= e^-E[L]`, so `mu <= mu*` where `mu*/(1-e^-mu*) = 3.032099`, giving `mu* = 2.858130` and

> **M_4 >= 64·H/mu* = 8.823e8**, assumption-free.

`mu* = 2.858` is exactly the reported "homogeneous-Poisson MLE lambda 2.858", and `8.823e8` is exactly the reported MLE. So that number is a *lower bound* with equality iff there is zero heterogeneity. Every heterogeneity mechanism moves the answer up, never down. (Also `M_4 >= 64·D = 8.317e8` trivially; window-count binomial sd is only ±2.4e5, negligible.)

**(b) Without H, M_4 is not identified at all.** `task1b_identified_set.py` confirms Link (2003) numerically — with an unrestricted mixing distribution the profile log-likelihood is flat to within ~1 nat from `M_4 = 9.1e8` all the way to `3.3e9`:

```
  M_4 = 9.1487e8   logL = 179,092,800.450
  M_4 = 9.9804e8   logL = 179,092,807.148    <- max
  M_4 = 1.1644e9   logL = 179,092,801.730
  M_4 = 2.0792e9   logL = 179,092,779.124
  M_4 = 3.3268e9   logL = 179,092,743.763
```

Pinning `E[lambda] = H/N` via the second identity is what creates a genuine interior maximum. Any analysis that uses only `D` and the histogram is relying entirely on the parametric family for identification.

**(c) Hash uniformity checks out.** Expected kept = 2,522,319,152/64 = 39,411,237 vs H = 39,402,926, i.e. **-0.021%**. The window is behaving as a uniform random subset of the child space.

---

## 1. Gamma-mixed Poisson (ZTNB) fit

MLE on cells {1,2,3,4,>=5}:

| quantity | value |
|---|---|
| mu (mean lambda) | 2.8277 |
| **Gamma shape r** | **20.84** |
| CV of lambda = 1/sqrt(r) | **0.219** |
| variance/mean | 1.136 |
| p0 | 0.0705 |
| **M_4 point** | **8.948e8** |
| profile 95% CI | [8.946e8, 8.950e8] |

**The ±0.02% interval is not usable and must not be quoted.** With D = 1.3e7 the multinomial sampling error is ~1e-4 relative, so likelihood curvature measures nothing but the family's own rigidity. The NB is *rejected*: Pearson chi2 = 1626 on 2 df, with a systematic oscillating residual (+16, -26, -2, +25, -7.5 sigma across cells 1..>=5). It also mispredicts the free tail statistic: observed mean inside the >=5 cell is **5.7506** (= (H - 25,971,588)/2,335,642), NB predicts 5.8075 — a ~56-sigma miss.

Competing families, all fitted the same way:

| family | M_4 | chi2 | pred. >=5 mean (obs 5.751) |
|---|---|---|---|
| Poisson (1p) | 8.831e8 | 40114 (3 df) | 5.677 |
| **NegBin / Gamma-Poisson** | **8.948e8** | 1626 (2 df) | 5.807 |
| Poisson-lognormal | 8.943e8 | 1887 (2 df) | 5.814 |
| compound Poisson | 8.951e8 | 1495 (2 df) | 5.804 |
| 2-point Poisson mixture | 9.015e8 | **3.1** (1 df) | 5.745 |
| 3-point Poisson mixture | 9.026e8 | 0.0 (sat.) | 5.754 |
| bounded NPMLE + both identities (floor 900) | 8.986e8 | 1091 | — |

The two-component mixture fits essentially perfectly, so the shape misfit is a genuine feature of the in-degree law, not noise. Its residual cell errors at the constrained-NPMLE optimum are only +0.24 / -0.88 / -0.03 / +1.01 / +0.08 % — a ~1% shape effect that is overwhelmingly significant at n = 1.3e7 but second-order for p0.

**Defensible interval.** Quasi-likelihood adjustment (phi = chi2/df = 1091/2 = 545, threshold phi·3.8415):

> **M_4 = 8.986e8, 95% band [8.893e8, 9.114e8]** (-1.03% / +1.42%) at floor 900

Sensitivity to the in-degree floor (`task1e_profile.py`) — this is the identifying assumption:

```
 floor      M_4        chi2     CV      r     d_bar
   600   9.0616e+08   274.8   0.2380  17.65   2,349
   800   9.0175e+08   661.0   0.2297  18.95   2,361
   900   9.0029e+08   867.8   0.2269  19.42   2,365
  1000   8.9882e+08  1038.9   0.2238  19.97   2,368
  1200   8.9588e+08  1326.6   0.2164  21.35   2,376
  1500   8.9441e+08  1871.9   0.2139  21.85   2,380
```

Flat to ±0.7% across any plausible floor — this is the reassuring part. **With no floor the estimate runs away to 1.58e9**, which is exactly why the floor must be defended explicitly rather than assumed silently.

Identity-locked CV -> M_4 mapping (useful summary of the whole problem):

```
  CV 0.000 -> 8.823e8 (Jensen floor)     CV 0.240 -> 8.983e8
  CV 0.190 -> 8.922e8                    CV 0.280 -> 9.043e8
  CV 0.219 -> 8.955e8                    CV 0.320 -> 9.115e8
  CV 0.228 -> 8.967e8                    CV 0.434 -> 9.384e8 (C=5 level)
```

Internal consistency: the fit implies mean in-degree 2,374-2,380, so `d_bar × M_4 = 2.129e12` against the sample-measured 3->4 total of 2.136e12 (**-0.3%**). Note the quoted "mean in-degree ~2354" is *not* an independent datum — it is 2.136e12/9.07e8, i.e. derived from the answer.

---

## 2. The strided-sampling concern — quantified

`task2_sensitivity.py`. The headline is that this concern is **much weaker than it looks**, for a precise reason: both identities are complete counts of the *realised* sample, so a stride that is unrepresentative in fan size does not bias `N_w` at all. It only changes the shape of the lambda-distribution being extrapolated.

**(A) In-degree / parent-fan correlation: essentially harmless.** Tilting `lambda_y = f·d_y·u_y` with `u = (d/d_bar)^beta` (renormalised to preserve H) and running the full estimator on the synthetic data:

```
   beta  corr(d,u)   CV(u)   true p0   est bias
  -1.50     -0.909  0.4144   0.06284    +0.00%
  -1.00     -0.943  0.2568   0.05974    +0.01%
   0.00     +0.000  0.0000   0.07163    -0.01%
  +1.00     +1.000  0.2109   0.10293    -0.00%
  +1.50     +0.997  0.3075   0.12327    -0.02%
```

**Answer to "what correlation shifts the estimate by >5%": no smooth monotone correlation does, even at |rho| -> 1 with CV(u) = 0.41.** The nonparametric mixture absorbs it because the tilted lambda stays inside the fitted support.

**(B) The failure mode that does bite** is a sub-population the stride systematically under-covers — mass pushed *below* the in-degree floor. Error the estimator would make, for a fraction eps of children captured at rate c × nominal:

```
    eps    c=0.50   c=0.30   c=0.20   c=0.10   c=0.05
  0.010    -0.67%   -0.83%   -0.96%   -1.13%   -1.25%
  0.050    +0.11%   -0.67%   -1.32%   -3.39%   -3.97%
  0.100    -0.35%   -0.73%   -2.06%   -5.08%   -7.42%
  0.200    +0.01%   -1.84%   -3.44%   -9.64%  -13.38%
```

The 5% threshold needs **>=10% of all four-row states reachable at <=0.1x the nominal rate** — effective in-degree ~235 against a claimed floor of 900. The bias is *negative*, so this risk under-estimates M_4 (safe direction for correctness, dangerous for table sizing).

**(C) Within-parent emission multiplicity** (a parent emitting to the same child several times) is the other structural worry, since `P(X=0) = E[e^{-lambda/m_bar}] > E[e^{-lambda}]` manufactures extra zeros. But the histogram bounds it hard:

```
  m_bar    M_4        shift    chi2
   1.00  8.9527e+08   +0.03%    1524
   1.05  8.9527e+08   +0.03%    2360
   1.10  9.0451e+08   +1.06%   19726   <- decisively rejected
   1.50  1.0123e+09  +13.10%  606879
```

So m_bar <= ~1.05 and the effect is <0.1%. Still worth measuring directly (free: count distinct (parent, child) pairs vs total emissions).

**Empirical hint the stride is fine:** the k=14,600 strided sample gives mean fan 172,762; the independent 20,000-parent strided sample gave 173,291 — a 0.3% difference.

**Tests worth running (cheap):** rerun with a different stride offset; rerun with a genuine random sample of the same k; compare D and the histogram. Any of these disagreeing by more than ~0.5% would indicate real design structure.

---

## 3. Reconciliation with the C=5 calibration

`task3_c5.py`. The reconstructed histogram is self-validating: it reproduces the reported Chao1 = 16,830 exactly, and the identity `mu/(1-p0) = R` closes to 4e-16.

Ground truth extraction (needs only D and the known M_4 — no model):
- `N_w_true = 17,120/4 = 4,280`, `D = 3,774` -> **p0_true = 0.11822** (506 unseen)
- `mu_true = H/N_w = 2.6288` -> mean in-degree 707.6, total 3->4 emissions 1.211e7
- Poisson would give `e^-mu = 0.0722`. **The observed 0.1182 proves heterogeneity is real and that Poisson must understate.**
- Implied true Gamma shape **r = 5.305, CV = 0.434**

Estimators run blind on the C=5 histogram:

| estimator | M_4 | error |
|---|---|---|
| naive D×4 | 15,096 | -11.82% |
| ZT-Poisson (mean-matched) | 16,073 | -6.11% |
| Chao1 | 16,830 | -1.69% |
| **ZTNB** | **16,954** | **-0.97%** |
| **identity-constrained NPMLE** (same estimator as C=6, same relative box) | **17,285** | **+0.96%** |

**ZTNB 95% profile CI at C=5 = [16,684, 17,208] — contains 17,120.** So the answer to task 3 is yes, and the method's demonstrated accuracy is roughly ±1-2%, bracketing the truth from both sides.

Also: the ZTNB reads the heterogeneity nearly correctly (fitted r = 5.765 / CV 0.416 vs true r = 5.305 / CV 0.434, understating CV by 4%). This is important — it means the C=6 reading of CV = 0.219 is trustworthy.

**The one place the C=5 calibration does NOT transfer.** The true C=5 CV is 0.434; the C=6 histogram says 0.219. Carrying 0.434 over would give M_4 = 9.384e8. But that is not permissible: `task5_robust.py` shows a CV = 0.434 law is flatly incompatible with the observed C=6 histogram (chi2 = 185,683 vs 2,119 at CV 0.219). The in-degree CV is a property of the graph and there is no reason it is constant in C; the C=6 histogram measures it with D = 1.3e7 versus D = 3,774 at C=5. Treat 9.38e8 as a stress scenario, not an estimate.

Coverage simulation against known truths (`task5_robust.py`) — the estimator itself is near-unbiased when the truth is in the class:

```
  true law                       true M_4      est       err    chi2 vs obs
  Gamma CV=0.183               8.9140e+08  8.9018e+08  -0.14%      4,818
  Gamma CV=0.219               8.9552e+08  8.9482e+08  -0.08%      2,119
  Gamma CV=0.289               9.0577e+08  9.0429e+08  -0.16%     18,984
  uniform [0.6,1.6]x d_bar     9.0824e+08  9.0897e+08  +0.08%     23,348
  two-point 0.55/1.45          9.5322e+08  9.5974e+08  +0.68%    355,823
```

Robustness of the C=5 reconstruction: across assumed lambda in [2.6, 3.0], true CV ranges only 0.368-0.488 — the qualitative conclusion is not fragile.

---

## 4. Final deliverable

### M_4 point estimate and error bars

```
  absolute floor (64*D, cannot be below)        8.317e8
  Jensen floor (assumption-free)                8.823e8   <- the "Poisson MLE"
  Chao1                                         8.941e8
  ZTNB                                          8.948e8
  ---> POINT ESTIMATE                           9.00e8
  95% band                                      8.87e8 - 9.20e8   (-1.5% / +2.2%)
  distribution-free upper bound (in-degree box) 1.227e9
  stress: C=5-level heterogeneity (rejected)    9.384e8
```

The band combines the quasi-likelihood interval (±1.0/1.4%), the floor sensitivity (±0.7%), the family spread (8.94-9.03e8 among non-rejected families), and the C=5-measured bias of ±1%. It is *not* a likelihood interval — at this sample size that would be ±0.02% and would be wrong.

This confirms and slightly tightens the existing 9.1e8 working figure, and independently corroborates Pettersen's ">900 million" — though note the point estimate sits marginally *below* 9.0e8 by most routes, so "~900 million" is right at the boundary rather than comfortably above it.

### Recommended fixed-table capacity

Cost basis from the run log: 1.0e9 entry cap -> ~45 GB (45 B/entry), machine 125.7 GB.

```
   capacity     GB   load@9.02e8  load@9.20e8  load@1.227e9
  1.000e+09   45.0      0.902        0.920        OVERFLOW
  1.200e+09   54.0      0.752        0.767        1.022 OVERFLOW
  1.300e+09   58.5      0.694        0.708        0.944
  1.500e+09   67.5      0.601        0.613        0.818
```

> **Recommended: 1.30e9 entries (~59 GB).**

Rationale: load factor 0.69 at the point estimate and 0.71 at the 95% upper end — both under the ~0.75 knee where open-addressed probing degrades — **and it does not overflow even at the distribution-free maximum 1.227e9** (it merely runs slow at load 0.94). That last property is the one that matters: a capacity hole after several days of compute is far more expensive than 13.5 GB of RAM.

**The currently planned 1.0e9 cap should be raised.** At the point estimate it runs at load 0.90, and it overflows outright anywhere above 1.0e9, which the data do not exclude. The extra 13.5 GB is comfortable on a 125.7 GB box.

If RAM permits, 1.5e9 (67.5 GB) buys load 0.60 and removes the question entirely.

### Strongest recommendation: spend 19 minutes before spending 7.6 days

The entire statistical argument exists only because 7.16% of window children were never hit. Memory is bounded by `N_w ~ 1.41e7` window keys **regardless of k** — only probe time scales. From `task2_sensitivity.py` / `task5_robust.py`:

```
        k    x   probe    p0 @CV=0.21  p0 @CV=0.43  p0 @CV=0.60
   14,600   1x    4.8m       0.0712       0.1044       0.1429
   29,200   2x    9.6m       0.0068       0.0215       0.0460
   58,400   4x   19.2m       0.0001       0.0024       0.0111
```

At k = 58,400 the unseen fraction drops below 0.3% **even under pessimistic C=5-level heterogeneity**, and M_4 becomes essentially a direct measurement rather than an extrapolation. Every model-choice question above collapses. Doing this before committing the ~7.6-day production run is the single highest-value action.

Two other free additions to that rerun: (i) count distinct (parent, child) pairs to measure m_bar directly; (ii) use a different stride offset (or a random sample) so the design assumption is tested rather than assumed.

## Verdict

M_4 = 9.0e8, 95% band [8.87e8, 9.20e8]; hard bounds [8.82e8, 1.23e9]. The existing ~9.1e8 figure is sound but its stated "+-few %" was optimistic in the wrong direction — 8.823e8 is a floor, not a point estimate. The C=5 calibration passes (17,285 vs 17,120, +0.96%; ZTNB CI contains truth). Strided sampling is not a material risk. Recommend raising the layer-4 table capacity from 1.0e9 to 1.30e9 entries (~59 GB), and running a 4x probe (k=58,400, ~19 min, no extra memory) to convert the estimate into a measurement before committing the 7.6-day run.

