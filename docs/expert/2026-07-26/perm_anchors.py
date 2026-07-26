"""
Permanent-based scale anchors for a row-incremental layer DP over 2xC sudoku
band states, C = 5 and 6.

Model: n = 2C slots = pairs (b,s), slot v = 2*b + s, box(v) = v >> 1.
n symbols; a row is a bijection symbols -> slots; across rows a symbol uses
each box at most once.  After L rows symbol i has mask_i = set of L slots
(one per box max).

FREE:  F[i][v] = 1 iff box(v) not used by mask_i     -> 2(C-L)-regular
USED:  U[i][v] = 1 iff v in mask_i                   -> L-regular

per(FREE) = number of labelled valid next rows.
per(USED) = number of labelled single-row peelings.

Sampling: sequential construction from the unique 1-row state (row 1 =
identity, symbol i -> slot i); each further row is a *uniform* random perfect
matching of the current FREE graph.  Uniformity is exact: we use the subset
DP g[mask] = number of ways to complete a partial matching in which symbols
0..popcount(mask)-1 occupy exactly the slots of `mask`.  g[mask | bit(v)] is
precisely the residual permanent (rows 0..i and their chosen columns plus v
deleted) that the sequential residual-Ryser method would compute, so drawing
slot v with probability g[mask|bit(v)]/g[mask] is the exact sequential
method, memoized.  No backtracking fallback / bias is needed.

Weighting caveat: this sampler weights a labelled L-row state by
  P(state) = sum over ordered histories (id, r2, ..., rL) of
             prod_l 1/per(FREE at intermediate state before row l).
The task's F_L (number of ordered row histories, recursion
F_L = sum_{r in matchings(USED)} F_{L-1}(state - r), F_1 = [state == identity])
equals the history count; P(state) is proportional to F_L only when per(FREE)
is constant across intermediate states at each level (exact for L=2, an
approximation for L=3).  We report the 1/F_L importance-reweighted mean for
L <= 3 as instructed, raw only for L >= 4.
"""

import json
import math
import random
import sys
import time
from collections import Counter
from itertools import permutations


# ----------------------------------------------------------------------
# Permanent by Ryser formula, Gray code, bitset row masks.
# ----------------------------------------------------------------------
def ryser_perm(rows, n):
    """per(A) for an n x n 0/1 matrix; rows[i] = bitmask of columns j with A[i][j]=1."""
    col_rows = [[] for _ in range(n)]
    for i in range(n):
        r = rows[i]
        while r:
            b = r & -r
            col_rows[b.bit_length() - 1].append(i)
            r ^= b
    cnt = [0] * n
    nzero = n            # number of rows whose partial sum is 0
    gray = 0
    bits = 0             # popcount(gray) = |S|
    total = 0
    for k in range(1, 1 << n):
        j = (k & -k).bit_length() - 1
        bit = 1 << j
        gray ^= bit
        if gray & bit:
            bits += 1
            for i in col_rows[j]:
                c = cnt[i]
                if c == 0:
                    nzero -= 1
                cnt[i] = c + 1
        else:
            bits -= 1
            for i in col_rows[j]:
                c = cnt[i]
                if c == 1:
                    nzero += 1
                cnt[i] = c - 1
        if nzero == 0:
            p = 1
            for c in cnt:
                p *= c
            if (n - bits) & 1:
                total -= p
            else:
                total += p
    return total


def brute_perm(rows, n):
    """O(n!) reference permanent for self-tests (small n only)."""
    tot = 0
    for perm in permutations(range(n)):
        p = 1
        for i in range(n):
            if not (rows[i] >> perm[i]) & 1:
                p = 0
                break
        tot += p
    return tot


# ----------------------------------------------------------------------
# Subset DP over slot masks: matching counts + exact uniform sampling.
# ----------------------------------------------------------------------
def make_popcount(n):
    size = 1 << n
    pc = [0] * size
    for m in range(1, size):
        pc[m] = pc[m >> 1] + (m & 1)
    return pc


def match_dp(rows, n, pc):
    """g[mask] = #ways to match symbols popcount(mask)..n-1 into slots outside mask.
    g[0] = per(matrix) = number of perfect matchings."""
    size = 1 << n
    g = [0] * size
    g[size - 1] = 1
    for mask in range(size - 2, -1, -1):
        avail = rows[pc[mask]] & ~mask
        s = 0
        while avail:
            b = avail & -avail
            s += g[mask | b]
            avail ^= b
        g[mask] = s
    return g


def sample_matching(rows, n, g, rng):
    """Exact uniform perfect matching; returns list of slot-bitmasks per symbol."""
    mask = 0
    out = [0] * n
    for i in range(n):
        r = rng.randrange(g[mask])
        avail = rows[i] & ~mask
        while True:
            b = avail & -avail
            w = g[mask | b]
            if r < w:
                out[i] = b
                mask |= b
                break
            r -= w
            avail ^= b
    return out


