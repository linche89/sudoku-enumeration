// multiset_q.cpp — N(2xC) quotient-transfer engine (2026-07-02).
//
// Same proven-correct state as multiset_fast/multiset_c6: the symbol-anonymous multiset
// of M=2C per-symbol (xmask,ymask) pairs.  Three exact reductions on top of multiset_c6:
//
//  (1) STACK-SWAP SYMMETRY.  The joint DP counts PAIRS (X,Y) of stack0 fillings sharing
//      the skeleton sequence; X and Y are interchangeable, so the state group extends from
//      S_C x S_C to (S_C x S_C) x Z2 (swap xmask<->ymask on every symbol).  canonMS2 takes
//      the min over both branches: ~2x fewer states.  Transitions are swap-equivariant
//      (buildHistFast treats X and Y identically), so orbit DP under the extended group is
//      exact.  Gate: C=2/3/4 must still give the exact N.
//
//  (2) SKELETON QUOTIENT (the HANDOFF §5 "quotient transfer matrix", finally viable).
//      For a source state P and skeleton A, the outgoing distribution over canonical
//      targets depends only on the canonical class of the TAGGED multiset (P, A):  tag the
//      C top symbols, canonicalise under the same extended group with tags riding along.
//      All (P,A) with the same tagged key share one transition computation.  This quotients
//      the (state x skeleton) enumeration by Aut(P) — a further ~|Aut| reduction of the
//      expensive cross-product work at the wide bands.
//
//  (3) T-SEEDED REFINEMENT CANON (from multiset_c6, 2026-07-02).  Real DP states are
//      column-regular, so degree-based refinement never splits them; seeding the initial
//      colours with the cross-stack incidence matrix T[a][b] (plus same-side co-occurrence
//      U,V) makes typical states discretise, cutting canon from (C!)^2 relabels to O(1).
//
// EXACTNESS is structural, not statistical: every canonical key produced here is the
// lex-min RELABELED COPY of its input over an explicitly enumerated subset of the group
// that is closed over refinement colour classes.  Equal keys therefore imply equal orbits
// (a key IS an orbit member) — under-merging is impossible by construction.  The only
// possible failure of a weak refinement is OVER-splitting, which costs speed, never
// correctness.  Invariance is nevertheless tested (canontest), and C=2/3/4 gates guard N.
//
// Verify: 288 (C=2), 28200960 (C=3), 29136487207403520 (C=4),
//   1903816047972624930994913280000 (C=5).
#include "big.hpp"
#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <array>
#include <vector>
#include <map>
#include <unordered_map>
#include <algorithm>
#include <string>
#include <functional>
#include <fstream>
#include <chrono>
#include <random>
#include <atomic>
#include <omp.h>

static int C, M, FULLC;
static std::vector<int> Askel;                 // all C-subsets of [2C]
static std::vector<std::array<int,8>> CP;      // perms of {0..C-1}
static std::vector<std::vector<int>> permMask; // permMask[p][mask]

static const int MAXM=12;
static const uint16_t TAG = 1u<<12;            // tag bit for skeleton-tagged canon (C<=6: pair fits 12 bits)
static const uint16_t VALMASK = TAG-1;
using Key = std::array<uint16_t,MAXM>;          // M sorted pairs, zero-padded
using PartKey = std::array<uint16_t,6>;         // one side's C pairs, sorted

struct KeyHash {
    size_t operator()(const Key& k) const {
        size_t h=1469598103934665603ull;
        for(int i=0;i<MAXM;++i){ h^=k[i]; h*=1099511628211ull; }
        return h;
    }
};

struct KeyPair {
    Key a{}, b{};
    bool operator==(const KeyPair& o) const { return a==o.a && b==o.b; }
};
struct KeyPairHash {
    size_t operator()(const KeyPair& p) const {
        KeyHash kh;
        size_t h=kh(p.a);
        h ^= kh(p.b) + 0x9e3779b97f4a7c15ull + (h<<6) + (h>>2);
        return h;
    }
};

struct PartKeyHash {
    size_t operator()(const PartKey& k) const {
        size_t h=1469598103934665603ull;
        for(uint16_t v:k){ h^=v; h*=1099511628211ull; }
        return h;
    }
};

static uint64_t partPairHash64(const PartKey& a, const PartKey& b){
    uint64_t h=1469598103934665603ull;
    for(uint16_t v:a){ h^=v; h*=1099511628211ull; }
    h^=0x9e3779b97f4a7c15ull; h*=1099511628211ull;
    for(uint16_t v:b){ h^=v; h*=1099511628211ull; }
    return h;
}

static PartKey canonPartBrute(const PartKey& p){
    uint16_t src[2][6]{};
    for(int i=0;i<C;++i){
        src[0][i]=p[i];
        uint16_t t=p[i];
        src[1][i]=(uint16_t)(((t>>C)&FULLC) | ((t&FULLC)<<C));
    }
    PartKey best{}; bool have=false;
    for(int br=0;br<2;++br){
        for(size_t xp=0; xp<CP.size(); ++xp){ const int* pmX=permMask[xp].data();
            for(size_t yp=0; yp<CP.size(); ++yp){ const int* pmY=permMask[yp].data();
                PartKey cand{};
                for(int i=0;i<C;++i){
                    int t=src[br][i];
                    cand[i]=(uint16_t)(pmX[t&FULLC] | (pmY[(t>>C)&FULLC]<<C));
                }
                std::sort(cand.begin(),cand.begin()+C);
                if(!have || cand<best){ best=cand; have=true; }
            }
        }
    }
    return best;
}

static void initPerms(){
    std::vector<int> p(C); for(int i=0;i<C;++i)p[i]=i;
    do { std::array<int,8> a{}; for(int i=0;i<C;++i)a[i]=p[i]; CP.push_back(a); } while(std::next_permutation(p.begin(),p.end()));
    permMask.assign(CP.size(), std::vector<int>(1<<C,0));
    for(size_t pi=0; pi<CP.size(); ++pi)
        for(int m=0;m<(1<<C);++m){ int r=0; for(int b=m;b;b&=b-1){int i=__builtin_ctz(b); r|=1<<CP[pi][i];} permMask[pi][m]=r; }
}

static inline int mkPair(int xm,int ym){ return xm | (ym<<C); }
static inline int pX(int t){ return t & FULLC; }
static inline int pY(int t){ return (t>>C) & FULLC; }
static inline uint16_t swapPair(uint16_t t){ return (uint16_t)(((t>>C)&FULLC) | ((t&FULLC)<<C)); }

static inline void isort(uint16_t* a, int n){
    for(int i=1;i<n;++i){ uint16_t v=a[i]; int j=i-1; while(j>=0 && a[j]>v){ a[j+1]=a[j]; --j; } a[j+1]=v; }
}
static inline int cmpArr(const uint16_t*a,const uint16_t*b,int n){
    for(int i=0;i<n;++i) if(a[i]!=b[i]) return a[i]<b[i]?-1:1; return 0;
}

static inline uint64_t factU(int n){ uint64_t f=1; for(int i=2;i<=n;++i)f*=i; return f; }

// ---- Big helpers for the duality combine (kept out of big.hpp: other TUs include it) ----
static Big mulBig(const Big& a, const Big& b){
    if(a.isZero()||b.isZero()) return Big();
    std::vector<unsigned __int128> t(a.d.size()+b.d.size(),0);
    for(size_t i=0;i<a.d.size();++i)
        for(size_t j=0;j<b.d.size();++j)
            t[i+j] += (unsigned __int128)a.d[i]*b.d[j];
    Big r; r.d.assign(t.size(),0);
    unsigned __int128 carry=0;
    for(size_t i=0;i<t.size();++i){ unsigned __int128 cur=t[i]+carry; r.d[i]=(uint32_t)(cur%Big::BASE); carry=cur/Big::BASE; }
    while(carry){ r.d.push_back((uint32_t)(carry%Big::BASE)); carry/=Big::BASE; }
    r.trim(); return r;
}
// a /= q exactly; returns false (and leaves the quotient) if a remainder was lost.
static bool divExactU64(Big& a, uint64_t q){
    unsigned __int128 rem=0;
    for(int i=(int)a.d.size()-1;i>=0;--i){
        unsigned __int128 cur=rem*Big::BASE + a.d[i];
        a.d[i]=(uint32_t)(cur/q); rem=cur%q;
    }
    a.trim(); return rem==0;
}

