// Measure: for a band-1 side histogram (the C-pair PartKeys of one side), how many
// DISTINCT canonical shapes are there modulo S_C acting on the C columns (single side)?
// If << raw count, state-level aggregation via side-shapes is viable.
// We reconstruct band-1 sides the same way the engine does: a side = C symbols each with
// a depth-1 (xmask,ymask) pair (popcount 1 each), placed one more band.
// Simpler proxy: generate the FULL set of band-1 side PartKeys for a representative source
// and canonicalize each part under S_C x S_C (the part lives in C x C frame).
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <array>
#include <vector>
#include <algorithm>
#include <set>
#include <map>
static int C,M,FULLC;
static std::vector<std::array<int,8>> CP; static std::vector<std::vector<int>> permMask;
static void initPerms(){std::vector<int>p(C);for(int i=0;i<C;++i)p[i]=i;do{std::array<int,8>a{};for(int i=0;i<C;++i)a[i]=p[i];CP.push_back(a);}while(std::next_permutation(p.begin(),p.end()));permMask.assign(CP.size(),std::vector<int>(1<<C,0));for(size_t pi=0;pi<CP.size();++pi)for(int m=0;m<(1<<C);++m){int r=0;for(int b=m;b;b&=b-1){int i=__builtin_ctz(b);r|=1<<CP[pi][i];}permMask[pi][m]=r;}}
// canon a C-pair part under S_C(Xcols) x S_C(Ycols)
static std::array<uint16_t,8> canonPart(const std::array<uint16_t,8>&part){
    std::array<uint16_t,8> best{}; bool have=false;
    for(size_t rp=0;rp<CP.size();++rp){const int*px=permMask[rp].data();
        for(size_t cp=0;cp<CP.size();++cp){const int*py=permMask[cp].data();
            std::array<uint16_t,8> c{};
            for(int i=0;i<C;++i){int t=part[i];c[i]=(uint16_t)(px[t&FULLC]|(py[(t>>C)&FULLC]<<C));}
            std::sort(c.begin(),c.begin()+C);
            if(!have||memcmp(c.data(),best.data(),C*2)<0){best=c;have=true;}}}
    return best;
}
int main(int argc,char**argv){
    C=atoi(argv[1]); M=2*C; FULLC=(1<<C)-1; initPerms();
    // build a depth-1 side: C symbols, each placed in band 0. After band0, a single stack's
    // C symbols each occupy 1 X-col and 1 Y-col. Represent the side as C pairs with popcount-1
    // masks. The band-1 transition places ANOTHER band: each symbol gets a 2nd X-col & Y-col.
    // We enumerate all (Xbij,Ybij) for the side and collect PartKeys, then canon them.
    // depth-1 side: symbol i has xmask={i}, ymask={i} (the canonical single-stack rep).
    std::array<uint16_t,8> xm{},ym{}; for(int i=0;i<C;++i){xm[i]=1<<i;ym[i]=1<<i;}
    std::set<std::array<uint16_t,8>> rawParts, canonParts;
    // all X-bijections and Y-bijections (col assignment avoiding existing)
    std::vector<std::array<int,8>> bij;
    {std::vector<int>p(C);for(int i=0;i<C;++i)p[i]=i;do{bool ok=true;for(int i=0;i<C;++i)if(xm[i]&(1<<p[i])){ok=false;break;}if(ok){std::array<int,8>a{};for(int i=0;i<C;++i)a[i]=p[i];bij.push_back(a);}}while(std::next_permutation(p.begin(),p.end()));}
    for(auto&ax:bij)for(auto&ay:bij){
        std::array<uint16_t,8> part{};
        for(int i=0;i<C;++i)part[i]=(uint16_t)((xm[i]|(1<<ax[i]))|((ym[i]|(1<<ay[i]))<<C));
        std::sort(part.begin(),part.begin()+C);
        rawParts.insert(part);
        canonParts.insert(canonPart(part));
    }
    printf("C=%d depth-1 side: rawParts=%zu  canonShapes(mod S_CxS_C)=%zu  bijections=%zu\n",
        C,rawParts.size(),canonParts.size(),bij.size());
    return 0;
}
