// state_count.cpp — count DISTINCT CANONICAL STATES per band for N(2xC).
// No weights, no Big: pure reachable-canonical-state BFS, band by band.
// State = symbol-anonymous multiset of M=2C pairs (xmask,ymask), canon under S_C x S_C.
// Transition = the SAME validated transition as multiset_fast.cpp (skeleton A, top/bot
// independent X/Y bijections), but we only collect the SET of distinct canonical targets.
//
// Reuses: orbit-aggregated within-side histogram (only its KEYS matter here), skeleton
// grouping, global canon memo (sorted raw -> canonical key) so repeated raws are cheap.
//
// Output: per-band count of distinct canonical states, printed incrementally.
#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <array>
#include <vector>
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <algorithm>
#include <string>
#include <chrono>

static int C, M, FULLC;
static std::vector<int> Askel;
static std::vector<std::array<int,8>> CP;
static std::vector<std::vector<int>> permMask;

static const int MAXM=12;
using Key = std::array<uint16_t,MAXM>;
using PartKey = std::array<uint16_t,6>;

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

static inline void isort(uint16_t* a, int n){
    for(int i=1;i<n;++i){ uint16_t v=a[i]; int j=i-1; while(j>=0 && a[j]>v){ a[j+1]=a[j]; --j; } a[j+1]=v; }
}

// Pruned-exact canon (validated identical to brute at C=3..6 in multiset_fast.cpp).
static Key canonMS(const uint16_t* pairs, std::vector<int>* repOut=nullptr){
    const int m=M;
    uint16_t bestArr[MAXM]; bool have=false;
    uint16_t cand[MAXM];
    uint16_t xms[MAXM], yms[MAXM], ymin[MAXM];
    for(int i=0;i<m;++i){ int t=pairs[i]; xms[i]=(uint16_t)(t&FULLC); yms[i]=(uint16_t)((t>>C)&FULLC);
        ymin[i]=(uint16_t)((1<<__builtin_popcount(yms[i]))-1); }
    uint16_t lb[MAXM];
    for(size_t rp=0; rp<CP.size(); ++rp){
        const int* pmX = permMask[rp].data();
        uint16_t xrel[MAXM];
        for(int i=0;i<m;++i) xrel[i]=(uint16_t)pmX[xms[i]];
        if(have){
            for(int i=0;i<m;++i) lb[i]=(uint16_t)(xrel[i] | (ymin[i]<<C));
            isort(lb,m);
            int cmp=0; for(int i=0;i<m;++i){ if(lb[i]!=bestArr[i]){ cmp = lb[i]<bestArr[i]?-1:1; break; } }
            if(cmp>=0) continue;
        }
        for(size_t cp=0; cp<CP.size(); ++cp){
            const int* pmY = permMask[cp].data();
            for(int i=0;i<m;++i){ cand[i]=(uint16_t)(xrel[i] | (pmY[yms[i]]<<C)); }
            isort(cand,m);
            if(!have){ std::memcpy(bestArr,cand,m*sizeof(uint16_t)); have=true; continue; }
            int cmp=0; for(int i=0;i<m;++i){ if(cand[i]!=bestArr[i]){ cmp = cand[i]<bestArr[i]?-1:1; break; } }
            if(cmp<0){ std::memcpy(bestArr,cand,m*sizeof(uint16_t)); }
        }
    }
    Key best{}; for(int i=0;i<M;++i) best[i]=bestArr[i];
    if(repOut){ repOut->assign(M,0); for(int i=0;i<M;++i)(*repOut)[i]=bestArr[i]; }
    return best;
}

static inline uint64_t factU(int n){ uint64_t f=1; for(int i=2;i<=n;++i)f*=i; return f; }

