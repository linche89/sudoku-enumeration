// canon_refine2.cpp — refinement-based canonical KEY (not lex-min) for the (xmask,ymask)
// multiset state.  KEY INSIGHT: nx[] only needs a consistent orbit-invariant key; it need
// NOT be the lexicographic minimum.  Equitable-partition refinement gives such a key in
// O(M*C) when it discretises the column partitions; for the residual (non-discrete) cases
// we individualise within the small automorphism group and take the min over that residual
// only.  Result is a complete, exact canonical form -- just a DIFFERENT one than brute lex-min.
//
// VALIDATION (the correct test): a canonical KEY must be INVARIANT under relabeling.  So for
// random state P and random (sigma,tau) in S_C x S_C, canonKey(P) must EQUAL canonKey(relabel(P)).
// We also check it SEPARATES non-equivalent states as well as brute does (same #distinct keys
// over a sample, i.e. no over-merging) by comparing the induced partition to brute's.
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <array>
#include <vector>
#include <algorithm>
#include <random>
#include <chrono>
#include <map>

static int C,M,FULLC;
static std::vector<std::array<int,8>> CP; static std::vector<std::vector<int>> permMask;
static const int MAXM=12; using Key=std::array<uint16_t,MAXM>;
static void initPerms(){CP.clear();permMask.clear();std::vector<int>p(C);for(int i=0;i<C;++i)p[i]=i;do{std::array<int,8>a{};for(int i=0;i<C;++i)a[i]=p[i];CP.push_back(a);}while(std::next_permutation(p.begin(),p.end()));permMask.assign(CP.size(),std::vector<int>(1<<C,0));for(size_t pi=0;pi<CP.size();++pi)for(int m=0;m<(1<<C);++m){int r=0;for(int b=m;b;b&=b-1){int i=__builtin_ctz(b);r|=1<<CP[pi][i];}permMask[pi][m]=r;}}
static inline void isort(uint16_t*a,int n){for(int i=1;i<n;++i){uint16_t v=a[i];int j=i-1;while(j>=0&&a[j]>v){a[j+1]=a[j];--j;}a[j+1]=v;}}
static int cmpArr(const uint16_t*a,const uint16_t*b,int n){for(int i=0;i<n;++i)if(a[i]!=b[i])return a[i]<b[i]?-1:1;return 0;}

static Key canonBrute(const uint16_t*pr){uint16_t bst[MAXM];bool h=false;uint16_t c[MAXM];for(size_t rp=0;rp<CP.size();++rp){const int*px=permMask[rp].data();for(size_t cp=0;cp<CP.size();++cp){const int*py=permMask[cp].data();for(int i=0;i<M;++i){int t=pr[i];c[i]=(uint16_t)(px[t&FULLC]|(py[(t>>C)&FULLC]<<C));}isort(c,M);if(!h||cmpArr(c,bst,M)<0){memcpy(bst,c,M*2);h=true;}}}Key k{};for(int i=0;i<M;++i)k[i]=bst[i];return k;}

// ---- exact equitable refinement of X-col and Y-col partitions ----
// returns #X-colours and #Y-colours; fills xcol[],ycol[] with colour ids in an
// ORDER-INVARIANT way (colour id = rank of the column's signature among signatures).
static void refine(const uint16_t*pr,int xcol[8],int ycol[8],int&nxc,int&nyc){
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
    int xs2=0,ys2=0;for(int c=0;c<C;++c){xs2|=1<<xcol[c];ys2|=1<<ycol[c];}
    nxc=__builtin_popcount(xs2);nyc=__builtin_popcount(ys2);
}

// canonical KEY: refine; then take the MIN packed-sorted over the RESIDUAL symmetry only
// (permutations within equal-colour classes, on both sides).  When discrete, residual = identity
// -> single candidate, O(M*C).  This is an orbit-invariant key (NOT lex-min over full S_C x S_C,
// but consistent: equivalent inputs -> identical refined colours -> identical residual search).
static void withinClassPerms(const int*col,int ncol,std::vector<std::array<int,8>>&out){
    std::vector<std::vector<int>> mem(ncol); for(int c=0;c<C;++c)mem[col[c]].push_back(c);
    // base positions: colour classes in colour-id order, contiguous
    int base[8],acc=0; for(int g=0;g<ncol;++g){base[g]=acc;acc+=mem[g].size();}
    std::vector<std::vector<std::array<int,8>>> gp(ncol);
    for(int g=0;g<ncol;++g){std::vector<int>v=mem[g];std::sort(v.begin(),v.end());do{std::array<int,8>a{};for(size_t i=0;i<v.size();++i)a[i]=v[i];gp[g].push_back(a);}while(std::next_permutation(v.begin(),v.end()));}
    std::vector<int> idx(ncol,0); out.clear();
    for(;;){std::array<int,8>perm{};for(int g=0;g<ncol;++g){const auto&a=gp[g][idx[g]];for(size_t j=0;j<mem[g].size();++j)perm[a[j]]=base[g]+j;}out.push_back(perm);int g=0;for(;g<ncol;++g){if(++idx[g]<(int)gp[g].size())break;idx[g]=0;}if(g>=ncol)break;}
}
static Key canonRefine(const uint16_t*pr){
    int xcol[8],ycol[8],nxc,nyc; refine(pr,xcol,ycol,nxc,nyc);
    std::vector<std::array<int,8>> xp,yp; withinClassPerms(xcol,nxc,xp); withinClassPerms(ycol,nyc,yp);
    uint16_t xms[MAXM],yms[MAXM]; for(int i=0;i<M;++i){xms[i]=pr[i]&FULLC;yms[i]=(pr[i]>>C)&FULLC;}
    uint16_t bst[MAXM];bool h=false;uint16_t cand[MAXM];
    for(const auto&xpp:xp){uint16_t xr[MAXM];for(int i=0;i<M;++i){int r=0;for(int b=xms[i];b;b&=b-1)r|=1<<xpp[__builtin_ctz(b)];xr[i]=r;}
        for(const auto&ypp:yp){for(int i=0;i<M;++i){int r=0;for(int b=yms[i];b;b&=b-1)r|=1<<ypp[__builtin_ctz(b)];cand[i]=(uint16_t)(xr[i]|(r<<C));}isort(cand,M);if(!h||cmpArr(cand,bst,M)<0){memcpy(bst,cand,M*2);h=true;}}}
    Key k{};for(int i=0;i<M;++i)k[i]=bst[i];return k;
}

