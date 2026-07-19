#include <algorithm>
#include <array>
#include <cassert>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <numeric>
#include <unordered_map>
#include <utility>
#include <vector>

#include <boost/multiprecision/cpp_int.hpp>

using boost::multiprecision::cpp_int;

namespace {
constexpr int MAX_C = 5;
constexpr int MAX_CELLS = MAX_C*MAX_C*MAX_C;
constexpr int MAX_WORDS = (2*MAX_CELLS + 63)/64;

struct Key {
    std::array<std::uint64_t, MAX_WORDS> w{};
    bool operator==(Key const& o) const noexcept { return w == o.w; }
};
struct KeyHash {
    std::size_t operator()(Key const& k) const noexcept {
        std::uint64_t h=0x9e3779b97f4a7c15ULL;
        for (auto x:k.w) {
            x ^= x >> 30; x *= 0xbf58476d1ce4e5b9ULL;
            x ^= x >> 27; x *= 0x94d049bb133111ebULL;
            x ^= x >> 31;
            h ^= x + 0x9e3779b97f4a7c15ULL + (h<<6) + (h>>2);
        }
        return (std::size_t)h;
    }
};

inline Key pack_cells(const std::array<std::uint8_t,MAX_CELLS>& a, int C) {
    Key k;
    int n=C*C*C;
    for(int i=0;i<n;i++) {
        int bit=2*i;
        k.w[bit>>6] |= (std::uint64_t(a[i]) << (bit&63));
        if ((bit&63)==63) k.w[(bit>>6)+1] |= (std::uint64_t(a[i]) >> 1);
    }
    return k;
}
inline std::array<std::uint8_t,MAX_CELLS> unpack_cells(const Key& k,int C) {
    std::array<std::uint8_t,MAX_CELLS> a{};
    int n=C*C*C;
    for(int i=0;i<n;i++) {
        int bit=2*i;
        std::uint64_t v=k.w[bit>>6]>>(bit&63);
        if ((bit&63)==63) v |= k.w[(bit>>6)+1]<<1;
        a[i]=std::uint8_t(v&3U);
    }
    return a;
}

struct Sig {
    std::array<std::uint8_t, 1 + 3*MAX_C*MAX_C> v{};
    int len=0;
    bool operator<(Sig const&o) const noexcept {
        return std::lexicographical_compare(v.begin(),v.begin()+len,o.v.begin(),o.v.begin()+o.len);
    }
    bool operator==(Sig const&o) const noexcept {
        return len==o.len && std::equal(v.begin(),v.begin()+len,o.v.begin());
    }
};

struct Canonicalizer {
    int C;
    bool allow_swap;
    std::uint64_t recursion_nodes=0;

    explicit Canonicalizer(int c,bool swap=true):C(c),allow_swap(swap){}

    static int idx(int C,int r,int a,int b){ return (r*C+a)*C+b; }

    void normalize_colors(std::array<std::uint8_t,MAX_C>& col) const {
        std::array<int,MAX_C> first; first.fill(999);
        for(int i=0;i<C;i++) first[col[i]]=std::min(first[col[i]],i);
        std::vector<std::pair<int,int>> labs;
        for(int c=0;c<C;c++) if(first[c]!=999) labs.push_back({first[c],c});
        std::sort(labs.begin(),labs.end());
        std::array<std::uint8_t,MAX_C> mp{};
        for(int j=0;j<(int)labs.size();j++) mp[labs[j].second]=j;
        for(int i=0;i<C;i++) col[i]=mp[col[i]];
    }

    int num_colors(const std::array<std::uint8_t,MAX_C>& c) const {
        int m=-1; for(int i=0;i<C;i++) m=std::max(m,(int)c[i]); return m+1;
    }

