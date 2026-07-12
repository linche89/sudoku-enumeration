// wl_canon.cpp — Weisfeiler-Leman refined canonicalisation for the joint state,
// VALIDATED against the slow full-S_{2C} canon (the oracle).
//
// State = 2C column-masks: [0,C)=X stack columns, [C,2C)=Y stack columns.
// Symmetry group: relabel 2C symbols (S_{2C}) x permute X-cols (S_C) x permute
// Y-cols (S_C).  The slow canon brute-forces all (2C)! symbol perms; the WL
// canon refines symbols & columns by incidence colour until stable, then only
// brute-forces symbol perms WITHIN equal final colour classes (and columns are
// handled by sorting).  If WL fully discriminates, the within-class perms are
// tiny -> fast.
//
// This file's main() is a DIFFERENTIAL TESTER: generate many random reachable-ish
// states and assert wl_canon == slow_canon for every one.  Only if it passes do
// we trust WL.
#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <vector>
#include <map>
#include <algorithm>
#include <string>
#include <random>

static int C, M;

// ---------- slow oracle: full S_{2C} canon ----------
static std::string slow_canon(const std::vector<int>& st){
    std::vector<int> p(M); for(int i=0;i<M;++i)p[i]=i;
    std::string best;
    do{
        std::vector<int> X(C),Y(C);
        for(int c=0;c<C;++c){int mx=0,my=0;
            for(int s=st[c];s;s&=s-1)mx|=1<<p[__builtin_ctz(s)];
            for(int s=st[C+c];s;s&=s-1)my|=1<<p[__builtin_ctz(s)];X[c]=mx;Y[c]=my;}
        std::sort(X.begin(),X.end());std::sort(Y.begin(),Y.end());
        std::string k;for(int v:X){k+=(char)(v&255);k+=(char)((v>>8)&255);}k+='|';
        for(int v:Y){k+=(char)(v&255);k+=(char)((v>>8)&255);}
        if(best.empty()||k<best)best=k;
    }while(std::next_permutation(p.begin(),p.end()));
    return best;
}