// ---- T-seeded equitable refinement (see multiset_c6.cpp for the full rationale) ----
static void refineColours(const uint16_t* pairs, int xcol[8], int ycol[8]){
    uint8_t T[8][8]={}, U[8][8]={}, V[8][8]={};
    for(int i=0;i<M;++i){
        int xm=pairs[i]&FULLC, ym=(pairs[i]>>C)&FULLC;
        for(int bx=xm; bx; bx&=bx-1){
            int a=__builtin_ctz(bx);
            for(int by=ym; by; by&=by-1) T[a][__builtin_ctz(by)]++;
            for(int b2=bx&(bx-1); b2; b2&=b2-1){ int a2=__builtin_ctz(b2); U[a][a2]++; U[a2][a]++; }
        }
        for(int by=ym; by; by&=by-1){
            int b=__builtin_ctz(by);
            for(int b2=by&(by-1); b2; b2&=b2-1){ int bb=__builtin_ctz(b2); V[b][bb]++; V[bb][b]++; }
        }
    }
    for(int c=0;c<C;++c){ xcol[c]=0; ycol[c]=0; }
    auto rank2=[&](std::pair<uint64_t,uint64_t>* s, int* col)->bool{
        int o[8]; for(int c=0;c<C;++c)o[c]=c;
        std::sort(o,o+C,[&](int A,int B){return s[A]<s[B];});
        int nc[8],g=0; nc[o[0]]=0;
        for(int i=1;i<C;++i){ if(s[o[i]]!=s[o[i-1]])++g; nc[o[i]]=g; }
        bool ch=false; for(int c=0;c<C;++c){ if(col[c]!=nc[c])ch=true; col[c]=nc[c]; }
        return ch;
    };
    auto sideSig=[&](const uint8_t cross[8][8], const uint8_t same[8][8],
                     const int* otherCol, const int* ownCol, bool transpose,
                     std::pair<uint64_t,uint64_t>* sig){
        for(int a=0;a<C;++a){
            uint8_t o[16], u[16];
            for(int b=0;b<C;++b) o[b]=(uint8_t)(otherCol[b]*16 + (transpose?cross[b][a]:cross[a][b]));
            for(int a2=0;a2<C;++a2) u[a2]=(uint8_t)(ownCol[a2]*16 + same[a][a2]);
            std::sort(o,o+C); std::sort(u,u+C);
            uint64_t p1=(uint64_t)ownCol[a], p2=0;
            for(int b=0;b<C;++b) p1=(p1<<8)|o[b];
            for(int b=0;b<C;++b) p2=(p2<<8)|u[b];
            sig[a]={p1,p2};
        }
    };
    for(int it=0; it<2*C+3; ++it){
        std::pair<uint64_t,uint64_t> sx[8], sy[8];
        sideSig(T,U,ycol,xcol,false,sx);
        sideSig(T,V,xcol,ycol,true, sy);
        bool c1=rank2(sx,xcol), c2=rank2(sy,ycol);
        if(!c1&&!c2)break;
    }
    // fall back to the finer per-symbol co-occurrence refinement if not yet discrete
    // (allocation-free version of the multiset_c6 loop; heap use here would serialise
    // threads on the CRT allocator lock)
    {
        int seenx=0, seeny=0;
        for(int c=0;c<C;++c){ seenx|=1<<xcol[c]; seeny|=1<<ycol[c]; }
        if(__builtin_popcount(seenx)<C || __builtin_popcount(seeny)<C){
            struct FSig { int n; uint32_t v[13]; };
            auto lessF=[](const FSig&a,const FSig&b){
                if(a.n!=b.n) return a.n<b.n;
                for(int i=0;i<a.n;++i) if(a.v[i]!=b.v[i]) return a.v[i]<b.v[i];
                return false;
            };
            auto rankF=[&](FSig* s,int* col)->bool{
                int o[8]; for(int c=0;c<C;++c)o[c]=c;
                std::sort(o,o+C,[&](int A,int B){return lessF(s[A],s[B]);});
                int nc[8],g=0; nc[o[0]]=0;
                for(int i=1;i<C;++i){ if(lessF(s[o[i-1]],s[o[i]]))++g; nc[o[i]]=g; }
                bool ch=false; for(int c=0;c<C;++c){ if(col[c]!=nc[c])ch=true; col[c]=nc[c]; }
                return ch;
            };
            for(int it=0; it<2*C+3; ++it){
                FSig xs[8], ys[8];
                for(int c=0;c<C;++c){ xs[c].n=1; xs[c].v[0]=(uint32_t)xcol[c]; ys[c].n=1; ys[c].v[0]=(uint32_t)ycol[c]; }
                for(int i=0;i<M;++i){ int xm=pairs[i]&FULLC, ym=(pairs[i]>>C)&FULLC;
                    uint32_t yh[8]={0}; for(int b=ym;b;b&=b-1) yh[ycol[__builtin_ctz(b)]]++;
                    uint32_t xh[8]={0}; for(int b=xm;b;b&=b-1) xh[xcol[__builtin_ctz(b)]]++;
                    uint32_t yco=0; for(int q=0;q<C;++q) yco=yco*16+yh[q];
                    uint32_t xco=0; for(int q=0;q<C;++q) xco=xco*16+xh[q];
                    for(int b=xm;b;b&=b-1){ FSig&s=xs[__builtin_ctz(b)]; s.v[s.n++]=yco; }
                    for(int b=ym;b;b&=b-1){ FSig&s=ys[__builtin_ctz(b)]; s.v[s.n++]=xco; } }
                for(int c=0;c<C;++c){ std::sort(xs[c].v+1,xs[c].v+xs[c].n); std::sort(ys[c].v+1,ys[c].v+ys[c].n); }
                bool c1=rankF(xs,xcol), c2=rankF(ys,ycol);
                if(!c1&&!c2)break;
            }
        }
    }
}

// Allocation-free odometer over the within-colour-class permutations of one side.
// (Heap allocations here were the real 32-thread bottleneck: the Windows CRT allocator
// serialises on a global lock, degrading a ~6us canon to ~100us under contention.)
struct ClassPerms {
    int ncol=0;
    int mem[8][8]; int msz[8]; int base[8];
    int cur[8][8];                 // current ordering of each class's member columns
    void init(const int* col){
        ncol=0; for(int c=0;c<C;++c) ncol=std::max(ncol,col[c]); ncol++;
        for(int g=0;g<ncol;++g) msz[g]=0;
        for(int c=0;c<C;++c){ int g=col[c]; mem[g][msz[g]++]=c; }   // members ascend in c
        int acc=0; for(int g=0;g<ncol;++g){ base[g]=acc; acc+=msz[g]; }
    }
    void first(int* perm){
        for(int g=0;g<ncol;++g)
            for(int j=0;j<msz[g];++j){ cur[g][j]=mem[g][j]; perm[mem[g][j]]=base[g]+j; }
    }
    bool next(int* perm){
        for(int g=0;g<ncol;++g){
            if(std::next_permutation(cur[g],cur[g]+msz[g])){
                for(int j=0;j<msz[g];++j) perm[cur[g][j]]=base[g]+j;
                return true;
            }
            // wrapped: cur[g] is back to sorted order; reset perm entries and carry
            for(int j=0;j<msz[g];++j) perm[cur[g][j]]=base[g]+j;
        }
        return false;
    }
};

// residual-search min over one branch (no swap), writing into best/have.
static void canonBranch(const uint16_t* pairs, uint16_t* bestArr, bool& have){
    int xcol[8],ycol[8]; refineColours(pairs,xcol,ycol);
    ClassPerms xcp,ycp; xcp.init(xcol); ycp.init(ycol);
    uint16_t xms[MAXM],yms[MAXM]; for(int i=0;i<M;++i){ xms[i]=pairs[i]&FULLC; yms[i]=(pairs[i]>>C)&FULLC; }
    uint16_t cand[MAXM];
    int xperm[8], yperm[8];
    xcp.first(xperm);
    do{
        uint16_t xr[MAXM];
        for(int i=0;i<M;++i){ int r=0; for(int b=xms[i];b;b&=b-1)r|=1<<xperm[__builtin_ctz(b)]; xr[i]=(uint16_t)r; }
        ycp.first(yperm);
        do{
            for(int i=0;i<M;++i){ int r=0; for(int b=yms[i];b;b&=b-1)r|=1<<yperm[__builtin_ctz(b)]; cand[i]=(uint16_t)(xr[i]|(r<<C)); }
            isort(cand,M);
            if(!have){ std::memcpy(bestArr,cand,M*sizeof(uint16_t)); have=true; continue; }
            if(cmpArr(cand,bestArr,M)<0) std::memcpy(bestArr,cand,M*sizeof(uint16_t));
        } while(ycp.next(yperm));
    } while(xcp.next(xperm));
}

// canonical form under (S_C x S_C) x Z2(stack swap).
static Key canonMS2(const uint16_t* pairs){
    uint16_t bestArr[MAXM]; bool have=false;
    canonBranch(pairs,bestArr,have);
    uint16_t sw[MAXM]; for(int i=0;i<M;++i) sw[i]=swapPair(pairs[i]);
    canonBranch(sw,bestArr,have);
    Key best{}; for(int i=0;i<M;++i) best[i]=bestArr[i];
    return best;
}

// brute canonical form over the full extended group (validation only).
static Key canonBrute2(const uint16_t* pairs){
    uint16_t bestArr[MAXM]; bool have=false; uint16_t cand[MAXM];
    uint16_t src[2][MAXM];
    for(int i=0;i<M;++i){ src[0][i]=pairs[i]; src[1][i]=swapPair(pairs[i]); }
    for(int br=0;br<2;++br)
    for(size_t rp=0; rp<CP.size(); ++rp){ const int* pmX=permMask[rp].data();
        for(size_t cp=0; cp<CP.size(); ++cp){ const int* pmY=permMask[cp].data();
            for(int i=0;i<M;++i){ int t=src[br][i]; cand[i]=(uint16_t)(pmX[t&FULLC]|(pmY[(t>>C)&FULLC]<<C)); }
            isort(cand,M);
            if(!have || cmpArr(cand,bestArr,M)<0){ std::memcpy(bestArr,cand,M*2); have=true; }
        }
    }
    Key best{}; for(int i=0;i<M;++i)best[i]=bestArr[i]; return best;
}

// ---- band-reversal duality helpers ----
// complement state: every symbol's remaining columns.  Completing v over the last k bands
// is the same layered problem as filling k bands from empty ending at comp(v), band for
// band, so g_b[v] = f_{C-b}[comp v] (the C=5 palindrome 7/38801/38801/7 with identical
// essential counts is this fact observed in the wild).
static Key complementKey(const Key& k){
    uint16_t r[MAXM]{};
    for(int i=0;i<M;++i){
        int x=k[i]&FULLC, y=(k[i]>>C)&FULLC;
        r[i]=(uint16_t)((FULLC&~x)|((FULLC&~y)<<C));
    }
    return canonMS2(r);
}
// discrete refinement <=> the engine key is the true orbit canon (the colour-aligned
// relabel is unique per branch, and it is an isomorphism invariant).
static bool isDiscreteRep(const Key& k){
    int xcol[8],ycol[8]; refineColours(k.data(),xcol,ycol);
    int sx=0,sy=0; for(int c=0;c<C;++c){ sx|=1<<xcol[c]; sy|=1<<ycol[c]; }
    return __builtin_popcount(sx)==C && __builtin_popcount(sy)==C;
}
// stabiliser order of a canonBranch-produced representative inside (S_C x S_C) x Z2.
// Automorphisms respect the refinement colouring, and canon representatives have their
// colour classes on contiguous slot ranges, so the ClassPerms enumeration (which sends
// each class onto its slot range) covers the stabiliser exactly.
static uint64_t autCount(const uint16_t* pairs){
    uint16_t src[2][MAXM]; int xcol[2][8], ycol[2][8];
    for(int i=0;i<M;++i){ src[0][i]=pairs[i]; src[1][i]=swapPair(pairs[i]); }
    for(int br=0;br<2;++br) refineColours(src[br],xcol[br],ycol[br]);
    uint64_t cnt=0;
    uint16_t cand[MAXM];
    for(int br=0;br<2;++br){
        uint16_t xms[MAXM],yms[MAXM];
        for(int i=0;i<M;++i){ xms[i]=src[br][i]&FULLC; yms[i]=(src[br][i]>>C)&FULLC; }
        ClassPerms xcp,ycp; xcp.init(xcol[br]); ycp.init(ycol[br]);
        int xperm[8], yperm[8];
        xcp.first(xperm);
        do{
            uint16_t xr[MAXM];
            for(int i=0;i<M;++i){ int r=0; for(int b=xms[i];b;b&=b-1)r|=1<<xperm[__builtin_ctz(b)]; xr[i]=(uint16_t)r; }
            ycp.first(yperm);
            do{
                for(int i=0;i<M;++i){ int r=0; for(int b=yms[i];b;b&=b-1)r|=1<<yperm[__builtin_ctz(b)]; cand[i]=(uint16_t)(xr[i]|(r<<C)); }
                isort(cand,M);
                if(cmpArr(cand,pairs,M)==0) ++cnt;
            } while(ycp.next(yperm));
        } while(xcp.next(xperm));
    }
    return cnt;
}

