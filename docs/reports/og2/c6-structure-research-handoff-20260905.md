# C=6 精确计数：结构突破研究交接说明

Prepared: 2026-09-05, Asia/Shanghai. Numerical production baseline:
`STATUS.md` dated 2026-08-15, generation 37 / cursor 38.
Repository baseline: `ee5d173a789b03f5ac7d6afe76f759a99e14e8c7`.

## 给项目所有者的建议

按每天最多 8 小时、还需与其他工作共享电脑的约束，现有方案剩余约
111--124 小时的测量外推，对应约 14--16 个满额计算日，实际日历时间可能更长。
这是剩余任务的成本估算，不是完整 C=6 已经验证的工期。

建议保留当前检查点，把现有 DP 留作可恢复的后备方案，先用有限研究预算
征求结构性改进。已经投入的时间不要求继续投入同样多的时间。外部研究可以
先从定义和小 C 开始，不需要交出或加载 32.5 GB 的生产检查点。

优先询问是否能减少必须处理的转移，或直接精确计算最终加权平方和。
四行归一化锚定等工程优化仍值得有界对照，但即使只将 S1 加速两倍，
S2 的约两天连续计算量仍然存在，不能因此承诺把整个任务降到几个晚上。
建议先以 1--2 个工作日作为研究评估预算，而非无期限等待新数学。

下面的英文正文可以整份单独转交。它不依赖收件人拥有仓库；末尾的文件路径
只是可选证据定位。研究问题不等于已验证结论，也不授权启动或修改生产计算。

---

# Research request: can exact 2x6 Sudoku counting avoid trillions of transitions?

## 1. Requested outcome and practical constraint

We seek an independently reproducible exact count of labelled 2x6 Sudoku
grids. A verified global dynamic program is available, but its remaining
work occupies our workstation for roughly 111--124 hours by current
measurements. The owner can provide at most eight hours per day, and needs
the machine for other work. Finishing unchanged would consume approximately
14--16 fully dedicated eight-hour days, potentially more calendar days.

Please investigate a mathematically specified algorithm that reduces the
total work, rather than assuming that completing this existing computation
is a prerequisite for understanding its structure. A several-fold overall
reduction would already matter; a tenfold reduction would change the
practical decision. These are useful targets, not claims that such an
algorithm exists or mandatory thresholds for a worthwhile result.

Either of these output contracts is worth investigating:

1. All exact final class counts, with their weights, more efficiently.
2. The exact final scalar N(6), without explicitly computing every class
   count or every intermediate state, provided there is an independent
   verification strategy.

The second contract is deliberately allowed. Existing production tooling
uses a full class certificate, but research should not be forced to produce
that vector if it can rigorously avoid it. A scalar-only candidate would
need a different final certificate; this brief does not change the existing
production acceptance rules.

Do not assume a low-rank representation is necessary. Fast application of a
full-rank operator, an exact aggregate recurrence for the actual input, or a
direct scalar contraction could be useful. Conversely, merely naming tensor
networks, representation theory, inclusion-exclusion, graph isomorphism, or
meet-in-the-middle is not yet an algorithm.

## 2. The exact counting problem

We count labelled 2C by 2C Sudoku grids. For convenience transpose the usual
2-by-C boxes: the grid has two horizontal bands, each with C rows and C
boxes, each box having C rows and two columns. Transposition is a bijection,
so this is the same counting problem. Rows, columns, and symbols are labelled;
the answer is not a count modulo Sudoku symmetries.

Use n = 2C symbols. There are n column slots in a band, paired as
`(b,s)` with box `b in {0,...,C-1}` and side `s in {0,1}`.

An L-row incidence state x is a multiset of n masks u. Each mask is a subset
of these n slots, with exactly L elements and at most one element from each
box pair. Each slot occurs in exactly L masks. Multiset copies may be given
temporary distinct symbol labels when defining the following graph.

Let Q_x be the bipartite graph between n symbols and n slots, with an edge
when a symbol's mask contains a slot. It is a simple L-regular bipartite
graph. Define F_L(x) as its number of ordered decompositions into L perfect
matchings. Each matching is one row; row order is included. Equivalently,
F_L(x) counts proper edge colorings with L fixed, labelled row colors.
Relabelling the temporary symbol copies leaves this number unchanged.

Every balanced mask multiset of this form is row-reachable: a regular
bipartite graph decomposes into perfect matchings, and the mask restriction
already enforces no repeated symbol within a box. In particular F_1 = 1;
for L = 2, F_2 = 2 to the number of cycle components of Q_x.

