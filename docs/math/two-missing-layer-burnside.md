# Exact two-missing-box layer census

The independent standard-library counter
[`layer_two_missing_burnside.py`](../../experiments/proto/layer_two_missing_burnside.py)
counts native coordinate orbits at layer `L=C-2`. It does not compute layer
weights, factorization values, or `N(C)`, and performs no checkpoint I/O.

For C=6 the exact result is

```text
raw native states       = 41,602,261,536,160
Burnside numerator      = 41,628,607,626,240
coordinate group order  = 46,080
native four-row orbits  = 903,398,603
```

The distinction between raw states and orbits matters: symbol labels have
already been removed in both counts. Raw states retain labelled box/side
coordinates; the orbit count also quotients `C_2 wr S_C`.

## Definition and missing graph

A native state is a multiset of `2C` occurrence masks, each occupying `L`
distinct boxes, one side per occupied box. Each slot has degree `L`. At
`L=C-2`, every mask misses exactly two boxes and each box is missed by exactly
four masks. Consequently the missing pairs form a loopless 4-regular
multigraph H on C labelled boxes. Its edge multiplicities sum to `2C` and
each multiplicity is at most four.

The elementary row-degree recursion in the counter enumerates all such H
once. There are 1, 1, 15, 158, and 3,355 labelled H for C=2 through C=6.
No graph-isomorphism catalogue is an input.

For an edge e of multiplicity m, its symbols form a multiset of m side words
on the `C-2` occupied boxes. Specifying these word multisets for every edge
determines a unique raw native state, provided the side counts balance.
Repeated masks contribute no extra factorial: they are multiset elements.

## Identity fixed points

Write `h_m` for the complete homogeneous symmetric polynomial. For a
missing-edge group e, its decoration inventory is

\[
h_m\!\left(\left\{\prod_{b\notin e}x_b^{v_b}:
v_b\in\{0,1\}\right\}\right)
=\sum_{\lambda\vdash m}\frac1{z_\lambda}
\prod_{d\in\lambda}\prod_{b\notin e}(1+x_b^d),
\]

where `z_lambda = product_d d^(a_d) a_d!` and `a_d` is the multiplicity of d
in lambda. This is the permutation-cycle count for multisets: Burnside on
m temporary labels assigns one side word to each permutation cycle, and a
cycle of length d repeats that word d times.

Multiply the inventories over the missing-edge groups and extract the
coefficient of `product_b x_b^(C-2)`. The missing graph already fixes total
box occupancy at `2(C-2)`, so this enforces both side degrees. After choosing
one partition for each edge group, coefficient extraction factors into C
univariate extractions.

The implementation multiplies each partition term by `m!/z_lambda`, then
divides the complete graph numerator by `product_e m_e!`; it checks exact
division for every H. No floating-point arithmetic enters a count.

At C=6 there are only 53,500 partition terms in total. The worst individual
H requires at most 125 terms, attained by three multiplicity-four groups.

## Nonidentity signed fixed points

A group element acts by

\[
g(b,s)=(\sigma(b),s\mathbin{\mathrm{xor}}f_b).
\]

First require H to be invariant under sigma. Its multiplicities must be
constant along each orbit of unordered missing pairs. Consider an edge
orbit of length r with multiplicity m and choose one representative e.
Its word multiset must be invariant under `g^r`. Conversely, any such
multiset propagates uniquely around the r edges and is consistent on
returning to e. This is a bijection; no transporter or stabilizer factor
is missing.

The code constructs the `g^r` orbits of the `2^(C-2)` side words. A word
orbit of length d must occur with constant multiplicity k, consuming `dk`
of the m symbols. Enumerate all nonnegative choices with total consumption
m, propagate their masks through the r edges, and record side-one degree
contributions. Different word choices with the same degree vector add
their integer multiplicities.

For the resulting g-invariant state, side degrees along a signed box cycle
are related by equality or complementation relative to total occupancy.
A positive cycle needs one tracked degree. A negative cycle forces its
degree to exactly half the occupancy and needs no tracked coordinate.
Thus the target vector has one `C-2` entry per positive cycle. For a
nonidentity C=6 element it has at most five entries, and the capped DP has
at most `5^5=3,125` states.

Multiplying the edge-orbit inventories and extracting this target counts
every fixed native state exactly once. The enumeration omits negative-cycle
coordinates only because their balance follows from invariance and the
already enforced 4-regular missing graph.

## Burnside sum

Signed conjugacy types are a pair of partitions `(lambda_plus,lambda_minus)`
whose total size is C. Their centralizer orders are

