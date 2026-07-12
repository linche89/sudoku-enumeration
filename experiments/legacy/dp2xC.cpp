// dp2xC.cpp — direct 2xC transfer DP with symbol-symmetry-reduced states.
//
// Counts full (2C)x(2C) Sudoku grids with 2xC boxes by processing the C bands
// (each 2 rows) top to bottom.  State after k bands = the symbol/column incidence
// (each column holds 2k symbols), kept CANONICAL modulo:
//   * relabeling the 2C symbols          (count is symbol-relabel invariant)
//   * permuting the C columns in a stack  (columns in a box-stack are equivalent)
//   * swapping the two stacks             (stack symmetry)
//
// Band structure (proven): a band = a 2-colouring of symbols into A | comp (C each)
// plus, INDEPENDENTLY per stack, a bijection A->cols and a bijection comp->cols.
// In stack0 each column gets one A-symbol (row0) + one comp-symbol (row1); in
// stack1 each column gets one comp-symbol (row0) + one A-symbol (row1).  A band
// may not place a symbol already present in a column (column = global permutation).
//
// The per-band transfer operator is identical at every band, and the reachable
// canonical-state set is tiny (2, 3, 22, ... distinct states).  We therefore
// MEMOISE the transition {source canonical state -> multiset of target states with
// multiplicities} and apply it C times.  The transition is enumerated by the
// A-colouring + per-stack assignment backtracking, with conflict pruning.
//
// Gates: 2x2=288, 2x3=28200960, 2x4=29136487207403520,
//        2x5=1903816047972624930994913280000.
#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <algorithm>
#include <chrono>
#include <functional>

using u128 = unsigned __int128;
static int C, M, FULL;   // M = 2C, FULL = (1<<M)-1

static std::string p128(u128 x){ std::string s; if(!x)s="0"; while(x){s=char('0'+(int)(x%10))+s;x/=10;} return s; }

// ---------------- canonical key of a state ----------------
// state = M column masks (cols 0..C-1 stack0, C..2C-1 stack1).  Exact canonical
// modulo (S_M symbols) x (S_C x S_C cols) x (stack swap): the lexicographically
// minimal packed (sorted-columns) form over all symbol relabelings and stack swap.
//
// We make the symbol minimisation cheap by first refining symbols into colour
// classes (colour refinement / WL on the symbol-column incidence): only symbols in
// the SAME final class can be interchanged in a way that changes the key, and we
// lex-minimise by permuting WITHIN each class.  Refinement shrinks classes to size
// 1 except for genuinely symmetric symbols, keeping the within-class factorial tiny.
static void relabelStack(const int* cols, int lo, const int* map, int* out){
    for(int j=0;j<C;++j){ int m=0; for(int s=cols[lo+j]; s; s&=s-1) m|=1<<map[__builtin_ctz(s)]; out[j]=m; }
    std::sort(out, out+C);
}
static u128 canonKey(const int* colsIn){
    u128 best = ~(u128)0;
    int cols[24];
    for(int swap=0; swap<2; ++swap){
        for(int j=0;j<C;++j) cols[j]   = colsIn[ swap? C+j : j ];
        for(int j=0;j<C;++j) cols[C+j] = colsIn[ swap? j   : C+j ];

        // colour refinement on symbols
        long long symCol[24];
        for(int s=0;s<M;++s){ int d0=0,d1=0;
            for(int j=0;j<C;++j) if(cols[j]>>s&1) d0++;
            for(int j=0;j<C;++j) if(cols[C+j]>>s&1) d1++;
            symCol[s]=(long long)d0*64+d1;
        }
        for(int iter=0; iter<M+2; ++iter){
            std::vector<long long> vals(symCol,symCol+M); std::sort(vals.begin(),vals.end());
            vals.erase(std::unique(vals.begin(),vals.end()),vals.end());
            auto rank=[&](long long v){ return (int)(std::lower_bound(vals.begin(),vals.end(),v)-vals.begin()); };
            long long colCol[24];
            for(int j=0;j<M;++j){ std::vector<int> cc; for(int s=cols[j];s;s&=s-1) cc.push_back(rank(symCol[__builtin_ctz(s)])); std::sort(cc.begin(),cc.end());
                long long h=1469598103934665603LL; for(int x:cc){ h=(h^(x+1))*1099511628211LL; } colCol[j]=h; }
            long long ns[24];
            for(int s=0;s<M;++s){ std::vector<long long> cc; for(int j=0;j<M;++j) if(cols[j]>>s&1) cc.push_back(colCol[j]); std::sort(cc.begin(),cc.end());
                long long h=symCol[s]*2654435761LL+1; for(long long x:cc){ h=(h*1099511628211LL)^x; } ns[s]=h; }
            bool same=true; for(int s=0;s<M;++s) if(ns[s]!=symCol[s]) same=false;
            for(int s=0;s<M;++s) symCol[s]=ns[s];
            if(same) break;
        }
        // order symbols by refined colour, group into classes, lex-minimise within
        int order[24]; for(int i=0;i<M;++i) order[i]=i;
        std::stable_sort(order, order+M, [&](int a,int b){ return symCol[a]<symCol[b]; });
        std::vector<std::pair<int,int>> cls;
        for(int i=0;i<M;){ int jx=i; while(jx<M && symCol[order[jx]]==symCol[order[i]]) ++jx; cls.push_back({i,jx}); i=jx; }
        int map[24];
        std::function<void(size_t)> rec = [&](size_t ci){
            if(ci==cls.size()){
                for(int label=0; label<M; ++label) map[ order[label] ] = label;
                int o0[12],o1[12]; relabelStack(cols,0,map,o0); relabelStack(cols,C,map,o1);
                u128 k=0; for(int j=0;j<C;++j) k=(k<<M)|(unsigned)o0[j]; for(int j=0;j<C;++j) k=(k<<M)|(unsigned)o1[j];
                if(k<best) best=k; return;
            }
            int lo=cls[ci].first, hi=cls[ci].second;
            std::sort(order+lo, order+hi);
            do { rec(ci+1); } while(std::next_permutation(order+lo, order+hi));
        };
        rec(0);
    }
    return best;
}

