"""
Verify claimed identification between eps symmetry classes and complete band classes.

eps = (A_1,...,A_C), each A_j a C-subset of [2C].
Group: sigma in S_2C on symbols, pi in S_C on band indices, flips f_j (complement A_j).

Quotient by sigma: eps -> multiset of 2C membership vectors in {0,1}^C
(vector of symbol i = (1 if i in A_j else 0)_j), column sums all = C.
Residual group H = C2 wr S_C on {0,1}^C (pi permutes coordinates, f XOR-flips them).

Checks:
  orbit counts: C=2 -> 2, C=3 -> 4, C=4 -> 26
  Phi constant on orbits (C=2,3), and sum_orbits |orbit_eps| * Phi^2 = N(C):
      N(2)=288, N(3)=28200960.
"""

import itertools
import json
import random
import time
from collections import defaultdict

T0 = time.time()

# ----------------------------------------------------------------------
# Group H = C2 wr S_C acting on {0,1}^C
# ----------------------------------------------------------------------

def H_elements(C):
    return [(pi, f)
            for pi in itertools.permutations(range(C))
            for f in itertools.product((0, 1), repeat=C)]

def apply_h(vec, pi, f):
    return tuple(vec[pi[k]] ^ f[k] for k in range(len(pi)))

def canonical_multiset(mult, H):
    """min over h in H of sorted image of the multiset (mult = sorted tuple of vectors)."""
    best = None
    for pi, f in H:
        img = tuple(sorted(apply_h(v, pi, f) for v in mult))
        if best is None or img < best:
            best = img
    return best

# ----------------------------------------------------------------------
# Phi(eps): ordered proper C-edge-colorings of the C-regular bipartite graph
# symbols [2C] vs slots (j,s); edge (i,(j,s)) iff (i in A_j) == (s==0).
# Slot (j,s) is encoded as integer 2*j + s.
# ----------------------------------------------------------------------

def phi_of_eps(eps, C):
    n = 2 * C

    def enum_pms(slotlists):
        res = []
        assign = [0] * n

        def bt(i, used):
            if i == n:
                res.append(assign.copy())
                return
            for s in slotlists[i]:
                b = 1 << s
                if not (used & b):
                    assign[i] = s
                    bt(i + 1, used | b)

        bt(0, 0)
        return res

    def rec(slotlists, depth):
        if depth == 0:
            assert all(len(sl) == 0 for sl in slotlists), "edges left at depth 0"
            return 1
        total = 0
        for pm in enum_pms(slotlists):
            new = [[s for s in slotlists[i] if s != pm[i]] for i in range(n)]
            total += rec(new, depth - 1)
        return total

    slots0 = [[2 * j + (0 if i in eps[j] else 1) for j in range(C)]
              for i in range(n)]
    return rec(slots0, C)

def membership_multiset(eps, C):
    return tuple(sorted(tuple(1 if i in eps[j] else 0 for j in range(C))
                        for i in range(2 * C)))

# ----------------------------------------------------------------------
# Direct enumeration of all eps tuples (C = 2, 3): orbits, orbit sizes, Phi
# ----------------------------------------------------------------------

def analyze_full(C, N_expected, n_phi_samples=200, seed=12345):
    H = H_elements(C)
    subsets = [frozenset(c) for c in itertools.combinations(range(2 * C), C)]
    canon_cache = {}          # multiset key -> canonical form
    phi_cache = {}            # multiset key -> Phi (Phi factors through the multiset;
                              #                     validated by raw random samples below)
    orbit_eps_count = defaultdict(int)      # canonical -> number of eps tuples
    orbit_phis = defaultdict(set)           # canonical -> set of Phi values seen
    orbit_multisets = defaultdict(set)      # canonical -> distinct multiset keys
    all_eps = list(itertools.product(subsets, repeat=C))

    for eps in all_eps:
        key = membership_multiset(eps, C)
        if key not in canon_cache:
            canon_cache[key] = canonical_multiset(key, H)
        if key not in phi_cache:
            phi_cache[key] = phi_of_eps(eps, C)
        can = canon_cache[key]
        orbit_eps_count[can] += 1
        orbit_phis[can].add(phi_cache[key])
        orbit_multisets[can].add(key)

    # Validate that Phi genuinely factors through the multiset (i.e. the memoization
    # is legitimate): recompute Phi from scratch for random eps tuples.
    rng = random.Random(seed)
    sample = rng.sample(all_eps, min(n_phi_samples, len(all_eps)))
    sample_ok = True
    for eps in sample:
        key = membership_multiset(eps, C)
        if phi_of_eps(eps, C) != phi_cache[key]:
            sample_ok = False
            break

    phi_const = all(len(s) == 1 for s in orbit_phis.values()) and sample_ok
    weighted = sum(orbit_eps_count[can] * next(iter(orbit_phis[can])) ** 2
                   for can in orbit_eps_count)
    raw_total = sum(phi_cache[membership_multiset(eps, C)] ** 2 for eps in all_eps)

    info = {
        "orbits": len(orbit_eps_count),
        "orbit_details": sorted((orbit_eps_count[c], sorted(orbit_phis[c])[0])
                                for c in orbit_eps_count),
        "total_eps": len(all_eps),
        "phi_constant": phi_const,
        "phi_sample_ok": sample_ok,
        "weighted_sum": weighted,
        "raw_total": raw_total,
        "N_expected": N_expected,
        "weighted_ok": (weighted == N_expected and raw_total == N_expected),
    }
    return info

# ----------------------------------------------------------------------
# Burnside count of H-orbits of valid multisets (any C)
# valid multiset: size 2C over {0,1}^C, every column sum = C
# ----------------------------------------------------------------------