// Number of concrete labelled quotient states represented by a canonical multiset key.
// The DP weights are stored on quotient states, so a meet-in-the-middle dual combine must
// divide by this value; otherwise the shared middle state is counted once per representative.
static uint64_t concreteStateCount(const Key& k){
    uint64_t labels=factU(M);
    int i=0;
    while(i<M){
        int j=i+1; while(j<M && k[j]==k[i]) ++j;
        labels /= factU(j-i);
        i=j;
    }
    uint64_t group = 2 * factU(C) * factU(C); // S_C x S_C x optional stack swap
    uint64_t aut = autCount(k.data());
    return (group / aut) * labels;
}

// ---- skeleton-tagged canon, batched per source state ----
// Precomputes refinement + residual perms once per state (both swap branches), then
// canonicalises the tagged multiset for every skeleton A cheaply.
struct TaggedCanon {
    uint16_t src[2][MAXM];
    int xcol[2][8], ycol[2][8];
    void init(const uint16_t* pairs){
        for(int i=0;i<M;++i){ src[0][i]=pairs[i]; src[1][i]=swapPair(pairs[i]); }
        for(int br=0;br<2;++br) refineColours(src[br],xcol[br],ycol[br]);
    }
    // A = bitmask over element positions 0..M-1 (top symbols)
    Key canon(int A) const {
        uint16_t bestArr[MAXM]; bool have=false; uint16_t cand[MAXM];
        for(int br=0;br<2;++br){
            const uint16_t* pr=src[br];
            uint16_t xms[MAXM],yms[MAXM],tg[MAXM];
            for(int i=0;i<M;++i){ xms[i]=pr[i]&FULLC; yms[i]=(pr[i]>>C)&FULLC; tg[i]=(A>>i)&1?TAG:0; }
            ClassPerms xcp,ycp; xcp.init(xcol[br]); ycp.init(ycol[br]);
            int xperm[8], yperm[8];
            xcp.first(xperm);
            do{
                uint16_t xr[MAXM];
                for(int i=0;i<M;++i){ int r=0; for(int b=xms[i];b;b&=b-1)r|=1<<xperm[__builtin_ctz(b)]; xr[i]=(uint16_t)r; }
                ycp.first(yperm);
                do{
                    for(int i=0;i<M;++i){ int r=0; for(int b=yms[i];b;b&=b-1)r|=1<<yperm[__builtin_ctz(b)]; cand[i]=(uint16_t)(xr[i]|(r<<C)|tg[i]); }
                    isort(cand,M);
                    if(!have){ std::memcpy(bestArr,cand,M*sizeof(uint16_t)); have=true; continue; }
                    if(cmpArr(cand,bestArr,M)<0) std::memcpy(bestArr,cand,M*sizeof(uint16_t));
                } while(ycp.next(yperm));
            } while(xcp.next(xperm));
        }
        Key best{}; for(int i=0;i<M;++i) best[i]=bestArr[i];
        return best;
    }
};

// ---- global lock-free raw->canon cache ----
// canonMS2 is a pure function of the raw multiset, and the same raws recur massively
// across essential classes (band-2 of C=5: ~10^10 cross pairs but only a few 10^8 distinct
// raws).  A fixed-capacity open-addressing table with atomic publication makes every
// repeat a single DRAM probe.  Correctness: full 24-byte raw keys are compared (fingerprint
// collisions cannot alias), and on any probe failure/full table we simply recompute the
// canon — degradation is performance-only.
static bool gProfile=false;
static bool gPartOrbitAudit=false;
static long long gPartOrbitMax=0;

struct CacheStatsSnapshot {
    unsigned long long hits=0, misses=0, inserts=0, probeFail=0;
};

struct RawCanonCache {
    size_t cap=0, mask=0;
    std::vector<std::atomic<uint64_t>> fp;   // 0=empty, 1=reserved, else hash(|2)
    std::vector<uint16_t> rawv, ckv;         // cap*MAXM each
    std::atomic<unsigned long long> statHits{0}, statMisses{0}, statInserts{0}, statProbeFail{0};
    static uint64_t hash64(const Key& k){
        uint64_t h=1469598103934665603ull;
        for(int i=0;i<MAXM;++i){ h^=k[i]; h*=1099511628211ull; }
        h^=h>>33; h*=0xff51afd7ed558ccdull; h^=h>>33;
        return h|2ull;                        // never 0 or 1
    }
    void init(int log2cap){
        cap=1ull<<log2cap; mask=cap-1;
        fp=std::vector<std::atomic<uint64_t>>(cap);
        rawv.assign(cap*MAXM,0); ckv.assign(cap*MAXM,0);
    }
    void resetStats(){
        statHits.store(0,std::memory_order_relaxed);
        statMisses.store(0,std::memory_order_relaxed);
        statInserts.store(0,std::memory_order_relaxed);
        statProbeFail.store(0,std::memory_order_relaxed);
    }
    CacheStatsSnapshot stats() const {
        CacheStatsSnapshot s;
        s.hits=statHits.load(std::memory_order_relaxed);
        s.misses=statMisses.load(std::memory_order_relaxed);
        s.inserts=statInserts.load(std::memory_order_relaxed);
        s.probeFail=statProbeFail.load(std::memory_order_relaxed);
        return s;
    }
    Key get(const Key& raw){
        // Linear probe; STOP at the first empty slot (the key cannot be past it except in a
        // benign concurrent-insert race, whose worst case is one redundant canon computation).
        const int PROBE=64;
        uint64_t h=hash64(raw); size_t idx=h&mask;
        long long freeIdx=-1;
        for(int p=0;p<PROBE;++p){
            uint64_t f=fp[idx].load(std::memory_order_acquire);
            if(f==h){
                const uint16_t* r=&rawv[idx*MAXM];
                bool eq=true; for(int i=0;i<MAXM;++i) if(r[i]!=raw[i]){eq=false;break;}
                if(eq){
                    if(gProfile) statHits.fetch_add(1,std::memory_order_relaxed);
                    Key ck; std::memcpy(ck.data(),&ckv[idx*MAXM],MAXM*2); return ck;
                }
            } else if(f==0){ freeIdx=(long long)idx; break; }
            idx=(idx+1)&mask;
        }
        if(gProfile){
            statMisses.fetch_add(1,std::memory_order_relaxed);
            if(freeIdx<0) statProbeFail.fetch_add(1,std::memory_order_relaxed);
        }
        Key ck=canonMS2(raw.data());
        if(freeIdx>=0){
            uint64_t expect=0;
            if(fp[freeIdx].compare_exchange_strong(expect,1ull,std::memory_order_acq_rel)){
                std::memcpy(&rawv[freeIdx*MAXM],raw.data(),MAXM*2);
                std::memcpy(&ckv[freeIdx*MAXM],ck.data(),MAXM*2);
                fp[freeIdx].store(h,std::memory_order_release);
                if(gProfile) statInserts.fetch_add(1,std::memory_order_relaxed);
            }
        }
        return ck;
    }
};
static RawCanonCache gCache;

struct PhaseStats {
    unsigned long long tasks=0, emptyTop=0, emptyBot=0;
    unsigned long long sideHits=0, sideMisses=0, sideClears=0;
    unsigned long long topEntries=0, botEntries=0, crossPairs=0, targets=0;
    unsigned long long maxTop=0, maxBot=0, maxCross=0, maxTargets=0;
    void add(const PhaseStats& o){
        tasks+=o.tasks; emptyTop+=o.emptyTop; emptyBot+=o.emptyBot;
        sideHits+=o.sideHits; sideMisses+=o.sideMisses; sideClears+=o.sideClears;
        topEntries+=o.topEntries; botEntries+=o.botEntries; crossPairs+=o.crossPairs; targets+=o.targets;
        maxTop=std::max(maxTop,o.maxTop); maxBot=std::max(maxBot,o.maxBot);
        maxCross=std::max(maxCross,o.maxCross); maxTargets=std::max(maxTargets,o.maxTargets);
    }
};

// ---- FAST aggregated within-side histogram (unchanged from multiset_fast, difftested) ----
template<class CB>
static void genColAssign(const int* validmask, const int* gn, int G, int Ccols, CB&& cb){
    int cap[8]; for(int k=0;k<G;++k)cap[k]=gn[k];
    int assign[8];
    struct Rec {
        const int* validmask; const int* gnn; int G; int Cc; int* cap; int* assign; CB& cb;
        void go(int a){
            if(a==Cc){ cb(assign); return; }
            for(int k=0;k<G;++k){
                if(cap[k]>0 && !(validmask[k] & (1<<a))){
                    cap[k]--; assign[a]=k;
                    go(a+1);
                    cap[k]++;
                }
            }
        }
    } r{validmask, gn, G, Ccols, cap, assign, cb};
    r.go(0);
}

