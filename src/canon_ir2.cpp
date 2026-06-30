// canon_ir2.cpp — exact fast canon via incremental column assignment with prefix pruning.
// Goal: replace the (C!)^2 brute canonMS (degenerate under uniform popcount) for C=6.
//
// Problem: min over sigma,tau in S_C of sort([ sigma(xmask_i) | tau(ymask_i)<<C ]).
//
// APPROACH (exact, no false ordering assumption): branch-and-bound over the X-permutation
// sigma built incrementally column-by-column.  Partial sigma fixes where some X-columns map.
// LOWER BOUND given a partial sigma: for each pair, its xmask bits that are ALREADY assigned
// contribute known low bits; unassigned xmask bits can (optimistically) go to the smallest
// still-free positions; ymask gets its independent minimum (1<<popcount)-1.  Sort these
// optimistic pairs -> a true lower bound on any completion.  If >= best, prune.
// When sigma is complete, solve the inner tau-min exactly (a smaller (C!) search, itself
// B&B-able), compare, update best.
//
// This makes NO assumption about colour/signature ordering (the trap that sank ir v1).
// Validated by `difftest` byte-identical to brute over many random multisets at C=3,4,5.
#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <array>
#include <vector>
#include <algorithm>
#include <string>
#include <random>
#include <chrono>

static int C, M, FULLC;
static std::vector<std::array<int,8>> CP;
static std::vector<std::vector<int>> permMask;
static const int MAXM=12;
using Key=std::array<uint16_t,MAXM>;

static void initPerms(){
    CP.clear(); permMask.clear();
    std::vector<int> p(C); for(int i=0;i<C;++i)p[i]=i;
    do{ std::array<int,8> a{}; for(int i=0;i<C;++i)a[i]=p[i]; CP.push_back(a);}while(std::next_permutation(p.begin(),p.end()));
    permMask.assign(CP.size(),std::vector<int>(1<<C,0));
    for(size_t pi=0;pi<CP.size();++pi)for(int m=0;m<(1<<C);++m){int r=0;for(int b=m;b;b&=b-1){int i=__builtin_ctz(b);r|=1<<CP[pi][i];}permMask[pi][m]=r;}
}
static inline void isort(uint16_t*a,int n){for(int i=1;i<n;++i){uint16_t v=a[i];int j=i-1;while(j>=0&&a[j]>v){a[j+1]=a[j];--j;}a[j+1]=v;}}
static int cmpArr(const uint16_t*a,const uint16_t*b,int n){for(int i=0;i<n;++i)if(a[i]!=b[i])return a[i]<b[i]?-1:1;return 0;}

static Key canonBrute(const uint16_t*pairs){
    uint16_t best[MAXM];bool have=false;uint16_t cand[MAXM];
    for(size_t rp=0;rp<CP.size();++rp){const int*pmX=permMask[rp].data();
        for(size_t cp=0;cp<CP.size();++cp){const int*pmY=permMask[cp].data();
            for(int i=0;i<M;++i){int t=pairs[i];cand[i]=(uint16_t)(pmX[t&FULLC]|(pmY[(t>>C)&FULLC]<<C));}
            isort(cand,M);
            if(!have||cmpArr(cand,best,M)<0){std::memcpy(best,cand,M*2);have=true;}
        }}
    Key k{};for(int i=0;i<M;++i)k[i]=best[i];return k;
}

// inner: given a FIXED relabeled xrel[i] for every pair, find min sort over tau in S_C of
// [ xrel_i | tau(ymask_i)<<C ].  B&B over tau (Y-perm) incrementally with prefix LB.
// Simpler: just enumerate Y-perms but with the same independent-min prune as before; since
// xrel is fixed this is at most C! and usually pruned hard.  We keep it exact + brute-ish
// (C! <= 720 for C=6) — cheap relative to the sigma search.
static void innerTau(const uint16_t* xrel, const uint16_t* yms, uint16_t* best, bool& have){
    uint16_t cand[MAXM];
    for(size_t cp=0;cp<CP.size();++cp){const int*pmY=permMask[cp].data();
        for(int i=0;i<M;++i) cand[i]=(uint16_t)(xrel[i]|(pmY[yms[i]]<<C));
        isort(cand,M);
        if(!have||cmpArr(cand,best,M)<0){std::memcpy(best,cand,M*2);have=true;}
    }
}

// sigma search: assign X-columns to new positions incrementally (newpos for old col).
// state: which old cols assigned (mask), to which new positions; we choose, for new position
// being filled next, NO -- simpler: choose a permutation by deciding sigma[oldcol] for each
// old col in order 0..C-1, with B&B prefix lower bound.
static uint16_t g_best[MAXM]; static bool g_have;
static const uint16_t* g_xms; static const uint16_t* g_yms; static int g_nm;
static int g_assignedPosMask;   // which NEW positions used
static int g_sigma[8];          // sigma[oldcol] = newpos, -1 if unassigned