def count_fixed(cycles, C):
    """Number of multiplicity assignments constant on cycles meeting the constraints."""
    target_total = 2 * C
    target_cols = (C,) * C
    states = {(0, (0,) * C): 1}
    for size, S in cycles:
        new = defaultdict(int)
        for (tot, cols), cnt in states.items():
            k = 0
            while True:
                t2 = tot + k * size
                if t2 > target_total:
                    break
                c2 = tuple(cols[m] + k * S[m] for m in range(C))
                if any(c > C for c in c2):
                    break
                new[(t2, c2)] += cnt
                k += 1
        states = new
    return states.get((target_total, target_cols), 0)

def burnside_orbits(C):
    vecs = list(itertools.product((0, 1), repeat=C))
    H = H_elements(C)
    total = 0
    n_valid = None
    for pi, f in H:
        perm = {v: apply_h(v, pi, f) for v in vecs}
        seen = set()
        cycles = []
        for v in vecs:
            if v in seen:
                continue
            cyc = []
            u = v
            while u not in seen:
                seen.add(u)
                cyc.append(u)
                u = perm[u]
            S = tuple(sum(x[m] for x in cyc) for m in range(C))
            cycles.append((len(cyc), S))
        fx = count_fixed(cycles, C)
        if pi == tuple(range(C)) and all(x == 0 for x in f):
            n_valid = fx  # identity fixes every valid multiset
        total += fx
    assert total % len(H) == 0, "Burnside sum not divisible by |H|"
    return total // len(H), n_valid

# ----------------------------------------------------------------------
# Direct DFS enumeration of valid multisets with canonical dedup (cross-check, C=4)
# ----------------------------------------------------------------------

def direct_multiset_orbits(C):
    vecs = list(itertools.product((0, 1), repeat=C))
    n = 2 * C
    valid = []

    def dfs(idx, tot, cols, chosen):
        if tot == n:
            if all(c == C for c in cols):
                valid.append(tuple(chosen))
            return
        if idx == len(vecs):
            return
        rem = n - tot
        if any(C - c > rem for c in cols):
            return
        v = vecs[idx]
        for k in range(rem + 1):
            c2 = tuple(cols[m] + k * v[m] for m in range(C))
            if any(c > C for c in c2):
                break
            dfs(idx + 1, tot + k, c2, chosen + [v] * k)

    dfs(0, 0, (0,) * C, [])
    H = H_elements(C)
    canset = set()
    for mult in valid:
        canset.add(canonical_multiset(mult, H))
    return len(canset), len(valid)

# ----------------------------------------------------------------------
# Run everything
# ----------------------------------------------------------------------

results = {}

print("=== C=2: full eps enumeration ===")
info2 = analyze_full(2, 288)
print(info2)
results[2] = info2

print("\n=== C=3: full eps enumeration ===")
info3 = analyze_full(3, 28200960)
print(info3)
results[3] = info3

print("\n=== Burnside cross-validation at C=2,3 ===")
b2, nv2 = burnside_orbits(2)
b3, nv3 = burnside_orbits(3)
print(f"Burnside C=2: {b2} orbits ({nv2} valid multisets); direct gave {info2['orbits']}")
print(f"Burnside C=3: {b3} orbits ({nv3} valid multisets); direct gave {info3['orbits']}")
assert b2 == info2["orbits"], "Burnside disagrees with direct at C=2"
assert b3 == info3["orbits"], "Burnside disagrees with direct at C=3"

print("\n=== C=4: Burnside on H = C2 wr S4, |H| = 384 ===")
b4, nv4 = burnside_orbits(4)
print(f"Burnside C=4: {b4} orbits ({nv4} valid multisets)")

print("\n=== C=4: direct DFS multiset enumeration with canonical dedup ===")
d4, dv4 = direct_multiset_orbits(4)
print(f"Direct C=4: {d4} orbits from {dv4} valid multisets")
assert dv4 == nv4, "valid multiset counts disagree (DFS vs identity fix)"
assert d4 == b4, "direct orbit count disagrees with Burnside at C=4"

elapsed = time.time() - T0

orbit_counts = {"2": info2["orbits"], "3": info3["orbits"], "4": b4}
expected = {"2": 2, "3": 4, "4": 26}
all_counts_ok = all(orbit_counts[k] == expected[k] for k in expected)
phi_const = bool(info2["phi_constant"] and info3["phi_constant"])

data = {
    "orbit_counts": orbit_counts,
    "orbit_counts_expected": expected,
    "orbit_counts_match": all_counts_ok,
    "phi_constant_on_orbits": phi_const,
    "weighted_sum_check": {
        "2": f"sum |orbit|*Phi^2 = {info2['weighted_sum']} (raw per-eps sum {info2['raw_total']}), expected 288 -> {info2['weighted_ok']}",
        "3": f"sum |orbit|*Phi^2 = {info3['weighted_sum']} (raw per-eps sum {info3['raw_total']}), expected 28200960 -> {info3['weighted_ok']}",
    },
    "orbit_details_C2 (eps_count, Phi)": info2["orbit_details"],
    "orbit_details_C3 (eps_count, Phi)": info3["orbit_details"],
    "method_c4": ("Burnside on H = C2 wr S4 (|H|=384): fixed multisets counted by DP over "
                  "cycle-multiplicity variables with size and column-sum constraints; "
                  f"cross-checked by direct DFS enumeration of all {dv4} valid multisets "
                  "with canonical (min-over-H) dedup, and Burnside code validated against "
                  "full eps enumeration at C=2,3"),
    "valid_multisets": {"2": nv2, "3": nv3, "4": nv4},
    "seconds": round(elapsed, 2),
}

print("\n=== FINAL ===")
print(json.dumps(data, indent=2))

with open("eps_orbit_result.json", "w") as fh:
    json.dump(data, fh, indent=2)