static void buildHistFast(const int* syms, const std::vector<int>& xm, const std::vector<int>& ym,
                          std::map<PartKey,uint64_t>& out){
    int gx[8], gy[8], gn[8], G=0;
    int sx[8], sy[8];
    for(int i=0;i<C;++i){ sx[i]=xm[syms[i]]; sy[i]=ym[syms[i]]; }
    for(int i=0;i<C;++i){
        int k=-1; for(int j=0;j<G;++j) if(gx[j]==sx[i]&&gy[j]==sy[i]){k=j;break;}
        if(k<0){ k=G++; gx[k]=sx[i]; gy[k]=sy[i]; gn[k]=0; }
        gn[k]++;
    }
    uint64_t gfact=1; for(int k=0;k<G;++k) gfact*=factU(gn[k]);

    std::vector<std::array<int,8>> xAs, yAs;
    {
        int va[8]; for(int k=0;k<G;++k)va[k]=gx[k];
        genColAssign(va, gn, G, C, [&](const int* assign){ std::array<int,8> a{}; for(int c=0;c<C;++c)a[c]=assign[c]; xAs.push_back(a); });
        for(int k=0;k<G;++k)va[k]=gy[k];
        genColAssign(va, gn, G, C, [&](const int* assign){ std::array<int,8> a{}; for(int c=0;c<C;++c)a[c]=assign[c]; yAs.push_back(a); });
    }
    if(xAs.empty()||yAs.empty()) return;

    PartKey part{};
    for(const auto& xa : xAs){
        int xcols[8][8], xn[8]={0};
        for(int c=0;c<C;++c){ int k=xa[c]; xcols[k][xn[k]++]=c; }
        for(const auto& ya : yAs){
            int ycols[8][8], yn[8]={0};
            for(int c=0;c<C;++c){ int k=ya[c]; ycols[k][yn[k]++]=c; }
            struct GR {
                int G; const int* gx; const int* gy; int (*xcols)[8]; int (*ycols)[8]; const int* xn;
                PartKey& part; int pos; uint64_t gfact; std::map<PartKey,uint64_t>& out; int C;
                void go(int k){
                    if(k==G){
                        PartKey p2=part; std::sort(p2.begin(), p2.begin()+pos);
                        out[p2]+=gfact;
                        return;
                    }
                    int n=xn[k];
                    int perm[8]; for(int i=0;i<n;++i)perm[i]=i;
                    do{
                        int save=pos;
                        for(int i=0;i<n;++i){
                            int a=xcols[k][i], b=ycols[k][perm[i]];
                            part[pos++]=(uint16_t)((gx[k]|(1<<a)) | ((gy[k]|(1<<b))<<C));
                        }
                        go(k+1);
                        pos=save;
                    } while(std::next_permutation(perm,perm+n));
                }
            } gr{G,gx,gy,xcols,ycols,xn,part,0,gfact,out,C};
            gr.go(0);
        }
    }
}

struct HistDPKey {
    uint16_t ymask=0, code=0;
    PartKey part{};
    bool operator==(const HistDPKey& o) const { return ymask==o.ymask && code==o.code && part==o.part; }
};
struct HistDPKeyHash {
    size_t operator()(const HistDPKey& k) const {
        size_t h=1469598103934665603ull;
        h^=k.ymask; h*=1099511628211ull;
        h^=k.code; h*=1099511628211ull;
        for(uint16_t v:k.part){ h^=v; h*=1099511628211ull; }
        return h;
    }
};

// Side histogram as a coloured restricted permanent over X columns and Y columns.
// This is a prototype foundation for the direct middle-band kernel: it avoids the
// xAssignments*yAssignments materialisation in buildHistFast, while producing the
// exact same PartKey histogram.
static void buildHistPermDP(const int* syms, const std::vector<int>& xm, const std::vector<int>& ym,
                            std::map<PartKey,uint64_t>& out){
    int gx[8], gy[8], gn[8], G=0;
    int sx[8], sy[8];
    for(int i=0;i<C;++i){ sx[i]=xm[syms[i]]; sy[i]=ym[syms[i]]; }
    for(int i=0;i<C;++i){
        int k=-1; for(int j=0;j<G;++j) if(gx[j]==sx[i]&&gy[j]==sy[i]){ k=j; break; }
        if(k<0){ k=G++; gx[k]=sx[i]; gy[k]=sy[i]; gn[k]=0; }
        gn[k]++;
    }
    uint64_t gfact=1; for(int k=0;k<G;++k) gfact*=factU(gn[k]);
    uint16_t mult[8]; mult[0]=1;
    for(int k=1;k<G;++k) mult[k]=(uint16_t)(mult[k-1]*(gn[k-1]+1));
    uint16_t targetCode=0; for(int k=0;k<G;++k) targetCode += (uint16_t)(gn[k]*mult[k]);
    int legal[8][8][8]; int ln[8][8]={};
    for(int a=0;a<C;++a) for(int b=0;b<C;++b)
        for(int g=0; g<G; ++g)
            if(!(gx[g]&(1<<a)) && !(gy[g]&(1<<b)))
                legal[a][b][ln[a][b]++]=g;

    std::unordered_map<HistDPKey,uint64_t,HistDPKeyHash> cur,nx;
    cur.reserve(128);
    cur[HistDPKey{}]=1;
    for(int a=0;a<C;++a){
        nx.clear();
        nx.reserve(cur.size()*C);
        for(const auto& kv:cur){
            const HistDPKey& st=kv.first;
            uint64_t ways=kv.second;
            for(int b=0;b<C;++b){
                if(st.ymask&(1<<b)) continue;
                for(int ii=0; ii<ln[a][b]; ++ii){
                    int g=legal[a][b][ii];
                    int used=(st.code/mult[g])%(gn[g]+1);
                    if(used>=gn[g]) continue;
                    HistDPKey ns=st;
                    ns.ymask |= (uint16_t)(1<<b);
                    ns.code += mult[g];
                    ns.part[a]=(uint16_t)((gx[g]|(1<<a)) | ((gy[g]|(1<<b))<<C));
                    std::sort(ns.part.begin(), ns.part.begin()+a+1);
                    nx[ns]+=ways;
                }
            }
        }
        cur.swap(nx);
    }
    for(const auto& kv:cur){
        if(kv.first.ymask==(uint16_t)FULLC && kv.first.code==targetCode)
            out[kv.first.part] += kv.second * gfact;
    }
}

static Key makeSideKey(const uint16_t* ms){
    Key sk{};
    for(int i=0;i<C;++i) sk[i]=ms[i];
    return sk;
}

static std::vector<std::pair<PartKey,uint64_t>> buildSideHistFromKey(const Key& sk){
    std::vector<int> xm(C),ym(C); int syms[8];
    for(int i=0;i<C;++i){ syms[i]=i; xm[i]=pX(sk[i]); ym[i]=pY(sk[i]); }
    std::map<PartKey,uint64_t> hm;
    buildHistFast(syms,xm,ym,hm);
    return std::vector<std::pair<PartKey,uint64_t>>(hm.begin(),hm.end());
}

static std::string histBlob(const std::vector<std::pair<PartKey,uint64_t>>& h){
    std::string s;
    s.reserve(h.size()*(sizeof(uint64_t)+sizeof(uint16_t)*6));
    for(const auto& e:h){
        s.append(reinterpret_cast<const char*>(e.first.data()), sizeof(uint16_t)*6);
        s.append(reinterpret_cast<const char*>(&e.second), sizeof(uint64_t));
    }
    return s;
}

static void auditEssentialBand(const std::vector<std::pair<Key,const Big*>>& ess, long long auditMax){
    long long limit=(auditMax>0 && auditMax<(long long)ess.size())?auditMax:(long long)ess.size();
    std::unordered_map<Key,int,KeyHash> sideToHist;
    std::unordered_map<Key,unsigned long long,KeyHash> sideSize;
    std::unordered_map<Key,int,KeyHash> topSides, botSides;
    std::unordered_map<KeyPair,int,KeyPairHash> sidePairs;
    std::unordered_map<std::string,int> histIds;
    std::unordered_map<unsigned long long,int> histPairs;
    unsigned long long topEntries=0, botEntries=0, crossPairs=0;
    unsigned long long maxTop=0, maxBot=0, maxCross=0;
    auto getHistId=[&](const Key& side)->int{
        auto it=sideToHist.find(side);
        if(it!=sideToHist.end()) return it->second;
        auto flat=buildSideHistFromKey(side);
        sideSize.emplace(side,(unsigned long long)flat.size());
        std::string blob=histBlob(flat);
        auto hi=histIds.find(blob);
        int id;
        if(hi==histIds.end()){
            id=(int)histIds.size();
            histIds.emplace(std::move(blob),id);
        } else id=hi->second;
        sideToHist.emplace(side,id);
        return id;
    };
    for(long long ei=0; ei<limit; ++ei){
        const Key& tk=ess[(size_t)ei].first;
        uint16_t topMS[8], botMS[8]; int nt=0, nb=0;
        for(int i=0;i<M;++i){ if(tk[i]&TAG) topMS[nt++]=(uint16_t)(tk[i]&VALMASK); else botMS[nb++]=tk[i]; }
        isort(topMS,C); isort(botMS,C);
        Key top=makeSideKey(topMS), bot=makeSideKey(botMS);
        topSides.emplace(top,0);
        botSides.emplace(bot,0);
        sidePairs.emplace(KeyPair{top,bot},0);
        int ht=getHistId(top), hb=getHistId(bot);
        histPairs.emplace((unsigned long long)(uint32_t)ht<<32 | (uint32_t)hb,0);
        unsigned long long hts=sideSize[top], hbs=sideSize[bot];
        unsigned long long cr=hts*hbs;
        topEntries+=hts; botEntries+=hbs;
        crossPairs+=cr;
        maxTop=std::max(maxTop,hts); maxBot=std::max(maxBot,hbs); maxCross=std::max(maxCross,cr);
    }
    double den=limit?double(limit):1.0;
    std::fprintf(stderr,
        "  audit: inspected=%lld/%zu uniqueTopSide=%zu uniqueBotSide=%zu uniqueSideAny=%zu uniqueSidePair=%zu\n"
        "    uniqueHist=%zu uniqueHistPair=%zu avgTopHist=%.2f avgBotHist=%.2f avgCross=%.2f maxTop=%llu maxBot=%llu maxCross=%llu\n",
        limit,ess.size(),topSides.size(),botSides.size(),sideToHist.size(),sidePairs.size(),
        histIds.size(),histPairs.size(),topEntries/den,botEntries/den,crossPairs/den,maxTop,maxBot,maxCross);
    std::fflush(stderr);
}

