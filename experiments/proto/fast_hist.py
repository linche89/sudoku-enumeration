#!/usr/bin/env python3
"""Prototype + differential test for the orbit-aggregated within-side histogram.

ONE SIDE has C symbols, each with an old pair (xmask, ymask) (subsets of {0..C-1}).
We enumerate:
  - X-bijection: assign each symbol a DISTINCT X-col in {0..C-1}, col not in symbol's xmask
  - Y-bijection: independently, distinct Y-col, not in symbol's ymask
Each symbol's new pair = (xmask | {Xcol}, ymask | {Ycol}).
Histogram bins by the sorted multiset of the C new pairs.

brute_hist: enumerate all (C!)^2 valid (Xbij, Ybij).  Ground truth.
fast_hist:  group-level aggregation exploiting interchangeability of same-old-pair symbols.

The encoded pair value matches the C++: pair = xmask | (ymask << C).
"""
import itertools, random
from collections import defaultdict
from math import factorial

def mkpair(xm, ym, C):
    return xm | (ym << C)

def valid_bijections(masks, C):
    """All permutations perm (symbol i -> col perm[i]) with perm[i] not in masks[i]."""
    out = []
    for perm in itertools.permutations(range(C)):
        ok = True
        for i in range(C):
            if masks[i] & (1 << perm[i]):
                ok = False; break
        if ok:
            out.append(perm)
    return out

def brute_hist(xm, ym, C):
    """Ground truth: enumerate all valid (Xbij x Ybij). xm,ym lists of len C (the side's symbols)."""
    h = defaultdict(int)
    AX = valid_bijections(xm, C)
    AY = valid_bijections(ym, C)
    for ax in AX:
        for ay in AY:
            part = []
            for i in range(C):
                part.append(mkpair(xm[i] | (1 << ax[i]), ym[i] | (1 << ay[i]), C))
            part.sort()
            h[tuple(part)] += 1
    return dict(h)