// ---------------- transition (one band), per-stack factorised ----------------
// Given the source state and a colouring A, the two stacks are filled INDEPENDENTLY
// (they use disjoint columns; only the colouring A is shared).  So we enumerate the
// distinct results of stack0 and of stack1 separately, then form their outer
// product.  This replaces the (C!)^4 leaf enumeration by 2*(C!)^2.
//
// A stack result = the new masks of its C columns (a packed C-tuple) with a count.
// stack0 columns get {one A-symbol (row0), one comp-symbol (row1)}; stack1 columns
// get {one comp-symbol (row0), one A-symbol (row1)}.  Either way, per stack we place
// the C A-symbols (one per column) and the C comp-symbols (one per column) avoiding
// existing column contents -> two independent perfect matchings.
static int s_cols[12];                 // working masks for one stack (C columns)
static int s_base[12];                 // original masks of this stack
static int s_remA, s_remC;
struct U128Hash{ size_t operator()(u128 x) const { return std::hash<unsigned long long>()((unsigned long long)x ^ (unsigned long long)(x>>64)); } };
static std::unordered_map<u128,u128,U128Hash>* s_out;

static inline u128 packStack(){ u128 k=0; for(int j=0;j<C;++j) k=(k<<M)|(unsigned)s_cols[j]; return k; }

// place A-symbols and comp-symbols, one of each per column, avoiding existing.
static void placeS(int j){
    if(j==C){ (*s_out)[packStack()] += 1; return; }
    int avA=s_remA & ~s_cols[j];
    for(int a=avA;a;a&=a-1){ int ab=a&-a;
        int avC=s_remC & ~s_cols[j] & ~ab;
        for(int c=avC;c;c&=c-1){ int cb=c&-c;
            int sv=s_cols[j]; s_cols[j]|=ab|cb;
            int sa=s_remA,sc=s_remC; s_remA&=~ab; s_remC&=~cb;
            placeS(j+1);
            s_remA=sa; s_remC=sc; s_cols[j]=sv;
        }
    }
}
// fill a stack's result distribution given its base masks (lo..lo+C-1 of cols[])
static void stackResults(const int* cols, int lo, int A, int comp,
                         std::unordered_map<u128,u128,U128Hash>& out){
    out.clear();
    for(int j=0;j<C;++j) s_cols[j]=cols[lo+j];
    s_remA=A; s_remC=comp; s_out=&out;
    placeS(0);
}

