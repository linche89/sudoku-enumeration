// multiset_fast.cpp — N(2xC) via the CORRECT symbol-anonymous multiset state,
// with an ORBIT-AGGREGATED within-side histogram (replaces the (C!)^2 brute
// buildHist of multiset2xC.cpp) and a faster canonicalisation.
//
// STATE (proven correct, identical to multiset2xC.cpp):
//   symbol-anonymous multiset of M=2C per-symbol pairs (xmask,ymask),
//   canonicalised under S_C (perm X-cols) x S_C (perm Y-cols).
//   pair encoding: t = xmask | (ymask<<C).
//
// FAST WITHIN-SIDE HISTOGRAM (the deliverable):
//   One side = C symbols, each with old (xmask,ymask).  We need the histogram,
//   over all valid (X-bijection x Y-bijection), of the sorted multiset of new
//   pairs (xmask|{Xcol}, ymask|{Ycol}).  Symbols sharing an old pair are
//   interchangeable, so we GROUP by old pair (mult n_k) and enumerate:
//     (1) assignment of X-cols {0..C-1} to groups (group k gets n_k cols, col a
//         forbidden if a in xmask_k);
//     (2) independently, assignment of Y-cols to groups;
//     (3) within each group, all bijections pairing its X-cols to its Y-cols.
//   Weight = prod_k n_k!  (labeled symbols -> distinct slots).  Bin by output
//   multiset.  Validated byte-identical to brute over thousands of random states
//   at C=2,3,4 (see experiments/proto/fast_hist.py and the built-in --difftest).
//
// Verify: 288 (C=2), 28200960 (C=3), 29136487207403520 (C=4),
//   1903816047972624930994913280000 (C=5); C=6 is the new value.
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
#include <chrono>
#include <random>
#include <atomic>

static int C, M, FULLC;
static std::vector<int> Askel;                 // all C-subsets of [2C]
static std::vector<std::array<int,8>> CP;      // perms of {0..C-1}
static std::vector<std::vector<int>> permMask; // permMask[p][mask]

static const int MAXM=12;
using Key = std::array<uint16_t,MAXM>;          // M sorted pairs, zero-padded
using PartKey = std::array<uint16_t,6>;         // one side's C pairs, sorted

struct KeyHash {
    size_t operator()(const Key& k) const {
        size_t h=1469598103934665603ull;
        for(int i=0;i<MAXM;++i){ h^=k[i]; h*=1099511628211ull; }
        return h;
    }
};

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

// insertion sort for tiny arrays of small ints (M<=12).  Faster than std::sort here.
static inline void isort(uint16_t* a, int n){
    for(int i=1;i<n;++i){ uint16_t v=a[i]; int j=i-1; while(j>=0 && a[j]>v){ a[j+1]=a[j]; --j; } a[j+1]=v; }
}

// Reference brute canon (no pruning), used by `canontest` to validate the pruned canonMS.
static Key canonMSbrute(const uint16_t* pairs){
    const int m=M; uint16_t bestArr[MAXM]; bool have=false; uint16_t cand[MAXM];
    for(size_t rp=0; rp<CP.size(); ++rp){ const int* pmX=permMask[rp].data();
        for(size_t cp=0; cp<CP.size(); ++cp){ const int* pmY=permMask[cp].data();
            for(int i=0;i<m;++i){ int t=pairs[i]; cand[i]=(uint16_t)(pmX[t&FULLC]|(pmY[(t>>C)&FULLC]<<C)); }
            isort(cand,m);
            if(!have){ std::memcpy(bestArr,cand,m*sizeof(uint16_t)); have=true; continue; }
            int cmp=0; for(int i=0;i<m;++i){ if(cand[i]!=bestArr[i]){ cmp=cand[i]<bestArr[i]?-1:1; break; } }
            if(cmp<0) std::memcpy(bestArr,cand,m*sizeof(uint16_t));
        }
    }
    Key best{}; for(int i=0;i<M;++i)best[i]=bestArr[i]; return best;
}

