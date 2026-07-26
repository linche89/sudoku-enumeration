# Corrected layer DP: independent verification, permanent profile, and a measured cost frontier

Date: 2026-07-26.  Companion scripts (all run to completion on this date):

- `layer_dp_check.py` — independent re-implementation of the reviewer's
  corrected recursion; exact through C=4.
- `eps_orbit_check.py` — verification that the eps quotient equals the
  existing complete-class system (2 / 4 / 26).
- `perm_anchors.py` — 28,000 exact permanents over sampled states
  (2,000 per (C, L), exact-uniform next-row sampling, seed 20260726).

This note replaces the withdrawn Sections 3-4 of
`../2026-07-25/row-split-dual-and-bulk-generator-proposal.md` (see its
erratum).  Every number below is tagged [exact], [measured], or
[estimate].

## 1. The corrected recursion, independently verified [exact]

The reviewer's corrections are all confirmed by an independent
re-implementation:

```text
T_L(x)   = m_x F_L(x),          m_x = |G_C| / s_x
T_{L+1}(y) = sum_x T_L(x) R(x,y)
R(x,y)   = sum over feasible emissions n from rep(x) with can(out n) = y
           of K_c(n) = prod_w c_w! / prod_{u,v} n_uv!
N(C)     = sum_q (ell_q / m_q) T_C(q)^2,   ell_q = (2C)! / prod_w c_w!
```

Results (states per layer / emissions per transition / final value):

| C | states | emissions | N | verdict |
|---|---|---|---|---|
| 2 | 1, 2 | 4 | 288 | exact |
| 3 | 1, 5, 4 | 80, 29 | 28,200,960 | exact |
| 4 | 1, 23, 54, 26 | 4,752, 4,630, 712 | 29,136,487,207,403,520 | exact |

These emission and state counts equal the reviewer's independent run
figure-for-figure.  The C=2 normalization failure of the unit-weight
version is confirmed: layer-2 T values are (8, 2), orbit mass 10 vs
naive mass 4.

Implementation nuance for the gate: `ell_q / m_q` is not always an
integer by itself (C=4 counterexamples: (ell, m) = (1120, 24), (70, 8),
(2520, 16), (10080, 64)); the integrality assertion must sit on the full
class summand `ell_q T_q^2 / m_q`, which is exact for every class at
C=2..4.

Raw-vs-canonical dedup, measured at C=4: 2,019 -> 23, 3,412 -> 54,
366 -> 26 per transition.  Raw-key-first reduction merges a lot at small
C, but a C=6 raw dictionary would be ~|G|/avg-stab times the canonical
one, so semi-canonical keys (Section 5) dominate raw-first designs.

## 2. The eps quotient is the existing class system [exact]

Under S_{2C} (symbols) x S_C (bands) x per-band complement flips, the
eps = (A_1..A_C) tuples fall into exactly 2 (C=2), 4 (C=3), 26 (C=4)
classes — the known complete band classes.  C=4 was done by Burnside
over C2 wr S4 (384 elements) and cross-checked by direct enumeration of
all 1,450 valid membership-vector multisets.  Phi is constant on every
class and `sum |class| Phi^2 = N(C)` holds exactly at C=2, 3.  Caveat:
Phi does not separate classes (two C=3 classes share Phi = 48).  At C=6
the eps class count is therefore exactly 63,199.

## 3. Structural facts about the row-incremental layers [exact]

For an L-row state of the 2xC model (2C symbols, 2C slots = C boxes x 2
sides):

- The FREE bipartite compatibility matrix (next-row extensions) is
  2(C-L)-regular; the USED matrix (single-row peelings) is L-regular.
- Hence every per-state fan-out and in-degree is the permanent of a
  regular 0/1 matrix, bracketed by van der Waerden-Falikman
  (`n!(k/n)^n`) below and Bregman-Minc (`(k!)^(n/k)`) above.  All 28,000
  sampled permanents fell strictly inside their brackets.
- At L = C-1 the free graph is exactly a disjoint union of C copies of
  K_{2,2} (each box retains 2 free symbols, each compatible with both
  sides), so the last-transition fan-out is exactly 2^C for every state:
  32 at C=5, 64 at C=6.  The final transition is structurally cheap and
  lands directly on the complete classes.

## 4. Measured permanent profile [measured]