def all_matchings(rows, n):
    """Enumerate all perfect matchings (backtracking); rows as column bitmasks."""
    res = []
    assign = [0] * n

    def rec(i, used):
        if i == n:
            res.append(assign.copy())
            return
        avail = rows[i] & ~used
        while avail:
            b = avail & -avail
            assign[i] = b
            rec(i + 1, used | b)
            avail ^= b

    rec(0, 0)
    return res


# ----------------------------------------------------------------------
# State helpers.
# ----------------------------------------------------------------------
def free_rows_of(boxes, C, n):
    rows = []
    for i in range(n):
        fr = 0
        bx = boxes[i]
        for b in range(C):
            if not (bx >> b) & 1:
                fr |= 3 << (2 * b)
        rows.append(fr)
    return rows


def check_regular(rows, n, k, name):
    for i in range(n):
        assert bin(rows[i]).count("1") == k, f"{name}: row {i} not {k}-regular"
    for j in range(n):
        cs = sum((rows[i] >> j) & 1 for i in range(n))
        assert cs == k, f"{name}: col {j} sum {cs} != {k}"


def merge_factor(masks):
    f = 1
    for mult in Counter(masks).values():
        f *= math.factorial(mult)
    return f


# ----------------------------------------------------------------------
# Exact ordered-history counts F_L for L <= 3.
# ----------------------------------------------------------------------
def F1(masks, n):
    return 1 if all(masks[i] == (1 << i) for i in range(n)) else 0


def F2_generic(masks, n):
    tot = 0
    for r in all_matchings(masks, n):
        tot += F1([masks[i] ^ r[i] for i in range(n)], n)
    return tot


def F3_closed(masks, n):
    """F_3 = # matchings r of USED with, for every i, slot i still in mask_i - r_i.
    (For a column-regular 2-row state s', F_2(s') = 1 iff identity is contained
    in s', else 0 -- the complement of the identity diagonal is automatically a
    matching.  Spot-checked against the generic recursion below.)"""
    tot = 0
    for r in all_matchings(masks, n):
        ok = True
        for i in range(n):
            if not ((masks[i] ^ r[i]) >> i) & 1:
                ok = False
                break
        tot += ok
    return tot


def F3_generic(masks, n):
    tot = 0
    for r in all_matchings(masks, n):
        tot += F2_generic([masks[i] ^ r[i] for i in range(n)], n)
    return tot


# ----------------------------------------------------------------------
# Stats.
# ----------------------------------------------------------------------
def pctile(sv, p):
    m = len(sv) - 1
    idx = p * m
    lo = int(idx)
    hi = min(lo + 1, m)
    frac = idx - lo
    val = sv[lo] * (1 - frac) + sv[hi] * frac
    iv = int(val)
    return iv if val == iv else val


def stats_of(vals):
    sv = sorted(vals)
    return {
        "min": sv[0],
        "q25": pctile(sv, 0.25),
        "med": pctile(sv, 0.50),
        "mean": sum(sv) / len(sv),
        "q75": pctile(sv, 0.75),
        "max": sv[-1],
    }


# ----------------------------------------------------------------------
# Self tests.
# ----------------------------------------------------------------------
def self_test(rng):
    # per(J_n) = n!, per(I_n) = 1
    for n in (4, 6):
        full = (1 << n) - 1
        assert ryser_perm([full] * n, n) == math.factorial(n)
        assert ryser_perm([1 << i for i in range(n)], n) == 1
    # random 0/1 matrices: Ryser vs brute force vs DP
    pc6 = make_popcount(6)
    for _ in range(20):
        rows = [rng.randrange(1 << 6) for _ in range(6)]
        a = ryser_perm(rows, 6)
        b = brute_perm(rows, 6)
        c = match_dp(rows, 6, pc6)[0]
        assert a == b == c, (rows, a, b, c)
    print("self-tests passed (Ryser == brute force == DP on random 6x6)")


