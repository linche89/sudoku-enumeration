// multiset2xC.cpp — N(2xC) via the CORRECT sufficient statistic (alloc-light).
//
// WHY: final2xC.cpp / quotient2xC.cpp used the cross-stack intersection matrix
// T[a][b]=|X_a cap Y_b| as the joint state.  T only *counts* how many symbols
// co-occur in each (X-col,Y-col) pair; it FORGETS which symbols.  At C=3 that
// happened to coincide with the true orbit invariant, but at C=4 T over-merges
// distinct joint orbits -> over-counts (gave 29971601054760960 vs true
// 29136487207403520, confirmed by independent oracle sudoku_rc).
//
// CORRECT STATE: the symbol-anonymous multiset of per-symbol (xmask,ymask) pairs
//   [xmask = set of X-columns containing the symbol; ymask = set of Y-columns],
//   canonicalised under S_C (perm X-cols) x S_C (perm Y-cols).  This is EXACTLY
//   the joint orbit under S_{2C} x S_C x S_C (the provably-correct jointcanon
//   oracle) but in (C!)^2 work instead of (2C)!.
//
// TRANSITION (still convolution): next multiset = topPart  UNION  botPart, disjoint
//   (each symbol is in skeleton A "top" or ~A "bot"; its new masks depend only on
//   its own old masks + its own assigned X-col & Y-col).  Per source rep & A:
//   histogram top over (X-bij x Y-bij), histogram bot likewise, convolve, canon.
//   Identical weight to brute explicit-placement, binned by multiset.
//
// Verify: 288 (C=2), 28200960 (C=3), 29136487207403520 (C=4),
//   1903816047972624930994913280000 (C=5); C=6 is the new value.
#include "big.hpp"
#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <array>
#include <vector>
#include <map>
#include <algorithm>
#include <string>
#include <chrono>

static int C, M, FULLC;                       // M=2C symbols, FULLC=(1<<C)-1
static std::vector<int> Askel;                // all C-subsets of [2C]
static std::vector<std::array<int,8>> CP;     // perms of {0..C-1}
static std::vector<std::vector<int>> permMask;// permMask[p][mask] = mask with cols permuted by CP[p]

static const int MAXM=12;
using Key = std::array<uint16_t,MAXM>;         // state key: M sorted pairs, zero-padded
using PartKey = std::array<uint16_t,6>;        // one side's C pairs, sorted

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

// Canonicalise a multiset of M pairs under S_C x S_C.  Optionally return a
// representative per-symbol pair list (canonical sorted order).
static Key canonMS(const std::vector<int>& pairs, std::vector<int>* repOut=nullptr){
    Key best{}; bool have=false; uint16_t bestArr[MAXM];
    uint16_t cand[MAXM];
    for(size_t rp=0; rp<CP.size(); ++rp){
        const int* pmX = permMask[rp].data();
        for(size_t cp=0; cp<CP.size(); ++cp){
            const int* pmY = permMask[cp].data();
            for(int i=0;i<M;++i) cand[i] = (uint16_t)mkPair(pmX[pX(pairs[i])], pmY[pY(pairs[i])]);
            std::sort(cand,cand+M);
            if(!have || std::lexicographical_compare(cand,cand+M,bestArr,bestArr+M)){
                std::copy(cand,cand+M,bestArr); have=true;
            }
        }
    }
    for(int i=0;i<M;++i) best[i]=bestArr[i];
    if(repOut){ repOut->assign(M,0); for(int i=0;i<M;++i)(*repOut)[i]=bestArr[i]; }
    return best;
}

// Valid column-bijections for `syms` (avoid each symbol's existing mask).
static std::vector<std::array<int,8>> validBijections(const int* syms,const std::vector<int>& mask){
    std::vector<std::array<int,8>> out;
    std::vector<int> perm(C); for(int i=0;i<C;++i)perm[i]=i;
    do{
        bool ok=true;
        for(int i=0;i<C;++i) if(mask[syms[i]] & (1<<perm[i])){ ok=false; break; }
        if(ok){ std::array<int,8> a{}; for(int i=0;i<C;++i)a[i]=perm[i]; out.push_back(a); }
    }while(std::next_permutation(perm.begin(),perm.end()));
    return out;
}

