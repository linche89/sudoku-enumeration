// Exact rank certificates for the orbit-transfer matrices used by
// global_linear_histogram_dp.cpp.  A full rank modulo one prime certifies
// the same full rank over Q.
#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <numeric>
#include <stdexcept>
#include <unordered_map>
#include <utility>
#include <vector>

constexpr int MAX_C=6, MAX_MASK=1<<MAX_C;
using Hist=std::array<std::uint8_t,MAX_MASK>;
using Perm=std::array<std::uint8_t,MAX_C>;
struct HH{std::size_t operator()(Hist const&h)const noexcept{std::uint64_t x=0x9e3779b97f4a7c15ULL;for(auto v:h)x^=(std::uint64_t)v+0x9e3779b97f4a7c15ULL+(x<<6)+(x>>2);return x;}};
struct Can{int c,full;std::array<std::vector<int>,MAX_C+1> masks;std::vector<std::array<std::uint8_t,MAX_MASK>> images;explicit Can(int c):c(c),full((1<<c)-1){for(int m=0;m<=full;m++)masks[__builtin_popcount((unsigned)m)].push_back(m);std::vector<int>p(c);std::iota(p.begin(),p.end(),0);do{std::array<std::uint8_t,MAX_MASK>im{};for(int m=0;m<=full;m++){int q=0;for(int a=0;a<c;a++)if(m&(1<<a))q|=1<<p[a];im[m]=q;}images.push_back(im);}while(std::next_permutation(p.begin(),p.end()));}Hist operator()(Hist const&h,int r)const{Hist best{};bool first=1;for(auto const&im:images){Hist z{};for(int m:masks[r])z[im[m]]=h[m];bool less=0;for(int m:masks[r]){if(z[m]<best[m]){less=1;break;}if(z[m]>best[m])break;}if(first||less){best=z;first=0;}}return best;}};
std::uint64_t fact(int n){std::uint64_t z=1;for(int i=2;i<=n;i++)z*=i;return z;}
struct Enum{int c;std::vector<std::pair<int,int>>types;std::array<std::uint8_t,MAX_C>cap{};std::array<std::uint64_t,2*MAX_C+1>f{};Hist target{};std::unordered_map<Hist,std::uint64_t,HH>raw;Enum(int c,Hist const&s):c(c){cap.fill(2);for(int i=0;i<=2*c;i++)f[i]=fact(i);for(int m=0;m<(1<<c);m++)if(s[m])types.push_back({m,s[m]});std::sort(types.begin(),types.end(),[&](auto x,auto y){int ax=c-__builtin_popcount((unsigned)x.first),ay=c-__builtin_popcount((unsigned)y.first);return ax!=ay?ax<ay:x.second>y.second;});}void run(){types_rec(0,1);}void types_rec(int i,std::uint64_t w){if(i==(int)types.size()){for(int a=0;a<c;a++)if(cap[a])return;raw[target]+=w;return;}auto[m,h]=types[i];int avail=0;for(int a=0;a<c;a++)if(!(m&(1<<a)))avail+=cap[a];if(avail<h)return;std::array<std::uint8_t,MAX_C>x{};alloc(i,m,h,0,h,1,w,x);}void alloc(int i,int m,int h,int a,int left,std::uint64_t den,std::uint64_t w,std::array<std::uint8_t,MAX_C>&x){if(a==c){if(left)return;for(int b=0;b<c;b++){cap[b]-=x[b];if(x[b])target[m|(1<<b)]+=x[b];}types_rec(i+1,w*(f[h]/den));for(int b=0;b<c;b++){if(x[b])target[m|(1<<b)]-=x[b];cap[b]+=x[b];}return;}if(m&(1<<a)){x[a]=0;alloc(i,m,h,a+1,left,den,w,x);return;}int later=0;for(int b=a+1;b<c;b++)if(!(m&(1<<b)))later+=cap[b];int lo=std::max(0,left-later),hi=std::min<int>(cap[a],left);for(int v=lo;v<=hi;v++){x[a]=v;alloc(i,m,h,a+1,left-v,den*f[v],w,x);}x[a]=0;}};
long long modpow(long long a,long long e,int p){long long r=1;for(;e;e>>=1,a=a*a%p)if(e&1)r=r*a%p;return r;}
int rank_mod(std::vector<std::vector<int>>&a,int p){int m=a.size(),n=m?a[0].size():0,r=0;for(int col=0;col<n&&r<m;col++){int q=r;while(q<m&&a[q][col]==0)q++;if(q==m)continue;std::swap(a[q],a[r]);long long inv=modpow(a[r][col],p-2,p);for(int j=col;j<n;j++)a[r][j]=(long long)a[r][j]*inv%p;for(int i=0;i<m;i++)if(i!=r&&a[i][col]){int z=a[i][col];for(int j=col;j<n;j++){int v=a[i][j]-(long long)z*a[r][j]%p;if(v<0)v+=p;a[i][j]=v;}}r++;}return r;}
int main(int ac,char**av){int c=ac>1?std::atoi(av[1]):6,p=ac>2?std::atoi(av[2]):1000003;if(c<1||c>6)throw std::runtime_error("C 1..6");Can can(c);std::vector<Hist>cur(1);cur[0][0]=2*c;cur[0]=can(cur[0],0);for(int r=0;r<c;r++){std::unordered_map<Hist,int,HH>target_id;std::vector<Hist>nxt;std::unordered_map<Hist,Hist,HH>cache;struct Edge{int s,t;std::uint64_t v;};std::vector<Edge>edges;for(int s=0;s<(int)cur.size();s++){Enum en(c,cur[s]);en.run();std::unordered_map<Hist,std::uint64_t,HH>tr;for(auto const&[raw,v]:en.raw){auto[it,ins]=cache.emplace(raw,Hist{});if(ins)it->second=can(raw,r+1);tr[it->second]+=v;}for(auto const&[h,v]:tr){auto[it,ins]=target_id.emplace(h,target_id.size());if(ins)nxt.push_back(h);edges.push_back({s,it->second,v});}}std::vector<std::vector<int>>M(nxt.size(),std::vector<int>(cur.size()));for(auto const&e:edges)M[e.t][e.s]=e.v%p;int rank=rank_mod(M,p);std::cout<<r<<"->"<<r+1<<" dims="<<nxt.size()<<"x"<<cur.size()<<" nnz="<<edges.size()<<" rank_mod_"<<p<<"="<<rank<<"\n";cur.swap(nxt);} }
