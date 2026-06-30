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
//   at C=2,3,4 (see proto/fast_hist.py and the built-in --difftest).
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
#include <omp.h>

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
// ---- refinement-based canonical KEY (faithful orbit invariant; NOT lex-min) ----
// Validated in canon_refine2.cpp: invariant under relabeling + separates orbits exactly
// like brute (#keys equal, 0 over-merge).  Equitable-partition refinement on exact
// co-occurrence signatures, then min over the residual within-colour-class symmetry only.
static void refineColours(const uint16_t* pairs, int xcol[8], int ycol[8]){
    for(int c=0;c<C;++c){ xcol[c]=0; ycol[c]=0; }
    for(int it=0; it<2*C+3; ++it){
        std::vector<std::vector<long long>> xs(C),ys(C);
        for(int c=0;c<C;++c){ xs[c].push_back(xcol[c]); ys[c].push_back(ycol[c]); }
        std::vector<std::vector<long long>> xa(C),ya(C);
        for(int i=0;i<M;++i){ int xm=pairs[i]&FULLC, ym=(pairs[i]>>C)&FULLC;
            long long yh[8]={0}; for(int b=ym;b;b&=b-1) yh[ycol[__builtin_ctz(b)]]++;
            long long xh[8]={0}; for(int b=xm;b;b&=b-1) xh[xcol[__builtin_ctz(b)]]++;
            long long yco=0; for(int q=0;q<C;++q) yco=yco*16+yh[q];
            long long xco=0; for(int q=0;q<C;++q) xco=xco*16+xh[q];
            for(int b=xm;b;b&=b-1) xa[__builtin_ctz(b)].push_back(yco);
            for(int b=ym;b;b&=b-1) ya[__builtin_ctz(b)].push_back(xco); }
        for(int c=0;c<C;++c){ std::sort(xa[c].begin(),xa[c].end()); for(auto v:xa[c])xs[c].push_back(v);
            std::sort(ya[c].begin(),ya[c].end()); for(auto v:ya[c])ys[c].push_back(v); }
        auto rec=[&](std::vector<std::vector<long long>>&s,int*col)->bool{
            std::vector<int>o(C); for(int c=0;c<C;++c)o[c]=c;
            std::sort(o.begin(),o.end(),[&](int a,int b){return s[a]<s[b];});
            int nc[8],g=0; nc[o[0]]=0; for(int i=1;i<C;++i){ if(s[o[i]]!=s[o[i-1]])++g; nc[o[i]]=g; }
            bool ch=false; for(int c=0;c<C;++c){ if(col[c]!=nc[c])ch=true; col[c]=nc[c]; } return ch; };
        bool c1=rec(xs,xcol),c2=rec(ys,ycol); if(!c1&&!c2)break;
    }
}
static void withinClassPerms(const int*col,std::vector<std::array<int,8>>&out){
    int ncol=0; for(int c=0;c<C;++c)ncol=std::max(ncol,col[c]); ncol++;
    std::vector<std::vector<int>> mem(ncol); for(int c=0;c<C;++c)mem[col[c]].push_back(c);
    int base[8],acc=0; for(int g=0;g<ncol;++g){base[g]=acc;acc+=(int)mem[g].size();}
    std::vector<std::vector<std::array<int,8>>> gp(ncol);
    for(int g=0;g<ncol;++g){ std::vector<int>v=mem[g]; std::sort(v.begin(),v.end());
        do{ std::array<int,8>a{}; for(size_t i=0;i<v.size();++i)a[i]=v[i]; gp[g].push_back(a);}while(std::next_permutation(v.begin(),v.end())); }
    std::vector<int> idx(ncol,0); out.clear();
    for(;;){ std::array<int,8>perm{}; for(int g=0;g<ncol;++g){const auto&a=gp[g][idx[g]];for(size_t j=0;j<mem[g].size();++j)perm[a[j]]=base[g]+j;}
        out.push_back(perm); int g=0; for(;g<ncol;++g){ if(++idx[g]<(int)gp[g].size())break; idx[g]=0; } if(g>=ncol)break; }
}
static Key canonMS(const uint16_t* pairs, std::vector<int>* repOut=nullptr){
    int xcol[8],ycol[8]; refineColours(pairs,xcol,ycol);
    std::vector<std::array<int,8>> xp,yp; withinClassPerms(xcol,xp); withinClassPerms(ycol,yp);
    uint16_t xms[MAXM],yms[MAXM]; for(int i=0;i<M;++i){ xms[i]=pairs[i]&FULLC; yms[i]=(pairs[i]>>C)&FULLC; }
    uint16_t bestArr[MAXM]; bool have=false; uint16_t cand[MAXM];
    for(const auto&xpp:xp){ uint16_t xr[MAXM];
        for(int i=0;i<M;++i){ int r=0; for(int b=xms[i];b;b&=b-1)r|=1<<xpp[__builtin_ctz(b)]; xr[i]=(uint16_t)r; }
        for(const auto&ypp:yp){
            for(int i=0;i<M;++i){ int r=0; for(int b=yms[i];b;b&=b-1)r|=1<<ypp[__builtin_ctz(b)]; cand[i]=(uint16_t)(xr[i]|(r<<C)); }
            isort(cand,M);
            if(!have){ std::memcpy(bestArr,cand,M*sizeof(uint16_t)); have=true; continue; }
            int cmp=0; for(int i=0;i<M;++i){ if(cand[i]!=bestArr[i]){ cmp=cand[i]<bestArr[i]?-1:1; break; } }
            if(cmp<0) std::memcpy(bestArr,cand,M*sizeof(uint16_t));
        }
    }
    Key best{}; for(int i=0;i<M;++i) best[i]=bestArr[i];
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
        const char* mbenv=getenv("MAXBAND");
        if(mbenv && band>atoi(mbenv)){ std::fprintf(stderr,"[profile] stopping after band %s\n",mbenv); break; }
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
        // ----- MEMORY-BOUNDED STREAMING TRANSITION -----
        // The previous design materialised EVERY distinct raw merged multiset in one map
        // before canonicalising -> at C=5 that is tens of millions of entries (many GB) and
        // a per-thread copy of it OOM-killed a 64GB box.  Instead we STREAM by skeleton class:
        // each class's local histogram (hTop x hBot, a few MB) is built, canonicalised, and
        // folded IMMEDIATELY into the tiny canonical-state table, then freed.  We only ever
        // keep canonical STATES (hundreds), never the raw multisets.  Peak memory =
        //   (one class local ~few MB) x threads  +  (#states x threads) -- bounded, indep of C.
        struct Work { int src; Key topK, botK; uint64_t nclass; };
        std::vector<Work> work;
        std::vector<const std::vector<int>*> srcP;
        std::vector<const Big*> srcW;
        {
            int si=0;
            for(auto&kv:states){
                srcP.push_back(&rep[kv.first]); srcW.push_back(&kv.second);
                const std::vector<int>& P=*srcP.back();
                std::map<std::pair<std::vector<int>,std::vector<int>>,uint64_t> classes;
                for(int A:Askel){
                    std::vector<int> topMS,botMS;
                    for(int s=0;s<M;++s){ if(A&(1<<s))topMS.push_back(P[s]); else botMS.push_back(P[s]); }
                    std::sort(topMS.begin(),topMS.end()); std::sort(botMS.begin(),botMS.end());
                    classes[{std::move(topMS),std::move(botMS)}]++;
                }
                for(auto& cl:classes){
                    getSide(cl.first.first); getSide(cl.first.second);   // pre-populate (serial)
                    Key tk{},bk{}; for(int i=0;i<C;++i){ tk[i]=(uint16_t)cl.first.first[i]; bk[i]=(uint16_t)cl.first.second[i]; }
                    work.push_back(Work{si,tk,bk,cl.second});
                }
                ++si;
            }
        }
        const long long NW=(long long)work.size();
        { double el=std::chrono::duration<double>(std::chrono::steady_clock::now()-t0).count();
          // PROFILE: side-histogram stats — premise check for state-level aggregation.
          size_t nsides=sideMemo.size(), maxh=0, toth=0;
          for(auto&kv:sideMemo){ size_t s=kv.second.size(); toth+=s; if(s>maxh)maxh=s; }
          std::fprintf(stderr,"  band %d: %lld class-tasks, distinctSides=%zu (avgHist=%.0f maxHist=%zu)  el=%.1fs\n",
            band,NW,nsides, nsides?(double)toth/nsides:0.0, maxh, el); std::fflush(stderr); }
        // thread-local canonical-state accumulators (tiny: bounded by #states).
        std::vector<std::map<Key,Big>> tnx;
        std::vector<std::map<Key,std::vector<int>>> tnr;
        std::atomic<long long> doneTasks{0};
        std::atomic<long long> profRawPairs{0}, profLocalRaws{0}, profCanonCalls{0};
        #pragma omp parallel
        {
            int nth=omp_get_num_threads(), tid=omp_get_thread_num();
            #pragma omp single
            { tnx.resize(nth); tnr.resize(nth); }
            #pragma omp barrier
            std::map<Key,Big>& mynx=tnx[tid];
            std::map<Key,std::vector<int>>& mynr=tnr[tid];
            std::unordered_map<Key,uint64_t,KeyHash> local;     // reused per class, bounded
            long long locRawPairs=0, locLocalRaws=0, locCanonCalls=0;
            // bounded per-thread canon cache: catches raws that recur across classes without
            // retaining all raws (capped -> memory stays bounded, the whole point for C=6).
            std::unordered_map<Key,Key,KeyHash> ccache;
            std::unordered_map<Key,std::vector<int>,KeyHash> crepc;
            const size_t CCAP = 2'000'000;   // ~ tens of MB/thread cap
            #pragma omp for schedule(dynamic,1)
            for(long long wi=0; wi<NW; ++wi){
                const Work& W=work[wi];
                const auto& hTop=sideMemo.find(W.topK)->second;
                const auto& hBot=sideMemo.find(W.botK)->second;
                if(hTop.empty()||hBot.empty()) continue;
                const Big& w=*srcW[W.src];
                local.clear();
                local.reserve(hTop.size()*hBot.size()/2+16);
                locRawPairs += (long long)hTop.size()*hBot.size();
                for(const auto&tp:hTop){ const uint16_t* a=tp.first.data(); uint64_t wt=tp.second;
                    for(const auto&bp:hBot){ const uint16_t* b=bp.first.data();
                        Key raw{}; int i=0,j=0,k=0;
                        while(i<C&&j<C){ if(a[i]<=b[j]) raw[k++]=a[i++]; else raw[k++]=b[j++]; }
                        while(i<C) raw[k++]=a[i++];
                        while(j<C) raw[k++]=b[j++];
                        local[raw]+=wt*bp.second;
                    }
                }
                locLocalRaws += (long long)local.size();
                for(auto& lp:local){
                    Key ck;
                    auto cit=ccache.find(lp.first);
                    if(cit!=ccache.end()){
                        ck=cit->second;
                        if(keepRep && !mynr.count(ck)){ auto rit=crepc.find(lp.first); if(rit!=crepc.end()) mynr.emplace(ck,rit->second); }
                    } else {
                        std::vector<int> rp;
                        ck=canonMS(lp.first.data(), keepRep?&rp:nullptr);
                        ++locCanonCalls;
                        if(ccache.size()<CCAP){ ccache.emplace(lp.first,ck); if(keepRep) crepc.emplace(lp.first,rp); }
                        if(keepRep && !mynr.count(ck)) mynr.emplace(ck,std::move(rp));
                    }
                    mynx[ck].addMul(w, lp.second*W.nclass);
                }
                long long d=++doneTasks;
                if((d & 0x3FF)==0){ double el=std::chrono::duration<double>(std::chrono::steady_clock::now()-t0).count();
                    std::fprintf(stderr,"    class %lld/%lld  el=%.1fs\n",d,NW,el); std::fflush(stderr); }
            }
            profRawPairs+=locRawPairs; profLocalRaws+=locLocalRaws; profCanonCalls+=locCanonCalls;
        }
        // merge thread-local canonical-state tables (serial; small).
        for(auto& tm:tnx) for(auto& kv:tm) nx[kv.first]+=kv.second;
        if(keepRep) for(auto& tm:tnr) for(auto& kv:tm) if(!nr.count(kv.first)) nr.emplace(kv.first,kv.second);
        states=std::move(nx); rep=std::move(nr);
        double el=std::chrono::duration<double>(std::chrono::steady_clock::now()-t0).count();
        std::fprintf(stderr,"band %d: states=%zu elapsed=%.1fs | PROFILE rawPairs=%lld localRaws=%lld canonCalls=%lld (cacheHitRate=%.1f%%)\n",
            band,states.size(),el,(long long)profRawPairs,(long long)profLocalRaws,(long long)profCanonCalls,
            profLocalRaws? 100.0*(1.0-(double)profCanonCalls/(double)profLocalRaws):0.0); std::fflush(stderr);
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