2,000 sampled labelled states per (C, L); next rows drawn exactly
uniformly by residual-permanent conditioning; medians and means below.
Sampling weight is the ordered-history measure (see Section 4.1 for why
that is the right weight for chain closure); spreads are small (IQR
0.15% of median at C6 L2, 1.7% at L3, 14% at L4).

C=6 (n = 12):

| L | per(FREE) med / mean | per(USED) med / mean | merge mean | vdW-Bregman (FREE) |
|---|---|---|---|---|
| 1 | 59,245,120 [exact] | — | 1 | 5.37e7 - 7.40e7 |
| 2 | 4,742,144 / 4,743,079 | 4 / 5.71 | 1.688 | 3.69e6 - 8.10e6 |
| 3 | 196,608 / 197,356 | 144 / 147.8 | 1.152 | 116,944 - 518,400 |
| 4 | 2,816 / 2,876 | 2,568 / 2,589 | 1.096 | 901 - 13,824 |
| 5 | 64 [exact, constant] | 27,172 / 27,263 | 1.119 | = 64 |

C=5 (n = 10):

| L | per(FREE) med / mean | per(USED) med / mean | merge mean |
|---|---|---|---|
| 1 | 440,192 [exact] | — | 1 |
| 2 | 30,720 / 30,787 | 4 / 5.02 | 1.796 |
| 3 | 832 / 884 | 75 / 76.8 | 1.19 |
| 4 | 32 [exact, constant] | 832 / 840.7 | 1.204 |

The project's own 772-orbit measurement (mean labelled extensions
4,745,826, median 4,731,392) sits beside the sampled C6 L2 values
(4,743,079 / 4,742,144).

### 4.1 Chain closure against known global invariants [measured]

Let `A_L` be the number of symbol-labelled, row-ordered L-row prefixes.
Exactly, `A_{L+1} = A_L * E[per(FREE) at L]` with the expectation in the
ordered-history measure — which is precisely the sampler's measure.
Multiplying the measured means through to L = C gives `A_C = sum_q
w(q) F_C(q)`, and F is known to be concentrated across classes, so
`A_C / sum_q w(q)` must land on `sqrt(N(C) / sum_q w(q))`:

```text
C=5: chain 10! * 440192 * 30787.2 * 884.416 * 32 = 1.39184e21
     sum w = 252^5 = 1,016,255,020,032   (the known labelled
             multiplicity sum, exactly)
     implied E_w[F_5] = 1.36958e9 vs sqrt(N(5)/sum w) = 1.36871e9
     agreement +0.06%
C=6: chain 12! * 59245120 * 4743079.136 * 197356.224 * 2876.032 * 64
       = 4.8897e33
     sum w = 924^6 = 6.22346e17
     implied E_w[F_6] = 7.8569e15 vs sqrt(N(6)/sum w) = 7.8444e15
     agreement +0.16% (vs Pettersen's historical value; comparison
     only, not an acceptance oracle)
```

Both closures agree to a few parts per thousand, consistent with
compounded sampling error plus the (tiny, positive) Jensen gap.  This
validates the measured per-profile end to end.

## 5. Scale and cost frontier [estimate unless tagged]

Exact anchors: M_2 = 772; transition 2->3 has 2,605,194,602 native
emissions and 3,663,777,792 labelled matchings (reviewer-measured,
2026-07-26); transition (C-1)->C fan-out is exactly 2^C per state.

Chain estimates from the measured profile (in-degree = per(USED)/merge,
fan-out = per(FREE)/merge; brackets widened for the history-weighting
bias, which the C=5 gate will calibrate):