// Canonicalise a multiset of M pairs under S_C x S_C.
// Hot kernel: for each (X-perm, Y-perm) relabel all M pairs (table-driven), sort, keep min.
// NOTE: a Y-restriction shortcut (only perms achieving the lex-min ymask-multiset) was tried
// and REJECTED by differential test (764/4000 mismatch at C=3): xmask is packed BELOW ymask,
// so a worse ymask arrangement can pair with a better xmask for a smaller packed sorted seq.
// Canonicalisation stays exact over all (C!)^2 perm pairs.
//
// SPEED: exact branch-and-bound over the X-permutation.  For each X-perm we form a valid
// LOWER BOUND on any reachable sorted sequence by pairing each X-relabeled xmask with the
// independent-minimum relabeled ymask of that pair = (1<<popcount(ymask))-1 (a column-perm
// can always push a ymask's bits to the lowest positions).  If sorted(LB) >= current best,
// the whole inner Y-perm loop is skipped.  This is EXACT (same optimum as full (C!)^2);
// verified by the `canontest` mode against the brute canon over many random multisets.
static Key canonMS(const uint16_t* pairs, std::vector<int>* repOut=nullptr){
    const int m=M;
    uint16_t bestArr[MAXM]; bool have=false;
    uint16_t cand[MAXM];
    // Precompute, per source pair, its xmask, ymask, and the independent-min relabeled ymask
    // = (1<<popcount(ymask))-1 (smallest mask with that many bits; any column-perm can reach it).
    uint16_t xms[MAXM], yms[MAXM]; uint16_t ymin[MAXM];
    for(int i=0;i<m;++i){ int t=pairs[i]; xms[i]=(uint16_t)(t&FULLC); yms[i]=(uint16_t)((t>>C)&FULLC);
        ymin[i]=(uint16_t)((1<<__builtin_popcount(yms[i]))-1); }
    uint16_t lb[MAXM];
    for(size_t rp=0; rp<CP.size(); ++rp){
        const int* pmX = permMask[rp].data();
        uint16_t xrel[MAXM];
        for(int i=0;i<m;++i) xrel[i]=(uint16_t)pmX[xms[i]];
        if(have){
            // Lower bound for this X-perm: pair each xrel_i with its independent-min ymask.
            // If sorted(lb) >= best, no Y-perm under this X-perm can beat best -> skip.
            for(int i=0;i<m;++i) lb[i]=(uint16_t)(xrel[i] | (ymin[i]<<C));
            isort(lb,m);
            int cmp=0; for(int i=0;i<m;++i){ if(lb[i]!=bestArr[i]){ cmp = lb[i]<bestArr[i]?-1:1; break; } }
            if(cmp>=0) continue;   // lb >= best -> prune entire X-perm
        }
        for(size_t cp=0; cp<CP.size(); ++cp){
            const int* pmY = permMask[cp].data();
            for(int i=0;i<m;++i){ int t=pairs[i]; cand[i]=(uint16_t)(xrel[i] | (pmY[(t>>C)&FULLC]<<C)); }
            isort(cand,m);
            if(!have){ std::memcpy(bestArr,cand,m*sizeof(uint16_t)); have=true; continue; }
            int cmp=0; for(int i=0;i<m;++i){ if(cand[i]!=bestArr[i]){ cmp = cand[i]<bestArr[i]?-1:1; break; } }
            if(cmp<0){ std::memcpy(bestArr,cand,m*sizeof(uint16_t)); }
        }
    }
    Key best{};
    for(int i=0;i<M;++i) best[i]=bestArr[i];
    if(repOut){ repOut->assign(M,0); for(int i=0;i<M;++i)(*repOut)[i]=bestArr[i]; }
    return best;
}

// ---- FAST aggregated within-side histogram ----
// Input: syms[0..C-1] = the side's symbols; xm[s],ym[s] = old masks (indexed by symbol id).
// Output: histogram of sorted C-pair PartKeys -> count (uint64).
struct FastHist {
    // groups
    int G;
    int gx[8], gy[8], gn[8];        // group X-mask, Y-mask, multiplicity
    uint64_t gfact;                 // product of n_k!
    // recursion scratch
    std::map<PartKey,uint64_t>* out;
    // per-group owned columns during recursion
    int xcols[8][8], xn[8];         // X-cols owned by group k
    int ycols[8][8], yn[8];
};

static inline uint64_t factU(int n){ uint64_t f=1; for(int i=2;i<=n;++i)f*=i; return f; }

