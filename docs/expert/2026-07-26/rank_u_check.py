"""Rank of the completion operator U: layer-(C-1) states -> complete classes.

Question (the target-only contraction probe): N(C) = T U D U^T T^T with
D = diag(ell_q/m_q) > 0, so any exact evaluation that factors through class
values needs exactly rank(U) channels.  If rank(U) = min(M_{C-1}, #classes)
(full), there is no exact linear compression of the target space as seen from
the last partial layer.  Computed exactly over Q (Bareiss, fraction-free).

Also reports rank of every intermediate transition matrix R_{L,L+1}.
"""

import itertools
import math
import time
from collections import Counter
from fractions import Fraction

from layer_dp_check import build_group, canonical, stab_order, emissions_of


def rank_over_Q(rows):
    """Exact rank of an integer matrix (list of lists) via fraction pivoting."""
    m = [[Fraction(v) for v in row] for row in rows]
    rank = 0
    ncols = len(m[0]) if m else 0
    r = 0
    for c in range(ncols):
        piv = None
        for i in range(r, len(m)):
            if m[i][c] != 0:
                piv = i
                break
        if piv is None:
            continue
        m[r], m[piv] = m[piv], m[r]
        pv = m[r][c]
        for i in range(r + 1, len(m)):
            if m[i][c] != 0:
                f = m[i][c] / pv
                for j in range(c, ncols):
                    m[i][j] -= f * m[r][j]
        r += 1
        rank += 1
        if r == len(m):
            break
    return rank


def run(C):
    t0 = time.perf_counter()
    n = 2 * C
    group = build_group(C)
    Gorder = len(group)

    x1 = tuple(sorted((v,) for v in range(n)))
    layer = [canonical(x1, group)]
    R_matrices = []

    for _step in range(1, C):
        idx = {x: i for i, x in enumerate(layer)}
        children = {}
        R = [dict() for _ in layer]
        canon_cache = {}
        for x in layer:
            i = idx[x]
            for child in emissions_of(x, n):
                cc = canon_cache.get(child)
                if cc is None:
                    cc = canonical(child, group)
                    canon_cache[child] = cc
                if cc not in children:
                    children[cc] = len(children)
                K = 1
                for c in Counter(child).values():
                    K *= math.factorial(c)
                j = children[cc]
                R[i][j] = R[i].get(j, 0) + K
        next_layer = [None] * len(children)
        for st, j in children.items():
            next_layer[j] = st
        dense = [[R[i].get(j, 0) for j in range(len(next_layer))]
                 for i in range(len(layer))]
        R_matrices.append(dense)
        layer = next_layer

    ranks = [rank_over_Q(M) for M in R_matrices]
    dims = [(len(M), len(M[0])) for M in R_matrices]

    # D-weighted final check: rank(U sqrt(D)) == rank(U) since D > 0; verify
    # D positivity explicitly.
    for q in layer:
        ccnt = Counter(q)
        denom = 1
        for c in ccnt.values():
            denom *= math.factorial(c)
        ell = math.factorial(n) // denom
        s = stab_order(q, group)
        m_orb = Gorder // s
        assert ell > 0 and m_orb > 0

    dt = time.perf_counter() - t0
    return dims, ranks, dt


def main():
    for C in (3, 4):
        dims, ranks, dt = run(C)
        print(f"C={C} ({dt:.1f}s):")
        for k, ((r_, c_), rk) in enumerate(zip(dims, ranks), start=1):
            full = min(r_, c_)
            tag = "FULL" if rk == full else f"DEFICIENT (< {full})"
            star = "  <-- completion operator U" if k == len(dims) else ""
            print(f"  R_{k}->{k+1}: {r_}x{c_}  rank = {rk}  [{tag}]{star}")


if __name__ == "__main__":
    main()