The coordinate group is `G_C = C_2 wr S_C`, of order `2^C C!`. It permutes
boxes and independently swaps their two sides. Symbol permutations have
already been quotiented by storing a multiset. For a coordinate orbit x,
write

```text
s_x = |Stab_G(x)|,
m_x = |G_C| / s_x,
c_u(x) = multiplicity of mask u,
ell_x = n! / product_u c_u(x)!.
```

Thus `m_x ell_x` is the number of symbol-labelled incidences in that orbit.
It is not a factor to divide the final answer by.

For a complete state q (L = C), every symbol occupies exactly one side of
each box. The second band's symbol set in each slot is the complement of
the first band's set. This complement graph is obtained by swapping both
sides in every box, so its ordered factorization count is also F_C(q).
Therefore the required identity is

```text
N(C) = sum_[q] m_q ell_q F_C(q)^2.
```

The square is essential. The corresponding linear sum counts single-band
objects and is not N(C). An independent weight check is

```text
sum_[q] m_q ell_q = binomial(2C,C)^C.
```

There is no missing row-order, symbol-factorial, or box-permutation factor
in these formulas.

## 3. A complete specification of the current global recurrence

The stored coefficient is the coordinate-orbit total

```text
T_L(x) = m_x F_L(x).
```

There is one one-row orbit x1, with T_1(x1) = 1.

For a representative x, group equal masks u with multiplicity a_u. For
each u choose a subset S_u of slots satisfying all of the following:

- |S_u| = a_u;
- every chosen slot belongs to a box absent from u;
- all S_u are disjoint, and their union is the full set of n slots.

This is a contingency table with entries `n_(u,v) in {0,1}` and column
sums one. Do not add permutations of the a_u identical input copies.

The emitted child multiset z contains one mask `u union {v}` for each
`v in S_u`. Let d_w be its output-mask multiplicities. Its exact coefficient
for this emission is

```text
K(z) = product_w d_w!.
```

This is the one-row specialization of
`product_w d_w! / product_(u,v) n_(u,v)!`; the denominator is one here.
Using the input multiplicities in place of the output factorials gives the
wrong normalization.

Canonicalize z under G_C to obtain y, and accumulate

```text
T_(L+1)(y) += T_L(x) K(z).
```

Equivalently, with R_L(x,y) the sum of K over all emissions from x landing
in orbit y,

```text
T_(L+1)(y) = sum_x T_L(x) R_L(x,y).
F_C(q) = T_C(q) / m_q,
N(C) = sum_q (ell_q / m_q) T_C(q)^2.
```

The final division `T_C(q)/m_q` is exact. `ell_q/m_q` alone need not be an
integer. Implement the final sum with exact arithmetic, not floating point.
The current implementation uses 64-bit intermediate T values through C=6
layer 5, 128-bit final T values, and arbitrary precision for the final
weighted squares, which can exceed 128 bits.

Independent reference pseudocode (a full group scan suffices at small C):

```text
T = { canonical(multiset of all singleton slots): 1 }
for L = 1,...,C-1:
    next = empty exact-integer dictionary
    for (x, tx) in T:
        for each feasible disjoint family (S_u) defined above:
            z = multiset(u union {v} for u and v in S_u)
            k = product(factorial(multiplicity(w,z)) for distinct w in z)
            next[canonical(z)] += tx * k
    T = next
answer = 0
for (q,tq) in T:
    m = (2^C C!) / stabilizer_order(q)
    ell = (2C)! / product(factorial(multiplicity(w,q)))
    assert tq is divisible by m
    answer += m * ell * (tq/m)^2
```

Canonical means choose one representative consistently from the entire
signed-coordinate orbit, then sort the multiset. A different canonical
convention is acceptable for independent mathematics, but comparisons must
identify orbits instead of comparing incompatible key bytes.

## 4. Small cases and exact acceptance anchors

Complete native layer sizes and totals are:

| C | M_1,...,M_C | exact N(C) |
|---:|---|---:|
| 2 | 1, 2 | 288 |
| 3 | 1, 5, 4 | 28200960 |
| 4 | 1, 23, 54, 26 | 29136487207403520 |
| 5 | 1, 107, 16150, 17120, 355 | 1903816047972624930994913280000 |

Native emission counts, using precisely the grouped one-row convention in
section 3, are:

```text
C=2: 4
C=3: 80, 29
C=4: 4752, 4630, 712
C=5: 440192, 2324325, 11957632, 462403
```

