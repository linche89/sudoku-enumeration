// final2xC.cpp — N(2xC) via quotient transfer on the T-matrix state.
//   * states discovered by ALIGNMENT (single stack is unique per band -> Y = relabeling of X);
//   * transition weights via the validated convolution (distA (*) distB) with cross-terms;
//   * weights in arbitrary-precision Big (u128 overflows at C>=6).
// Verification chain: must reproduce python golden per-band tables for C=3 and the OEIS values.
#include "big.hpp"
#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <vector>
#include <map>
#include <set>
#include <algorithm>
#include <string>
#include <cstring>
#include <chrono>

static int C, M;
static std::vector<int> Askel;
static std::vector<std::vector<int>> CP;            // perms of {0..C-1}

static std::string canonT(const int T[8][8]) {
    // Allocation-free hot loop: build each candidate into a stack buffer and keep the
    // byte-min via memcmp.  Only the winner is turned into a std::string (one alloc).
    // (A std::string temporary per (rp,cp) would exceed libstdc++'s 15-byte SSO at C>=4
    //  -- C*C=16 -- and heap-allocate ~ (C!)^2 times per call, the real C=4 wall.)
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

// convolution side histogram (Delta with cross-terms).  For each distinct Delta-key we
// also keep ONE witness placement (addX/addY = the new symbols this side adds to each
// X-/Y-column), so a concrete representative (X',Y') can be reconstructed as a byproduct.
struct SideEntry { uint64_t cnt; std::vector<int> addX, addY; };
static std::map<std::string,SideEntry> sideDist(const std::vector<int>& X,const std::vector<int>& Y,const std::vector<int>& side){
    std::map<std::string,SideEntry> h;
    for(auto&pX:CP){ bool ok=true; for(int i=0;i<C;++i) if(X[pX[i]]&(1<<side[i])){ok=false;break;} if(!ok)continue;
        for(auto&pY:CP){ bool o2=true; for(int i=0;i<C;++i) if(Y[pY[i]]&(1<<side[i])){o2=false;break;} if(!o2)continue;
            int D[8][8]={{0}};
            std::vector<int> addX(C,0),addY(C,0);
            for(int i=0;i<C;++i){ int s=side[i],ax=pX[i],ay=pY[i];
                addX[ax]|=1<<s; addY[ay]|=1<<s;
                D[ax][ay]++;
                for(int b=0;b<C;++b) if(Y[b]&(1<<s)) D[ax][b]++;
                for(int a=0;a<C;++a) if(X[a]&(1<<s)) D[a][ay]++;
            }
            std::string k; k.reserve(C*C); for(int a=0;a<C;++a)for(int b=0;b<C;++b)k.push_back((char)D[a][b]);
            auto it=h.find(k);
            if(it==h.end()) h.emplace(std::move(k),SideEntry{1,std::move(addX),std::move(addY)});
            else it->second.cnt++;
        }
    }
    return h;
}

int main(int argc,char**argv){
    C=(argc>1)?atoi(argv[1]):3; M=2*C;
    bool dump=(argc>2 && std::string(argv[2])=="dump");
    for(int m=0;m<(1<<M);++m) if(__builtin_popcount(m)==C) Askel.push_back(m);
    { std::vector<int> p(C); for(int i=0;i<C;++i)p[i]=i; do CP.push_back(p); while(std::next_permutation(p.begin(),p.end())); }
    auto t0=std::chrono::steady_clock::now();

    // band 0 starts from empty (0 bands filled). states: weight per canonical T.
    std::vector<int> empty(C,0);
    std::map<std::string,Big> states; std::map<std::string,std::pair<std::vector<int>,std::vector<int>>> rep;
    { std::string k=keyXY(empty,empty); states[k]=Big((uint64_t)1); rep[k]={empty,empty}; }

    for(int band=0; band<C; ++band){
        std::map<std::string,Big> nx;
        // representatives reconstructed as a byproduct of the weight pass: every target
        // canonical state comes from some (da,db) placement pair, so we capture the first
        // witness and rebuild a concrete (X',Y').  Guaranteed to cover every state in nx.
        std::map<std::string,std::pair<std::vector<int>,std::vector<int>>> nr;
        for(auto&kv:states){
            const auto& XY=rep[kv.first]; const std::vector<int>& X=XY.first; const std::vector<int>& Y=XY.second; const Big& w=kv.second;
            int Tb[8][8]; for(int a=0;a<C;++a)for(int b=0;b<C;++b)Tb[a][b]=__builtin_popcount(X[a]&Y[b]);
            for(int A:Askel){
                std::vector<int> Av,Acv; for(int s=0;s<M;++s){ if(A&(1<<s))Av.push_back(s); else Acv.push_back(s); }
                auto hA=sideDist(X,Y,Av); if(hA.empty())continue;
                auto hB=sideDist(X,Y,Acv); if(hB.empty())continue;
                for(auto&da:hA) for(auto&db:hB){
                    int T[8][8];
                    for(int a=0;a<C;++a)for(int b=0;b<C;++b)
                        T[a][b]=Tb[a][b]+(unsigned char)da.first[a*C+b]+(unsigned char)db.first[a*C+b];
                    std::string ck=canonT(T);
                    nx[ck].addMul(w, da.second.cnt*db.second.cnt);
                    if(band+1<C && !nr.count(ck)){
                        std::vector<int> Xn(C),Yn(C);
                        for(int a=0;a<C;++a){
                            Xn[a]=X[a]|da.second.addX[a]|db.second.addX[a];
                            Yn[a]=Y[a]|da.second.addY[a]|db.second.addY[a];
                        }
                        nr.emplace(ck,std::make_pair(std::move(Xn),std::move(Yn)));
                    }
                }
            }
        }
        states=std::move(nx); rep=std::move(nr);
        double el=std::chrono::duration<double>(std::chrono::steady_clock::now()-t0).count();
        std::fprintf(stderr,"band %d: states=%zu elapsed=%.1fs\n",band,states.size(),el); std::fflush(stderr);
        if(dump) for(auto&kv:states){ std::fprintf(stdout,"  band%d ",band); for(char c:kv.first)std::fprintf(stdout,"%d",(int)(unsigned char)c); std::fprintf(stdout," -> %s\n",kv.second.str().c_str()); }
    }
    std::vector<int> full(C,(1<<M)-1); std::string fk=keyXY(full,full);
    Big N = states.count(fk)?states[fk]:Big();
    std::printf("C=%d: N=%s\n",C,N.str().c_str());
    const char*e=(C==2)?"288":(C==3)?"28200960":(C==4)?"29136487207403520":(C==5)?"1903816047972624930994913280000":nullptr;
    if(e)std::printf("  expect %s [%s]\n",e, N.str()==std::string(e)?"OK":"BAD");
    return 0;
}