static void targetAuditEssentialBand(const std::vector<std::pair<Key,const Big*>>& ess, long long auditMax){
    long long limit=(auditMax>0 && auditMax<(long long)ess.size())?auditMax:(long long)ess.size();
    std::unordered_map<Key,std::vector<std::pair<PartKey,uint64_t>>,KeyHash> sideMemo;
    std::unordered_map<Key,int,KeyHash> globalTargets;
    std::unordered_map<PartKey,int,PartKeyHash> globalTopParts, globalBotParts;
    std::unordered_map<uint64_t,int> globalPartPairs;
    std::unordered_map<Key,uint64_t,KeyHash> wtSum;
    unsigned long long crossPairs=0, targetRefs=0, maxTargets=0;
    unsigned long long maxTop=0, maxBot=0, sideHits=0, sideMisses=0;
    bool oldProfile=gProfile;
    gProfile=true;
    gCache.resetStats();
    auto getSide=[&](const uint16_t* ms)->const std::vector<std::pair<PartKey,uint64_t>>&{
        Key sk=makeSideKey(ms);
        auto it=sideMemo.find(sk);
        if(it!=sideMemo.end()){ ++sideHits; return it->second; }
        ++sideMisses;
        std::vector<std::pair<PartKey,uint64_t>> flat=buildSideHistFromKey(sk);
        return sideMemo.emplace(std::move(sk),std::move(flat)).first->second;
    };
    for(long long ei=0; ei<limit; ++ei){
        const Key& tk=ess[(size_t)ei].first;
        uint16_t topMS[8], botMS[8]; int nt=0, nb=0;
        for(int i=0;i<M;++i){ if(tk[i]&TAG) topMS[nt++]=(uint16_t)(tk[i]&VALMASK); else botMS[nb++]=tk[i]; }
        isort(topMS,C); isort(botMS,C);
        const auto& hTop=getSide(topMS);
        const auto& hBot=getSide(botMS);
        maxTop=std::max(maxTop,(unsigned long long)hTop.size());
        maxBot=std::max(maxBot,(unsigned long long)hBot.size());
        wtSum.clear();
        for(const auto& tp:hTop){ const uint16_t* a=tp.first.data(); uint64_t wt=tp.second;
            globalTopParts.emplace(tp.first,0);
            for(const auto& bp:hBot){ const uint16_t* b=bp.first.data();
                globalBotParts.emplace(bp.first,0);
                globalPartPairs.emplace(partPairHash64(tp.first,bp.first),0);
                Key raw{}; int i=0,j=0,k=0;
                while(i<C&&j<C){ if(a[i]<=b[j]) raw[k++]=a[i++]; else raw[k++]=b[j++]; }
                while(i<C) raw[k++]=a[i++];
                while(j<C) raw[k++]=b[j++];
                wtSum[gCache.get(raw)] += wt*bp.second;
            }
        }
        unsigned long long cr=(unsigned long long)hTop.size()*(unsigned long long)hBot.size();
        crossPairs+=cr;
        targetRefs+=(unsigned long long)wtSum.size();
        maxTargets=std::max(maxTargets,(unsigned long long)wtSum.size());
        for(const auto& kv:wtSum) globalTargets.emplace(kv.first,0);
    }
    CacheStatsSnapshot cs=gCache.stats();
    gProfile=oldProfile;
    size_t orbitSample=0, partOrbits=0;
    if(gPartOrbitAudit){
        std::vector<PartKey> parts;
        parts.reserve(globalTopParts.size()+globalBotParts.size());
        for(const auto& kv:globalTopParts) parts.push_back(kv.first);
        for(const auto& kv:globalBotParts) parts.push_back(kv.first);
        long long limitParts=(gPartOrbitMax>0 && gPartOrbitMax<(long long)parts.size())?gPartOrbitMax:(long long)parts.size();
        std::unordered_map<PartKey,int,PartKeyHash> orbs;
        for(long long i=0;i<limitParts;++i) orbs.emplace(canonPartBrute(parts[(size_t)i]),0);
        orbitSample=(size_t)limitParts;
        partOrbits=orbs.size();
    }
    double den=limit?double(limit):1.0;
    double cacheDen=(cs.hits+cs.misses)?double(cs.hits+cs.misses):1.0;
    std::fprintf(stderr,
        "  targetaudit: inspected=%lld/%zu sideMemo=%zu sideHit=%llu sideMiss=%llu\n"
        "    cross=%llu avgCross=%.2f maxTop=%llu maxBot=%llu\n"
        "    partTop=%zu partBot=%zu partPairHash=%zu partPairCompression=%.2f\n"
        "    globalTargets=%zu avgTargetsPerTask=%.2f maxTargetsPerTask=%llu targetCompression=%.2f\n"
        "    rawCache hit=%llu miss=%llu hitRate=%.2f%% insert=%llu probeFail=%llu\n",
        limit,ess.size(),sideMemo.size(),sideHits,sideMisses,
        crossPairs,crossPairs/den,maxTop,maxBot,
        globalTopParts.size(),globalBotParts.size(),globalPartPairs.size(),crossPairs/(double)std::max<size_t>(globalPartPairs.size(),1),
        globalTargets.size(),targetRefs/den,maxTargets,crossPairs/(double)std::max<size_t>(globalTargets.size(),1),
        cs.hits,cs.misses,100.0*cs.hits/cacheDen,cs.inserts,cs.probeFail);
    if(gPartOrbitAudit){
        std::fprintf(stderr,
            "    partOrbit sample=%zu orbits=%zu compression=%.2f\n",
            orbitSample,partOrbits,orbitSample/(double)std::max<size_t>(partOrbits,1));
    }
    std::fflush(stderr);
}

// brute within-side histogram (reference for difftest)
static std::vector<std::array<int,8>> validBijections(const int* syms,const std::vector<int>& mask){
    std::vector<std::array<int,8>> outv;
    std::vector<int> perm(C); for(int i=0;i<C;++i)perm[i]=i;
    do{
        bool ok=true;
        for(int i=0;i<C;++i) if(mask[syms[i]] & (1<<perm[i])){ ok=false; break; }
        if(ok){ std::array<int,8> a{}; for(int i=0;i<C;++i)a[i]=perm[i]; outv.push_back(a); }
    }while(std::next_permutation(perm.begin(),perm.end()));
    return outv;
}
static void buildHistBrute(const int* syms,const std::vector<int>& xm,const std::vector<int>& ym,
                           std::map<PartKey,uint64_t>& out){
    auto AX=validBijections(syms,xm); auto AY=validBijections(syms,ym);
    for(auto&ax:AX) for(auto&ay:AY){
        PartKey part{};
        for(int i=0;i<C;++i){ int s=syms[i]; part[i]=(uint16_t)mkPair(xm[s]|(1<<ax[i]), ym[s]|(1<<ay[i])); }
        std::sort(part.begin(),part.begin()+C);
        out[part]++;
    }
}

// ---------- test modes ----------
static void relabelPairs(const uint16_t* in, const int* sx, const int* sy, bool sw, uint16_t* out){
    for(int i=0;i<M;++i){
        int xm=in[i]&FULLC, ym=(in[i]>>C)&FULLC, nx=0, ny=0;
        for(int b=xm;b;b&=b-1) nx |= 1<<sx[__builtin_ctz(b)];
        for(int b=ym;b;b&=b-1) ny |= 1<<sy[__builtin_ctz(b)];
        out[i]=sw ? (uint16_t)(ny|(nx<<C)) : (uint16_t)(nx|(ny<<C));
    }
}
static void randomPairs(std::mt19937& rng, uint16_t* out){
    for(int i=0;i<M;++i){
        int xm=(int)(rng()&FULLC), ym=(int)(rng()&FULLC);
        out[i]=(uint16_t)(xm|(ym<<C));
    }
}
static bool balancedPairs(std::mt19937& rng, int d, uint16_t* out){
    for(int side=0; side<2; ++side){
        int cap[8]; for(int c=0;c<C;++c)cap[c]=2*d;
        for(int i=0;i<M;++i){
            int mask=0, got=0, tries=0;
            while(got<d){
                int c=(int)(rng()%C);
                if(!(mask&(1<<c)) && cap[c]>0){ mask|=1<<c; cap[c]--; got++; }
                if(++tries>2000) return false;
            }
            if(side==0) out[i]=(uint16_t)mask; else out[i]|=(uint16_t)(mask<<C);
        }
    }
    return true;
}

static int canonSelfTest(int ntests, unsigned seed){
    std::mt19937 rng(seed);
    int invariantBad=0, tagBad=0;
    uint16_t pr[MAXM], pr2[MAXM];
    for(int t=0;t<2*ntests;++t){
        if(t&1){ int d=1+(int)(rng()%(C-1)); if(!balancedPairs(rng,d,pr)) continue; }
        else randomPairs(rng, pr);
        Key k0=canonMS2(pr);
        TaggedCanon tc0; tc0.init(pr);
        int A=0; { int got=0; while(got<C){ int i=(int)(rng()%M); if(!(A&(1<<i))){A|=1<<i;++got;} } }
        Key tk0=tc0.canon(A);
        for(int r=0;r<3;++r){
            int sx[8], sy[8];
            for(int i=0;i<C;++i){ sx[i]=i; sy[i]=i; }
            std::shuffle(sx, sx+C, rng);
            std::shuffle(sy, sy+C, rng);
            bool sw=(rng()&1)!=0;
            relabelPairs(pr, sx, sy, sw, pr2);
            if(canonMS2(pr2)!=k0){
                ++invariantBad;
                if(invariantBad<=5) std::fprintf(stderr,"CANON2 INVARIANCE MISMATCH t=%d\n",t);
                break;
            }
            // tagged invariance: tags follow the SYMBOLS (elements), which relabelPairs keeps in place
            TaggedCanon tc2; tc2.init(pr2);
            if(tc2.canon(A)!=tk0){
                ++tagBad;
                if(tagBad<=5) std::fprintf(stderr,"TAGGED INVARIANCE MISMATCH t=%d\n",t);
                break;
            }
        }
    }
    int separationConflicts=0; size_t qKeys=0, bKeys=0; bool didSep=false;
    if(C<=4){
        didSep=true;
        std::map<Key,Key> q2b; std::map<Key,int> qk, bk;
        for(int t=0;t<2*ntests;++t){
            if(t&1){ int d=1+(int)(rng()%(C-1)); if(!balancedPairs(rng,d,pr)) continue; }
            else randomPairs(rng, pr);
            Key kq=canonMS2(pr), kb=canonBrute2(pr);
            auto it=q2b.find(kq);
            if(it==q2b.end()) q2b.emplace(kq,kb);
            else if(!(it->second==kb)){ ++separationConflicts;
                if(separationConflicts<=5) std::fprintf(stderr,"CANON2 SEPARATION CONFLICT t=%d\n",t); }
            qk[kq]=1; bk[kb]=1;
        }
        qKeys=qk.size(); bKeys=bk.size();
    }
    bool ok = invariantBad==0 && tagBad==0 && (!didSep || (separationConflicts==0 && qKeys==bKeys));
    std::printf("canontest C=%d: invarianceTests=%d violations=%d taggedViolations=%d", C, 2*ntests, invariantBad, tagBad);
    if(didSep) std::printf("  separationKeys q=%zu brute=%zu conflicts=%d", qKeys, bKeys, separationConflicts);
    else std::printf("  separation=SKIP(C>4)");
    std::printf("  [%s]\n", ok?"OK":"FAIL");
    return ok?0:1;
}