A minimal normalization test is C=2. Represent a complete mask by a two-bit
word giving its side in each box. The two complete orbits may be represented
as follows:

```text
qA = {0,0,3,3}:  s=4, m=2, ell=6,  F=4, T=8, contribution=192
qB = {0,1,2,3}:  s=8, m=1, ell=24, F=2, T=2, contribution=96
N(2) = 192 + 96 = 288.
```

At C=6, exactly 63,199 complete orbits are independently known. Two
individual factorization counts have been closed by other engines:

```text
G1 words: 0 5 10 19 29 30 39 43 44 48 54 57
G1 stabilizer = 120; m = 384; F_6 = 6986348258918400

G2 words: 0 5 10 23 27 28 35 44 47 48 54 57
G2 stabilizer = 8;   m = 5760; F_6 = 7053808087203840
```

Here bit b of each six-bit word is the side chosen in box b. These lists
fully specify the graphs without relying on a repository class index.

The repository's historical literature audit records Pettersen's announced
value (2006-11-14):

```text
N(6) = 38296278920738107863746324732012492486187417600000.
```

The present project has NOT independently verified that full value. Treat
it as the historical comparison target, never as input to a reconstruction
or a substitute for independent computation. The audit did not locate
buildable historical source. No new literature search was conducted for
this handoff.

## 5. Current C=6 computation: measured work, not a complexity lower bound

| Layer | number of native coordinate orbits | status |
|---|---:|---|
| L1 | 1 | exact |
| L2 | 772 | exact |
| L3 | 12,324,872 | complete exact snapshot |
| L4 | at least 903,398,602 | observed distinct keys in partial production; full count not closed |
| L5 | 96,452,755 | independent exact Burnside count; production weights not computed |
| L6 | 63,199 | exact outer inventory; global F vector not computed |

The older sample estimate M4 = 902,863,734 is now slightly below the observed
production lower bound. It was an estimate, not an upper bound or a proof of
completion. The 1.35-billion table capacity is operational headroom, not a
theorem about the exact population.

The complete initial transitions used:

```text
1->2:    59,245,120 native emissions
2->3: 2,605,194,602 native emissions
L3 sum of coordinate orbit sizes: 566,455,903,200
```

The reusable exact L3 snapshot was produced in 521.3 seconds under a 2-GiB,
15-minute guard, with 661.1 MiB peak working set. The dominant projections
are instead

```text
3->4: approximately 2.13308e12 native emissions
4->5: approximately 2.36323e12 native emissions
```

The second projection uses a nearly saturated canonical hash window and a
20,000-state approximately uniform L4 sample: mean fan 2617.482, sampling
SE 4.408. A much larger sparse rehearsal gave mean 2617.908, corroborating
the scale. These are measured projections, not exact complete-layer totals.

Current production is S1, transition 3->4, generation 37:

```text
completed parent chunks: 38/124, with chunk size 100000
cumulative emissions:   704741992192
real child keys:         903398602
checkpoint bytes:       32522350448
last full window:        8.4374 hours on 24 threads
window emissions:       219681931472
window new keys:         7
peak RSS:                61.856 GiB
```

T4 coefficients are sums over the processed parent prefix only. No closed
L4 snapshot exists. Near-saturation of the key set does not make its weights
complete, and sampling the remaining parents does not give an exact answer.

Each new native emission normally sorts the child masks, canonicalizes,
looks up the canonical child, and accumulates an exact contribution. The
per-parent raw-child cache resets at every parent; its cumulative production
hit rate is about 0.00486%. This does not measure global orbit reuse.
Measured checkpoint writing occupied about 0.8% of the last window.

At the last window's average rate, remaining S1 work projects to roughly
55 hours by emission estimate or 60 hours by chunk count. Existing S2
measurements of 84.8--96.7 ns/emission on 24 threads suggest 56--64 hours.
Production scheduling, memory behavior, and validation add uncertainty;
these numbers are planning baselines, not confidence bounds or guaranteed
durations. S3 is much smaller in emission count, but remains uncompleted.

A denominator-100 rehearsal already traversed all three stages with sparse
parent selection, restart, replay, and external exact arithmetic. It reached
903,346,741 L4 keys, 96,452,753 L5 keys and 63,117 final classes. Its weights
belong to an artificial sparse operator, not the full problem. This is strong
evidence that discovering most keys is much cheaper than summing all weights.

