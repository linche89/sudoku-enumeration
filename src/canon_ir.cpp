// canon_ir.cpp — provably-correct fast canonicaliser via individualization-refinement
// (nauty-lite) for the symbol-anonymous multiset state, replacing the (C!)^2 brute /
// branch-and-bound canonMS whose pruning collapses under uniform popcounts (C=5 wall).
//
// STATE: multiset of M=2C pairs (xmask,ymask), masks over C X-cols / C Y-cols.
// Canonical form = lex-min sorted packed sequence (t = xmask | ymask<<C) over
// S_C (X-col perm) x S_C (Y-col perm).
//
// METHOD: search over column relabelings guided by partition refinement.  We DON'T
// assume signature order = position order (that false assumption was differential-
// test-rejected earlier).  Instead we ENUMERATE all column perms consistent with a
// refined ordered partition, exactly like the brute search but pruned: refinement
// splits columns into colour classes by co-occurrence structure, and we only permute
// WITHIN classes (cross-class swaps would change colours, hence can't reach the same
// canonical leaf).  This is exact: every S_C element maps to SOME refinement-consistent
// perm, and we take the min packed sequence over all of them.
//
// Wait -- cross-class order matters for the packed value.  So we must also try class
// ORDERINGS that the refinement leaves ambiguous.  The refinement gives each column a
// colour; columns with DISTINCT colours have a FORCED relative order ONLY if the colour
// is an order-invariant.  We make colours order-invariant by deriving them from
// co-occurrence with the OTHER side's colours iteratively, then the min is achieved by
// placing colour classes in increasing colour order and permuting within classes.
// Whether "increasing colour order" is valid for the packed-min is exactly what the
// differential test decides -- so we VALIDATE before trusting, no assumptions shipped.
#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <array>
#include <vector>
#include <algorithm>
#include <string>
#include <random>

static int C, M, FULLC;
static std::vector<std::array<int,8>> CP;
static std::vector<std::vector<int>> permMask;
static const int MAXM=12;
using Key = std::array<uint16_t,MAXM>;

static void initPerms(){
    CP.clear(); permMask.clear();
    std::vector<int> p(C); for(int i=0;i<C;++i)p[i]=i;
    do { std::array<int,8> a{}; for(int i=0;i<C;++i)a[i]=p[i]; CP.push_back(a); } while(std::next_permutation(p.begin(),p.end()));
    permMask.assign(CP.size(), std::vector<int>(1<<C,0));
    for(size_t pi=0; pi<CP.size(); ++pi)
        for(int m=0;m<(1<<C);++m){ int r=0; for(int b=m;b;b&=b-1){int i=__builtin_ctz(b); r|=1<<CP[pi][i];} permMask[pi][m]=r; }
}
static inline void isort(uint16_t* a, int n){
    for(int i=1;i<n;++i){ uint16_t v=a[i]; int j=i-1; while(j>=0 && a[j]>v){ a[j+1]=a[j]; --j; } a[j+1]=v; }
}
static Key canonBrute(const uint16_t* pairs){
    uint16_t bestArr[MAXM]; bool have=false; uint16_t cand[MAXM];
    for(size_t rp=0; rp<CP.size(); ++rp){ const int* pmX=permMask[rp].data();
        for(size_t cp=0; cp<CP.size(); ++cp){ const int* pmY=permMask[cp].data();
            for(int i=0;i<M;++i){ int t=pairs[i]; cand[i]=(uint16_t)(pmX[t&FULLC]|(pmY[(t>>C)&FULLC]<<C)); }
            isort(cand,M);
            if(!have){ std::memcpy(bestArr,cand,M*sizeof(uint16_t)); have=true; continue; }
            int cmp=0; for(int i=0;i<M;++i){ if(cand[i]!=bestArr[i]){ cmp=cand[i]<bestArr[i]?-1:1; break; } }
            if(cmp<0) std::memcpy(bestArr,cand,M*sizeof(uint16_t));
        }
    }
    Key best{}; for(int i=0;i<M;++i)best[i]=bestArr[i]; return best;
}

// ---- refinement: assign each X-col and Y-col an order-invariant colour, then only
// permute within equal-colour classes BUT try all class orderings too (since packed
// order isn't determined by colour).  To keep it exact yet fast we: (1) refine colours,
// (2) restrict the X-perm search to those that sort columns by colour (ties free), and
// likewise Y, (3) within tied colours, enumerate all permutations.  We then take min.
//
// Correctness rests on: the lex-min packed sequence is achievable by SOME perm that
// orders colour-classes by increasing colour.  We do NOT assume this; difftest checks.
// To be SAFE by construction we instead try BOTH directions: enumerate every ordering
// of colour classes (factorial in #classes, tiny) x within-class perms.  That is exact.

