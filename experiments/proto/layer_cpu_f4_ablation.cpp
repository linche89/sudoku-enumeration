// PREPARED independent three-kernel CPU differential/ablation. No checkpoints.
// Include the released bridge/graph source verbatim; call its hot arithmetic.
#define NOMINMAX
#include "layer_shared_f4_bridge.cpp"
#include "layer_gpu_f4_nodp_core.h"
#include "layer_cpu_f4_incremental_core.h"
#include <bcrypt.h>
#include <psapi.h>
#include <condition_variable>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>
#include <thread>
#include <omp.h>

namespace cpu_f4_ablation {
using U64=std::uint64_t;using Wide=unsigned __int128;
using Clock=std::chrono::steady_clock;
using Graph=std::array<std::uint16_t,MAX_M>;
static void need(bool ok,const std::string&s){if(!ok)throw std::runtime_error(s);}
static U64 integer(const std::string&s){
    need(!s.empty()&&s.size()<=20,"invalid unsigned integer");U64 n=0;
    for(char c:s){need(c>='0'&&c<='9',"invalid unsigned integer");need(n<=(UINT64_MAX-U64(c-'0'))/10,"integer overflow");n=n*10+U64(c-'0');}return n;
}
static std::string decimal(Wide n){if(!n)return"0";std::string s;while(n){s.push_back(char('0'+n%10));n/=10;}std::reverse(s.begin(),s.end());return s;}
static double elapsed(Clock::time_point t){return std::chrono::duration<double>(Clock::now()-t).count();}
struct Options {int c=0;U64 limit=0,rounds=0,repeats=0,seconds=0,rss=0;std::string input,oracle;};
static Options parse(int argc,char**argv){
    std::map<std::string,std::string>a;
    for(int i=1;i<argc;++i){std::string s=argv[i];auto p=s.find('=');need(p!=std::string::npos&&p&&p+1<s.size(),"key=value required");need(a.emplace(s.substr(0,p),s.substr(p+1)).second,"duplicate option");}
    const std::vector<std::string>keys={"C","input","oracle","limit","rounds","repeats","maxseconds","maxrssmib"};
    need(a.size()==keys.size(),"wrong option count");for(const auto&k:keys)need(a.count(k),"missing option: "+k);
    Options o;o.c=int(integer(a.at("C")));o.input=a.at("input");o.oracle=a.at("oracle");o.limit=integer(a.at("limit"));
    o.rounds=integer(a.at("rounds"));o.repeats=integer(a.at("repeats"));o.seconds=integer(a.at("maxseconds"));o.rss=integer(a.at("maxrssmib"));
    need(o.c>=4&&o.c<=6&&o.limit==U64(o.c==4?26:o.c==5?17120:1024),"complete C4/C5 or uniform1024 C6 domain required");
    need(o.rounds==3&&o.repeats>0&&o.repeats<=(o.c==4?256:1)&&o.seconds>0&&o.seconds<=120&&o.rss>=128&&o.rss<=1900,
         "three balanced rounds, positive repeats/time and <=1900MiB native bound required");return o;
}
struct Watchdog {
    std::mutex mutex;std::condition_variable condition;bool done=false;std::thread thread;
    std::atomic<U64> peak{0};Clock::time_point start=Clock::now();
    explicit Watchdog(const Options&o){thread=std::thread([&,o]{std::unique_lock<std::mutex>lock(mutex);
        while(!condition.wait_for(lock,std::chrono::milliseconds(20),[&]{return done;})){
            PROCESS_MEMORY_COUNTERS p{};p.cb=sizeof(p);
            if(!GetProcessMemoryInfo(GetCurrentProcess(),&p,sizeof(p))){std::fprintf(stderr,"BOUND RSS unavailable\n");std::fflush(stderr);std::_Exit(124);}
            peak.store(std::max(peak.load(),U64(p.PeakWorkingSetSize)));
            if(p.WorkingSetSize>(o.rss<<20)||elapsed(start)>o.seconds){std::fprintf(stderr,"BOUND CPU ablation time/RSS; no completed certificate\n");std::fflush(stderr);std::_Exit(124);}
        }});}
    ~Watchdog(){{std::lock_guard<std::mutex>lock(mutex);done=true;}condition.notify_all();thread.join();}
};
static std::string read(const std::string&path){
    const auto bytes=std::filesystem::file_size(path);need(bytes>0&&bytes<=(4ULL<<20),"input text exceeds4MiB");
    std::ifstream in(path,std::ios::binary);need(bool(in),"input unavailable");std::string data(size_t(bytes),'\0');
    in.read(data.data(),std::streamsize(bytes));need(in.gcount()==std::streamsize(bytes)&&in.peek()==EOF,"input changed or truncated");return data;
}
static std::string sha(const std::string&raw){
    BCRYPT_ALG_HANDLE alg=nullptr;BCRYPT_HASH_HANDLE h=nullptr;std::array<unsigned char,32>out{};
    need(BCryptOpenAlgorithmProvider(&alg,BCRYPT_SHA256_ALGORITHM,nullptr,0)>=0,"SHA provider failed");
    struct Close{BCRYPT_ALG_HANDLE&a;BCRYPT_HASH_HANDLE&h;~Close(){if(h)BCryptDestroyHash(h);if(a)BCryptCloseAlgorithmProvider(a,0);}}close{alg,h};
    need(BCryptCreateHash(alg,&h,nullptr,0,nullptr,0,0)>=0,"SHA create failed");
    need(BCryptHashData(h,reinterpret_cast<PUCHAR>(const_cast<char*>(raw.data())),ULONG(raw.size()),0)>=0&&
         BCryptFinishHash(h,out.data(),32,0)>=0,"SHA failed");
    const char*d="0123456789ABCDEF";std::string s;for(auto x:out){s+=d[x>>4];s+=d[x&15];}return s;
}
struct Input {Graph graph{};U64 expected=0,leaves=0,oldNodes=0;bool textExpected=false;};
static std::vector<Input> inputs(const Options&o,std::string&inputSha,std::string&oracleSha){
    const std::string raw=read(o.input),reference=read(o.oracle);inputSha=sha(raw);oracleSha=sha(reference);
    const char*inputPins[]={"A03FB5EDEFD7D7B384EE783AFF403E66072DBECE291308AD27BE4E296A0F28F9",
        "DBF5BCDEC67D5B16E9B1D1F2F1D972341A0AA029C99629DD2951F17A80848B43",
        "38F4B8EF65FA4BEEF7FB05995E2A64B7096162A5E1212A0388AF5A9E195CA99C"};
    const char*oraclePins[]={"41380C7044EB5DB069C49BC02F8D678BD4147F683EED7966119EDCF7BD39ED4D",
        "FC09FB4390EF269AACC226BAF1DA173B4A8ADD7166A012B08701C72038136C62",
        "8B3DD7335033F834631AB10C930807E4ADEB0C3CE4B8571145737A6EEFE80F1A"};
    need(inputSha==inputPins[o.c-4]&&oracleSha==oraclePins[o.c-4],"unrecognized complete/sample input or independent oracle SHA");
    std::vector<Input>result;std::istringstream stream(raw);std::string line;
    while(std::getline(stream,line)){
        line=line.substr(0,line.find('#'));std::istringstream row(line);row>>std::ws;if(row.eof())continue;
        Input x;std::array<unsigned,MAX_M>column{};std::string token;
        for(int i=0;i<M;++i){need(bool(row>>token),"missing mask");U64 mask=integer(token);need(mask<=U64(FULL)&&std::popcount(mask)==4,"invalid mask");
            for(int b=0;b<C;++b)need(((mask>>(2*b))&3)!=3,"non-native paired mask");
            x.graph[i]=std::uint16_t(mask);for(int j=0;j<M;++j)column[j]+=(mask>>j)&1;}
        for(int j=0;j<M;++j)need(column[j]==4,"unbalanced column");
        if(row>>token){need(token.rfind("expected=",0)==0,"invalid suffix");x.expected=integer(token.substr(9));x.textExpected=true;need(!(row>>token),"extra suffix");}
        need(o.c==6||x.textExpected,"complete small domain needs every exact expected F4");
        std::sort(x.graph.begin(),x.graph.begin()+M);result.push_back(x);need(result.size()<=o.limit,"input longer than exact domain");
    }
    need(result.size()==o.limit,"input shorter than exact domain");
    std::vector<bool>seen(result.size());std::istringstream csv(reference);U64 count=0;
    while(std::getline(csv,line)){
        if(line.empty()||line[0]<'0'||line[0]>'9'||line.find(',')==std::string::npos)continue;
        std::replace(line.begin(),line.end(),',',' ');std::istringstream row(line);std::string i,f,l,n,extra;
        need(bool(row>>i>>f>>l>>n)&&!(row>>extra),"malformed oracle row");U64 id=integer(i);
        need(id>0&&id<=result.size()&&!seen[id-1],"duplicate/out-of-range oracle row");seen[id-1]=true;++count;
        Input&x=result[id-1];U64 value=integer(f);need(!x.textExpected||x.expected==value,"exact fixture/oracle F4 mismatch");
        x.expected=value;x.leaves=integer(l);x.oldNodes=integer(n);need(value>0&&value%24==0&&x.leaves&&x.oldNodes,"invalid closed oracle");
    }
    need(count==result.size(),"oracle incomplete");return result;
}
struct NoClockGuard {unsigned operator()() const noexcept{return 0;}};
// Both variants are bounded by the SAME external watchdog. No per-poll chrono
// calls are charged to only one kernel; the no-DP node/overflow guards remain.
struct Answer {U64 value=0,leaves=0,nodes=0,iterations=0;};
static Answer kernel(int kind,const Graph&graph){
    if(kind==0){const auto value=computeDegree4RootedSplit(graph);need(value.value&&value.value<=UINT64_MAX,"old value exceeds closed u64 bound");
        return{U64(value.value),value.leaves,value.nodes,0};}
    NoClockGuard guard;
    const nodp::Result value=kind==1?nodp::solve(graph.data(),M,2000000,guard):incremental_nodp::solve(graph.data(),M,2000000,guard);
    need(value.status==0,"new kernel failed closed, status="+std::to_string(value.status));return{value.value,value.leaves,value.nodes,value.iterations};
}
static void valid(const Answer&a,const Input&x,U64 bound){need(a.value==x.expected&&a.leaves==x.leaves&&a.value<=bound&&a.value%24==0,"per-graph F4/leaf/overflow differential");}
struct Error {std::atomic<bool>failed{false};std::mutex mutex;std::string message;
    void set(const std::exception&e){failed.store(true);std::lock_guard<std::mutex>lock(mutex);if(message.empty())message=e.what();}
    void check(){if(failed.load())throw std::runtime_error(message);}};
struct Phase {int threads=0,round=0,position=0,kind=0;U64 calls=0;Wide values=0,leaves=0;U64 nodes=0,iterations=0;double seconds=0;};
static const char*name(int kind){return kind==0?"RELEASED_TERNARY_DP":kind==1?"FROZEN_LEAF_DSU_NODP":"INCREMENTAL_DSU_NODP";}
static std::vector<Phase> benchmark(const Options&o,const std::vector<Input>&input,
                                  const std::vector<std::array<Answer,3>>&exact,int threads,U64 bound){
    std::vector<Phase>phases;Error error;Clock::time_point start;Phase result;
    Wide values=0,leaves=0;U64 nodes=0,iterations=0,calls=0;
    const U64 jobs=o.repeats*input.size();
#pragma omp parallel num_threads(threads) shared(start,result,values,leaves,nodes,iterations,calls,error,phases)
    {
        // Every thread warms BOTH old TLS storage and the two tiny-array variants.
        try{const auto&x=input[size_t(omp_get_thread_num())%input.size()];for(int k=0;k<3;++k)valid(kernel(k,x.graph),x,bound);}
        catch(const std::exception&e){error.set(e);}
#pragma omp barrier
        for(unsigned round=0;round<o.rounds;++round)for(int position=0;position<3;++position){
            const int kind=(int(round)+position)%3;
#pragma omp single
            {values=leaves=0;nodes=iterations=calls=0;start=Clock::now();}
#pragma omp for schedule(static) reduction(+:values,leaves,nodes,iterations,calls)
            for(int64_t job=0;job<int64_t(jobs);++job){
                if(error.failed.load(std::memory_order_relaxed))continue;
                try{const size_t id=size_t(job)%input.size();const Answer a=kernel(kind,input[id].graph);valid(a,input[id],bound);
                    const Answer&ref=exact[id][kind];need(a.nodes==ref.nodes&&a.iterations==ref.iterations,"timed traversal counters changed");
                    values+=a.value;leaves+=a.leaves;nodes+=a.nodes;iterations+=a.iterations;++calls;
                }catch(const std::exception&e){error.set(e);}
            }
#pragma omp single
            {result={threads,int(round),position,kind,calls,values,leaves,nodes,iterations,elapsed(start)};phases.push_back(result);}
        }
    }
    error.check();for(const auto&p:phases)need(p.calls==jobs,"timed phase incomplete");return phases;
}
int run(int argc,char**argv){
    const Options o=parse(argc,argv);Watchdog watchdog(o);shared_f4_bridge::initialize(o.c);omp_set_dynamic(0);
    std::string inputSha,oracleSha;const auto input=inputs(o,inputSha,oracleSha);
    Wide boundWide=1;for(int i=0;i<M;++i)boundWide*=24;need(boundWide<=UINT64_MAX,"F4 bound overflow");const U64 bound=U64(boundWide);
    std::vector<std::array<Answer,3>>exact(input.size());Error error;const auto verifyStart=Clock::now();
#pragma omp parallel for num_threads(24) schedule(static)
    for(int64_t i=0;i<int64_t(input.size());++i){
        if(error.failed.load(std::memory_order_relaxed))continue;
        try{for(int k=0;k<3;++k){exact[size_t(i)][k]=kernel(k,input[size_t(i)].graph);valid(exact[size_t(i)][k],input[size_t(i)],bound);}
            need(exact[size_t(i)][0].nodes==input[size_t(i)].oldNodes,"released nodes differ from retained independent oracle");
            need(exact[size_t(i)][1].nodes==exact[size_t(i)][2].nodes&&exact[size_t(i)][1].iterations==exact[size_t(i)][2].iterations,
                 "incremental DSU changed the frozen no-DP traversal tree");
        }catch(const std::exception&e){error.set(e);}
    }
    error.check();const double verifySeconds=elapsed(verifyStart);
    std::array<std::map<std::pair<U64,U64>,U64>,4>hist;Wide sumValues=0,sumLeaves=0;
    std::cout<<"row";for(int i=0;i<M;++i)std::cout<<",mask"<<i;
    std::cout<<",expected_F4,oracle_leaves,old_F4,leaf_F4,incremental_F4,old_nodes,leaf_nodes,incremental_nodes,leaf_iterations,incremental_iterations\n";
    for(size_t i=0;i<input.size();++i){const auto&x=input[i];++hist[3][{x.expected,x.leaves}];sumValues+=x.expected;sumLeaves+=x.leaves;
        std::cout<<i+1;for(int j=0;j<M;++j)std::cout<<','<<x.graph[j];std::cout<<','<<x.expected<<','<<x.leaves;
        for(int k=0;k<3;++k){++hist[k][{exact[i][k].value,exact[i][k].leaves}];std::cout<<','<<exact[i][k].value;}
        for(int k=0;k<3;++k)std::cout<<','<<exact[i][k].nodes;
        std::cout<<','<<exact[i][1].iterations<<','<<exact[i][2].iterations<<'\n';}
    for(int k=0;k<3;++k)need(hist[k]==hist[3],"complete F4/leaf histogram mismatch");
    std::cout<<"HISTOGRAM entries="<<hist[3].size()<<" exact_all_three=PASS sum_F4="<<decimal(sumValues)<<" sum_leaves="<<decimal(sumLeaves)<<'\n';
    for(const auto&[key,count]:hist[3])std::cout<<"HIST F4="<<key.first<<" leaves="<<key.second<<" multiplicity="<<count<<'\n';
    std::cout<<std::fixed<<std::setprecision(9);
    for(int threads:{1,24}){
        const auto phases=benchmark(o,input,exact,threads,bound);
        for(const auto&p:phases){need(p.values==sumValues*o.repeats&&p.leaves==sumLeaves*o.repeats,"timed full sums differ");
            std::cout<<"TIMING threads="<<p.threads<<" round="<<p.round<<" position="<<p.position<<" variant="<<name(p.kind)
                     <<" calls="<<p.calls<<" seconds="<<p.seconds<<" sum_F4="<<decimal(p.values)<<" sum_leaves="<<decimal(p.leaves)
                     <<" nodes="<<p.nodes<<" iterations="<<p.iterations<<'\n';}
    }
    need(sha(read(o.input))==inputSha&&sha(read(o.oracle))==oracleSha,"input/oracle changed during ablation");
    PROCESS_MEMORY_COUNTERS pm{};pm.cb=sizeof(pm);need(GetProcessMemoryInfo(GetCurrentProcess(),&pm,sizeof(pm)),"final RSS unavailable");
    need(elapsed(watchdog.start)<=o.seconds&&pm.PeakWorkingSetSize<=(o.rss<<20),"final hard bound exceeded");
    std::cout<<"SUMMARY status=EXACT_SAME_BINARY_ABLATION C="<<C<<" rows="<<input.size()<<" variants=3 warm_threads=1,24 balanced_rounds="<<o.rounds
             <<" repeats="<<o.repeats<<" all_values_leaves_histogram=PASS input_SHA256="<<inputSha<<" oracle_SHA256="<<oracleSha
             <<" initial_exact_gate_seconds="<<verifySeconds<<" total_seconds="<<elapsed(watchdog.start)<<" peak_rss_bytes="<<pm.PeakWorkingSetSize
             <<" original_source_unchanged=YES catalogue_reads=0 checkpoint_writes=0 N6=NOT_COMPUTED\n";
    std::cout<<"CPU THREE KERNEL EXACT ABLATION PASSED\n";return 0;
}
}
int main(int argc,char**argv)try{return cpu_f4_ablation::run(argc,argv);}catch(const std::exception&e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}