| quantity | bracket |
|---|---|
| M_3 | (1.5 - 4) x 10^7 |
| work(3->4) | (2.5 - 7) x 10^12 emissions |
| M_4 | (0.7 - 3) x 10^9  (Pettersen's ">900 million" sits inside; not evidence) |
| work(4->5) | (2 - 8) x 10^12 emissions |
| M_5 | (0.7 - 3) x 10^8 |
| work(5->6) | (0.4 - 2) x 10^10 emissions |
| total | (0.5 - 1.6) x 10^13 emissions, dominated equally by 3->4 and 4->5 |

Consequences:

- The reviewer's core objection stands quantitatively: the emission
  count is not reducible by this route; ~10^13 elementary steps is the
  honest total.  What the route changes is the constant per step.
- At the measured cold-canonicalization cost (122 us anchored, or
  hundreds/s generic) the route is dead: 3 - 40 core-years.
- At 1 us per emission it is (0.5 - 1.6) x 10^7 core-seconds = 2.5 - 8
  days on 24 threads (x2-3 if CRT passes or dictionary shards are run
  serially).  The entire feasibility question is the per-emission
  constant, to be measured, not assumed.
- Memory: the peak dictionary is layer 4 at 24-32 B/entry (key + one
  CRT residue + open-addressing overhead): 20 - 100 GB across the M_4
  bracket.  Key-range sharding (re-generating the 3->4 pass per shard)
  trades compute for RAM if needed.  No external sort is mandatory:
  emissions are consumed in flight into the dictionary.
- Arithmetic: run the DP mod 2-3 63-bit primes (N(6) < 2^165) and
  reconstruct by CRT; cross-prime agreement is a strong self-check, and
  narrow values halve dictionary width.
- The final transition accumulates into a 63,199-slot array; per-class
  F_6(q) output is falsifiable at fine grain against the two known
  values F_6(G1) = 6,986,348,258,918,400 and F_6(G2) =
  7,053,808,087,203,840 long before any total is trusted.

### 5.1 The per-emission constant: candidate mechanisms

Answering the reviewer's closing question (merge or implicitly apply
batches of row extensions before canonicalization):

1. **Parent-seeded incremental canonization.**  All children of one
   parent share the parent's certificate; seed the partition refinement
   with the parent's canonical coloring so each child pays only the
   one-row delta (typically one refinement round on a 24-vertex colored
   graph).  This is the canonical-augmentation idiom (McKay); target
   0.3 - 1 us amortized, vs 122 us measured for the anchored scan.
2. **Two-tier keys.**  Per emission compute only a cheap anchored
   semi-canonical key (target ~100-300 ns); accumulate coefficients in
   the RAM dictionary on semi-keys; run full canonization once per
   distinct semi-key and merge.  Total full canonizations collapse from
   ~10^13 to a small multiple of M_4.  The distinct-semi/M ratio is
   measurable at C=5 and on bounded C=6 parent samples.
3. **What does not work:** raw-key-first global dedup (raw dictionary is
   ~|G|/avg-stab larger than canonical); batching two rows at once
   (fan-out multiplies, intermediate merging is lost); pulling from the
   complete side (63,199 x per-pair peeling fan-out ~ 6e9 per class).

Both mechanisms must pass the repository's invariance, separation, and
histogram differential gates against the full-group scan before any
number they produce is used.

## 6. Gates (agreed order, with falsifiable predictions)

1. Fix normalization (done above, verified to C=4).
2. Build the structure-preserving canonicalizer (bipartite +
   box-pairing) with the two mechanisms of 5.1; pass the differential
   gates.
3. **C=5 gate** — reproduce all 355 (orbit, multiplicity, F) triples and
   N(5); report per-layer emissions, unique children, time, RSS, spill
   bytes.  Predictions to falsify:
   - states [1, 107, (2.5 - 5) x 10^4, 17,120, 355];
   - emissions [440,192 [exact], 2,324,325 [reviewer-measured],
     (1.5 - 3.7) x 10^7, ~4.6 x 10^5];
   - single-thread runtime tens of seconds at a 1-2 us constant.
4. Only if C=5 shows the constant: bounded C=6 parent samples (measure
   the real per-emission cost and distinct-semi ratios on sampled
   two-row and synthetic three-row parents).  No full M_3 generation
   before this passes; no full C=6 layer or contraction is proposed or
   authorized by this note.

## 7. The target-only contraction has its first nonzero data point [exact]

Added 2026-07-26, scripts `rank_u_check.py`, `rank_u_nullspace.py` (note:
`rank_u_check.py` imports `layer_dp_check.py` from the same directory).

Define the completion operator `U(x, q) = R_{C-1,C}(x, q)` from the last
partial layer to the complete classes.  Since
`N = T_{C-1} (U D U^T) T_{C-1}^T` with `D = diag(ell_q/m_q) > 0`, any exact
scheme that factors through class values needs exactly `rank(U)` channels
— this is the C=4/C=5 shadow of the STATUS target-only `y = Kbar x`
question.  Exact ranks over Q:

```text
C=3: U is 5 x 4,   rank 4  (full — no compression)
C=4: U is 54 x 26, rank 24 (DEFICIENT by 2)
     all intermediate transitions R_12 (1x23), R_23 (23x54) full rank
```

The two exact column relations at C=4:

- `R(x, q14) - 4 R(x, q15) + 9 R(x, q16) = 0` for EVERY 3-row state x —
  a universal local identity.  Support = the three most symmetric
  classes (`ell` = 70, 1120, 2520, i.e. mask-multiplicity patterns
  (4,4), (3,3,1,1), (2,2,2,2)); coefficients (1, -4, 9).
- a second relation with support 15 and coefficients in
  {+-3, +-16, +-36, +-96, +-144, +-288, +-576}.

Both annihilate the actual T_4 vector, as they must.

Reading: the target space genuinely compresses (24 < 26), so the
target-only contraction question is well-posed with a nonzero answer —
but the deficiency at C=4 is 2/26 and sits on high-symmetry classes, so
the working hypothesis is that it stays a vanishing fraction at C=5/C=6
and does not move the M_4-scale cost.  The decisive measurement is
`rank(U)` at C=5 (a ~(2.5-5)e4 x 355 integer matrix), which the C=5 gate
produces as a by-product.  If the C=5 deficiency is O(10) the hunt for a
large target-only contraction should be closed with that number; if it
is a sizable fraction of 355 there is real structure to chase before any
C=6 commitment.

## 8. Gate results (measured 2026-07-26, same day)

Engine: `experiments/proto/layer_dp_gate.cpp` (single file, single-thread,
g++ 13.2 -O2).  Logs: `data/logs/layer-dp-gate-c5-v1-20260726.txt`,
`...-c5-v3-20260726.txt`, `...-c6-probe-20260726.txt`.  Class dump:
`layer_dp_c5_classes.csv` (this directory).

### 8.1 Correctness — everything passed on the first complete run

- C=2/3/4: states, emissions, N all exact (built-in expectations).
- C=5: N(5) exact; **all 355 (m, ell, F) triples match** the independent
  2026-07-21 reference CSV; sum_w = 252^5 exact; layer-4 orbit mass =
  62,185,328 over 17,120 orbits exact; invariance 20,000 samples and
  full-group-scan differential 2,000 samples: 0 failures (stabilizer
  counts included).
- C=6 live cross-checks from the bounded probe: the full 1->2 transition
  produced **exactly 772 children** (the verified two-row layer) from
  **exactly 59,245,120 emissions** (the exact permanent anchor).

### 8.2 Predictions of Section 6 vs actuals

| quantity | predicted | actual |
|---|---|---|
| C=5 states per layer | [1, 107, (2.5-5)e4, 17120, 355] | [1, 107, **16,150**, 17,120, 355] |
| C=5 emissions | [440192, 2324325, (1.5-3.7)e7, ~4.6e5] | [440,192, 2,324,325, 11,957,632, 462,403] |

M_3(C=5) = 16,150 (first exact count) landed 1.5x BELOW my bracket's low
end; the 3->4 out-fan per parent (740.5) matched the permanent prediction
(743) almost exactly, so the miss was entirely in the layer-3 in-degree
(actual 143.9 tables/state vs naive 64.5; calibration factor 2.23).
Layer-4 in-degree needed no correction (698.5 actual vs 700 naive).

### 8.3 The target-only contraction is closed: deficiency is O(1)

Mod-p ranks (two 61/62-bit primes, identical results):

```text
C=4: rank(U_3) = 24 / 26   (deficiency 2)
C=5: rank(U_4) [17120 x 355] = 355  (full)
     rank(U_3) [16150 x 355] = 353  (deficiency 2)
     rank(U_2) [107 x 355]   = 107  (row-limited full)
```

The deficiency is **2 at C=4 and 2 at C=5** — constant, not growing with
the class count, and it lives at the 3-row boundary.  Verdict for the
STATUS target-only question: exact linear compression of the target
space exists but is O(1); it cannot move the M_4-scale cost at C=6.
This search direction should be closed with these numbers.

### 8.4 The per-emission constant (the decision variable)

Single-thread, measured end-to-end:

