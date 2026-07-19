#include <algorithm>
#include <array>
#include <cassert>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <numeric>
#include <unordered_map>
#include <vector>
#include <omp.h>
#include <boost/multiprecision/cpp_int.hpp>
using boost::multiprecision::cpp_int;

namespace {
constexpr int MAX_C=6;
constexpr int MAX_E=2*MAX_C;
constexpr int BITS_PER_E=6; // degree 2 bits, mate+1 4 bits (0 means none)
constexpr int MAX_BITS=MAX_C*MAX_E*BITS_PER_E;
constexpr int MAX_WORDS=(MAX_BITS+63)/64;

struct State {
    std::array<std::uint8_t,MAX_C*MAX_E> deg{};
    std::array<std::int8_t,MAX_C*MAX_E> mate{};
    State(){mate.fill(-1);}    
};
struct Key {std::array<std::uint64_t,MAX_WORDS>w{};bool operator==(Key const&o)const noexcept{return w==o.w;}};
struct KeyHash{std::size_t operator()(Key const&k)const noexcept{std::uint64_t h=0x9e3779b97f4a7c15ULL;for(auto x:k.w){x^=x>>30;x*=0xbf58476d1ce4e5b9ULL;x^=x>>27;x*=0x94d049bb133111ebULL;x^=x>>31;h^=x+0x9e3779b97f4a7c15ULL+(h<<6)+(h>>2);}return (std::size_t)h;}};

inline int ei(int C,int r,int e){return r*(2*C)+e;}
Key pack(const State&s,int C){
    Key k;int bit=0;
    for(int r=0;r<C;r++)for(int e=0;e<2*C;e++){
        std::uint64_t val=std::uint64_t(s.deg[ei(C,r,e)]) | (std::uint64_t(s.mate[ei(C,r,e)]+1)<<2);
        int wi=bit>>6,off=bit&63;k.w[wi]|=val<<off;if(off>64-BITS_PER_E)k.w[wi+1]|=val>>(64-off);bit+=BITS_PER_E;
    }
    return k;
}
State unpack(const Key&k,int C){
    State s;int bit=0;
    for(int r=0;r<C;r++)for(int e=0;e<2*C;e++){
        int wi=bit>>6,off=bit&63;std::uint64_t val=k.w[wi]>>off;if(off>64-BITS_PER_E)val|=k.w[wi+1]<<(64-off);val&=63;
        s.deg[ei(C,r,e)]=val&3;s.mate[ei(C,r,e)]=std::int8_t((val>>2)-1);bit+=BITS_PER_E;
    }
    return s;
}

struct Sig{std::vector<int>v;bool operator<(Sig const&o)const{return v<o.v;}bool operator==(Sig const&o)const{return v==o.v;}};

struct Canon {
    int C;bool allow_swap;std::uint64_t nodes=0;
    Canon(int c,bool sw=true):C(c),allow_swap(sw){}
    int ncol(auto const&x)const{int m=-1;for(int i=0;i<C;i++)m=std::max(m,(int)x[i]);return m+1;}