\[
2^{\ell(\lambda_+)+\ell(\lambda_-)}z_{\lambda_+}z_{\lambda_-}.
\]

Indeed, each cycle has its cyclic rotations and a common sign change, and
equal signed cycles can be permuted. The class size is the group order
divided by this centralizer. The script constructs one representative per
type, checks that all class sizes sum to `2^C C!`, sums class size times
fixed points, and checks exact division by the group order.

The complete differential sequence is:

| C | Layer | Raw native states | Burnside numerator | Coordinate orbits |
|---:|---:|---:|---:|---:|
| 2 | 0 | 1 | 8 | 1 |
| 3 | 1 | 1 | 48 | 1 |
| 4 | 2 | 2,019 | 8,832 | 23 |
| 5 | 3 | 59,661,280 | 62,016,000 | 16,150 |
| 6 | 4 | 41,602,261,536,160 | 41,628,607,626,240 | 903,398,603 |

C=2 is the auxiliary empty layer. The C=4 and C=5 orbit totals agree with
the independently materialized layer-DP inventories. Regular bipartite
factorization ensures every balanced state is row-reachable; the census
does not include unattainable native states.

## Verification and bounded use

Quick complete C=2..5 gate, including an independent C=4 referee:

```powershell
python experiments/proto/layer_two_missing_burnside.py --max-c 5 --self-test
```

Complete C=2..6 count, with explicit bounds:

```powershell
python experiments/proto/layer_two_missing_burnside.py --self-test --time-limit 180 --max-states 5000
```

Add `--terms` to print every signed fixed-point term. The defaults are C=6,
180 seconds, and a 5,000-state limit per degree frontier. Time checks are
cooperative and include inner DP boundaries. A bounded stop exits nonzero
and does not accept a partial Burnside sum. The script writes only stdout
and stderr and neither opens nor creates checkpoints.

The optional self-test constructs all 2,019 balanced C=4 raw mask multisets
directly, without missing graphs or cycle indices, and compares all twenty
signed fixed-point counts individually with the main algorithm.

During the initial 2026-09-05 read-only analysis, the complete combined
prototype reproduced the table above, with the C=6 signed sum taking
56.773 seconds (plus approximately 0.33 seconds for the identity term).
A separate direct multiset polynomial calculation on the 24 unlabelled
missing-graph representatives independently reproduced the C=6 raw count
in 4.904 seconds, without using the cycle-index identity. An independent
240-mask-orbit generating function also reproduced eleven signed fixed
points; these are retained as exact regression values in the script.
Those analysis checks did not read any production state.

The retained script was then executed on 2026-09-05 with the complete
command above. It passed the direct C=4 differential, all eleven retained
C=6 fixed-point checks, and every C=2..6 inventory check. Its final output
was:

```text
COMPLETE C=6 L=4 H=3355 signed_types=65 identity_terms=53500 raw=41602261536160 numerator=41628607626240 orbits=903398603 peak_dp=3125 checked_C6_terms=11 seconds=53.179312
PASS max_C=6 elapsed=53.440704s no_checkpoint_io=1
```

The quick `--max-c 5 --self-test` command completed in 0.260706 seconds.
Two separate failure checks also passed: `--time-limit 0.0001` exited with
code 1 on the wall-time guard, and `--max-states 1` exited with code 1 on a
two-state frontier. Both reported that no partial orbit total was accepted.

## What this certifies, and what it does not

The August 15 production status records 903,398,602 real, distinct canonical
L4 keys. Assuming the established key legality and uniqueness guarantees,
that catalogue is exactly one coordinate orbit short of the complete
support. This statement compares the reported count with the independent
census; this counter has not inspected the production image.

There is also an independent weighted support certificate. If a set of
distinct legal canonical keys has stabilizers `s_x`, its coordinate mass is

\[
\sum_x 46080/s_x.
\]

Equality to 41,602,261,536,160 proves complete support, since omitted orbits
have strictly positive size. If exactly one orbit is missing, the deficit
is its size, and `46080 / deficit` is its stabilizer order.

Neither certificate closes the unfinished values `T_4(x)`. The identity
`T_4(x)=m_x F_4(x)` suggests separately evaluating exact F4 values on a
certified catalogue, but its production cost must be benchmarked. This
document establishes the finite support census, not a speedup claim or a
completed N(6) computation.

## Penultimate raw mass and constructive witnesses

The companion
[`layer_support_witnesses.py`](../../experiments/proto/layer_support_witnesses.py)
constructs two L5 witnesses and checks their full coordinate orbits without
reading a catalogue. It also computes the exact raw L5 mass independently
of the earlier penultimate Burnside numerator.