## 6. Prior negative results and exactly what they exclude

The following entries concern DIFFERENT state spaces and operators. Do not
identify their dimensions just because several are called a band kernel.

| Proposal or representation | established evidence | scope of the conclusion |
|---|---|---|
| 132-state one-copy marginal | Outputs 96 and 460800 at C=2,3 instead of N(C) | Computes a linear sum, not the required square |
| Natural 3+3 irreducible-channel truncation | Exact ranks 630/630 at C=4 and 8001/8001 at C=5; symmetry forces no channel loss at C=6 | Ordinary small-rank truncation unsupported; fast full-rank application is not excluded |
| Reduced paired band kernel | C=5 middle map is 38801 by 38801 with certified rational rank at least 1024 | Excludes a few-hundred-channel factorization, not all structured transforms or full rank |
| Native layer-DP completion operator | C=5 U4 is 17120 by 355, rank 355; U3 is 16150 by 355, rank 353; C=4 U3 is 54 by 26, rank 24 | Exactly two final response relations at the measured C=4,5 boundary; no C=6 corank theorem is proved here |
| Independent sweep of 63,199 outer classes | G2 alone has 221,438,460 tail states and a 9.08-GB frontier; even its fully cached tail takes 824.440 seconds | Per-class closure does not supply affordable broad reuse; G1/G2 symmetry is not representative |
| Shared G1/G2 half-kernel table | Later-class coverage can be zero; a ten-class sample had no repeated selected kernel pair or complete relative signature | This fixed table and naive pair batching do not generalize |
| Class-local 2+4 with shared F4 memo | Ordinary graphs have about 1.33--1.39 billion two-factors; later sampled F4 keys hit the old memo only 2--4%; a ten-class union saves 0.56% | That top-down enumeration/memo design fails; this is not a universal theorem about global F4 organization |
| Immediate reverse 2+2 gluing | 772 two-row orbits; 276 have trivial stabilizer; their 38226 unordered pairs alone require 1761454080 relative placements; a generic 100k prefix is 99.4% distinct after canonicalization | Brute pairwise relative-placement enumeration is too large; streaming alone does not reduce its arithmetic |
| Connectivity plus naive subset/permanent operator | At C=5 symbol 3 there are 83776 states; 857244 assignments give 857244 raw targets and zero merged subset prefixes | Given the fixed source, partial target degrees recover processed choices; that subset state does not compress |
| Target-labelled joint-histogram lift | Six of seven C=5 sources exceed 10M states; the closed source has 6323400 raw targets from 6516556 leaves | Retaining target identity defeats the small scalar allocation DP |
| Fixed-source paired column frontier | A reachable trivial-stabilizer C=6 source forces 5489549616 labelled terminal targets and at least 42191464 fixed-3+3 flattening rank | Applies to that fixed-source, full-target linear output contract, including signed reorderings with the same semantics |
| Full coherent-configuration / orbital materialization | The trivial-stabilizer reverse-gluing pairs already expose 1761454080 relative coordinates | A full change of basis does not reduce dimension; an actually smaller downstream response representation remains a separate question |
| Generic PSD / Kraus rewriting | Algebraically exact, but proposed factors reproduced already large history-to-boundary maps | Positivity or a Gram identity alone is not a compressed algorithm |

For clarity, the fixed-source frontier theorem concerns paired states
`((S_i,T_i))_(i=1..2C)` in a separate box-order recurrence. A new symbol-to-box
map avoids S_i (or T_i) and assigns two symbols to each box. Its map count is
`permanent(B_S)/2^C`, where B_S has two identical available-slot columns per
box. The complete labelled target recovers both maps by set difference.
A retained witness has map counts 74119 and 74064, with minimum three-box
projected supports 6488 and 6503. Their products give the bounds above.
This is NOT a lower bound on applying native R_L to the actual T_L vector,
or on the final scalar after summing over sources.

In particular, do not infer either of these stronger claims:

- "C=4,5 have corank two, therefore every C=6 algorithm is expensive."
- "A fixed-source polynomial has huge support, therefore the actual
  globally aggregated scalar must materialize that support."

Neither follows. Matrix rank is a dimension obstruction for specified linear
representations; it is not a general arithmetic circuit lower bound.

## 7. Concrete open questions worth attempting

These are prompts for constructions, not a claim that none has ever been
considered in related mathematics. The repository contains no validated
implementation that resolves them at the required scale.

### A. Can the square be contracted directly on the actual reachable vector?

Take T_L as a row vector and define

