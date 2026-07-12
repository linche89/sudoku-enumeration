// jointcanon.cpp — GROUND TRUTH for the joint-square transfer of N(2xC).
//
// N(2xC) = # of pairs (X-fill, Y-fill) of stack0 that share the same skeleton
// sequence (A_0..A_{C-1}).  Band-by-band joint transfer; the joint state is
// (X: C column-sets, Y: C column-sets).  We canonicalise EXACTLY under
// S_{2C} (relabel symbols) x S_C (permute X-cols) x S_C (permute Y-cols), by
// brute force over all (2C)! symbol permutations and sorting columns.  Slow, but
// CORRECT — this is the oracle that every fast invariant must match.
//
// Validation: must give N(2x2)=288, N(2x3)=28200960.
// Output: also the canonical-state count per band (the polynomial-dimension we
// need to confirm), so we know exactly how fine the fast invariant must be.
#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <vector>
#include <map>
#include <set>
#include <algorithm>
#include <string>
using u128 = unsigned __int128;

static int C, M;                       // M = 2C symbols
// a column-set is a bitmask over M symbols. A stack-state = C such masks.
// joint state = 2C masks (first C = X cols, last C = Y cols).

static std::vector<int> Askel;         // all C-subsets of [M]

// canonicalise a joint state under symbol-perm x Xcol-perm x Ycol-perm.
// EXACT but fast: a symbol's "type" = (sorted multiset of X-columns it lies in,
// sorted multiset of Y-columns) is invariant under column permutations only up to
// relabeling columns — so we use a coarser-but-safe refinement: group symbols by
// their (degX, degY) signature, and only permute symbols WITHIN a group (symbols
// in different groups can never be exchanged by any symmetry).  We still try all
// column permutations implicitly via sorting.  This is exact: any symmetry maps a
// symbol to one of identical (degX,degY), so the minimal key is achieved within
// the group-respecting permutations.
static std::string canon(const std::vector<int>& st) {
    // degree signature per symbol
    int degX[16] = {0}, degY[16] = {0};
    for (int c = 0; c < C; ++c) {
        for (int s = st[c];   s; s &= s-1) degX[__builtin_ctz(s)]++;
        for (int s = st[C+c]; s; s &= s-1) degY[__builtin_ctz(s)]++;
    }
    // bucket symbols by (degX,degY); permute only within buckets.
    // Buckets MUST be ordered by an INVARIANT (the (degX,degY) key) so the label
    // assignment is canonical, not dependent on symbol indices.
    std::vector<std::pair<std::pair<int,int>,std::vector<int>>> bk;  // (sig)->symbols
    {
        std::map<std::pair<int,int>,std::vector<int>> tmp;
        for (int s = 0; s < M; ++s) tmp[{degX[s],degY[s]}].push_back(s);
        for (auto& kv : tmp) bk.push_back({kv.first, kv.second});     // std::map sorts by key
    }
    const int NB = (int)bk.size();
    std::vector<int> labelBase(NB);
    { int acc=0; for (int b=0;b<NB;++b){ labelBase[b]=acc; acc+=(int)bk[b].second.size(); } }

    std::string best;
    std::vector<std::vector<std::vector<int>>> bucketPerms(NB);
    for (int b=0;b<NB;++b){
        std::vector<int> v=bk[b].second; std::sort(v.begin(),v.end());
        do { bucketPerms[b].push_back(v); } while(std::next_permutation(v.begin(),v.end()));
    }
    std::vector<int> idx(NB,0);
    for (;;) {
        int perm[16];
        for (int b=0;b<NB;++b){
            const std::vector<int>& bp = bucketPerms[b][idx[b]];
            for (int j=0;j<(int)bk[b].second.size();++j) perm[ bp[j] ] = labelBase[b]+j;
        }
        std::vector<int> X(C), Y(C);
        for (int c=0;c<C;++c){
            int mx=0,my=0;
            for (int s=st[c];   s; s&=s-1) mx|=1<<perm[__builtin_ctz(s)];
            for (int s=st[C+c]; s; s&=s-1) my|=1<<perm[__builtin_ctz(s)];
            X[c]=mx; Y[c]=my;
        }
        std::sort(X.begin(),X.end()); std::sort(Y.begin(),Y.end());
        std::string key; key.reserve(4*C+1);
        for (int v:X){ key.push_back((char)(v&0xff)); key.push_back((char)((v>>8)&0xff)); }
        key.push_back('|');
        for (int v:Y){ key.push_back((char)(v&0xff)); key.push_back((char)((v>>8)&0xff)); }
        if (best.empty()||key<best) best=key;
        int b=0; for (; b<NB; ++b){ if(++idx[b] < (int)bucketPerms[b].size()) break; idx[b]=0; }
        if (b==NB) break;
    }
    return best;
}

