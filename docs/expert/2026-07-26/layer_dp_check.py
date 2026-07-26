"""Independent verification of the layered DP for counting 2xC sudoku band skeletons.

Model (as specified):
- 2C slots: (b, s), b in 0..C-1 boxes, s in {0,1} sides.  Slot index = 2*b + s.
- 2C anonymous symbols.  A state after L rows: multiset of 2C masks, each mask a
  set of L slots with at most one slot per box.  Symbol labels and row order forgotten.
- Group G = C2 wr S_C acting on slots: g=(pi, f), g(b,s) = (pi(b), s ^ f_b), |G| = 2^C C!.
- Canonical form: lex-min sorted multiset over G (masks as sorted tuples,
  state as sorted tuple of masks).
"""

import itertools
import math
import time
import json
from collections import Counter


def build_group(C):
    """Return list of slot-permutation maps (tuples of length 2C)."""
    elems = []
    for pi in itertools.permutations(range(C)):
        for flips in itertools.product((0, 1), repeat=C):
            gmap = [0] * (2 * C)
            for b in range(C):
                for s in (0, 1):
                    gmap[2 * b + s] = 2 * pi[b] + (s ^ flips[b])
            elems.append(tuple(gmap))
    return elems


def apply_state(gmap, state):
    return tuple(sorted(tuple(sorted(gmap[v] for v in m)) for m in state))


def canonical(state, group):
    return min(apply_state(g, state) for g in group)


def stab_order(state, group):
    s = tuple(sorted(tuple(sorted(m)) for m in state))
    return sum(1 for g in group if apply_state(g, s) == s)