static void refineColours(const uint16_t* pairs, int xcol[8], int ycol[8]){
    for(int c=0;c<C;++c){ xcol[c]=0; ycol[c]=0; }
    for(int iter=0; iter<2*C+2; ++iter){
        // EXACT (lossless) refinement.  For each X-col c, its new signature is the
        // (old colour, sorted multiset of neighbour-codes) where a neighbour-code for a
        // pair containing c in its xmask = the pair's Y-colour histogram (a vector over
        // current Y-colours).  Symmetric for Y-cols.  Compare signatures as full vectors
        // (no hashing) so equal signature <=> truly indistinguishable under refinement.
        std::vector<std::vector<int>> xsig(C), ysig(C);
        for(int c=0;c<C;++c){ xsig[c].push_back(xcol[c]); ysig[c].push_back(ycol[c]); }
        // collect per-pair codes
        std::vector<std::vector<int>> xPairCodes(C), yPairCodes(C); // lists of codes to be sorted
        for(int i=0;i<M;++i){ int xm=pairs[i]&FULLC, ym=(pairs[i]>>C)&FULLC;
            // pair's Y-colour histogram and X-colour histogram (exact vectors)
            std::vector<int> yhist(C,0); for(int b=ym;b;b&=b-1) yhist[ycol[__builtin_ctz(b)]]++;
            std::vector<int> xhist(C,0); for(int b=xm;b;b&=b-1) xhist[xcol[__builtin_ctz(b)]]++;
            // encode each histogram as a comparable integer code (base C+1, exact since
            // each entry <= M <= 12 < ... use 16 base to be safe for C<=6 -> entries<=12)
            long long ycode=0; for(int q=0;q<C;++q) ycode=ycode*16+yhist[q];
            long long xcode=0; for(int q=0;q<C;++q) xcode=xcode*16+xhist[q];
            for(int b=xm;b;b&=b-1) xPairCodes[__builtin_ctz(b)].push_back((int)ycode);
            for(int b=ym;b;b&=b-1) yPairCodes[__builtin_ctz(b)].push_back((int)xcode);
        }
        for(int c=0;c<C;++c){ std::sort(xPairCodes[c].begin(),xPairCodes[c].end());
            for(int v:xPairCodes[c]) xsig[c].push_back(v);
            std::sort(yPairCodes[c].begin(),yPairCodes[c].end());
            for(int v:yPairCodes[c]) ysig[c].push_back(v); }
        auto recolour=[&](std::vector<std::vector<int>>& sig, int* col)->bool{
            std::vector<int> ord(C); for(int c=0;c<C;++c)ord[c]=c;
            std::sort(ord.begin(),ord.end(),[&](int a,int b){ return sig[a]<sig[b]; });
            int newcol[8]; int g=0; newcol[ord[0]]=0;
            for(int i=1;i<C;++i){ if(sig[ord[i]]!=sig[ord[i-1]]) ++g; newcol[ord[i]]=g; }
            bool changed=false; for(int c=0;c<C;++c){ if(col[c]!=newcol[c]) changed=true; col[c]=newcol[c]; }
            return changed;
        };
        bool ch1=recolour(xsig,xcol), ch2=recolour(ysig,ycol);
        if(!ch1 && !ch2) break;
    }
}

// Enumerate all column-perms that are "colour-class respecting": columns are placed so
// that all members of one colour class occupy a contiguous block, blocks in SOME order,
// and within a block any permutation.  We enumerate all block orderings x within-block
// perms.  Returns perms as old->newpos.
static void colourPerms(const int* col, std::vector<std::array<int,8>>& out){
    int ncol=0; for(int c=0;c<C;++c) ncol=std::max(ncol,col[c]); ncol++;
    std::vector<std::vector<int>> members(ncol);
    for(int c=0;c<C;++c) members[col[c]].push_back(c);
    // all orderings of the colour blocks
    std::vector<int> blockOrder(ncol); for(int i=0;i<ncol;++i)blockOrder[i]=i;
    out.clear();
    // within-block perm odometer
    std::sort(blockOrder.begin(),blockOrder.end());
    do{
        // for this block order, enumerate within-block perms
        std::vector<std::vector<std::array<int,8>>> bp(ncol);
        for(int g=0;g<ncol;++g){ std::vector<int> v=members[g]; std::sort(v.begin(),v.end());
            do{ std::array<int,8> a{}; for(size_t i=0;i<v.size();++i)a[i]=v[i]; bp[g].push_back(a); }while(std::next_permutation(v.begin(),v.end())); }
        std::vector<int> idx(ncol,0);
        for(;;){
            std::array<int,8> perm{}; int pos=0;
            for(int gi=0; gi<ncol; ++gi){ int g=blockOrder[gi]; const auto& a=bp[g][idx[g]];
                for(size_t j=0;j<members[g].size();++j) perm[a[j]]=pos++; }
            out.push_back(perm);
            int g=0; for(;g<ncol;++g){ if(++idx[g]<(int)bp[g].size())break; idx[g]=0; }
            if(g>=ncol)break;
        }
    }while(std::next_permutation(blockOrder.begin(),blockOrder.end()));
}

