// kernel2xC.cpp — polynomial-aspiring joint transfer for N(2xC), with profiling.
//
// N(2xC) = sum over skeleton sequences (A_0..A_{C-1}) of stack0(A_*)^2
//        = a single band-by-band JOINT transfer where each band picks a SHARED
//          skeleton A and places it independently in copy X and copy Y; the
//          state is the canonicalised joint column-profile (X-cols, Y-cols).
//
// This version still ENUMERATES placements per band (the known bottleneck) but:
//   (1) uses the proven exact degree-bucket canonicalisation,
//   (2) prints per-band profiling (states, raw transitions, elapsed) so a long
//       run is never silent, and
//   (3) is structured so the placement loops can later be replaced by a
//       permanent-aggregated weight (the true polynomial kernel) WITHOUT
//       changing the state/canon machinery.
//
// Validate: C=2 -> 288, C=3 -> 28200960.
#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <vector>
#include <map>
#include <algorithm>
#include <string>
#include <chrono>
using u128 = unsigned __int128;

static int C, M;
static std::vector<int> Askel;

// ---- exact fast canonicalisation (degree buckets), proven == full S_{2C} canon
static std::string canon(const std::vector<int>& st) {   // st: [0,C)=X masks, [C,2C)=Y masks
    int degX[16]={0}, degY[16]={0};
    for (int c=0;c<C;++c){ for(int s=st[c];s;s&=s-1)degX[__builtin_ctz(s)]++;
                           for(int s=st[C+c];s;s&=s-1)degY[__builtin_ctz(s)]++; }
    std::map<std::pair<int,int>,std::vector<int>> tmp;
    for (int s=0;s<M;++s) tmp[{degX[s],degY[s]}].push_back(s);
    std::vector<std::pair<std::pair<int,int>,std::vector<int>>> bk(tmp.begin(),tmp.end());
    int NB=(int)bk.size(); std::vector<int> base(NB);
    { int a=0; for(int b=0;b<NB;++b){base[b]=a;a+=(int)bk[b].second.size();} }
    std::vector<std::vector<std::vector<int>>> bp(NB);
    for (int b=0;b<NB;++b){ std::vector<int> v=bk[b].second; std::sort(v.begin(),v.end());
        do bp[b].push_back(v); while(std::next_permutation(v.begin(),v.end())); }
    std::vector<int> idx(NB,0); std::string best;
    for(;;){
        int perm[16];
        for(int b=0;b<NB;++b){ const auto&P=bp[b][idx[b]]; for(int j=0;j<(int)P.size();++j) perm[P[j]]=base[b]+j; }
        std::vector<int> X(C),Y(C);
        for(int c=0;c<C;++c){ int mx=0,my=0;
            for(int s=st[c];s;s&=s-1)mx|=1<<perm[__builtin_ctz(s)];
            for(int s=st[C+c];s;s&=s-1)my|=1<<perm[__builtin_ctz(s)]; X[c]=mx;Y[c]=my; }
        std::sort(X.begin(),X.end()); std::sort(Y.begin(),Y.end());
        std::string k; k.reserve(4*C+1);
        for(int v:X){k.push_back((char)(v&255));k.push_back((char)((v>>8)&255));}
        k.push_back('|');
        for(int v:Y){k.push_back((char)(v&255));k.push_back((char)((v>>8)&255));}
        if(best.empty()||k<best)best=k;
        int b=0; for(;b<NB;++b){ if(++idx[b]<(int)bp[b].size())break; idx[b]=0; }
        if(b==NB)break;
    }
    return best;
}

static void placeRec(const std::vector<int>& cols, const std::vector<int>& Av,
                     const std::vector<int>& Acv, int c, int uT, int uB,
                     std::vector<int>& cur, std::vector<std::vector<int>>& out){
    if(c==C){ out.push_back(cur); return; }
    for(int ti=0;ti<C;++ti){ if(uT&(1<<ti))continue; int ts=Av[ti]; if(cols[c]&(1<<ts))continue;
        for(int bi=0;bi<C;++bi){ if(uB&(1<<bi))continue; int bs=Acv[bi]; if(cols[c]&(1<<bs))continue;
            cur[c]=cols[c]|(1<<ts)|(1<<bs);
            placeRec(cols,Av,Acv,c+1,uT|(1<<ti),uB|(1<<bi),cur,out); } }
}
static std::vector<std::vector<int>> place(const std::vector<int>& cols,int A){
    std::vector<int> Av,Acv; for(int s=0;s<M;++s){ if(A&(1<<s))Av.push_back(s); else Acv.push_back(s); }
    std::vector<std::vector<int>> out; std::vector<int> cur(C,0);
    placeRec(cols,Av,Acv,0,0,0,cur,out); return out;
}

int main(int argc,char**argv){
    C=(argc>1)?atoi(argv[1]):3; M=2*C;
    for(int m=0;m<(1<<M);++m) if(__builtin_popcount(m)==C) Askel.push_back(m);
    auto t0=std::chrono::steady_clock::now();

    std::map<std::string,u128> states; std::map<std::string,std::vector<int>> rep;
    { std::vector<int> e(2*C,0); std::string k=canon(e); states[k]=1; rep[k]=e; }
    for(int band=0;band<C;++band){
        std::map<std::string,u128> nx; std::map<std::string,std::vector<int>> nr;
        long long rawT=0;
        for(auto&kv:states){
            const std::vector<int>& st=rep[kv.first]; u128 w=kv.second;
            std::vector<int> Xc(st.begin(),st.begin()+C), Yc(st.begin()+C,st.end());
            for(int A:Askel){
                auto xo=place(Xc,A); if(xo.empty())continue;
                auto yo=place(Yc,A); if(yo.empty())continue;
                for(auto&a:xo)for(auto&b:yo){
                    std::vector<int> js; js.reserve(2*C);
                    js.insert(js.end(),a.begin(),a.end()); js.insert(js.end(),b.begin(),b.end());
                    std::string k=canon(js); nx[k]+=w; if(!nr.count(k))nr[k]=js; ++rawT;
                }
            }
        }
        states=std::move(nx); rep=std::move(nr);
        double el=std::chrono::duration<double>(std::chrono::steady_clock::now()-t0).count();
        std::fprintf(stderr,"band %d: states=%zu rawTrans=%lld elapsed=%.1fs\n",band,states.size(),rawT,el);
        std::fflush(stderr);
    }
    std::vector<int> full(2*C,(1<<M)-1); std::string fk=canon(full);
    u128 N=states.count(fk)?states[fk]:0;
    std::string s; if(!N)s="0"; { u128 x=N; while(x){s=char('0'+(int)(x%10))+s;x/=10;} }
    std::printf("C=%d: N=%s\n",C,s.c_str());
    const char*e=(C==2)?"288":(C==3)?"28200960":(C==4)?"29136487207403520":nullptr;
    if(e)std::printf("  expect %s [%s]\n",e,s==e?"OK":"BAD");
    return 0;
}