static int difftest(int ntests, unsigned seed){
    std::mt19937 rng(seed);
    int mismatches=0, empties=0;
    std::vector<int> xm(M,0), ym(M,0);
    for(int t=0;t<ntests;++t){
        int syms[8]; for(int i=0;i<C;++i)syms[i]=i;
        for(;;){
            for(int i=0;i<C;++i){ xm[i]=rng()&FULLC; ym[i]=rng()&FULLC; }
            if(!validBijections(syms,xm).empty() && !validBijections(syms,ym).empty()) break;
        }
        std::map<PartKey,uint64_t> hb, hf;
        buildHistBrute(syms,xm,ym,hb);
        buildHistFast(syms,xm,ym,hf);
        if(hb!=hf){
            mismatches++;
            if(mismatches<=5) std::fprintf(stderr,"HIST MISMATCH C=%d t=%d\n",C,t);
        }
        if(hb.empty()) empties++;
    }
    std::printf("difftest C=%d: tests=%d mismatches=%d empties=%d  [%s]\n",
                C,ntests,mismatches,empties, mismatches==0?"OK":"FAIL");
    return mismatches==0?0:1;
}

static uint64_t histSum(const std::map<PartKey,uint64_t>& h){
    uint64_t s=0; for(const auto& kv:h) s+=kv.second; return s;
}

static int permDptest(int ntests, unsigned seed){
    std::mt19937 rng(seed);
    int mismatches=0, sumBad=0, empties=0;
    std::vector<int> xm(M,0), ym(M,0);
    for(int t=0;t<ntests;++t){
        int syms[8]; for(int i=0;i<C;++i)syms[i]=i;
        std::vector<std::array<int,8>> ax, ay;
        for(;;){
            for(int i=0;i<C;++i){ xm[i]=rng()&FULLC; ym[i]=rng()&FULLC; }
            ax=validBijections(syms,xm);
            ay=validBijections(syms,ym);
            if(!ax.empty() && !ay.empty()) break;
        }
        std::map<PartKey,uint64_t> hb,hf,hp;
        buildHistBrute(syms,xm,ym,hb);
        buildHistFast(syms,xm,ym,hf);
        buildHistPermDP(syms,xm,ym,hp);
        if(hb!=hf || hb!=hp){
            ++mismatches;
            if(mismatches<=5)
                std::fprintf(stderr,"PERMDP HIST MISMATCH C=%d t=%d brute=%zu fast=%zu perm=%zu\n",
                             C,t,hb.size(),hf.size(),hp.size());
        }
        uint64_t expect=(uint64_t)ax.size()*(uint64_t)ay.size();
        if(histSum(hp)!=expect){
            ++sumBad;
            if(sumBad<=5)
                std::fprintf(stderr,"PERMDP SUM BAD C=%d t=%d got=%llu expect=%llu\n",
                             C,t,(unsigned long long)histSum(hp),(unsigned long long)expect);
        }
        if(hp.empty()) ++empties;
    }
    bool ok=mismatches==0 && sumBad==0;
    std::printf("permdptest C=%d: tests=%d mismatches=%d sumBad=%d empties=%d  [%s]\n",
                C,ntests,mismatches,sumBad,empties,ok?"OK":"FAIL");
    return ok?0:1;
}

static int histBench(int ntests, unsigned seed){
    std::mt19937 rng(seed);
    std::vector<int> xm(M,0), ym(M,0);
    std::vector<std::vector<int>> xs, ys;
    xs.reserve(ntests); ys.reserve(ntests);
    for(int t=0;t<ntests;++t){
        int syms[8]; for(int i=0;i<C;++i)syms[i]=i;
        for(;;){
            for(int i=0;i<C;++i){ xm[i]=rng()&FULLC; ym[i]=rng()&FULLC; }
            if(!validBijections(syms,xm).empty() && !validBijections(syms,ym).empty()) break;
        }
        xs.push_back(xm); ys.push_back(ym);
    }
    int syms[8]; for(int i=0;i<C;++i)syms[i]=i;
    auto now=[]{ return std::chrono::steady_clock::now(); };
    size_t fastKeys=0, permKeys=0; uint64_t fastMass=0, permMass=0;
    auto t1=now();
    for(int t=0;t<ntests;++t){
        std::map<PartKey,uint64_t> h;
        buildHistFast(syms,xs[t],ys[t],h);
        fastKeys+=h.size(); fastMass+=histSum(h);
    }
    auto t2=now();
    for(int t=0;t<ntests;++t){
        std::map<PartKey,uint64_t> h;
        buildHistPermDP(syms,xs[t],ys[t],h);
        permKeys+=h.size(); permMass+=histSum(h);
    }
    auto t3=now();
    double fastSec=std::chrono::duration<double>(t2-t1).count();
    double permSec=std::chrono::duration<double>(t3-t2).count();
    std::printf("histbench C=%d tests=%d fast=%.6fs permDP=%.6fs ratio=%.2f avgKeys fast=%.2f perm=%.2f mass %llu/%llu\n",
                C,ntests,fastSec,permSec,permSec/(fastSec>0?fastSec:1e-9),
                fastKeys/(double)ntests,permKeys/(double)ntests,
                (unsigned long long)fastMass,(unsigned long long)permMass);
    return 0;
}

// ---------- checkpoint and dual combine ----------
static bool isPositiveIntArg(const std::string& s){
    if(s.empty()) return false;
    for(char c:s) if(c<'0'||c>'9') return false;
    return true;
}

static std::string checkpointPath(const std::string& prefix, int c, int band){
    char buf[256];
    const char* pre = prefix.empty() ? "data/mp_q" : prefix.c_str();
    std::snprintf(buf,sizeof(buf),"%s_C%d_band%d.chk",pre,c,band);
    return std::string(buf);
}

static bool saveCheckpoint(const std::string& path, int band, const std::map<Key,Big>& states){
    std::string tmp = path + ".tmp";
    std::ofstream out(tmp, std::ios::out | std::ios::trunc);
    if(!out) return false;
    out << "MPQCHK1\n";
    out << "C " << C << "\n";
    out << "M " << M << "\n";
    out << "band " << band << "\n";
    out << "states " << states.size() << "\n";
    for(const auto& kv:states){
        for(int i=0;i<MAXM;++i) out << kv.first[i] << ' ';
        out << kv.second.str() << "\n";
    }
    out.close();
    if(!out) return false;
    std::remove(path.c_str());
    return std::rename(tmp.c_str(), path.c_str())==0;
}

static bool loadCheckpoint(const std::string& path, int& band, std::map<Key,Big>& states){
    std::ifstream in(path);
    if(!in) return false;
    std::string magic, tok;
    int c=0,m=0; size_t n=0;
    in >> magic;
    if(magic!="MPQCHK1") return false;
    in >> tok >> c;
    if(tok!="C" || c!=C) return false;
    in >> tok >> m;
    if(tok!="M" || m!=M) return false;
    in >> tok >> band;
    if(tok!="band") return false;
    in >> tok >> n;
    if(tok!="states") return false;
    std::map<Key,Big> loaded;
    for(size_t row=0; row<n; ++row){
        Key k{};
        for(int i=0;i<MAXM;++i){
            unsigned int v=0; in >> v; k[i]=(uint16_t)v;
        }
        std::string dec; in >> dec;
        if(!in) return false;
        loaded.emplace(k, Big::fromDec(dec));
    }
    states=std::move(loaded);
    return true;
}

static bool loadLatestCheckpoint(const std::string& prefix, int maxBand, int& band, std::map<Key,Big>& states){
    for(int b=maxBand; b>=0; --b){
        std::string p=checkpointPath(prefix,C,b);
        if(loadCheckpoint(p,band,states)){
            std::fprintf(stderr,"loaded checkpoint %s (%zu states)\n",p.c_str(),states.size());
            return true;
        }
    }
    return false;
}

static bool dualCombine(const std::map<Key,Big>& hi, const std::map<Key,Big>& lo, Big& N){
    long long matched=0, missing=0, nonExact=0;
    N=Big();
    for(const auto& kv:hi){
        Key ck=complementKey(kv.first);
        auto it=lo.find(ck);
        if(it==lo.end()){ ++missing; continue; }
        Big prod=mulBig(kv.second,it->second);
        uint64_t q=concreteStateCount(kv.first);
        if(!divExactU64(prod,q)){
            ++nonExact;
            if(nonExact<=5) std::fprintf(stderr,"dual combine non-exact division q=%llu\n",(unsigned long long)q);
        }
        N += prod;
        ++matched;
    }
    std::fprintf(stderr,"dual combine: hi=%zu lo=%zu matched=%lld missing=%lld nonExact=%lld\n",
                 hi.size(),lo.size(),matched,missing,nonExact);
    return missing==0 && nonExact==0;
}

struct Rat {
    Big n;
    uint64_t d=1;
};

static bool sameRat(const Rat& a, const Rat& b){
    return mulBig(a.n,Big(b.d)) == mulBig(b.n,Big(a.d));
}

static std::string invMultiplicity(const Key& k){
    std::string s;
    int i=0;
    while(i<M){
        int j=i+1; while(j<M && k[j]==k[i]) ++j;
        s += std::to_string(j-i);
        s += ',';
        i=j;
    }
    return s;
}

static std::string invTUV(const Key& k, bool includeUV){
    uint8_t T[8][8]={}, U[8][8]={}, V[8][8]={};
    for(int i=0;i<M;++i){
        int xm=k[i]&FULLC, ym=(k[i]>>C)&FULLC;
        for(int bx=xm; bx; bx&=bx-1){
            int a=__builtin_ctz(bx);
            for(int by=ym; by; by&=by-1) T[a][__builtin_ctz(by)]++;
            for(int b2=bx&(bx-1); b2; b2&=b2-1){ int a2=__builtin_ctz(b2); U[a][a2]++; U[a2][a]++; }
        }
        for(int by=ym; by; by&=by-1){
            int b=__builtin_ctz(by);
            for(int b2=by&(by-1); b2; b2&=b2-1){ int bb=__builtin_ctz(b2); V[b][bb]++; V[bb][b]++; }
        }
    }
    std::string s;
    s.reserve(includeUV?3*C*C:C*C);
    for(int a=0;a<C;++a) for(int b=0;b<C;++b) s.push_back((char)T[a][b]);
    if(includeUV){
        for(int a=0;a<C;++a) for(int b=0;b<C;++b) s.push_back((char)U[a][b]);
        for(int a=0;a<C;++a) for(int b=0;b<C;++b) s.push_back((char)V[a][b]);
    }
    return s;
}