// place skeleton A into a stack-state (C masks) -> list of resulting stack-states.
// top row places A (bijection to C cols), bot row places ~A (bijection), no symbol
// repeated within a column.
static void placeRec(const std::vector<int>& cols, const std::vector<int>& Av,
                     const std::vector<int>& Acv, int c, int usedTop, int usedBot,
                     std::vector<int>& cur, std::vector<std::vector<int>>& out) {
    if (c == C) { out.push_back(cur); return; }
    for (int ti = 0; ti < C; ++ti) {
        int ts = Av[ti]; if (usedTop & (1<<ti)) continue;
        if (cols[c] & (1<<ts)) continue;
        for (int bi = 0; bi < C; ++bi) {
            int bs = Acv[bi]; if (usedBot & (1<<bi)) continue;
            if (cols[c] & (1<<bs)) continue;
            // ts != bs always (A and ~A disjoint)
            cur[c] = cols[c] | (1<<ts) | (1<<bs);
            placeRec(cols, Av, Acv, c+1, usedTop|(1<<ti), usedBot|(1<<bi), cur, out);
        }
    }
}
static std::vector<std::vector<int>> place(const std::vector<int>& cols, int A) {
    std::vector<int> Av, Acv;
    for (int s = 0; s < M; ++s) { if (A&(1<<s)) Av.push_back(s); else Acv.push_back(s); }
    std::vector<std::vector<int>> out; std::vector<int> cur(C,0);
    placeRec(cols, Av, Acv, 0, 0, 0, cur, out);
    return out;
}

int main(int argc, char** argv) {
    C = (argc>1)?atoi(argv[1]):3; M = 2*C;
    for (int m = 0; m < (1<<M); ++m) if (__builtin_popcount(m)==C) Askel.push_back(m);

    // joint transfer over C bands, keyed by canonical joint state
    std::map<std::string, u128> states;
    std::map<std::string, std::vector<int>> rep;     // canonical -> a representative joint state
    {
        std::vector<int> empty(2*C, 0);
        states[canon(empty)] = 1; rep[canon(empty)] = empty;
    }
    for (int band = 0; band < C; ++band) {
        std::map<std::string, u128> nxt;
        std::map<std::string, std::vector<int>> nrep;
        for (auto& kv : states) {
            const std::vector<int>& st = rep[kv.first]; u128 w = kv.second;
            std::vector<int> Xc(st.begin(), st.begin()+C), Yc(st.begin()+C, st.end());
            for (int A : Askel) {
                auto xo = place(Xc, A); if (xo.empty()) continue;
                auto yo = place(Yc, A); if (yo.empty()) continue;
                for (auto& nx : xo) for (auto& ny : yo) {
                    std::vector<int> js; js.reserve(2*C);
                    js.insert(js.end(), nx.begin(), nx.end());
                    js.insert(js.end(), ny.begin(), ny.end());
                    std::string k = canon(js);
                    nxt[k] += w;
                    if (!nrep.count(k)) nrep[k] = js;
                }
            }
        }
        states = std::move(nxt); rep = std::move(nrep);
        std::fprintf(stderr, "band %d: canonical states = %zu\n", band, states.size());
    }
    // final: all columns full
    std::vector<int> full(2*C, (1<<M)-1);
    std::string fk = canon(full);
    u128 N = states.count(fk) ? states[fk] : 0;
    std::string s; if(!N)s="0"; { u128 x=N; while(x){s=char('0'+(int)(x%10))+s;x/=10;} }
    std::printf("C=%d: N = %s\n", C, s.c_str());
    const char* e = (C==2)?"288":(C==3)?"28200960":(C==4)?"29136487207403520":nullptr;
    if (e) std::printf("   expect %s [%s]\n", e, s==e?"OK":"BAD");
    return 0;
}