    bool refine_part(const std::array<std::uint8_t,MAX_CELLS>& x,
                     int part,
                     const std::array<std::uint8_t,MAX_C>& cr,
                     const std::array<std::uint8_t,MAX_C>& ca,
                     const std::array<std::uint8_t,MAX_C>& cb,
                     std::array<std::uint8_t,MAX_C>& out) const {
        std::array<Sig,MAX_C> sigs;
        int nr=num_colors(cr), na=num_colors(ca), nb=num_colors(cb);
        for(int v=0;v<C;v++) {
            Sig s;
            auto const& own = (part==0?cr:(part==1?ca:cb));
            s.v[0]=own[v];
            int dim1=(part==0?na:nr);
            int dim2=(part==2?na:nb); // part0: na,nb; part1: nr,nb; part2: nr,na
            if(part==1){dim1=nr;dim2=nb;}
            if(part==2){dim1=nr;dim2=na;}
            int pos=1;
            for(int c1=0;c1<dim1;c1++) for(int c2=0;c2<dim2;c2++) for(int val=0;val<3;val++) {
                int count=0;
                if(part==0) {
                    for(int a=0;a<C;a++) if(ca[a]==c1)
                        for(int b=0;b<C;b++) if(cb[b]==c2)
                            count += (x[idx(C,v,a,b)]==val);
                } else if(part==1) {
                    for(int r=0;r<C;r++) if(cr[r]==c1)
                        for(int b=0;b<C;b++) if(cb[b]==c2)
                            count += (x[idx(C,r,v,b)]==val);
                } else {
                    for(int r=0;r<C;r++) if(cr[r]==c1)
                        for(int a=0;a<C;a++) if(ca[a]==c2)
                            count += (x[idx(C,r,a,v)]==val);
                }
                s.v[pos++]=std::uint8_t(count);
            }
            s.len=pos;
            sigs[v]=s;
        }
        std::vector<int> ord(C); std::iota(ord.begin(),ord.end(),0);
        std::sort(ord.begin(),ord.end(),[&](int i,int j){
            if(sigs[i]==sigs[j]) return i<j;
            return sigs[i]<sigs[j];
        });
        std::uint8_t color=0;
        out[ord[0]]=0;
        for(int k=1;k<C;k++) {
            if(!(sigs[ord[k]]==sigs[ord[k-1]])) ++color;
            out[ord[k]]=color;
        }
        auto const& old=(part==0?cr:(part==1?ca:cb));
        for(int i=0;i<C;i++) if(out[i]!=old[i]) return true;
        return false;
    }

    void refine(const std::array<std::uint8_t,MAX_CELLS>& x,
                std::array<std::uint8_t,MAX_C>& cr,
                std::array<std::uint8_t,MAX_C>& ca,
                std::array<std::uint8_t,MAX_C>& cb) const {
        // Simultaneous color refinement.  Recompute all three parts from the same old partition.
        for(;;){
            std::array<std::uint8_t,MAX_C> nr{},na{},nb{};
            refine_part(x,0,cr,ca,cb,nr);
            refine_part(x,1,cr,ca,cb,na);
            refine_part(x,2,cr,ca,cb,nb);
            bool changed=(nr!=cr)||(na!=ca)||(nb!=cb);
            cr=nr;ca=na;cb=nb;
            if(!changed) break;
        }
    }

    Key encode_discrete(const std::array<std::uint8_t,MAX_CELLS>& x,
                        const std::array<std::uint8_t,MAX_C>& cr,
                        const std::array<std::uint8_t,MAX_C>& ca,
                        const std::array<std::uint8_t,MAX_C>& cb) const {
        std::array<int,MAX_C> rr{},aa{},bb{};
        for(int i=0;i<C;i++){rr[cr[i]]=i;aa[ca[i]]=i;bb[cb[i]]=i;}
        std::array<std::uint8_t,MAX_CELLS> y{};
        for(int r=0;r<C;r++)for(int a=0;a<C;a++)for(int b=0;b<C;b++)
            y[idx(C,r,a,b)]=x[idx(C,rr[r],aa[a],bb[b])];
        return pack_cells(y,C);
    }

    bool key_less(const Key&a,const Key&b) const {
        // packed little-endian by cell index; compare decoded lexicographically
        auto x=unpack_cells(a,C), y=unpack_cells(b,C);
        int n=C*C*C;
        for(int i=0;i<n;i++){if(x[i]!=y[i]) return x[i]<y[i];}
        return false;
    }

    Key recurse(const std::array<std::uint8_t,MAX_CELLS>& x,
                std::array<std::uint8_t,MAX_C> cr,
                std::array<std::uint8_t,MAX_C> ca,
                std::array<std::uint8_t,MAX_C> cb) {
        ++recursion_nodes;
        refine(x,cr,ca,cb);
        if(num_colors(cr)==C && num_colors(ca)==C && num_colors(cb)==C)
            return encode_discrete(x,cr,ca,cb);

        int bestpart=-1,bestcolor=-1,bestsize=999;
        auto inspect=[&](int part,const auto& col){
            std::array<int,MAX_C> cnt{};
            for(int i=0;i<C;i++)cnt[col[i]]++;
            for(int c=0;c<C;c++) if(cnt[c]>1 && cnt[c]<bestsize){bestsize=cnt[c];bestpart=part;bestcolor=c;}
        };
        inspect(0,cr);inspect(1,ca);inspect(2,cb);
        assert(bestpart>=0);

        bool first=true; Key best{};
        auto branch=[&](int v){
            auto xr=cr,xa=ca,xb=cb;
            auto& cc=(bestpart==0?xr:(bestpart==1?xa:xb));
            // Split selected singleton from its cell. Unique high color; subsequent refinement canonicalizes.
            cc[v]=std::uint8_t(num_colors(cc));
            Key z=recurse(x,xr,xa,xb);
            if(first || key_less(z,best)){best=z;first=false;}
        };
        auto const& cc=(bestpart==0?cr:(bestpart==1?ca:cb));
        for(int v=0;v<C;v++) if(cc[v]==bestcolor) branch(v);
        return best;
    }

