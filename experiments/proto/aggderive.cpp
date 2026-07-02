// Derive the CORRECT aggregation by testing candidate identities against brute, for ONE class.
// Setup: hTop, hBot are weighted sets of C-pair parts (column-labeled, in shared C-col frame).
// brute: target[canon(tp ⊎ bp)] += w_tp w_bp  over all pairs.
//
// Candidate A (orbit-rep top + transform bot by coset reps):
//   group hTop into G-orbits; for orbit (rep tp0, members = {g·tp0}), the contribution of the
//   WHOLE orbit paired with a fixed bp equals sum over distinct members tp of canon(tp ⊎ bp).
//   We test: does grouping help if we, per orbit, iterate its DISTINCT members (dedup'd) but
//   reuse canon? No reduction. So instead test Candidate B:
//
// Candidate B (both sides as orbits + full G alignment, divided by stabilizer):
//   For top-orbit (tp0, size St) and bot-orbit (bp0, size Sb), the multiset of canon(tp⊎bp)
//   over the St*Sb member pairs = for each g in G: canon(tp0 ⊎ g·bp0), each such g-term
//   counted (St*Sb*|Stab(tp0)∩...|)/|G| times.  Hard to get exact -> instead we EMPIRICALLY
//   build, per orbit-pair, the histogram of canon(tp0 ⊎ g·bp0) over all g in G, and find the
//   scalar so that it matches the brute restricted to that orbit-pair.
//
// We just MEASURE: for each (top-orbit, bot-orbit), is  brute_restricted  proportional to
//   H(g) := histogram over g in G of canon(tp0 ⊎ g·bp0) ?   and what's the constant?
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
using Part=std::array<uint16_t,8>; using Full=std::array<uint16_t,12>;
static void initPerms(){std::vector<int>p(C);for(int i=0;i<C;++i)p[i]=i;do{std::array<int,8>a{};for(int i=0;i<C;++i)a[i]=p[i];CP.push_back(a);}while(std::next_permutation(p.begin(),p.end()));permMask.assign(CP.size(),std::vector<int>(1<<C,0));for(size_t pi=0;pi<CP.size();++pi)for(int m=0;m<(1<<C);++m){int r=0;for(int b=m;b;b&=b-1){int i=__builtin_ctz(b);r|=1<<CP[pi][i];}permMask[pi][m]=r;}}
static Full canonFull(const Full&v){Full best{};bool h=false;for(size_t rp=0;rp<CP.size();++rp){const int*px=permMask[rp].data();for(size_t cp=0;cp<CP.size();++cp){const int*py=permMask[cp].data();Full c{};for(int i=0;i<M;++i){int t=v[i];c[i]=(uint16_t)(px[t&FULLC]|(py[(t>>C)&FULLC]<<C));}std::sort(c.begin(),c.begin()+M);if(!h||memcmp(c.data(),best.data(),M*2)<0){best=c;h=true;}}}return best;}
// apply group element (rp,cp) to a part
static Part applyG(const Part&part,int rp,int cp){const int*px=permMask[rp].data();const int*py=permMask[cp].data();Part c{};for(int i=0;i<C;++i){int t=part[i];c[i]=(uint16_t)(px[t&FULLC]|(py[(t>>C)&FULLC]<<C));}std::sort(c.begin(),c.begin()+C);return c;}
static Part canonPart(const Part&part){Part best{};bool h=false;int G=CP.size();for(int rp=0;rp<G;++rp)for(int cp=0;cp<G;++cp){Part c=applyG(part,rp,cp);if(!h||memcmp(c.data(),best.data(),C*2)<0){best=c;h=true;}}return best;}
static Full merge(const Part&tp,const Part&bp){Full v{};for(int i=0;i<C;++i){v[i]=tp[i];v[C+i]=bp[i];}std::sort(v.begin(),v.begin()+M);return v;}

int main(int argc,char**argv){
    C=atoi(argv[1]);M=2*C;FULLC=(1<<C)-1;initPerms();
    std::mt19937 rng(argc>2?atoi(argv[2]):1);
    auto randpart=[&](){Part p{};for(int i=0;i<C;++i){auto rm=[&](){int m=0,c=0;while(c<2){int b=1<<(rng()%C);if(!(m&b)){m|=b;c++;}}return m;};p[i]=(uint16_t)(rm()|(rm()<<C));}std::sort(p.begin(),p.begin()+C);return p;};
    std::map<Part,uint64_t> hTop,hBot;
    for(int i=0;i<30;++i)hTop[randpart()]+=1+(rng()%4);
    for(int i=0;i<30;++i)hBot[randpart()]+=1+(rng()%4);
    int G=CP.size(); long long G2=(long long)G*G;
    // brute
    std::map<Full,uint64_t> tb;
    for(auto&tp:hTop)for(auto&bp:hBot)tb[canonFull(merge(tp.first,bp.first))]+=tp.second*bp.second;
    // Candidate C (the principled one): canon(tp⊎bp) only depends on the JOINT orbit of (tp,bp)
    // under diagonal G.  Key the pair by ( canon-as-joint ).  Aggregate by that.
    // jointKey(tp,bp) = min over g in G of (g·tp, g·bp) packed.  Then target = canonFull of any member.
    // We test: grouping pairs by jointKey and summing weights == brute, AND #jointKeys < |hTop||hBot|.
    std::map<std::array<uint16_t,16>,uint64_t> jointGroups; // packed (canon pair)
    std::map<std::array<uint16_t,16>,Full> jointTarget;
    for(auto&tp:hTop)for(auto&bp:hBot){
        // canonical joint (diagonal G): min over g of (g tp, g bp)
        std::array<uint16_t,16> best{};bool h=false;
        for(int rp=0;rp<G;++rp)for(int cp=0;cp<G;++cp){
            Part a=applyG(tp.first,rp,cp), b=applyG(bp.first,rp,cp);
            std::array<uint16_t,16> k{};for(int i=0;i<C;++i){k[i]=a[i];k[C+i]=b[i];}
            // pack: need a fixed total order; compare first C (a) then next C (b)
            if(!h||memcmp(k.data(),best.data(),2*C*2)<0){best=k;h=true;}
        }
        jointGroups[best]+=tp.second*bp.second;
        if(!jointTarget.count(best)){Part a{},b{};for(int i=0;i<C;++i){a[i]=best[i];b[i]=best[C+i];}jointTarget[best]=canonFull(merge(a,b));}
    }
    // reconstruct target from joint groups
    std::map<Full,uint64_t> tc;
    for(auto&jg:jointGroups)tc[jointTarget[jg.first]]+=jg.second;
    bool ok=(tb==tc);
    printf("C=%d: brute=%zu jointGroups=%zu (of %zu pairs) targets=%zu  JOINT-CANON IDENTITY=%s\n",
        C,tb.size(),jointGroups.size(),hTop.size()*hBot.size(),tc.size(),ok?"HOLDS":"*** FAILS ***");
    return ok?0:1;
}
