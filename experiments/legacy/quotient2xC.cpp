// quotient2xC.cpp — N(2xC) via a QUOTIENT transfer matrix (Clarke-style symmetry reduction).
//
// State = canonical cross-stack intersection matrix T (row/col-perm S_C x S_C). The transition is
// PROVEN well-defined on T (independent of the concrete (X,Y) representative). So we:
//   (1) discover all reachable canonical T-states per band, keeping ONE concrete representative each
//       (captured as a free byproduct of a placement, NOT via a separate search);
//   (2) compute each state's out-weights by the VALIDATED convolution (distA (*) distB);
//   (3) the band DP is just: nx[T'] += w * weight(T->T').
// Representative discovery uses a bounded placement pass that stops once every weighted target has a
// representative -- but the WEIGHTS come from the convolution, never from enumeration.
//
// VERIFICATION: reproduces python golden per-band tables for C=3 (4320/12960/8640; ...; 28200960).
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
static std::vector<std::vector<int>> CP;

static std::string canonT(const int T[8][8]) {
    std::string best;
    for (auto& rp : CP) for (auto& cp : CP) {
        std::string k; k.reserve(C*C);
        for (int a=0;a<C;++a) for (int b=0;b<C;++b) k.push_back((char)T[rp[a]][cp[b]]);
        if (best.empty()||k<best) best=k;
    }
    return best;
}
static std::string canonXY(const std::vector<int>& X,const std::vector<int>& Y){
    int T[8][8]; for(int a=0;a<C;++a)for(int b=0;b<C;++b)T[a][b]=__builtin_popcount(X[a]&Y[b]);
    return canonT(T);
}
// A-side / ~A-side Delta histogram (convolution factor)
static std::map<std::string,u128> sideDist(const std::vector<int>& X,const std::vector<int>& Y,const std::vector<int>& side){
    std::map<std::string,u128> h;
    for(auto&pX:CP){ bool ok=true; for(int i=0;i<C;++i) if(X[pX[i]]&(1<<side[i])){ok=false;break;} if(!ok)continue;
        for(auto&pY:CP){ bool o2=true; for(int i=0;i<C;++i) if(Y[pY[i]]&(1<<side[i])){o2=false;break;} if(!o2)continue;
            int D[8][8]={{0}};
            for(int i=0;i<C;++i){ int s=side[i],ax=pX[i],ay=pY[i];
                D[ax][ay]++;
                for(int b=0;b<C;++b) if(Y[b]&(1<<s)) D[ax][b]++;
                for(int a=0;a<C;++a) if(X[a]&(1<<s)) D[a][ay]++;
            }
            std::string k; k.reserve(C*C); for(int a=0;a<C;++a)for(int b=0;b<C;++b)k.push_back((char)D[a][b]);
            h[k]+=1;
        }
    }
    return h;
}
// one valid placement (greedy first) of skeleton into a stack, to capture a representative.
// returns false if none. We collect ALL via recursion but stop early outside.
static bool firstPlacement(const std::vector<int>& cols,const std::vector<int>& Av,const std::vector<int>& Acv,std::vector<int>& out){
    for(auto&pT:CP){ bool ok=true; for(int i=0;i<C;++i) if(cols[pT[i]]&(1<<Av[i])){ok=false;break;} if(!ok)continue;
        for(auto&pB:CP){ bool o2=true; for(int i=0;i<C;++i) if(cols[pB[i]]&(1<<Acv[i])){o2=false;break;} if(!o2)continue;
            out=cols; for(int i=0;i<C;++i){ out[pT[i]]|=1<<Av[i]; out[pB[i]]|=1<<Acv[i]; } return true;
        }
    }
    return false;
}
// all placements (for representative discovery of DISTINCT targets)
static std::vector<std::vector<int>> allPlacements(const std::vector<int>& cols,const std::vector<int>& Av,const std::vector<int>& Acv){
    std::vector<std::vector<int>> out;
    for(auto&pT:CP){ bool ok=true; for(int i=0;i<C;++i) if(cols[pT[i]]&(1<<Av[i])){ok=false;break;} if(!ok)continue;
        for(auto&pB:CP){ bool o2=true; for(int i=0;i<C;++i) if(cols[pB[i]]&(1<<Acv[i])){o2=false;break;} if(!o2)continue;
            std::vector<int> nc=cols; for(int i=0;i<C;++i){ nc[pT[i]]|=1<<Av[i]; nc[pB[i]]|=1<<Acv[i]; } out.push_back(std::move(nc));
        }
    }
    return out;
}