def emissions_of(x, n):
    """Yield child multisets (sorted tuples of masks) for each emission from state x.

    x: canonical state = sorted tuple of masks (each mask sorted tuple of slots).
    An emission: for each distinct mask u with multiplicity a_u pick a set S_u of
    a_u pairwise-disjoint slots compatible with u (box not already used by u),
    union over u = all n slots.  Child: replace the a_u copies of u by
    {u UNION {v} : v in S_u}.
    """
    cnt = Counter(x)
    items = list(cnt.items())

    def rec(i, remaining, acc):
        if i == len(items):
            # sum of multiplicities == n and all S_u disjoint => remaining empty
            assert not remaining
            yield tuple(sorted(acc))
            return
        u, a = items[i]
        boxes_u = {v // 2 for v in u}
        comp = [v for v in remaining if v // 2 not in boxes_u]
        if len(comp) < a:
            return
        for S in itertools.combinations(comp, a):
            new_masks = [tuple(sorted(u + (v,))) for v in S]
            yield from rec(i + 1, remaining - set(S), acc + new_masks)

    yield from rec(0, frozenset(range(n)), [])


def run(C):
    t0 = time.perf_counter()
    n = 2 * C
    group = build_group(C)
    Gorder = len(group)
    assert Gorder == (2 ** C) * math.factorial(C)

    # Layer 1: all singletons.
    x1 = tuple(sorted((v,) for v in range(n)))
    x1c = canonical(x1, group)
    s1 = stab_order(x1c, group)
    assert Gorder % s1 == 0
    assert Gorder // s1 == 1, f"C={C}: layer-1 orbit size {Gorder // s1} != 1"

    T = {x1c: 1}
    states_per_layer = [len(T)]
    emissions_per_transition = []
    raw_dedup = []  # [raw distinct children, canonical distinct children] per transition
    layer_T_history = [dict(T)]

    for _layer in range(1, C):
        Tnext = {}
        em_count = 0
        raw_children = set()
        canon_children = set()
        canon_cache = {}
        for x, tx in T.items():
            for child in emissions_of(x, n):
                em_count += 1
                raw_children.add(child)
                if child in canon_cache:
                    cc = canon_cache[child]
                else:
                    cc = canonical(child, group)
                    canon_cache[child] = cc
                canon_children.add(cc)
                ccnt = Counter(child)
                K = 1
                for c in ccnt.values():
                    K *= math.factorial(c)
                Tnext[cc] = Tnext.get(cc, 0) + tx * K
        emissions_per_transition.append(em_count)
        raw_dedup.append([len(raw_children), len(canon_children)])
        T = Tnext
        states_per_layer.append(len(T))
        layer_T_history.append(dict(T))

    # Final contraction.  Each summand (ell_q / m_q) * T_q^2 must be an exact
    # integer; ell_q / m_q alone need not be (diagnostic recorded if it is not).
    N = 0
    nonintegral_ell_over_m = []
    for q, tq in T.items():
        # every mask must be complete: one slot per box
        for m in q:
            assert len(m) == C and len({v // 2 for v in m}) == C
        ccnt = Counter(q)
        denom = 1
        for c in ccnt.values():
            denom *= math.factorial(c)
        num = math.factorial(n)
        assert num % denom == 0, f"C={C}: ell not integral for {q}"
        ell = num // denom
        s = stab_order(q, group)
        assert Gorder % s == 0, f"C={C}: stabilizer {s} does not divide |G|={Gorder}"
        m_orb = Gorder // s
        if ell % m_orb != 0:
            nonintegral_ell_over_m.append((ell, m_orb, tq))
        term_num = ell * tq * tq
        assert term_num % m_orb == 0, (
            f"C={C}: summand ell*T^2={term_num} not divisible by orbit size {m_orb}")
        N += term_num // m_orb

    dt = time.perf_counter() - t0
    return {
        "emissions": emissions_per_transition,
        "states": states_per_layer,
        "N": N,
        "raw_dedup": raw_dedup,
        "layer_T_history": layer_T_history,
        "nonintegral_ell_over_m": nonintegral_ell_over_m,
        "seconds": dt,
    }


EXPECTED = {
    2: {"emissions": [4], "states": [1, 2], "N": 288},
    3: {"emissions": [80, 29], "states": [1, 5, 4], "N": 28200960},
    4: {"emissions": [4752, 4630, 712], "states": [1, 23, 54, 26],
        "N": 29136487207403520},
}


def main():
    out = {}
    all_ok = True
    for C in (2, 3, 4):
        r = run(C)
        exp = EXPECTED[C]
        n_match = (r["N"] == exp["N"])
        em_match = (r["emissions"] == exp["emissions"])
        st_match = (r["states"] == exp["states"])
        ok = n_match and em_match and st_match
        all_ok = all_ok and ok
        entry = {
            "emissions": r["emissions"],
            "states": r["states"],
            "N": str(r["N"]),
            "n_match": n_match,
            "emissions_match": em_match,
            "states_match": st_match,
            "raw_dedup": r["raw_dedup"],
            "nonintegral_ell_over_m": r["nonintegral_ell_over_m"],
            "seconds": round(r["seconds"], 4),
        }
        if C == 2:
            layer2 = r["layer_T_history"][1]
            tvals = [layer2[k] for k in sorted(layer2)]
            entry["c2_layer2_T"] = tvals
            entry["c2_layer2_T_sum"] = sum(tvals)
            entry["c2_layer2_states"] = [
                [list(m) for m in st] for st in sorted(layer2)
            ]
        out[str(C)] = entry
        print(f"C={C}: emissions={r['emissions']} (exp {exp['emissions']}) "
              f"states={r['states']} (exp {exp['states']}) N={r['N']} "
              f"(exp {exp['N']}) match={ok} raw_dedup={r['raw_dedup']} "
              f"time={r['seconds']:.3f}s")
        if C == 2:
            print(f"  C=2 layer-2 T values (canonical order): {entry['c2_layer2_T']}, "
                  f"sum = {entry['c2_layer2_T_sum']}")
            print(f"  C=2 layer-2 canonical states: {entry['c2_layer2_states']}")
    print("ALL MATCH" if all_ok else "MISMATCH DETECTED")
    print("JSON:", json.dumps(out))


if __name__ == "__main__":
    main()