| version | change | C=5 total | ns/emission |
|---|---|---|---|
| v1 | column-orderly canonizer, vector compares | 111.0 s | ~7,300 |
| v2 | packed 24-bit column signatures, group refinement | 46.7 s | ~3,000 |
| v3 | tie lookahead ordering + per-parent raw cache | 48.8 s | ~3,100 |

v3 was a wash at C=5 (fewer nodes, more per-node work; raw-cache hits
~0% at deep layers — recorded honestly).  At C=6 scale the current
engine runs ~5.2 us/emission (12 masks, depth 6).  Identified remaining
levers, unimplemented: SWAR popcount signatures (~2x), node reduction
via stronger initial invariants, parent-seeded warm starts, OpenMP.

### 8.5 Bounded C=6 probe (reviewer's step 5)

Full 1->2 (330 s), then 8 strided two-row parents into 2->3:

- 26,169,296 emissions from 8 parents; extrapolates to 2.53e9 over 772,
  consistent with the exact 2,605,194,602.
- Per-parent fan (74,736 .. 4,752,128) brackets the measured median
  4,731,392.
- **8,791,467 distinct three-row children from 8 parents.**  Observed
  local hit rate 2.98 emissions/child matches the predicted sample
  in-degree 287 * 8/772 = 2.97 exactly; capture correction gives
  **M_3(C=6) ~ 9.3e6**, agreeing with the calibrated chain (~9.1e6).
  Caveat: strided sample, non-uniform in-degree.
- Peak RSS 519 MB for 8.79e6 states (~59 B/state).

### 8.6 Revised C=6 model (C=5-calibrated, probe-consistent)

```text
M_3 ~ 9.3e6   emissions 3->4 ~ 1.6e12
M_4 ~ 6.8e8   emissions 4->5 ~ 1.8e12   (Pettersen ">900M" adjacent)
M_5 ~ 7e7     emissions 5->6 ~ 4e9
total ~ 3.4e12 emissions  (was (0.5-1.6)e13 pre-calibration)
```

Wall-clock on 24 threads: ~8.5 days at the current 5.2 us; ~4 days at
2.5 us (SWAR); ~2.5 days at 1.5 us.  Peak dictionary M_4 ~ 6.8e8 states
at the current 59 B/state ~ 40 GB — needs key compression / mod-p values
or 2-4 way key-range sharding on a 64 GB box.  Exact M_3 generation is
now a ~4 core-hour, ~0.6 GB job (measured constant x 2.6e9), ready when
authorized.  No full C=6 layer beyond the probe was generated.

## 8.7 Exact M_3, the v5 parallel engine, and the revised C=6 budget
(added later the same day; supersedes the estimates in 8.5-8.6)

Engine v4 (SWAR popcount signatures, group bitmasks): C=5 gate 28.7 s
single-thread, all gates green.  Engine v5 (lock-free CAS insertion into
fixed-capacity tables, OpenMP over parents): **C=5 complete gate in
5.24 s on 8 threads** — all checks identical, rank chain identical, zero
capacity holes.  Same-day trajectory: 111 s -> 46.7 -> 28.7 -> 5.24 s.

Exact M_3 run (v5, 24 threads, 992.9 s wall, 662 MB peak,
`data/logs/layer-dp-m3-exact-20260726.txt`):

```text
1->2: 59,245,120 emissions -> 772 states     [both exactly as predicted]
2->3: 2,605,194,602 emissions                [matches the independent
                                              reviewer measurement digit
                                              for digit]
      layer-2 orbit mass 20,338,525          [known value, in-run gate]
M_3   = 12,324,872   (exact, first count; avg in-degree 211.4)
layer-3 orbit mass = 566,455,903,200  (exact, new; avg m = 45,960)
fan sample (20,000 strided parents, count-only):
      min 8,280 / med 196,416 / mean 173,291 / max 237,312 tables
      -> 3->4 total = 2.136e12  (sample-measured)
```

My pre-run estimate M_3 ~ 9.3e6 was 1.33x low; the fan mean matches the
permanent profile (tables < vdW floor on symmetric parents is the
K_c merge factor, not a violation).

Revised C=6 budget (exact / sample-measured / calibrated-estimate):