Each missing-box group contains an unordered pair of words. In its cycle
index, choose either the identity term or the transposition term. If k of
the six groups use the latter, define

\[
f(t)=[x^5](1+x)^{10-2t}(1+x^2)^t,
\qquad (f(0),\ldots,f(5))=(252,112,52,24,12,0).
\]

The raw count is therefore

\[
\frac1{64}\sum_{k=0}^{6}\binom6k
f(k-1)^k f(k)^{6-k}
=\frac{284158216900608}{64}
=4,439,972,139,072.
\]

At the endpoints, a factor with exponent zero is omitted. This raw count
is not the penultimate Burnside numerator 4,444,542,950,400.

The first witness consists of the all-side-zero and all-side-one masks
omitting each box. Its used graph is two copies of `K_(6,6)` minus a perfect
matching. The second witness uses the following symmetric matrix:

```text
 0  1  1  1  1  1
 1  0  1 -1 -1  1
 1  1  0  1 -1 -1
 1 -1  1  0  1 -1
 1 -1 -1  1  0  1
 1  1 -1 -1  1  0
```

Its square is `5I`, checked with integer arithmetic. For each row, the
diagonal marks the missing box; the off-diagonal signs give a side word
and its complement. Complementary pairs immediately enforce side balance.
The two sorted mask lists are:

```text
disconnected:
341 682 1109 1301 1349 1361 1364 2218 2602 2698 2722 2728

conference:
421 602 1129 1364 1418 1574 1681 2198 2329 2402 2629 2728
```

Scanning all 46,080 coordinate transformations gives stabilizers 1,440
and 240, hence orbit sizes 32 and 192. The full script, including its L4
sector inventory below, completed in 0.523600 seconds on 2026-09-05.

A separate read-only audit reported 96,452,753 real rehearsal L5 keys with
raw coordinate mass 4,439,972,138,848. Both constructed witness orbits were
absent from that catalogue. The two witnesses supply exactly its missing
two orbit classes and missing mass `32+192=224`. Consequently these are
sufficient to repair the support, assuming the audited key legality,
uniqueness, and stabilizer guarantees. Their construction does not repair
the rehearsal's intentionally incomplete weights.

The same script exhausts the L4 sector with two used-graph components of
six vertices on each bipartition side. Each side component is the
complement of a 2-regular missing multigraph on six boxes. There are 130
labelled such graphs and 47 unordered-pair coordinate classes. Exactly
five classes have stabilizer 12; a separate membership audit found all
five already present. This excludes that particular component sector as
the location of the missing L4 key. It does not assert that the missing
state is connected, and does not exclude a `4+8` component split.

## Complete order-three search for the missing L4 orbit

The audited production L4 raw mass is 41,602,261,532,320. The difference
from the independently counted complete mass is 3,840. Since exactly one
orbit is missing, its stabilizer must have order `46080/3840=12`.

Every group of order divisible by three has a nonidentity element of
order three. An elementary proof counts triples `(a,b,c)` with `abc=1`:
there are `|H|^2` triples, and cyclic rotation partitions them into orbits
of size one or three. Fixed triples are `(a,a,a)` with `a^3=1`. Their
number is divisible by three and includes the identity triple, so there
are nonidentity solutions.

An order-three element in `C_2 wr S_6` has one or two positive box cycles
of length three, positive fixed boxes, and no negative cycle. It is
therefore conjugate to either `+(3,1,1,1)` or `+(3,3)`. Every native orbit
with stabilizer 12 consequently has a representative fixed by one of
these two chosen elements. This covers all component shapes.

The retained
[`layer_order3_witnesses.py`](../../experiments/proto/layer_order3_witnesses.py)
enumerates both complete fixed-state sets directly:

- For `+(3,3)`, all 240 legal masks lie in 80 three-cycles. Choose four
  mask orbits with replacement and enforce degree four on one slot from
  each of the four slot cycles.
- For `+(3,1,1,1)`, there are 76 three-cycles and twelve singleton masks.
  Each singleton occupies the first three boxes on one common side and
  one of the six fixed slots. Once the three-cycle masks are selected,
  singleton multiplicities are exactly a `2 by 6` nonnegative contingency
  table with prescribed margins. Enumerating these tables completes each
  state without a larger target-distinguishing DP.

The script independently checks every emitted state's twelve slot degrees,
its fixed-point property, and uniqueness within each type. The counts
match the independent Burnside terms 280 and 41,620. On 2026-09-05:

```text
FIXED plus=(3,1,1,1) masks=240 singleton_orbits=12 triple_orbits=76 states=280
FIXED plus=(3,3) masks=240 singleton_orbits=0 triple_orbits=80 states=41620
PASS records=41900 distinct_raw=41896 visits=92646 elapsed=1.023278s checkpoint_io=0
```

The four cross-type duplicates are intentional. Native canonicalization
and stabilizer filtering of this finite union can therefore produce an
exhaustive membership target set for the missing orbit. No conjecture
about its graph shape is required.

The generated 2,340,012-byte record file was
`data/logs/c6-direct-route-20260905/l4-order3-fixed.txt`, with SHA-256
`FDC93524A3F3FFDB0EC29BC6900AF4914B32703D9833C19DEE6552299DA40B90`.
It contains twelve space-separated true slot masks per line, not the
engine's packed native keys. Generated logs remain outside ordinary Git
history. The generator never opens production checkpoints and refuses to
replace an existing output file.

Read-only reconstruction, with a 180-second wall bound, a 50,000-record
storage bound, and five-million-visit bound:

```powershell
python experiments/proto/layer_order3_witnesses.py --time-limit 180 --max-states 50000 --max-visits 5000000
```

To retain a new record file, add `--output` with a fresh filename in an
existing directory. The complete counts and validation pass before that
file is created. This is a finite support certificate, not a factorization
weight calculation.

## The missing L4 witness and both support closures

Native canonicalization of the 41,900 order-three records produced 1,060
distinct orbits, with the following stabilizer inventory:

```text
stabilizer:    3    6   12   24   48   96  144  384
orbits:      726  237   66   16    9    2    2    2
```

All 66 stabilizer-12 candidates were independently checked by full-group
stabilizer scans. Their 253,440 distinct coordinate images formed an
exhaustive membership target set. A complete read-only scan of production
generation 37 found 65 candidates once each and target 48 absent. The
unique missing orbit is represented by:

```text
294 294 554 554 1161 1161 1360 1360 2181 2181 2640 2640
```

The companion witness script independently verifies its legality,
stabilizer 12, and orbit size 3,840 over all 46,080 group elements. Its full
lexicographic representative is
`(85,85,90,90,1313,1313,1570,1570,2440,2440,2692,2692)`; this full-lex form
must not be mistaken for the engine's particular native canonical form.

The membership log is
`data/logs/c6-direct-route-20260905/l4-order3-membership.log`. It reports
50.3434 seconds, peak RSS 31,117,312 bytes, verified header and payload
hashes, and SHA-256
`ECF0837315B0FDF8AE21C394FDA6676490E43B1828A0825529A344EC17E4E844`.
Its source hash matches the original production image. The analogous L5
log is `data/logs/c6-direct-route-20260905/l5-target-membership.txt`, with
source SHA-256
`A5FDDB22F8C79FD4DDC0015795558FBF1647F0DB556248F5C9551F2DB24891DF`.

The three witnesses therefore close both support inventories arithmetically:

| Layer | Existing real keys | Added keys | Complete support | Existing raw mass | Added raw mass | Complete raw mass |
|---:|---:|---:|---:|---:|---:|---:|
| 4 | 903,398,602 | 1 | 903,398,603 | 41,602,261,532,320 | 3,840 | 41,602,261,536,160 |
| 5 | 96,452,753 | 2 | 96,452,755 | 4,439,972,138,848 | 224 | 4,439,972,139,072 |

The small retained fixture
[`og2-c6-support-witnesses.json`](../../data/golden/og2-c6-support-witnesses.json)
contains all three masks, exact stabilizers and orbit masses, source
hashes, membership-log provenance, and the limits of the source audit.
Verify its geometry and closure arithmetic with:

```powershell
python experiments/proto/layer_support_witnesses.py --fixture data/golden/og2-c6-support-witnesses.json --time-limit 30
```

This command passed on 2026-09-05 in 0.593995 seconds and printed
`"fixture_verified": true`. It constructs and checks the witnesses; it
does not reread either large source image or claim to repeat membership.

The conclusion relies on the original engine's established legality and
uniqueness guarantees and on the stored stabilizers used in the catalogue
mass audits. The source scans explicitly reported
`semantic_full_key_audit=no` and `stabilizers_recomputed=no`; they did not
independently reprove every property of all approximately one billion
keys. Witness legality and all selected stabilizers were independently
checked. No weight closure follows: both original images retain their
unfinished production or rehearsal weights. The fixture specifies the
three additions for a separately constructed complete support; the witness
and census programs do not modify the active images.
