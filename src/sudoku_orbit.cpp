// sudoku_orbit.cpp — orbit-generation counter for 2xC Sudoku (boxes 2xC),
// via #(2xC)=#(Cx2).  Box 0 is fixed to T={0..C-1}|{C..2C-1} (factor C(2C,C));
// the remaining C-1 boxes are reduced under the relabel stabiliser S_C x S_C.
//
// Under S_C x S_C the orbit of (S_1,...,S_{C-1}) is determined by a pair of
// "membership-pattern multisets": for each T-symbol, which boxes contain it (a
// (C-1)-bit pattern), and likewise for each ~T-symbol.  So
//
//   N = C(2C,C) * sum over compatible (muT, mu~T) of
//          (C!/prod muT!) * (C!/prod mu~T!) * B(sigma)^2,
//
// where muT, mu~T are multisets of C patterns from {0..2^{C-1}-1}, compatible
// means for every box b: (#T-symbols with b) + (#~T-symbols with b) = C, and
// B(sigma) is the C-row band fill count for the reconstructed signature.
//
// Validation gates: 2x3 = 28200960, 2x4 = 29136487207403520,
//                   2x5 = 1903816047972624930994913280000.
#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <cmath>
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <chrono>
#include <functional>

static int C, M, FULL;
static int rem_[16];

// ---- B(sigma): C-row band fill = sum over first C-2 rows of 2^(residual cycles)
static int residual_cycles() {
    int par[40]; for (int i = 0; i < 2*M; ++i) par[i] = i;
    auto find = [&](int x){ while (par[x] != x){ par[x]=par[par[x]]; x=par[x]; } return x; };
    int comps = 2*M;
    for (int col = 0; col < M; ++col)
        for (int s = rem_[col]; s; s &= s-1) {
            int sym = __builtin_ctz(s), a = find(col), b = find(M+sym);
            if (a != b) { par[a]=b; --comps; }
        }
    return comps;
}
static int64_t enum_row(int rowsLeft, int col, int usedRow);
static int64_t Bfill(int rowsLeft) {
    if (rowsLeft == 2) return (int64_t)1 << residual_cycles();
    if (rowsLeft == 1) return 1;
    return enum_row(rowsLeft, 0, 0);
}
static int64_t enum_row(int rowsLeft, int col, int usedRow) {
    if (col == M) return Bfill(rowsLeft-1);
    int64_t s = 0; int avail = rem_[col] & ~usedRow;
    while (avail) { int bit = avail&-avail; avail-=bit; rem_[col]^=bit; s+=enum_row(rowsLeft,col+1,usedRow|bit); rem_[col]^=bit; }
    return s;
}
static int64_t Bcount(const int* colset) { for (int j=0;j<M;++j) rem_[j]=colset[j]; return Bfill(C); }

// relabel-canonical memo for B (anchor each box -> {0..C-1}|{C..2C-1}, sort, min)
static std::map<unsigned __int128,int64_t> Bmemo;
static unsigned __int128 canon_key(const int* colset) {
    unsigned __int128 best = ~(unsigned __int128)0;
    for (int a = 0; a < C; ++a) {
        int map[20], t;
        t=0; for (int s=colset[2*a];   s; s&=s-1) map[__builtin_ctz(s)]=t++;
        t=C; for (int s=colset[2*a+1]; s; s&=s-1) map[__builtin_ctz(s)]=t++;
        int b0[20], b1[20];
        for (int b=0;b<C;++b){ int x=0,y=0;
            for (int s=colset[2*b];   s; s&=s-1) x|=1<<map[__builtin_ctz(s)];
            for (int s=colset[2*b+1]; s; s&=s-1) y|=1<<map[__builtin_ctz(s)];
            if (x>y){int tmp=x;x=y;y=tmp;} b0[b]=x; b1[b]=y; }
        int ord[20]; for (int b=0;b<C;++b) ord[b]=b;
        std::sort(ord,ord+C,[&](int i,int j){return b0[i]!=b0[j]?b0[i]<b0[j]:b1[i]<b1[j];});
        unsigned __int128 key=0;
        for (int b=0;b<C;++b){ key=(key<<M)|(unsigned)b0[ord[b]]; key=(key<<M)|(unsigned)b1[ord[b]]; }
        if (key<best) best=key;
    }
    return best;
}
static int64_t Bget(const int* colset) {
    unsigned __int128 k = canon_key(colset);
    auto it = Bmemo.find(k); if (it!=Bmemo.end()) return it->second;
    int64_t v = Bcount(colset); Bmemo.emplace(k,v); return v;
}