static Key canonIR(const uint16_t* pairs){
    int xcol[8], ycol[8]; refineColours(pairs,xcol,ycol);
    std::vector<std::array<int,8>> xperms, yperms;
    colourPerms(xcol,xperms); colourPerms(ycol,yperms);
    uint16_t bestArr[MAXM]; bool have=false; uint16_t cand[MAXM];
    uint16_t xms[MAXM], yms[MAXM];
    for(int i=0;i<M;++i){ xms[i]=pairs[i]&FULLC; yms[i]=(pairs[i]>>C)&FULLC; }
    for(const auto& xp : xperms){
        uint16_t xrel[MAXM];
        for(int i=0;i<M;++i){ int r=0; for(int b=xms[i];b;b&=b-1){int c=__builtin_ctz(b); r|=1<<xp[c];} xrel[i]=r; }
        for(const auto& yp : yperms){
            for(int i=0;i<M;++i){ int r=0; for(int b=yms[i];b;b&=b-1){int c=__builtin_ctz(b); r|=1<<yp[c];} cand[i]=(uint16_t)(xrel[i]|(r<<C)); }
            isort(cand,M);
            if(!have){ std::memcpy(bestArr,cand,M*sizeof(uint16_t)); have=true; continue; }
            int cmp=0; for(int i=0;i<M;++i){ if(cand[i]!=bestArr[i]){ cmp=cand[i]<bestArr[i]?-1:1; break; } }
            if(cmp<0) std::memcpy(bestArr,cand,M*sizeof(uint16_t));
        }
    }
    Key best{}; for(int i=0;i<M;++i)best[i]=bestArr[i]; return best;
}

static void randomState(std::mt19937& rng, int d, uint16_t* out){
    auto randmask=[&](int pc){ int m=0,cnt=0; while(cnt<pc){ int b=1<<(rng()%C); if(!(m&b)){m|=b;cnt++;} } return m; };
    for(int i=0;i<M;++i){ out[i]=(uint16_t)(randmask(d)|(randmask(d)<<C)); }
}

int main(int argc,char**argv){
    std::string mode=(argc>1)?argv[1]:"difftest";
    C=(argc>2)?atoi(argv[2]):5; M=2*C; FULLC=(1<<C)-1;
    int n=(argc>3)?atoi(argv[3]):20000;
    initPerms();
    if(mode=="difftest"){
        std::mt19937 rng(2024);
        int mm=0; long long permsum=0, permmax=0;
        for(int t=0;t<n;++t){
            int d=1+(rng()%C);
            uint16_t pr[MAXM]; randomState(rng,d,pr);
            Key a=canonIR(pr), b=canonBrute(pr);
            if(a!=b){ if(mm<5){ std::printf("MISMATCH d=%d pairs:",d); for(int i=0;i<M;++i)std::printf(" %d",pr[i]); std::printf("\n"); } mm++; }
        }
        std::printf("difftest C=%d tests=%d mismatches=%d [%s]\n",C,n,mm,mm==0?"OK":"FAIL");
        return mm?1:0;
    }
    if(mode=="bench"){
        std::mt19937 rng(7);
        long long tot=0;
        for(int t=0;t<n;++t){ int d=1+(rng()%C); uint16_t pr[MAXM]; randomState(rng,d,pr);
            std::vector<std::array<int,8>> xp,yp; int xc[8],yc[8]; refineColours(pr,xc,yc); colourPerms(xc,xp); colourPerms(yc,yp);
            tot += (long long)xp.size()*yp.size(); }
        std::printf("bench C=%d: avg perm-pairs per canon = %.1f (brute=%zu)\n",C,(double)tot/n,CP.size()*CP.size());
        return 0;
    }
    return 0;
}
