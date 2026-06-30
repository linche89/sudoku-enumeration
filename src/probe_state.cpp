// probe_state.cpp — determine a SUFFICIENT compact state descriptor for the 2xC
// band DP by computing exact completion counts of every reachable (canonical)
// state and checking which descriptors separate them.
//
// We run the correct (full) DP forward to enumerate reachable canonical states at
// each depth, AND compute each state's completion count (number of ways to finish).
// Then we test candidate compact descriptors: do all states sharing a descriptor
// value share the same completion count?  If yes for the chosen descriptor, the DP
// can use that descriptor as its state -> polynomial.
#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <vector>
#include <map>
#include <set>
#include <algorithm>
#include <functional>
#include <string>
using u128=unsigned __int128;
static int C,M,FULL;

// canonical key (same as dp2xC)
static void relabelStack(const int* cols,int lo,const int* map,int* out){
    for(int j=0;j<C;++j){int m=0;for(int s=cols[lo+j];s;s&=s-1)m|=1<<map[__builtin_ctz(s)];out[j]=m;}
    std::sort(out,out+C);
}
static u128 canonKey(const int* colsIn){
    u128 best=~(u128)0; int cols[24];
    for(int sw=0;sw<2;++sw){
        for(int j=0;j<C;++j) cols[j]=colsIn[sw?C+j:j];
        for(int j=0;j<C;++j) cols[C+j]=colsIn[sw?j:C+j];
        int d0[24]={0},d1[24]={0};
        for(int j=0;j<C;++j) for(int s=cols[j];s;s&=s-1) d0[__builtin_ctz(s)]++;
        for(int j=0;j<C;++j) for(int s=cols[C+j];s;s&=s-1) d1[__builtin_ctz(s)]++;
        std::vector<int> ord(M); for(int i=0;i<M;++i)ord[i]=i;
        std::stable_sort(ord.begin(),ord.end(),[&](int a,int b){return d0[a]!=d0[b]?d0[a]<d0[b]:d1[a]<d1[b];});
        std::vector<std::pair<int,int>> cls;
        for(int i=0;i<M;){int jx=i;while(jx<M&&d0[ord[jx]]==d0[ord[i]]&&d1[ord[jx]]==d1[ord[i]])++jx;cls.push_back({i,jx});i=jx;}
        int map[24];
        std::function<void(size_t)> rec=[&](size_t ci){
            if(ci==cls.size()){
                for(int l=0;l<M;++l) map[ord[l]]=l;
                int o0[12],o1[12]; relabelStack(cols,0,map,o0); relabelStack(cols,C,map,o1);
                u128 k=0; for(int j=0;j<C;++j)k=(k<<M)|(unsigned)o0[j]; for(int j=0;j<C;++j)k=(k<<M)|(unsigned)o1[j];
                if(k<best)best=k; return;
            }
            int lo=cls[ci].first,hi=cls[ci].second;
            std::sort(ord.begin()+lo,ord.begin()+hi);
            do{rec(ci+1);}while(std::next_permutation(ord.begin()+lo,ord.begin()+hi));
        };
        rec(0);
    }
    return best;
}

// full band enumeration transition
static std::vector<int> subs;
static void unpack(u128 key,int* cols){ for(int j=2*C-1;j>=0;--j){cols[j]=(int)(key&FULL);key>>=M;} }

// descriptor candidate: multiset of (within-stack column-pair overlaps) +
// (cross-stack column-pair overlaps), as sorted vectors, plus per-column popcount.
static std::string descriptor(const int* cols){
    // pairwise overlaps split by same-stack vs cross-stack
    std::vector<int> same, cross;
    for(int i=0;i<M;++i)for(int j=i+1;j<M;++j){
        int ov=__builtin_popcount(cols[i]&cols[j]);
        bool sameStack=((i<C)==(j<C));
        if(sameStack) same.push_back(ov); else cross.push_back(ov);
    }
    std::sort(same.begin(),same.end()); std::sort(cross.begin(),cross.end());
    std::string s="S"; for(int x:same)s+=('0'+x); s+="|X"; for(int x:cross)s+=('0'+x);
    return s;
}