def fast_hist(xm, ym, C):
    """Group-level aggregation.

    Group symbols by old pair (xm[i], ym[i]).  Symbols in a group are interchangeable.
    A configuration assigns to each group k a SET S_k of (a,b) pairs, |S_k| = n_k, where:
      - all a-values (X-cols) across groups are distinct & cover {0..C-1}
      - all b-values (Y-cols) across groups are distinct & cover {0..C-1}
      - each (a,b) in S_k has a not in xm_k, b not in ym_k
    Number of labeled (Xbij,Ybij) tuples realizing the group-config = prod_k n_k!
    (assign the n_k labeled symbols of group k to its n_k distinct slots).
    Output multiset depends only on {(xm_k|bit(a), ym_k|bit(b)) : (a,b) in S_k}.

    We enumerate group-configs by:
      - choosing, for each group, which X-cols it owns (a set partition of {0..C-1} into
        group-sized blocks, respecting validity), AND which Y-cols it owns;
      - then within a group, a bijection between its owned X-cols and owned Y-cols
        (which a pairs with which b).  Each such bijection is one S_k.
    Weight = prod_k n_k!  (labeled symbol assignment).

    To avoid enumerating set-partitions explicitly, note the X-side and Y-side ownership
    are themselves independent permutation problems PER COLUMN, but the pairing inside a
    group couples them.  We do a single recursive enumeration over X-cols, assigning each
    X-col to a group (validity a not in xm_group); simultaneously this is a multiset over
    groups.  Then independently the Y-cols.  Then within each group pair its X-cols to its
    Y-cols (all bijections).  Weight prod n_k! folds in.
    """
    # build groups
    pairs = list(zip(xm, ym))
    groups = {}
    order = []
    for p in pairs:
        if p not in groups:
            groups[p] = 0
            order.append(p)
        groups[p] += 1
    glist = order                      # list of distinct (xm,ym)
    ncnt  = [groups[g] for g in glist] # multiplicities
    G = len(glist)
    gx = [g[0] for g in glist]
    gy = [g[1] for g in glist]

    h = defaultdict(int)

    # Enumerate assignment of X-cols {0..C-1} to groups: xassign[a] = group index,
    # with each group k receiving exactly ncnt[k] cols, and a not in gx[k].
    # Same for Y-cols.  Then for each group, enumerate bijections between its X-cols
    # and Y-cols.
    # We enumerate xassign and yassign as functions col->group.
    def col_assignments():
        # yield all functions f: {0..C-1} -> group, with capacity ncnt[k] and validity
        results = []
        cap = ncnt[:]
        assign = [-1]*C
        def rec(a):
            if a == C:
                results.append(assign[:]); return
            for k in range(G):
                if cap[k] > 0 and not (gx[k] & (1<<a)):  # X validity here; reused for Y via param
                    pass
            # placeholder
        return results

    # Simpler: separate generators with their own validity masks.
    def gen_assign(validmask):  # validmask[k] = forbidden col mask for group k
        cap = ncnt[:]
        assign = [-1]*C
        out = []
        def rec(a):
            if a == C:
                out.append(assign[:]); return
            for k in range(G):
                if cap[k] > 0 and not (validmask[k] & (1<<a)):
                    cap[k]-=1; assign[a]=k
                    rec(a+1)
                    cap[k]+=1; assign[a]=-1
        rec(0)
        return out

    xassigns = gen_assign(gx)   # which group owns each X-col
    yassigns = gen_assign(gy)

    fact = [factorial(n) for n in ncnt]

    for xa in xassigns:
        # X-cols owned by each group
        xcols = [[] for _ in range(G)]
        for a in range(C): xcols[xa[a]].append(a)
        for ya in yassigns:
            ycols = [[] for _ in range(G)]
            for b in range(C): ycols[ya[b]].append(b)
            # within each group, pair its X-cols to its Y-cols via all bijections
            weight = 1
            for k in range(G): weight *= fact[k]
            # enumerate cartesian product of per-group bijections
            def group_bijs(k):
                # all bijections between xcols[k] (size n_k) and ycols[k] (size n_k)
                xs = xcols[k]; ys = ycols[k]
                res = []
                for perm in itertools.permutations(ys):
                    res.append(list(zip(xs, perm)))
                return res
            per_group = [group_bijs(k) for k in range(G)]
            for combo in itertools.product(*per_group):
                part = []
                for k in range(G):
                    for (a,b) in combo[k]:
                        part.append(mkpair(gx[k] | (1<<a), gy[k] | (1<<b), C))
                part.sort()
                h[tuple(part)] += weight
    return dict(h)


def random_side(C, rng):
    """Random old (xm,ym) for C symbols, ensuring at least one valid X-bij and Y-bij exist.
    masks are subsets of {0..C-1}; to guarantee a valid bijection exists we keep masks
    not-too-full but allow arbitrary including full-ish."""
    while True:
        xm = [rng.randrange(0, 1<<C) for _ in range(C)]
        ym = [rng.randrange(0, 1<<C) for _ in range(C)]
        if valid_bijections(xm, C) and valid_bijections(ym, C):
            return xm, ym


def diff_test(C, ntests, seed=12345):
    rng = random.Random(seed)
    mismatch = 0
    empty = 0
    for t in range(ntests):
        xm, ym = random_side(C, rng)
        b = brute_hist(xm, ym, C)
        f = fast_hist(xm, ym, C)
        if b != f:
            mismatch += 1
            if mismatch <= 5:
                print(f"MISMATCH C={C} test {t}: xm={xm} ym={ym}")
                print(f"  brute total={sum(b.values())} fast total={sum(f.values())}")
                # show diff keys
                allk = set(b)|set(f)
                for k in sorted(allk):
                    if b.get(k,0)!=f.get(k,0):
                        print(f"   key={k} brute={b.get(k,0)} fast={f.get(k,0)}")
        if not b:
            empty += 1
    print(f"C={C}: {ntests} tests, mismatches={mismatch}, empty={empty}")
    return mismatch == 0


if __name__ == "__main__":
    ok = True
    for C in [2,3,4]:
        ok &= diff_test(C, 2000 if C<=3 else 800)
    print("ALL OK" if ok else "FAILURES")