```text
U_L = R_L R_(L+1) ... R_(C-1),
D = diag(ell_q / m_q),
B_L = U_L D U_L^T.
Then N(C) = T_L B_L T_L^T.
```

The identity is elementary; the open problem is an inexpensive exact
representation/application for this scalar on the actual reachable T_L.
Explicitly forming U_L, B_L, or an arbitrary large Gram factor defeats the
purpose. Summing independent squared contributions also fails, because the
cross-source terms in the square must survive.

Could a special response algebra, an invariant recurrence, coefficient
extraction, or a direct two-copy construction compute this scalar without
paying for all native L4/L5 coefficients? Please specify the representation,
closure rule, boundary weight D, and the size of its reachable state space.
Any simpler moment or marginal must be proved sufficient for this scalar.

### B. Can weighted incoming contributions be summed in bulk?

At production scale almost every child already exists, but roughly trillions
of contributions remain. Is there structure in the weighted incidence
`T_L(x) R_L(x,y)` that permits aggregation before the expensive per-emission
canonicalization or before enumerating every contribution?

A reverse/incoming recurrence, an incidence transform, grouped orbit
transitions, or direct evaluation of F_L may be acceptable, but must count
its actual incoming work. Merely transposing the same sparse operator,
selecting one canonical parent per child, or observing that keys repeat is
not enough. Selecting one parent loses other histories unless their exact
weights are recovered by a proved recurrence.

### C. Does the missing-box structure support a smaller weighted model?

At C=6,L=4, each of the twelve symbols misses two boxes and each box is
missed by four symbols. The missing pairs form a loopless 4-regular
multigraph on six box vertices, with twelve symbol edges, together with
side-choice decorations on the other four boxes of each symbol.

At L=5, each box is missed by exactly two symbols. This structure already
supports the exact Burnside state count and the production five-row
canonicalization anchor.

Can the L4 multigraph and its decorations be summed by an exact local
invariant, finite transfer algebra, or another efficiently evaluable
factorization? Forgetting the decorations is not automatically sufficient:
prove that the retained quantities preserve all required weighted responses,
or exhibit a separating small-C counterexample quickly.

The USED incidence graph is L-regular; the FREE next-row compatibility graph
is `2(C-L)`-regular on each bipartition. A permanent counts one next-row
matching, whereas F_L counts a full ordered factorization. Do not substitute
one for the other without an identity. The grouped native fan can be smaller
than the labelled permanent because identical masks are not distinguished.

### D. If the emission count survives, can its cost fall enough overall?

The retained C=6 five-row canonicalizer uses missing-pair invariants; the
four-row analogue has only an old prototype suggestion, not a validated
production improvement. The prior report's search-candidate reductions
cannot be treated as C++ end-to-end speedups. Additional invariant work,
hash lookups, memory traffic and atomic additions must be charged.

Exact semicanonical buckets with bounded multiplicity, a cheap invariant
index with exact collision resolution, or a faster compatible canonicalizer
could matter. State the bucket blowup, construction time, exact fallback,
and the effect on BOTH major stages. A probabilistic fingerprint alone is
not exact orbit identification. A new key convention must also account for
migrating or replacing existing partial production state.

## 8. Requested response and decision gates

Please return your strongest one or two concrete candidates, or a precise
negative result, rather than an unranked catalogue of techniques.

For each candidate provide:

1. **Object being computed:** all F values, the actual aggregate vector,
   or only N; define every retained state and normalization.
2. **Exact mechanism:** equations or executable pseudocode, including base
   cases and how cross terms or multiplicities survive.
3. **Why prior obstructions do not apply:** identify the specific avoided
   output contract or eliminated work, not just a different algorithm name.
4. **Smallest decisive test:** complete C=2..4; if claiming an operator
   identity, compare its coefficients or action, not only one final scalar.
   A scalar-only method needs an independent derivation/test appropriate to
   its own contract and need not manufacture a full class vector.
5. **C=5 evidence before a large C=6 attempt:** reproduce exact N(5), and all
   355 class triples if the method computes class values. Report state counts,
   operations/emissions, canonical calls, wall time, threads and peak RAM.
6. **C=6 projection and falsification criterion:** show whether trillions of
   transitions disappear or are merely cheaper. Include preprocessing,
   intermediate storage, merging, exact arithmetic and final verification.
   Specify what bounded sample would cause the idea to be abandoned.