int main(int argc,char**argv){
    C=(argc>1)?atoi(argv[1]):3; M=2*C;
    bool dump=(argc>2 && std::string(argv[2])=="dump");
    for(int m=0;m<(1<<M);++m) if(__builtin_popcount(m)==C) Askel.push_back(m);
    { std::vector<int> p(C); for(int i=0;i<C;++i)p[i]=i; do CP.push_back(p); while(std::next_permutation(p.begin(),p.end())); }
    auto t0=std::chrono::steady_clock::now();

    std::vector<int> empty(C,0);
    std::map<std::string,u128> states;
    std::map<std::string,std::pair<std::vector<int>,std::vector<int>>> rep;
    { std::string k=canonXY(empty,empty); states[k]=1; rep[k]={empty,empty}; }

    for(int band=0; band<C; ++band){
        std::map<std::string,u128> nx;
        std::map<std::string,std::pair<std::vector<int>,std::vector<int>>> nr;
        for(auto&kv:states){
            const auto& XY=rep[kv.first]; const std::vector<int>& X=XY.first; const std::vector<int>& Y=XY.second; u128 w=kv.second;
            int Tb[8][8]; for(int a=0;a<C;++a)for(int b=0;b<C;++b)Tb[a][b]=__builtin_popcount(X[a]&Y[b]);
            for(int A:Askel){
                std::vector<int> Av,Acv; for(int s=0;s<M;++s){ if(A&(1<<s))Av.push_back(s); else Acv.push_back(s); }
                auto hA=sideDist(X,Y,Av); if(hA.empty())continue;
                auto hB=sideDist(X,Y,Acv); if(hB.empty())continue;
                // WEIGHTS via convolution (fast, validated)
                for(auto&da:hA) for(auto&db:hB){
                    int T[8][8];
                    for(int a=0;a<C;++a)for(int b=0;b<C;++b)
                        T[a][b]=Tb[a][b]+(unsigned char)da.first[a*C+b]+(unsigned char)db.first[a*C+b];
                    nx[canonT(T)] += w * da.second * db.second;
                }
                // REPRESENTATIVES: capture one (X',Y') per distinct target T' (only if still missing)
                if(band+1<C){
                    auto Xp=allPlacements(X,Av,Acv);
                    auto Yp=allPlacements(Y,Av,Acv);
                    for(auto&a:Xp){ for(auto&b:Yp){
                        int T[8][8]; for(int x=0;x<C;++x)for(int y=0;y<C;++y)T[x][y]=__builtin_popcount(a[x]&b[y]);
                        std::string k=canonT(T);
                        if(!nr.count(k)) nr[k]={a,b};
                    } if(nr.size()==nx.size()) break; }
                }
            }
        }
        states=std::move(nx); rep=std::move(nr);
        double el=std::chrono::duration<double>(std::chrono::steady_clock::now()-t0).count();
        std::fprintf(stderr,"band %d: states=%zu elapsed=%.1fs\n",band,states.size(),el); std::fflush(stderr);
        if(dump){ for(auto&kv:states){ std::fprintf(stdout,"  band%d ",band); for(char c:kv.first)std::fprintf(stdout,"%d",(int)(unsigned char)c);
            std::string s; u128 x=kv.second; if(!x)s="0"; while(x){s=char('0'+(int)(x%10))+s;x/=10;} std::fprintf(stdout," -> %s\n",s.c_str()); } std::fflush(stdout);}
    }
    std::vector<int> full(C,(1<<M)-1); std::string fk=canonXY(full,full);
    u128 N=states.count(fk)?states[fk]:0;
    std::string s; if(!N)s="0"; { u128 x=N; while(x){s=char('0'+(int)(x%10))+s;x/=10;} }
    std::printf("C=%d: N=%s\n",C,s.c_str());
    const char*e=(C==2)?"288":(C==3)?"28200960":(C==4)?"29136487207403520":(C==5)?"1903816047972624930994913280000":nullptr;
    if(e)std::printf("  expect %s [%s]\n",e,s==e?"OK":"BAD");
    return 0;
}
