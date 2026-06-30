// band_count.cpp — kjellfp's polynomial RxC band counter (validated building block).
//
// Counts the number of valid bands of an RxC Sudoku: R boxes side by side, each
// box R rows x C cols, the whole band R rows x (R*C) cols, every row a
// permutation of {0..R*C-1} and every box containing all R*C symbols once.
//
//   band(R,C) = (C!)^{R^2} * N(R, R, C, a0)
//
// where the state a : E(L,R) -> N maps each L-subset U of the R rows to the
// number of symbols occurring in exactly the rows U; a0 maps the single full set
// N_R -> R*C.  N is built by the split recursion (split L boxes into M + (L-M)):
//
//   N(1,..,a) = 1
//   N(L,..,a) = sum over split-vectors b of
//                 N(M,..,a(b)) * N(L-M,..,a(^b))
//                 * prod_U a(U)! / prod_{(V,U)} b(V,U)!
//
//   b(V,U): for each U (|U|=L) and V subset U (|V|=M), how many of the a(U)
//   symbols go to the M-box subband in row-set V; constraints
//     (i)  sum_{V subset U,|V|=M} b(V,U) = a(U)
//     (ii) a(b)(V) = sum_{U superset V} b(V,U) is a valid M-state (auto by (i)).
//   ^b(V,U) = b(U\V, U) is the complementary (L-M)-state's split.
//
// Validation: band(2,C) = (2C)!*(C!)^2 ; band(3,C) = (3C)!*(C!)^6 * Franel(C).
#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <vector>
#include <map>
#include <algorithm>
#include <string>

using u128 = unsigned __int128;
static int R, Cg;

static u128 fact(int n){ u128 f=1; for(int i=2;i<=n;++i) f*=(u128)i; return f; }

// subsets of {0..R-1} of size L, in a fixed order; index lookup
struct Subsets {
    std::vector<int> masks;                 // size-L subsets as bitmasks
    std::map<int,int> idx;                   // mask -> position
    void build(int R, int L){
        masks.clear(); idx.clear();
        for(int m=0; m<(1<<R); ++m) if(__builtin_popcount(m)==L){ idx[m]=(int)masks.size(); masks.push_back(m);}
    }
    int size() const { return (int)masks.size(); }
};

// state a = vector of counts over E(L,R) (indexed by Subsets for that L)
using State = std::vector<int>;

static std::vector<Subsets> SS;             // SS[L] = size-L subsets of N_R

// memo: N(L, a) keyed by (L, packed state)
static std::map<std::pair<int,std::vector<int>>, u128> Nmemo;

// forward
static u128 N(int L, const State& a);

// enumerate split-vectors b for splitting L into M + (L-M).
// b is indexed by pairs (U in E(L,R), V in E(M,U)). We iterate per-U the ways to
// write a(U) as an ordered sum over the C(L,M) sub-subsets V of U, then combine.
// To keep it simple and correct, we recurse over the list of U's, carrying the
// partial M-state and (L-M)-state and the running multinomial factor.

struct SplitCtx {
    int L, M;
    const State* a;
    const Subsets* SL;   // E(L,R)
    const Subsets* SM;   // E(M,R)
    State mState;        // a(b): accumulates over V (M-subsets)
    State cState;        // a(^b): accumulates over U\V (also M-subsets... actually (L-M)-subsets)
    const Subsets* SLM;  // E(L-M,R)
    u128 acc;            // running product of a(U)!/prod b(V,U)!  and the two N's at the end
    u128 total;
};

// For a given U (mask, with |U|=L), enumerate distributions of n=a(U) symbols
// into the C(L,M) sub-subsets V of U; multiply 1/prod(b!), update mState[V]+=b,
// cState[U\V]+=b.
static void distributeU(SplitCtx& cx, const std::vector<int>& Us, int ui,
                        u128 numerFact, u128 denomFact);

