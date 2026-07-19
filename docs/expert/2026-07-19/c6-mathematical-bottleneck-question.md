# C=6 independent verification: the mathematical bottleneck after the current scale gates

This is a request for an exact mathematical construction or lower bound, not
for another class-by-class implementation.  The verified project state is in
`../../../STATUS.md`; the claims below summarize completed local experiments.

## Exact formulation

Let `n=2C`.  For a balanced cut `A` of the `n` symbols, let `M_{A,k}` be the
single-stack grade-`k` transition that assigns the two rows of one band to
unused columns.  Each row assignment is a bijection, so `M_{A,k}` contains two
row-permanent constraints.  The required squared count is

\[
N(C)=\left\langle f\otimes f\middle|
K_{C-1}\cdots K_0\middle|e\otimes e\right\rangle,
\qquad
K_k=\sum_{|A|=C} M_{A,k}\otimes M_{A,k}.
\]

The two copies must share the same cut `A`; this correlation is precisely what
turns a cheap linear sum into the required square.  After quotienting symbol
labels, the two independent column groups, and copy exchange, write the exact
operator as `Kbar_k`.  At `C=5` the reduced grade dimensions are

```text
1, 7, 38801, 38801, 7, 1.
```

The equivalent outer-graph formulation is

\[
N(6)=\sum_{[Q]} w([Q])F_6(Q)^2,
\]

over exactly 63,199 outer skeleton orbits.  The goal is an independent exact
verification of the historically announced value, not discovery of a target
integer.

## What the bounded experiments have established

1. **Independent outer-class evaluation does not scale.**  The second C=6
   class has an exact 221,438,460-state, 9.08-GB frontier; even with a complete
   shared half-kernel table its tail takes 824 seconds.  Later classes show
   little or no reuse of complete kernel-pair signatures.
2. **The obvious reverse `4+2` incidence enumerations are both too large.**
   Bottom-up gluing is exact through C=5, but the trivial-stabilizer part of
   the C=6 two-row inventory alone forces at least 1,761,454,080 double-coset
   placements.  A generic 100,000-leaf prefix has 99,423 distinct canonical
   outputs.  In the top-down dual view, an ordinary outer graph has about
   1.33--1.39 billion spanning two-factors; sampled residual `F4` keys are more
   than 99.8% unique, with only about 2--4% coverage from the existing table.
3. **Reordering the local assignments has not compressed them.**  A scalar
   residual-degree DP counts 652,001,548 C=5 allocation leaves with only
   49,890 memo states, but it discards target identity and cycle weight.  Its
   exact target-aware lift is nearly injective: the one completed source has
   6,323,400 terminal states from 6,516,556 leaves.  The separate
   double-permanent subset frontier is provably injective in the assignment
   prefix for a fixed labelled source.
4. **Tiny-rank explanations are not supported.**  The C=5 middle `Kbar_2` is
   `38801 x 38801` and has certified rational rank at least 1,024.  The natural
   3+3 representation channels are already full-row-rank at C=5, and symmetry
   forces no channel loss at C=6.  This does not prove full rank, but it rules
out the hoped-for few-hundred-dimensional factorization.

These results expose one common obstruction.  If a state retains enough
information to distinguish the final joint target and its cycle weight, the
transition currently recovers almost every assignment/configuration prefix.
If that identity is marginalized early, the computation becomes small but no
longer determines the squared objective.  External sorting controls memory;
it does not change this arithmetic count.

The central question is therefore no longer merely whether the operator has
small rank.  It is whether this possibly full-rank operator has a fast exact
transform or circuit whose cost is much smaller than its enumerated support.

## Precise request

Can one contract this identity information *implicitly*, without emitting one
record per assignment, placement, or residual graph?  A useful answer may take
either of the following forms.

1. **A fast exact application of `Kbar_k`.**  Give a factorization, generating
   function, coherent-configuration algebra, balanced-switch transform, or
   other circuit that applies `Kbar_k` to a reduced vector substantially
   faster than enumerating its transition support.  It need not be low rank.
   The construction must explain how it preserves the shared cut, both
   permanent constraints, target aggregation, and the shared-cut multiplicity
   (equivalently, its cycle-factor formulation).  The most immediate candidate
   to clarify is a fused within-band column frontier
   quotiented jointly with its source, rather than the rejected labelled
   target frontier.
2. **A genuinely global reverse-gluing incidence contraction.**  Give an exact
   way to build or apply the weighted `4+2` incidence/Gram operator across all
   outer classes at once, without enumerating either the known 1.761-billion
   bottom-up placement subset or a roughly 1.3-billion two-factor list for
   each ordinary `Q`.  Merely sharing an `F4` memo or streaming the same lists
   is not such a construction.

An impossibility result would also be valuable: for example, a proof that any
exact column frontier with the stated output semantics must distinguish all
assignment prefixes, or a meaningful rank/communication lower bound for the
relevant operator family.

A proposal is actionable only if it supplies (i) an exact identity and
normalization, (ii) a proof that the retained object is sufficient for the
square rather than only the linear marginal, (iii) a realistic C=5/C=6 state
or operation bound using the inventories above, and (iv) a small-C gate that
can reproduce every C=3/C=4 transition coefficient and the known C=5 total.
Generic Burnside decomposition, a larger cache, external sorting alone, or a
different traversal of the same near-injective records does not address the
measured bottleneck.
