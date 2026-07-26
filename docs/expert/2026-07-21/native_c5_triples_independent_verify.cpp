#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>
#include <boost/multiprecision/cpp_int.hpp>

using namespace std;
using boost::multiprecision::cpp_int;
static constexpr int C=5, N=10;

struct Key {
    uint64_t lo=0, hi=0;
    bool operator==(Key const&o)const noexcept{return lo==o.lo&&hi==o.hi;}
};
struct KeyHash {
    size_t operator()(Key const&k)const noexcept{
        uint64_t x=k.lo ^ (k.hi+0x9e3779b97f4a7c15ULL+(k.lo<<6)+(k.lo>>2));
        x^=x>>30; x*=0xbf58476d1ce4e5b9ULL; x^=x>>27; x*=0x94d049bb133111ebULL; x^=x>>31;
        return (size_t)x;
    }
};

static Key pack_rows(array<uint16_t,N> rows){
    sort(rows.begin(),rows.end());
    unsigned __int128 x=0;
    for(int i=0;i<N;i++) x |= (unsigned __int128)rows[i] << (10*i);
    return {uint64_t(x),uint64_t(x>>64)};
}
static array<uint16_t,N> unpack_rows(Key k){
    unsigned __int128 x=((unsigned __int128)k.hi<<64)|k.lo;
    array<uint16_t,N>a{};
    for(int i=0;i<N;i++)a[i]=uint16_t((x>>(10*i))&1023);
    return a;
}

unordered_map<Key,uint64_t,KeyHash> memo;
uint64_t transitions=0;

uint64_t count_factorizations(Key key){
    auto it=memo.find(key); if(it!=memo.end()) return it->second;
    auto rows=unpack_rows(key);
    if(rows.back()==0){memo.emplace(key,1);return 1;}
    array<int,N> order{}; for(int i=0;i<N;i++)order[i]=i;
    sort(order.begin(),order.end(),[&](int a,int b){return __builtin_popcount(rows[a])<__builtin_popcount(rows[b]);});
    array<uint8_t,N> chosen{};
    uint64_t total=0;
    auto dfs = [&](auto&& self,int depth,uint16_t used)->void{
        if(depth==N){
            ++transitions;
            auto child=rows;
            for(int i=0;i<N;i++) child[i]&=uint16_t(~(1u<<chosen[i]));
            total += count_factorizations(pack_rows(child));
            return;
        }
        int i=order[depth];
        uint16_t avail=rows[i]&uint16_t(~used);
        while(avail){
            uint16_t bit=uint16_t(avail & -avail); avail-=bit;
            chosen[i]=uint8_t(__builtin_ctz(bit));
            self(self,depth+1,uint16_t(used|bit));
        }
    };
    dfs(dfs,0,0);
    memo.emplace(key,total);
    return total;
}

int main(int argc,char**argv){
    string path=argc>1?argv[1]:"/mnt/data/native_c5_response_quotient_triples.csv";
    ifstream f(path); if(!f){cerr<<"cannot open "<<path<<"\n";return 2;}
    string line; getline(f,line);
    struct Row{vector<int>words;uint64_t m,ell,F;}; vector<Row> rows;
    while(getline(f,line)){
        // CSV format: qid,"w w ...",m,ell,F
        size_t q1=line.find('"'), q2=line.find('"',q1+1);
        if(q1==string::npos||q2==string::npos) continue;
        Row r; stringstream ws(line.substr(q1+1,q2-q1-1)); int w; while(ws>>w)r.words.push_back(w);
        string tail=line.substr(q2+2); stringstream ts(tail); string field;
        getline(ts,field,','); r.m=stoull(field);
        getline(ts,field,','); r.ell=stoull(field);
        getline(ts,field,','); r.F=stoull(field);
        rows.push_back(move(r));
    }
    if(rows.size()!=355){cerr<<"bad row count "<<rows.size()<<"\n";return 3;}
    memo.reserve(4000000);
    auto t0=chrono::steady_clock::now();
    cpp_int N5=0; size_t mismatch=0;
    for(size_t ri=0;ri<rows.size();ri++){
        array<uint16_t,N> graph{};
        for(int i=0;i<N;i++){
            uint16_t m=0; int word=rows[ri].words[i];
            for(int p=0;p<C;p++)m|=uint16_t(1u<<(2*p+((word>>p)&1)));
            graph[i]=m;
        }
        uint64_t got=count_factorizations(pack_rows(graph));
        if(got!=rows[ri].F){cerr<<"mismatch row "<<ri<<" expected "<<rows[ri].F<<" got "<<got<<"\n";mismatch++;}
        cpp_int F=got; N5 += cpp_int(rows[ri].m)*rows[ri].ell*F*F;
        if(ri%25==0)cerr<<"row="<<ri<<" memo="<<memo.size()<<" transitions="<<transitions<<" sec="<<chrono::duration<double>(chrono::steady_clock::now()-t0).count()<<"\n";
    }
    cpp_int expected("1903816047972624930994913280000");
    if(mismatch||N5!=expected){cerr<<"verification failed mismatches="<<mismatch<<" N5="<<N5<<"\n";return 4;}
    cout<<"classes "<<rows.size()<<"\nall_F5_match 1\nmemo_states "<<memo.size()<<"\nperfect_matching_transitions "<<transitions<<"\nN5 "<<N5<<"\nseconds "<<chrono::duration<double>(chrono::steady_clock::now()-t0).count()<<"\n";
}