# ----------------------------------------------------------------------
# Main experiment.
# ----------------------------------------------------------------------
def run(n_samples_c5, n_samples_c6, seed):
    t0 = time.time()
    rng = random.Random(seed)
    self_test(rng)

    out = {}
    one_row_perF = {}
    reweighted = {}
    bracket_violations = 0
    f3_spotchecks = []

    for C, N in ((5, n_samples_c5), (6, n_samples_c6)):
        n = 2 * C
        Ls = [2, 3, 4] + ([5] if C == 6 else [])
        Lmax = Ls[-1]
        pc = make_popcount(n)

        # exact 1-row fan-out (identity state)
        id_masks = [1 << i for i in range(n)]
        id_boxes = [1 << (i >> 1) for i in range(n)]
        fr1 = free_rows_of(id_boxes, C, n)
        check_regular(fr1, n, 2 * (C - 1), "FREE L=1")
        p1 = ryser_perm(fr1, n)
        one_row_perF[f"C{C}"] = p1
        print(f"C={C}: one-row per(FREE) (1->2 fan-out) = {p1}")

        acc = {L: {"perF": [], "perU": [], "merge": [], "FL": []} for L in Ls}

        for t in range(N):
            masks = id_masks.copy()
            boxes = id_boxes.copy()
            prevF = p1  # Ryser per(FREE) of current state, for DP cross-check
            for L in range(2, Lmax + 1):
                frows = free_rows_of(boxes, C, n)
                g = match_dp(frows, n, pc)
                assert g[0] == prevF, f"DP {g[0]} != Ryser {prevF} at C={C} L={L-1}"
                row = sample_matching(frows, n, g, rng)
                for i in range(n):
                    masks[i] |= row[i]
                    boxes[i] |= 1 << (row[i].bit_length() - 1 >> 1)

                # record state at L rows
                kF = 2 * (C - L)
                kU = L
                Frows = free_rows_of(boxes, C, n)
                Urows = masks.copy()
                check_regular(Frows, n, kF, f"FREE C={C} L={L}")
                check_regular(Urows, n, kU, f"USED C={C} L={L}")
                perF = ryser_perm(Frows, n)
                perU = ryser_perm(Urows, n)
                prevF = perF
                a = acc[L]
                a["perF"].append(perF)
                a["perU"].append(perU)
                a["merge"].append(merge_factor(masks))

                # classical brackets
                for k, val in ((kF, perF), (kU, perU)):
                    vdw = math.factorial(n) * (k / n) ** n
                    breg = math.factorial(k) ** (n / k)
                    if not (vdw <= val <= breg):
                        bracket_violations += 1
                        print(f"BRACKET VIOLATION C={C} L={L} k={k}: "
                              f"{vdw} <= {val} <= {breg}")

                # exact ordered-history counts for L <= 3
                if L == 2:
                    f2 = F2_generic(masks, n)
                    assert f2 == 1, f"F_2 = {f2} != 1"
                    a["FL"].append(f2)
                elif L == 3:
                    f3 = F3_closed(masks, n)
                    if t < 3:  # spot-check closed form vs generic recursion
                        f3g = F3_generic(masks, n)
                        assert f3 == f3g, (f3, f3g)
                        f3_spotchecks.append((C, f3))
                    a["FL"].append(f3)

        for L in Ls:
            a = acc[L]
            kF, kU = 2 * (C - L), L
            key = f"C{C}_L{L}"
            out[key] = {
                "n_samples": len(a["perF"]),
                "perF": stats_of(a["perF"]),
                "perU": stats_of(a["perU"]),
                "merge_mean": sum(a["merge"]) / len(a["merge"]),
                "vdw_F": math.factorial(n) * (kF / n) ** n,
                "breg_F": math.factorial(kF) ** (n / kF),
                "vdw_U": math.factorial(n) * (kU / n) ** n,
                "breg_U": math.factorial(kU) ** (n / kU),
            }
            if L <= 3:
                ws = [1.0 / f for f in a["FL"]]
                rw = sum(w * v for w, v in zip(ws, a["perF"])) / sum(ws)
                reweighted[key] = {
                    "raw_mean_perF": sum(a["perF"]) / len(a["perF"]),
                    "reweighted_mean_perF": rw,
                    "mean_FL": sum(a["FL"]) / len(a["FL"]),
                    "max_FL": max(a["FL"]),
                }
            print(f"{key}: N={out[key]['n_samples']}  "
                  f"perF med={out[key]['perF']['med']} mean={out[key]['perF']['mean']:.6g}  "
                  f"perU med={out[key]['perU']['med']} mean={out[key]['perU']['mean']:.6g}  "
                  f"merge_mean={out[key]['merge_mean']:.4f}")

    secs = time.time() - t0
    data = {
        **out,
        "one_row_perF": one_row_perF,
        "reweighted_perF_L2_L3": reweighted,
        "comparison_C6_L2_project": {
            "orbit_reps": 772,
            "total_labelled_extensions": 3663777792,
            "mean": 3663777792 / 772,
            "median": 4731392,
            "note": ("project numbers are unweighted over 772 two-row orbit "
                     "representatives; our sample is uniform over labelled "
                     "2-row states (F_2 = 1 exactly), i.e. orbit-size weighted"),
        },
        "bracket_violations": bracket_violations,
        "f3_spotchecks_closed_vs_generic": f3_spotchecks,
        "seed": seed,
        "seconds": secs,
        "caveats": [
            "L>=4 (and to a lesser degree L=3) raw means are history-count "
            "weighted (sequential construction); no exact reweighting done "
            "for L>=4.",
            "1/F_L reweighting for L=3 ignores variation of per(FREE) across "
            "intermediate 2-row states within a history, so it is itself an "
            "approximation to the uniform-over-states mean; exact for L=2 "
            "(F_2 = 1, sampler already uniform).",
        ],
    }
    print(f"\nbracket violations: {bracket_violations}")
    print(f"elapsed: {secs:.1f} s")
    print("\n===JSON===")
    print(json.dumps(data))
    return data


if __name__ == "__main__":
    n5 = int(sys.argv[1]) if len(sys.argv) > 1 else 300
    n6 = int(sys.argv[2]) if len(sys.argv) > 2 else 250
    seed = int(sys.argv[3]) if len(sys.argv) > 3 else 20260726
    run(n5, n6, seed)