int main(int argc,char**argv){
    C=(argc>1)?atoi(argv[1]):3; M=2*C; FULLC=(1<<C)-1;
    bool dump=(argc>2&&std::string(argv[2])=="dump");
    for(int m=0;m<(1<<M);++m) if(__builtin_popcount(m)==C) Askel.push_back(m);
    initPerms();
    auto t0=std::chrono::steady_clock::now();

    std::map<Key,Big> states;
    std::map<Key,std::vector<int>> rep;
    { std::vector<int> empty(M, mkPair(0,0)); Key k=canonMS(empty); states[k]=Big((uint64_t)1); rep[k]=empty; }

    for(int band=0; band<C; ++band){
        std::map<Key,Big> nx;
        std::map<Key,std::vector<int>> nr;
        for(auto&kv:states){
            const std::vector<int>& P = rep[kv.first];
            const Big& w = kv.second;
            std::vector<int> xm(M),ym(M);
            for(int s=0;s<M;++s){ xm[s]=pX(P[s]); ym[s]=pY(P[s]); }
            for(int A:Askel){
                int top[8],bot[8],nt=0,nb=0;
                for(int s=0;s<M;++s){ if(A&(1<<s))top[nt++]=s; else bot[nb++]=s; }
                auto tX=validBijections(top,xm); if(tX.empty())continue;
                auto tY=validBijections(top,ym); if(tY.empty())continue;
                auto bX=validBijections(bot,xm); if(bX.empty())continue;
                auto bY=validBijections(bot,ym); if(bY.empty())continue;
                auto buildHist=[&](const int* syms,const std::vector<std::array<int,8>>& AX,const std::vector<std::array<int,8>>& AY){
                    std::map<PartKey,uint64_t> h;
                    for(auto&ax:AX) for(auto&ay:AY){
                        PartKey part{};
                        for(int i=0;i<C;++i){ int s=syms[i]; part[i]=(uint16_t)mkPair(xm[s]|(1<<ax[i]), ym[s]|(1<<ay[i])); }
                        std::sort(part.begin(),part.begin()+C);
                        h[part]++;
                    }
                    return h;
                };
                auto hTop=buildHist(top,tX,tY);
                auto hBot=buildHist(bot,bX,bY);
                std::vector<int> merged(M);
                for(auto&tp:hTop) for(auto&bp:hBot){
                    for(int i=0;i<C;++i){ merged[i]=tp.first[i]; merged[C+i]=bp.first[i]; }
                    std::vector<int> repPairs;
                    Key ck = canonMS(merged, (band+1<C)?&repPairs:nullptr);
                    nx[ck].addMul(w, tp.second*bp.second);
                    if(band+1<C && !nr.count(ck)) nr.emplace(ck,std::move(repPairs));
                }
            }
        }
        states=std::move(nx); rep=std::move(nr);
        double el=std::chrono::duration<double>(std::chrono::steady_clock::now()-t0).count();
        std::fprintf(stderr,"band %d: states=%zu elapsed=%.1fs\n",band,states.size(),el); std::fflush(stderr);
        if(dump) for(auto&kv:states){ std::fprintf(stdout,"  band%d -> %s\n",band,kv.second.str().c_str()); }
    }
    std::vector<int> full(M, mkPair(FULLC,FULLC)); Key fk=canonMS(full);
    Big N = states.count(fk)?states[fk]:Big();
    std::printf("C=%d: N=%s\n",C,N.str().c_str());
    const char*e=(C==2)?"288":(C==3)?"28200960":(C==4)?"29136487207403520":(C==5)?"1903816047972624930994913280000":nullptr;
    if(e)std::printf("  expect %s [%s]\n",e, N.str()==std::string(e)?"OK":"BAD");
    return 0;
}