template<class CB>
static void genColAssign(const int* validmask, const int* gn, int G, int Ccols, CB&& cb){
    int cap[8]; for(int k=0;k<G;++k)cap[k]=gn[k];
    int assign[8];
    struct Rec {
        const int* validmask; int G; int Cc; int* cap; int* assign; CB& cb;
        void go(int a){
            if(a==Cc){ cb(assign); return; }
            for(int k=0;k<G;++k){
                if(cap[k]>0 && !(validmask[k] & (1<<a))){
                    cap[k]--; assign[a]=k; go(a+1); cap[k]++;
                }
            }
        }
    } r{validmask, G, Ccols, cap, assign, cb};
    r.go(0);
}

// Side: produce the SET of distinct resulting PartKeys (we ignore counts for enumeration).
static void buildSideKeys(const std::vector<int>& pairMS, std::vector<PartKey>& outKeys){
    int gx[8], gy[8], gn[8], G=0;
    int sx[8], sy[8];
    for(int i=0;i<C;++i){ sx[i]=pX(pairMS[i]); sy[i]=pY(pairMS[i]); }
    for(int i=0;i<C;++i){
        int k=-1; for(int j=0;j<G;++j) if(gx[j]==sx[i]&&gy[j]==sy[i]){k=j;break;}
        if(k<0){ k=G++; gx[k]=sx[i]; gy[k]=sy[i]; gn[k]=0; }
        gn[k]++;
    }
    std::vector<std::array<int,8>> xAs, yAs;
    { int va[8];
      for(int k=0;k<G;++k)va[k]=gx[k];
      genColAssign(va, gn, G, C, [&](const int* assign){ std::array<int,8> a{}; for(int c=0;c<C;++c)a[c]=assign[c]; xAs.push_back(a); });
      for(int k=0;k<G;++k)va[k]=gy[k];
      genColAssign(va, gn, G, C, [&](const int* assign){ std::array<int,8> a{}; for(int c=0;c<C;++c)a[c]=assign[c]; yAs.push_back(a); });
    }
    std::set<PartKey> uniq;
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
                PartKey& part; int pos; std::set<PartKey>& uniq; int C;
                void go(int k){
                    if(k==G){ PartKey p2=part; std::sort(p2.begin(), p2.begin()+pos); uniq.insert(p2); return; }
                    int n=xn[k];
                    int perm[8]; for(int i=0;i<n;++i)perm[i]=i;
                    do{
                        int save=pos;
                        for(int i=0;i<n;++i){ int a=xcols[k][i], b=ycols[k][perm[i]];
                            part[pos++]=(uint16_t)((gx[k]|(1<<a)) | ((gy[k]|(1<<b))<<C)); }
                        go(k+1); pos=save;
                    } while(std::next_permutation(perm,perm+n));
                }
            } gr{G,gx,gy,xcols,ycols,xn,part,0,uniq,C};
            gr.go(0);
        }
    }
    outKeys.assign(uniq.begin(), uniq.end());
}