static std::string invAutQ(const Key& k){
    return std::to_string(autCount(k.data())) + "/" + std::to_string(concreteStateCount(k));
}

static void phiAuditOne(const char* name, const std::map<Key,Rat>& phi,
                        const std::function<std::string(const Key&)>& inv){
    struct Group { std::vector<Rat> vals; int count=0; };
    std::unordered_map<std::string,Group> groups;
    for(const auto& kv:phi){
        std::string key=inv(kv.first);
        Group& g=groups[key];
        ++g.count;
        bool seen=false;
        for(const Rat& r:g.vals) if(sameRat(r,kv.second)){ seen=true; break; }
        if(!seen) g.vals.push_back(kv.second);
    }
    int collisionGroups=0, maxGroup=0, maxVals=0;
    long long collisionStates=0;
    for(const auto& kv:groups){
        maxGroup=std::max(maxGroup,kv.second.count);
        maxVals=std::max(maxVals,(int)kv.second.vals.size());
        if(kv.second.vals.size()>1){
            ++collisionGroups;
            collisionStates+=kv.second.count;
        }
    }
    std::fprintf(stderr,
        "  phiaudit %-12s groups=%zu collisionGroups=%d collisionStates=%lld maxGroup=%d maxPhiVals=%d\n",
        name,groups.size(),collisionGroups,collisionStates,maxGroup,maxVals);
}

static void phiAudit(const std::map<Key,Big>& low){
    std::map<Key,Rat> phi;
    for(const auto& kv:low){
        Key target=complementKey(kv.first);
        phi[target]=Rat{kv.second,concreteStateCount(target)};
    }
    std::fprintf(stderr,"phiaudit: low=%zu targetPhi=%zu\n",low.size(),phi.size());
    phiAuditOne("multiplicity",phi,[](const Key& k){ return invMultiplicity(k); });
    phiAuditOne("T",phi,[](const Key& k){ return invTUV(k,false); });
    phiAuditOne("TUV",phi,[](const Key& k){ return invTUV(k,true); });
    phiAuditOne("aut/Q",phi,[](const Key& k){ return invAutQ(k); });
    std::fflush(stderr);
}