    Sig sig_band(const State&s,int r,auto const&cr,auto const&ca,auto const&cb)const{
        Sig z;z.v.push_back(cr[r]);int na=ncol(ca),nb=ncol(cb);
        // endpoint degree counts
        for(int side=0;side<2;side++){
            int nc=side==0?na:nb;
            for(int cc=0;cc<nc;cc++)for(int d=0;d<3;d++){
                int cnt=0;for(int x=0;x<C;x++)if((side==0?ca[x]:cb[x])==cc && s.deg[ei(C,r,side*C+x)]==d)cnt++;
                z.v.push_back(cnt);
            }
        }
        // path-pair types within band, as unordered endpoint categories
        struct Cat{int side,col;bool operator<(Cat const&o)const{return side<o.side||(side==o.side&&col<o.col);} };
        std::vector<std::array<int,4>> types;
        for(int e=0;e<2*C;e++) if(s.deg[ei(C,r,e)]==1){int m=s.mate[ei(C,r,e)];if(e<m){
            Cat x{e/C,(e<C?ca[e]:cb[e-C])};Cat y{m/C,(m<C?ca[m]:cb[m-C])};if(y<x)std::swap(x,y);types.push_back({x.side,x.col,y.side,y.col});
        }}
        std::sort(types.begin(),types.end());
        z.v.push_back(-1);for(auto const&t:types)for(int q:t)z.v.push_back(q);
        return z;
    }
    Sig sig_color(const State&s,int side,int x,auto const&cr,auto const&ca,auto const&cb)const{
        Sig z;auto const&own=side==0?ca:cb;z.v.push_back(own[x]);int nr=ncol(cr),na=ncol(ca),nb=ncol(cb);
        for(int rc=0;rc<nr;rc++)for(int d=0;d<3;d++){
            int cnt=0;for(int r=0;r<C;r++)if(cr[r]==rc&&s.deg[ei(C,r,side*C+x)]==d)cnt++;z.v.push_back(cnt);
        }
        // degree-1 mate categories
        std::vector<std::array<int,3>> mts;
        for(int r=0;r<C;r++)if(s.deg[ei(C,r,side*C+x)]==1){int m=s.mate[ei(C,r,side*C+x)];int ms=m/C,mc=(m<C?ca[m]:cb[m-C]);mts.push_back({cr[r],ms,mc});}
        std::sort(mts.begin(),mts.end());z.v.push_back(-1);for(auto const&t:mts)for(int q:t)z.v.push_back(q);
        // include dimensions to avoid accidental signature collisions after refinements
        z.v.push_back(-2);z.v.push_back(na);z.v.push_back(nb);
        return z;
    }
    template<class Arr>
    Arr recolor(std::array<Sig,MAX_C> const&sigs)const{
        std::vector<int>o(C);std::iota(o.begin(),o.end(),0);std::sort(o.begin(),o.end(),[&](int i,int j){if(sigs[i]==sigs[j])return i<j;return sigs[i]<sigs[j];});
        Arr out{};int c=0;out[o[0]]=0;for(int i=1;i<C;i++){if(!(sigs[o[i]]==sigs[o[i-1]]))c++;out[o[i]]=c;}return out;
    }
    void refine(const State&s,std::array<std::uint8_t,MAX_C>&cr,std::array<std::uint8_t,MAX_C>&ca,std::array<std::uint8_t,MAX_C>&cb)const{
        for(;;){std::array<Sig,MAX_C>sr,sa,sb;for(int i=0;i<C;i++){sr[i]=sig_band(s,i,cr,ca,cb);sa[i]=sig_color(s,0,i,cr,ca,cb);sb[i]=sig_color(s,1,i,cr,ca,cb);}auto nr=recolor<std::array<std::uint8_t,MAX_C>>(sr),na=recolor<std::array<std::uint8_t,MAX_C>>(sa),nb=recolor<std::array<std::uint8_t,MAX_C>>(sb);bool ch=nr!=cr||na!=ca||nb!=cb;cr=nr;ca=na;cb=nb;if(!ch)break;}
    }
    Key encode(const State&s,auto const&cr,auto const&ca,auto const&cb)const{
        std::array<int,MAX_C>rr{},aa{},bb{};for(int i=0;i<C;i++){rr[cr[i]]=i;aa[ca[i]]=i;bb[cb[i]]=i;}
        State t;
        for(int nr=0;nr<C;nr++){
            int orr=rr[nr];
            for(int ne=0;ne<2*C;ne++){
                int oe=(ne<C?aa[ne]:C+bb[ne-C]);int oldi=ei(C,orr,oe),newi=ei(C,nr,ne);t.deg[newi]=s.deg[oldi];int om=s.mate[oldi];if(om>=0)t.mate[newi]=(om<C?ca[om]:C+cb[om-C]);
            }
        }
        return pack(t,C);
    }
    bool less(const Key&a,const Key&b)const{
        State x=unpack(a,C),y=unpack(b,C);for(int r=0;r<C;r++)for(int e=0;e<2*C;e++){int i=ei(C,r,e);if(x.deg[i]!=y.deg[i])return x.deg[i]<y.deg[i];if(x.mate[i]!=y.mate[i])return x.mate[i]<y.mate[i];}return false;
    }
    Key rec(const State&s,std::array<std::uint8_t,MAX_C>cr,std::array<std::uint8_t,MAX_C>ca,std::array<std::uint8_t,MAX_C>cb){
        nodes++;refine(s,cr,ca,cb);if(ncol(cr)==C&&ncol(ca)==C&&ncol(cb)==C)return encode(s,cr,ca,cb);
        int bp=-1,bc=-1,bs=999;auto scan=[&](int p,auto const&x){std::array<int,MAX_C>cnt{};for(int i=0;i<C;i++)cnt[x[i]]++;for(int c=0;c<C;c++)if(cnt[c]>1&&cnt[c]<bs){bp=p;bc=c;bs=cnt[c];}};scan(0,cr);scan(1,ca);scan(2,cb);
        bool first=true;Key best{};auto const&base=bp==0?cr:(bp==1?ca:cb);for(int v=0;v<C;v++)if(base[v]==bc){auto xr=cr,xa=ca,xb=cb;auto&z=bp==0?xr:(bp==1?xa:xb);z[v]=ncol(z);Key q=rec(s,xr,xa,xb);if(first||less(q,best)){best=q;first=false;}}return best;
    }
    Key one(const State&s){std::array<std::uint8_t,MAX_C>z{};return rec(s,z,z,z);} // copies are intentional values
    State swapped(const State&s)const{
        State t;for(int r=0;r<C;r++)for(int e=0;e<2*C;e++){int ne=e<C?C+e:e-C;int i=ei(C,r,e),j=ei(C,r,ne);t.deg[j]=s.deg[i];int m=s.mate[i];if(m>=0)t.mate[j]=(m<C?C+m:m-C);}return t;
    }
    Key canonical(const State&s){Key a=one(s);if(!allow_swap)return a;Key b=one(swapped(s));return less(b,a)?b:a;}
};

std::vector<std::array<std::uint8_t,MAX_C>> perms(int C){std::vector<std::array<std::uint8_t,MAX_C>>v;std::vector<int>p(C);std::iota(p.begin(),p.end(),0);do{std::array<std::uint8_t,MAX_C>q{};for(int i=0;i<C;i++)q[i]=p[i];v.push_back(q);}while(std::next_permutation(p.begin(),p.end()));return v;}

// Add one edge in a band. Returns factor 1 or 2, and 0 if invalid.
int add_edge(State&s,int C,int r,int a,int b){
    int u=a,v=C+b,iu=ei(C,r,u),iv=ei(C,r,v);int du=s.deg[iu],dv=s.deg[iv];if(du>=2||dv>=2)return 0;
    if(du==0&&dv==0){s.deg[iu]=s.deg[iv]=1;s.mate[iu]=v;s.mate[iv]=u;return 1;}
    if(du==1&&dv==0){int mu=s.mate[iu];s.deg[iu]=2;s.mate[iu]=-1;s.deg[iv]=1;s.mate[iv]=mu;s.mate[ei(C,r,mu)]=v;return 1;}
    if(du==0&&dv==1){int mv=s.mate[iv];s.deg[iv]=2;s.mate[iv]=-1;s.deg[iu]=1;s.mate[iu]=mv;s.mate[ei(C,r,mv)]=u;return 1;}
    int mu=s.mate[iu],mv=s.mate[iv];s.deg[iu]=s.deg[iv]=2;s.mate[iu]=s.mate[iv]=-1;
    if(mu==v){assert(mv==u);return 2;}
    s.mate[ei(C,r,mu)]=mv;s.mate[ei(C,r,mv)]=mu;return 1;
}


std::array<std::uint8_t,MAX_C> invperm(const std::array<std::uint8_t,MAX_C>&p,int C){std::array<std::uint8_t,MAX_C>q{};for(int i=0;i<C;i++)q[p[i]]=i;return q;}
std::array<std::uint8_t,MAX_C> composeperm(const std::array<std::uint8_t,MAX_C>&p,const std::array<std::uint8_t,MAX_C>&q,int C){std::array<std::uint8_t,MAX_C>z{};for(int i=0;i<C;i++)z[i]=p[q[i]];return z;}
struct PairKey {std::array<std::uint8_t,2*MAX_C> x{};bool operator==(PairKey const&o)const noexcept{return x==o.x;}};
struct PairHash{std::size_t operator()(PairKey const&k)const noexcept{std::uint64_t h=0;for(auto v:k.x)h=h*13+v+1;return h;}};
PairKey canonical_pair(const std::array<std::uint8_t,MAX_C>&p,const std::array<std::uint8_t,MAX_C>&q,int C,const std::vector<std::array<std::uint8_t,MAX_C>>&ps){
    PairKey best{};bool first=true;
    for(auto const&g:ps){auto gi=invperm(g,C);auto gp=composeperm(composeperm(g,p,C),gi,C);auto gq=composeperm(composeperm(g,q,C),gi,C);PairKey a{},b{};for(int i=0;i<C;i++){a.x[i]=gp[i];a.x[MAX_C+i]=gq[i];b.x[i]=gq[i];b.x[MAX_C+i]=gp[i];}if(first||a.x<best.x){best=a;first=false;}if(b.x<best.x)best=b;}
    return best;
}
static inline void checked_add(std::uint64_t &dst, std::uint64_t a, std::uint64_t b=1){
    __uint128_t v=(__uint128_t)a*b + dst;
    if(v>std::numeric_limits<std::uint64_t>::max()){
        std::cerr<<"uint64 overflow\n"; std::abort();
    }
    dst=(std::uint64_t)v;
}
std::unordered_map<Key,std::uint64_t,KeyHash> initialize_two_symbols(int C,bool sw,const std::vector<std::array<std::uint8_t,MAX_C>>&ps){
    std::unordered_map<PairKey,std::uint64_t,PairHash> mult;
    for(auto const&p:ps)for(auto const&q:ps)mult[canonical_pair(p,q,C,ps)]++;
    Canon can(C,sw);std::unordered_map<Key,std::uint64_t,KeyHash> cur;
    std::uint64_t first_mult=std::uint64_t(ps.size())*ps.size();
    for(auto const&kv:mult){std::array<std::uint8_t,MAX_C>p{},q{};for(int i=0;i<C;i++){p[i]=kv.first.x[i];q[i]=kv.first.x[MAX_C+i];}State st;int fac=1;for(int r=0;r<C;r++){int a=add_edge(st,C,r,r,r);assert(a==1);}for(int r=0;r<C;r++){int a=add_edge(st,C,r,p[r],q[r]);assert(a);fac*=a;}checked_add(cur[can.canonical(st)], first_mult, kv.second*fac);}
    return cur;
}
struct Result{cpp_int total;std::vector<std::size_t>states;};
Result run(int C,bool sw=true,int stop=-1){
    auto ps=perms(C);struct Move{std::array<std::uint8_t,MAX_C>p,q;};std::vector<Move>ms;for(auto const&p:ps)for(auto const&q:ps)ms.push_back({p,q});
    Canon can(C,sw);State z;std::unordered_map<Key,std::uint64_t,KeyHash>cur,nxt;Result out;out.states.push_back(1);int lim=stop>=0?stop:2*C;int start_m=0;
    if(C>=5 && lim>=2){cur=initialize_two_symbols(C,sw,ps);out.states.push_back(1);out.states.push_back(cur.size());start_m=2;}else{cur.emplace(can.canonical(z),std::uint64_t(1));}
    for(int m=start_m;m<lim;m++){
        auto t0=std::chrono::steady_clock::now();nxt.clear();nxt.reserve(cur.size()*4+100);std::uint64_t valid=0;can.nodes=0;
        std::vector<std::pair<Key,std::uint64_t>> src; src.reserve(cur.size()); for(auto const&kv:cur) src.push_back(kv);
        int nt=std::min(32, omp_get_max_threads());
        std::vector<std::unordered_map<Key,std::uint64_t,KeyHash>> locals(nt);
        std::vector<std::uint64_t> vcnt(nt), ncnt(nt);
        #pragma omp parallel num_threads(nt)
        { int tid=omp_get_thread_num(); Canon lc(C,sw); auto &lm=locals[tid]; lm.reserve(src.size()*2/nt+100);
          #pragma omp for schedule(dynamic,8)
          for(std::size_t si=0;si<src.size();si++){auto const&kv=src[si];State s=unpack(kv.first,C);for(auto const&mv:ms){State t=s;int fac=1;for(int r=0;r<C;r++){int q=add_edge(t,C,r,mv.p[r],mv.q[r]);if(!q){fac=0;break;}fac*=q;}if(!fac)continue;vcnt[tid]++;Key k=lc.canonical(t);checked_add(lm[k],kv.second,fac);} } ncnt[tid]=lc.nodes; }
        for(int t=0;t<nt;t++){valid+=vcnt[t];can.nodes+=ncnt[t];for(auto const&kv:locals[t])checked_add(nxt[kv.first],kv.second);}
        cur.swap(nxt);out.states.push_back(cur.size());double sec=std::chrono::duration<double>(std::chrono::steady_clock::now()-t0).count();std::cerr<<"C="<<C<<" m="<<m+1<<" states="<<cur.size()<<" valid="<<valid<<" nodes="<<can.nodes<<" sec="<<sec<<"\n";
    }
    if(lim==2*C){assert(cur.size()==1);out.total=cur.begin()->second;}return out;
}
}
int main(int argc,char**argv){int C=argc>1?std::atoi(argv[1]):3;int stop=argc>2?std::atoi(argv[2]):-1;bool sw=argc>3?std::atoi(argv[3])!=0:true;if(C<2||C>6)return 2;auto r=run(C,sw,stop);std::cout<<"states:";for(auto x:r.states)std::cout<<" "<<x;std::cout<<"\n";if(stop<0||stop==2*C)std::cout<<"N("<<C<<")="<<r.total<<"\n";}