static void randomState(std::mt19937&rng,int d,uint16_t*o){auto rm=[&](int pc){int m=0,c=0;while(c<pc){int b=1<<(rng()%C);if(!(m&b)){m|=b;c++;}}return m;};for(int i=0;i<M;++i)o[i]=(uint16_t)(rm(d)|(rm(d)<<C));}
static void relabel(const uint16_t*in,const int*sx,const int*sy,uint16_t*out){for(int i=0;i<M;++i){int xm=in[i]&FULLC,ym=(in[i]>>C)&FULLC,nx=0,ny=0;for(int b=xm;b;b&=b-1)nx|=1<<sx[__builtin_ctz(b)];for(int b=ym;b;b&=b-1)ny|=1<<sy[__builtin_ctz(b)];out[i]=(uint16_t)(nx|(ny<<C));}}

int main(int argc,char**argv){
    std::string mode=argc>1?argv[1]:"invariance"; C=argc>2?atoi(argv[2]):6; M=2*C; FULLC=(1<<C)-1; int n=argc>3?atoi(argv[3]):20000; initPerms();
    if(mode=="invariance"){
        // CORRECT TEST: canonRefine must give the SAME key to a state and its relabelings.
        std::mt19937 rng(1234); int bad=0;
        for(int t=0;t<n;++t){int d=1+(rng()%C);uint16_t pr[MAXM];randomState(rng,d,pr);
            Key k0=canonRefine(pr);
            for(int r=0;r<3;++r){int sx[8],sy[8];for(int i=0;i<C;++i){sx[i]=i;sy[i]=i;}std::shuffle(sx,sx+C,rng);std::shuffle(sy,sy+C,rng);
                uint16_t pr2[MAXM];relabel(pr,sx,sy,pr2);Key k1=canonRefine(pr2);
                if(k0!=k1){bad++;if(bad<=3){printf("INVARIANCE FAIL t=%d\n",t);}break;}}}
        printf("invariance C=%d n=%d: violations=%d [%s]\n",C,n,bad,bad?"FAIL":"OK");
        return bad?1:0;
    }
    if(mode=="separation"){
        // canonRefine must SEPARATE orbits exactly like brute: same #distinct keys over a sample,
        // and same equivalence classes.  Compare partition induced by canonRefine vs canonBrute.
        std::mt19937 rng(55); std::map<Key,int> rk,bk; std::map<Key,Key> r2b; int conflict=0;
        for(int t=0;t<n;++t){int d=1+(rng()%C);uint16_t pr[MAXM];randomState(rng,d,pr);
            Key kr=canonRefine(pr),kb=canonBrute(pr);
            auto it=r2b.find(kr);
            if(it==r2b.end())r2b[kr]=kb; else if(!(it->second==kb))conflict++;  // refine merged two brute-distinct
            rk[kr]=1;bk[kb]=1;}
        printf("separation C=%d n=%d: refineKeys=%zu bruteKeys=%zu over-merge-conflicts=%d [%s]\n",
            C,n,rk.size(),bk.size(),conflict,(rk.size()==bk.size()&&conflict==0)?"OK":"FAIL");
        return (rk.size()==bk.size()&&conflict==0)?0:1;
    }
    if(mode=="bench"){
        std::mt19937 rng(5);int reps=n;uint16_t prs[2000][MAXM];int np=std::min(reps,2000);
        for(int i=0;i<np;++i){int d=1+(rng()%C);randomState(rng,d,prs[i]);}
        volatile uint64_t acc=0;auto t0=std::chrono::steady_clock::now();
        for(int r=0;r<reps;++r){Key k=canonRefine(prs[r%np]);acc+=k[0];}
        auto t1=std::chrono::steady_clock::now();double rf=std::chrono::duration<double>(t1-t0).count();
        printf("bench C=%d reps=%d: refine=%.3fs (%.2fus/call)\n",C,reps,rf,1e6*rf/reps);
        return 0;
    }
    return 0;
}