// enumerate assignments of cols {0..C-1} to groups (cap gn[k], forbidden mask validmask[k]).
// calls cb(assign[]) where assign[col]=group.
template<class CB>
static void genColAssign(const int* validmask, const int* gn, int G, int Ccols, CB&& cb){
    int cap[8]; for(int k=0;k<G;++k)cap[k]=gn[k];
    int assign[8];
    // iterative-ish recursion via lambda
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

// Build the fast histogram into `out`.
static void buildHistFast(const int* syms, const std::vector<int>& xm, const std::vector<int>& ym,
                          std::map<PartKey,uint64_t>& out){
    // group symbols by old pair
    int gx[8], gy[8], gn[8], G=0;
    int sx[8], sy[8];
    for(int i=0;i<C;++i){ sx[i]=xm[syms[i]]; sy[i]=ym[syms[i]]; }
    for(int i=0;i<C;++i){
        int k=-1; for(int j=0;j<G;++j) if(gx[j]==sx[i]&&gy[j]==sy[i]){k=j;break;}
        if(k<0){ k=G++; gx[k]=sx[i]; gy[k]=sy[i]; gn[k]=0; }
        gn[k]++;
    }
    uint64_t gfact=1; for(int k=0;k<G;++k) gfact*=factU(gn[k]);

    // collect all X-assignments and Y-assignments first
    std::vector<std::array<int,8>> xAs, yAs;
    {
        int va[8]; for(int k=0;k<G;++k)va[k]=gx[k];
        genColAssign(va, gn, G, C, [&](const int* assign){ std::array<int,8> a{}; for(int c=0;c<C;++c)a[c]=assign[c]; xAs.push_back(a); });
        for(int k=0;k<G;++k)va[k]=gy[k];
        genColAssign(va, gn, G, C, [&](const int* assign){ std::array<int,8> a{}; for(int c=0;c<C;++c)a[c]=assign[c]; yAs.push_back(a); });
    }
    if(xAs.empty()||yAs.empty()) return;

    // For each (Xassign, Yassign), within each group pair X-cols to Y-cols (all bijections).
    PartKey part{};
    for(const auto& xa : xAs){
        int xcols[8][8], xn[8]={0};
        for(int c=0;c<C;++c){ int k=xa[c]; xcols[k][xn[k]++]=c; }
        for(const auto& ya : yAs){
            int ycols[8][8], yn[8]={0};
            for(int c=0;c<C;++c){ int k=ya[c]; ycols[k][yn[k]++]=c; }
            // enumerate per-group bijections via mixed odometer of permutation indices.
            // For group k with n_k cols, iterate all n_k! permutations of its ycols.
            // We do nested via recursion over groups.
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

// brute within-side histogram (reference), identical to multiset2xC buildHist.
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

// ---------- differential test mode ----------
static int difftest(int ntests, unsigned seed);

int main(int argc,char**argv){
    C=(argc>1)?atoi(argv[1]):3; M=2*C; FULLC=(1<<C)-1;
    std::string mode = (argc>2)?argv[2]:"";
    initPerms();

    if(mode=="difftest"){
        int n=(argc>3)?atoi(argv[3]):2000;
        unsigned seed=(argc>4)?(unsigned)atoi(argv[4]):12345u;
        return difftest(n,seed);
    }
    if(mode=="canontest"){
        int n=(argc>3)?atoi(argv[3]):20000;
        unsigned seed=(argc>4)?(unsigned)atoi(argv[4]):777u;
        std::mt19937 rng(seed); int mm=0;
        uint16_t pr[MAXM];
        for(int t=0;t<n;++t){
            // bias toward realistic multisets: random masks with some repeats
            for(int i=0;i<M;++i){ int xm=rng()&FULLC, ym=rng()&FULLC; pr[i]=(uint16_t)(xm|(ym<<C)); }
            Key a=canonMS(pr); Key b=canonMSbrute(pr);
            if(a!=b){ mm++; if(mm<=5){ std::fprintf(stderr,"CANON MISMATCH t=%d\n",t);} }
        }
        std::printf("canontest C=%d: tests=%d mismatches=%d [%s]\n",C,n,mm,mm==0?"OK":"FAIL");
        return mm==0?0:1;
    }

    bool dump=(mode=="dump");
    for(int m=0;m<(1<<M);++m) if(__builtin_popcount(m)==C) Askel.push_back(m);
    auto t0=std::chrono::steady_clock::now();

    std::map<Key,Big> states;
    std::map<Key,std::vector<int>> rep;
    { std::vector<uint16_t> empty(M, (uint16_t)mkPair(0,0)); Key k=canonMS(empty.data());
      std::vector<int> e(M, mkPair(0,0)); states[k]=Big((uint64_t)1); rep[k]=e; }

    for(int band=0; band<C; ++band){
        std::map<Key,Big> nx;
        std::map<Key,std::vector<int>> nr;
        bool keepRep = (band+1<C);
        // side-histogram memo: old-pair multiset (sorted) -> flat sorted histogram.
        std::unordered_map<Key,std::vector<std::pair<PartKey,uint64_t>>,KeyHash> sideMemo;
        auto getSide = [&](const std::vector<int>& pairMS)->const std::vector<std::pair<PartKey,uint64_t>>&{
            Key sk{}; for(int i=0;i<C;++i)sk[i]=(uint16_t)pairMS[i];   // already sorted by caller
            auto it=sideMemo.find(sk);
            if(it!=sideMemo.end()) return it->second;
            std::vector<int> xm(C),ym(C); int syms[8];
            for(int i=0;i<C;++i){ syms[i]=i; xm[i]=pX(pairMS[i]); ym[i]=pY(pairMS[i]); }
            std::map<PartKey,uint64_t> hm; buildHistFast(syms,xm,ym,hm);
            std::vector<std::pair<PartKey,uint64_t>> flat(hm.begin(),hm.end());
            return sideMemo.emplace(std::move(sk),std::move(flat)).first->second;
        };
        // PHASE A (serial, cheap vs canon): accumulate each DISTINCT raw merged multiset
        // -> Big weight = sum over (source state, skeleton class, hTop x hBot) of
        //   w_src * topcnt * botcnt * nclass.  Canon deferred to phase B (parallel).
        std::unordered_map<Key,Big,KeyHash> rawW;
        size_t srcDone=0, srcTotal=states.size(), classDone=0;
        auto lastPhaseLog = std::chrono::steady_clock::now();
        auto logPhaseA = [&](const char* tag, size_t src, size_t classes, size_t raw, size_t ht, size_t hb){
            double el=std::chrono::duration<double>(std::chrono::steady_clock::now()-t0).count();
            std::fprintf(stderr,"  band %d phaseA %s: src=%zu/%zu classes=%zu raw=%zu hTop=%zu hBot=%zu el=%.1fs\n",
                         band,tag,src,srcTotal,classes,raw,ht,hb,el);
            std::fflush(stderr);
            lastPhaseLog = std::chrono::steady_clock::now();
        };
        for(auto&kv:states){
            ++srcDone;
            const std::vector<int>& P = rep[kv.first];
            const Big& w = kv.second;
            std::map<std::pair<std::vector<int>,std::vector<int>>,uint64_t> classes;
            for(int A:Askel){
                std::vector<int> topMS, botMS;
                for(int s=0;s<M;++s){ if(A&(1<<s))topMS.push_back(P[s]); else botMS.push_back(P[s]); }
                std::sort(topMS.begin(),topMS.end()); std::sort(botMS.begin(),botMS.end());
                classes[{std::move(topMS),std::move(botMS)}]++;
            }
            for(auto& cl : classes){
                ++classDone;
                const auto& hTop = getSide(cl.first.first); if(hTop.empty())continue;
                const auto& hBot = getSide(cl.first.second); if(hBot.empty())continue;
                auto now = std::chrono::steady_clock::now();
                uint64_t crossSize = (uint64_t)hTop.size() * (uint64_t)hBot.size();
                if(crossSize >= 1000000 ||
                   std::chrono::duration<double>(now-lastPhaseLog).count() >= 10.0){
                    logPhaseA("class",srcDone,classDone,rawW.size(),hTop.size(),hBot.size());
                }
                uint64_t nclass = cl.second;
                std::unordered_map<Key,uint64_t,KeyHash> local;
                local.reserve(hTop.size()*hBot.size()/2 + 16);
                for(const auto&tp:hTop){
                    const uint16_t* a=tp.first.data(); uint64_t wt=tp.second;
                    for(const auto&bp:hBot){
                        const uint16_t* b=bp.first.data();
                        Key raw{}; int i=0,j=0,k=0;
                        while(i<C&&j<C){ if(a[i]<=b[j]) raw[k++]=a[i++]; else raw[k++]=b[j++]; }
                        while(i<C) raw[k++]=a[i++];
                        while(j<C) raw[k++]=b[j++];
                        local[raw] += wt*bp.second;
                    }
                }
                for(auto& lp : local) rawW[lp.first].addMul(w, lp.second * nclass);
                now = std::chrono::steady_clock::now();
                if((classDone & 0x3FF)==0 ||
                   std::chrono::duration<double>(now-lastPhaseLog).count() >= 10.0){
                    logPhaseA("done",srcDone,classDone,rawW.size(),hTop.size(),hBot.size());
                }
            }
        }
        // PHASE B (parallel): canonicalise each distinct raw independently.
        std::vector<Key> raws; raws.reserve(rawW.size());
        for(auto& kv : rawW) raws.push_back(kv.first);
        const long long NR = (long long)raws.size();
        { double el=std::chrono::duration<double>(std::chrono::steady_clock::now()-t0).count();
          std::fprintf(stderr,"  band %d phaseA done: raw=%lld sideMemo=%zu el=%.1fs\n",
                       band,NR,sideMemo.size(),el); std::fflush(stderr); }
        std::vector<Key> cks(NR);
        std::vector<std::vector<int>> creps(keepRep?NR:0);
        std::atomic<long long> canonDone{0};
        #pragma omp parallel for schedule(dynamic,128)
        for(long long idx=0; idx<NR; ++idx){
            std::vector<int> rp;
            cks[idx] = canonMS(raws[idx].data(), keepRep?&rp:nullptr);
            if(keepRep) creps[idx] = std::move(rp);
            long long d=++canonDone;
            if((d & 0x3FFFF)==0){
                double el=std::chrono::duration<double>(std::chrono::steady_clock::now()-t0).count();
                std::fprintf(stderr,"  band %d phaseB: canon=%lld/%lld el=%.1fs\n",band,d,NR,el);
                std::fflush(stderr);
            }
        }
        // PHASE C (serial): reduce into nx + pick a representative per canonical state.
        for(long long idx=0; idx<NR; ++idx){
            const Key& ck = cks[idx];
            nx[ck] += rawW[raws[idx]];
            if(keepRep && !nr.count(ck)) nr.emplace(ck, creps[idx]);
        }
        states=std::move(nx); rep=std::move(nr);
        double el=std::chrono::duration<double>(std::chrono::steady_clock::now()-t0).count();
        std::fprintf(stderr,"band %d: states=%zu elapsed=%.1fs\n",band,states.size(),el); std::fflush(stderr);
        if(dump) for(auto&kv:states){ std::fprintf(stdout,"  band%d -> %s\n",band,kv.second.str().c_str()); }
    }
    std::vector<uint16_t> full(M, (uint16_t)mkPair(FULLC,FULLC)); Key fk=canonMS(full.data());
    Big N = states.count(fk)?states[fk]:Big();
    std::printf("C=%d: N=%s\n",C,N.str().c_str());
    const char*e=(C==2)?"288":(C==3)?"28200960":(C==4)?"29136487207403520":(C==5)?"1903816047972624930994913280000":nullptr;
    if(e)std::printf("  expect %s [%s]\n",e, N.str()==std::string(e)?"OK":"BAD");
    return 0;
}

// ---------- differential test ----------
static int difftest(int ntests, unsigned seed){
    std::mt19937 rng(seed);
    int mismatches=0, empties=0;
    std::vector<int> xm(M,0), ym(M,0);
    for(int t=0;t<ntests;++t){
        // random old masks for C "side" symbols (use symbol ids 0..C-1)
        int syms[8]; for(int i=0;i<C;++i)syms[i]=i;
        // retry until a valid bijection exists on both sides
        for(;;){
            for(int i=0;i<C;++i){ xm[i]=rng()&FULLC; ym[i]=rng()&FULLC; }
            if(!validBijections(syms,xm).empty() && !validBijections(syms,ym).empty()) break;
        }
        std::map<PartKey,uint64_t> hb, hf;
        buildHistBrute(syms,xm,ym,hb);
        buildHistFast(syms,xm,ym,hf);
        if(hb!=hf){
            mismatches++;
            if(mismatches<=5){
                uint64_t tb=0,tf=0; for(auto&p:hb)tb+=p.second; for(auto&p:hf)tf+=p.second;
                std::fprintf(stderr,"MISMATCH C=%d t=%d  bruteTotal=%llu fastTotal=%llu  bruteKeys=%zu fastKeys=%zu\n",
                             C,t,(unsigned long long)tb,(unsigned long long)tf,hb.size(),hf.size());
                std::fprintf(stderr,"  xm:"); for(int i=0;i<C;++i)std::fprintf(stderr," %d",xm[i]);
                std::fprintf(stderr,"  ym:"); for(int i=0;i<C;++i)std::fprintf(stderr," %d",ym[i]); std::fprintf(stderr,"\n");
            }
        }
        if(hb.empty()) empties++;
    }
    std::printf("difftest C=%d: tests=%d mismatches=%d empties=%d  [%s]\n",
                C,ntests,mismatches,empties, mismatches==0?"OK":"FAIL");
    return mismatches==0?0:1;
}
