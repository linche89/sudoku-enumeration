// Resumable exact reverse L5 gather from a NEW, CLOSED native L4 snapshot.
// Original C6 rehearsal L5 weights are validated for integrity, then erased.
#define main layer_native_original_main_for_reverse_f5
#include "layer_dp_gate.cpp"
#undef main
#include "layer_shared_catalog.h"
#include "layer_shared_chunks.h" // Reuse exclusive atomic I/O, not F4 records.
#include "../../src/layer_two_missing_prefix_canon.h"
// Narrow compatible-key substitution only in the reverse numerical core.
// The header was compiled before this macro, so its native fallback cannot
// recurse. Loader/repair/source audits below retain the original canonize.
#define canonize two_missing_prefix::canonicalize
#include "layer_reverse_f5_core.h"
#undef canonize
#include "layer_reverse_f5_chunks.h"

namespace reverse_f5_run {
namespace fs = std::filesystem;
using Clock = std::chrono::steady_clock;
inline constexpr const char* supportSha256 =
    "A5FDDB22F8C79FD4DDC0015795558FBF1647F0DB556248F5C9551F2DB24891DF";
inline constexpr const char* repairVersion = "L5-disconnected-then-conference-v1";
std::atomic<bool> stopRequested{false};
BOOL WINAPI console_stop(DWORD event) {
    if (event == CTRL_C_EVENT || event == CTRL_BREAK_EVENT || event == CTRL_CLOSE_EVENT) {
        stopRequested.store(true); return TRUE;
    }
    return FALSE;
}
struct Options {
    std::string l4, l4sha256, support, output, exportPath, verify;
    u64 chunkSize = 0, limit = 0;
    int threads = 0, maxRssGiB = 0;
    double workSeconds = 0, maxSeconds = 0;
    bool readonly = false;
};
struct Guard {
    std::mutex mutex; std::condition_variable cv; bool done = false;
    std::thread worker; std::atomic<u64> peak{0};
    Guard(double maxSeconds, u64 maxBytes) {
        worker = std::thread([this,maxSeconds,maxBytes] {
            const auto began = Clock::now(); std::unique_lock<std::mutex> held(mutex);
            while (!cv.wait_for(held,std::chrono::milliseconds(100),[this]{return done;})) {
                PROCESS_MEMORY_COUNTERS pm{}; pm.cb = sizeof(pm);
                if (!GetProcessMemoryInfo(GetCurrentProcess(),&pm,sizeof(pm)) ||
                    pm.WorkingSetSize > maxBytes || shared_catalog::seconds(began) > maxSeconds) {
                    std::fprintf(stderr,"BOUND reverse F5 time/RSS; only prior immutable chunks survive\n");
                    std::fflush(nullptr); std::_Exit(124);
                }
                peak.store(std::max(peak.load(),u64(pm.PeakWorkingSetSize)));
            }
        });
    }
    ~Guard() { {std::lock_guard<std::mutex> held(mutex);done=true;} cv.notify_all();worker.join(); }
};
struct Loaded {
    Layer layer; CkptHeader source{};
    std::string path, sha256; u64 bytes = 0, mass = 0, repairHash = 0;
    fs::file_time_type modified{};
};
struct RehearsalScope {
    u64 original;
    explicit RehearsalScope(u64 denominator) : original(g_rehearsalDenom) { g_rehearsalDenom=denominator; }
    ~RehearsalScope() { g_rehearsalDenom=original; }
};
u64 group_order() { return (1ULL<<C)*FACT[C]; }
void unchanged(const Loaded& input) {
    if (fs::file_size(input.path)!=input.bytes || fs::last_write_time(input.path)!=input.modified)
        throw std::runtime_error("reverse F5 readonly input changed during this run");
}
std::wstring normalized(const fs::path& path) {
    auto result=fs::weakly_canonical(fs::absolute(path)).wstring();
    for (auto& ch:result) ch=wchar_t(std::towlower(ch));
    return result;
}
Options parse(int argc,char** argv) {
    if(argc<2) throw std::runtime_error("usage: C l4=NEW_CLOSED.snap l4sha256=HEX l5support=READONLY.snap output=NEW_NAMESPACE chunk=N limit=N threads=N workseconds=S maxseconds=S maxrssgib=G checkpointreadonly [export=NEW_L5.snap] [verify=COMPLETE_L5.txt]");
    C=std::stoi(argv[1]);N2C=2*C;Options o;
    for(int i=2;i<argc;++i) {
        const std::string arg=argv[i];
        if(arg=="checkpointreadonly") {o.readonly=true;continue;}
        const auto at=arg.find('=');if(at==std::string::npos)throw std::runtime_error("expected option=value");
        const auto k=arg.substr(0,at),v=arg.substr(at+1);
        if(k=="l4")o.l4=v;else if(k=="l4sha256")o.l4sha256=v;
        else if(k=="l5support")o.support=v;else if(k=="output")o.output=v;
        else if(k=="export")o.exportPath=v;else if(k=="verify")o.verify=v;
        else if(k=="chunk")o.chunkSize=std::stoull(v);else if(k=="limit")o.limit=std::stoull(v);
        else if(k=="threads")o.threads=std::stoi(v);else if(k=="maxrssgib")o.maxRssGiB=std::stoi(v);
        else if(k=="workseconds")o.workSeconds=std::stod(v);else if(k=="maxseconds")o.maxSeconds=std::stod(v);
        else throw std::runtime_error("unknown option "+k);
    }
    for(char& ch:o.l4sha256)ch=char(std::toupper(static_cast<unsigned char>(ch)));
    if((C!=5&&C!=6)||!o.readonly||o.l4.empty()||o.support.empty()||o.output.empty()||
        o.l4sha256.size()!=64||!reverse_f5_chunks::sha_shape(o.l4sha256.data())||
        !o.limit||!o.chunkSize||o.chunkSize>1000000||o.threads<1||o.threads>24||
        o.maxRssGiB<1||o.maxRssGiB>55||!std::isfinite(o.workSeconds)||!std::isfinite(o.maxSeconds)||
        o.workSeconds<=0||o.maxSeconds<o.workSeconds+5||o.maxSeconds>28800)
        throw std::runtime_error("require C5/C6, readonly inputs, mandatory L4 SHA, positive chunk/record/time/RSS bounds");
    const fs::path directory=fs::weakly_canonical(fs::absolute(o.output));
    if(directory==directory.root_path()||normalized(directory)==normalized(fs::current_path()))
        throw std::runtime_error("F5 namespace must be a dedicated subdirectory");
    for(const std::string& input:{o.l4,o.support}) {
        const auto source=normalized(input),dest=normalized(directory);
        if(source==dest||source.starts_with(dest+L"\\")||source.starts_with(dest+L"/"))
            throw std::runtime_error("readonly input cannot reside inside writable F5 namespace");
        if(!o.exportPath.empty()&&normalized(o.exportPath)==source)
            throw std::runtime_error("export cannot overwrite an input");
    }
    if(normalized(o.l4)==normalized(o.support))throw std::runtime_error("L4 and L5 sources must differ");
    if(!o.exportPath.empty()&&fs::exists(o.exportPath))throw std::runtime_error("export target already exists");
    return o;
}
void init_meta(Loaded& result,const std::string& path) {
    result.path=fs::absolute(path).lexically_normal().string();
    result.bytes=fs::file_size(result.path);result.modified=fs::last_write_time(result.path);
}
u64 index_slots(u64 capacity) {u64 slots=64;while(slots<2*capacity)slots<<=1;return slots;}
void preflight(const Options& o) {
    CkptHeader l4{},l5{};
    if(!ck_read_header(o.l4,l4))throw std::runtime_error("closed L4 header/configuration validation failed");
    {RehearsalScope scope(C==6?100:0);
     if(!ck_read_header(o.support,l5))throw std::runtime_error("L5 support header/configuration validation failed");}
    const u64 l4Live=C==6?903398603ULL:17120ULL;
    const u64 l5Live=C==6?96452753ULL:355ULL;
    if(l4.layerIdx!=4||l4.parentLayer||l4.nWide||l4.nEntries-l4.holes!=l4Live||
        l4.holes>1000000||l5.layerIdx!=5||l5.nWide||l5.nEntries-l5.holes!=l5Live||l5.holes>1000000)
        throw std::runtime_error("require plain CLOSED production L4 and full designated L5 support");
    if(C==5&&l5.parentLayer&&l5.cursorChunk!=l5.nChunks)
        throw std::runtime_error("small-C L5 support transition is not closed");
    if(C==6&&(l5.gen!=4||l5.nEntries!=96452974||l5.holes!=221||
        fs::file_size(o.support)!=3472307192ULL))
        throw std::runtime_error("C6 L5 support is not pinned rehearsal generation4");
    const u64 capacity5=l5.nEntries+(C==6?2:0);
    const u64 peak=(l4.nEntries+capacity5)*36+
        (index_slots(l4.nEntries)+index_slots(capacity5))*4+o.chunkSize*8+(256ULL<<20);
    if(peak>(u64(o.maxRssGiB)<<30))throw std::runtime_error("F5 input allocation exceeds explicit RSS budget");
    MEMORYSTATUSEX memory{};memory.dwLength=sizeof(memory);
    if(!GlobalMemoryStatusEx(&memory))throw std::runtime_error("cannot inspect free RAM");
    const u64 need=C==6?std::max(65ULL<<30,peak+(8ULL<<30)):peak;
    if(memory.ullAvailPhys<need)throw std::runtime_error("insufficient currently available RAM for F5 inputs");
    std::cout<<"PREFLIGHT estimated_peak_bytes="<<peak<<" free_bytes="<<memory.ullAvailPhys<<'\n'<<std::flush;
}
void install(CkptImage& image,Layer& layer,u64 capacity) {
    layer.keys=std::move(image.keys);layer.T=std::move(image.T);layer.stab=std::move(image.stab);
    layer.keys.resize(size_t(capacity));layer.T.resize(size_t(capacity),0);layer.stab.resize(size_t(capacity),0);
    layer.fixedCap=true;layer.claimed=u32(image.h.nEntries);layer.holes=u32(image.h.holes);
}
void audit_native(const State& state,int degree,u32 storedStab) {
    unsigned columns[12]{};
    for(int i=0;i<N2C;++i) {
        if((i&&state.m[i]<state.m[i-1])||(state.m[i]>>N2C))throw std::runtime_error("invalid native mask order/domain");
        unsigned used=0;
        for(int b=0;b<C;++b) {
            const unsigned field=fld(state.m[i],b);
            if(field==1)throw std::runtime_error("invalid native field1");
            if(field){++used;++columns[2*b+field-2];}
        }
        if(used!=unsigned(degree))throw std::runtime_error("native source row degree mismatch");
    }
    for(int i=N2C;i<12;++i)if(state.m[i])throw std::runtime_error("nonzero native padding");
    for(int i=0;i<N2C;++i)if(columns[i]!=unsigned(degree))throw std::runtime_error("native source slot imbalance");
    u64 actual=0;const State key=canonize(state,&actual);
    if(key!=state||actual!=storedStab)throw std::runtime_error("native canonical key/stabilizer mismatch");
}
u64 rebuild_index(Layer& layer,int threads,bool auditSmall,int degree) {
    const u64 slots=index_slots(layer.keys.size());layer.table.assign(size_t(slots),0);layer.mask=slots-1;
    const u64 group=group_order();u64 mass=0,holes=0,invalid=0,duplicates=0,full=0;
    std::atomic<bool> auditFailed{false};
#pragma omp parallel for schedule(static) num_threads(threads) reduction(+:mass,holes,invalid,duplicates,full)
    for(int64_t i=0;i<int64_t(layer.size());++i) {
        const u32 stab=layer.stab[size_t(i)];
        if(!stab){++holes;continue;}if(group%stab){++invalid;continue;}mass+=group/stab;
        if(auditSmall)try{audit_native(layer.keys[size_t(i)],degree,stab);}catch(...){auditFailed.store(true);}
        const State& key=layer.keys[size_t(i)];u64 at=state_hash(key)&layer.mask;bool placed=false;
        for(u64 tries=0;tries<slots;++tries) {
            u32 expected=0;
            if(__atomic_compare_exchange_n(&layer.table[at],&expected,u32(i)+1,false,__ATOMIC_RELAXED,__ATOMIC_RELAXED))
                {placed=true;break;}
            if(layer.keys[expected-1]==key){++duplicates;placed=true;break;}
            at=(at+1)&layer.mask;
        }
        if(!placed)++full;
    }
    if(invalid||duplicates||full||holes!=layer.holes||auditFailed.load())
        throw std::runtime_error("native input index uniqueness/mass/hole/semantic audit failed");
    return mass;
}
Loaded load_l4(const Options& o) {
    const auto began=Clock::now();Loaded result;init_meta(result,o.l4);CkptImage image;
    CkptHeader checked{};
    if(!ck_read_header(result.path,checked)||checked.layerIdx!=4||checked.parentLayer||checked.nWide||
        checked.nEntries-checked.holes!=(C==6?903398603ULL:17120ULL)||checked.holes>1000000)
        throw std::runtime_error("closed L4 changed or has invalid pre-allocation bounds");
    if(!ck_read_file(result.path,image)||std::memcmp(&checked,&image.h,sizeof(checked)))
        throw std::runtime_error("closed L4 native payload validation failed/source changed");
    result.source=image.h;result.sha256=shared_catalog::detail::image_sha256(image);
    if(result.sha256!=o.l4sha256)throw std::runtime_error("closed L4 full SHA-256 mismatch");
    if(image.h.layerIdx!=4||image.h.parentLayer||image.h.nWide)
        throw std::runtime_error("L4 source changed or is not a plain CLOSED snapshot");
    unchanged(result);install(image,result.layer,result.source.nEntries);
    result.mass=rebuild_index(result.layer,o.threads,C==5,4);
    const u64 expectedMass=C==6?41602261536160ULL:62185328ULL;
    const u64 expectedLive=C==6?903398603ULL:17120ULL;
    if(result.mass!=expectedMass||result.layer.real_size()!=expectedLive)
        throw std::runtime_error("closed L4 support count/orbit mass is incomplete");
    u64 invalid=0;const u64 group=group_order();
#pragma omp parallel for schedule(static) num_threads(o.threads) reduction(+:invalid)
    for(int64_t i=0;i<int64_t(result.layer.size());++i) {
        const u32 stab=result.layer.stab[size_t(i)];u64& value=result.layer.T[size_t(i)];
        if(!stab){if(value)++invalid;continue;}
        const u64 orbit=group/stab;
        if(!value||value%orbit){++invalid;continue;}value/=orbit;
        if(value%24)++invalid; // Free permutation of all four ordered colors.
    }
    if(invalid)throw std::runtime_error("L4 weighted T has zero live values/nonzero holes/nonintegral T/m/nonfactorial F4");
    unchanged(result);
    std::cout<<"CLOSED_L4 sha256="<<result.sha256<<" live="<<result.layer.real_size()
             <<" mass="<<result.mass<<" T_divided_by_orbit=yes duplicates=0 wall_s="
             <<shared_catalog::seconds(began)<<'\n'<<std::flush;
    return result;
}
Loaded load_support(const Options& o) {
    const auto began=Clock::now();Loaded result;init_meta(result,o.support);CkptImage image;
    {RehearsalScope scope(C==6?100:0);CkptHeader header{};
     if(!ck_read_header(result.path,header)||header.layerIdx!=5||header.nWide||header.holes>1000000||
         header.nEntries-header.holes!=(C==6?96452753ULL:355ULL)||
         (C==6&&(header.gen!=4||header.nEntries!=96452974||header.holes!=221||result.bytes!=3472307192ULL))||
         (C==5&&header.parentLayer&&header.cursorChunk!=header.nChunks))
         throw std::runtime_error("L5 support header/configuration/pre-allocation validation failed");
     if(!ck_read_file(result.path,image,size_t(header.nEntries+(C==6?2:0)))||
         std::memcmp(&header,&image.h,sizeof(header)))
         throw std::runtime_error("L5 support native payload validation failed/source changed");}
    result.source=image.h;result.sha256=shared_catalog::detail::image_sha256(image);
    if(C==6&&result.sha256!=supportSha256)throw std::runtime_error("C6 rehearsal support full SHA-256 mismatch");
    unchanged(result);
#pragma omp parallel for schedule(static) num_threads(o.threads)
    for(int64_t i=0;i<int64_t(image.T.size());++i)image.T[size_t(i)]=0;
    install(image,result.layer,result.source.nEntries+(C==6?2:0));
    result.mass=rebuild_index(result.layer,o.threads,C==5,5);
    if(result.mass!=(C==6?4439972138848ULL:589392ULL))throw std::runtime_error("source L5 orbit mass mismatch");
    if(C==6) {
        const native_gather::Graph witnesses[2]={
            {341,682,1109,1301,1349,1361,1364,2218,2602,2698,2722,2728},
            {421,602,1129,1364,1418,1574,1681,2198,2329,2402,2629,2728}};
        const u64 expectedStabs[2]={1440,240};
        result.repairHash=ck_hash64(repairVersion,std::strlen(repairVersion),CK_SEED);
        for(unsigned i=0;i<2;++i) {
            u64 stab=0;const State key=canonize(native_gather::encode(witnesses[i]),&stab);
            if(stab!=expectedStabs[i]||result.layer.find(key)!=UINT32_MAX)
                throw std::runtime_error("L5 support repair witness precondition failed");
            audit_native(key,5,u32(stab));
            const u32 id=result.layer.find_or_add_mt(key,u32(stab));
            if(id!=result.source.nEntries+i)throw std::runtime_error("L5 repair did not preserve stable IDs");
            result.repairHash=ck_hash64(&key,sizeof(key),result.repairHash);
            const u32 smallStab=u32(stab);result.repairHash=ck_hash64(&smallStab,sizeof(smallStab),result.repairHash);
            result.mass+=group_order()/stab;
        }
        if(result.layer.real_size()!=96452755||result.mass!=4439972139072ULL)
            throw std::runtime_error("repaired L5 support does not close exact count/mass");
        // L5 is traversed by stable ID; no L5 hash index is needed thereafter.
        std::vector<u32>().swap(result.layer.table);result.layer.mask=0;
    }
    unchanged(result);
    std::cout<<"L5_SUPPORT sha256="<<result.sha256<<" live="<<result.layer.real_size()
             <<" mass="<<result.mass<<" old_T=erased duplicates=0 repair_hash="<<result.repairHash
             <<" runtime_rehearsal_denom="<<g_rehearsalDenom<<" wall_s="<<shared_catalog::seconds(began)<<'\n'<<std::flush;
    return result;
}
void verify_values(const std::string& path,const Layer& layer) {
    if(path.empty())return;
    if(C!=5||layer.real_size()!=355)throw std::runtime_error("text exact F5 verification requires complete C5");
    std::ifstream input(path);if(!input)throw std::runtime_error("F5 exact verification text unavailable");
    native_gather::Input row;std::vector<bool> seen(layer.size(),false);u64 count=0;
    while(native_gather::next(input,5,row)) {
        const State key=canonize(native_gather::encode(row.graph));const u32 id=layer.find(key);
        if(!row.expected||id==UINT32_MAX||seen[id]||u128(layer.T[id])!=row.value)
            throw std::runtime_error("closed F5 differs from independent complete C5 reference");
        seen[id]=true;++count;
    }
    if(count!=355)throw std::runtime_error("F5 reference did not verify all355 native values");
    std::cout<<"EXACT_VERIFIED_F5_NATIVE_VALUES=355\n";
}
void export_layer(const Options& o,Layer& layer) {
    if(o.exportPath.empty())return;
    const fs::path target=fs::absolute(o.exportPath);
    if(fs::exists(target)||!fs::is_directory(target.parent_path()))
        throw std::runtime_error("export must be a NEW path in an existing directory");
    if(fs::space(target.parent_path()).available<128+u64(layer.size())*36+(1ULL<<30))
        throw std::runtime_error("insufficient free disk for native L5 export");
    u64 invalid=0;const u64 group=group_order();
#pragma omp parallel for schedule(static) num_threads(o.threads) reduction(+:invalid)
    for(int64_t i=0;i<int64_t(layer.size());++i) {
        if(!layer.stab[size_t(i)])continue;
        const u128 weighted=u128(group/layer.stab[size_t(i)])*layer.T[size_t(i)];
        if(!weighted||weighted>UINT64_MAX)++invalid;else layer.T[size_t(i)]=u64(weighted);
    }
    if(invalid)throw std::runtime_error("closed native T5 overflows u64 or is zero");
    if(g_rehearsalDenom||g_forceWide||g_wlseed)throw std::runtime_error("L5 export must use production configuration");
    const auto privatePath=shared_chunks::temporary_for(target);
    if(fs::exists(privatePath)||fs::exists(fs::path(privatePath.string()+".tmp")))
        throw std::runtime_error("private export path already exists");
    if(!layer_save_file(privatePath.string(),layer,5,nullptr))throw std::runtime_error("closed native L5 export failed");
    CkptImage reread;
    if(!ck_read_file(privatePath.string(),reread)||reread.h.parentLayer||reread.h.layerIdx!=5||reread.h.nWide||
        reread.keys.size()!=layer.size()||reread.h.holes!=layer.holes)
        throw std::runtime_error("native L5 export readonly roundtrip failed; private file retained");
    for(size_t i=0;i<layer.size();++i)
        if(reread.keys[i]!=layer.keys[i]||reread.T[i]!=layer.T[i]||reread.stab[i]!=layer.stab[i])
            throw std::runtime_error("native L5 export roundtrip payload differs; private file retained");
    const std::string sha=shared_catalog::detail::image_sha256(reread);
    if(!MoveFileExW(privatePath.c_str(),target.c_str(),MOVEFILE_WRITE_THROUGH))
        throw std::runtime_error("exclusive native L5 commit failed; private file retained");
    std::cout<<"EXPORTED_CLOSED_NATIVE_L5 "<<target.string()<<" sha256="<<sha
             <<" native_readonly_roundtrip=yes independent_S3_certificate_required=yes\n";
}
int run(int argc,char** argv) {
    const Options o=parse(argc,argv);const auto began=Clock::now();Guard guard(o.maxSeconds,u64(o.maxRssGiB)<<30);
    shared_chunks::Lock lock(o.output);SetConsoleCtrlHandler(console_stop,TRUE);
    g_wlseed=false;g_forceWide=false;g_rehearsalDenom=0;
    FACT[0]=1;for(int i=1;i<=12;++i)FACT[i]=FACT[i-1]*u64(i);omp_set_num_threads(o.threads);
    preflight(o);auto l4=load_l4(o);auto support=load_support(o);Layer& target=support.layer;
    const u64 n=target.size();
    if(std::min(o.chunkSize,n)>o.limit)throw std::runtime_error("limit must permit at least one whole F5 chunk");
    reverse_f5_chunks::Header lineage;lineage.c=u32(C);lineage.l4HeaderHash=l4.source.headerHash;
    lineage.supportHeaderHash=support.source.headerHash;lineage.supportRepairHash=support.repairHash;
    std::memcpy(lineage.l4Sha256,l4.sha256.data(),64);std::memcpy(lineage.supportSha256,support.sha256.data(),64);
    lineage.totalEntries=n;lineage.chunkSize=o.chunkSize;
    reverse_f5_chunks::open_namespace(o.output,lineage);
    std::vector<u64> values;u64 prefix=0,resumed=0,totalLive=0,checksum=0;bool gap=false;
    for(u64 start=0;start<n;start+=o.chunkSize) {
        const auto path=reverse_f5_chunks::filename(o.output,start);
        if(!fs::exists(path)){gap=true;continue;}
        if(gap)throw std::runtime_error("noncontiguous immutable F5 chunks; namespace needs audit");
        const auto header=reverse_f5_chunks::read_chunk(path,lineage,target,start,values);
        std::copy(values.begin(),values.end(),target.T.begin()+size_t(start));
        prefix=start+header.count;totalLive+=header.liveEntries;checksum+=header.valueChecksum;++resumed;
    }
    std::cout<<"RESUMED_F5 chunks="<<resumed<<" closed_prefix="<<prefix<<" total_entries="<<n<<'\n'<<std::flush;
    const auto computing=Clock::now();u64 added=0,chunks=0;
    while(prefix<n&&!stopRequested.load()&&shared_catalog::seconds(began)<o.workSeconds) {
        const u64 count=std::min(o.chunkSize,n-prefix);if(count>o.limit-added)break;
        if(fs::space(o.output).available<256+count*8+(64ULL<<20))throw std::runtime_error("insufficient chunk disk headroom");
        values.assign(size_t(count),0);std::atomic<bool> failed{false};std::mutex errorMutex;std::string error;
        u64 records=0,weak=0,nodes=0;double canon=0,lookup=0,enumeration=0,audit=0;
        const auto chunkBegan=Clock::now();
#pragma omp parallel num_threads(o.threads) reduction(+:records,weak,nodes,canon,lookup,enumeration,audit)
        {
            reverse_f5::Worker worker;
#pragma omp for schedule(dynamic,1)
            for(int64_t j=0;j<int64_t(count);++j) {
                const size_t id=size_t(prefix+u64(j));if(failed.load(std::memory_order_relaxed)||!target.stab[id])continue;
                try {
                    const auto stamp=Clock::now();audit_native(target.keys[id],5,target.stab[id]);audit+=shared_catalog::seconds(stamp);
                    const auto result=worker.process(target.keys[id],l4.layer);values[size_t(j)]=result.F5;
                    records+=result.diagnostics.labelledMatchings;weak+=result.diagnostics.weakResiduals;
                    nodes+=result.diagnostics.canonicalizationNodes;canon+=result.diagnostics.canonicalizationSeconds;
                    lookup+=result.diagnostics.lookupSeconds;enumeration+=result.diagnostics.enumerationSeconds;
                } catch(const std::exception& exception) {
                    failed.store(true,std::memory_order_relaxed);std::lock_guard<std::mutex> held(errorMutex);
                    if(error.empty())error=exception.what();
                }
            }
        }
        if(failed.load())throw std::runtime_error("uncommitted F5 chunk rejected: "+error);
        auto header=lineage;header.begin=prefix;header.count=count;
        reverse_f5_chunks::validate_values(target,header,values.data());
        header.payloadHash=ck_hash64(values.data(),values.size()*sizeof(u64),CK_SEED);
        reverse_f5_chunks::commit(reverse_f5_chunks::filename(o.output,prefix),header,values.data());
        std::copy(values.begin(),values.end(),target.T.begin()+size_t(prefix));
        prefix+=count;added+=count;++chunks;totalLive+=header.liveEntries;checksum+=header.valueChecksum;
        std::cout<<std::fixed<<std::setprecision(6)<<"CLOSED_F5_CHUNK begin="<<header.begin<<" count="<<count
                 <<" live="<<header.liveEntries<<" labelled_matchings="<<records<<" weak_residuals="<<weak
                 <<" canon_nodes="<<nodes<<" source_audit_cpu_s="<<audit<<" canon_cpu_s="<<canon
                 <<" lookup_cpu_s="<<lookup<<" enumeration_cpu_s="<<enumeration
                 <<" wall_s="<<shared_catalog::seconds(chunkBegan)<<" closed_prefix="<<prefix<<'/'<<n<<'\n'<<std::flush;
    }
    unchanged(l4);unchanged(support);
    if(prefix==n) {
        auto whole=lineage;whole.begin=0;whole.count=n;
        reverse_f5_chunks::validate_values(target,whole,target.T.data());
        if(whole.liveEntries!=totalLive||totalLive!=target.real_size()||whole.valueChecksum!=checksum)
            throw std::runtime_error("complete F5 catalogue closure totals mismatch");
        verify_values(o.verify,target);
        l4.layer=Layer{}; // Free the giant predecessor catalogue before export readback.
        export_layer(o,target);
    }
    PROCESS_MEMORY_COUNTERS pm{};pm.cb=sizeof(pm);
    if(!GetProcessMemoryInfo(GetCurrentProcess(),&pm,sizeof(pm)))throw std::runtime_error("cannot inspect final F5 RSS");
    std::cout<<std::fixed<<std::setprecision(6)<<"SUMMARY status="<<(prefix==n?"CLOSED_F5_CATALOGUE":"INCOMPLETE_RESUMABLE")
             <<" new_chunks="<<chunks<<" new_indices="<<added<<" closed_prefix="<<prefix<<" total_entries="<<n
             <<" live_closed="<<totalLive<<" F5_checksum_mod2_64="<<checksum
             <<" compute_wall_s="<<shared_catalog::seconds(computing)<<" total_wall_s="<<shared_catalog::seconds(began)
             <<" peak_rss_bytes="<<std::max(guard.peak.load(),u64(pm.PeakWorkingSetSize))
             <<" source_checkpointreadonly=yes N6=NOT_COMPUTED\n";
    return 0;
}
} // namespace reverse_f5_run
#ifndef REVERSE_F5_NO_MAIN
int main(int argc,char** argv)try{return reverse_f5_run::run(argc,argv);}
catch(const std::exception& error){std::fprintf(stderr,"ERROR: %s\n",error.what());return 1;}
#endif