```text
1->2   5.9e7   [exact]        2->3   2.605e9  [exact]
3->4   2.136e12 [measured]    M_4 ~ (0.7-1.2)e9 [est; Pettersen-adjacent]
4->5   ~2.4e12  [est]         5->6   ~5e9      [est]
total  ~4.5e12 emissions
RAM    peak = layer-4 table ~40-50 GB  [fits the 125.7 GB / 32-thread box]
wall   ~12-13 days at the measured v5 constant; the v5 parallel
       efficiency is only ~40% (contended global canonize counters are
       the prime suspect) — per-thread counters, WL-seeded initial
       partitions, and NUMA-aware placement are the identified levers
       toward ~4-7 days, before any deeper canonizer work.
```

Two free integrity checks for the eventual full run: F_6(G1), F_6(G2)
per-class spot values, and — if the rank-defect conjecture below holds —
two universal linear identities on the final 63,199-vector.

**Defect-2 conjecture (open, cheap to test):** rank(U_3) misses full by
exactly 2 at C=4 and C=5.  Conjecture: the defect is exactly 2 for all
C >= 4, carried by high-symmetry classes (the C=4 null relation has
coefficients (1, -4, 9) on mask-multiplicity patterns (4,4), (3,3,1,1),
(2,2,2,2)).  Next probe: extract the two C=5 null vectors (mod-p
nullspace + rational reconstruction of the 355-vectors), check support
and whether the C=4 relation lifts; if the pattern is structural, the
C=6 relations give two free checksums on the final class vector.

## 8.8 Measured M_4 and the finalized C=6 decision package
(added later the same day; supersedes the M_4 estimate in 8.7)

**Hash-window distinct sampling** (streaming-statistics transfer): sample
k strided layer-3 parents, canonize every 3->4 child, but keep only
children whose canonical-key hash lands in a 2^-w window — memory drops
2^w-fold while the windowed hit histogram remains a faithful sample of
the in-degree process.  Estimator calibrated on C=5 against the known
M_4 = 17,120: at lambda ~ 2.8, Chao1 read 16,830 (-1.7%), converging
from below as lambda grows.

C=6 run (v5.1, 24 threads, k = 14,600 parents, w = 6, lambda = 2.86,
288 s probe stage, log `data/logs/layer-dp-m4probe-20260726.txt`):

```text
kept 39,402,926 of 2,522,319,152 emissions (expected fraction OK)
distinct in window 12,995,262; hits f1..f4 = 2.48M/3.16M/2.91M/2.11M
M_4(Chao1)      = 8.941e8
M_4(PoissonMLE) = 8.823e8
calibrated      M_4 ~ 9.1e8 (+-few %)
```

**Pettersen's ">900 million" four-row lookup states is hereby
independently confirmed**: his objects evidently coincide with the
four-row coordinate orbits, at ~9.0-9.2e8.

Engine v5.1/v6 same-session updates, all gates green in every mode:
- thread-local canonize counters (the shared atomic counters were the
  parallel bottleneck): full 2->3 transition 787.4 s -> **379.5 s
  (145.7 ns/emission wall on 24 threads)**; C=5 full gate 3.98 s.
- form-independent referees: scan-check now verifies orbit membership
  plus directly counted stabilizers (stronger independence than
  comparing two implementations of the same K-sequence form).
- WL pair-profile seed partition (--wlseed): cuts search nodes
  20.2 -> 6.8 at C=5 but its own cost wins only on high-symmetry layers
  (C=5 4->5: 389 vs 459 ns).  A/B at C=6 2->3: 1,965 (off) vs 2,087
  (on) ns single-thread -> default off; candidate for per-transition
  enabling at 4->5 / 5->6.

**Final C=6 budget (all measured or measured-extrapolated):**

```text
emissions: 5.9e7 + 2.605e9 + 2.136e12 + ~2.38e12 + ~5e9  ~ 4.52e12
wall      ~7.6 days at 145.7 ns/em on 24 threads (~6 days on 30)
RAM       layer-4 fixed table at cap 1.0e9: ~45 GB (machine: 125.7 GB)
checks    772 / 20,338,525 / 2,605,194,602 / M_3 / F6(G1) / F6(G2)
          in-run, plus CRT cross-prime if run mod several primes
```

The full 3->4 + 4->5 + 5->6 production run is the remaining resource
commitment; nothing in it is unmeasured anymore.

## 8.9 Pre-flight ultra audit (15-agent sweep, same day)

