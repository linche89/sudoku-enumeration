// exact2xC.cpp — joint-transfer for N(2xC) with EXACT canonicalisation, to test
// whether the cheap T-matrix state (final2xC) is a sufficient statistic.
//
// State = joint (X: C col-sets, Y: C col-sets) canonicalised EXACTLY under
//   S_{2C} (relabel symbols) x S_C (perm X-cols) x S_C (perm Y-cols).
// Canon (jointcanon-style, provably exact): bucket symbols by their invariant
//   (degX,degY) signature, permute only within buckets, sort columns.  Any
//   symmetry maps a symbol to one of identical (degX,degY), so the byte-min over
//   bucket-respecting symbol perms x column sorts is the true canonical form.
//
// Transition: explicit placement (top row = A, bot row = ~A, both bijections to
//   the C columns, no symbol repeated within a column) — NO convolution, so the
//   per-rep forward distribution is exact by construction.
//
// Band 0 (from empty): every symbol has (degX,degY)=(1,1) -> single 2C-bucket ->
//   (2C)! canon, the known blow-up.  But from EMPTY the cross-stack intersection
//   matrix T is itself a COMPLETE invariant (no history), so canonT is exact there.
//   We use canonT for band 0 (fast) and the exact canon for bands >= 1.
//
// Validation: must give 288 (C=2), 28200960 (C=3); the C=4 answer + per-band exact
//   canonical-state counts are the experiment.
#include "big.hpp"
#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <map>
#include <algorithm>
#include <string>
#include <chrono>

static int C, M;
static std::vector<int> Askel;
static std::vector<std::vector<int>> CP;

// ---- T-matrix canon (exact ONLY from empty; used for band 0) ----
static std::string canonT(const int T[8][8]) {
    unsigned char best[36]; bool have=false; const int n=C*C;
    for (auto& rp : CP) for (auto& cp : CP) {
        unsigned char cand[36]; int p=0;
        for (int a=0;a<C;++a) for (int b=0;b<C;++b) cand[p++]=(unsigned char)T[rp[a]][cp[b]];
        if(!have || std::memcmp(cand,best,n)<0){ std::memcpy(best,cand,n); have=true; }
    }
    return std::string((const char*)best, n);
}
static std::string keyXY(const std::vector<int>& X,const std::vector<int>& Y){
    int T[8][8]; for(int a=0;a<C;++a)for(int b=0;b<C;++b)T[a][b]=__builtin_popcount(X[a]&Y[b]);
    return canonT(T);
}

// ---- exact joint canon (bucket symbols by (degX,degY); permute within bucket; sort cols) ----
static std::string canonExact(const std::vector<int>& X,const std::vector<int>& Y){
    int degX[16]={0},degY[16]={0};
    for(int c=0;c<C;++c){ for(int s=X[c];s;s&=s-1)degX[__builtin_ctz(s)]++; for(int s=Y[c];s;s&=s-1)degY[__builtin_ctz(s)]++; }
    // buckets ordered by invariant (degX,degY) key
    std::map<std::pair<int,int>,std::vector<int>> tmp;
    for(int s=0;s<M;++s) tmp[{degX[s],degY[s]}].push_back(s);
    std::vector<std::vector<int>> buckets; std::vector<int> base; int acc=0;
    for(auto&kv:tmp){ buckets.push_back(kv.second); base.push_back(acc); acc+=(int)kv.second.size(); }
    const int NB=(int)buckets.size();
    std::vector<std::vector<std::vector<int>>> bperm(NB);
    for(int b=0;b<NB;++b){ std::vector<int> v=buckets[b]; std::sort(v.begin(),v.end());
        do bperm[b].push_back(v); while(std::next_permutation(v.begin(),v.end())); }
    std::vector<int> idx(NB,0);
    std::string best; bool have=false;
    for(;;){
        int perm[16];
        for(int b=0;b<NB;++b){ const std::vector<int>&bp=bperm[b][idx[b]];
            for(int j=0;j<(int)buckets[b].size();++j) perm[bp[j]]=base[b]+j; }
        int Xc[8],Yc[8];
        for(int c=0;c<C;++c){ int mx=0,my=0;
            for(int s=X[c];s;s&=s-1)mx|=1<<perm[__builtin_ctz(s)];
            for(int s=Y[c];s;s&=s-1)my|=1<<perm[__builtin_ctz(s)];
            Xc[c]=mx; Yc[c]=my; }
        std::sort(Xc,Xc+C); std::sort(Yc,Yc+C);
        std::string key; key.reserve(4*C+1);
        for(int c=0;c<C;++c){ key.push_back((char)(Xc[c]&0xff)); key.push_back((char)((Xc[c]>>8)&0xff)); }
        key.push_back('|');
        for(int c=0;c<C;++c){ key.push_back((char)(Yc[c]&0xff)); key.push_back((char)((Yc[c]>>8)&0xff)); }
        if(!have||key<best){best=std::move(key);have=true;}
        int b=0; for(;b<NB;++b){ if(++idx[b]<(int)bperm[b].size())break; idx[b]=0; }
        if(b==NB)break;
    }
    return best;
}

