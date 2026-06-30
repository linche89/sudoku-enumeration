// tmatrix.cpp — joint transfer for N(2xC) on the CROSS-STACK INTERSECTION-MATRIX
// state T[a][b] = |X_a ∩ Y_b|, canonicalised under row-perm x col-perm (C!^2).
//
// N(2xC) = sum over skeleton sequences of stack0^2 = a single band-by-band joint
// transfer (shared skeleton A per band; place independently in copies X and Y).
// The joint state's sufficient invariant (brute-validated C=2,3) is the C x C
// matrix of pairwise column intersections between the two stacks.
//
// This first version still ENUMERATES placements per band (correct but not yet
// fast); its job is to VALIDATE the T-matrix state at C=3 (=28200960) and reach
// C=4 (=29136487207403520).  Per-band profiling so it is never silent.
//
// Representation during transfer: we keep, per canonical state, ONE concrete
// representative (X cols, Y cols) as bitmasks, plus its weight.  Placing a
// skeleton expands placements; each new concrete (X,Y) is reduced to its
// T-matrix canonical key.
#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <vector>
#include <map>
#include <array>
#include <algorithm>
#include <string>
#include <chrono>
using u128 = unsigned __int128;

static int C, M;
static std::vector<int> Askel;
static std::vector<std::vector<int>> colperms;   // all permutations of {0..C-1}

// canonical key of the cross-stack intersection matrix T[a][b]=popcount(X[a]&Y[b])
static std::string tkey(const std::vector<int>& X, const std::vector<int>& Y) {
    int T[8][8];
    for (int a=0;a<C;++a) for (int b=0;b<C;++b) T[a][b]=__builtin_popcount(X[a]&Y[b]);
    std::string best;
    for (auto& rp : colperms) {
        for (auto& cp : colperms) {
            std::string k; k.reserve(C*C);
            for (int a=0;a<C;++a) for (int b=0;b<C;++b) k.push_back((char)T[rp[a]][cp[b]]);
            if (best.empty() || k<best) best=k;
        }
    }
    return best;
}

static void placeRec(const std::vector<int>& Av,const std::vector<int>& Acv,const std::vector<int>& cols,
                     int c,int uT,int uB,std::vector<int>& cur,std::vector<std::vector<int>>& out){
    if(c==C){ out.push_back(cur); return; }
    for(int ti=0;ti<C;++ti){ if(uT&(1<<ti))continue; int ts=Av[ti]; if(cols[c]&(1<<ts))continue;
        for(int bi=0;bi<C;++bi){ if(uB&(1<<bi))continue; int bs=Acv[bi]; if(cols[c]&(1<<bs))continue;
            cur[c]=cols[c]|(1<<ts)|(1<<bs);
            placeRec(Av,Acv,cols,c+1,uT|(1<<ti),uB|(1<<bi),cur,out); } }
}
static std::vector<std::vector<int>> place(const std::vector<int>& cols,int A){
    std::vector<int> Av,Acv; for(int s=0;s<M;++s){ if(A&(1<<s))Av.push_back(s); else Acv.push_back(s); }
    std::vector<std::vector<int>> out; std::vector<int> cur(C,0);
    placeRec(Av,Acv,cols,0,0,0,cur,out); return out;
}

int main(int argc,char**argv){
    C=(argc>1)?atoi(argv[1]):3; M=2*C;
    for(int m=0;m<(1<<M);++m) if(__builtin_popcount(m)==C) Askel.push_back(m);
    { std::vector<int> p(C); for(int i=0;i<C;++i)p[i]=i;
      do colperms.push_back(p); while(std::next_permutation(p.begin(),p.end())); }
    auto t0=std::chrono::steady_clock::now();

    std::map<std::string,u128> states; std::map<std::string,std::vector<int>> rep; // rep stores X||Y (2C masks)
    { std::vector<int> e(2*C,0); std::string k=tkey({e.begin(),e.begin()+C},{e.begin()+C,e.end()}); states[k]=1; rep[k]=e; }
    for(int band=0;band<C;++band){
        std::map<std::string,u128> nx; std::map<std::string,std::vector<int>> nr; long long rawT=0;
        for(auto&kv:states){
            const std::vector<int>& st=rep[kv.first]; u128 w=kv.second;
            std::vector<int> Xc(st.begin(),st.begin()+C), Yc(st.begin()+C,st.end());
            for(int A:Askel){
                auto xo=place(Xc,A); if(xo.empty())continue;
                auto yo=place(Yc,A); if(yo.empty())continue;
                for(auto&a:xo)for(auto&b:yo){
                    std::string k=tkey(a,b); nx[k]+=w;
                    if(!nr.count(k)){ std::vector<int> js; js.insert(js.end(),a.begin(),a.end()); js.insert(js.end(),b.begin(),b.end()); nr[k]=js; }
                    ++rawT;
                }
            }
        }
        states=std::move(nx); rep=std::move(nr);
        double el=std::chrono::duration<double>(std::chrono::steady_clock::now()-t0).count();
        std::fprintf(stderr,"band %d: states=%zu rawTrans=%lld elapsed=%.1fs\n",band,states.size(),rawT,el);
        std::fflush(stderr);
    }
    std::vector<int> full(2*C,(1<<M)-1); std::string fk=tkey({full.begin(),full.begin()+C},{full.begin()+C,full.end()});
    u128 N=states.count(fk)?states[fk]:0;
    std::string s; if(!N)s="0"; { u128 x=N; while(x){s=char('0'+(int)(x%10))+s;x/=10;} }
    std::printf("C=%d: N=%s\n",C,s.c_str());
    const char*e=(C==2)?"288":(C==3)?"28200960":(C==4)?"29136487207403520":(C==5)?"1903816047972624930994913280000":nullptr;
    if(e)std::printf("  expect %s [%s]\n",e,s==e?"OK":"BAD");
    return 0;
}