// lower bound on final sorted seq given current partial sigma.
static void lbAndMaybeRecurse(int oldcol){
    if(oldcol==C){
        // complete: relabel xmask, solve tau
        uint16_t xrel[MAXM];
        for(int i=0;i<g_nm;++i){ int r=0; for(int b=g_xms[i];b;b&=b-1){int c=__builtin_ctz(b); r|=1<<g_sigma[c];} xrel[i]=(uint16_t)r; }
        innerTau(xrel,g_yms,g_best,g_have);
        return;
    }
    // try assigning oldcol to each free new position
    for(int np=0;np<C;++np){
        if(g_assignedPosMask&(1<<np)) continue;
        g_sigma[oldcol]=np; g_assignedPosMask|=1<<np;
        // LB: build optimistic pairs.  For each pair: xmask bits already-assigned -> known;
        // unassigned xmask bits -> optimistically smallest free positions; ymask -> indep min.
        if(g_have){
            int freePos = (~g_assignedPosMask)&FULLC;
            uint16_t lb[MAXM];
            for(int i=0;i<g_nm;++i){
                int known=0,nun=0;
                for(int b=g_xms[i];b;b&=b-1){int c=__builtin_ctz(b); if(g_sigma[c]>=0)known|=1<<g_sigma[c]; else nun++;}
                // place nun unassigned bits into the smallest free positions
                int opt=known,fp=freePos;
                for(int t=0;t<nun;++t){ int lowest=fp&(-fp); opt|=lowest; fp^=lowest; }
                int ypc=__builtin_popcount(g_yms[i]);
                lb[i]=(uint16_t)(opt | (((1<<ypc)-1)<<C));
            }
            isort(lb,g_nm);
            if(cmpArr(lb,g_best,g_nm)>=0){ g_sigma[oldcol]=-1; g_assignedPosMask&=~(1<<np); continue; }
        }
        lbAndMaybeRecurse(oldcol+1);
        g_sigma[oldcol]=-1; g_assignedPosMask&=~(1<<np);
    }
}

static Key canonIR2(const uint16_t* pairs){
    uint16_t xms[MAXM],yms[MAXM];
    for(int i=0;i<M;++i){ xms[i]=(uint16_t)(pairs[i]&FULLC); yms[i]=(uint16_t)((pairs[i]>>C)&FULLC); }
    g_xms=xms; g_yms=yms; g_nm=M; g_have=false; g_assignedPosMask=0;
    for(int i=0;i<C;++i)g_sigma[i]=-1;
    lbAndMaybeRecurse(0);
    Key k{}; for(int i=0;i<M;++i)k[i]=g_best[i]; return k;
}

static void randomState(std::mt19937& rng,int d,uint16_t*out){
    auto rm=[&](int pc){int m=0,c=0;while(c<pc){int b=1<<(rng()%C);if(!(m&b)){m|=b;c++;}}return m;};
    for(int i=0;i<M;++i)out[i]=(uint16_t)(rm(d)|(rm(d)<<C));
}
int main(int argc,char**argv){
    std::string mode=argc>1?argv[1]:"difftest";
    C=argc>2?atoi(argv[2]):5; M=2*C; FULLC=(1<<C)-1;
    int n=argc>3?atoi(argv[3]):20000;
    initPerms();
    if(mode=="difftest"){
        std::mt19937 rng(99); int mm=0;
        for(int t=0;t<n;++t){ int d=1+(rng()%C); uint16_t pr[MAXM]; randomState(rng,d,pr);
            Key a=canonIR2(pr), b=canonBrute(pr);
            if(a!=b){ if(mm<5){std::printf("MISMATCH d=%d:",d);for(int i=0;i<M;++i)std::printf(" %d",pr[i]);std::printf("\n");} mm++; } }
        std::printf("difftest C=%d tests=%d mismatches=%d [%s]\n",C,n,mm,mm?"FAIL":"OK");
        return mm?1:0;
    }
    if(mode=="bench"){
        std::mt19937 rng(5); int reps=n;
        // time IR2 vs brute
        uint16_t prs[2000][MAXM]; int np=std::min(reps,2000);
        for(int i=0;i<np;++i){int d=1+(rng()%C);randomState(rng,d,prs[i]);}
        volatile uint64_t acc=0;
        auto t0=std::chrono::steady_clock::now();
        for(int r=0;r<reps;++r){ Key k=canonIR2(prs[r%np]); acc+=k[0]; }
        auto t1=std::chrono::steady_clock::now();
        for(int r=0;r<reps;++r){ Key k=canonBrute(prs[r%np]); acc+=k[0]; }
        auto t2=std::chrono::steady_clock::now();
        double ir=std::chrono::duration<double>(t1-t0).count(), br=std::chrono::duration<double>(t2-t1).count();
        std::printf("bench C=%d reps=%d: IR2=%.3fs (%.1fus/call)  brute=%.3fs (%.1fus/call)  speedup=%.1fx\n",
            C,reps,ir,1e6*ir/reps,br,1e6*br/reps,br/ir);
        return 0;
    }
    return 0;
}
