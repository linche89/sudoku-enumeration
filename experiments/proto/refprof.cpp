// Profile refine-canon on REAL DP states: what is the residual within-class search size
// (product of colour-class factorials, per side) after refinement?  If ~1 (discrete), canon
// is O(M*C) and fast; if large, that's the optimization target.
// We generate real depth-d states by running the actual transition a few bands (reuse the
// engine's reachable states).  Simpler proxy: sample real states from band-1 of C=4/C=5 by
// brute single-stack + alignment, then refine and report residual.
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <array>
#include <vector>
#include <algorithm>
#include <map>
#include <random>
static int C,M,FULLC;
static std::vector<std::array<int,8>> CP; static std::vector<std::vector<int>> permMask;
static const int MAXM=12; using Key=std::array<uint16_t,MAXM>;
static void initPerms(){std::vector<int>p(C);for(int i=0;i<C;++i)p[i]=i;do{std::array<int,8>a{};for(int i=0;i<C;++i)a[i]=p[i];CP.push_back(a);}while(std::next_permutation(p.begin(),p.end()));permMask.assign(CP.size(),std::vector<int>(1<<C,0));for(size_t pi=0;pi<CP.size();++pi)for(int m=0;m<(1<<C);++m){int r=0;for(int b=m;b;b&=b-1){int i=__builtin_ctz(b);r|=1<<CP[pi][i];}permMask[pi][m]=r;}}
static void refine(const uint16_t*pr,int xcol[8],int ycol[8]){
    for(int c=0;c<C;++c){xcol[c]=0;ycol[c]=0;}
    for(int it=0;it<2*C+3;++it){
        std::vector<std::vector<long long>> xs(C),ys(C);
        for(int c=0;c<C;++c){xs[c].push_back(xcol[c]);ys[c].push_back(ycol[c]);}
        std::vector<std::vector<long long>> xa(C),ya(C);
        for(int i=0;i<M;++i){int xm=pr[i]&FULLC,ym=(pr[i]>>C)&FULLC;
            long long yh[8]={0};for(int b=ym;b;b&=b-1)yh[ycol[__builtin_ctz(b)]]++;
            long long xh[8]={0};for(int b=xm;b;b&=b-1)xh[xcol[__builtin_ctz(b)]]++;
            long long yco=0;for(int q=0;q<C;++q)yco=yco*16+yh[q];
            long long xco=0;for(int q=0;q<C;++q)xco=xco*16+xh[q];
            for(int b=xm;b;b&=b-1)xa[__builtin_ctz(b)].push_back(yco);
            for(int b=ym;b;b&=b-1)ya[__builtin_ctz(b)].push_back(xco);}
        for(int c=0;c<C;++c){std::sort(xa[c].begin(),xa[c].end());for(auto v:xa[c])xs[c].push_back(v);std::sort(ya[c].begin(),ya[c].end());for(auto v:ya[c])ys[c].push_back(v);}
        auto rec=[&](std::vector<std::vector<long long>>&s,int*col)->bool{std::vector<int>o(C);for(int c=0;c<C;++c)o[c]=c;std::sort(o.begin(),o.end(),[&](int a,int b){return s[a]<s[b];});int nc[8],g=0;nc[o[0]]=0;for(int i=1;i<C;++i){if(s[o[i]]!=s[o[i-1]])++g;nc[o[i]]=g;}bool ch=false;for(int c=0;c<C;++c){if(col[c]!=nc[c])ch=true;col[c]=nc[c];}return ch;};
        bool c1=rec(xs,xcol),c2=rec(ys,ycol);if(!c1&&!c2)break;
    }
}
static long long residual(const int*col){int cnt[8]={0};for(int c=0;c<C;++c)cnt[col[c]]++;long long r=1;for(int g=0;g<C;++g){for(int k=2;k<=cnt[g];++k)r*=k;}return r;}
// generate a real reachable depth-d single-stack-pair state by greedy band placement on X and random relabel on Y
int main(int argc,char**argv){
    C=atoi(argv[1]);M=2*C;FULLC=(1<<C)-1;int depth=argc>2?atoi(argv[2]):2;int n=argc>3?atoi(argv[3]):3000;initPerms();
    std::mt19937 rng(7);
    // build reachable state: X side = place `depth` bands greedily into C cols; symbol s in band b
    // gets some col. We'll just simulate: each of M symbols gets `depth` distinct X-cols and `depth`
    // distinct Y-cols, column-balanced (each col holds 2*depth symbols). Random balanced assignment.
    std::map<long long,long long> hist; long long tot=0,maxr=1; int samples=0;
    for(int t=0;t<n;++t){
        // random column-balanced: each col gets exactly 2*depth symbols on X and on Y
        std::vector<int> xcnt(C,0),ycnt(C,0); uint16_t pr[MAXM]; bool okgen=true;
        for(int i=0;i<M;++i){pr[i]=0;}
        // assign each symbol depth distinct X-cols respecting capacity 2*depth
        auto assign=[&](int side)->bool{std::vector<int> cap(C,2*depth);for(int i=0;i<M;++i){int got=0,mask=0,tries=0;while(got<depth){int c=rng()%C;if(!(mask&(1<<c))&&cap[c]>0){mask|=1<<c;cap[c]--;got++;}if(++tries>1000)return false;}if(side==0)pr[i]|=mask;else pr[i]|=mask<<C;}return true;};
        if(!assign(0)||!assign(1)){continue;}
        int xc[8],yc[8];refine(pr,xc,yc);long long r=residual(xc)*residual(yc);
        hist[r]++;tot+=r;if(r>maxr)maxr=r;samples++;
    }
    printf("C=%d depth=%d samples=%d: avg residual perms/canon=%.1f  max=%lld  (brute would be (C!)^2=%lld)\n",
        C,depth,samples,(double)tot/std::max(1,samples),maxr,(long long)CP.size()*CP.size());
    printf("  residual distribution: ");for(auto&kv:hist)printf("%lld:%lld ",kv.first,kv.second);printf("\n");
    return 0;
}