static std::vector<int>* g_subs;
static std::unordered_map<u128, std::vector<std::pair<u128,u128>>, U128Hash> transMemo;

static const std::vector<std::pair<u128,u128>>& transition(u128 src){
    auto it=transMemo.find(src); if(it!=transMemo.end()) return it->second;
    int cols[24]; u128 key=src;
    for(int j=2*C-1;j>=0;--j){ cols[j]=(int)(key&FULL); key>>=M; }
    std::map<u128,u128> acc;
    std::unordered_map<u128,u128,U128Hash> r0, r1;
    int wc[24];
    for(int A : *g_subs){
        int comp=FULL^A;
        // stack0: A in row0, comp in row1 ; stack1: comp in row0, A in row1.
        // Placement avoids existing contents identically for either role assignment,
        // so the result distribution only depends on (which symbols are "A-set",
        // which are "comp-set"): stack0 uses (A,comp), stack1 uses (comp,A) -> same
        // pair of sets, identical placement enumeration.
        stackResults(cols, 0, A, comp, r0);
        stackResults(cols, C, A, comp, r1);   // stack1 same sets (order within pair irrelevant to column SET)
        for(auto& a0 : r0){
            // unpack stack0 result into wc[0..C-1]
            { u128 r=a0.first; for(int j=C-1;j>=0;--j){ wc[j]=(int)(r&FULL); r>>=M; } }
            for(auto& a1 : r1){
                { u128 r=a1.first; for(int j=C-1;j>=0;--j){ wc[C+j]=(int)(r&FULL); r>>=M; } }
                acc[canonKey(wc)] += a0.second * a1.second;
            }
        }
    }
    std::vector<std::pair<u128,u128>> v(acc.begin(), acc.end());
    auto& ref = transMemo[src]; ref=std::move(v);
    return ref;
}

int main(int argc, char** argv){
    C = (argc>1)?std::atoi(argv[1]):2; M=2*C; FULL=(1<<M)-1;
    auto t0=std::chrono::steady_clock::now();

    std::vector<int> subs; for(int m=0;m<=FULL;++m) if(__builtin_popcount(m)==C) subs.push_back(m);
    g_subs=&subs;

    std::map<u128,u128> cur, nxt;
    { int cols[24]={0}; cur[canonKey(cols)] = 1; }

    size_t maxStates=1;
    for(int band=0; band<C; ++band){
        nxt.clear();
        for(auto& kv : cur){
            const auto& tr = transition(kv.first);
            for(auto& pr : tr) nxt[pr.first] += kv.second * pr.second;
        }
        cur.swap(nxt);
        if(cur.size()>maxStates) maxStates=cur.size();
        std::fprintf(stderr,"band %d: states=%zu  transMemo=%zu\n", band, cur.size(), transMemo.size());
    }

    u128 N=0;
    for(auto& kv:cur){
        u128 key=kv.first; bool all=true;
        for(int j=0;j<M;++j){ if((int)(key&FULL)!=FULL){all=false;} key>>=M; }
        if(all) N+=kv.second;
    }
    double sec=std::chrono::duration<double>(std::chrono::steady_clock::now()-t0).count();
    std::printf("2x%d: N = %s   (maxStates=%zu, %.3fs)\n", C, p128(N).c_str(), maxStates, sec);
    const char* exp=(C==2)?"288":(C==3)?"28200960":(C==4)?"29136487207403520":
                    (C==5)?"1903816047972624930994913280000":nullptr;
    if(exp) std::printf("expected 2x%d = %s  [%s]\n", C, exp, p128(N)==exp?"OK":"MISMATCH");
    return 0;
}