int main(int argc,char**argv){
    C=(argc>1)?atoi(argv[1]):3; M=2*C; FULLC=(1<<C)-1;
    initPerms();
    for(int m=0;m<(1<<M);++m) if(__builtin_popcount(m)==C) Askel.push_back(m);
    auto t0=std::chrono::steady_clock::now();

    // band 0 starts from the empty state.
    std::vector<int> e(M, mkPair(0,0));
    std::vector<uint16_t> e16(M); for(int i=0;i<M;++i)e16[i]=(uint16_t)e[i];
    Key k0=canonMS(e16.data());
    std::unordered_map<Key,std::vector<int>,KeyHash> cur;   // canonical key -> a representative
    { std::vector<int> rep; canonMS(e16.data(), &rep); cur[k0]=rep; }

    for(int band=0; band<C; ++band){
        std::unordered_map<Key,std::vector<int>,KeyHash> nxt;
        std::unordered_map<Key,Key,KeyHash> canonMemo;     // sorted raw -> canonical key
        std::unordered_map<Key,std::vector<PartKey>,KeyHash> sideMemo;  // sorted old-pair MS -> side keys
        auto getSide=[&](const std::vector<int>& pairMS)->const std::vector<PartKey>&{
            Key sk{}; for(int i=0;i<C;++i)sk[i]=(uint16_t)pairMS[i];
            auto it=sideMemo.find(sk); if(it!=sideMemo.end()) return it->second;
            std::vector<PartKey> ks; buildSideKeys(pairMS, ks);
            return sideMemo.emplace(std::move(sk),std::move(ks)).first->second;
        };
        size_t srcIdx=0, nsrc=cur.size();
        unsigned long long newStatesThisBand=0, canonCalls=0, rawSeen=0, classDone=0;
        for(auto& kv : cur){
            ++srcIdx;
            classDone=0;
            const std::vector<int>& P = kv.second;
            // group skeletons by (top old-pair multiset, bot old-pair multiset)
            std::set<std::pair<std::vector<int>,std::vector<int>>> classes;
            for(int A:Askel){
                std::vector<int> topMS, botMS;
                for(int s=0;s<M;++s){ if(A&(1<<s))topMS.push_back(P[s]); else botMS.push_back(P[s]); }
                std::sort(topMS.begin(),topMS.end()); std::sort(botMS.begin(),botMS.end());
                classes.insert({std::move(topMS),std::move(botMS)});
            }
            for(const auto& cl : classes){
                const auto& hTop = getSide(cl.first); if(hTop.empty())continue;
                const auto& hBot = getSide(cl.second); if(hBot.empty())continue;
                ++classDone;
                for(const auto&tp:hTop){
                    const uint16_t* a=tp.data();
                    for(const auto&bp:hBot){
                        const uint16_t* b=bp.data();
                        Key raw{};
                        int i=0,j=0,k=0;
                        while(i<C&&j<C){ if(a[i]<=b[j]) raw[k++]=a[i++]; else raw[k++]=b[j++]; }
                        while(i<C) raw[k++]=a[i++];
                        while(j<C) raw[k++]=b[j++];
                        ++rawSeen;
                        if(canonMemo.find(raw)!=canonMemo.end()) continue;
                        std::vector<int> rep;
                        Key ck=canonMS(raw.data(), &rep);
                        ++canonCalls;
                        canonMemo.emplace(raw, ck);
                        if(nxt.find(ck)==nxt.end()){ nxt.emplace(ck, std::move(rep)); ++newStatesThisBand; }
                    }
                }
                // Fine-grained progress: after EACH class of the FIRST source state, report.
                if(srcIdx==1){
                    double el=std::chrono::duration<double>(std::chrono::steady_clock::now()-t0).count();
                    std::fprintf(stderr,"   [b%d src1] class %llu/%zu  distinctTargetsSoFar=%zu canonMemo=%zu canonCalls=%llu rawSeen=%llu el=%.1fs\n",
                        band,classDone,classes.size(),nxt.size(),canonMemo.size(),canonCalls,rawSeen,el); std::fflush(stderr);
                }
            }
            double el=std::chrono::duration<double>(std::chrono::steady_clock::now()-t0).count();
            std::fprintf(stderr,"  band%d src %zu/%zu  distinctStatesSoFar=%zu canonMemo=%zu canonCalls=%llu rawSeen=%llu el=%.1fs\n",
                band,srcIdx,nsrc,nxt.size(),canonMemo.size(),canonCalls,rawSeen,el); std::fflush(stderr);
        }
        cur=std::move(nxt);
        double el=std::chrono::duration<double>(std::chrono::steady_clock::now()-t0).count();
        std::fprintf(stderr,"BAND %d DONE: distinctCanonicalStates=%zu  el=%.1fs\n",band,cur.size(),el); std::fflush(stderr);
        std::printf("C=%d band %d: distinct canonical states = %zu  (%.1fs)\n",C,band,cur.size(),el); std::fflush(stdout);
    }
    return 0;
}