// ---------- WL refinement ----------
// symbol colours and column colours, refined together by incidence.
static std::string wl_canon(const std::vector<int>& st){
    // columns 0..C-1 = X, C..2C-1 = Y. incidence: symbol s in column j.
    // initial symbol colour: (degX,degY); initial column colour: (stack, popcount)
    std::vector<long long> symCol(M), colCol(2*C);
    for(int s=0;s<M;++s){ int dx=0,dy=0;
        for(int c=0;c<C;++c){ if(st[c]&(1<<s))dx++; if(st[C+c]&(1<<s))dy++; }
        symCol[s]=(long long)dx*1000+dy; }
    for(int j=0;j<2*C;++j){ int stack=(j<C)?0:1; colCol[j]=(long long)stack*100000+__builtin_popcount(st[j]); }
    // refine to fixpoint
    for(int iter=0; iter<2*M+4; ++iter){
        // new symbol colour = (old, sorted multiset of colours of columns containing s)
        std::vector<std::pair<long long,std::vector<long long>>> sk(M);
        for(int s=0;s<M;++s){ std::vector<long long> nb;
            for(int j=0;j<2*C;++j) if(st[j]&(1<<s)) nb.push_back(colCol[j]);
            std::sort(nb.begin(),nb.end()); sk[s]={symCol[s],nb}; }
        // new column colour = (old, sorted multiset of colours of symbols in column j)
        std::vector<std::pair<long long,std::vector<long long>>> ck(2*C);
        for(int j=0;j<2*C;++j){ std::vector<long long> nb;
            for(int s=0;s<M;++s) if(st[j]&(1<<s)) nb.push_back(symCol[s]);
            std::sort(nb.begin(),nb.end()); ck[j]={colCol[j],nb}; }
        // recolour by rank
        std::map<std::pair<long long,std::vector<long long>>,long long> rs, rc;
        for(int s=0;s<M;++s) rs[sk[s]];
        for(int j=0;j<2*C;++j) rc[ck[j]];
        long long id=0; for(auto&kv:rs) kv.second=id++;
        id=0; for(auto&kv:rc) kv.second=id++;
        std::vector<long long> ns(M),nc(2*C);
        for(int s=0;s<M;++s) ns[s]=rs[sk[s]];
        for(int j=0;j<2*C;++j) nc[j]=rc[ck[j]];
        bool same = (ns==symCol && nc==colCol);
        symCol=ns; colCol=nc;
        if(same) break;
    }
    // group symbols by final colour; brute-force perms only WITHIN groups
    std::map<long long,std::vector<int>> grp;
    for(int s=0;s<M;++s) grp[symCol[s]].push_back(s);
    std::vector<std::pair<long long,std::vector<int>>> g(grp.begin(),grp.end());
    int NG=(int)g.size(); std::vector<int> base(NG);
    { int a=0; for(int b=0;b<NG;++b){ base[b]=a; a+=(int)g[b].second.size(); } }
    std::vector<std::vector<std::vector<int>>> gp(NG);
    for(int b=0;b<NG;++b){ auto v=g[b].second; std::sort(v.begin(),v.end());
        do gp[b].push_back(v); while(std::next_permutation(v.begin(),v.end())); }
    std::vector<int> idx(NG,0); std::string best;
    for(;;){
        int perm[16];
        for(int b=0;b<NG;++b){ auto&P=gp[b][idx[b]]; for(int j=0;j<(int)P.size();++j) perm[P[j]]=base[b]+j; }
        std::vector<int> X(C),Y(C);
        for(int c=0;c<C;++c){int mx=0,my=0;
            for(int s=st[c];s;s&=s-1)mx|=1<<perm[__builtin_ctz(s)];
            for(int s=st[C+c];s;s&=s-1)my|=1<<perm[__builtin_ctz(s)];X[c]=mx;Y[c]=my;}
        std::sort(X.begin(),X.end());std::sort(Y.begin(),Y.end());
        std::string k;for(int v:X){k+=(char)(v&255);k+=(char)((v>>8)&255);}k+='|';
        for(int v:Y){k+=(char)(v&255);k+=(char)((v>>8)&255);}
        if(best.empty()||k<best)best=k;
        int b=0; for(;b<NG;++b){ if(++idx[b]<(int)gp[b].size())break; idx[b]=0; }
        if(b==NG)break;
    }
    return best;
}

// build a random VALID-ish joint state: each column a subset; X-cols and Y-cols
// each have the same total fill (b symbols per column for some band count). For
// the differential test we only need structural validity that the canon handles:
// any 2C masks. We'll also test genuine reachable states by simulating bands.
int main(int argc,char**argv){
    C=(argc>1)?atoi(argv[1]):3; M=2*C;
    std::mt19937 rng(12345);
    int trials=(argc>2)?atoi(argv[2]):3000;
    int fails=0, maxgrp=0;
    for(int t=0;t<trials;++t){
        // random fill level b in 0..C; each column gets b distinct symbols
        int b=rng()% (C+1);
        std::vector<int> st(2*C,0);
        for(int j=0;j<2*C;++j){ std::vector<int> syms(M); for(int i=0;i<M;++i)syms[i]=i;
            std::shuffle(syms.begin(),syms.end(),rng);
            for(int i=0;i<b;++i) st[j]|=1<<syms[i]; }
        std::string a=slow_canon(st), c=wl_canon(st);
        if(a!=c){ if(fails<5) std::fprintf(stderr,"MISMATCH at trial %d (b=%d)\n",t,b); ++fails; }
        // track max within-group size (proxy for WL speed)
        // (recompute grouping cheaply omitted)
    }
    std::printf("C=%d trials=%d  WL==slow mismatches=%d  [%s]\n",
                C,trials,fails, fails==0?"WL EXACT":"WL INSUFFICIENT");
    return fails?1:0;
}