7. **Practical net gain:** separate cold-run cost, remaining-run cost using
   the existing checkpoint, and implementation/migration cost. Do not count
   saved past work as future savings, or assume the workstation runs 24/7.
8. **Independent certificate:** explain how the complete result would be
   checked without assuming the historical decimal is correct.

A theorem or counterexample requiring no large computation is especially
welcome. A candidate need not preserve the existing DP representation.
The saved computation is a recoverable fallback, not a mathematical
constraint on a better algorithm.

## 9. Optional repository evidence and operational handoff

The mathematical specification above is sufficient to begin without these
files. For a recipient who also receives the repository:

- `STATUS.md`: authoritative production state and route decisions.
- `docs/methods/layer-dp.md`: retained recurrence and canonicalization gates.
- `experiments/proto/layer_dp_gate.cpp`: current global implementation.
- `docs/methods/reverse-gluing.md`: graph meaning of F and contingency weights.
- `docs/methods/factorization-orbit.md`: independent per-graph engine.
- `docs/math/penultimate-layer-burnside.md`: exact M5 construction.
- `docs/methods/layer-dp-certificate.md`: complete-class representation and checks.
- `docs/reports/og2/layer-dp-c6-production-s1-window4-20260810.md`: current window.
- `docs/reports/og2/layer-dp-m4-random-calibration-20260801.md`: uniform scale probes.
- `docs/reports/og2/layer-dp-c6-bounded-end-to-end-rehearsal-20260802.md`: sparse rehearsal.
- `docs/reports/og2/source-target-frontier-lower-bound-20260720.md`: scoped theorem.
- `docs/reports/og2/literature-audit-20260712.md`: historical claim and source links.
- `docs/expert/2026-07-27/layer5-anchor-design.md`: raw L4/L5 anchor proposal,
  not authoritative performance evidence.

Old documents can contain superseded launch restrictions or overbroad route
conclusions. Use current code/tests and STATUS for implementation state;
do not promote an extrapolated C=6 statement to a proved theorem.

Active partial S1 identity, for preservation rather than experimentation:

```text
base: layer_dp_c6_s1_prod_20260802
generation 37 (.a), cursor 38/124
SHA-256: ECF0837315B0FDF8AE21C394FDA6676490E43B1828A0825529A344EC17E4E844
older fallback: generation 36 (.b)
immutable parent: layer_dp_c6_layer3_20260731.snap
parent SHA-256: 1D882DB7B0B18490E981539F7C690FA90227ECEC243817F5B538BEC8DAC865B7
```

These hashes are the previously verified manifest identities, not new full
hashes computed for this document. External physical backups are recorded in
`data/checkpoints/MANIFEST.md`. Do not overwrite, migrate, recanonicalize,
or use partial T4 as a closed memo. This request authorizes research advice
and preparation of a handoff, not a new production window or external sending.
Any future writable C=6 experiment needs separately scoped time/RAM bounds,
the required exact gates, and checkpoint protection.

## 10. Checks performed while preparing this brief

The existing executable reproduced the complete C=5 inventory and value,
all 355 reference triples, and the following explicit sample gates:

```powershell
.\build\layer_dp_gate.exe 5 --threads 8 --caps 200,20000,20000,600 `
  --ref docs/expert/2026-07-21/native_c5_response_quotient_triples.csv `
  --invariance 100 --scan-check 100
```

Result: `ALL GATES PASSED for C=5`; zero invariance/stabilizer failures;
canonical separation conflicts zero and histogram match. A separate
`.\build\layer_dp_gate.exe 6 --bridge-only` reproduced the two representative
lists and stabilizers in section 4. That bridge check does not recompute
their factorization values or any global C=6 layer.

The mandatory complete repository gate was also run:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts\verify_all.ps1
```

Result: exit code 0, `ALL REPOSITORY CHECKS PASSED`. This rebuilt the active
programs and exercised the independent small-C engines, layer-DP numerical
and checkpoint gates, FJ9 reproduction, and the bounded C=6 graph-memo
read-only check. The latter returned F6(G1) = 6986348258918400 and verified
the graph-memo SHA-256 was unchanged. It did not resume production S1 or
recompute the full C=6 sum. No optional complete-gate component was skipped.

The changes for this handoff are documentation only: this new brief and its
entry in `docs/index.md`. No algorithm, production status, or checkpoint
manifest was changed; no commit was created. The pre-existing untracked
`paper/c5-open-verification/` directory was left untouched.

No new compression theorem, C=6 production result, or speedup is claimed
by this handoff.
