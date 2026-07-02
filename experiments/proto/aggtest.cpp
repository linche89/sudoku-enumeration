// Validate the shape-aggregation identity for ONE class transition:
//   brute: for all (tp in hTop, bp in hBot): target[canon(tp⊎bp)] += w_tp*w_bp
//   agg  : group hTop by canon-shape; for each shape (rep tp0, total weight Wtop):
//          for all bp in hBot: target2[canon(tp0⊎bp)] += Wtop * w_bp
// If target==target2 for random sides, the "fix top rep per shape, vary all bp" identity holds.
// (This already cuts cost from |hTop|*|hBot| to (#topShapes)*|hBot|.  Symmetric grouping of
//  hBot too would cut further; test the one-sided version first — simplest correct step.)
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <array>
#include <vector>
#include <algorithm>
#include <map>
#include <random>
static int C,M,FULLC;
static std::vector<std::array<int,8>> CP; static std::vector<std::vector<int>> permMask;
using Part=std::array<uint16_t,8>;
static void initPerms(){std::vector<int>p(C);for(int i=0;i<C;++i)p[i]=i;do{std::array<int,8>a{};for(int i=0;i<C;++i)a[i]=p[i];CP.push_back(a);}while(std::next_permutation(p.begin(),p.end()));permMask.assign(CP.size(),std::vector<int>(1<<C,0));for(size_t pi=0;pi<CP.size();++pi)for(int m=0;m<(1<<C);++m){int r=0;for(int b=m;b;b&=b-1){int i=__builtin_ctz(b);r|=1<<CP[pi][i];}permMask[pi][m]=r;}}
// canon a full M-pair multiset (target) under S_C x S_C
static std::array<uint16_t,12> canonFull(const std::array<uint16_t,12>&v){
    std::array<uint16_t,12> best{};bool h=false;
    for(size_t rp=0;rp<CP.size();++rp){const int*px=permMask[rp].data();for(size_t cp=0;cp<CP.size();++cp){const int*py=permMask[cp].data();
        std::array<uint16_t,12> c{};for(int i=0;i<M;++i){int t=v[i];c[i]=(uint16_t)(px[t&FULLC]|(py[(t>>C)&FULLC]<<C));}
        std::sort(c.begin(),c.begin()+M);if(!h||memcmp(c.data(),best.data(),M*2)<0){best=c;h=true;}}}
    return best;}
static Part canonPart(const Part&part){Part best{};bool h=false;for(size_t rp=0;rp<CP.size();++rp){const int*px=permMask[rp].data();for(size_t cp=0;cp<CP.size();++cp){const int*py=permMask[cp].data();Part c{};for(int i=0;i<C;++i){int t=part[i];c[i]=(uint16_t)(px[t&FULLC]|(py[(t>>C)&FULLC]<<C));}std::sort(c.begin(),c.begin()+C);if(!h||memcmp(c.data(),best.data(),C*2)<0){best=c;h=true;}}}return best;}
static std::array<uint16_t,12> merge(const Part&tp,const Part&bp){std::array<uint16_t,12>v{};for(int i=0;i<C;++i){v[i]=tp[i];v[C+i]=bp[i];}std::sort(v.begin(),v.begin()+M);return v;}

int main(int argc,char**argv){
    C=atoi(argv[1]);M=2*C;FULLC=(1<<C)-1;initPerms();
    std::mt19937 rng(argc>2?atoi(argv[2]):1);
    // build a random "side histogram": a set of random C-pair parts with depth-2 masks (popcount2)
    auto randpart=[&](){Part p{};for(int i=0;i<C;++i){auto rm=[&](){int m=0,c=0;while(c<2){int b=1<<(rng()%C);if(!(m&b)){m|=b;c++;}}return m;};p[i]=(uint16_t)(rm()|(rm()<<C));}std::sort(p.begin(),p.begin()+C);return p;};
    std::map<Part,uint64_t> hTop,hBot;
    for(int i=0;i<40;++i){hTop[randpart()]+=1+(rng()%5);}
    for(int i=0;i<40;++i){hBot[randpart()]+=1+(rng()%5);}
    // brute
    std::map<std::array<uint16_t,12>,uint64_t> tb;
    for(auto&tp:hTop)for(auto&bp:hBot)tb[canonFull(merge(tp.first,bp.first))]+=tp.second*bp.second;
    // agg: group hTop by shape
    std::map<Part,uint64_t> shapeW; std::map<Part,Part> shapeRep;
    for(auto&tp:hTop){Part s=canonPart(tp.first);shapeW[s]+=tp.second;if(!shapeRep.count(s))shapeRep[s]=tp.first;}
    std::map<std::array<uint16_t,12>,uint64_t> ta;
    for(auto&sh:shapeW){const Part&tp0=shapeRep[sh.first];for(auto&bp:hBot)ta[canonFull(merge(tp0,bp.first))]+=sh.second*bp.second;}
    // compare
    bool ok = (tb==ta);
    printf("C=%d: bruteTargets=%zu aggTargets=%zu  topShapes=%zu (hTop=%zu)  IDENTITY=%s\n",
        C,tb.size(),ta.size(),shapeW.size(),hTop.size(), ok?"HOLDS":"*** FAILS ***");
    return ok?0:1;
}
