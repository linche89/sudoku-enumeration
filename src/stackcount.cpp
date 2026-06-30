// stackcount.cpp — single-stack fill count and state-space probe.
//
// A single stack is a (2C) x C array: each of its C columns is a permutation of
// {0..2C-1}; the rows pair up by band, row 2i carries the C symbols of A_i and row
// 2i+1 the C symbols of comp(A_i).  StackCount(A_0..A_{C-1}) counts such arrays.
//
// We compute StackCount by a transfer over the C BANDS.  State after k bands =
// for each of the C columns, the SET of symbols placed so far (size 2k).  We
// reduce the state modulo permuting the C columns (interchangeable) -> a multiset
// of column masks.  (Symbols are NOT relabelled here because the row-sets A_i are
// concrete; but for the FULL N we will sum over A-sequences with symbol symmetry.)
//
// This program measures: (a) StackCount for sample A-sequences, (b) the size of the
// column-multiset state space, to judge polynomiality of a single-stack DP.
#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <vector>
#include <map>
#include <algorithm>
#include <string>
using u128=unsigned __int128;
static int C,M,FULL;

// add a band with coloring A to a single stack: column j gets one symbol of A
// (row 2i) and one of comp (row 2i+1), avoiding current contents.  We need the
// distribution over resulting column-multisets.  Enumerate the bijection of A to
// columns and comp to columns (each C! ) avoiding conflicts.  This is (C!)^2 per
// band per state -- still factorial, but C columns only (smaller than full DP) and
// we measure the multiset-state count.

// represent a single-stack state as sorted vector of C column masks
using SState=std::vector<int>;
static u128 weightOf; // unused placeholder

// transition: given sorted column masks `cols` (size C) and coloring A, accumulate
// into out[resulting sorted masks] += ways.  Enumerate A-symbol perm and comp perm.
static int g_cols[12]; static int g_A, g_comp;
static int g_remA, g_remC;
static std::map<SState,u128>* g_out;
static u128 g_w;
static void rec(int j){
    if(j==C){
        SState s(g_cols,g_cols+C); std::sort(s.begin(),s.end());
        (*g_out)[s]+=g_w; return;
    }
    int avA=g_remA & ~g_cols[j];
    for(int a=avA;a;a&=a-1){int ab=a&-a;
        int avC=g_remC & ~g_cols[j] & ~ab;
        for(int c=avC;c;c&=c-1){int cb=c&-c;
            int sv=g_cols[j]; g_cols[j]|=ab|cb;
            int sa=g_remA,sc=g_remC; g_remA&=~ab; g_remC&=~cb;
            rec(j+1);
            g_remA=sa; g_remC=sc; g_cols[j]=sv;
        }
    }
}

int main(int argc,char**argv){
    C=(argc>1)?atoi(argv[1]):3; M=2*C; FULL=(1<<M)-1;
    std::vector<int> subs; for(int m=0;m<=FULL;++m) if(__builtin_popcount(m)==C) subs.push_back(m);

    // measure single-stack state space if we FIX an A-sequence = all the same? No;
    // for the full N the A's vary. Here we explore: starting empty, apply C bands
    // with ALL POSSIBLE A each step, tracking the column-multiset state (symbols NOT
    // relabelled). Count distinct states. This bounds a single-stack DP that also
    // sums over A-sequences but keeps symbols concrete.
    std::map<SState,u128> cur, nxt;
    { SState s(C,0); cur[s]=1; }
    size_t maxst=1;
    for(int band=0;band<C;++band){
        nxt.clear();
        for(auto&kv:cur){
            for(int j=0;j<C;++j) g_cols[j]=kv.first[j];
            // but kv.first is the state; we must restore per A
            for(int A:subs){
                int comp=FULL^A;
                for(int j=0;j<C;++j) g_cols[j]=kv.first[j];
                g_remA=A; g_remC=comp; g_out=&nxt; g_w=kv.second;
                rec(0);
            }
        }
        cur.swap(nxt);
        if(cur.size()>maxst)maxst=cur.size();
        std::fprintf(stderr,"band %d: single-stack states (symbols concrete) = %zu\n", band, cur.size());
    }
    // total single-stack fillings summed over all A-sequences (should be (#stacks))
    u128 tot=0; for(auto&kv:cur){ bool full=true; for(int x:kv.first) if(x!=FULL)full=false; if(full) tot+=kv.second; }
    std::string s; u128 x=tot; if(!x)s="0"; while(x){s=char('0'+(int)(x%10))+s;x/=10;}
    std::printf("C=%d: sum over A-seq of StackCount = %s  (maxstates=%zu)\n", C, s.c_str(), maxst);
    return 0;
}