int main(int argc, char** argv) {
    C = (argc>1)?std::atoi(argv[1]):3; M=2*C; FULL=(1<<M)-1;
    const int B1 = C-1;                  // boxes 1..C-1
    const int P  = 1 << B1;              // membership patterns over those boxes
    auto fact=[](int n){ int64_t f=1; for(int i=2;i<=n;++i) f*=i; return f; };
    int64_t factC = fact(C);
    int64_t box0factor = 1;              // C(2C,C)
    for (int i=0;i<C;++i) box0factor = box0factor*(M-i)/(i+1);

    // enumerate all count-vectors = compositions of C into P parts
    struct Vec { std::vector<uint8_t> c; int64_t mult; int64_t marg; };
    std::vector<Vec> vecs;
    {
        std::vector<uint8_t> cur(P,0);
        // iterative composition generation
        std::vector<int> stack; // simple recursion via lambda
        std::function<void(int,int)> gen = [&](int pos,int rem){
            if (pos==P-1){ cur[pos]=(uint8_t)rem;
                int64_t m=factC; int64_t marg=0;
                for (int p=0;p<P;++p) m/=fact(cur[p]);
                for (int b=0;b<B1;++b){ int t=0; for(int p=0;p<P;++p) if(p>>b&1) t+=cur[p]; marg=marg*(C+1)+t; }
                vecs.push_back({cur,m,marg}); return; }
            for (int v=0; v<=rem; ++v){ cur[pos]=(uint8_t)v; gen(pos+1,rem-v); }
        };
        gen(0,C);
    }
    std::fprintf(stderr,"C=%d  M=%d  patterns P=%d  count-vectors=%zu\n",C,M,P,vecs.size());

    // group vectors by marginal for compatible lookup
    std::map<int64_t,std::vector<int>> byMarg;
    for (int i=0;i<(int)vecs.size();++i) byMarg[vecs[i].marg].push_back(i);
    // full-marginal value where every box has count C
    int64_t fullMarg=0; for (int b=0;b<B1;++b) fullMarg=fullMarg*(C+1)+C;

    auto t0=std::chrono::steady_clock::now();
    unsigned __int128 N=0;
    int colset[20]; colset[0]=(1<<C)-1; colset[1]=FULL^colset[0];   // box 0
    int64_t pairs=0;
    for (int i=0;i<(int)vecs.size();++i) {
        // required mu~T marginal: per box C - tb[b]; encode (same base traversal order)
        int64_t need = fullMarg - vecs[i].marg;            // since marg packs each box linearly, C-tb per box
        auto it = byMarg.find(need); if (it==byMarg.end()) continue;
        // build T-side box masks for this muT
        // assign T-symbols 0..C-1 to patterns
        int Smask[20]={0};                                 // Smask[b] over boxes 1..B1 (bit b)
        { int sym=0; for (int p=0;p<P;++p) for (int k=0;k<vecs[i].c[p];++k){ for(int b=0;b<B1;++b) if(p>>b&1) Smask[b]|=1<<sym; ++sym; } }
        for (int jj : it->second) {
            // assign ~T-symbols C..2C-1
            int Sm[20]; for (int b=0;b<B1;++b) Sm[b]=Smask[b];
            { int sym=C; for (int p=0;p<P;++p) for (int k=0;k<vecs[jj].c[p];++k){ for(int b=0;b<B1;++b) if(p>>b&1) Sm[b]|=1<<sym; ++sym; } }
            for (int b=0;b<B1;++b){ colset[2*(b+1)]=Sm[b]; colset[2*(b+1)+1]=FULL^Sm[b]; }
            int64_t Bv = Bget(colset);
            if (Bv) {
                int64_t orb = vecs[i].mult * vecs[jj].mult;     // orbit size under S_C x S_C
                N += (unsigned __int128)(uint64_t)orb * (uint64_t)(Bv*Bv);
            }
            ++pairs;
        }
    }
    N *= (unsigned __int128)(uint64_t)box0factor;

    auto u128=[](unsigned __int128 x){std::string s;if(!x)s="0";while(x){s=char('0'+(int)(x%10))+s;x/=10;}return s;};
    double sec=std::chrono::duration<double>(std::chrono::steady_clock::now()-t0).count();
    std::printf("2x%d (grid %dx%d): N = %s\n", C, M, M, u128(N).c_str());
    std::fprintf(stderr,"pairs=%lld  B-memo=%zu  time=%.3fs\n",(long long)pairs,Bmemo.size(),sec);
    const char* exp = (C==3)?"28200960":(C==4)?"29136487207403520":
                      (C==5)?"1903816047972624930994913280000":nullptr;
    if (exp) std::printf("expected 2x%d = %s  [%s]\n", C, exp, u128(N)==exp?"OK":"MISMATCH");
    return 0;
}
