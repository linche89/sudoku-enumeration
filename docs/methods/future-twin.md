# Future-twin factorization recurrence

This is the audited optional backend implemented by `src/future_twin.hpp` and
selected by `factorization_orbit.exe ... future`.  It does not replace the
default factorization engine and does not read or write the graph-memo
checkpoint.

## Exact state and transition

After some left vertices have been processed, every right column is represented
by

```text
(tau, K)
```

where `tau` is its remaining left neighbourhood and `K` is the set of colors
already used at that column.  Reachable states satisfy

```text
|K| = C - |tau|,
each color occurs in exactly the number of processed rows.
```

Columns with the same record are future twins.  Suppose the next row meets a
record group `(tau, K)` with multiplicity `m`.  The transition chooses an
`m`-subset of the colors not in `K`; choices for all affected groups are
disjoint and together use every color once.  The group contributes a factor
`m!`, because its columns are state-identical but remain labelled in the
underlying graph.  Thus the exact transition multiplier is

```text
product over affected groups of m!
```

and no perfect matching or residual `Q-M` graph is materialized.

## Canonical keys

The fixed-row mode quotients column records and color names.  Its refinement
canonicalizer is checked against brute force over all `S_C` color
permutations.

The joint modes additionally quotient the names of all remaining rows.  A
small exact individualization/refinement search canonicalizes the colored
bipartite incidence structure consisting of remaining rows, colors, and right
columns.  `canonical-last` removes the last canonical remaining row at every
step; this was the successful C=6 order.

Joint canonicalization has a per-call node budget.  If the budget is exhausted,
the algorithm stores the sorted labelled state as a safe weak key.  Weak keys
carry a separate key-kind bit, so they cannot collide with strong canonical
keys.  This fallback can only miss an isomorphism merge; it cannot merge two
different states or alter the count.

## Verification contract

The permanent tests are:

```powershell
.\build\factorization_orbit.exe 4 futuretest
.\build\factorization_orbit.exe 4 futurecheck pivot rooted4
.\build\factorization_orbit.exe 4 futurecheck futureorder=canonical-last pivot rooted4
.\build\factorization_orbit.exe 4 futurecheck futureorder=canonical-last `
    futurecanonbudget=1 pivot rooted4
.\build\factorization_orbit.exe 5 futurecheck pivot rooted4
.\build\factorization_orbit.exe 5 futurecheck `
    futureorder=canonical-last pivot rooted4
```

`futuretest` checks canonical invariance/separation for `C=2..6` and compares
grouped transitions with labelled enumeration for random `C=2..4` graphs.
`futurecheck` compares every outer class with the current exact engine, rather
than comparing only the final weighted sum.  The forced budget-one test makes
the weak-key path active and verifies that it remains exact.

## C=6 use

Future mode is intentionally cold and rejects `checkpoint=...`.  A bounded G1
reproduction is:

```powershell
.\build\factorization_orbit.exe 6 limit=1 future `
    futureorder=canonical-last futureprogress
```

On the reference machine this returned

```text
F6(G1) = 6986348258918400
class time = 1592.867 s
peak RSS = 1.670 GiB
canonical fallbacks = 0
```

This validates the backend but is slower than the existing checkpoint-free G1
result.  G2 remains open: the bounded evidence is recorded in
`../reports/og2/future-twin-c6-20260714.md`.
