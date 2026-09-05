// Read-only full native-catalogue lookup diagnostic. Stored T is never used
// as a factorization value. The optional support repair exists only in RAM.
#define NATIVE_GATHER_NO_MAIN
#include "layer_native_gather_bench.cpp"
#undef NATIVE_GATHER_NO_MAIN

#include <filesystem>

namespace catalog_lookup {
using native_gather::Clock;
using native_gather::seconds;
struct Options {
    std::string catalog, input, mode = "gather";
    u64 limit = 0, maxQueries = 0;
    int threads = 0;
    double maxSeconds = 0;
    bool readonly = false, repair = false;
};
struct Guard {
    std::mutex mutex;
    std::condition_variable cv;
    bool done = false;
    std::atomic<u64> peak{0};
    std::thread worker;
    explicit Guard(double maxSeconds) {
        worker = std::thread([this,maxSeconds] {
            const auto started = Clock::now();
            std::unique_lock<std::mutex> held(mutex);
            while (!cv.wait_for(held,std::chrono::milliseconds(100),[this]{return done;})) {
                PROCESS_MEMORY_COUNTERS pm{}; pm.cb = sizeof(pm);
                if (!GetProcessMemoryInfo(GetCurrentProcess(),&pm,sizeof(pm)) ||
                    seconds(started)>maxSeconds || pm.WorkingSetSize>(55ULL<<30)) {
                    std::fprintf(stderr,"BOUND catalogue diagnostic time/RSS; no result accepted\n");
                    std::fflush(nullptr); std::_Exit(124);
                }
                peak.store(std::max(peak.load(),u64(pm.WorkingSetSize)));
            }
        });
    }
    ~Guard() {
        { std::lock_guard<std::mutex> held(mutex); done=true; }
        cv.notify_all(); worker.join();
    }
};

int run(int argc,char** argv) {
    if(argc<2) throw std::runtime_error("usage: C catalog=... input=... mode=gather|keys limit=... maxqueries=... threads=... maxseconds=... checkpointreadonly [repair-l4-support]");
    C=std::stoi(argv[1]); N2C=2*C; Options o;
    for(int i=2;i<argc;++i) {
        const std::string arg=argv[i];
        if(arg=="checkpointreadonly") {o.readonly=true;continue;}
        if(arg=="repair-l4-support") {o.repair=true;continue;}
        const auto at=arg.find('=');
        if(at==std::string::npos) throw std::runtime_error("expected option=value");
        const std::string k=arg.substr(0,at),v=arg.substr(at+1);
        if(k=="catalog") o.catalog=v;
        else if(k=="input") o.input=v;
        else if(k=="mode") o.mode=v;
        else if(k=="limit") o.limit=std::stoull(v);
        else if(k=="maxqueries") o.maxQueries=std::stoull(v);
        else if(k=="threads") o.threads=std::stoi(v);
        else if(k=="maxseconds") o.maxSeconds=std::stod(v);
        else throw std::runtime_error("unknown option "+k);
    }
    if(C<2||C>6||!o.readonly||o.catalog.empty()||o.input.empty()||
       !o.limit||o.limit>10000||!o.maxQueries||o.maxQueries>5000000||
       o.threads<1||o.threads>32||o.maxSeconds<=0||o.maxSeconds>600||
       (o.mode!="gather"&&o.mode!="keys")||(o.repair&&C!=6))
        throw std::runtime_error("require readonly, positive bounds(limit<=10000 queries<=5m threads<=32 seconds<=600)");
    g_wlseed=false; g_forceWide=false; g_rehearsalDenom=0;
    const auto started=Clock::now(); Guard guard(o.maxSeconds);
    MEMORYSTATUSEX memory{}; memory.dwLength=sizeof(memory);
    if(!GlobalMemoryStatusEx(&memory)) throw std::runtime_error("cannot inspect available RAM");
    if(C==6&&memory.ullAvailPhys<(65ULL<<30)) throw std::runtime_error("C6 requires at least65GiB currently available RAM");
    const auto fileBytes=std::filesystem::file_size(o.catalog);
    const auto modified=std::filesystem::last_write_time(o.catalog);
    CkptHeader header{};
    if(!ck_read_header(o.catalog,header)) throw std::runtime_error("catalogue header/configuration checksum failed");
    if(header.nWide||header.layerIdx>=u32(C)) throw std::runtime_error("catalogue must be an intermediate narrow layer");
    if(C==6&&(header.layerIdx!=4||header.gen!=37||header.nEntries!=903398620||header.holes!=18))
        throw std::runtime_error("C6 diagnostic is pinned to original generation37 L4");
    const size_t capacity=size_t(header.nEntries)+(o.repair?1:0);
    const auto loading=Clock::now(); CkptImage image;
    if(!ck_read_file(o.catalog,image,capacity)) throw std::runtime_error("read-only catalogue payload validation failed");
    const double readHashSeconds=seconds(loading);
    std::cout<<"read_and_payload_hash_s="<<readHashSeconds<<" entries="<<header.nEntries
             <<" source_T_status=partial_opaque_never_used_as_F\n"<<std::flush;

    const auto indexing=Clock::now(); Layer layer;
    layer.keys=std::move(image.keys); layer.T=std::move(image.T); layer.stab=std::move(image.stab);
    layer.keys.resize(capacity);layer.T.resize(capacity,0);layer.stab.resize(capacity,0);
    size_t slots=64;while(slots<2*capacity)slots<<=1;
    layer.table.assign(slots,0);layer.mask=slots-1;
    layer.fixedCap=true;layer.claimed=u32(header.nEntries);layer.holes=u32(header.holes);
    u64 duplicates=0,invalidStabs=0,countedHoles=0,mass=0;
    u64 group=1ULL<<C;for(int i=2;i<=C;++i)group*=i;
    omp_set_num_threads(o.threads);
#pragma omp parallel for schedule(static) reduction(+:duplicates,invalidStabs,countedHoles,mass)
    for(int64_t pos=0;pos<int64_t(header.nEntries);++pos) {
        const u32 stab=layer.stab[size_t(pos)];
        if(!stab){++countedHoles;continue;}
        if(group%stab){++invalidStabs;continue;}
        mass+=group/stab;
        const auto& key=layer.keys[size_t(pos)];u64 hash=state_hash(key)&layer.mask;
        for(;;) {
            u32 wanted=0;
            if(__atomic_compare_exchange_n(&layer.table[hash],&wanted,u32(pos)+1,false,
                                           __ATOMIC_RELAXED,__ATOMIC_RELAXED))break;
            if(layer.keys[wanted-1]==key){++duplicates;break;}
            hash=(hash+1)&layer.mask;
        }
    }
    if(duplicates||invalidStabs||countedHoles!=header.holes)
        throw std::runtime_error("full index uniqueness/stabilizer/hole check failed");
    const double indexSeconds=seconds(indexing);
    std::cout<<"index_s="<<indexSeconds<<" table_slots="<<slots<<" duplicates="<<duplicates
             <<" stored_orbit_mass="<<mass<<" index_threads="<<o.threads<<'\n'<<std::flush;
    const auto repairing=Clock::now();
    if(o.repair) {
        const native_gather::Graph raw={294,294,554,554,1161,1161,1360,1360,2181,2181,2640,2640};
        u64 stab=0;const State missing=canonize(native_gather::encode(raw),&stab);
        if(stab!=12||layer.find(missing)!=UINT32_MAX)throw std::runtime_error("in-memory witness precondition failed");
        const u32 id=layer.find_or_add_mt(missing,u32(stab));
        if(id!=header.nEntries||layer.size()!=header.nEntries+1)throw std::runtime_error("memory repair changed original IDs");
        std::cout<<"memory_only_repair_id="<<id<<" stab="<<stab<<" F=PROBE\n";
    }
    const double repairSeconds=seconds(repairing);

    std::ifstream input(o.input);if(!input)throw std::runtime_error("query input unavailable");
    const int degree=int(header.layerIdx)+(o.mode=="gather"?1:0);
    native_gather::Input row;
    u64 sources=0,records=0,weak=0,queries=0,hits=0,misses=0,checksum=0;
    double pivotSeconds=0,enumerationSeconds=0,canonSeconds=0,lookupSeconds=0;
    const auto querying=Clock::now();
    auto query=[&](const State& raw) {
        auto stamp=Clock::now();const State key=canonize(raw);canonSeconds+=seconds(stamp);
        stamp=Clock::now();const u32 id=layer.find(key);
        if(id==UINT32_MAX)++misses;
        else {++hits;checksum^=sample_mix64(u64(id)^layer.keys[id].m[0]^(u64(layer.stab[id])<<32));}
        lookupSeconds+=seconds(stamp);++queries;
        if(queries>o.maxQueries)throw std::runtime_error("query bound exceeded");
    };
    while(sources<o.limit&&native_gather::next(input,degree,row)) {
        if(o.mode=="keys")query(native_gather::encode(row.graph));
        else {
            auto stamp=Clock::now();const auto root=native_gather::pivot(row.graph);pivotSeconds+=seconds(stamp);
            if(root.frequency>o.maxQueries-records)throw std::runtime_error("rooted matching bound would be exceeded");
            native_gather::Batch batch{row.graph,{}, {},0,root.frequency,o.maxQueries};
            batch.chosen[0]=root.bit;stamp=Clock::now();
            batch.enumerate(u16(((1u<<N2C)-1)^1u),root.bit);enumerationSeconds+=seconds(stamp);
            if(batch.records!=root.frequency)throw std::runtime_error("rooted matching count mismatch");
            records+=batch.records;weak+=batch.raw.size();
            for(const auto& item:batch.raw)query(item.first);
        }
        ++sources;
    }
    if(!sources||misses)throw std::runtime_error("empty query set or missing catalogue key");
    if(std::filesystem::file_size(o.catalog)!=fileBytes||std::filesystem::last_write_time(o.catalog)!=modified)
        throw std::runtime_error("source catalogue changed during diagnostic");
    std::cout<<std::fixed<<std::setprecision(9)
             <<"summary sources="<<sources<<" mode="<<o.mode<<" matches="<<records<<" weak="<<weak
             <<" queries="<<queries<<" hits="<<hits<<" misses="<<misses<<" checksum="<<checksum
             <<" read_and_hash_s="<<readHashSeconds<<" index_s="<<indexSeconds<<" repair_s="<<repairSeconds
             <<" pivot_s="<<pivotSeconds<<" enumeration_s="<<enumerationSeconds
             <<" canonicalization_s="<<canonSeconds<<" lookup_s="<<lookupSeconds
             <<" query_wall_s="<<seconds(querying)<<" wall_s="<<seconds(started)
             <<" peak_rss_bytes="<<guard.peak.load()<<'\n'
             <<"[OK] checkpointreadonly=yes production_T_used_as_F=no file_writes=none F=PROBE\n";
    return 0;
}
}
int main(int argc,char** argv)try{return catalog_lookup::run(argc,argv);}
catch(const std::exception& e){std::fprintf(stderr,"ERROR: %s\n",e.what());return 1;}