    Key canonical_one(const std::array<std::uint8_t,MAX_CELLS>& x) {
        std::array<std::uint8_t,MAX_C> cr{},ca{},cb{};
        return recurse(x,cr,ca,cb);
    }

    Key canonical(const std::array<std::uint8_t,MAX_CELLS>& x) {
        Key a=canonical_one(x);
        if(!allow_swap) return a;
        std::array<std::uint8_t,MAX_CELLS> y{};
        for(int r=0;r<C;r++)for(int i=0;i<C;i++)for(int j=0;j<C;j++)
            y[idx(C,r,i,j)]=x[idx(C,r,j,i)];
        Key b=canonical_one(y);
        return key_less(b,a)?b:a;
    }
};

std::vector<std::array<std::uint8_t,MAX_C>> all_perms(int C){
    std::vector<std::array<std::uint8_t,MAX_C>> out;
    std::vector<int> p(C);std::iota(p.begin(),p.end(),0);
    do{std::array<std::uint8_t,MAX_C> q{};for(int i=0;i<C;i++)q[i]=p[i];out.push_back(q);}while(std::next_permutation(p.begin(),p.end()));
    return out;
}

std::uint64_t slice_perm(const std::array<std::uint8_t,MAX_CELLS>& x,int C,int r,
                         const std::vector<std::array<std::uint8_t,MAX_C>>& perms){
    std::uint64_t ans=0;
    for(auto const&p:perms){
        std::uint64_t z=1;
        for(int a=0;a<C;a++)z*=x[Canonicalizer::idx(C,r,a,p[a])];
        ans+=z;
    }
    return ans;
}

struct RunResult { cpp_int total; std::vector<std::size_t> states; };
RunResult run(int C,bool swap=true,int stop=-1){
    auto perms=all_perms(C);
    struct Move{std::array<std::uint8_t,MAX_C> p,q;};
    std::vector<Move> moves;
    for(auto const&p:perms)for(auto const&q:perms)moves.push_back({p,q});

    Canonicalizer canon(C,swap);
    std::array<std::uint8_t,MAX_CELLS> zero{};
    Key start=canon.canonical(zero);
    std::unordered_map<Key,cpp_int,KeyHash> cur,nxt;
    cur.emplace(start,cpp_int(1));
    RunResult res;res.states.push_back(1);
    int maxm=(stop>=0?stop:2*C);
    for(int m=0;m<maxm;m++){
        auto t0=std::chrono::steady_clock::now();
        nxt.clear();
        // rough reserve; avoid pathological over-allocation
        nxt.reserve(cur.size()*4+100);
        std::uint64_t valid_moves=0,canon_calls=0;
        canon.recursion_nodes=0;
        for(auto const& kv:cur){
            auto x=unpack_cells(kv.first,C);
            std::array<std::uint8_t,MAX_C*MAX_C> da{},db{};
            for(int r=0;r<C;r++)for(int a=0;a<C;a++)for(int b=0;b<C;b++){
                auto v=x[Canonicalizer::idx(C,r,a,b)];
                da[r*C+a]+=v;db[r*C+b]+=v;
            }
            for(auto const&mv:moves){
                bool ok=true;
                for(int r=0;r<C;r++) if(da[r*C+mv.p[r]]>=2 || db[r*C+mv.q[r]]>=2){ok=false;break;}
                if(!ok)continue;
                ++valid_moves;
                auto y=x;
                for(int r=0;r<C;r++) ++y[Canonicalizer::idx(C,r,mv.p[r],mv.q[r])];
                Key z=canon.canonical(y);++canon_calls;
                nxt[z]+=kv.second;
            }
        }
        cur.swap(nxt);
        res.states.push_back(cur.size());
        auto t1=std::chrono::steady_clock::now();
        double sec=std::chrono::duration<double>(t1-t0).count();
        std::cerr<<"C="<<C<<" m="<<(m+1)<<" states="<<cur.size()<<" valid="<<valid_moves
                 <<" canon="<<canon_calls<<" recnodes="<<canon.recursion_nodes<<" sec="<<sec<<"\n";
    }
    if(maxm==2*C){
        cpp_int total=0;
        for(auto const&kv:cur){
            auto x=unpack_cells(kv.first,C);
            std::uint64_t w=1;
            for(int r=0;r<C;r++)w*=slice_perm(x,C,r,perms);
            total += kv.second*w;
        }
        res.total=total;
    }
    return res;
}

} // namespace

int main(int argc,char**argv){
    int C=argc>1?std::atoi(argv[1]):3;
    int stop=argc>2?std::atoi(argv[2]):-1;
    bool swap=argc>3?std::atoi(argv[3])!=0:true;
    if(C<2||C>MAX_C){std::cerr<<"C must be 2..5\n";return 2;}
    auto r=run(C,swap,stop);
    std::cout<<"states:";for(auto x:r.states)std::cout<<" "<<x;std::cout<<"\n";
    if(stop<0||stop==2*C)std::cout<<"N("<<C<<")="<<r.total<<"\n";
}