// place skeleton A into a single stack -> resulting stacks (top=A bij, bot=~A bij, col-valid)
static void placeRec(const std::vector<int>&cols,const std::vector<int>&Av,const std::vector<int>&Acv,
                     int c,int ut,int ub,std::vector<int>&cur,std::vector<std::vector<int>>&out){
    if(c==C){ out.push_back(cur); return; }
    for(int ti=0;ti<C;++ti){ int ts=Av[ti]; if(ut&(1<<ti))continue; if(cols[c]&(1<<ts))continue;
        for(int bi=0;bi<C;++bi){ int bs=Acv[bi]; if(ub&(1<<bi))continue; if(cols[c]&(1<<bs))continue;
            cur[c]=cols[c]|(1<<ts)|(1<<bs);
            placeRec(cols,Av,Acv,c+1,ut|(1<<ti),ub|(1<<bi),cur,out); } }
}
static std::vector<std::vector<int>> place(const std::vector<int>&cols,int A){
    std::vector<int> Av,Acv; for(int s=0;s<M;++s){ if(A&(1<<s))Av.push_back(s); else Acv.push_back(s); }
    std::vector<std::vector<int>> out; std::vector<int> cur(C,0);
    placeRec(cols,Av,Acv,0,0,0,cur,out); return out;
}

int main(int argc,char**argv){
    C=(argc>1)?atoi(argv[1]):3; M=2*C;
    bool dump=false, tc=false;
    for(int i=2;i<argc;++i){ std::string a=argv[i]; if(a=="dump")dump=true; else if(a=="tc")tc=true; }
    // tc: use T-canon (keyXY) for ALL bands instead of exact canon -> isolates whether
    // the cheap T state is a sufficient statistic for the EXACT explicit-placement transition.
    for(int m=0;m<(1<<M);++m) if(__builtin_popcount(m)==C) Askel.push_back(m);
    { std::vector<int> p(C); for(int i=0;i<C;++i)p[i]=i; do CP.push_back(p); while(std::next_permutation(p.begin(),p.end())); }
    auto t0=std::chrono::steady_clock::now();

    std::map<std::string,Big> states;
    std::map<std::string,std::pair<std::vector<int>,std::vector<int>>> rep;
    std::vector<int> empty(C,0);
    states[keyXY(empty,empty)]=Big((uint64_t)1); rep[keyXY(empty,empty)]={empty,empty};

    for(int band=0;band<C;++band){
        std::map<std::string,Big> nx;
        std::map<std::string,std::pair<std::vector<int>,std::vector<int>>> nr;
        bool b0=(band==0)||tc;
        for(auto&kv:states){
            const auto&XY=rep[kv.first]; const std::vector<int>&X=XY.first; const std::vector<int>&Y=XY.second; const Big&w=kv.second;
            for(int A:Askel){
                auto xo=place(X,A); if(xo.empty())continue;
                auto yo=place(Y,A); if(yo.empty())continue;
                for(auto&nxX:xo) for(auto&nyY:yo){
                    std::string k = b0 ? keyXY(nxX,nyY) : canonExact(nxX,nyY);
                    nx[k]+=w;
                    if(!nr.count(k)) nr.emplace(k,std::make_pair(nxX,nyY));
                }
            }
        }
        states=std::move(nx); rep=std::move(nr);
        double el=std::chrono::duration<double>(std::chrono::steady_clock::now()-t0).count();
        std::fprintf(stderr,"band %d: states=%zu elapsed=%.1fs\n",band,states.size(),el); std::fflush(stderr);
        if(dump) for(auto&kv:states){ std::fprintf(stdout,"  band%d %s -> %s\n",band,kv.first.c_str(),kv.second.str().c_str()); }
    }
    std::vector<int> full(C,(1<<M)-1);
    std::string fk = tc?keyXY(full,full):((C==1)?keyXY(full,full):canonExact(full,full));
    Big N=states.count(fk)?states[fk]:Big();
    std::printf("C=%d: N=%s\n",C,N.str().c_str());
    const char*e=(C==2)?"288":(C==3)?"28200960":(C==4)?"29136487207403520":(C==5)?"1903816047972624930994913280000":nullptr;
    if(e)std::printf("  expect %s [%s]\n",e,N.str()==std::string(e)?"OK":"BAD");
    return 0;
}