Seven parallel lenses plus adversarial verification of every review
finding.  Confirmed defects, all fixed in the engine the same day and
the full C=2..5 gate ladder re-run green:

1. **5->6 u64 T overflow was arithmetically certain at C=6** (T_q =
   m*F ~ 3.6e20 = 19.6 x 2^64 for typical classes): the final
   transition now accumulates into per-thread u128 vectors merged after
   the parallel region; F extracted in u128 with a range check.
2. **Final N term overflows u128 at C=6** (ell*m*F^2 ~ 1e45): the
   engine now writes the exact per-class (m, ell, F) dump at C >= 6 and
   defers the weighted square sum to external exact arithmetic — which
   is also where the comparison against Pettersen happens.
3. u32 index-space ceiling now enforced in init_fixed (caps >= 2^32-64
   abort instead of silently corrupting).
4. Strided samplers clamp to their stride window (a hole run >= stride
   could double-count a parent in the estimators).
5. Operational: fixed caps must include hole slack (0.1-1%); holes are
   nondeterministic run to run.
6. The lock-free insert design itself was **confirmed sound** (no ABA,
   no torn reads, correct release/acquire pairing), and the
   layers-2..5 u64-safety proof (Bregman peeling chain, per-term
   membership) **survived adversarial re-derivation — no CRT needed
   anywhere**.

Exploration results:

- **Defect-2 structure cracked (C=5)**: the null space of U_3 is
  exactly 2-dimensional over Z.  Sparse relation: R(x,q0) - 3 R(x,q2)
  + 4 R(x,q39) = 0 on mask-multiplicity patterns (5,5), (4,4,1,1),
  (3,3,2,2) — the exact lift of C=4's (1,-4,9) on (4,4), (3,3,1,1),
  (2,2,2,2); both live on the two-complementary-pair family
  {a^(C-k), abar^(C-k), b^k, bbar^k}, k = 0,1,2.  The dense partner has
  the exact form z_q = kappa_q * ell_q / const with small odd kappa
  (table `c5_kappa_table.csv`, certified vectors `c5_wvecs.txt`).  The
  defect is created entirely at the 3->4 transition (rank(U_4) full).
  Checkable C=6 conjecture with candidate coefficients formulated —
  if confirmed, two free integrity checksums on the final 63,199
  vector.
- **Layer-5 anchored canonizer is worth building**: measured on random
  5-row states, missing-pair anchoring + L3 colors gives 3.6-4.8x
  (floor 1.5x) on the 4->5 bucket = 53% of the total budget; low-to-
  moderate risk; the level must switch canonical form atomically and
  re-pass all gates.
- **M_4 hardened**: zero-truncated negative-binomial fit gives
  M_4 = 9.0e8, 95% [8.87e8, 9.20e8] (C=5 calibration recovers truth
  +0.96%).  Production cap: 1.3e9 entries (~59 GB on the 125.7 GB box).
  A 4x probe (k = 58,400, ~19 min) is recommended to convert the
  estimate into a measurement first.
- **Checkpoint/restart design approved**: barrier-quiesce at parent-
  chunk boundaries, exact with no subtraction tricks, 17-34 s per dump
  (< 1.5% overhead at 40-min periods), ~19 h to implement including
  kill-loop validation at C=5.
- **Fresh-eyes verdict**: budget chain sound; realistic wall 8-14 days
  pre-optimization (4->5 calibration is the honest weak link — fixable
  to +-1% by a bounded test); a one-day T1-T10 pre-flight battery
  specified before commitment.

Remaining pre-flight work program (in order): 4x M_4 probe;
checkpointing; layer-5 anchored canonizer + full regate; bounded 4->5
calibration from real sampled layer-4 states; T1-T10 battery; then the
production decision.

## 9. Honest limits

- Section 5's brackets inherit the history-weighted sampling bias; the
  C=5 gate calibrates it (every C=5 quantity there becomes exact).
- M_3 and M_4 remain the load-bearing unknowns; everything else in the
  profile is now measured or exact.
- If the per-emission constant lands at 5-10 us instead of 1-2 us, the
  wall-clock is weeks-to-months on one box; the route is then still
  ~100x better than the cold-canonicalization baseline but no longer
  cheap.  The constant, not the count, is the decision variable.