// completion count via canonical-memoised DP
static std::map<u128,u128> compMemo;
static std::vector<u128> allbandtargets; // not used; we enumerate live
static u128 completion(u128 key,int kleft){
    if(kleft==0){ int cols[24]; unpack(key,cols); for(int j=0;j<M;++j) if(cols[j]!=FULL) return 0; return 1; }
    auto it=compMemo.find(key); // memo keyed by (key) — but kleft determined by popcount, so ok
    if(it!=compMemo.end()) return it->second;
    int cols[24]; unpack(key,cols);
    u128 tot=0;
    // enumerate bands
    std::function<void(int,int,int,int,int,int*)> dummy;
    // place via recursion like dp2xC
    int work[24];
    std::function<void(int,int,int,int,int)> place1;
    std::function<void(int,int,int,int,int)> place0;
    // closures capturing work[] etc.
    int remAtop,remCbot,remCtop,remAbot;
    std::function<void(int)> p1=[&](int j){
        if(j==C){ u128 ck=canonKey(work); tot+=completion(ck,kleft-1); return; }
        int col=C+j; int avT=remCtop&~work[col];
        for(int t=avT;t;t&=t-1){int tb=t&-t; int avB=remAbot&~work[col]&~tb;
            for(int b=avB;b;b&=b-1){int bb=b&-b; int sv=work[col]; work[col]|=tb|bb;
                int sa=remCtop,sc=remAbot; remCtop&=~tb; remAbot&=~bb; p1(j+1);
                remCtop=sa; remAbot=sc; work[col]=sv;}}
    };
    std::function<void(int)> p0=[&](int j){
        if(j==C){ p1(0); return; }
        int col=j; int avT=remAtop&~work[col];
        for(int t=avT;t;t&=t-1){int tb=t&-t; int avB=remCbot&~work[col]&~tb;
            for(int b=avB;b;b&=b-1){int bb=b&-b; int sv=work[col]; work[col]|=tb|bb;
                int sa=remAtop,sc=remCbot; remAtop&=~tb; remCbot&=~bb; p0(j+1);
                remAtop=sa; remCbot=sc; work[col]=sv;}}
    };
    for(int A:subs){int comp=FULL^A; for(int j=0;j<M;++j)work[j]=cols[j];
        remAtop=A;remCbot=comp;remCtop=comp;remAbot=A; p0(0);}
    compMemo[key]=tot; return tot;
}

int main(int argc,char**argv){
    C=(argc>1)?atoi(argv[1]):3; M=2*C; FULL=(1<<M)-1;
    for(int m=0;m<=FULL;++m) if(__builtin_popcount(m)==C) subs.push_back(m);
    // enumerate reachable canonical states at each depth via forward DP
    std::vector<std::set<u128>> layers(C+1);
    { int cols[24]={0}; layers[0].insert(canonKey(cols)); }
    // forward expand
    for(int d=0; d<C; ++d){
        for(u128 key: layers[d]){
            int cols[24]; unpack(key,cols);
            int work[24]; int remAtop,remCbot,remCtop,remAbot;
            std::function<void(int)> p1=[&](int j){
                if(j==C){ layers[d+1].insert(canonKey(work)); return; }
                int col=C+j; int avT=remCtop&~work[col];
                for(int t=avT;t;t&=t-1){int tb=t&-t; int avB=remAbot&~work[col]&~tb;
                    for(int b=avB;b;b&=b-1){int bb=b&-b; int sv=work[col]; work[col]|=tb|bb;
                        int sa=remCtop,sc=remAbot; remCtop&=~tb; remAbot&=~bb; p1(j+1);
                        remCtop=sa; remAbot=sc; work[col]=sv;}}
            };
            std::function<void(int)> p0=[&](int j){
                if(j==C){ p1(0); return; }
                int col=j; int avT=remAtop&~work[col];
                for(int t=avT;t;t&=t-1){int tb=t&-t; int avB=remCbot&~work[col]&~tb;
                    for(int b=avB;b;b&=b-1){int bb=b&-b; int sv=work[col]; work[col]|=tb|bb;
                        int sa=remAtop,sc=remCbot; remAtop&=~tb; remCbot&=~bb; p0(j+1);
                        remAtop=sa; remCbot=sc; work[col]=sv;}}
            };
            for(int A:subs){int comp=FULL^A; for(int j=0;j<M;++j)work[j]=cols[j];
                remAtop=A;remCbot=comp;remCtop=comp;remAbot=A; p0(0);}
        }
        std::fprintf(stderr,"layer %d: %zu canonical states\n", d+1, layers[d+1].size());
    }
    // For each layer, compute completion count of each state and group by descriptor
    for(int d=1; d<C; ++d){
        std::map<std::string, std::set<std::string>> desc2comp;
        for(u128 key: layers[d]){
            int cols[24]; unpack(key,cols);
            u128 cc=completion(key, C-d);
            std::string cstr; { u128 x=cc; if(!x)cstr="0"; while(x){cstr=char('0'+(int)(x%10))+cstr;x/=10;} }
            desc2comp[descriptor(cols)].insert(cstr);
        }
        int bad=0; for(auto&kv:desc2comp) if(kv.second.size()>1) ++bad;
        std::fprintf(stderr,"layer %d: %zu states, %zu descriptor-classes, %d ambiguous\n",
                     d, layers[d].size(), desc2comp.size(), bad);
        if(bad){ for(auto&kv:desc2comp) if(kv.second.size()>1){ std::fprintf(stderr,"  AMBIG %s ->",kv.first.c_str()); for(auto&s:kv.second)std::fprintf(stderr," %s",s.c_str()); std::fprintf(stderr,"\n"); } }
    }
    return 0;
}
