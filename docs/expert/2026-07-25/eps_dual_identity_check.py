# Exact check of the row-split (epsilon-side) dual identity
#
#   N(C) = sum over eps = (A_1..A_C), A_j in C([2C],C), of  Phi(eps)^2
#
# where G_eps is the C-regular bipartite graph on symbols [2C] x slots
# ([C] bands x {0,1} sides), with edge (i,(j,s)) iff (i in A_j) == (s==0),
# and Phi(eps) = number of ORDERED proper C-edge-colorings of G_eps
# (= ordered 1-factorizations).
#
# Derivation: linearize the per-band cycle weight 2^kappa(P_a u P_b) as the
# delta=2 Brauer/O(2) Gram form  <v_P, v_Q>,  v_P in (R^2)^{x 2C}, then expand
# [full monomial] B_C^C in the side basis e_eps.  This script verifies the
# identity exactly at C=2 (288) and C=3 (28200960).

import itertools, sys

def perfect_matchings(adj, left):
    # adj: dict left-> set of right; enumerate PMs as tuples right[i] per left order
    if not left:
        yield ()
        return
    i = left[0]
    rest = left[1:]
    for r in adj[i]:
        sub = {j: adj[j] - {r} for j in rest}
        if any(not sub[j] for j in rest):
            continue
        for tail in perfect_matchings(sub, rest):
            yield (r,) + tail

def ordered_factorizations(adj, left, k):
    # count ordered sequences of k disjoint PMs covering all edges
    if k == 0:
        return 1 if all(not adj[i] for i in left) else 0
    total = 0
    for pm in perfect_matchings(adj, left):
        nadj = {i: adj[i] - {pm[t]} for t, i in enumerate(left)}
        total += ordered_factorizations(nadj, left, k - 1)
    return total

def check(C, expected):
    n = 2 * C
    syms = tuple(range(n))
    total = 0
    subsets = list(itertools.combinations(range(n), C))
    for eps in itertools.product(subsets, repeat=C):
        adj = {i: set() for i in syms}
        for j, A in enumerate(eps):
            Aset = set(A)
            for i in syms:
                s = 0 if i in Aset else 1
                adj[i].add((j, s))
        phi = ordered_factorizations(adj, syms, C)
        total += phi * phi
    print(f"C={C}: sum Phi^2 = {total}  expected = {expected}  "
          f"{'OK' if total == expected else 'MISMATCH'}")
    return total == expected

ok = check(2, 288) and check(3, 28200960)
sys.exit(0 if ok else 1)
