import itertools, random
# Exact canonicalization with X-perm pruning via a valid lower bound.
# pair = xmask | (ymask<<C). Canon = lex-min sorted relabeled sequence over (pX,pY).
def pm(mask,perm,C):
    r=0
    for b in range(C):
        if mask&(1<<b): r|=1<<perm[b]
    return r

def canon_brute(pairs,C):
    perms=list(itertools.permutations(range(C)))
    best=None
    for px in perms:
        for py in perms:
            cand=tuple(sorted((pm(p&((1<<C)-1),px,C))|(pm((p>>C)&((1<<C)-1),py,C)<<C) for p in pairs))
            if best is None or cand<best: best=cand
    return best

def canon_bb(pairs,C):
    perms=list(itertools.permutations(range(C)))
    xms=[p&((1<<C)-1) for p in pairs]
    yms=[(p>>C)&((1<<C)-1) for p in pairs]
    M=len(pairs)
    # For each pX, a valid LOWER BOUND on the achievable sorted full-sequence:
    #   take relabeled xmasks under pX; the ymask contribution is >=0 but we don't know
    #   the pairing.  A valid LB element-wise: sort the relabeled xmasks alone (ymask=0
    #   would be the smallest possible high bits, but ymask CANNOT be 0 unless original is).
    #   Safe LB: sorted( xrel_i | (minYpacked) ) is NOT valid generally.
    # Instead: LB = sorted xmask multiset packed with the per-pair MINIMUM achievable ymask
    #   over all pY for that pair INDEPENDENTLY (relaxation: drop the permutation constraint
    #   on pY, allow each ym_i to be relabeled to its own min independently).
    #   min relabeled ymask of ym_i over all column perms = the mask with popcount bits at
    #   lowest positions = (1<<popcount(ym_i))-1.
    def minrelable(m,C):
        return (1<<bin(m).count('1'))-1
    best=None
    # order pX by sorted xrel multiset (helps find good best early)
    def xrel_sig(px):
        return tuple(sorted(pm(x,px,C) for x in xms))
    order=sorted(perms,key=xrel_sig)
    for px in order:
        xrel=[pm(x,px,C) for x in xms]
        # LB for this px: pair each xrel_i with the independent-min ymask, sort
        lb=tuple(sorted(xrel[i] | (minrelable(yms[i],C)<<C) for i in range(M)))
        if best is not None and lb>=best:
            continue  # prune: even the relaxed optimum can't beat best
        for py in perms:
            cand=tuple(sorted(xrel[i] | (pm(yms[i],py,C)<<C) for i in range(M)))
            if best is None or cand<best: best=cand
    return best

def test(C,n,seed):
    rng=random.Random(seed); mm=0
    for _ in range(n):
        M=2*C
        pairs=[rng.randrange(0,1<<C)|(rng.randrange(0,1<<C)<<C) for _ in range(M)]
        a=canon_brute(pairs,C); b=canon_bb(pairs,C)
        if a!=b:
            mm+=1
            if mm<=3: print("MISMATCH",C,pairs,a,b)
    print(f"C={C} n={n} mismatches={mm}")

for C in [2,3,4]:
    test(C,3000 if C<4 else 1000,200+C)

# measure prune rate
def canon_bb_count(pairs,C):
    perms=list(itertools.permutations(range(C)))
    xms=[p&((1<<C)-1) for p in pairs]; yms=[(p>>C)&((1<<C)-1) for p in pairs]; M=len(pairs)
    def minrelable(m,C): return (1<<bin(m).count('1'))-1
    best=None; pxdone=0; yloops=0
    def xrel_sig(px): return tuple(sorted(pm(x,px,C) for x in xms))
    order=sorted(perms,key=xrel_sig)
    for px in order:
        pxdone+=1
        xrel=[pm(x,px,C) for x in xms]
        lb=tuple(sorted(xrel[i]|(minrelable(yms[i],C)<<C) for i in range(M)))
        if best is not None and lb>=best: continue
        for py in perms:
            yloops+=1
            cand=tuple(sorted(xrel[i]|(pm(yms[i],py,C)<<C) for i in range(M)))
            if best is None or cand<best: best=cand
    return yloops
import statistics
for C in [4,5]:
    rng=random.Random(7); tot=[]
    for _ in range(300):
        M=2*C; pairs=[rng.randrange(0,1<<C)|(rng.randrange(0,1<<C)<<C) for _ in range(M)]
        tot.append(canon_bb_count(pairs,C))
    import math
    full=math.factorial(C)**2
    print(f"C={C}: avg yloops={statistics.mean(tot):.0f} (full (C!)^2={full}, ratio {statistics.mean(tot)/full:.3f})")