// ---------- main DP ----------
int main(int argc,char**argv){
    C=(argc>1)?atoi(argv[1]):3; M=2*C; FULLC=(1<<C)-1;
    std::string mode = (argc>2)?argv[2]:"";
    initPerms();

    if(mode=="difftest"){
        int n=(argc>3)?atoi(argv[3]):2000;
        unsigned seed=(argc>4)?(unsigned)atoi(argv[4]):12345u;
        return difftest(n,seed);
    }
    if(mode=="permdptest"){
        int n=(argc>3)?atoi(argv[3]):2000;
        unsigned seed=(argc>4)?(unsigned)atoi(argv[4]):24680u;
        return permDptest(n,seed);
    }
    if(mode=="histbench"){
        int n=(argc>3)?atoi(argv[3]):20000;
        unsigned seed=(argc>4)?(unsigned)atoi(argv[4]):13579u;
        return histBench(n,seed);
    }
    if(mode=="canontest"){
        int n=(argc>3)?atoi(argv[3]):20000;
        unsigned seed=(argc>4)?(unsigned)atoi(argv[4]):777u;
        return canonSelfTest(n,seed);
    }

    bool dump=false, dual=false, resume=false, checkpoint=true, profile=false, sideGlobal=false, phiAuditMode=false;
    std::string checkpointPrefix;
    size_t sideGlobalCap=1000000;
    int auditBand=-1;
    long long auditMax=0;
    int targetAuditBand=-1;
    long long targetAuditMax=0;
    int cacheLog2=0;
    for(int ai=2; ai<argc; ++ai){
        std::string a=argv[ai];
        if(a=="dump") dump=true;
        else if(a=="dual") dual=true;
        else if(a=="resume") resume=true;
        else if(a=="nocheckpoint") checkpoint=false;
        else if(a=="profile") profile=true;
        else if(a=="phiaudit") phiAuditMode=true;
        else if(a=="sideglobal") sideGlobal=true;
        else if(a.rfind("sideglobal=",0)==0){ sideGlobal=true; sideGlobalCap=(size_t)std::strtoull(a.c_str()+11,nullptr,10); }
        else if(a.rfind("auditband=",0)==0) auditBand=atoi(a.c_str()+10);
        else if(a.rfind("auditmax=",0)==0) auditMax=(long long)std::strtoll(a.c_str()+9,nullptr,10);
        else if(a.rfind("targetauditband=",0)==0) targetAuditBand=atoi(a.c_str()+16);
        else if(a.rfind("targetauditmax=",0)==0) targetAuditMax=(long long)std::strtoll(a.c_str()+15,nullptr,10);
        else if(a=="partorbit") gPartOrbitAudit=true;
        else if(a.rfind("partorbitmax=",0)==0){ gPartOrbitAudit=true; gPartOrbitMax=(long long)std::strtoll(a.c_str()+13,nullptr,10); }
        else if(a.rfind("ckpt=",0)==0) checkpointPrefix=a.substr(5);
        else if(isPositiveIntArg(a)) cacheLog2=atoi(a.c_str());
        else std::fprintf(stderr,"warning: ignoring unknown argument '%s'\n",a.c_str());
    }
    const int H=(C+1)/2;      // dual: forward bands 0..H-1, pair S_H against S_{C-H}
    const int L=C-H;
    const int targetBands=phiAuditMode?L:(dual?H:C);
    gProfile=profile;
    if(cacheLog2<10) cacheLog2 = (C>=5)?28:20;
    gCache.init(cacheLog2);
    std::fprintf(stderr,"raw-canon cache: 2^%d slots (%.1f GB)\n",cacheLog2,(double)(1ull<<cacheLog2)*56.0/1e9);
    std::fprintf(stderr,"mode: %s%s%s%s%s\n",
                 dual?"dual ":"", dump?"dump ":"", resume?"resume ":"",
                 checkpoint?"checkpoint ":"", profile?"profile ":"");
    if(phiAuditMode) std::fprintf(stderr,"phiaudit: target shallow bands=%d\n",targetBands);
    if(sideGlobal) std::fprintf(stderr,"sideglobal: cap=%zu unique side keys\n",sideGlobalCap);
    if(auditBand>=0) std::fprintf(stderr,"audit: band=%d max=%lld\n",auditBand,auditMax);
    if(targetAuditBand>=0) std::fprintf(stderr,"targetaudit: band=%d max=%lld\n",targetAuditBand,targetAuditMax);
    if(gPartOrbitAudit) std::fprintf(stderr,"partorbit audit: max=%lld\n",gPartOrbitMax);
    for(int m=0;m<(1<<M);++m) if(__builtin_popcount(m)==C) Askel.push_back(m);
    auto t0=std::chrono::steady_clock::now();
    auto el=[&]{ return std::chrono::duration<double>(std::chrono::steady_clock::now()-t0).count(); };

    std::map<Key,Big> states;
    { std::vector<uint16_t> empty(M,(uint16_t)0); states[canonMS2(empty.data())]=Big((uint64_t)1); }
    std::map<Key,Big> snap;   // dual, odd C: snapshot of S_{C-H} (after band C-H-1)
    int startBand=0;
    if(resume){
        int loadedBand=-1;
        if(loadLatestCheckpoint(checkpointPrefix,targetBands-1,loadedBand,states)){
            startBand=loadedBand+1;
        } else {
            std::fprintf(stderr,"resume requested, but no usable checkpoint found for C=%d up to band %d\n",C,targetBands-1);
        }
    }
    if(dual && L>0 && startBand>L){
        int snapBand=-1;
        if(!loadCheckpoint(checkpointPath(checkpointPrefix,C,L-1),snapBand,snap)){
            std::fprintf(stderr,"dual resume needs checkpoint for shallow midpoint band %d\n",L-1);
            return 2;
        }
        std::fprintf(stderr,"loaded dual shallow snapshot band %d (%zu states)\n",snapBand,snap.size());
    }

    for(int band=startBand; band<targetBands; ++band){
        // ---- PHASE 1 (parallel over states): skeleton-tagged quotient ----
        // essential class = canonical tagged multiset; coeff = sum of w_src over all (state,A)
        // mapping to it.  Different states cannot share a tagged key (stripping tags recovers
        // the state's orbit), so per-thread maps merge disjointly.
        std::vector<const Key*> skeys; skeys.reserve(states.size());
        std::vector<const Big*> sw;    sw.reserve(states.size());
        for(auto&kv:states){ skeys.push_back(&kv.first); sw.push_back(&kv.second); }
        const long long NS=(long long)skeys.size();
        std::unordered_map<Key,Big,KeyHash> essential;
        {
            int nth=0;
            std::vector<std::unordered_map<Key,Big,KeyHash>> tess;
            #pragma omp parallel
            {
                #pragma omp single
                { nth=omp_get_num_threads(); tess.resize(nth); }
                #pragma omp barrier
                auto& mine=tess[omp_get_thread_num()];
                #pragma omp for schedule(dynamic,16)
                for(long long si=0; si<NS; ++si){
                    TaggedCanon tc; tc.init(skeys[si]->data());
                    const Big& w=*sw[si];
                    for(int A:Askel) mine[tc.canon(A)]+=w;
                }
            }
            size_t tot=0; for(auto&m:tess)tot+=m.size();
            essential.reserve(tot);
            for(auto&m:tess){ for(auto&kv:m) essential[kv.first]+=kv.second; m.clear(); }
        }
        std::vector<std::pair<Key,const Big*>> ess; ess.reserve(essential.size());
        for(auto&kv:essential) ess.push_back({kv.first,&kv.second});
        const long long NE=(long long)ess.size();
        std::fprintf(stderr,"  band %d: states=%lld essential=%lld (raw state*skel=%lld)  el=%.1fs\n",
                     band,NS,NE,NS*(long long)Askel.size(),el()); std::fflush(stderr);
        if(auditBand==band){
            auditEssentialBand(ess,auditMax);
            std::printf("C=%d audit band %d complete (no final N computed)\n",C,band);
            return 0;
        }
        if(targetAuditBand==band){
            targetAuditEssentialBand(ess,targetAuditMax);
            std::printf("C=%d targetaudit band %d complete (no final N computed)\n",C,band);
            return 0;
        }

        std::vector<Key> globalSideKeys;
        std::vector<std::vector<std::pair<PartKey,uint64_t>>> globalSideVals;
        bool useGlobalSide=false;
        if(sideGlobal){
            globalSideKeys.reserve((size_t)NE*2);
            for(const auto& ep:ess){
                const Key& tk=ep.first;
                uint16_t topMS[8], botMS[8]; int nt=0, nb=0;
                for(int i=0;i<M;++i){ if(tk[i]&TAG) topMS[nt++]=(uint16_t)(tk[i]&VALMASK); else botMS[nb++]=tk[i]; }
                isort(topMS,C); isort(botMS,C);
                globalSideKeys.push_back(makeSideKey(topMS));
                globalSideKeys.push_back(makeSideKey(botMS));
            }
            std::sort(globalSideKeys.begin(),globalSideKeys.end());
            globalSideKeys.erase(std::unique(globalSideKeys.begin(),globalSideKeys.end()),globalSideKeys.end());
            if(sideGlobalCap==0 || globalSideKeys.size()<=sideGlobalCap){
                useGlobalSide=true;
                globalSideVals.resize(globalSideKeys.size());
                const long long NG=(long long)globalSideKeys.size();
                #pragma omp parallel for schedule(dynamic,64)
                for(long long gi=0; gi<NG; ++gi)
                    globalSideVals[(size_t)gi]=buildSideHistFromKey(globalSideKeys[(size_t)gi]);
                std::fprintf(stderr,"  sideglobal band %d: precomputed %zu side histograms  el=%.1fs\n",
                             band,globalSideKeys.size(),el());
            } else {
                std::fprintf(stderr,"  sideglobal band %d: %zu unique side keys exceeds cap %zu; using thread-local memo\n",
                             band,globalSideKeys.size(),sideGlobalCap);
                globalSideKeys.clear();
            }
            std::fflush(stderr);
        }

        // ---- PHASE 2 (parallel over essential classes): transitions ----
        std::vector<std::map<Key,Big>> tnx;
        std::vector<PhaseStats> pstats;
        std::atomic<long long> done{0};
        if(profile) gCache.resetStats();
        #pragma omp parallel
        {
            int nth=omp_get_num_threads(), tid=omp_get_thread_num();
            #pragma omp single
            { tnx.resize(nth); pstats.resize(nth); }
            #pragma omp barrier
            std::map<Key,Big>& mynx=tnx[tid];
            PhaseStats& pst=pstats[tid];
            // per-thread side-histogram memo (bounded)
            std::unordered_map<Key,std::vector<std::pair<PartKey,uint64_t>>,KeyHash> sideMemo;
            const size_t SIDECAP=200000;
            // NOTE: no clear() in here!  The 2026-07-03 09:53 APPCRASH (0xc0000005, 37% into
            // band-2) was a use-after-free: the cap-clear ran inside getSide, so fetching
            // hBot could destroy the just-returned hTop reference.  The cap check now runs
            // once at task start, BEFORE any reference is taken; within a task the memo only
            // grows (node-based => references stable under rehash).
            auto getSide=[&](const uint16_t* ms)->const std::vector<std::pair<PartKey,uint64_t>>&{
                Key sk=makeSideKey(ms);
                if(useGlobalSide){
                    auto it=std::lower_bound(globalSideKeys.begin(),globalSideKeys.end(),sk);
                    if(it!=globalSideKeys.end() && *it==sk){
                        if(profile) ++pst.sideHits;
                        return globalSideVals[(size_t)(it-globalSideKeys.begin())];
                    }
                    if(profile) ++pst.sideMisses;
                    static const std::vector<std::pair<PartKey,uint64_t>> emptySide;
                    return emptySide;
                }
                auto it=sideMemo.find(sk);
                if(it!=sideMemo.end()){ if(profile) ++pst.sideHits; return it->second; }
                if(profile) ++pst.sideMisses;
                std::vector<std::pair<PartKey,uint64_t>> flat=buildSideHistFromKey(sk);
                return sideMemo.emplace(std::move(sk),std::move(flat)).first->second;
            };
            // per-task target aggregation in uint64 first (task mass <= (C!)^4 ~ 2e8, safe),
            // so Big arithmetic runs once per (task, target) instead of once per raw.
            std::unordered_map<Key,uint64_t,KeyHash> wtSum;   // small (#targets/task), reuse OK
            #pragma omp for schedule(dynamic,16)
            for(long long ei=0; ei<NE; ++ei){
                if(sideMemo.size()>=SIDECAP){ sideMemo.clear(); if(profile) ++pst.sideClears; }   // safe: no live references here
                if(profile) ++pst.tasks;
                const Key& tk=ess[ei].first;
                const Big& coeff=*ess[ei].second;
                // split by tag; strip tags
                uint16_t topMS[8], botMS[8]; int nt=0, nb=0;
                for(int i=0;i<M;++i){ if(tk[i]&TAG) topMS[nt++]=(uint16_t)(tk[i]&VALMASK); else botMS[nb++]=tk[i]; }
                isort(topMS,C); isort(botMS,C);
                const auto& hTop=getSide(topMS); if(hTop.empty()){ if(profile) ++pst.emptyTop; ++done; continue; }
                const auto& hBot=getSide(botMS); if(hBot.empty()){ if(profile) ++pst.emptyBot; ++done; continue; }
                if(profile){
                    unsigned long long ht=(unsigned long long)hTop.size(), hb=(unsigned long long)hBot.size();
                    unsigned long long cr=ht*hb;
                    pst.topEntries+=ht; pst.botEntries+=hb; pst.crossPairs+=cr;
                    pst.maxTop=std::max(pst.maxTop,ht); pst.maxBot=std::max(pst.maxBot,hb); pst.maxCross=std::max(pst.maxCross,cr);
                }
                wtSum.clear();
                for(const auto&tp:hTop){ const uint16_t* a=tp.first.data(); uint64_t wt=tp.second;
                    for(const auto&bp:hBot){ const uint16_t* b=bp.first.data();
                        Key raw{}; int i=0,j=0,k=0;
                        while(i<C&&j<C){ if(a[i]<=b[j]) raw[k++]=a[i++]; else raw[k++]=b[j++]; }
                        while(i<C) raw[k++]=a[i++];
                        while(j<C) raw[k++]=b[j++];
                        wtSum[gCache.get(raw)] += wt*bp.second;
                    }
                }
                if(profile){
                    unsigned long long ts=(unsigned long long)wtSum.size();
                    pst.targets+=ts; pst.maxTargets=std::max(pst.maxTargets,ts);
                }
                for(auto& lp:wtSum) mynx[lp.first].addMul(coeff, lp.second);
                long long d=++done;
                if((d & 0x3FFFF)==0){ std::fprintf(stderr,"    ess %lld/%lld  el=%.1fs\n",d,NE,el()); std::fflush(stderr); }
            }
        }
        if(profile){
            PhaseStats ps; for(const auto& s:pstats) ps.add(s);
            CacheStatsSnapshot cs=gCache.stats();
            double taskDen=ps.tasks?double(ps.tasks):1.0;
            double cacheDen=(cs.hits+cs.misses)?double(cs.hits+cs.misses):1.0;
            std::fprintf(stderr,
                "  profile band %d: tasks=%llu emptyTop=%llu emptyBot=%llu sideHit=%llu sideMiss=%llu sideClear=%llu\n"
                "    hist avgTop=%.2f avgBot=%.2f maxTop=%llu maxBot=%llu cross=%llu avgCross=%.2f maxCross=%llu targets=%llu avgTargets=%.2f maxTargets=%llu\n"
                "    rawCache hit=%llu miss=%llu hitRate=%.2f%% insert=%llu probeFail=%llu\n",
                band,ps.tasks,ps.emptyTop,ps.emptyBot,ps.sideHits,ps.sideMisses,ps.sideClears,
                ps.topEntries/taskDen,ps.botEntries/taskDen,ps.maxTop,ps.maxBot,ps.crossPairs,ps.crossPairs/taskDen,ps.maxCross,
                ps.targets,ps.targets/taskDen,ps.maxTargets,
                cs.hits,cs.misses,100.0*cs.hits/cacheDen,cs.inserts,cs.probeFail);
            std::fflush(stderr);
        }
        std::map<Key,Big> nx;
        for(auto&tm:tnx) for(auto&kv:tm) nx[kv.first]+=kv.second;
        states=std::move(nx);
        std::fprintf(stderr,"band %d: states=%zu elapsed=%.1fs\n",band,states.size(),el()); std::fflush(stderr);
        if(dump) for(auto&kv:states) std::fprintf(stdout,"  band%d -> %s\n",band,kv.second.str().c_str());
        if(dual && band==L-1) snap=states;
        if(checkpoint){
            std::string p=checkpointPath(checkpointPrefix,C,band);
            if(saveCheckpoint(p,band,states))
                std::fprintf(stderr,"checkpoint saved: %s\n",p.c_str());
            else
                std::fprintf(stderr,"checkpoint save FAILED: %s\n",p.c_str());
            std::fflush(stderr);
        }
    }
    if(phiAuditMode){
        phiAudit(states);
        std::printf("C=%d phiaudit complete (no final N computed)\n",C);
        return 0;
    }
    Big N;
    bool okCombine=true;
    if(dual){
        const std::map<Key,Big>* lo=&snap;
        if(L==H) lo=&states;
        if(lo->empty()){
            std::fprintf(stderr,"dual combine has no shallow midpoint states\n");
            return 2;
        }
        okCombine=dualCombine(states,*lo,N);
    } else {
        std::vector<uint16_t> full(M,(uint16_t)mkPair(FULLC,FULLC));
        Key fk=canonMS2(full.data());
        N = states.count(fk)?states[fk]:Big();
    }
    std::printf("C=%d%s: N=%s\n",C,dual?" dual":"",N.str().c_str());
    const char*e=(C==2)?"288":(C==3)?"28200960":(C==4)?"29136487207403520":(C==5)?"1903816047972624930994913280000":nullptr;
    if(e)std::printf("  expect %s [%s]\n",e, N.str()==std::string(e)?"OK":"BAD");
    return okCombine?0:2;
}
