// Isolated readonly full-index geometry-rekey diagnostic, not a value engine.
// The old index is freed before per-ID key replacement; no checkpoint writes.
#define GEOMETRY_REKEY_NO_MAIN
#include "layer_geometry_rekey_bench.cpp"

namespace geometry_rekey_full {
using geometry_rekey::Query;
using geometry_rekey::Target;
using missing_geometry_probe::Inventory;
using missing_geometry_probe::Canon;
using Clock=native_gather::Clock;
struct Options {
    geometry_rekey::Options small;
    unsigned threads=0,rounds=0;
    u64 rssGiB=0;
    bool readonly=false,ack=false,repair=false;
};
Options parse(int argc,char** argv) {
    if(argc<3)throw std::runtime_error("C input=... catalogue=... limit=... maxqueries=... maxseconds=... maxrssgib=... threads=... rounds=... checkpointreadonly [ack-large-c6 repair-l4-support]");
    Options o;o.small.c=std::stoi(argv[1]);
    for(int i=2;i<argc;++i) {
        const std::string word=argv[i];
        if(word=="checkpointreadonly"){o.readonly=true;continue;}
        if(word=="ack-large-c6"){o.ack=true;continue;}
        if(word=="repair-l4-support"){o.repair=true;continue;}
        const auto at=word.find('=');if(at==std::string::npos)throw std::runtime_error("option=value required");
        const auto key=word.substr(0,at),value=word.substr(at+1);
        if(key=="input")o.small.input=value;else if(key=="catalogue")o.small.catalogue=value;
        else if(key=="limit")o.small.limit=std::stoull(value);
        else if(key=="maxqueries")o.small.maxQueries=std::stoull(value);
        else if(key=="maxseconds")o.small.maxSeconds=std::stod(value);
        else if(key=="maxrssgib")o.rssGiB=std::stoull(value);
        else if(key=="threads")o.threads=std::stoul(value);
        else if(key=="rounds")o.rounds=std::stoul(value);
        else throw std::runtime_error("unknown option");
    }
    const auto& s=o.small;
    if((s.c!=5&&s.c!=6)||!o.readonly||s.input.empty()||s.catalogue.empty()||
       !s.limit||!s.maxQueries||s.maxQueries>5000000||!std::isfinite(s.maxSeconds)||s.maxSeconds<=0||
       !o.threads||o.threads>24||!o.rounds||o.rounds>5||!o.rssGiB||o.rssGiB>55||
       (s.c==5&&(s.limit!=17120||s.maxSeconds>120||o.rssGiB>1||o.threads>2||o.ack||o.repair))||
       (s.c==6&&(s.limit!=512||s.maxSeconds>270||!o.ack||!o.repair)))
        throw std::runtime_error("need complete C5 <=1GiB/120s/2threads or explicit C6 readonly+repair+ack limit512 <=55GiB/270s/24threads");
    return o;
}
struct Error {
    std::atomic<bool> failed{false};std::mutex mutex;std::string message;
    void set(const std::exception& e) {
        failed.store(true,std::memory_order_relaxed);std::lock_guard<std::mutex> lock(mutex);
        if(message.empty())message=e.what();
    }
    void check() {if(failed.load())throw std::runtime_error(message);}
};
std::string input_sha256(const std::string& path) {
    const u64 bytes=std::filesystem::file_size(path);
    if(!bytes||bytes>(4ULL<<20))throw std::runtime_error("bounded query text must be <=4MiB");
    std::ifstream input(path,std::ios::binary);
    if(!input)throw std::runtime_error("query text unavailable");
    std::string data(size_t(bytes),'\0');input.read(data.data(),std::streamsize(bytes));
    if(input.gcount()!=std::streamsize(bytes)||input.peek()!=EOF)
        throw std::runtime_error("query text changed during SHA read");
    shared_catalog::detail::Sha256 hash;hash.add(data.data(),data.size());return hash.finish();
}
struct Timed {u64 nodes=0,checksum=0,hits=0;double seconds=0;};

// kind0 establishes authoritative native IDs with the original canonicalizer.
// kind1 uses the released compatible prefix; kind2 uses the alternate key.
Timed query_pass(int kind,unsigned threads,const Inventory& inventory,Layer& layer,
                 std::vector<Query>& queries,std::vector<u32>& auts) {
    const auto began=Clock::now();Error error;u64 nodes=0,checksum=0,hits=0;
#pragma omp parallel num_threads(threads) reduction(+:nodes,checksum,hits)
    {
        Canon geometry(inventory);two_missing_prefix::Canon prefix;
#pragma omp for schedule(dynamic,1024)
        for(int64_t pos=0;pos<int64_t(queries.size());++pos) {
            if(error.failed.load(std::memory_order_relaxed))continue;
            try {
                Query& q=queries[size_t(pos)];u64 aut=0;State key;
                if(kind==0) {
                    const u64 before=tl_canon_nodes;key=canonize(q.raw,&aut);nodes+=tl_canon_nodes-before;
                } else if(kind==1) {key=prefix.strict(q.raw,&aut);nodes+=prefix.nodes;}
                else {key=geometry.run(q.raw,&aut);nodes+=geometry.nodes;}
                const u32 id=layer.find(key);
                if(id==UINT32_MAX||layer.stab[id]!=aut)
                    throw std::runtime_error("query missing or wrong stabilizer");
                if(kind==0){q.id=id;auts[size_t(pos)]=u32(aut);}
                else if(id!=q.id||aut!=auts[size_t(pos)])
                    throw std::runtime_error("query did not preserve original native ID and stabilizer");
                ++hits;checksum+=u64(id+1)*q.multiplicity;
            }catch(const std::exception& e){error.set(e);}
        }
    }
    error.check();
    if(hits!=queries.size())throw std::runtime_error("incomplete query phase");
    return {nodes,checksum,hits,native_gather::seconds(began)};
}

struct Rekey {u64 live=0,holes=0,changed=0,nodes=0;double seconds=0;};
Rekey rekey(unsigned threads,const Inventory& inventory,Layer& layer) {
    const auto began=Clock::now();
    // swap destroys the old table allocation before ANY key is changed.
    std::vector<u32>().swap(layer.table);layer.mask=0;
    Error error;u64 live=0,holes=0,changed=0,nodes=0;
#pragma omp parallel num_threads(threads) reduction(+:live,holes,changed,nodes)
    {
        Canon geometry(inventory);two_missing_prefix::Canon validator;
#pragma omp for schedule(dynamic,4096)
        for(int64_t pos=0;pos<int64_t(layer.size());++pos) {
            if(error.failed.load(std::memory_order_relaxed))continue;
            try {
                const u32 id=u32(pos),stored=layer.stab[id];
                if(!stored){++holes;continue;}
                // Explicit domain audit before entering the specialized key.
                if(!validator.prepare(layer.keys[id]))
                    throw std::runtime_error("source not a balanced unseeded two-missing state");
                u64 aut=0;const State key=geometry.run(layer.keys[id],&aut);
                if(aut!=stored)throw std::runtime_error("rekey changed stored stabilizer");
                if(!(key==layer.keys[id]))++changed;
                // This thread owns exactly this ID; all other data is immutable.
                layer.keys[id]=key;++live;nodes+=geometry.nodes;
            }catch(const std::exception& e){error.set(e);}
        }
    }
    error.check();
    if(live!=layer.real_size()||holes!=layer.holes)
        throw std::runtime_error("rekey live/hole census failed");
    return {live,holes,changed,nodes,native_gather::seconds(began)};
}

double rebuild(unsigned threads,Layer& layer) {
    if(!layer.table.empty())throw std::runtime_error("second index prohibited");
    const auto began=Clock::now();size_t slots=64;
    while(slots<2*layer.size())slots*=2;
    layer.table.assign(slots,0);layer.mask=slots-1;
    u64 duplicates=0,exhausted=0,live=0;
#pragma omp parallel for num_threads(threads) schedule(static) reduction(+:duplicates,exhausted,live)
    for(int64_t pos=0;pos<int64_t(layer.size());++pos) {
        const u32 id=u32(pos);if(!layer.stab[id])continue;
        const State& key=layer.keys[id];u64 at=state_hash(key)&layer.mask;bool placed=false;
        for(u64 probe=0;probe<slots;++probe) {
            u32 found=0;
            if(__atomic_compare_exchange_n(&layer.table[at],&found,id+1,false,__ATOMIC_ACQ_REL,__ATOMIC_ACQUIRE)) {
                placed=true;++live;break;
            }
            if(layer.keys[found-1]==key){++duplicates;placed=true;break;}
            at=(at+1)&layer.mask;
        }
        if(!placed)++exhausted;
    }
    if(duplicates||exhausted||live!=layer.real_size())
        throw std::runtime_error("rekey index duplicates/exhaustion/live-count failure");
    return native_gather::seconds(began);
}

void values_c5(const Layer& layer,const std::vector<Query>& queries,
               const std::vector<Target>& targets,const char* phase) {
    if(C!=5)return; // C6 never interprets the discarded zero T payload as F.
    std::vector<u128> sums(targets.size());
    for(const auto& q:queries)sums[q.target]+=u128(q.multiplicity)*layer.T[q.id];
    geometry_rekey::verify_values(sums,targets,phase);
}

int run(int argc,char** argv) {
    const Options o=parse(argc,argv);C=o.small.c;N2C=2*C;
    g_wlseed=false;g_forceWide=false;g_rehearsalDenom=0;
    FACT[0]=1;for(int i=1;i<=12;++i)FACT[i]=FACT[i-1]*i;
    const auto began=Clock::now();shared_catalog::Options bounds;
    bounds.maxSeconds=o.small.maxSeconds;bounds.maxResidentBytes=o.rssGiB<<30;
    shared_catalog::detail::LoadBudget watchdog(bounds);
    const u64 inputBytes=std::filesystem::file_size(o.small.input);
    const auto inputTime=std::filesystem::last_write_time(o.small.input);
    const std::string inputSha=input_sha256(o.small.input);
    if(C==6&&inputSha!="B535C873117F8583CA5069A16D2430767DAFC79FCFE6949325ABE9B5F853CE25")
        throw std::runtime_error("full C6 requires the audited uniform512 sample SHA");
    missing_geometry_probe::Budget smallBudget{began,std::min(120.0,o.small.maxSeconds),0};
    Inventory inventory;inventory.build(smallBudget);
    std::vector<Query> queries;queries.reserve(o.small.maxQueries);
    std::vector<Target> targets;targets.reserve(o.small.limit);
    const auto gatherStart=Clock::now();
    const u64 records=geometry_rekey::queries(o.small,queries,targets,smallBudget);
    if(std::filesystem::file_size(o.small.input)!=inputBytes||
       std::filesystem::last_write_time(o.small.input)!=inputTime)
        throw std::runtime_error("query sample changed during parsing");
    const double gatherSeconds=native_gather::seconds(gatherStart);
    if(C==6&&(records!=2691041||queries.size()!=2471101))
        throw std::runtime_error("full C6 diagnostic requires the pinned 512-source workload");
    std::vector<u32> auts(queries.size());
    shared_catalog::Result source;
    const auto loadStart=Clock::now();
    if(C==5) {
        source.sourcePath=std::filesystem::absolute(o.small.catalogue).lexically_normal().string();
        source.sourceBytes=std::filesystem::file_size(source.sourcePath);
        source.sourceModified=std::filesystem::last_write_time(source.sourcePath);
        source.layer=geometry_rekey::small_catalogue(o.small,smallBudget,source.sourceSha256);
    } else {
        shared_catalog::Options load=bounds;load.path=o.small.catalogue;
        load.threads=int(o.threads);load.expectedLayer=4;load.checkpointReadonly=true;load.repairC6=true;
        load.maxEntries=903398621ULL;
        source=shared_catalog::load(load);
    }
    const double loadSeconds=native_gather::seconds(loadStart);
    Layer& layer=source.layer;
    std::cout<<std::fixed<<std::setprecision(9)<<"LOAD source_SHA256="<<source.sourceSha256
             <<" input_SHA256="<<inputSha
             <<" ids="<<layer.size()<<" live="<<layer.real_size()<<" holes="<<layer.holes
             <<" repair="<<source.repairVersion<<" read_s="<<source.readSeconds
             <<" source_SHA_s="<<source.sha256Seconds<<" source_zero_s="<<source.zeroSeconds
             <<" native_index_s="<<source.indexSeconds<<" total_load_s="<<loadSeconds
             <<" C6_T=DISCARDED_NOT_F\n"<<std::flush;
    auto stamp=Clock::now();const std::string fingerprint=geometry_rekey::payload_identity(layer);
    const double fingerprintBefore=native_gather::seconds(stamp);
    const Timed reference=query_pass(0,o.threads,inventory,layer,queries,auts);
    std::cout<<"REFERENCE kind=ORIGINAL_NATIVE threads="<<o.threads<<" hits="<<reference.hits
             <<" checksum="<<reference.checksum<<" seconds="<<reference.seconds<<'\n'<<std::flush;
    auto passes=[&](int kind) {
        for(unsigned phase=0;phase<=o.rounds;++phase) {
            const unsigned threads=phase==0?1:o.threads;
            const Timed time=query_pass(kind,threads,inventory,layer,queries,auts);
            if(time.checksum!=reference.checksum)throw std::runtime_error("phase checksum mismatch");
            std::cout<<"QUERY kind="<<(kind==1?"COMPATIBLE_PREFIX":"GEOMETRY_RAM_KEY")
                     <<" threads="<<threads<<" pass="<<phase<<" hits="<<time.hits
                     <<" nodes="<<time.nodes<<" checksum="<<time.checksum<<" seconds="<<time.seconds
                     <<" per_query_ID_stab=EXACT\n"<<std::flush;
        }
    };
    passes(1);values_c5(layer,queries,targets,"ORIGINAL_NATIVE_IDS");
    const Rekey transformed=rekey(o.threads,inventory,layer);
    std::cout<<"REKEY live="<<transformed.live<<" holes="<<transformed.holes
             <<" changed="<<transformed.changed<<" nodes="<<transformed.nodes
             <<" seconds="<<transformed.seconds<<" per_ID_stab_audit=ALL\n"<<std::flush;
    const double indexSeconds=rebuild(o.threads,layer);
    stamp=Clock::now();
    if(geometry_rekey::payload_identity(layer)!=fingerprint)
        throw std::runtime_error("stable-ID values/stabilizers fingerprint changed");
    const double fingerprintAfter=native_gather::seconds(stamp);
    std::cout<<"INDEX rebuild_s="<<indexSeconds<<" duplicate_keys=0 simultaneous_indexes=1"
             <<" stable_ID_payload_SHA256="<<fingerprint<<" fingerprint_before_s="<<fingerprintBefore
             <<" fingerprint_after_s="<<fingerprintAfter<<'\n'<<std::flush;
    passes(2);values_c5(layer,queries,targets,"GEOMETRY_NATIVE_IDS");
    shared_catalog::verify_source_unchanged(source);
    if(std::filesystem::file_size(o.small.input)!=inputBytes||
       std::filesystem::last_write_time(o.small.input)!=inputTime)
        throw std::runtime_error("query sample changed during diagnostic");
    PROCESS_MEMORY_COUNTERS pm{};pm.cb=sizeof(pm);
    if(!GetProcessMemoryInfo(GetCurrentProcess(),&pm,sizeof(pm)))throw std::runtime_error("RSS unavailable");
    const u64 peak=std::max(watchdog.peak.load(),u64(pm.WorkingSetSize));
    const double elapsed=native_gather::seconds(began);
    if(peak>bounds.maxResidentBytes||elapsed>bounds.maxSeconds)throw std::runtime_error("final bound exceeded");
    std::cout<<"SUMMARY domain="<<(C==6?"FULL_C6_READONLY_LOOKUP_ONLY":"COMPLETE_C5_VALUE_CHAIN")
             <<" sources="<<targets.size()<<" labelled_matchings="<<records<<" weak_queries="<<queries.size()
             <<" native_IDs_preserved=ALL original_stabilizers_preserved=ALL key_rebuild_duplicates=0"
             <<" max_simultaneous_indexes=1 query_record_bytes="<<queries.size()*sizeof(Query)
             <<" extra_query_stabilizer_bytes="<<auts.size()*sizeof(u32)<<" gather_s="<<gatherSeconds
             <<" wall_s="<<elapsed<<" peak_bytes="<<peak<<" C6_F4_F5_VALUES=NOT_READ N6=NOT_COMPUTED\n"
             <<"GEOMETRY ONE INDEX REKEY DIFFERENTIAL PASSED\n";
    return 0;
}
} // namespace geometry_rekey_full
int main(int argc,char** argv) try{return geometry_rekey_full::run(argc,argv);}
catch(const std::exception& e){std::cerr<<"ERROR "<<e.what()<<'\n';return 1;}
