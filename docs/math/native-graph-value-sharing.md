# Exact graph-value sharing without merging native responses

This note concerns the degree-four layer of the 2xC count, with C=4..6.
It does not evaluate N(6). All statements below are proved here, except where
explicitly marked as finite tests or performance measurements.

## Objects and the distinction that matters

A native state x is a multiset of 2C symbol masks on 2C column slots, grouped
into C fixed pairs (the boxes). A symbol occupies at most one slot of a box.
At layer L each mask and each slot has degree L. Forgetting the pairing gives
an L-regular bipartite incidence graph Q_x, with symbols and slots as its two
vertex sets. Let F_L(Q) count ordered decompositions of its edges into L
perfect matchings.

The native coordinate group has order g=2^C C!. If s_x is the native
stabilizer, its orbit size is m_x=g/s_x and its exact layer weight is
T_L(x)=m_x F_L(Q_x).

Graphs with different native presentations have the same F_L, because
independent vertex relabelling and whole-bipartition exchange give explicit
bijections on ordered matching decompositions. They need not have the same
downstream native transition row. Consequently we share only the calculation
of F_L; we retain a separate state, stabilizer, weight and downstream response
for every native orbit. No one-dimensional scalar quotient is asserted.

## Enumerating a complete fiber

Fix a labelled bipartite graph Q with equal vertex sets of size 2C. A pairing
of its slot vertices is admissible precisely when the two vertices in every
pair have disjoint symbol neighborhoods. This is exactly the condition that
no symbol occupies both sides of a box.

For each admissible pairing P, choose an arbitrary order of pairs and an
arbitrary orientation of each pair, then form a native state and apply the
unchanged native canonicalizer. The resulting key is independent of those
arbitrary choices: changing them is an element of the native coordinate
group. Call the set of distinct resulting keys R(Q).

Every native presentation of Q supplies one admissible pairing, and every
admissible pairing supplies a native presentation. Thus R(Q) is exactly the
fiber of Q when the two bipartitions remain distinguished. Duplicating an
isomorphic pairing merely duplicates a key and is removed by set deduplication;
no orbit-weight division is part of this enumeration.

Define B(Q)=R(Q) union R(Q^T). Independent relabellings permute the admissible
pairings, and transposition exchanges the two sets. Therefore B is invariant
under graph isomorphism allowing whole-bipartition exchange. Conversely, if
two such sets intersect, their common native key supplies isomorphisms from
both graphs to one incidence graph, possibly transposed. The graphs are then
isomorphic, and their whole B sets agree.

It follows that the minimum native key in B(Q) is a complete, exact graph key
for graphs having at least one native presentation. For any native source
the set is nonempty because its original box pairing is admissible. It is
not necessary for the transposed graph to have an admissible pairing.

At C=6 there are at most 11!!=10,395 pairings on either side. Consequently
20,790 is a proved per-state bound on the complete two-sided pairing scan.
Actual sampled degree-four graphs have far fewer admissible pairings.

## Shared values with stable native IDs

Let a complete native catalogue retain its original IDs, including explicit
insertion holes. For each live ID i, look up the minimum key of B(Q_i) in that
same catalogue and call the resulting ID r(i). Then r(r(i))=r(i), and
F_4(Q_i)=F_4(Q_r(i)). The catalogue need not be sorted and r(i) need not be
less than i.

A two-pass computation is exact:

1. Store every alias r(i), but compute a closed F4 value only when r(i)=i.
   Each representative is therefore evaluated exactly once across a complete
   pass, independently of processing order and thread scheduling.
2. After every alias is known and every representative value is closed,
   restore every separate native weight as
   T4(i)=(g/s_i) F4(r(i)).

There is no use of an old partial native weight in either step. In particular,
having discovered a key during an unfinished forward transition does not make
its accumulated T a reference value.

## Reverse fifth-row calculation

For any edge e of a d-regular bipartite graph Q,

    F_d(Q) = d * sum_{perfect matchings M containing e} F_(d-1)(Q-M).

In an ordered factorization, e belongs to exactly one color. The d possible
color choices have equal cardinality by relabelling the colors. Restricting
to its first color gives the displayed sum, with the remaining colors still
ordered. Grouping equal residual native keys must retain their labelled
matching multiplicities.

For d=5 the complete closed native F4 table can therefore be used directly
as an exact predecessor lookup. Restoring the target orbit multiplier is a
separate final operation; it is not inserted inside the graph recurrence.

## Finite correctness gates and scope

Complete pairing-fiber censuses were run using the retained native
canonicalizer:

| Domain | Native keys | Fixed-bipartition graph fibers | Transpose-merged fibers |
|---|---:|---:|---:|
| C4, L3 | 54 | 38 | 33 |
| C5, L3 | 16,150 | 1,160 | 721 |
| C5, L4 | 17,120 | 14,237 | 12,543 |

Every fiber member was present, each minimum represented its exact full fiber,
the fibers partitioned the catalogue, and all available independent F values
agreed within fibers. Arbitrary row/column relabelling and transpose
invariance were also checked. On the first 256 actual uniform C6 samples,
every reported fiber field agreed with an independent NetworkX automorphism
and pairing implementation.

The shared F4 core reproduced all 26 C4 and 17,120 C5 values and native
weights. The core test deliberately poisoned the old T array and verified it
was neither read nor changed. A C6 sample closed under the complete fiber
operation contains 12,345 native keys in 1,024 graph fibers; all of its F4
values were independently computed and matched by the combined shared engine.
This last set is a sample domain, not the full C6 catalogue.

## What can and cannot be inferred from sampling

Let M be the native population size, H the number of graph fibers, and b(x)
the size of the fiber containing a uniformly sampled native x. The exact
identity is H/M=E[1/b(x)]: each fiber contributes exactly one to the sum.

For a chosen minimum representative, let t(r(x)) be its cold evaluation work.
The total representative work is exactly

    M * E_native[t(r(x))/b(x)].

It is generally incorrect to multiply the mean cost of arbitrary native
presentations by the mean reciprocal fiber size. The rooted kernel cost can
depend on the representative's presentation and correlate with fiber size.
Likewise a union of complete fibers from sampled sources is size-biased: its
native throughput is not a uniform-population timing estimate.

These identities do not account for parallel scaling, complete-catalogue
memory access, checkpoint I/O or verification. Those require separately
measured, bounded end-to-end gates.

Implementation: `experiments/proto/layer_shared_f4.cpp`, its core/bridge and
closed-chunk helpers. Evidence and exact execution paths are retained in
`docs/reports/og2/c6-catalogue-direct-route-20260905.md`.
