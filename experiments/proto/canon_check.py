import itertools, random
# pair encoding: xmask | (ymask<<C). sort is on this packed value (ymask dominates).
def canon_brute(pairs, C):
    M=len(pairs)
    best=None
    perms=list(itertools.permutations(range(C)))
    def pm(mask,perm):
        r=0
        for b in range(C):
            if mask&(1<<b): r|=1<<perm[b]
        return r
    for px in perms:
        for py in perms:
            cand=sorted(((pm(p& ((1<<C)-1),px)) | (pm((p>>C)&((1<<C)-1),py)<<C) for p in pairs))
            cand=tuple(cand)
            if best is None or cand<best: best=cand
    return best

def canon_yrestrict(pairs, C):
    M=len(pairs)
    perms=list(itertools.permutations(range(C)))
    def pm(mask,perm):
        r=0
        for b in range(C):
            if mask&(1<<b): r|=1<<perm[b]
        return r
    # step1: find lex-min sorted ymask-multiset over py
    best_ym=None; goodpy=[]
    for py in perms:
        ym=tuple(sorted(pm((p>>C)&((1<<C)-1),py) for p in pairs))
        if best_ym is None or ym<best_ym:
            best_ym=ym; goodpy=[py]
        elif ym==best_ym:
            goodpy.append(py)
    # step2: among goodpy x all px, minimize full packed sorted
    best=None
    for px in perms:
        for py in goodpy:
            cand=tuple(sorted((pm(p&((1<<C)-1),px) | (pm((p>>C)&((1<<C)-1),py)<<C)) for p in pairs))
            if best is None or cand<best: best=cand
    return best

def test(C, n, seed):
    rng=random.Random(seed); mm=0
    FC=(1<<C)-1
    for _ in range(n):
        M=2*C
        pairs=[ (rng.randrange(0,1<<C)) | (rng.randrange(0,1<<C)<<C) for _ in range(M)]
        a=canon_brute(pairs,C); b=canon_yrestrict(pairs,C)
        if a!=b:
            mm+=1
            if mm<=3: print("MISMATCH C",C,"pairs",pairs,"\n brute",a,"\n yres ",b)
    print(f"C={C} n={n} mismatches={mm}")

for C in [2,3,4]:
    test(C, 4000 if C<4 else 1500, 100+C)
