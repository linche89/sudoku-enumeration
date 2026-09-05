// Independent byte/alias auditor. No producer headers or counting algorithms.
// Build: g++ -O3 -mpopcnt -std=c++20 this.cpp -o build/...exe -lbcrypt -lpsapi
#define NOMINMAX
#include <windows.h>
#include <bcrypt.h>
#include <psapi.h>
#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>
using U64 = uint64_t;
namespace fs = std::filesystem;
static constexpr U64 SEED=0x5344464a434b3031ULL, N=903398621, CHUNK=25000;
static constexpr U64 SOURCE_HEADER=10599038447932118899ULL, REPAIR=8375264605690466664ULL;
static const std::string SOURCE="ECF0837315B0FDF8AE21C394FDA6676490E43B1828A0825529A344EC17E4E844";
static constexpr U64 MAX_BYTES=12*N+256*((N+CHUNK-1)/CHUNK+1);
static void need(bool ok,const std::string& s){if(!ok)throw std::runtime_error(s);}
static U64 number(const std::string& s){
    need(!s.empty()&&s.size()<=20,"invalid unsigned integer"); U64 n=0;
    for(char c:s){need(c>='0'&&c<='9',"invalid unsigned integer");
        need(n<=(UINT64_MAX-U64(c-'0'))/10,"unsigned integer overflow");n=n*10+U64(c-'0');}
    return n;
}
static U64 le(const uint8_t*p,size_t n){U64 v=0;for(size_t i=0;i<n;++i)v|=U64(p[i])<<(8*i);return v;}
static U64 ck(const uint8_t*p,size_t n,U64 seed=SEED){
    U64 h=seed^(0x9e3779b97f4a7c15ULL+n);size_t i=0;
    for(;i+8<=n;i+=8){h^=le(p+i,8);h*=0xff51afd7ed558ccdULL;h^=h>>33;}
    h^=le(p+i,n-i);h*=0xc4ceb9fe1a85ec53ULL;return h^(h>>33);
}
static bool hexsha(const std::string&s){return s.size()==64&&std::all_of(s.begin(),s.end(),[](char c){return(c>='0'&&c<='9')||(c>='A'&&c<='F');});}
class Sha {
    BCRYPT_ALG_HANDLE algorithm=nullptr;
public:
    Sha(){need(BCryptOpenAlgorithmProvider(&algorithm,BCRYPT_SHA256_ALGORITHM,nullptr,0)>=0,"SHA provider failure");}
    ~Sha(){if(algorithm)BCryptCloseAlgorithmProvider(algorithm,0);}
    std::string operator()(const uint8_t*p,size_t n){
        need(n<=ULONG_MAX,"SHA bounded buffer overflow");BCRYPT_HASH_HANDLE h=nullptr;
        need(BCryptCreateHash(algorithm,&h,nullptr,0,nullptr,0,0)>=0,"SHA create failure");
        std::array<uint8_t,32> out{};
        auto a=BCryptHashData(h,const_cast<PUCHAR>(p),ULONG(n),0);
        auto b=BCryptFinishHash(h,out.data(),ULONG(out.size()),0);BCryptDestroyHash(h);
        need(a>=0&&b>=0,"SHA computation failure");
        const char*d="0123456789ABCDEF";std::string s;for(uint8_t v:out){s+=d[v>>4];s+=d[v&15];}return s;
    }
    std::string operator()(const std::vector<uint8_t>& v){return (*this)(v.data(),v.size());}
};
static std::vector<uint8_t> read(const fs::path&p,U64 expected,U64 maximum){
    need(expected<=maximum,"read exceeds declared byte bound");
    HANDLE h=CreateFileW(p.c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,
        FILE_FLAG_SEQUENTIAL_SCAN|FILE_FLAG_OPEN_REPARSE_POINT,nullptr);
    need(h!=INVALID_HANDLE_VALUE,"cannot open immutable input: "+p.string());
    struct Close{HANDLE h;~Close(){CloseHandle(h);}}close{h};
    BY_HANDLE_FILE_INFORMATION info{};LARGE_INTEGER size{};
    need(GetFileInformationByHandle(h,&info)&&!(info.dwFileAttributes&(FILE_ATTRIBUTE_REPARSE_POINT|FILE_ATTRIBUTE_DIRECTORY)),"input is not an ordinary file");
    need(GetFileSizeEx(h,&size)&&size.QuadPart>=0&&U64(size.QuadPart)==expected,"input byte length differs: "+p.string());
    std::vector<uint8_t> v(static_cast<size_t>(expected));size_t at=0;
    while(at<v.size()){DWORD got=0,want=DWORD(std::min<size_t>(v.size()-at,1<<20));
        need(ReadFile(h,v.data()+at,want,&got,nullptr)&&got>0,"short input read");at+=got;}
    uint8_t extra;DWORD got=0;need(ReadFile(h,&extra,1,&got,nullptr)&&got==0,"input has trailing bytes");return v;
}
struct Header {U64 begin,count,payload,reps,live,sum;bool manifest;};
static Header decode(const std::vector<uint8_t>&v){
    need(v.size()>=256,"truncated header");
    std::string magic(reinterpret_cast<const char*>(v.data()),8);
    need(magic=="SFR4CK01"||magic=="SFR4MT01","wrong shared header magic");
    need(le(v.data()+8,4)==1&&le(v.data()+12,4)==6,"wrong header version/domain");
    need(std::string(reinterpret_cast<const char*>(v.data()+16),64)==SOURCE,"unrecognized source SHA lineage");
    need(le(v.data()+80,8)==0x31304e494d344653ULL&&le(v.data()+88,8)==SOURCE_HEADER&&
         le(v.data()+96,8)==REPAIR&&le(v.data()+104,8)==N&&le(v.data()+112,8)==CHUNK,
         "wrong production semantics/source/repair/domain lineage");
    need(std::all_of(v.begin()+176,v.begin()+256,[](uint8_t x){return x==0;}),"nonzero header reserved bytes");
    std::array<uint8_t,256>b;std::copy_n(v.begin(),256,b.begin());std::fill(b.begin()+144,b.begin()+152,0);
    need(ck(b.data(),b.size())==le(v.data()+144,8),"header checksum mismatch");
    Header h{le(v.data()+120,8),le(v.data()+128,8),le(v.data()+136,8),
        le(v.data()+152,8),le(v.data()+160,8),le(v.data()+168,8),magic=="SFR4MT01"};
    if(h.manifest)need(v.size()==256&&!(h.begin|h.count|h.payload|h.reps|h.live|h.sum),"manifest contains record data");
    return h;
}
struct Row {std::string name,sha,payloadSha;U64 bytes=0,begin=0,count=0,live=0,reps=0,sum=0;bool old=false;};
static std::string chunkName(U64 begin){std::ostringstream s;s<<"chunk-"<<std::hex<<std::setw(16)<<std::setfill('0')<<begin<<".bin";return s.str();}
static std::vector<std::string> tokens(const std::string&line,size_t n){
    std::istringstream s(line);std::vector<std::string> v;std::string x;while(s>>x)v.push_back(x);
    need(v.size()==n,"malformed plan row/token count");return v;
}
int main(int argc,char**argv){
    std::atomic<bool> done=false;std::thread watchdog;
    try{
        std::map<std::string,std::string>a;
        for(int i=1;i<argc;++i){std::string s=argv[i];auto p=s.find('=');
            need(p!=std::string::npos&&p&&p+1<s.size(),"expected key=value option");
            need(a.emplace(s.substr(0,p),s.substr(p+1)).second,"duplicate option");}
        const std::vector<std::string> keys={"plan","plan_sha256","current","after","before","maxseconds","maxrssmib"};
        need(a.size()==keys.size(),"wrong option count");for(const auto&k:keys)need(a.count(k),"missing option: "+k);
        U64 seconds=number(a.at("maxseconds")),rss=number(a.at("maxrssmib"));
        need(seconds>0&&seconds<=180&&rss>=128&&rss<=1024,"invalid hard time/RSS bound");
        auto started=std::chrono::steady_clock::now();
        watchdog=std::thread([&]{while(!done.load()){
            PROCESS_MEMORY_COUNTERS p{};p.cb=sizeof(p);
            if(!GetProcessMemoryInfo(GetCurrentProcess(),&p,sizeof(p))||p.WorkingSetSize>(rss<<20)||
               std::chrono::duration<double>(std::chrono::steady_clock::now()-started).count()>seconds){
                std::cerr<<"BOUND independent native audit time/RSS; no completed report accepted\n";std::cerr.flush();ExitProcess(124);}
            std::this_thread::sleep_for(std::chrono::milliseconds(20));}});
        Sha sha;fs::path plan=fs::u8path(a.at("plan"));
        auto rawPlan=read(plan,fs::file_size(plan),16<<20);
        need(hexsha(a.at("plan_sha256"))&&sha(rawPlan)==a.at("plan_sha256"),"plan SHA mismatch");
        std::string text(rawPlan.begin(),rawPlan.end());need(text.find('\0')==std::string::npos,"NUL in plan");
        std::istringstream input(text);std::string line;need(bool(std::getline(input,line))&&line=="SHARED_F4_BITSET_PLAN_V1","wrong plan schema");
        need(bool(std::getline(input,line)),"missing plan geometry");auto p=tokens(line,5);
        U64 prefix=number(p[0]),prior=number(p[1]),fileCount=number(p[2]),byteBound=number(p[3]);
        bool replay=p[4]=="replay";need(replay||p[4]=="resume","invalid plan mode");
        need(prefix>0&&prefix<=N&&(prefix%CHUNK==0||prefix==N),"invalid prefix boundary");
        need(prior>0&&prior<=prefix&&(replay?prior==prefix:prior<prefix)&&
             (prior%CHUNK==0||prior==N),"invalid predecessor prefix");
        need(fileCount==(prefix+CHUNK-1)/CHUNK+1&&byteBound>0&&byteBound<=MAX_BYTES,"invalid plan bounds");
        std::vector<Row> rows;U64 bytes=0;
        for(U64 i=0;i<fileCount;++i){need(bool(std::getline(input,line)),"truncated plan");auto t=tokens(line,4);
            Row r;r.name=t[0];r.bytes=number(t[1]);r.sha=t[2];r.old=t[3]=="1";
            need((r.old||t[3]=="0")&&hexsha(r.sha),"invalid plan old flag/SHA");
            U64 begin=i*CHUNK;bool manifest=i+1==fileCount;
            U64 size=manifest?256:256+12*std::min(CHUNK,N-begin);
            need(r.name==(manifest?"manifest.bin":chunkName(begin))&&r.bytes==size,"plan filename/range/length mismatch");
            need(r.old==(!replay&&(manifest||begin<prior)),"plan prior-copy flag differs from exact prefix");
            bytes+=r.bytes;rows.push_back(r);
        }
        need(!std::getline(input,line),"trailing plan data");need(bytes<=byteBound,"planned bytes exceed bound");
        fs::path current=fs::u8path(a.at("current")),after=fs::u8path(a.at("after")),before=fs::u8path(a.at("before"));
        std::vector<uint8_t> representatives(size_t((prefix+7)/8),0);
        U64 live=0,reps=0,sum=0,priorLive=0,priorReps=0,priorSum=0;
        for(auto&r:rows){
            auto raw=read(current/r.name,r.bytes,300256);need(sha(raw)==r.sha,"current file SHA mismatch: "+r.name);
            auto copy=read(after/r.name,r.bytes,300256);need(copy==raw&&sha(copy)==r.sha,"after physical copy differs: "+r.name);
            if(r.old){copy=read(before/r.name,r.bytes,300256);need(copy==raw&&sha(copy)==r.sha,"before physical copy differs: "+r.name);}
            Header h=decode(raw);if(r.name=="manifest.bin"){need(h.manifest,"manifest name/magic mismatch");continue;}
            need(!h.manifest&&r.name==chunkName(h.begin)&&h.count==std::min(CHUNK,N-h.begin)&&
                 h.begin<prefix&&h.count<=prefix-h.begin,"chunk range mismatch");
            const uint8_t*aliases=raw.data()+256;const uint8_t*values=aliases+4*h.count;
            need(ck(values,size_t(8*h.count),ck(aliases,size_t(4*h.count)))==h.payload,"payload checksum mismatch");
            r.begin=h.begin;r.count=h.count;r.payloadSha=sha(raw.data()+256,raw.size()-256);
            for(U64 i=0;i<h.count;++i){U64 alias=le(aliases+4*i,4),value=le(values+8*i,8),id=h.begin+i;
                if(alias==UINT32_MAX){need(value==0,"hole stores F4");continue;}
                need(alias<N,"alias outside full native domain");++r.live;
                if(alias==id){need(value>0&&value%24==0,"self representative has invalid F4");
                    ++r.reps;r.sum+=value;representatives[size_t(id>>3)]|=uint8_t(1u<<(id&7));}
                else need(value==0,"nonrepresentative stores F4");
            }
            need(r.live==h.live&&r.reps==h.reps&&r.sum==h.sum,"decoded counters differ from header");
            live+=r.live;reps+=r.reps;sum+=r.sum;
            if(h.begin<prior){priorLive+=r.live;priorReps+=r.reps;priorSum+=r.sum;}
        }
        U64 resolved=0,future=0,holes=0;
        for(const auto&r:rows){auto raw=read(current/r.name,r.bytes,300256);
            need(sha(raw)==r.sha,"current file changed between passes: "+r.name);
            if(r.name=="manifest.bin")continue;
            for(U64 i=0;i<r.count;++i){U64 alias=le(raw.data()+256+4*i,4);
                if(alias==UINT32_MAX){++holes;continue;}
                if(alias<prefix){need(representatives[size_t(alias>>3)]&(1u<<(alias&7)),"alias into committed prefix is not a closed representative");++resolved;}
                else{need(alias<N,"second-pass alias outside full native domain");++future;}
            }
        }
        need(live+holes==prefix&&resolved+future==live,"closure counters inconsistent");
        for(const auto&r:rows){std::cout<<"{\"name\":\""<<r.name<<"\",\"bytes\":"<<r.bytes<<",\"sha256\":\""<<r.sha<<"\"";
            if(r.name!="manifest.bin")std::cout<<",\"begin\":"<<r.begin<<",\"count\":"<<r.count<<",\"live\":"<<r.live<<",\"representatives\":"<<r.reps<<",\"value_checksum\":"<<r.sum<<",\"payload_sha256\":\""<<r.payloadSha<<"\"";
            std::cout<<"}\n";}
        PROCESS_MEMORY_COUNTERS memory{};memory.cb=sizeof(memory);need(GetProcessMemoryInfo(GetCurrentProcess(),&memory,sizeof(memory)),"cannot query final RSS");
        std::cout<<"{\"worker_status\":\"PASS\",\"closed_prefix\":"<<prefix<<",\"total_bytes\":"<<bytes
          <<",\"live_records\":"<<live<<",\"holes\":"<<holes<<",\"closed_representatives\":"<<reps<<",\"value_checksum\":"<<sum
          <<",\"prior_live_records\":"<<priorLive<<",\"prior_closed_representatives\":"<<priorReps<<",\"prior_value_checksum\":"<<priorSum
          <<",\"aliases_to_closed_representatives\":"<<resolved<<",\"aliases_to_future_uncomputed_representatives\":"<<future
          <<",\"representative_bitset_bytes\":"<<representatives.size()<<",\"representative_bitset_sha256\":\""<<sha(representatives)<<"\""
          <<",\"elapsed_seconds\":"<<std::chrono::duration<double>(std::chrono::steady_clock::now()-started).count()
          <<",\"peak_rss_bytes\":"<<memory.PeakWorkingSetSize<<"}\n";
        std::cout.flush();need(bool(std::cout),"worker output failed");done=true;watchdog.join();return 0;
    }catch(const std::exception&e){done=true;if(watchdog.joinable())watchdog.join();std::cerr<<"FAIL "<<e.what()<<"\n";return 1;}
}
