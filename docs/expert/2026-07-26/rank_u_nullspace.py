"""Extract the exact column null space of the C=4 completion operator U.

Uz = 0 with z indexed by the 26 complete classes: each z is an exact linear
relation satisfied by ALL class columns, i.e. sum_q z_q * R(x,q) = 0 for every
layer-3 state x, hence sum_q z_q T_4(q) = 0 identically.  Prints integer basis
vectors and the class data (ell, m, T) to see whether the relations have
visible structure.
"""

import math
import time
from collections import Counter
from fractions import Fraction

from layer_dp_check import build_group, canonical, stab_order, emissions_of


def nullspace_int(rows, ncols):
    """Exact rational null space of the column space: solve M z = 0."""
    m = [[Fraction(v) for v in row] for row in rows]
    nrows = len(m)
    # Gauss-Jordan to RREF
    pivots = []
    r = 0
    for c in range(ncols):
        piv = None
        for i in range(r, nrows):
            if m[i][c] != 0:
                piv = i
                break
        if piv is None:
            continue
        m[r], m[piv] = m[piv], m[r]
        pv = m[r][c]
        m[r] = [v / pv for v in m[r]]
        for i in range(nrows):
            if i != r and m[i][c] != 0:
                f = m[i][c]
                m[i] = [a - f * b for a, b in zip(m[i], m[r])]
        pivots.append(c)
        r += 1
        if r == nrows:
            break
    free = [c for c in range(ncols) if c not in pivots]
    basis = []
    for fc in free:
        z = [Fraction(0)] * ncols
        z[fc] = Fraction(1)
        for ri, pc in enumerate(pivots):
            z[pc] = -m[ri][fc]
        # clear denominators to integers
        denlcm = 1
        for v in z:
            denlcm = denlcm * v.denominator // math.gcd(denlcm, v.denominator)
        zi = [int(v * denlcm) for v in z]
        g = 0
        for v in zi:
            g = math.gcd(g, abs(v))
        if g > 1:
            zi = [v // g for v in zi]
        basis.append(zi)
    return basis


def main():
    C = 4
    n = 2 * C
    t0 = time.perf_counter()
    group = build_group(C)
    Gorder = len(group)

    x1 = tuple(sorted((v,) for v in range(n)))
    layer = [canonical(x1, group)]
    T = {layer[0]: 1}

    U = None
    classes = None
    for step in range(1, C):
        children = {}
        rows = {x: {} for x in layer}
        canon_cache = {}
        Tnext = {}
        for x in layer:
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
                rows[x][j] = rows[x].get(j, 0) + K
                Tnext[cc] = Tnext.get(cc, 0) + T[x] * K
        next_layer = [None] * len(children)
        for st, j in children.items():
            next_layer[j] = st
        if step == C - 1:
            U = [[rows[x].get(j, 0) for j in range(len(next_layer))]
                 for x in layer]
            classes = next_layer
        layer = next_layer
        T = Tnext

    basis = nullspace_int(U, len(classes))
    print(f"null space dimension: {len(basis)}")
    # class data
    info = []
    for q in classes:
        ccnt = Counter(q)
        denom = 1
        for c in ccnt.values():
            denom *= math.factorial(c)
        ell = math.factorial(n) // denom
        s = stab_order(q, group)
        m_orb = Gorder // s
        info.append((ell, m_orb, T[q]))
    for k, z in enumerate(basis):
        supp = [(j, z[j]) for j in range(len(z)) if z[j] != 0]
        print(f"relation {k}: support size {len(supp)}: {supp}")
        # check it kills the actual T vector too
        dot = sum(z[j] * info[j][2] for j in range(len(z)))
        print(f"  sum z_q * T_4(q) = {dot}  (must be 0)")
        for j, coef in supp:
            ell, m_orb, tq = info[j]
            print(f"    class {j}: coef {coef:>3}  ell={ell:>7} m={m_orb:>4} "
                  f"T={tq}  F=T/m={Fraction(tq, m_orb)}")
    print(f"({time.perf_counter()-t0:.1f}s)")


if __name__ == "__main__":
    main()