static void enumV(SplitCtx& cx, const std::vector<int>& Us, int ui,
                  const std::vector<int>& Vs, int vi, int remain,
                  u128 numerFact, u128 denomFact) {
    if (vi == (int)Vs.size()-1) {
        int b = remain;                       // last V gets the rest
        int V = Vs[vi];
        cx.mState[ cx.SM->idx.at(V) ] += b;
        cx.cState[ cx.SLM->idx.at(Us[ui]^V) ] += b;
        distributeU(cx, Us, ui+1, numerFact, denomFact*fact(b));
        cx.mState[ cx.SM->idx.at(V) ] -= b;
        cx.cState[ cx.SLM->idx.at(Us[ui]^V) ] -= b;
        return;
    }
    int V = Vs[vi];
    for (int b=0; b<=remain; ++b) {
        cx.mState[ cx.SM->idx.at(V) ] += b;
        cx.cState[ cx.SLM->idx.at(Us[ui]^V) ] += b;
        enumV(cx, Us, ui, Vs, vi+1, remain-b, numerFact, denomFact*fact(b));
        cx.mState[ cx.SM->idx.at(V) ] -= b;
        cx.cState[ cx.SLM->idx.at(Us[ui]^V) ] -= b;
    }
}

static void distributeU(SplitCtx& cx, const std::vector<int>& Us, int ui,
                        u128 numerFact, u128 denomFact) {
    if (ui == (int)Us.size()) {
        // all U processed: this split-vector b is complete
        u128 term = (numerFact / denomFact) * N(cx.M, cx.mState) * N(cx.L - cx.M, cx.cState);
        cx.total += term;
        return;
    }
    int U = Us[ui];
    int n = (*cx.a)[ cx.SL->idx.at(U) ];
    // sub-subsets V of U with |V|=M
    std::vector<int> Vs;
    for (int v=U; ; v=(v-1)&U) { if (__builtin_popcount(v)==cx.M) Vs.push_back(v); if (v==0) break; }
    enumV(cx, Us, ui, Vs, 0, n, numerFact*fact(n), denomFact);
}

static u128 N(int L, const State& a) {
    if (L == 1) return 1;
    auto key = std::make_pair(L, a);
    auto it = Nmemo.find(key); if (it!=Nmemo.end()) return it->second;
    int M = L/2;                              // split near half
    SplitCtx cx;
    cx.L=L; cx.M=M; cx.a=&a;
    cx.SL=&SS[L]; cx.SM=&SS[M]; cx.SLM=&SS[L-M];
    cx.mState.assign(SS[M].size(),0);
    cx.cState.assign(SS[L-M].size(),0);
    cx.total=0;
    std::vector<int> Us = SS[L].masks;
    distributeU(cx, Us, 0, 1, 1);
    Nmemo[key]=cx.total;
    return cx.total;
}

int main(int argc, char** argv){
    R = (argc>1)?atoi(argv[1]):2;
    Cg = (argc>2)?atoi(argv[2]):3;
    SS.assign(R+1, Subsets());
    for (int L=0; L<=R; ++L) SS[L].build(R, L);

    // a0: full set N_R -> R*C
    State a0(SS[R].size(), 0);
    int full = (1<<R)-1;
    a0[ SS[R].idx.at(full) ] = R*Cg;

    u128 Nval = N(R, a0);
    u128 band = 1; for (int i=0;i<R*R;++i) band *= fact(Cg);   // (C!)^{R^2}
    band *= Nval;

    auto pr=[](u128 x){ std::string s; if(!x)s="0"; while(x){s=char('0'+(int)(x%10))+s;x/=10;} return s; };
    std::printf("band(R=%d,C=%d): N(R,R,C,a0)=%s  band=%s\n", R, Cg, pr(Nval).c_str(), pr(band).c_str());

    // validation against closed forms
    auto factll=[](int n){ u128 f=1; for(int i=2;i<=n;++i) f*=(u128)i; return f; };
    if (R==2) { u128 e=factll(2*Cg)*factll(Cg)*factll(Cg); std::printf("  expect (2C)!(C!)^2 = %s [%s]\n", pr(e).c_str(), band==e?"OK":"BAD"); }
    if (R==3) { long long fr=0; for(int k=0;k<=Cg;++k){ long long c=1; for(int i=0;i<k;++i) c=c*(Cg-i)/(i+1); fr+=c*c*c; }
                u128 e=factll(3*Cg)*factll(Cg); for(int i=0;i<5;++i) e*=factll(Cg); e*=(u128)fr;
                std::printf("  expect (3C)!(C!)^6*Franel(%lld) = %s [%s]\n", fr, pr(e).c_str(), band==e?"OK":"BAD"); }
    return 0;
}
